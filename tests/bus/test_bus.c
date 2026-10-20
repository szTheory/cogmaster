/* The bus and the run loop through the public API, with the beam and the
   CPU read through src/internal.h: cogmaster-test-bus <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define TICKS_PER_FRAME 179208u
#define TICKS_PER_FRAME_PAL 214092u
#define ORIGIN_LINE 192u
#define ORIGIN_TICK 607u
#define MAX_OVERSHOOT 69u
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

/* A new Master System instance on a tv set running prog, padded with $00 to
   1 KiB, the smallest image accepted. */
static cogmaster *load_tv(const uint8_t *prog, size_t n, uint32_t tv)
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
    memset(image, 0, sizeof image);
    memcpy(image, prog, n);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = COGMASTER_MACHINE_SMS2;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

static cogmaster *load(const uint8_t *prog, size_t n)
{
    return load_tv(prog, n, COGMASTER_TV_AUTO);
}

static cogmaster_run_result run(cogmaster *m)
{
    cogmaster_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
    return r;
}

static void expect_origin(const char *what, const cogmaster *m)
{
    if (m->bus.line != ORIGIN_LINE || m->bus.t != ORIGIN_TICK || m->ticks != 0u ||
        m->frame != 0u) {
        fprintf(stderr, "%s: line %lu t %lu ticks %llu frame %llu, expected line 192 t 607\n", what,
                (unsigned long)m->bus.line, (unsigned long)m->bus.t,
                (unsigned long long)m->ticks, (unsigned long long)m->frame);
        check_failures++;
    }
}

/* DI; HALT runs ten frames: each stops at the frame with at most 69 ticks
   over, the overshoot is charged to the next frame, so the total is ten
   frames plus the last overshoot, and the CPU is still halted. */
static void halt_bound_di(uint32_t tv, uint64_t frame_ticks)
{
    static const uint8_t prog[] = {0xF3u, 0x76u};
    cogmaster *m = load_tv(prog, sizeof prog, tv);
    uint32_t overshoot = 0u;
    for (uint32_t i = 0; i < 10u; i++) {
        cogmaster_run_result r = run(m);
        CHECK_EQ_U64(r.stop, COGMASTER_STOP_FRAME);
        CHECK(r.overshoot_ticks <= MAX_OVERSHOOT);
        overshoot = r.overshoot_ticks;
    }
    CHECK_EQ_U64(m->ticks, 10u * frame_ticks + overshoot);
    CHECK_EQ_U64(m->frame, 10);
    CHECK_EQ_U64(m->z80.halted, 1);
    CHECK_EQ_U64(m->z80.iff1, 0);
    CHECK_EQ_HEX(m->z80.pc, 0x0002);
    /* The beam is back at the origin, overshoot ticks on. */
    CHECK_EQ_U64(m->bus.line, ORIGIN_LINE);
    CHECK_EQ_U64(m->bus.t, ORIGIN_TICK + overshoot);
    cogmaster_destroy(m);
}

static void test_halt_bound_di(void)
{
    halt_bound_di(COGMASTER_TV_AUTO, TICKS_PER_FRAME);
}

/* The same under PAL: 313 lines, so 214,092 ticks a frame. */
static void test_halt_bound_di_pal(void)
{
    halt_bound_di(COGMASTER_TV_PAL, TICKS_PER_FRAME_PAL);
}

/* Load and both resets put the beam at the frame origin with ticks 0. */
static void test_frame_origin(void)
{
    static const uint8_t prog[] = {0xF3u, 0x76u};
    cogmaster *m = load(prog, sizeof prog);
    expect_origin("after load", m);
    const uint32_t kinds[] = {COGMASTER_RESET_SOFT, COGMASTER_RESET_POWER};
    for (size_t k = 0; k < 2u; k++) {
        (void)run(m);
        (void)run(m);
        CHECK(m->ticks >= 2u * TICKS_PER_FRAME);
        CHECK_EQ_U64(cogmaster_reset(m, kinds[k]), COGMASTER_STATUS_OK);
        expect_origin(k == 0u ? "after a soft reset" : "after a power reset", m);
        CHECK_EQ_HEX(m->z80.pc, 0x0000);
        CHECK_EQ_U64(m->z80.halted, 0);
    }
    /* The first frame after a reset is a whole frame too. */
    cogmaster_run_result r = run(m);
    CHECK_EQ_U64(m->ticks, TICKS_PER_FRAME + r.overshoot_ticks);
    cogmaster_destroy(m);
}

/* An undefined ED opcode runs as a no-operation and records the first
   fault: kind ed_undefined (5), device cpu (1), the second byte as the
   opcode and the instruction's own address as pc. */
static void test_fault_ed_undefined(void)
{
    static const uint8_t prog[] = {0xEDu, 0x00u, 0x76u};
    cogmaster *m = load(prog, sizeof prog);
    (void)run(m);
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(f.present, 1);
    CHECK_EQ_U64(f.kind, COGMASTER__FAULT_ED_UNDEFINED);
    CHECK_EQ_U64(f.kind, 5);
    CHECK_EQ_U64(f.device, COGMASTER__DEVICE_CPU);
    CHECK_EQ_U64(f.device, 1);
    CHECK_EQ_HEX(f.opcode, 0x00);
    CHECK_EQ_HEX(f.pc, 0x0000);
    CHECK_EQ_HEX(f.addr, 0x0001);
    CHECK_EQ_U64(f.width, 8);
    CHECK_EQ_U64(f.frame, 0);
    /* Eight T-states after the origin: line 192, dot (607 + 24) / 2. */
    CHECK_EQ_U64(f.line, ORIGIN_LINE);
    CHECK_EQ_U64(f.dot, (ORIGIN_TICK + 24u) / 2u);
    /* The CPU ran on into the HALT. */
    CHECK_EQ_U64(m->z80.halted, 1);
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"fault.ed_undefined", test_fault_ed_undefined},
    {"frame_origin", test_frame_origin},
    {"halt_bound.di", test_halt_bound_di},
    {"halt_bound.di_pal", test_halt_bound_di_pal},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-bus <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-bus: unknown case %s\n", argv[1]);
    return 2;
}
