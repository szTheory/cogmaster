/* The audio cadence and the host's drain, through the public API over the
   built-in test card: cogmaster-test-audio <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define POISON 0x5A5A
#define FRAMES 1000u

static int16_t pcm[COGMASTER__AUDIO_MAX_FRAMES * 2u + 4u];

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

static cogmaster_media media_for(uint32_t machine, uint32_t tv, uint32_t video)
{
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = machine;
    media.tv = tv;
    media.video_mode = video;
    return media;
}

/* A new instance with the test card of the profile loaded. */
static cogmaster *make(uint32_t machine, uint32_t tv, uint32_t video)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    cogmaster_media media = media_for(machine, tv, video);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

/* These tests set the synthesiser's levels directly, so the sound chip is
   parked at its last step of the frame: its own steps would write over the
   levels set here. The chip has its own tests. */
static void frame(cogmaster *m)
{
    m->psg.at = (m->frame + 1u) * cogmaster__frame_ticks(m) / 48u * 48u;
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
}

/* One drain call into the shared buffer, which is poisoned first. */
static cogmaster_status drain(cogmaster *m, cogmaster_audio *a, uint32_t capacity)
{
    for (size_t i = 0; i < sizeof pcm / sizeof pcm[0]; i++) {
        pcm[i] = POISON;
    }
    memset(a, 0, sizeof *a);
    a->size = (uint32_t)sizeof *a;
    a->samples = pcm;
    a->capacity = capacity;
    return cogmaster_get_audio(m, a);
}

/* Runs one frame and takes all of it; returns the sample frames delivered. */
static uint32_t take_frame(cogmaster *m)
{
    cogmaster_audio a;
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    return a.written;
}

/* Frames F on the exact cadence: floor(F x num / den). */
static uint64_t expected_total(uint64_t frames, uint64_t num, uint64_t den)
{
    return frames * num / den;
}

/* Every frame holds short or short + 1 sample frames and the running total
   after each frame is on the exact formula, with the long frames counted. */
static void cadence_run(uint32_t tv, uint32_t shortest, uint64_t num, uint64_t den,
                        uint32_t long_frames)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, tv, COGMASTER_VIDEO_SMS);
    uint64_t total = 0u;
    uint32_t longs = 0u, bad = 0u, off_formula = 0u;
    for (uint32_t f = 1u; f <= FRAMES; f++) {
        uint32_t n = take_frame(m);
        total += n;
        longs += n == shortest + 1u;
        bad += n != shortest && n != shortest + 1u;
        off_formula += total != expected_total(f, num, den);
    }
    CHECK_EQ_U64(bad, 0u);
    CHECK_EQ_U64(off_formula, 0u);
    CHECK_EQ_U64(longs, long_frames);
    cogmaster_destroy(m);
}

static void test_cadence(void)
{
    cadence_run(COGMASTER_TV_NTSC, 801u, 3504512u, 4375u, 31u);
    cadence_run(COGMASTER_TV_PAL, 965u, 685094400u, 709379u, 766u);
}

/* Nothing before the first frame; a zero capacity needs no buffer; two
   partial reads return one frame; the rest of a frame the host never took is
   counted; a missing buffer is refused. */
static void drain_pcm(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    cogmaster_audio a;
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 0u);
    CHECK_EQ_U64(a.channels, 1u);
    CHECK_EQ_U64(pcm[0], (uint16_t)POISON);

    frame(m);
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 0u);
    CHECK_EQ_U64(a.channels, 1u);
    a.capacity = 1u;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_ERR_ARGUMENT);

    CHECK_EQ_U64(drain(m, &a, 400u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 400u);
    CHECK_EQ_U64(pcm[399], 0u);
    CHECK_EQ_U64(pcm[400], (uint16_t)POISON);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 401u);
    CHECK_EQ_U64(pcm[400], 0u);
    CHECK_EQ_U64(pcm[401], (uint16_t)POISON);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 0u);
    CHECK_EQ_U64(a.dropped, 0u);

    /* Two frames with one drain between: the first frame's 801 are lost. */
    frame(m);
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 801u);
    CHECK_EQ_U64(a.dropped, 801u);

    CHECK_EQ_U64(cogmaster_get_audio(NULL, &a), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_audio(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    a.size = COGMASTER__FIRST_SIZE_AUDIO - 1u;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_ERR_STRUCT_SIZE);
    cogmaster_destroy(m);
}

/* A reload restarts the cadence and drops what was pending; a refused load
   changes nothing; a SOFT reset keeps the cadence and the pending samples; a
   POWER reset restarts both. */
