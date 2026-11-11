/* The controller ports at pin level, against the formulas they replaced: a
   small image is loaded through the public API, and the ports are written and
   read through the I/O functions in src/internal.h: cogmaster-test-ports
   <case>. The oracles below are literal copies of the expressions src/io.c
   held inline before the port layer existed. They are copies on purpose: a
   refactor that moves the H-counter latch or a pad pin is the fault, so the
   expected value must not come from the code under test. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define IMAGE_SIZE 1024u

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

static const struct {
    uint32_t machine;
    uint32_t region;
} profiles[] = {
    {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_JAPAN},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN},
    {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_GG, COGMASTER_REGION_JAPAN},
};
#define PROFILES (sizeof profiles / sizeof profiles[0])

static int is_gg(const cogmaster *m)
{
    return m->machine == COGMASTER_MACHINE_GG;
}

/* Copy of the effective TH pins before the port layer. */
static uint8_t oracle_th_pins(const cogmaster *m, uint8_t ctrl)
{
    uint8_t pins = 0xC0u;
    uint8_t echo = m->region == COGMASTER_REGION_JAPAN ? 0u : ctrl;
    if (!is_gg(m)) {
        if ((ctrl & 2u) == 0u) {
            pins = (uint8_t)((pins & ~0x40u) | ((echo & 0x20u) << 1));
        }
        if ((ctrl & 8u) == 0u) {
            pins = (uint8_t)((pins & ~0x80u) | (echo & 0x80u));
        }
    }
    return pins;
}

/* Copy of the inline latch condition: low-to-high on either TH. */
static int oracle_rising(const cogmaster *m, uint8_t old_ctrl, uint8_t new_ctrl)
{
    uint8_t old = oracle_th_pins(m, old_ctrl);
    return ((uint8_t)~old & oracle_th_pins(m, new_ctrl) & 0xC0u) != 0u;
}

/* The control: the same condition on the falling edge instead. */
static int oracle_falling(const cogmaster *m, uint8_t old_ctrl, uint8_t new_ctrl)
{
    uint8_t old = oracle_th_pins(m, old_ctrl);
    return (old & (uint8_t)~oracle_th_pins(m, new_ctrl) & 0xC0u) != 0u;
}

static int core_latched(cogmaster *m, uint8_t old_ctrl, uint8_t new_ctrl)
{
    m->io.io_ctrl = old_ctrl;
    m->vdp.hheld = 0u;
    cogmaster__io_out(m, 0x003Fu, new_ctrl);
    CHECK_EQ_HEX(m->io.io_ctrl, new_ctrl);
    return m->vdp.hheld != 0u;
}

/* Every pair of $3F values on every profile latches the H counter exactly
   when the old formula says it does. */
static void test_latch(void)
{
    uint64_t pairs = 0;
    uint64_t fired = 0;
    for (size_t k = 0; k < PROFILES; k++) {
        cogmaster *m = load(profiles[k].machine, profiles[k].region);
        for (uint32_t o = 0; o < 256u; o++) {
            for (uint32_t n = 0; n < 256u; n++) {
                int want = oracle_rising(m, (uint8_t)o, (uint8_t)n);
                int got = core_latched(m, (uint8_t)o, (uint8_t)n);
                if (want != got) {
                    fprintf(stderr, "profile %zu old %02X new %02X: want %d got %d\n", k, o, n,
                            want, got);
                    check_failures++;
                }
                pairs++;
                fired += (uint64_t)got;
            }
        }
        CHECK_EQ_U64(cogmaster__ports_pins(m, 0u, 0u), 0x3Fu);
        cogmaster_destroy(m);
    }
    CHECK_EQ_U64(pairs, PROFILES * 65536u);
    /* The sweep is not vacuous: some pairs latch and some do not. */
    CHECK(fired > 0u && fired < pairs);
}

/* An oracle that latches on the falling edge disagrees with the core on some
   pair, so the sweep above can tell the two edges apart. */
static void test_latch_control(void)
{
    uint64_t disagree = 0;
    for (size_t k = 0; k < PROFILES; k++) {
        cogmaster *m = load(profiles[k].machine, profiles[k].region);
        for (uint32_t o = 0; o < 256u; o++) {
            for (uint32_t n = 0; n < 256u; n++) {
                int got = core_latched(m, (uint8_t)o, (uint8_t)n);
                disagree += (uint64_t)(oracle_falling(m, (uint8_t)o, (uint8_t)n) != got);
            }
        }
        cogmaster_destroy(m);
    }
    CHECK(disagree > 0u);
}

