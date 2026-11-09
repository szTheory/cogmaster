/* The Sega PSG through the bus and against closed forms: cogmaster-test-psg
   <case>. Writes go in through the bus the Z80 uses, and the chip's registers
   and the trace log are read back through src/internal.h. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define TRACE 4096u
#define STEP 48u

#define STATE_BUF 393216u

static uint8_t image[IMAGE_SIZE];
static uint8_t state_a[STATE_BUF];
static uint8_t state_b[STATE_BUF];
static uint8_t state_c[STATE_BUF];
static int16_t pcm[COGMASTER__AUDIO_MAX_FRAMES * 2u];
static cogmaster_audio_record log_record[TRACE];

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* A new instance of the profile running an image of HALTs, with a trace log
   of TRACE records. */
static cogmaster *make(uint32_t machine, uint32_t region, uint32_t video)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cfg.trace_records = TRACE;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    memset(image, 0x76, sizeof image);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    media.region = region;
    media.tv = COGMASTER_TV_NTSC;
    media.video_mode = video;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

static cogmaster *export_sms(void)
{
    return make(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_SMS);
}

/* The records waiting in the log, oldest first; empties the log. */
static uint32_t drain_trace(cogmaster *m)
{
    cogmaster_audio_trace t;
    memset(&t, 0, sizeof t);
    t.size = (uint32_t)sizeof t;
    t.records = log_record;
    t.capacity = TRACE;
    CHECK_EQ_U64(cogmaster_get_audio_trace(m, &t), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.dropped, 0u);
    return t.written;
}

/* Runs one frame and returns the largest sample magnitude of its PCM. */
static uint32_t frame_peak(cogmaster *m)
{
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = pcm;
    a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    uint32_t peak = 0u;
    for (uint32_t i = 0u; i < a.written * a.channels; i++) {
        uint32_t mag = pcm[i] < 0 ? (uint32_t)-(int32_t)pcm[i] : (uint32_t)pcm[i];
        peak = mag > peak ? mag : peak;
    }
    return peak;
}

/* A Z80 OUT of v to port at tick: the bus is the one the CPU drives. */
static void out_at(cogmaster *m, uint64_t tick, uint16_t port, uint8_t v)
{
    m->ticks = tick;
    cogmaster__bus_out(m, port, v);
}

/* Tone 0 at period $0FE (254) and full volume, written through $7F at known
   ticks. One frame later the log holds source 0 changes exactly 254 steps (48
   ticks each) apart, and the PCM has left silence; a write to $7E reaches the
   chip as well, and a read of it is still the beam. */
static void test_through_bus(void)
{
    cogmaster *m = export_sms();
    out_at(m, 1000u, 0x7Fu, 0x8Eu);
    out_at(m, 1003u, 0x7Fu, 0x0Fu);
    out_at(m, 1006u, 0x7Fu, 0x90u);
    CHECK_EQ_U64(m->psg.period[0], 254u);
    CHECK_EQ_U64(m->psg.att[0], 0u);
    CHECK_EQ_U64(m->synth.stats.psg_writes, 3u);

    uint32_t peak = frame_peak(m);
    CHECK(peak > 1000u);

    uint32_t n = drain_trace(m);
    uint32_t seen = 0u;
    uint64_t last = 0u;
    uint32_t bad = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        if (log_record[i].source != 0u || log_record[i].tick <= 1006u) {
            continue;
        }
        if (seen > 0u) {
            bad += log_record[i].tick - last != 254u * STEP;
        }
        last = log_record[i].tick;
        seen++;
    }
    CHECK(seen >= 10u);
    CHECK_EQ_U64(bad, 0u);

    /* The other port in the range: tone 1 through $7E. */
    out_at(m, m->ticks, 0x7Eu, 0xA5u);
    CHECK_EQ_U64(m->psg.period[1], 5u);
    /* A read of $7E is the beam's counter and leaves the chip alone. */
    struct cogmaster__psg chip = m->psg;
    m->ticks += 3u;
    (void)cogmaster__bus_in(m, 0x7Eu);
    CHECK(memcmp(&chip, &m->psg, sizeof chip) == 0);
    cogmaster_destroy(m);
}

/* The levels a chip in its power-on state presents: tone channels hold their
   output high at period 0, the noise output bit is 0, every attenuation is
   off. */
static const uint32_t power_on_level[4] = {0x1Fu, 0x1Fu, 0x1Fu, 0x0Fu};

/* The records of the log: the snapshot a load or a reset leaves is one per
   active source, at tick 0, in source order. */
static void expect_snapshot(cogmaster *m, const char *what, uint32_t sources, uint32_t mask)
{
    uint32_t n = drain_trace(m);
    if (n != sources) {
        fprintf(stderr, "%s: %lu records, wanted %lu\n", what, (unsigned long)n,
                (unsigned long)sources);
        check_failures++;
        return;
    }
    for (uint32_t i = 0u; i < n; i++) {
        CHECK_EQ_U64(log_record[i].tick, 0u);
        CHECK_EQ_U64(log_record[i].source, i);
        CHECK_EQ_U64(log_record[i].level, i < 4u ? power_on_level[i] : mask);
    }
}

/* A load and both resets leave one record per active source at tick 0: four
   on a Master System, five on a Game Gear in native mode (the stereo mask),
   four on a Game Gear in Master System mode, where the port is not decoded.
   The synthesiser's stored levels are the chip's. */
