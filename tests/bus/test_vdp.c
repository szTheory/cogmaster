/* The VDP's timing half through the bus's port functions, with the beam
   placed and the VDP read through src/internal.h: cogmaster-test-vdp
   <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define LINES_NTSC 262u
#define LINES_PAL 313u
#define LINE_TICKS 684u
#define ORIGIN_LINE 192u
#define ORIGIN_TICK 607u
#define NMI_TICK 611u
#define MAX_STEP_TICKS 69u
#define NMI_ACCEPT_TICKS 33u
#define STEP_GUARD 400000u

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

/* A new instance of a profile running prog, padded with $00 to 1 KiB. */
static cogmaster *load_profile(const uint8_t *prog, size_t n, uint32_t machine, uint32_t region,
                               uint32_t tv)
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
    media.machine = machine;
    media.region = region;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

/* An export Master System on an NTSC set running DI; HALT. */
static cogmaster *load_sms(void)
{
    static const uint8_t prog[] = {0xF3u, 0x76u};
    return load_profile(prog, sizeof prog, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT,
                        COGMASTER_TV_NTSC);
}

/* Puts the beam at (line, t) at the current tick. */
static void put_beam(cogmaster *m, uint32_t line, uint32_t t)
{
    m->bus.line = line;
    m->bus.t = t;
    m->bus.at = m->ticks;
}

/* Runs n ticks with no CPU access and brings the beam up to them. */
static void pass(cogmaster *m, uint32_t n)
{
    m->ticks += n;
    cogmaster__bus_sync(m);
}

static uint8_t in(cogmaster *m, uint16_t port)
{
    return cogmaster__bus_in(m, port);
}

static void out(cogmaster *m, uint16_t port, uint8_t v)
{
    cogmaster__bus_out(m, port, v);
}

/* Two control bytes: an address or register value, then the code. */
static void control(cogmaster *m, uint8_t lo, uint8_t hi)
{
    out(m, 0xBFu, lo);
    out(m, 0xBFu, hi);
}

/* The V counter at $7E against a table built from the documented read
   sequence: in every line the value is that line's entry until tick 603
   and the next line's from tick 604. */
static void vcounter(uint32_t tv, const uint8_t ranges[][2], size_t nranges, uint32_t lines)
{
    uint8_t table[LINES_PAL];
    size_t n = 0;
    for (size_t r = 0; r < nranges; r++) {
        for (uint32_t v = ranges[r][0]; v <= ranges[r][1] && n < LINES_PAL; v++) {
            table[n++] = (uint8_t)v;
        }
    }
    CHECK_EQ_U64(n, lines);
    if (n != lines) {
        return;
    }
    static const uint8_t prog[] = {0xF3u, 0x76u};
    cogmaster *m =
        load_profile(prog, sizeof prog, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, tv);
    for (uint32_t line = 0; line < lines; line++) {
        put_beam(m, line, 603u);
        uint8_t before = in(m, 0x7Eu);
        pass(m, 1u);
        uint8_t after = in(m, 0x7Eu);
        if (before != table[line] || after != table[(line + 1u) % lines]) {
            fprintf(stderr, "line %lu: $%02X then $%02X, expected $%02X then $%02X\n",
                    (unsigned long)line, before, after, table[line], table[(line + 1u) % lines]);
            check_failures++;
        }
    }
    cogmaster_destroy(m);
}

/* NTSC 192: $00-$DA, then $D5-$FF (SMS-VDP section 4.4). */
static void test_vcounter_ntsc(void)
{
    static const uint8_t ranges[][2] = {{0x00u, 0xDAu}, {0xD5u, 0xFFu}};
    vcounter(COGMASTER_TV_NTSC, ranges, 2u, LINES_NTSC);
}

/* PAL 192: $00-$F2, then $BA-$FF. */
static void test_vcounter_pal(void)
{
    static const uint8_t ranges[][2] = {{0x00u, 0xF2u}, {0xBAu, 0xFFu}};
    vcounter(COGMASTER_TV_PAL, ranges, 2u, LINES_PAL);
}

