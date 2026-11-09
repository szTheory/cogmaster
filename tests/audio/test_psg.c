/* The Sega PSG through the bus and against closed forms: cogmaster-test-psg
   <case>. Writes go in through the bus the Z80 uses, and the chip's registers
   and the trace log are read back through src/internal.h. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define TRACE 4096u
#define STEP 48u

static uint8_t image[IMAGE_SIZE];
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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"through_bus", test_through_bus},
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
