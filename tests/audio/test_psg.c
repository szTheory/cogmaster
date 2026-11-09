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
    out_at(m, 40000u, 0x7Fu, 0x85u);
    out_at(m, 40003u, 0x7Fu, 0x90u);
    (void)drain_trace(m);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    frames++;
    total += take_pcm(m);
    CHECK_EQ_U64(total, frames * 3504512u / 4375u);
    uint32_t n = drain_trace(m);
    uint32_t toggles = 0u, off_grid = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        if (log_record[i].source == 0u && log_record[i].tick > 40003u) {
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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"through_bus", test_through_bus},
    {"snapshot", test_snapshot},
    {"reset_soft", test_reset_soft},
    {"state_continue", test_state_continue},
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