/* $7F is the upper 8 bits of the 9-bit pixel counter, which jumps from
   pixel 295 to 466 at tick 592: $00-$93, then $E9-$FF (SMS-VDP section
   4.3). */
static void test_hcounter(void)
{
    cogmaster *m = load_sms();
    for (uint32_t t = 0; t < LINE_TICKS; t++) {
        put_beam(m, 10u, t);
        uint32_t want = (t >= 592u ? t + 340u : t) >> 2;
        uint8_t got = in(m, 0x7Fu);
        if (got != want) {
            fprintf(stderr, "t %lu: $%02X, expected $%02lX\n", (unsigned long)t, got,
                    (unsigned long)want);
            check_failures++;
        }
    }
    const uint32_t ticks[] = {0u, 591u, 592u, 683u};
    const uint8_t values[] = {0x00u, 0x93u, 0xE9u, 0xFFu};
    for (size_t i = 0; i < 4u; i++) {
        put_beam(m, 10u, ticks[i]);
        CHECK_EQ_HEX(in(m, 0x7Fu), values[i]);
    }
    cogmaster_destroy(m);
}

/* The control port's two-byte latch, the four codes, R0-R10, and the
   latch reset by a status read and by a data-port access (SMS-VDP section
   2.2). */
static void test_control_port(void)
{
    cogmaster *m = load_sms();
    struct cogmaster__vdp *v = &m->vdp;
    CHECK_EQ_HEX(v->reg[2], 0x0E);
    CHECK_EQ_HEX(v->reg[10], 0xFF);
    out(m, 0xBFu, 0x34u);
    CHECK_EQ_U64(v->latch, 1);
    out(m, 0xBFu, 0x52u);
    CHECK_EQ_U64(v->latch, 0);
    CHECK_EQ_U64(v->code, 1);
    CHECK_EQ_HEX(v->addr, 0x1234);
    /* Code 0 loads the read-ahead byte and steps the address. */
    v->vram[0x0123] = 0x5Au;
    control(m, 0x23u, 0x01u);
    CHECK_EQ_U64(v->code, 0);
    CHECK_EQ_HEX(v->buffer, 0x5A);
    CHECK_EQ_HEX(v->addr, 0x0124);
    /* Code 2 writes R0-R10. */
    for (uint32_t r = 0; r < COGMASTER__VDP_REGS; r++) {
        control(m, (uint8_t)(0x40u + r), (uint8_t)(0x80u | r));
        CHECK_EQ_HEX(v->reg[r], 0x40u + r);
        CHECK_EQ_U64(v->code, 2);
    }
    /* R11-R15 do not exist. */
    uint8_t regs[COGMASTER__VDP_REGS];
    memcpy(regs, v->reg, sizeof regs);
    for (uint32_t r = COGMASTER__VDP_REGS; r < 16u; r++) {
        control(m, 0x99u, (uint8_t)(0x80u | r));
    }
    CHECK(memcmp(regs, v->reg, sizeof regs) == 0);
    /* Code 3 selects CRAM. */
    control(m, 0x05u, 0xC0u);
    CHECK_EQ_U64(v->code, 3);
    CHECK_EQ_HEX(v->addr, 0x0005);
    /* A status read resets the latch: the next byte starts a new pair. */
    out(m, 0xBFu, 0x77u);
    (void)in(m, 0xBFu);
    CHECK_EQ_U64(v->latch, 0);
    control(m, 0x08u, 0x87u);
    CHECK_EQ_HEX(v->reg[7], 0x08);
    /* So does a data-port access. */
    out(m, 0xBFu, 0x66u);
    (void)in(m, 0xBEu);
    CHECK_EQ_U64(v->latch, 0);
    out(m, 0xBFu, 0x11u);
    out(m, 0xBEu, 0x00u);
    CHECK_EQ_U64(v->latch, 0);
    control(m, 0x09u, 0x87u);
    CHECK_EQ_HEX(v->reg[7], 0x09);
    cogmaster_destroy(m);
}

