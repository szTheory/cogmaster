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
    if (m->bus.line != ORIGIN_LINE || m->bus.t != ORIGIN_TICK || m->ticks != 0u || m->frame != 0u) {
        fprintf(stderr, "%s: line %lu t %lu ticks %llu frame %llu, expected line 192 t 607\n", what,
                (unsigned long)m->bus.line, (unsigned long)m->bus.t, (unsigned long long)m->ticks,
                (unsigned long long)m->frame);
        check_failures++;
    }
}

/* A halted image runs ten frames: each stops at the frame with at most 69
   ticks over, the overshoot is charged to the next frame, so the total is
   ten frames plus the last overshoot, and the CPU is still halted. iff1 is
   the enable the image leaves. */
static cogmaster *halt_bound(const uint8_t *prog, size_t n, uint32_t tv, uint64_t frame_ticks,
                             uint8_t iff1)
{
    cogmaster *m = load_tv(prog, n, tv);
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
    CHECK_EQ_U64(m->z80.iff1, iff1);
    CHECK_EQ_HEX(m->z80.pc, 0x0002);
    /* The beam is back at the origin, overshoot ticks on. */
    CHECK_EQ_U64(m->bus.line, ORIGIN_LINE);
    CHECK_EQ_U64(m->bus.t, ORIGIN_TICK + overshoot);
    return m;
}

/* DI; HALT. */
static void halt_bound_di(uint32_t tv, uint64_t frame_ticks)
{
    static const uint8_t prog[] = {0xF3u, 0x76u};
    cogmaster_destroy(halt_bound(prog, sizeof prog, tv, frame_ticks, 0u));
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

/* EI; HALT with the frame interrupt masked in the VDP (R1 IE0 clear, as
   load leaves it): the frame flag rises every frame but /INT stays high,
   so the CPU stays halted with interrupts enabled, on NTSC and PAL. */
static void test_halt_bound_ei_masked(void)
{
    static const uint8_t prog[] = {0xFBu, 0x76u};
    const uint32_t tvs[] = {COGMASTER_TV_NTSC, COGMASTER_TV_PAL};
    const uint64_t lengths[] = {TICKS_PER_FRAME, TICKS_PER_FRAME_PAL};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = halt_bound(prog, sizeof prog, tvs[k], lengths[k], 1u);
        CHECK_EQ_HEX(m->vdp.status & 0x80u, 0x80);
        CHECK_EQ_U64(m->z80.int_line, 0);
        cogmaster_destroy(m);
    }
}

/* The positive control: with IE0 set, each frame interrupt reaches the
   CPU once. The handler at $0038 reads the status port, which releases
   /INT, and counts in $C100.
     $0000  DI; IM 1; LD A,$20; OUT ($BF),A; LD A,$81; OUT ($BF),A  (R1 = IE0)
     $000B  EI; HALT; JR $000B
     $0038  IN A,($BF); LD HL,$C100; INC (HL); EI; RETI */
static void test_halt_bound_ie_positive(void)
{
    static const uint8_t prog[] = {
        0xF3u, 0xEDu, 0x56u, 0x3Eu, 0x20u, 0xD3u, 0xBFu, 0x3Eu, 0x81u, 0xD3u, 0xBFu, 0xFBu, 0x76u,
        0x18u, 0xFCu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0xDBu, 0xBFu, 0x21u, 0x00u, 0xC1u, 0x34u, 0xFBu, 0xEDu, 0x4Du};
    _Static_assert(sizeof prog == 0x38u + 9u, "the handler starts at $0038");
    cogmaster *m = load(prog, sizeof prog);
    /* The first frame starts just after its own interrupt tick, so its
       interrupt is served at the start of the second. */
    (void)run(m);
    CHECK_EQ_HEX(m->vdp.reg[1], 0x20);
    uint8_t count = m->ram[0x100];
    for (uint32_t i = 0; i < 10u; i++) {
        cogmaster_run_result r = run(m);
        CHECK_EQ_U64(r.stop, COGMASTER_STOP_FRAME);
        CHECK(r.overshoot_ticks <= MAX_OVERSHOOT);
        CHECK_EQ_U64(m->ram[0x100], (uint8_t)(count + 1u));
        count = m->ram[0x100];
        CHECK_EQ_U64(m->z80.halted, 1);
    }
    cogmaster_destroy(m);
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

/* The fault record a run of prog leaves, through the public API. */
static cogmaster_fault fault_of(const cogmaster *m)
{
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
    return f;
}

/* LD A,0; IN A,($00); HALT on the export console: nothing drives a read of
   $00-$3F, so the read returns open bus, is counted, and records the first
   fault: kind unmapped_read (1), device ctrl (5), the bus address A<<8|n,
   the IN's own address and opcode, and the beam at the I/O cycle's data
   T-state, 7 + 10 T-states after the origin. */
static void test_fault_unmapped_read(void)
{
    static const uint8_t prog[] = {0x3Eu, 0x00u, 0xDBu, 0x00u, 0x76u};
    cogmaster *m = load(prog, sizeof prog);
    (void)run(m);
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 1);
    CHECK_EQ_U64(f.kind, 1);
    CHECK_EQ_U64(f.device, 5);
    CHECK_EQ_HEX(f.addr, 0x0000);
    CHECK_EQ_HEX(f.pc, 0x0002);
    CHECK_EQ_HEX(f.opcode, 0xDB);
    CHECK_EQ_U64(f.width, 8);
    CHECK_EQ_U64(f.frame, 0);
    CHECK_EQ_U64(f.line, ORIGIN_LINE);
    CHECK_EQ_U64(f.dot, (ORIGIN_TICK + 3u * 17u) / 2u);
    CHECK_EQ_HEX(m->z80.reg[COGMASTER__Z80_A], 0xFF);
    CHECK_EQ_U64(m->counters.open_bus_reads, 1);
    cogmaster_destroy(m);
}