/* Copy of the TR mixing before the port layer. */
static uint8_t oracle_tr_pin(const cogmaster *m, uint8_t v, uint8_t pin, uint8_t dir, uint8_t level)
{
    if (!is_gg(m) && (m->io.io_ctrl & dir) == 0u) {
        v = (uint8_t)(v & ~pin);
        if (m->region != COGMASTER_REGION_JAPAN && (m->io.io_ctrl & level) != 0u)
            v |= pin;
    }
    return v;
}

/* Copy of $DC before the port layer. */
static uint8_t oracle_dc(const cogmaster *m)
{
    uint32_t p1 = m->input.port1 == COGMASTER_DEVICE_NONE ? 0u : m->input.p1;
    uint32_t p2 = m->input.port2 == COGMASTER_DEVICE_NONE || is_gg(m) ? 0u : m->input.p2;
    return oracle_tr_pin(m, (uint8_t)~(p1 | ((p2 & 3u) << 6)), 0x20u, 1u, 0x10u);
}

/* Copy of $DD before the port layer. */
static uint8_t oracle_dd(const cogmaster *m)
{
    uint32_t p2 = m->input.port2 == COGMASTER_DEVICE_NONE || is_gg(m) ? 0u : m->input.p2;
    uint32_t reset = m->machine == COGMASTER_MACHINE_SMS1 && m->region == COGMASTER_REGION_EXPORT &&
                             m->input.port1 != COGMASTER_DEVICE_NONE
                         ? m->input.reset
                         : 0u;
    uint8_t v = oracle_tr_pin(m, (uint8_t)~((p2 >> 2) | (reset << 4)), 8u, 4u, 0x40u);
    return (uint8_t)((v & 0x3Fu) | oracle_th_pins(m, m->io.io_ctrl));
}

/* $DC and $DD read the old bytes for every p1 and p2, for a device or none on
   each port, with the reset button and the console's own TR output clear and
   set. */
static void test_pins(void)
{
    static const uint8_t ctrls[] = {0xFFu, 0xEEu, 0xBBu, 0x00u, 0x55u, 0xA5u};
    uint64_t reads = 0;
    uint64_t pressed = 0;
    for (size_t k = 0; k < PROFILES; k++) {
        cogmaster *m = load(profiles[k].machine, profiles[k].region);
        for (uint32_t dev = 0; dev < 4u; dev++) {
            m->input.port1 = dev & 1u ? COGMASTER_DEVICE_NONE : COGMASTER_DEVICE_PAD;
            m->input.port2 = dev & 2u ? COGMASTER_DEVICE_NONE : COGMASTER_DEVICE_PAD;
            for (uint32_t p1 = 0; p1 < 64u; p1++) {
                for (uint32_t p2 = 0; p2 < 64u; p2++) {
                    m->input.p1 = p1;
                    m->input.p2 = p2;
                    for (uint32_t r = 0; r < 2u; r++) {
                        m->input.reset = r;
                        for (size_t c = 0; c < sizeof ctrls; c++) {
                            m->io.io_ctrl = ctrls[c];
                            uint8_t dc = cogmaster__io_in(m, 0x00DCu);
                            uint8_t dd = cogmaster__io_in(m, 0x00DDu);
                            CHECK_EQ_HEX(dc, oracle_dc(m));
                            CHECK_EQ_HEX(dd, oracle_dd(m));
                            reads += 2u;
                            pressed += (uint64_t)(dc != 0xFFu);
                            if (check_failures > 20) {
                                return;
                            }
                        }
                    }
                }
            }
        }
        CHECK_EQ_U64(cogmaster__ports_pins(m, 1u, 0u) & 0xC0u, 0u);
        cogmaster_destroy(m);
    }
    CHECK_EQ_U64(reads, PROFILES * 4u * 64u * 64u * 2u * sizeof ctrls * 2u);
    /* Not vacuous: most reads have a key down. */
    CHECK(pressed > reads / 4u);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"latch", test_latch},
    {"latch_control", test_latch_control},
    {"pins", test_pins},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-ports <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-ports: unknown case %s\n", argv[1]);
    return 2;
}
