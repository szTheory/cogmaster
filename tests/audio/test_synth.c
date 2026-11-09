/* The audio cadence and the host's drain, through the public API over the
   built-in test card: cogmaster-test-audio <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
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

static void frame(cogmaster *m)
{
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

    /* A load and both resets empty the log and its drop count. */
    append(m, 0u, 6u);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(drain_log(m, &t, 8u), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.written + t.dropped, 0u);
    for (uint32_t kind = COGMASTER_RESET_SOFT; kind <= COGMASTER_RESET_POWER; kind++) {
        append(m, 0u, 6u);
        CHECK_EQ_U64(cogmaster_reset(m, kind), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(drain_log(m, &t, 8u), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(t.written + t.dropped, 0u);
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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"cadence", test_cadence},
    {"drain", test_drain},
    {"step", test_step},
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