static void drain_loads(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    cogmaster_audio a;
    cogmaster_media media =
        media_for(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    for (uint32_t i = 0u; i < 20u; i++) {
        (void)take_frame(m);
    }
    frame(m);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 0u);
    uint64_t total = 0u;
    for (uint32_t i = 0u; i < 32u; i++) {
        total += take_frame(m);
    }
    CHECK_EQ_U64(total, expected_total(32u, 3504512u, 4375u));

    /* A refused load: the 801 pending samples and the phase stay. */
    frame(m);
    media.machine = 99u;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_PROFILE_REJECTED);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written,
                 expected_total(33u, 3504512u, 4375u) - expected_total(32u, 3504512u, 4375u));
    cogmaster_destroy(m);

    /* SOFT keeps the total on the formula across the reset and the samples
       not yet taken; POWER starts again. */
    m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    total = 0u;
    for (uint32_t i = 0u; i < 20u; i++) {
        total += take_frame(m);
    }
    frame(m);
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    total += a.written;
    CHECK_EQ_U64(a.written, 801u);
    for (uint32_t i = 0u; i < 11u; i++) {
        total += take_frame(m);
    }
    CHECK_EQ_U64(total, expected_total(32u, 3504512u, 4375u));
    frame(m);
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 0u);
    total = 0u;
    for (uint32_t i = 0u; i < 32u; i++) {
        total += take_frame(m);
    }
    CHECK_EQ_U64(total, expected_total(32u, 3504512u, 4375u));
    cogmaster_destroy(m);
}

/* The Game Gear delivers left and right pairs in either video mode. */
static void drain_channels(void)
{
    static const uint32_t video[] = {COGMASTER_VIDEO_GG, COGMASTER_VIDEO_SMS};
    for (size_t i = 0u; i < 2u; i++) {
        cogmaster *m = make(COGMASTER_MACHINE_GG, COGMASTER_TV_NTSC, video[i]);
        cogmaster_audio a;
        frame(m);
        CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(a.channels, 2u);
        CHECK_EQ_U64(a.written, 801u);
        CHECK_EQ_U64(pcm[801u * 2u - 1u], 0u);
        CHECK_EQ_U64(pcm[801u * 2u], (uint16_t)POISON);
        cogmaster_destroy(m);
    }
}

/* The allocator the trace tests count with: allocations so far, and a size
   that fails when it is asked for. */
static unsigned allocations;
static size_t refuse_size;

static void *counting_alloc(void *user, size_t size)
{
    (void)user;
    if (size == refuse_size) {
        return NULL;
    }
    allocations++;
    return malloc(size);
}

static cogmaster *make_traced(uint32_t trace_records, uint32_t config_size, cogmaster_status want)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = config_size;
    cfg.allocator.alloc = counting_alloc;
    cfg.allocator.free = test_free;
    cfg.trace_records = trace_records;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), want);
    return m;
}

static cogmaster_audio_record log_record[8];

static cogmaster_status drain_log(cogmaster *m, cogmaster_audio_trace *t, uint32_t capacity)
{
    memset(log_record, 0xA5, sizeof log_record);
    memset(t, 0, sizeof *t);
    t->size = (uint32_t)sizeof *t;
    t->records = log_record;
    t->capacity = capacity;
    return cogmaster_get_audio_trace(m, t);
}

/* A load and each reset leave the sound chips' snapshot in the log: one
   record per PSG source at tick 0, nothing else (the appends before it are
   gone, and nothing was lost). */
