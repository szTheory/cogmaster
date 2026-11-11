/* The Light Phaser on the internal API: cogmaster-test-ports-gun <case>. The
   oracle below is a literal copy of the H-counter expression src/vdp.c held
   before it became a function of the tick. It is a copy on purpose: a
   refactor that moves the latch origin is the fault, so the expected value
   must not come from the code under test. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define LINE_TICKS 684u

static uint8_t image[IMAGE_SIZE];

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

static cogmaster *load(uint32_t machine, uint32_t region)
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
    memset(image, 0x76, sizeof image);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    media.region = region;
    media.tv = COGMASTER_TV_NTSC;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

/* Copy of the pre-refactor counter, with the origin as a parameter so the
   control can move it by one. */
static uint8_t oracle_hcount(uint32_t tick, uint32_t origin)
{
    uint32_t t = (tick + origin) % LINE_TICKS;
    return (uint8_t)((t >= 592u ? t + 340u : t) >> 2);
}

/* The counter at every in-line tick equals the copy, and the latch holds the
   value of the tick it is given, not of the bus tick. */
static void test_latch_at(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    uint32_t ticks = 0u;
    uint32_t moved = 0u;
    for (uint32_t t = 0u; t < LINE_TICKS; t++) {
        uint8_t want = oracle_hcount(t, 32u);
        CHECK_EQ_HEX(cogmaster__vdp_hcount_at(m, t), want);
        /* The control: an origin one lower must disagree somewhere. */
        moved += oracle_hcount(t, 31u) != want ? 1u : 0u;
        m->bus.t = t;
        m->vdp.hheld = 0u;
        cogmaster__vdp_latch_h(m);
        CHECK_EQ_HEX(m->vdp.hcounter, want);
        CHECK_EQ_U64(m->vdp.hheld, 1u);
        ticks++;
    }
    CHECK_EQ_U64(ticks, LINE_TICKS);
    CHECK(moved > 0u);
    for (uint32_t k = 0u; k < 16u; k++) {
        uint32_t t1 = (k * 41u) % LINE_TICKS;
        uint32_t t2 = (k * 41u + 300u + k) % LINE_TICKS;
        m->bus.t = t1;
        m->vdp.hheld = 0u;
        cogmaster__vdp_latch_h_at(m, t2);
        CHECK_EQ_HEX(m->vdp.hcounter, oracle_hcount(t2, 32u));
        CHECK_EQ_U64(m->vdp.hheld, 1u);
    }
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"latch_at", test_latch_at},
};

int main(int argc, char **argv)
{
    if (argc == 2) {
        for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            if (strcmp(argv[1], cases[i].name) == 0) {
                cases[i].run();
                CHECK_DONE();
            }
        }
    }
    fprintf(stderr, "cogmaster-test-ports-gun: unknown case\n");
    return 2;
}