/* The data port: every access steps the 14-bit address; a read returns the
   read-ahead byte and refills it, a write stores the byte in VRAM (or CRAM
   under code 3) and in the read-ahead byte (SMS-VDP section 2.3). */
static void test_data_port(void)
{
    cogmaster *m = load_sms();
    struct cogmaster__vdp *v = &m->vdp;
    control(m, 0xFFu, 0x7Fu);
    out(m, 0xBEu, 0xA1u);
    out(m, 0xBEu, 0xB2u);
    CHECK_EQ_HEX(v->vram[0x3FFF], 0xA1);
    CHECK_EQ_HEX(v->vram[0x0000], 0xB2);
    CHECK_EQ_HEX(v->addr, 0x0001);
    /* The write left its byte in the read-ahead. */
    v->vram[0x0001] = 0xC3u;
    CHECK_EQ_HEX(in(m, 0xBEu), 0xB2);
    CHECK_EQ_HEX(v->buffer, 0xC3);
    CHECK_EQ_HEX(v->addr, 0x0002);
    /* Reads stream VRAM one byte behind the address, across the wrap. */
    v->vram[0x3FFE] = 0x11u;
    v->vram[0x3FFF] = 0x22u;
    v->vram[0x0000] = 0x33u;
    control(m, 0xFEu, 0x3Fu);
    CHECK_EQ_HEX(in(m, 0xBEu), 0x11);
    CHECK_EQ_HEX(in(m, 0xBEu), 0x22);
    CHECK_EQ_HEX(in(m, 0xBEu), 0x33);
    CHECK_EQ_HEX(v->addr, 0x0002);
    /* Code 3 writes CRAM, 32 bytes, and leaves VRAM alone. */
    uint8_t vram0 = v->vram[0x001F];
    control(m, 0x1Fu, 0xC0u);
    out(m, 0xBEu, 0x3Fu);
    out(m, 0xBEu, 0x2Au);
    CHECK_EQ_HEX(v->cram[31], 0x3F);
    CHECK_EQ_HEX(v->cram[0], 0x2A);
    CHECK_EQ_HEX(v->vram[0x001F], vram0);
    /* Codes 0 and 2 write VRAM too: code 0 has already stepped past its
       read-ahead byte, and code 2 leaves its two bytes as the address. */
    control(m, 0x40u, 0x00u);
    out(m, 0xBEu, 0x5Du);
    CHECK_EQ_HEX(v->vram[0x0041], 0x5D);
    control(m, 0x00u, 0x8Au);
    out(m, 0xBEu, 0x6Eu);
    CHECK_EQ_HEX(v->vram[0x0A00], 0x6E);
    cogmaster_destroy(m);
}

/* The frame flag rises at tick 607 of line 192 and no other tick; a status
   read returns it with bits 4-0 set, clears it and releases /INT; /INT is
   the flag AND R1 IE0 (SMS-VDP sections 2.6 and 4.6). */
static void test_status(void)
{
    cogmaster *m = load_sms();
    struct cogmaster__vdp *v = &m->vdp;
    CHECK_EQ_HEX(v->status, 0x00);
    put_beam(m, 191u, 600u);
    pass(m, 20u);
    CHECK_EQ_HEX(v->status, 0x00);
    put_beam(m, 192u, 600u);
    pass(m, 6u);
    CHECK_EQ_U64(m->bus.t, 606);
    CHECK_EQ_HEX(v->status, 0x00);
    pass(m, 1u);
    CHECK_EQ_HEX(v->status, 0x80);
    CHECK_EQ_U64(m->z80.int_line, 0);
    CHECK_EQ_HEX(in(m, 0xBFu), 0x9F);
    CHECK_EQ_HEX(v->status, 0x00);
    CHECK_EQ_HEX(in(m, 0xBFu), 0x1F);
    /* Passing the tick inside a longer stretch raises it too. */
    put_beam(m, 192u, 550u);
    pass(m, 69u);
    CHECK_EQ_HEX(v->status, 0x80);
    /* /INT follows IE0 while the flag is pending. */
    control(m, 0x20u, 0x81u);
    CHECK_EQ_U64(m->z80.int_line, 1);
    control(m, 0x00u, 0x81u);
    CHECK_EQ_U64(m->z80.int_line, 0);
    control(m, 0x20u, 0x81u);
    CHECK_EQ_U64(m->z80.int_line, 1);
    CHECK_EQ_HEX(in(m, 0xBFu), 0x9F);
    CHECK_EQ_U64(m->z80.int_line, 0);
    /* With IE0 set, the next rise drives /INT at once. */
    put_beam(m, 192u, 606u);
    pass(m, 1u);
    CHECK_EQ_U64(m->z80.int_line, 1);
    cogmaster_destroy(m);
}