static void expect_snapshot_only(cogmaster *m)
{
    cogmaster_audio_trace t;
    CHECK_EQ_U64(drain_log(m, &t, 8u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written, 4u);
    CHECK_EQ_U64(t.dropped, 0u);
    for (uint32_t i = 0u; i < 4u; i++) {
        CHECK_EQ_U64(log_record[i].tick, 0u);
        CHECK_EQ_U64(log_record[i].source, i);
    }
}

static void append(cogmaster *m, uint32_t first, uint32_t count)
{
    for (uint32_t i = first; i < first + count; i++) {
        cogmaster__synth_trace(m, 1000u + i, i % 7u, 100u + i);
    }
}

/* The log is the host's: its size is capped at create, allocated once at load,
   drained oldest first with every loss counted, and emptied by a load or a
   reset. */
static void drain_trace(void)
{
    const uint32_t first = (uint32_t)(4u * sizeof(void *));
    cogmaster_audio_trace t;
    cogmaster_media media =
        media_for(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);

    /* The cap: one past a million records is refused, a million is not; a
       config at its first size reads no later field, so it keeps no log. */
    CHECK(make_traced((1u << 20) + 1u, (uint32_t)sizeof(cogmaster_config),
                      COGMASTER_STATUS_ERR_ARGUMENT) == NULL);
    cogmaster *m = make_traced(1u << 20, (uint32_t)sizeof(cogmaster_config), COGMASTER_STATUS_OK);
    cogmaster_destroy(m);
    m = make_traced(5u, first, COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain_log(m, &t, 4u), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_destroy(m);

    /* Asked for none, there is nothing to drain. */
    m = make_traced(0u, (uint32_t)sizeof(cogmaster_config), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain_log(m, &t, 4u), COGMASTER_STATUS_ERR_ARGUMENT);
    append(m, 0u, 1u);
    cogmaster_destroy(m);

    /* A log of four: before a load there is no content; a failed allocation
       is ERR_NO_MEMORY and the instance stays unloaded. */
    refuse_size = 4u * sizeof(cogmaster_audio_record);
    m = make_traced(4u, (uint32_t)sizeof(cogmaster_config), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain_log(m, &t, 4u), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_NO_MEMORY);
    CHECK_EQ_U64(drain_log(m, &t, 4u), COGMASTER_STATUS_ERR_NO_CONTENT);
    cogmaster_destroy(m);
    refuse_size = 0u;
    m = make_traced(4u, (uint32_t)sizeof(cogmaster_config), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    unsigned after_load = allocations;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(allocations, after_load);
    expect_snapshot_only(m);

    /* Overflow: six appends into four slots keep the first four in order and
       count two dropped. */
    append(m, 0u, 6u);
    CHECK_EQ_U64(drain_log(m, &t, 8u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written, 4u);
    CHECK_EQ_U64(t.dropped, 2u);
    for (uint32_t i = 0u; i < 4u; i++) {
        CHECK_EQ_U64(log_record[i].tick, 1000u + i);
        CHECK_EQ_U64(log_record[i].source, i % 7u);
        CHECK_EQ_U64(log_record[i].level, 100u + i);
    }
    CHECK_EQ_HEX(log_record[4].level, 0xA5A5A5A5u);
    CHECK_EQ_U64(drain_log(m, &t, 8u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written + t.dropped, 0u);

    /* A small capacity copies in order and counts the rest; zero capacity
       needs no buffer and counts all; a NULL buffer with capacity is refused. */
    append(m, 0u, 4u);
    CHECK_EQ_U64(drain_log(m, &t, 3u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written, 3u);
    CHECK_EQ_U64(t.dropped, 1u);
    CHECK_EQ_U64(log_record[2].level, 102u);
    CHECK_EQ_HEX(log_record[3].level, 0xA5A5A5A5u);
    append(m, 0u, 2u);
    t.capacity = 2u;
    t.records = NULL;
    CHECK_EQ_U64(cogmaster_get_audio_trace(m, &t), COGMASTER_STATUS_ERR_ARGUMENT);
    t.capacity = 0u;
    CHECK_EQ_U64(cogmaster_get_audio_trace(m, &t), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written, 0u);
    CHECK_EQ_U64(t.dropped, 2u);
    CHECK_EQ_U64(drain_log(m, &t, 8u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written + t.dropped, 0u);

    /* A load and both resets empty the log and its drop count, then leave
       the snapshot. */
    append(m, 0u, 6u);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    expect_snapshot_only(m);
    for (uint32_t kind = COGMASTER_RESET_SOFT; kind <= COGMASTER_RESET_POWER; kind++) {
        append(m, 0u, 6u);
        CHECK_EQ_U64(cogmaster_reset(m, kind), COGMASTER_STATUS_OK);
        expect_snapshot_only(m);
    }

    /* The checks every call makes. */
    CHECK_EQ_U64(cogmaster_get_audio_trace(NULL, &t), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_audio_trace(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    t.size = COGMASTER__FIRST_SIZE_AUDIO_TRACE - 1u;
    CHECK_EQ_U64(cogmaster_get_audio_trace(m, &t), COGMASTER_STATUS_ERR_STRUCT_SIZE);
    cogmaster_destroy(m);
}

static void test_drain(void)
{
    drain_pcm();
    drain_loads();
    drain_channels();
    drain_trace();
}

/* The sample index a change at tick falls in during the first frame of an
   NTSC instance whose cadence phase is 0. */
static uint32_t index_of(uint64_t tick)
{
    return (uint32_t)(tick * COGMASTER__AUDIO_INC_NTSC / COGMASTER__AUDIO_MOD_NTSC);
}

/* Every kernel row sums to 32,768, and the table's largest taps are the ones
   the accumulation bound in synth.c was derived from. */
static void kernel_table(void)
{
    int64_t tap_max[COGMASTER__SYNTH_TAPS];
    memset(tap_max, 0, sizeof tap_max);
    int64_t biggest = 0;
    for (uint32_t row = 0u; row < 32u; row++) {
        int64_t sum = 0;
        for (uint32_t k = 0u; k < COGMASTER__SYNTH_TAPS; k++) {
            int64_t v = cogmaster__synth_kernel(row, k);
            sum += v;
            int64_t mag = v < 0 ? -v : v;
            tap_max[k] = mag > tap_max[k] ? mag : tap_max[k];
            biggest = mag > biggest ? mag : biggest;
        }
        CHECK_EQ_U64((uint64_t)sum, 32768u);
    }
    /* The partner of row 31 is row 0 one tap to the right. */
    int64_t tap_sum = 0;
    for (uint32_t k = 1u; k < COGMASTER__SYNTH_TAPS; k++) {
        int64_t v = cogmaster__synth_kernel(0u, k - 1u);
        tap_max[k] = (v < 0 ? -v : v) > tap_max[k] ? (v < 0 ? -v : v) : tap_max[k];
    }
    for (uint32_t k = 0u; k < COGMASTER__SYNTH_TAPS; k++) {
        tap_sum += tap_max[k];
    }
    CHECK_EQ_U64((uint64_t)tap_sum, 70842u);
    CHECK_EQ_U64((uint64_t)biggest, 28452u);
}

/* A level change is a band-limited, DC-blocked step: nothing before its
   sample, the rise through the kernel, the full level 16 samples on less the
   estimator's decay (a 256th of the gap a sample), then the slow decay itself.
   The silent card stays exactly silent. */
static void test_step(void)
{
    kernel_table();
    const uint64_t tick = 20000u;
    const uint32_t at = index_of(tick);
    const int32_t level = 6720; /* attenuation 0 at the Master System gain */
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    cogmaster__synth_level(m, tick, 0u, 0x10u);
    cogmaster_audio a;
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 801u);
    uint32_t before = 0u;
    for (uint32_t i = 0u; i <= at; i++) {
        before += pcm[i] != 0;
    }
    CHECK_EQ_U64(before, 0u);
    int32_t peak = 0;
    for (uint32_t i = at; i < at + 16u; i++) {
        peak = pcm[i] > peak ? pcm[i] : peak;
    }
    /* The kernel overshoots its final value by under 9 percent. */
    CHECK(peak > level && peak < level + level / 11);
    CHECK(pcm[at + 4u] > -level / 20 && pcm[at + 4u] < level / 20);
    CHECK(pcm[at + 9u] > level * 9 / 10);
    int32_t full = pcm[at + 16u];
    CHECK(full > level * 93 / 100 && full < level * 98 / 100);
    int32_t late = pcm[at + 16u + 256u];
    int32_t decayed = full * 3678 / 10000; /* (255 / 256) to the 256th */
    CHECK(late > decayed - level / 100 && late < decayed + level / 100);
    CHECK_EQ_U64(m->synth.stats.clamps, 0u);
    CHECK_EQ_U64(m->synth.stats.kernel_apps, 16u);
    CHECK_EQ_U64(m->synth.stats.max_index, at + 16u);
    cogmaster_destroy(m);

    /* No change, no sound: the card is zero sample for sample. */
    m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    uint32_t nonzero = 0u;
    for (uint32_t f = 0u; f < 4u; f++) {
        frame(m);
        CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
        for (uint32_t i = 0u; i < a.written; i++) {
            nonzero += pcm[i] != 0;
        }
    }
    CHECK_EQ_U64(nonzero, 0u);
    cogmaster_destroy(m);
}

/* ---- Tones driven straight into the synth interface ---- */

/* A square wave on one source: a change every half ticks, alternating the
   source between off and on_level. */
struct tone {
    uint64_t next; /* tick of the next change */
    uint32_t half;
    uint32_t source;
    uint32_t on_level;
    uint32_t on;
};

/* Emits the changes that fall in the frame about to run. */
static void tone_frame(cogmaster *m, struct tone *t)
{
    uint64_t end = (m->frame + 1u) * cogmaster__frame_ticks(m);
    while (t->next < end) {
        t->on ^= 1u;
        cogmaster__synth_level(m, t->next, t->source, t->on != 0u ? t->on_level : 0u);
        t->next += t->half;
    }
}

#define PSG_FULL 0x10u   /* output high, attenuation 0 */
#define STEP_HEIGHT 6720 /* that level through the Master System gain */
#define MINUS_ONE_DBFS 29204

/* ---- The spectral proof: an integer DFT, identical on every platform ---- */

#define DFT_M 8192u
#define DFT_BINS 2730u /* 16 kHz at 48 kHz over M */
#define DFT_SKIP 4096u
#define DFT_FRAMES 16u
#define Q30 (INT64_C(1) << 30)

static int32_t trig_cos[DFT_M], trig_sin[DFT_M];
static int32_t window[DFT_M];
static int64_t windowed[DFT_M];
static int16_t tone_pcm[DFT_FRAMES * 802u];

static uint64_t isqrt_u64(uint64_t v)
{
    uint64_t r = 0u, bit = UINT64_C(1) << 62;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0u) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

/* Q30 cosine and sine of 2 pi k / M. Eleven anchors come from halving the
   angle from a quarter turn with an integer square root (cosine of half an
   angle, then the sine from the angle-doubling relation); an entry is the
   product of the anchors of its set low bits, then whole quarter turns for
   bits 11 and 12. Every product is at most 2^60 and a two-term sum 2^61. */
static void build_trig(void)
{
    int64_t anchor_c[11], anchor_s[11];
    int64_t c = 0, s = Q30;
    for (int b = 10; b >= 0; b--) {
        int64_t c2 = (int64_t)isqrt_u64((uint64_t)(Q30 + c) * (uint64_t)Q30 / 2u);
        s = s * Q30 / (2 * c2);
        c = c2;
        anchor_c[b] = c;
        anchor_s[b] = s;
    }
    for (uint32_t k = 0u; k < DFT_M; k++) {
        int64_t kc = Q30, ks = 0;
        for (uint32_t b = 0u; b < 11u; b++) {
            if (((k >> b) & 1u) != 0u) {
                int64_t nc = (kc * anchor_c[b] - ks * anchor_s[b]) / Q30;
                ks = (kc * anchor_s[b] + ks * anchor_c[b]) / Q30;
                kc = nc;
            }
        }
        for (uint32_t q = 0u; q < (k >> 11); q++) {
            int64_t t = kc;
            kc = -ks;
            ks = t;
        }
        trig_cos[k] = (int32_t)kc;
        trig_sin[k] = (int32_t)ks;
    }
}

/* The four-term Blackman-Harris window in Q30; returns the sum of the window.
   The coefficients are the usual 0.35875, 0.48829, 0.14128 and 0.01168 times
   2^30, rounded. Every term is at most 2^60, the sum at most 2^61. */
static int64_t build_window(void)
{
    static const int64_t a0 = 385204879, a1 = 524297395, a2 = 151698245, a3 = 12541305;
    int64_t sum = 0;
    for (uint32_t n = 0u; n < DFT_M; n++) {
        int64_t v = a0 * Q30 - a1 * trig_cos[n] + a2 * trig_cos[(2u * n) & (DFT_M - 1u)] -
                    a3 * trig_cos[(3u * n) & (DFT_M - 1u)];
        window[n] = (int32_t)(v / Q30);
        sum += window[n];
    }
    return sum;
}

/* The squared magnitude of bin k of the windowed block, in sample units. A
   windowed sample is at most 2^30 and a term of the sum at most 2^45, so the
   accumulators over M terms stay under 2^58; the final value is at most 2^28
   and its square at most 2^57. */
static uint64_t dft_power(uint32_t k)
{
    int64_t re = 0, im = 0;
    uint32_t idx = 0u;
    for (uint32_t n = 0u; n < DFT_M; n++) {
        re += windowed[n] * trig_cos[idx] / 32768;
        im += windowed[n] * trig_sin[idx] / 32768;
        idx = (idx + k) & (DFT_M - 1u);
    }
    re /= Q30;
    im /= Q30;
    return (uint64_t)(re * re) + (uint64_t)(im * im);
}

/* Tenths of a decibel in a power ratio r >= 1 (one hundred log10 of r), from
   the base 2 logarithm: the integer part is the bit length, ten fractional
   bits come from repeated squaring. */
static uint32_t tenths_db(uint64_t r)
{
    uint32_t e = 0u;
    while ((r >> (e + 1u)) != 0u) {
        e++;
    }
    uint64_t x = e <= 30u ? r << (30u - e) : r >> (e - 30u);
    uint32_t frac = 0u;
    for (uint32_t i = 0u; i < 10u; i++) {
        x = (x * x) >> 30;
        frac <<= 1;
        if (x >= (UINT64_C(2) << 30)) {
            x >>= 1;
            frac |= 1u;
        }
    }
    return (uint32_t)(((uint64_t)e * 1024u + frac) * 30103u / 1024000u);
}

/* Renders the tone through the synth, windows 8,192 samples after the first
   4,096 and checks every bin from 5 up to 16 kHz against the reference, which
   is the programmed step height as a sinusoid. The tone's own harmonics all lie
   above 16 kHz for these two tones, so no bin is masked but the first four,
   the window's main lobe around DC. Returns the fundamental's magnitude over
   the reference in per mille when it falls below Nyquist, else 0. */
static void spectral_tone(uint32_t half_ticks, uint32_t fundamental_bin)
{
    build_trig();
    int64_t wsum = build_window();
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, COGMASTER_VIDEO_SMS);
    struct tone t = {half_ticks, half_ticks, 0u, PSG_FULL, 0u};
    uint32_t have = 0u;
    for (uint32_t f = 0u; f < DFT_FRAMES; f++) {
        cogmaster_audio a;
        tone_frame(m, &t);
        frame(m);
        CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
        memcpy(&tone_pcm[have], pcm, (size_t)a.written * sizeof pcm[0]);
        have += a.written;
    }
    CHECK(have >= DFT_SKIP + DFT_M);
    CHECK_EQ_U64(m->synth.stats.clamps, 0u);
    CHECK(m->synth.stats.kernel_apps > 16u * 3000u); /* the changes really entered the ring */
    cogmaster_destroy(m);
    for (uint32_t n = 0u; n < DFT_M; n++) {
        windowed[n] = (int64_t)tone_pcm[DFT_SKIP + n] * window[n] / 32768;
    }
    uint64_t ref = (uint64_t)STEP_HEIGHT * (uint64_t)wsum / (UINT64_C(1) << 31);
    uint64_t ref2 = ref * ref;
    uint64_t limit = ref2 / 100000000u;
    uint64_t peak = 0u;
    uint32_t peak_bin = 0u;
    for (uint32_t k = 5u; k <= DFT_BINS; k++) {
        uint64_t p = dft_power(k);
        if (p > peak) {
            peak = p;
            peak_bin = k;
        }
    }
    /* A tone that leaves nothing above the 16-bit rounding has no peak at all. */
    uint32_t margin = peak == 0u ? 1200u : tenths_db(ref2 / peak);
    printf("spectral half period %lu ticks: worst bin %lu, %lu tenths of a dB below the reference "
           "(the rule needs 800)\n",
           (unsigned long)half_ticks, (unsigned long)peak_bin, (unsigned long)margin);
    CHECK(peak <= limit);
    /* The test is not vacuous: a tone below Nyquist shows up at its bin. */
    if (fundamental_bin != 0u) {
        uint64_t best = 0u;
        for (uint32_t k = fundamental_bin - 2u; k <= fundamental_bin + 2u; k++) {
            uint64_t p = dft_power(k);
            best = p > best ? p : best;
        }
        CHECK(best > ref2 / 4u);
    }
}

/* The 16 and 96 Z80 cycle half periods: 111.9 kHz and 18.6 kHz squares. The
   second's fundamental sits at bin 3181 or 3182 of 8,192. */
static void test_spectral(void)
{
    spectral_tone(48u, 0u);
    spectral_tone(288u, 3181u);
}

/* ---- Headroom, accumulation, overshoot, routing and continuity ---- */

static cogmaster *make_gg(void)
{
    return make(COGMASTER_MACHINE_GG, COGMASTER_TV_NTSC, COGMASTER_VIDEO_GG);
}

/* The extremes of the first n sample frames of the drained PCM, either
   channel. */
static void extremes(uint32_t n, uint32_t channels, int32_t *lo, int32_t *hi)
{
    for (uint32_t i = 0u; i < n * channels; i++) {
        *lo = pcm[i] < *lo ? pcm[i] : *lo;
        *hi = pcm[i] > *hi ? pcm[i] : *hi;
    }
}

/* Four PSG channels at full level stepping together, on both Game Gear
   channels, at every kernel row: the peak stays 1 dB under full scale and
   nothing saturates, with the Master System gain and with the gain beside the
   FM unit. With a fall to silence a few samples later (the kernel rings, so
   the pair can pass the 1 dB mark) nothing saturates either. */
static int32_t no_clip_run(uint32_t fm, uint32_t fall_after, int32_t *low)
{
    int32_t peak = 0;
    *low = 0;
    for (uint64_t tick = 40000u; tick < 40000u + 224u; tick += 3u) {
        cogmaster *m = make_gg();
        if (fm != 0u) {
            m->fm_unit = 1u;
            m->fm_mask = 3u;
        }
        cogmaster__synth_level(m, 0u, 4u, 0xFFu);
        for (uint32_t src = 0u; src < 4u; src++) {
            cogmaster__synth_level(m, tick, src, PSG_FULL);
            if (fall_after != 0u) {
                cogmaster__synth_level(m, tick + fall_after, src, 0u);
            }
        }
        cogmaster_audio a;
        frame(m);
        CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
        extremes(a.written, a.channels, low, &peak);
        CHECK_EQ_U64(m->synth.stats.clamps, 0u);
        cogmaster_destroy(m);
    }
    return peak;
}

static void test_no_clip(void)
{
    int32_t low = 0;
    int32_t psg = no_clip_run(0u, 0u, &low);
    int32_t fm = no_clip_run(1u, 0u, &low);
    CHECK(psg <= MINUS_ONE_DBFS);
    CHECK(fm <= MINUS_ONE_DBFS);
    /* The overshoot is real (the sum is 26,880) and under the 1 dB bound. */
    CHECK(psg > 4 * STEP_HEIGHT);
    CHECK(fm > 4 * 3928);
    printf(
        "no_clip: four channels peak %ld of %d (overshoot %ld per mille); with the FM gain %ld\n",
        (long)psg, 4 * STEP_HEIGHT, (long)(psg * 1000 / (4 * STEP_HEIGHT) - 1000), (long)fm);
    int32_t pulse = no_clip_run(0u, 900u, &low);
    CHECK(pulse < 32767 && low > -32768);
}

static int64_t ring_extreme(const cogmaster *m)
{
    int64_t big = 0;
    for (uint32_t ch = 0u; ch < 2u; ch++) {
        for (uint32_t i = 0u; i < COGMASTER__SYNTH_RING; i++) {
            int64_t v = m->synth.ring[ch][i];
            big = (v < 0 ? -v : v) > big ? (v < 0 ? -v : v) : big;
        }
    }
    return big;
}

/* The accumulation bound: every sample position takes 128 changes on each
   channel (32 stereo mask flips that each move four sources), alternating in
   sign, for a whole frame. No slot and no sum leaves the bound the state and
   the static assertion in synth.c are built on, and a frame later the ring is
   empty again. */
static void test_amplitude_bound(void)
{
    cogmaster *m = make_gg();
    cogmaster__synth_level(m, 0u, 4u, 0xFFu);
    for (uint32_t src = 0u; src < 4u; src++) {
        cogmaster__synth_level(m, 0u, src, PSG_FULL);
    }
    uint32_t flip = 0u;
    for (uint64_t tick = 7u; tick < cogmaster__frame_ticks(m); tick += 7u) {
        cogmaster__synth_level(m, tick, 4u, (flip++ & 1u) != 0u ? 0xFFu : 0x00u);
    }
    const int64_t bound = INT64_C(128) * 32767 * 70842 * 65536;
    int64_t big = ring_extreme(m);
    CHECK(big < bound);
    CHECK(bound < COGMASTER__SYNTH_SLOT_MAX);
    CHECK(big > (INT64_C(1) << 40)); /* the hammer really loaded the ring */
    for (uint32_t f = 0u; f < 3u; f++) {
        frame(m);
        for (uint32_t ch = 0u; ch < 2u; ch++) {
            int64_t v = m->synth.integ[ch];
            CHECK((v < 0 ? -v : v) < COGMASTER__SYNTH_INTEG_MAX / 16);
            v = m->synth.dc[ch];
            CHECK((v < 0 ? -v : v) < COGMASTER__SYNTH_DC_MAX / 16);
        }
        cogmaster_audio a;
        CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    }
    CHECK_EQ_U64(ring_extreme(m), 0u);
    cogmaster_destroy(m);
}

/* A change stamped up to 69 ticks past the deadline (the longest step) lands
   at or after the frame's own samples, within the saved window, and is heard
   in the next frame, on either TV standard. */
static void overshoot_case(uint32_t tv, uint32_t n_frames_in)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, tv, COGMASTER_VIDEO_SMS);
    cogmaster_audio a;
    cogmaster__synth_level(m, cogmaster__frame_ticks(m) + 69u, 0u, PSG_FULL);
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, n_frames_in);
    uint32_t heard = 0u;
    for (uint32_t i = 0u; i < a.written; i++) {
        heard += pcm[i] != 0;
    }
    CHECK_EQ_U64(heard, 0u);
    CHECK(m->synth.stats.max_index >= n_frames_in + 16u);
    CHECK(m->synth.stats.max_index < 992u);
    CHECK(n_frames_in + COGMASTER__SYNTH_SAVED < COGMASTER__SYNTH_RING);
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    int32_t lo = 0, hi = 0;
    extremes(16u, 1u, &lo, &hi);
    CHECK(hi > STEP_HEIGHT / 2);
    cogmaster_destroy(m);
}

static void test_ring_overshoot(void)
{
    overshoot_case(COGMASTER_TV_NTSC, 801u);
    overshoot_case(COGMASTER_TV_PAL, 965u);
}

/* One frame of a Game Gear with the given mask, source 0 full from tick 20000;
   the samples go to out, interleaved. */
static uint32_t mask_frame(uint32_t mask, uint64_t flip_at, uint32_t flip_to, int16_t *out)
{
    cogmaster *m = make_gg();
    cogmaster_audio a;
    cogmaster__synth_level(m, 0u, 4u, mask);
    cogmaster__synth_level(m, 20000u, 0u, PSG_FULL);
    if (flip_at != 0u) {
        cogmaster__synth_level(m, flip_at, 4u, flip_to);
    }
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.channels, 2u);
    memcpy(out, pcm, (size_t)a.written * 2u * sizeof pcm[0]);
    cogmaster_destroy(m);
    return a.written;
}

static int16_t left_run[COGMASTER__AUDIO_MAX_FRAMES * 2u];
static int16_t right_run[COGMASTER__AUDIO_MAX_FRAMES * 2u];

/* The mask byte sends each PSG channel to the left with its high bit and to
   the right with its low bit; changing it mid-tone is a band-limited step on
   the channel it affects and leaves the other bit for bit as it was. */
static void test_stereo_mask(void)
{
    static const struct {
        uint32_t mask;
        int left, right;
    } cases[] = {{0xF0u, 1, 0}, {0x0Fu, 0, 1}, {0x00u, 0, 0}, {0xFFu, 1, 1}};
    for (size_t c = 0u; c < sizeof cases / sizeof cases[0]; c++) {
        uint32_t n = mask_frame(cases[c].mask, 0u, 0u, left_run);
        int32_t peak[2] = {0, 0};
        for (uint32_t i = 0u; i < n; i++) {
            for (uint32_t ch = 0u; ch < 2u; ch++) {
                int32_t v = left_run[i * 2u + ch];
                peak[ch] = v > peak[ch] ? v : peak[ch];
            }
        }
        CHECK((peak[0] > STEP_HEIGHT / 2) == (cases[c].left != 0));
        CHECK((peak[1] > STEP_HEIGHT / 2) == (cases[c].right != 0));
        CHECK(cases[c].left != 0 || peak[0] == 0);
        CHECK(cases[c].right != 0 || peak[1] == 0);
    }
    /* Left off at tick 60000 against a run that keeps both. */
    const uint64_t flip_at = 60000u;
    const uint32_t at = index_of(flip_at);
    uint32_t n = mask_frame(0xFFu, flip_at, 0x0Fu, left_run);
    CHECK_EQ_U64(mask_frame(0xFFu, 0u, 0u, right_run), n);
    uint32_t left_same_before = 0u, left_differs_after = 0u, right_differs = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        left_same_before += i < at && left_run[i * 2u] == right_run[i * 2u];
        left_differs_after += i >= at + 8u && i < at + 24u && left_run[i * 2u] != right_run[i * 2u];
        right_differs += left_run[i * 2u + 1u] != right_run[i * 2u + 1u];
    }
    CHECK_EQ_U64(left_same_before, at);
    CHECK(left_differs_after >= 12u);
    CHECK_EQ_U64(right_differs, 0u);
}

static int16_t *pcm_quiet_buffer(void)
{
    static int16_t buf[COGMASTER__AUDIO_MAX_FRAMES * 2u];
    return buf;
}

static void take_into(cogmaster *m, int16_t *out, uint32_t *written, uint32_t *channels)
{
    cogmaster_audio a;
    frame(m);
    CHECK_EQ_U64(drain(m, &a, COGMASTER__AUDIO_MAX_FRAMES), COGMASTER_STATUS_OK);
    memcpy(out, pcm, (size_t)a.written * a.channels * sizeof pcm[0]);
    *written = a.written;
    *channels = a.channels;
}

/* A state saved mid-tone and loaded into a second instance continues with the
   same PCM bytes as the instance it came from, and a third instance that was
   told nothing stays silent beside them. */
static void test_state_continue(void)
{
    static int16_t from_a[COGMASTER__AUDIO_MAX_FRAMES * 2u];
    static int16_t from_b[COGMASTER__AUDIO_MAX_FRAMES * 2u];
    cogmaster *a = make_gg();
    cogmaster *b = make_gg();
    cogmaster *quiet = make_gg();
    struct tone t0 = {977u, 977u, 0u, PSG_FULL, 0u};
    struct tone t2 = {1531u, 1531u, 2u, 0x12u, 0u};
    uint32_t n = 0u, ch = 0u;
    cogmaster__synth_level(a, 0u, 4u, 0xFFu);
    for (uint32_t f = 0u; f < 8u; f++) {
        tone_frame(a, &t0);
        tone_frame(a, &t2);
        take_into(a, from_a, &n, &ch);
    }
    size_t size = cogmaster__state_size(a);
    uint8_t *buf = malloc(size);
    if (buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(a, buf, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(b, buf, size), COGMASTER_STATUS_OK);
    free(buf);
    struct tone b0 = t0, b2 = t2;
    uint32_t loud = 0u, quiet_nonzero = 0u, differs = 0u;
    for (uint32_t f = 0u; f < 10u; f++) {
        uint32_t nb = 0u, chb = 0u, nq = 0u, chq = 0u;
        tone_frame(a, &t0);
        tone_frame(a, &t2);
        take_into(a, from_a, &n, &ch);
        tone_frame(b, &b0);
        tone_frame(b, &b2);
        take_into(b, from_b, &nb, &chb);
        take_into(quiet, pcm_quiet_buffer(), &nq, &chq);
        CHECK_EQ_U64(nb, n);
        CHECK_EQ_U64(chb, ch);
        differs += memcmp(from_a, from_b, (size_t)n * ch * sizeof from_a[0]) != 0;
        for (uint32_t i = 0u; i < n * ch; i++) {
            loud += from_a[i] != 0;
            quiet_nonzero += pcm_quiet_buffer()[i] != 0;
        }
    }
    CHECK_EQ_U64(differs, 0u);
    CHECK(loud > 1000u);
    CHECK_EQ_U64(quiet_nonzero, 0u);
    cogmaster_destroy(a);
    cogmaster_destroy(b);
    cogmaster_destroy(quiet);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"cadence", test_cadence},
    {"drain", test_drain},
    {"step", test_step},
    {"spectral", test_spectral},
    {"no_clip", test_no_clip},
    {"amplitude_bound", test_amplitude_bound},
    {"ring_overshoot", test_ring_overshoot},
    {"stereo_mask", test_stereo_mask},
    {"state_continue", test_state_continue},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-audio <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-audio: unknown case %s\n", argv[1]);
    return 2;
}