static void test_snapshot(void)
{
    cogmaster *sms = export_sms();
    expect_snapshot(sms, "sms load", 4u, 0u);
    cogmaster *gg = make(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_GG);
    expect_snapshot(gg, "gg load", 5u, 0xFFu);
    cogmaster *gg_sms = make(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_SMS);
    expect_snapshot(gg_sms, "gg in sms mode load", 4u, 0u);
    cogmaster *each[3] = {sms, gg, gg_sms};
    const uint32_t count[3] = {4u, 5u, 4u};
    for (uint32_t i = 0u; i < 3u; i++) {
        for (uint32_t kind = COGMASTER_RESET_SOFT; kind <= COGMASTER_RESET_POWER; kind++) {
            out_at(each[i], 5000u, 0x7Fu, 0x90u);
            out_at(each[i], 5003u, 0x7Fu, 0xE5u);
            CHECK(drain_trace(each[i]) >= 1u);
            CHECK_EQ_U64(frame_peak(each[i]) <= 20000u, 1u);
            CHECK_EQ_U64(cogmaster_reset(each[i], kind), COGMASTER_STATUS_OK);
            expect_snapshot(each[i], "reset", count[i], i == 1u ? 0xFFu : 0u);
            for (uint32_t ch = 0u; ch < 4u; ch++) {
                CHECK_EQ_U64(each[i]->synth.level[ch], power_on_level[ch]);
            }
            CHECK_EQ_U64(each[i]->psg.at, 0u);
        }
        cogmaster_destroy(each[i]);
    }
}

/* One frame's PCM, taken as a host does; the cumulative count is checked on
   the cadence formula by the caller. */
static uint32_t take_pcm(cogmaster *m)
{
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = pcm;
    a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    return a.written;
}

static uint32_t peak_of(uint32_t n)
{
    uint32_t peak = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        uint32_t mag = pcm[i] < 0 ? (uint32_t)-(int32_t)pcm[i] : (uint32_t)pcm[i];
        peak = mag > peak ? mag : peak;
    }
    return peak;
}

/* A SOFT reset restarts the tick count, and the chip with it: its step
   origin returns to 0 (a stale origin would freeze it until the rewound count
   passed), the synthesiser's stored levels are the chip's again, the step to
   silence is heard in the first samples of the next frame, the sample count
   stays on the cadence, and the pre-reset volume written again is audible
   because the synthesiser knows the silence it was left at. */
static void test_reset_soft(void)
{
    cogmaster *m = export_sms();
    uint64_t frames = 0u, total = 0u;
    /* Tone 0 at period 1 holds its output high at full volume: a step up in
       the first frame, then a steady level that the DC estimator has mostly
       removed by the third. */
    out_at(m, 1000u, 0x7Fu, 0x81u);
    out_at(m, 1003u, 0x7Fu, 0x00u);
    out_at(m, 1006u, 0x7Fu, 0x90u);
    uint32_t before = 0u;
    for (uint32_t f = 0u; f < 3u; f++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
        frames++;
        uint32_t n = take_pcm(m);
        total += n;
        before = before > peak_of(n) ? before : peak_of(n);
    }
    CHECK(before > 4000u);
    CHECK(m->synth.gained[0][0] > 0u);
    (void)drain_trace(m);

    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    for (uint32_t ch = 0u; ch < 4u; ch++) {
        CHECK_EQ_U64(m->synth.level[ch], cogmaster__psg_level(m, ch));
        CHECK_EQ_U64(m->synth.gained[ch][0], 0u);
    }
    CHECK_EQ_U64(m->psg.at, 0u);

    /* The next frame, with no write: the held tone falls to silence at its
       start. */
    (void)drain_trace(m);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    frames++;
    total += take_pcm(m);
    CHECK_EQ_U64(total, frames * 3504512u / 4375u);
    int32_t fall = (int32_t)pcm[0] - (int32_t)pcm[15];
    int32_t later = (int32_t)pcm[16] - (int32_t)pcm[40];
    CHECK(fall > 4000);
    CHECK(later > -1500 && later < 1500);

    /* The chip steps from tick 0: tone 0 at period 5 toggles on the 48-tick
       grid in this frame. */
    uint64_t wrote = m->ticks + 40000u;
    out_at(m, wrote, 0x7Fu, 0x85u);
    out_at(m, wrote + 3u, 0x7Fu, 0x90u);
    (void)drain_trace(m);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    frames++;
    total += take_pcm(m);
    CHECK_EQ_U64(total, frames * 3504512u / 4375u);
    uint32_t n = drain_trace(m);
    uint32_t toggles = 0u, off_grid = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        if (log_record[i].source == 0u && log_record[i].tick > wrote + 3u) {
            toggles++;
            off_grid += log_record[i].tick % STEP != 0u;
        }
    }
    CHECK(toggles > 100u);
    CHECK_EQ_U64(off_grid, 0u);

    /* Writing the volume it had before the reset is heard: from silence the
       held tone steps up. */
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    (void)take_pcm(m);
    out_at(m, 2000u, 0x7Fu, 0x90u);
    uint32_t after = frame_peak(m);
    CHECK(after * 2u >= before);
    cogmaster_destroy(m);
}