/* Each port's response everywhere in $00-$FF: the export Master System
   mirrors $7E/$7F over $40-$7F and $BE/$BF over $80-$BF by A0; the
   Japanese console and the Game Gear answer only at $7E, $7F, $BE and $BF.
   Any other port reads $FF and a write to it changes nothing in the VDP. */
static void port_decode(uint32_t machine, uint32_t region, int mirrors)
{
    static const uint8_t prog[] = {0xF3u, 0x76u};
    cogmaster *m = load_profile(prog, sizeof prog, machine, region, COGMASTER_TV_NTSC);
    struct cogmaster__vdp *v = &m->vdp;
    struct cogmaster__vdp before;
    for (uint32_t p = 0; p < 256u; p++) {
        int vdp = (p & 0xC0u) == 0x40u || (p & 0xC0u) == 0x80u;
        if (!mirrors) {
            vdp = p == 0x7Eu || p == 0x7Fu || p == 0xBEu || p == 0xBFu;
        }
        /* Line 50, tick 400: V counter $32, H counter $64. */
        put_beam(m, 50u, 400u);
        (void)in(m, 0xBFu);
        if (!vdp) {
            memcpy(&before, v, sizeof before);
            CHECK_EQ_HEX(in(m, (uint16_t)p), 0xFF);
            out(m, (uint16_t)p, 0x81u);
            out(m, (uint16_t)p, 0x81u);
            if (memcmp(&before, v, sizeof before) != 0) {
                fprintf(stderr, "port $%02lX changed the VDP\n", (unsigned long)p);
                check_failures++;
            }
            continue;
        }
        if ((p & 0xC0u) == 0x40u) {
            CHECK_EQ_HEX(in(m, (uint16_t)p), (p & 1u) ? 0x64 : 0x32);
        } else if (p & 1u) {
            out(m, (uint16_t)p, 0x00u);
            CHECK_EQ_U64(v->latch, 1);
            CHECK_EQ_HEX(in(m, (uint16_t)p) & 0x1Fu, 0x1F);
            CHECK_EQ_U64(v->latch, 0);
        } else {
            control(m, 0x00u, 0x40u);
            out(m, (uint16_t)p, (uint8_t)p);
            CHECK_EQ_HEX(v->vram[0], p);
        }
    }
    cogmaster_destroy(m);
}

static void test_port_decode(void)
{
    port_decode(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, 1);
    port_decode(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, 0);
    port_decode(COGMASTER_MACHINE_GG, COGMASTER_REGION_AUTO, 0);
}

/* Steps the CPU once and brings the beam up to the step's end. */
static void step(cogmaster *m)
{
    cogmaster__z80_step(m);
    cogmaster__bus_sync(m);
}

/* Pause pressed during line 100 raises the NMI at /HSYNC's rise, tick 611
   of line 260 (NTSC) or 311 (PAL), the start of the frame's last line. The
   V counter has stepped to that last line's $FF at tick 604, and steps to
   $00 only at tick 604 of the next line, so the handler's IN A,($7E)
   reads $FF; an NMI one line later would read $00.
     $0000  DI; HALT; JR $0001
     $0066  IN A,($7E); LD ($C100),A; RETN */
