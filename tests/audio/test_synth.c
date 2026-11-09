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

static void test_drain(void)
{
    drain_pcm();
    drain_loads();
    drain_channels();
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"cadence", test_cadence},
    {"drain", test_drain},
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