/* Poked into a copy, a field the state refuses leaves the target as it was. */
static void expect_refused(cogmaster *src, cogmaster *target, const char *what)
{
    size_t size = cogmaster__state_size(src);
    size_t target_size = cogmaster__state_size(target);
    CHECK(size > 64u && size <= STATE_BUF && size == target_size);
    CHECK_EQ_U64(cogmaster__state_save(src, state_c, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_save(target, state_a, size), COGMASTER_STATUS_OK);
    if (cogmaster__state_load(target, state_c, size) == COGMASTER_STATUS_OK) {
        fprintf(stderr, "%s: accepted\n", what);
        check_failures++;
    }
    CHECK_EQ_U64(cogmaster__state_save(target, state_b, size), COGMASTER_STATUS_OK);
    if (memcmp(state_a, state_b, size) != 0) {
        fprintf(stderr, "%s: the refused load changed the instance\n", what);
        check_failures++;
    }
}

/* One extra write, identical for both instances. */
static void both_write(cogmaster *a, cogmaster *b, uint16_t port, uint8_t v)
{
    out_at(a, a->ticks + 20u, port, v);
    out_at(b, b->ticks + 20u, port, v);
}

/* An instance with tone 0 at period 254, tone 1 at 7, tone 2 at 3, a noise
   channel on white noise at the middle rate and every channel loud, run for two
   frames so every counter is part way through a count. */
static cogmaster *playing(void)
{
    cogmaster *m = export_sms();
    static const uint8_t bytes[] = {0x8Eu, 0x0Fu, 0x90u, 0xA7u, 0x00u, 0xB2u,
                                    0xC3u, 0x00u, 0xD4u, 0xE5u, 0xF1u};
    for (uint32_t i = 0u; i < sizeof bytes; i++) {
        out_at(m, 1000u + 3u * i, 0x7Fu, bytes[i]);
    }
    for (uint32_t f = 0u; f < 2u; f++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
        (void)take_pcm(m);
    }
    (void)drain_trace(m);
    return m;
}

/* A state saved between frames, with the chip part way through every count,
   loads into a second instance and both go on with the same records and the
   same PCM while the same bytes keep arriving. */
static void continue_between_frames(void)
{
    cogmaster *a = playing();
    cogmaster *b = export_sms();
    size_t size = cogmaster__state_size(a);
    CHECK(size > 64u && size <= STATE_BUF);
    CHECK_EQ_U64(cogmaster__state_save(a, state_a, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(b, state_a, size), COGMASTER_STATUS_OK);
    CHECK(memcmp(&a->psg, &b->psg, sizeof a->psg) == 0);
    (void)drain_trace(b);

    static cogmaster_audio_record copy[TRACE];
    static int16_t pcm_a[COGMASTER__AUDIO_MAX_FRAMES * 2u];
    uint32_t differing = 0u, loud = 0u, records = 0u;
    for (uint32_t f = 0u; f < 6u; f++) {
        both_write(a, b, 0x7Fu, (uint8_t)(0x90u + (f & 3u)));
        CHECK_EQ_U64(cogmaster_run_frame(a, NULL), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_run_frame(b, NULL), COGMASTER_STATUS_OK);
        uint32_t na = take_pcm(a);
        memcpy(pcm_a, pcm, sizeof pcm);
        uint32_t nb = take_pcm(b);
        CHECK_EQ_U64(na, nb);
        differing += memcmp(pcm_a, pcm, (size_t)na * sizeof pcm[0]) != 0;
        loud += peak_of(nb) > 500u;
        uint32_t ra = drain_trace(a);
        memcpy(copy, log_record, (size_t)ra * sizeof copy[0]);
        uint32_t rb = drain_trace(b);
        CHECK_EQ_U64(ra, rb);
        differing += memcmp(copy, log_record, (size_t)ra * sizeof copy[0]) != 0;
        records += ra;
    }
    CHECK_EQ_U64(differing, 0u);
    CHECK_EQ_U64(loud, 6u);
    CHECK(records > 1000u);
    CHECK(memcmp(&a->psg, &b->psg, sizeof a->psg) == 0);
    cogmaster_destroy(a);
    cogmaster_destroy(b);
}

/* A state saved part way through a frame, after the chip's lazy origin has
   fallen behind the tick count, restores the chip exactly: the records that
   follow are the same. (The synthesiser saves only what is pending between
   frames, so the PCM of that one frame is not compared.) */
static void continue_mid_frame(void)
{
    cogmaster *a = playing();
    cogmaster *b = export_sms();
    out_at(a, a->ticks + 20u, 0x7Fu, 0x82u);
    for (uint32_t i = 0u; i < 500u; i++) {
        cogmaster__z80_step(a);
    }
    CHECK(a->ticks < a->bus.deadline);
    CHECK(a->ticks - a->psg.at > 1000u);
    size_t size = cogmaster__state_size(a);
    CHECK(size > 64u && size <= STATE_BUF);
    (void)drain_trace(a);
    CHECK_EQ_U64(cogmaster__state_save(a, state_a, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(b, state_a, size), COGMASTER_STATUS_OK);
    CHECK(memcmp(&a->psg, &b->psg, sizeof a->psg) == 0);
    (void)drain_trace(b);
    CHECK_EQ_U64(cogmaster_run_frame(a, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_run_frame(b, NULL), COGMASTER_STATUS_OK);
    static cogmaster_audio_record copy[TRACE];
    uint32_t ra = drain_trace(a);
    memcpy(copy, log_record, (size_t)ra * sizeof copy[0]);
    uint32_t rb = drain_trace(b);
    CHECK_EQ_U64(ra, rb);
    CHECK(ra > 100u);
    CHECK(memcmp(copy, log_record, (size_t)ra * sizeof copy[0]) == 0);
    CHECK(memcmp(&a->psg, &b->psg, sizeof a->psg) == 0);
    cogmaster_destroy(a);
    cogmaster_destroy(b);
}

/* Every chip field the state refuses, one at a time, is refused with the
   instance unchanged, and the unpoked state loads. */
static void refuse_chip_fields(void)
{
    cogmaster *bad = playing();
    cogmaster *target = export_sms();
    size_t size = cogmaster__state_size(bad);
    CHECK_EQ_U64(cogmaster__state_save(bad, state_a, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(target, state_a, size), COGMASTER_STATUS_OK);
    struct cogmaster__psg good = bad->psg;

    bad->psg.lfsr = 0u;
    expect_refused(bad, target, "lfsr 0");
    bad->psg = good;
    bad->psg.period[1] = 1024u;
    expect_refused(bad, target, "period 1024");
    bad->psg = good;
    bad->psg.counter[2] = 1024u;
    expect_refused(bad, target, "tone counter 1024");
    bad->psg = good;
    bad->psg.counter[3] = 65u;
    expect_refused(bad, target, "noise counter 65");
    bad->psg = good;
    bad->psg.att[0] = 16u;
    expect_refused(bad, target, "attenuation 16");
    bad->psg = good;
    bad->psg.out[1] = 2u;
    expect_refused(bad, target, "flip-flop 2");
    bad->psg = good;
    bad->psg.noise = 8u;
    expect_refused(bad, target, "noise control 8");
    bad->psg = good;
    bad->psg.noise_div = 2u;
    expect_refused(bad, target, "noise divider 2");
    bad->psg = good;
    bad->psg.latch = 8u;
    expect_refused(bad, target, "latch 8");
    bad->psg = good;
    bad->psg.at = bad->ticks + STEP;
    expect_refused(bad, target, "step origin after the tick count");
    bad->psg = good;
    bad->psg.at = good.at + 1u;
    expect_refused(bad, target, "step origin off the 48 tick grid");
    bad->psg = good;
    bad->psg.at = 0u;
    expect_refused(bad, target, "step origin a frame behind");
    bad->psg = good;
    CHECK_EQ_U64(cogmaster__state_save(bad, state_c, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(target, state_c, size), COGMASTER_STATUS_OK);
    cogmaster_destroy(bad);
    cogmaster_destroy(target);
}

static void test_state_continue(void)
{
    continue_between_frames();
    continue_mid_frame();
    refuse_chip_fields();
}

/* A chip, and its trace log, back at power-on. */
static void power_on(cogmaster *m)
{
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
    (void)drain_trace(m);
}

static void put(cogmaster *m, uint64_t tick, uint8_t v)
{
    cogmaster__psg_write(m, tick, v);
}

/* The latch byte for channel ch's tone or noise control (type 0) or its
   attenuation (type 1), with four data bits. */
static uint8_t latch(uint32_t ch, uint32_t type, uint32_t data)
{
    return (uint8_t)(0x80u | ch << 5 | type << 4 | (data & 15u));
}

/* A tone period of n on channel ch, written as the two bytes a game writes: a
   latch byte with the low four bits, then a data byte with the high six. */
static void put_period(cogmaster *m, uint64_t tick, uint32_t ch, uint32_t n)
{
    put(m, tick, latch(ch, 0u, n));
    put(m, tick, (uint8_t)(n >> 4));
}

/* Records of source ch after tick: how many, and how many are not exactly
   spacing ticks after the one before. */
static uint32_t spaced(uint32_t n, uint32_t ch, uint64_t after, uint64_t spacing, uint32_t *bad)
{
    uint32_t seen = 0u;
    uint64_t last = 0u;
    *bad = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        if (log_record[i].source != ch || log_record[i].tick <= after) {
            continue;
        }
        if (seen > 0u) {
            *bad += log_record[i].tick - last != spacing;
        }
        last = log_record[i].tick;
        seen++;
    }
    return seen;
}

/* A tone of period n on channel ch at full volume: from the first toggle on,
   its records are exactly 48 n ticks apart (the chip steps every 48 ticks and
   a period of n toggles every n steps). A period of 0 or 1 holds the output
   high: it leaves no record after the volume write. */
static void check_period(cogmaster *m, uint32_t ch, uint32_t n)
{
    power_on(m);
    put_period(m, 100u, ch, n);
    put(m, 100u, latch(ch, 1u, 0u));
    cogmaster__psg_catch_up(m, 100u + 48u * (5u * (n > 1u ? n : 1u) + 4u));
    uint32_t count = drain_trace(m);
    uint32_t bad = 0u;
    uint32_t seen = spaced(count, ch, 100u, 48u * n, &bad);
    if (n < 2u) {
        CHECK_EQ_U64(seen, 0u);
        CHECK_EQ_U64(cogmaster__psg_level(m, ch), 0x10u);
    } else {
        CHECK(seen >= 4u);
        CHECK_EQ_U64(bad, 0u);
    }
    if (seen < 4u && n >= 2u) {
        fprintf(stderr, "period %lu on channel %lu: %lu records\n", (unsigned long)n,
                (unsigned long)ch, (unsigned long)seen);
    }
}

/* All 1,024 periods on tone 0, and a spread of them on tones 1 and 2. */
static void test_periods(void)
{
    cogmaster *m = export_sms();
    for (uint32_t n = 0u; n < 1024u; n++) {
        check_period(m, 0u, n);
    }
    static const uint16_t spread[] = {0u, 1u, 2u, 3u, 7u, 254u, 1023u};
    for (uint32_t ch = 1u; ch < 3u; ch++) {
        for (uint32_t i = 0u; i < sizeof spread / sizeof spread[0]; i++) {
            check_period(m, ch, spread[i]);
        }
    }
    cogmaster_destroy(m);
}

/* round(32767 x 10^(-i / 10)) for i below 15 and 0 at 15: the step ratio is
   10^(-1/10), which is 213,225,862 / 2^28 to nine digits, applied to a Q20
   value, then rounded. Every unrounded value is at least 0.04 from a half, so
   the nine digits decide nothing. */
static uint32_t law(uint32_t i)
{
    if (i >= 15u) {
        return 0u;
    }
    uint64_t x = (uint64_t)32767u << 20;
    for (uint32_t k = 0u; k < i; k++) {
        x = x * 213225862u >> 28;
    }
    return (uint32_t)((x + (1u << 19)) >> 20);
}

/* Every attenuation on every channel: the record is output << 4 | attenuation
   (the tone channels hold their output high at period 0, the noise output is
   0), a tone's gained level is the law through the gain, and the PCM step is
   that level within the kernel's overshoot and its settling. */
static void test_volumes(void)
{
    CHECK_EQ_U64(law(0u), 32767u);
    CHECK_EQ_U64(law(7u), 6538u);
    CHECK_EQ_U64(law(15u), 0u);
    cogmaster *m = export_sms();
    for (uint32_t ch = 0u; ch < 4u; ch++) {
        for (uint32_t att = 0u; att < 16u; att++) {
            power_on(m);
            put(m, 100u, latch(ch, 1u, att));
            uint32_t out = ch < 3u ? 1u : 0u;
            CHECK_EQ_U64(cogmaster__psg_level(m, ch), out << 4 | att);
            uint32_t n = drain_trace(m);
            if (att == 15u) {
                CHECK_EQ_U64(n, 0u);
            } else if (n == 1u) {
                CHECK_EQ_U64(log_record[0].tick, 100u);
                CHECK_EQ_U64(log_record[0].source, ch);
                CHECK_EQ_U64(log_record[0].level, out << 4 | att);
            } else {
                CHECK_EQ_U64(n, 1u);
            }
            uint32_t gained = out != 0u ? (law(att) * 840u + 2048u) / 4096u : 0u;
            CHECK_EQ_U64(m->synth.gained[ch][0], gained);
            if (ch < 3u) {
                uint32_t peak = frame_peak(m);
                CHECK(peak * 100u >= gained * 90u);
                CHECK(peak * 100u <= gained * 109u);
            }
        }
    }
    cogmaster_destroy(m);
}

#define SHIFTS 20u

/* Steps the chip from power-on one step at a time with noise control control
   (and, for control 3, a tone 2 period of tone2) and notes the step each shift
   happened at, and the register after it, for the first SHIFTS shifts. */
static void run_noise(cogmaster *m, uint32_t control, uint32_t tone2, uint32_t steps[SHIFTS],
                      uint16_t after[SHIFTS])
{
    power_on(m);
    put_period(m, 0u, 2u, tone2);
    put(m, 0u, latch(3u, 1u, 0u));
    put(m, 0u, latch(3u, 0u, control));
    CHECK_EQ_U64(m->psg.lfsr, 0x8000u);
    uint32_t found = 0u;
    uint16_t seen = m->psg.lfsr;
    for (uint32_t k = 1u; found < SHIFTS && k < 20000u; k++) {
        cogmaster__psg_catch_up(m, 48u * k);
        if (m->psg.lfsr != seen) {
            seen = m->psg.lfsr;
            steps[found] = k;
            after[found] = seen;
            found++;
        }
    }
    CHECK_EQ_U64(found, SHIFTS);
}

/* The shift register after n shifts from $8000. Shifted right, a 1 enters at
   the top when the bit shifted out is 1 (periodic noise), or when bit 0 and bit
   3 differ (white noise): so the lone bit walks down, and white noise first
   differs when it reaches bit 3, at the 13th shift. */
static uint16_t lfsr_after(uint32_t n, int white)
{
    if (n <= 12u) {
        return (uint16_t)(0x8000u >> n);
    }
    if (!white) {
        return (uint16_t)(0x8000u >> (n % 16u));
    }
    static const uint16_t tail[3] = {0x8004u, 0x4002u, 0x2001u};
    return tail[n - 13u];
}

/* The eight noise controls: bits 1 and 0 pick the shift interval (32, 64 or 128
   steps, or tone 2's rising edge) and bit 2 white or periodic feedback. The
   first shift is on the first step; the register walks as lfsr_after says; the
   noise channel's output is the bit shifted out, first high at the 16th shift.
   With tone 2 as the clock, a period of 0 acts as 1. */
static void check_noise(cogmaster *m, uint32_t control, uint32_t tone2)
{
    static uint32_t steps[SHIFTS];
    static uint16_t after[SHIFTS];
    uint32_t mode = control & 3u;
    int white = (control & 4u) != 0u;
    uint32_t interval = mode < 3u ? 32u << mode : 2u * (tone2 > 1u ? tone2 : 1u);
    run_noise(m, control, tone2, steps, after);
    uint32_t bad_step = 0u, bad_state = 0u;
    for (uint32_t j = 0u; j < SHIFTS; j++) {
        bad_step += steps[j] != 1u + j * interval;
        bad_state += after[j] != lfsr_after(j + 1u, white) && j < (white ? 15u : SHIFTS);
    }
    CHECK_EQ_U64(bad_step, 0u);
    CHECK_EQ_U64(bad_state, 0u);

    power_on(m);
    put_period(m, 0u, 2u, tone2);
    put(m, 0u, latch(3u, 1u, 0u));
    put(m, 0u, latch(3u, 0u, control));
    (void)drain_trace(m);
    cogmaster__psg_catch_up(m, 48u * (1u + 15u * interval));
    uint32_t n = drain_trace(m);
    uint32_t high = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        if (log_record[i].source == 3u && log_record[i].level == 0x10u) {
            high++;
            CHECK_EQ_U64(log_record[i].tick, 48u * (1u + 15u * interval));
        }
    }
    CHECK_EQ_U64(high, 1u);
}

static void test_noise_controls(void)
{
    cogmaster *m = export_sms();
    for (uint32_t control = 0u; control < 8u; control++) {
        if ((control & 3u) != 3u) {
            check_noise(m, control, 0u);
        } else {
            static const uint16_t clock[] = {0u, 1u, 3u, 10u};
            for (uint32_t i = 0u; i < sizeof clock / sizeof clock[0]; i++) {
                check_noise(m, control, clock[i]);
            }
        }
    }
    cogmaster_destroy(m);
}

/* From $8000 the 16 bit register with taps on bits 0 and 3 comes back to
   $8000 after exactly 57,337 shifts; periodic noise after 16. Tone 2 at period
   0 clocks the noise every second step, which keeps the run short. */
static uint32_t shifts_to_return(cogmaster *m, uint32_t control)
{
    power_on(m);
    put(m, 0u, latch(3u, 0u, control));
    uint32_t shifts = 0u;
    uint16_t seen = m->psg.lfsr;
    for (uint32_t k = 1u; k < 400000u; k++) {
        cogmaster__psg_catch_up(m, 48u * k);
        if (m->psg.lfsr != seen) {
            seen = m->psg.lfsr;
            shifts++;
            if (seen == 0x8000u) {
                return shifts;
            }
        }
    }
    return 0u;
}

static void test_lfsr_period(void)
{
    cogmaster *m = export_sms();
    CHECK_EQ_U64(shifts_to_return(m, 7u), 57337u);
    CHECK_EQ_U64(shifts_to_return(m, 3u), 16u);
    cogmaster_destroy(m);
}

/* The write protocol (SMS-AUDIO section 2.1): a latch byte names a register and
   gives four bits, a data byte after it goes to the same register, and the
   latch stays put. */
static void test_protocol(void)
{
    cogmaster *m = export_sms();
    power_on(m);
    /* Power-on latches register 3, the volume of channel 1. */
    put(m, 0u, 0x05u);
    CHECK_EQ_U64(m->psg.att[1], 5u);

    /* A tone: the latch byte holds the low four bits, a data byte the high six,
       and a second data byte changes the high bits only. */
    put(m, 0u, 0x8Eu);
    CHECK_EQ_U64(m->psg.period[0], 0x0Eu);
    put(m, 0u, 0x0Fu);
    CHECK_EQ_U64(m->psg.period[0], 0xFEu);
    put(m, 0u, 0x01u);
    CHECK_EQ_U64(m->psg.period[0], 0x1Eu);
    put(m, 0u, 0x3Fu);
    CHECK_EQ_U64(m->psg.period[0], 0x3FEu);
    put(m, 0u, 0x8Au);
    CHECK_EQ_U64(m->psg.period[0], 0x3FAu);

    /* A volume: a data byte after the volume latch rewrites the volume, from its
       low four bits, and the latch persists. */
    put(m, 0u, 0x95u);
    CHECK_EQ_U64(m->psg.att[0], 5u);
    put(m, 0u, 0x03u);
    CHECK_EQ_U64(m->psg.att[0], 3u);
    put(m, 0u, 0x3Fu);
    CHECK_EQ_U64(m->psg.att[0], 15u);
    put(m, 0u, 0x02u);
    CHECK_EQ_U64(m->psg.att[0], 2u);
    CHECK_EQ_U64(m->psg.period[0], 0x3FAu);

    /* Another latch moves the target; the tone period of channel 2 and the
       attenuation of channel 3 are reached the same way. */
    put(m, 0u, latch(2u, 0u, 9u));
    put(m, 0u, 0x12u);
    CHECK_EQ_U64(m->psg.period[2], 0x129u);
    put(m, 0u, latch(3u, 1u, 7u));
    put(m, 0u, 0x0Cu);
    CHECK_EQ_U64(m->psg.att[3], 12u);
    CHECK_EQ_U64(m->psg.period[2], 0x129u);
    CHECK_EQ_U64(m->psg.att[0], 2u);

    /* The noise control, by latch byte or by data byte, resets the shift
       register to $8000; a noise volume write does not. Three bits are kept. */
    put(m, 0u, latch(3u, 0u, 6u));
    CHECK_EQ_U64(m->psg.noise, 6u);
    cogmaster__psg_catch_up(m, 48u * 40u);
    CHECK(m->psg.lfsr != 0x8000u);
    put(m, 48u * 40u, 0x7Cu);
    CHECK_EQ_U64(m->psg.noise, 4u);
    CHECK_EQ_U64(m->psg.lfsr, 0x8000u);
    cogmaster__psg_catch_up(m, 48u * 120u);
    CHECK(m->psg.lfsr != 0x8000u);
    uint16_t dirty = m->psg.lfsr;
    put(m, 48u * 120u, latch(3u, 1u, 2u));
    CHECK_EQ_U64(m->psg.lfsr, dirty);
    put(m, 48u * 120u, latch(3u, 0u, 1u));
    CHECK_EQ_U64(m->psg.lfsr, 0x8000u);
    CHECK_EQ_U64(m->psg.noise, 1u);
    cogmaster_destroy(m);
}

/* A write stamped exactly on a step tick is applied after that step; one tick
   earlier it is applied before. The tone's counter shows which: the step at 48
   reloads it from the period it finds, and the records show the order. */
static void test_tie_rule(void)
{
    cogmaster *m = export_sms();
    power_on(m);
    put_period(m, 10u, 0u, 2u);
    put(m, 10u, latch(0u, 1u, 0u));
    (void)drain_trace(m);
    put(m, 48u, latch(0u, 1u, 5u));
    CHECK_EQ_U64(m->psg.at, 48u);
    uint32_t n = drain_trace(m);
    CHECK_EQ_U64(n, 2u);
    if (n == 2u) {
        /* The step's toggle, then the write. */
        CHECK_EQ_U64(log_record[0].tick, 48u);
        CHECK_EQ_U64(log_record[0].level, 0x10u);
        CHECK_EQ_U64(log_record[1].tick, 48u);
        CHECK_EQ_U64(log_record[1].level, 0x15u);
    }

    /* The period written at 48 met a step that had already reloaded from the
       old period 0; one tick earlier it is the new one that is reloaded. */
    power_on(m);
    put(m, 48u, latch(0u, 0u, 5u));
    CHECK_EQ_U64(m->psg.at, 48u);
    CHECK_EQ_U64(m->psg.counter[0], 0u);
    power_on(m);
    put(m, 47u, latch(0u, 0u, 5u));
    CHECK_EQ_U64(m->psg.at, 0u);
    cogmaster__psg_catch_up(m, 48u);
    CHECK_EQ_U64(m->psg.counter[0], 5u);
    cogmaster_destroy(m);
}

/* Two instances written the same way. */
static cogmaster *configured(void)
{
    cogmaster *m = export_sms();
    put_period(m, 1000u, 0u, 5u);
    put(m, 1000u, latch(0u, 1u, 0u));
    put_period(m, 1100u, 1u, 7u);
    put(m, 1100u, latch(1u, 1u, 3u));
    put(m, 1200u, latch(3u, 1u, 0u));
    put(m, 1200u, latch(3u, 0u, 5u));
    return m;
}

/* The frame end runs the chip to the frame's deadline, inclusive: after the
   second frame, whose deadline 358,416 is 7,467 steps, the last step is at the
   deadline, and no step is taken past it. An empty frame leaves no record of a
   tone channel (periods 0 and 1 are silent). Catching up in one call or in many
   gives the same chip and the same records, and a frame's records are those of
   catching the chip up to its deadline by hand. */
static void test_frame_flush(void)
{
    cogmaster *m = export_sms();
    (void)drain_trace(m);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    uint32_t n = drain_trace(m);
    for (uint32_t i = 0u; i < n; i++) {
        CHECK(log_record[i].source >= 3u);
    }
    CHECK_EQ_U64(m->synth.stats.psg_steps, 3733u);
    CHECK_EQ_U64(m->psg.at, 3733u * 48u);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->synth.stats.psg_steps, 7467u);
    CHECK_EQ_U64(m->psg.at, 358416u);
    CHECK_EQ_U64(m->bus.deadline, 3u * 179208u);
    cogmaster_destroy(m);

    cogmaster *one = configured();
    cogmaster *many = configured();
    (void)drain_trace(one);
    (void)drain_trace(many);
    cogmaster__psg_catch_up(one, 60000u);
    uint64_t t = 1200u;
    uint32_t lcg = 12345u;
    while (t < 60000u) {
        lcg = lcg * 1103515245u + 12345u;
        t += 1u + (lcg >> 16) % 200u;
        cogmaster__psg_catch_up(many, t < 60000u ? t : 60000u);
    }
    static cogmaster_audio_record first[TRACE];
    uint32_t ra = drain_trace(one);
    memcpy(first, log_record, (size_t)ra * sizeof first[0]);
    uint32_t rb = drain_trace(many);
    CHECK(ra > 300u);
    CHECK_EQ_U64(ra, rb);
    CHECK(memcmp(first, log_record, (size_t)ra * sizeof first[0]) == 0);
    CHECK(memcmp(&one->psg, &many->psg, sizeof one->psg) == 0);
    cogmaster_destroy(one);
    cogmaster_destroy(many);

    /* A frame run through the bus, and the same writes and a catch-up by hand. */
    cogmaster *bus = export_sms();
    cogmaster *hand = export_sms();
    (void)drain_trace(bus);
    (void)drain_trace(hand);
    static const struct {
        uint64_t tick;
        uint8_t v;
    } writes[] = {{1000u, 0x85u},  {1003u, 0x00u},  {1006u, 0x90u},  {50000u, 0xA3u},
                  {50003u, 0x00u}, {50006u, 0xB1u}, {90000u, 0xE5u}, {90003u, 0xF0u}};
    for (size_t i = 0u; i < sizeof writes / sizeof writes[0]; i++) {
        out_at(bus, writes[i].tick, 0x7Fu, writes[i].v);
        put(hand, writes[i].tick, writes[i].v);
    }
    CHECK_EQ_U64(cogmaster_run_frame(bus, NULL), COGMASTER_STATUS_OK);
    cogmaster__psg_catch_up(hand, 179208u);
    ra = drain_trace(bus);
    memcpy(first, log_record, (size_t)ra * sizeof first[0]);
    rb = drain_trace(hand);
    CHECK(ra > 300u);
    CHECK_EQ_U64(ra, rb);
    CHECK(memcmp(first, log_record, (size_t)ra * sizeof first[0]) == 0);
    CHECK(memcmp(&bus->psg, &hand->psg, sizeof bus->psg) == 0);
    cogmaster_destroy(bus);
    cogmaster_destroy(hand);
}

/* Runs one frame of a stereo instance and returns the largest sample magnitude
   of each output, left then right, and how many sample frames have the two
   outputs different. */
static uint32_t stereo_frame(cogmaster *m, uint32_t peak[2])
{
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = pcm;
    a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.channels, 2u);
    uint32_t apart = 0u;
    peak[0] = 0u;
    peak[1] = 0u;
    for (uint32_t i = 0u; i < a.written; i++) {
        for (uint32_t ch = 0u; ch < 2u; ch++) {
            int32_t v = pcm[i * 2u + ch];
            uint32_t mag = (uint32_t)(v < 0 ? -v : v);
            peak[ch] = mag > peak[ch] ? mag : peak[ch];
        }
        apart += pcm[i * 2u] != pcm[i * 2u + 1u];
    }
    return apart;
}

/* PSG channel ch at full volume and sounding at once: a tone at period 1 holds
   its output high, so volume alone makes a step; the noise channel at control 0
   raises its output at the 16th shift, 24,576 ticks in, well inside a frame. */
static void sound_channel(cogmaster *m, uint64_t tick, uint32_t ch)
{
    if (ch < 3u) {
        out_at(m, tick, 0x7Fu, (uint8_t)(0x81u | (ch << 5)));
        out_at(m, tick + 3u, 0x7Fu, 0x00u);
    } else {
        out_at(m, tick, 0x7Fu, 0xE0u);
    }
    out_at(m, tick + 6u, 0x7Fu, (uint8_t)(0x90u | (ch << 5)));
}

/* The Game Gear's $06 in native mode: bit n + 4 sends channel n to the left
   and bit n to the right. The write catches the chip up and leaves one record
   for source 4 at its own tick. $FF, $0F, $F0 and $00 sound both outputs, the
   right one, the left one and neither; each of the eight bits alone sounds
   exactly the output it names for exactly the channel it names. */
static void test_gg_routing(void)
{
    static const struct {
        uint8_t mask;
        uint32_t left, right;
    } whole[] = {{0xFFu, 1u, 1u}, {0x0Fu, 0u, 1u}, {0xF0u, 1u, 0u}, {0x00u, 0u, 0u}};
    for (size_t k = 0u; k < sizeof whole / sizeof whole[0]; k++) {
        cogmaster *m = make(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_GG);
        (void)drain_trace(m);
        out_at(m, 1000u, 0x06u, whole[k].mask);
        CHECK_EQ_HEX(m->io.gg[5], whole[k].mask);
        /* The chip was caught up to the write, and the record is the mask. */
        CHECK_EQ_U64(m->psg.at, 1000u / STEP * STEP);
        uint32_t n = drain_trace(m);
        uint32_t source4 = 0u;
        for (uint32_t i = 0u; i < n; i++) {
            source4 += log_record[i].source == 4u;
        }
        CHECK_EQ_U64(source4, 1u);
        CHECK(n >= 1u);
        if (n >= 1u) {
            CHECK_EQ_U64(log_record[n - 1u].source, 4u);
            CHECK_EQ_U64(log_record[n - 1u].tick, 1000u);
            CHECK_EQ_HEX(log_record[n - 1u].level, whole[k].mask);
        }
        sound_channel(m, 2000u, 0u);
        uint32_t peak[2];
        (void)stereo_frame(m, peak);
        CHECK_EQ_U64(peak[0] > 1000u, whole[k].left);
        CHECK_EQ_U64(peak[1] > 1000u, whole[k].right);
        CHECK(whole[k].left != 0u || peak[0] == 0u);
        CHECK(whole[k].right != 0u || peak[1] == 0u);
        cogmaster_destroy(m);
    }
    for (uint32_t bit = 0u; bit < 8u; bit++) {
        cogmaster *m = make(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_GG);
        out_at(m, 1000u, 0x06u, (uint8_t)(1u << bit));
        sound_channel(m, 2000u, bit & 3u);
        uint32_t peak[2];
        (void)stereo_frame(m, peak);
        uint32_t side = bit >= 4u ? 0u : 1u;
        CHECK(peak[side] > 1000u);
        CHECK_EQ_U64(peak[1u - side], 0u);
        /* The other three channels are routed nowhere by this mask. */
        for (uint32_t ch = 0u; ch < 4u; ch++) {
            if (ch != (bit & 3u)) {
                sound_channel(m, m->ticks + 1000u, ch);
            }
        }
        (void)stereo_frame(m, peak);
        CHECK_EQ_U64(peak[1u - side], 0u);
        cogmaster_destroy(m);
    }
}

/* In Master System video mode a Game Gear does not decode $06: the write
   records no source 4, leaves the mask at its power-on value, and every sample
   of the two outputs is the same, as when the mask is $FF. */
static void test_gg_port06_sms_mode(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_SMS);
    (void)drain_trace(m);
    out_at(m, 1000u, 0x06u, 0xF0u);
    CHECK_EQ_U64(drain_trace(m), 0u);
    CHECK_EQ_HEX(m->io.gg[5], 0xFF);
    sound_channel(m, 2000u, 0u);
    uint32_t peak[2];
    uint32_t apart = stereo_frame(m, peak);
    CHECK(peak[0] > 1000u);
    CHECK_EQ_U64(peak[0], peak[1]);
    CHECK_EQ_U64(apart, 0u);
    uint32_t n = drain_trace(m);
    for (uint32_t i = 0u; i < n; i++) {
        CHECK(log_record[i].source < 4u);
    }
    cogmaster_destroy(m);

    /* The control: the same writes in native mode split the two outputs. */
    m = make(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_GG);
    out_at(m, 1000u, 0x06u, 0xF0u);
    sound_channel(m, 2000u, 0u);
    apart = stereo_frame(m, peak);
    CHECK(apart > 100u);
    CHECK_EQ_U64(peak[1], 0u);
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"through_bus", test_through_bus},
    {"snapshot", test_snapshot},
    {"reset_soft", test_reset_soft},
    {"state_continue", test_state_continue},
    {"periods", test_periods},
    {"volumes", test_volumes},
    {"noise_controls", test_noise_controls},
    {"lfsr_period", test_lfsr_period},
    {"protocol", test_protocol},
    {"tie_rule", test_tie_rule},
    {"frame_flush", test_frame_flush},
    {"gg_routing", test_gg_routing},
    {"gg_port06_sms_mode", test_gg_port06_sms_mode},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-psg <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-psg: unknown case %s\n", argv[1]);
    return 2;
}