static void pause_nmi(uint32_t tv, uint32_t lines)
{
    static const uint8_t prog[] = {
        0xF3u, 0x76u, 0x18u, 0xFDu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xDBu, 0x7Eu,
        0x32u, 0x00u, 0xC1u, 0xEDu, 0x45u};
    _Static_assert(sizeof prog == 0x66u + 7u, "the handler starts at $0066");
    cogmaster *m =
        load_profile(prog, sizeof prog, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, tv);
    const uint32_t nmi_line = lines - 2u;
    m->ram[0x100] = 0x55u;
    /* Line 100 of the first frame, past this frame's NMI line. */
    uint32_t guard = 0;
    while (m->bus.line != 100u && guard++ < STEP_GUARD) {
        step(m);
        CHECK_EQ_U64(m->z80.nmi_pending, 0);
    }
    CHECK_EQ_U64(m->bus.line, 100);
    m->vdp.pause = 1u;
    /* The NMI tick, counted from the origin at line 192 tick 607. */
    const uint64_t at =
        (uint64_t)(lines - ORIGIN_LINE + nmi_line) * LINE_TICKS + (NMI_TICK - ORIGIN_TICK);
    uint64_t before = m->ticks;
    guard = 0;
    while (!m->z80.nmi_pending && guard++ < STEP_GUARD) {
        before = m->ticks;
        step(m);
    }
    CHECK_EQ_U64(m->z80.nmi_pending, 1);
    CHECK(before < at && at <= m->ticks);
    CHECK_EQ_U64(m->bus.line, nmi_line);
    CHECK_EQ_U64(m->vdp.pause, 0);
    /* The next step accepts it. */
    uint64_t start = m->ticks;
    step(m);
    CHECK_EQ_HEX(m->z80.pc, 0x0066);
    CHECK(start >= at);
    CHECK(m->ticks <= at + MAX_STEP_TICKS + NMI_ACCEPT_TICKS);
    guard = 0;
    while (m->z80.pc >= 0x0066u && guard++ < 16u) {
        step(m);
    }
    CHECK_EQ_HEX(m->ram[0x100], 0xFF);
    /* One press, one NMI: a whole frame more brings none. */
    guard = 0;
    uint64_t end = m->ticks + (uint64_t)lines * LINE_TICKS;
    while (m->ticks < end && guard++ < STEP_GUARD) {
        step(m);
        CHECK(m->z80.pc < 0x0066u);
    }
    CHECK_EQ_U64(m->z80.nmi_pending, 0);
    /* A line later the same read gives $00. */
    put_beam(m, nmi_line + 1u, NMI_TICK + 30u);
    CHECK_EQ_HEX(in(m, 0x7Eu), 0x00);
    cogmaster_destroy(m);
}

static void test_pause_nmi(void)
{
    pause_nmi(COGMASTER_TV_NTSC, LINES_NTSC);
    pause_nmi(COGMASTER_TV_PAL, LINES_PAL);
    /* The Game Gear has no Pause button: in its own mode Start is only a
       port bit (SMS-SYSTEM section 7), so a latched press raises nothing. */
    static const uint8_t prog[] = {0xF3u, 0x76u};
    cogmaster *m = load_profile(prog, sizeof prog, COGMASTER_MACHINE_GG, COGMASTER_REGION_AUTO,
                                COGMASTER_TV_NTSC);
    m->vdp.pause = 1u;
    for (uint32_t i = 0; i < 2u; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
    CHECK_EQ_U64(m->z80.nmi_pending, 0);
    CHECK_EQ_HEX(m->z80.pc, 0x0002);
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"control_port", test_control_port},   {"data_port", test_data_port},
    {"hcounter", test_hcounter},           {"pause_nmi", test_pause_nmi},
    {"port_decode", test_port_decode},     {"status", test_status},
    {"vcounter.ntsc", test_vcounter_ntsc}, {"vcounter.pal", test_vcounter_pal},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-vdp <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-vdp: unknown case %s\n", argv[1]);
    return 2;
}