/* Every access is stamped at its data T-state: T3 of a memory cycle, T4 of
   an I/O cycle (the automatic wait state included). For each instruction,
   the VDP access tick is the start tick plus 3 x the zero-based index of
   that T-state, as the cycle templates the vectors check lay it out; a
   memory instruction ends 3 x its T-states after it starts.
     $0000  OUT ($BF),A       T-state 10 of 11
     $0002  LD C,$BE
     $0004  OUT (C),A         T-state 11 of 12
     $0006  LD HL,$C000; LD B,1
     $000B  OUTI              T-state 15 of 16
     $000D  IN A,($7E)        T-state 10 of 11
     $000F  LD ($C100),A      13 T-states
     $0012  LD A,($C100)      13 T-states
     $0015  HALT */
static void test_stamps(void)
{
    static const uint8_t prog[] = {0xD3u, 0xBFu, 0x0Eu, 0xBEu, 0xEDu, 0x79u, 0x21u, 0x00u,
                                   0xC0u, 0x06u, 0x01u, 0xEDu, 0xA3u, 0xDBu, 0x7Eu, 0x32u,
                                   0x00u, 0xC1u, 0x3Au, 0x00u, 0xC1u, 0x76u};
    static const struct {
        uint16_t pc;
        uint32_t data_t; /* the VDP access T-state, or 0 for none */
        uint32_t length; /* T-states, or 0 when not checked */
    } steps[] = {
        {0x0000u, 10u, 11u}, {0x0002u, 0u, 7u},  {0x0004u, 11u, 12u},
        {0x0006u, 0u, 10u},  {0x0009u, 0u, 7u},  {0x000Bu, 15u, 16u},
        {0x000Du, 10u, 11u}, {0x000Fu, 0u, 13u}, {0x0012u, 0u, 13u},
    };
    cogmaster *m = load(prog, sizeof prog);
    for (size_t i = 0; i < sizeof steps / sizeof steps[0]; i++) {
        CHECK_EQ_HEX(m->z80.pc, steps[i].pc);
        uint64_t start = m->ticks;
        uint64_t vdp_at = m->bus.vdp_at;
        cogmaster__z80_step(m);
        if (steps[i].data_t != 0u) {
            CHECK_EQ_U64(m->bus.vdp_at, start + 3u * steps[i].data_t);
        } else {
            CHECK_EQ_U64(m->bus.vdp_at, vdp_at);
        }
        CHECK_EQ_U64(m->ticks, start + 3u * steps[i].length);
    }
    /* The bytes went where the stamps say: the first control byte set the
       address's low byte, OUT (C),A wrote A there and OUTI wrote the byte
       at $C000 after it. */
    CHECK_EQ_HEX(m->vdp.vram[0x00FF], 0xFF);
    CHECK_EQ_HEX(m->vdp.vram[0x0100], 0xAB);
    cogmaster_destroy(m);
}

/* A jammed image hammering the VDP ports, with no HALT and interrupts
   disabled, still returns every frame:
     $0000  OUT ($BF),A; OUT ($BE),A; INC A; JP $0000 */
static void hostile_vdp_loop(uint32_t tv, uint64_t frame_ticks)
{
    static const uint8_t prog[] = {0xD3u, 0xBFu, 0xD3u, 0xBEu, 0x3Cu, 0xC3u, 0x00u, 0x00u};
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
    CHECK_EQ_U64(m->z80.halted, 0);
    CHECK(m->bus.vdp_at > 9u * frame_ticks);
    cogmaster_destroy(m);
}

static void test_hostile_vdp_loop(void)
{
    hostile_vdp_loop(COGMASTER_TV_NTSC, TICKS_PER_FRAME);
    hostile_vdp_loop(COGMASTER_TV_PAL, TICKS_PER_FRAME_PAL);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"fault.ed_undefined", test_fault_ed_undefined},
    {"fault.unmapped_read", test_fault_unmapped_read},
    {"frame_origin", test_frame_origin},
    {"halt_bound.di", test_halt_bound_di},
    {"halt_bound.di_pal", test_halt_bound_di_pal},
    {"halt_bound.ei_masked", test_halt_bound_ei_masked},
    {"halt_bound.ie_positive", test_halt_bound_ie_positive},
    {"hostile_vdp_loop", test_hostile_vdp_loop},
    {"stamps", test_stamps},
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
