/* Z80 behaviour the SingleStepTests vectors do not cover: interrupt and NMI
   acceptance, the EI delay, HALT exit, prefix runs, R across prefixes and
   the ED holes. Each case scripts /INT and /NMI around a few instructions
   in a flat 64 KiB memory. Usage: cogmaster-test-z80int CASE. The bus seam
   is defined here, as in the vector harness, so the library is not linked. */
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "internal.h"

static uint8_t mem[65536];
static unsigned ed_calls;
static uint8_t ed_op;
static cogmaster machine;

uint8_t cogmaster__bus_fetch(cogmaster *m, uint16_t addr, uint16_t refresh)
{
    (void)m;
    (void)refresh;
    return mem[addr];
}

uint8_t cogmaster__bus_read(cogmaster *m, uint16_t addr)
{
    (void)m;
    return mem[addr];
}

void cogmaster__bus_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    (void)m;
    mem[addr] = v;
}

uint8_t cogmaster__bus_in(cogmaster *m, uint16_t port)
{
    (void)m;
    (void)port;
    return 0xFF;
}

void cogmaster__bus_out(cogmaster *m, uint16_t port, uint8_t v)
{
    (void)m;
    (void)port;
    (void)v;
}

void cogmaster__bus_ed_undefined(cogmaster *m, uint8_t op)
{
    (void)m;
    ed_calls++;
    ed_op = op;
}

/* A fresh CPU at PC 0x0100 with SP 0x8000 and the bytes code[0..n). */
static struct cogmaster__z80 *setup(const uint8_t *code, size_t n)
{
    memset(mem, 0, sizeof mem);
    memset(&machine, 0, sizeof machine);
    if (n != 0) {
        memcpy(&mem[0x0100], code, n);
    }
    ed_calls = 0;
    machine.z80.pc = 0x0100;
    machine.z80.sp = 0x8000;
    return &machine.z80;
}

/* One step; returns the T-states it took. */
static uint32_t step(void)
{
    uint64_t before = machine.ticks;
    cogmaster__z80_step(&machine);
    return (uint32_t)((machine.ticks - before) / 3u);
}

static uint16_t top_of_stack(const struct cogmaster__z80 *z)
{
    return (uint16_t)(mem[(uint16_t)(z->sp + 1u)] << 8 | mem[z->sp]);
}

static void im1(void)
{
    static const uint8_t code[] = {0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 1;
    z->iff1 = z->iff2 = 1;
    z->r = 0x85;
    z->q = 0x55;
    z->int_line = 1;
    CHECK_EQ_U64(step(), 13);
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_HEX(z->wz, 0x0038);
    CHECK_EQ_HEX(z->sp, 0x7FFE);
    CHECK_EQ_HEX(top_of_stack(z), 0x0100);
    CHECK_EQ_U64(z->iff1, 0);
    CHECK_EQ_U64(z->iff2, 0);
    CHECK_EQ_U64(z->q, 0);
    CHECK_EQ_HEX(z->r, 0x86);
}

static void im0(void)
{
    static const uint8_t code[] = {0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 0;
    z->iff1 = z->iff2 = 1;
    z->int_line = 1;
    CHECK_EQ_U64(step(), 13);
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_HEX(top_of_stack(z), 0x0100);
    CHECK_EQ_U64(z->iff1, 0);
}

static void im2(void)
{
    static const uint8_t code[] = {0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 2;
    z->i = 0x12;
    mem[0x12FF] = 0x34;
    mem[0x1300] = 0x56;
    z->iff1 = z->iff2 = 1;
    z->int_line = 1;
    CHECK_EQ_U64(step(), 19);
    CHECK_EQ_HEX(z->pc, 0x5634);
    CHECK_EQ_HEX(z->wz, 0x5634);
    CHECK_EQ_HEX(top_of_stack(z), 0x0100);
    CHECK_EQ_U64(z->iff1, 0);
}

/* EI; NOP with /INT held: the NOP runs before acceptance. A run of EIs
   defers acceptance until the instruction after the last one. */
static void ei_delay(void)
{
    static const uint8_t code[] = {0xFB, 0x00, 0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 1;
    z->int_line = 1;
    step();
    CHECK_EQ_HEX(z->pc, 0x0101);
    step();
    CHECK_EQ_HEX(z->pc, 0x0102);
    CHECK_EQ_U64(step(), 13);
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_HEX(top_of_stack(z), 0x0102);

    static const uint8_t run[] = {0xFB, 0xFB, 0xFB, 0x00};
    z = setup(run, sizeof run);
    z->im = 1;
    z->int_line = 1;
    for (uint16_t k = 1; k <= 4; k++) {
        step();
        CHECK_EQ_HEX(z->pc, 0x0100 + k);
    }
    step();
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_HEX(top_of_stack(z), 0x0104);
}

static void di(void)
{
    static const uint8_t code[] = {0xFB, 0xF3, 0x00, 0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 1;
    z->int_line = 1;
    for (uint16_t k = 1; k <= 4; k++) {
        step();
        CHECK_EQ_HEX(z->pc, 0x0100 + k);
    }
    CHECK_EQ_U64(z->iff1, 0);
}

/* An NMI edge is accepted under DI too; it clears IFF1 and keeps IFF2. */
static void nmi(void)
{
    static const uint8_t code[] = {0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->nmi_pending = 1;
    CHECK_EQ_U64(step(), 11);
    CHECK_EQ_HEX(z->pc, 0x0066);
    CHECK_EQ_HEX(top_of_stack(z), 0x0100);
    CHECK_EQ_U64(z->nmi_pending, 0);
    CHECK_EQ_U64(z->iff1, 0);
    CHECK_EQ_U64(z->iff2, 0);

    z = setup(code, sizeof code);
    z->iff1 = z->iff2 = 1;
    z->r = 0x7F;
    z->nmi_pending = 1;
    z->int_line = 1;
    CHECK_EQ_U64(step(), 11);
    CHECK_EQ_HEX(z->pc, 0x0066);
    CHECK_EQ_U64(z->iff1, 0);
    CHECK_EQ_U64(z->iff2, 1);
    CHECK_EQ_HEX(z->r, 0x00);

    /* EI delays /INT only: an NMI right after EI is accepted. */
    static const uint8_t ei[] = {0xFB, 0x00};
    z = setup(ei, sizeof ei);
    step();
    z->nmi_pending = 1;
    CHECK_EQ_U64(step(), 11);
    CHECK_EQ_HEX(top_of_stack(z), 0x0101);
    CHECK_EQ_U64(z->iff1, 0);
    CHECK_EQ_U64(z->iff2, 1);
}

static void retn(void)
{
    static const uint8_t code[] = {0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    mem[0x0066] = 0xED;
    mem[0x0067] = 0x45;
    z->iff1 = z->iff2 = 1;
    z->nmi_pending = 1;
    step();
    CHECK_EQ_U64(z->iff1, 0);
    CHECK_EQ_U64(step(), 14);
    CHECK_EQ_HEX(z->pc, 0x0100);
    CHECK_EQ_U64(z->iff1, 1);
    CHECK_EQ_U64(z->iff2, 1);
}

/* NMOS: an interrupt accepted right after LD A,I clears P/V in F. */
static void ld_a_i(void)
{
    static const uint8_t code[] = {0xED, 0x57, 0xED, 0x57, 0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 1;
    z->i = 0x80;
    z->iff1 = z->iff2 = 1;
    step();
    CHECK_EQ_U64(z->p, 1);
    CHECK(z->reg[COGMASTER__Z80_F] & 0x04u);
    z->int_line = 1;
    step();
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_U64(z->p, 0);
    CHECK_EQ_HEX(z->reg[COGMASTER__Z80_F] & 0x04u, 0);
}

/* HALT then acceptance: the pushed address is the one after HALT. */
static void halt_exit(void)
{
    static const uint8_t code[] = {0x76, 0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 1;
    z->iff1 = z->iff2 = 1;
    step();
    CHECK_EQ_U64(z->halted, 1);
    CHECK_EQ_U64(step(), 4);
    CHECK_EQ_HEX(z->pc, 0x0101);
    z->int_line = 1;
    step();
    CHECK_EQ_U64(z->halted, 0);
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_HEX(top_of_stack(z), 0x0101);
}

/* An interrupt between LDIR iterations pushes the LDIR's own address. */
static void block_op(void)
{
    static const uint8_t code[] = {0xED, 0xB0, 0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->im = 1;
    z->iff1 = z->iff2 = 1;
    z->reg[COGMASTER__Z80_C] = 3;
    z->reg[COGMASTER__Z80_H] = 0x20;
    z->reg[COGMASTER__Z80_D] = 0x30;
    CHECK_EQ_U64(step(), 21);
    CHECK_EQ_HEX(z->pc, 0x0100);
    z->int_line = 1;
    step();
    CHECK_EQ_HEX(z->pc, 0x0038);
    CHECK_EQ_HEX(top_of_stack(z), 0x0100);
    CHECK_EQ_HEX(z->reg[COGMASTER__Z80_C], 2);
}

/* 65,536 DD bytes: each a 4 T-state step that adds 1 to R's low 7 bits
   and keeps bit 7; /INT is never accepted between a prefix and the next
   byte. */
static void prefix_run(void)
{
    struct cogmaster__z80 *z = setup(NULL, 0);
    memset(mem, 0xDD, sizeof mem);
    z->im = 1;
    z->iff1 = z->iff2 = 1;
    z->r = 0x80;
    CHECK_EQ_U64(step(), 4);
    z->int_line = 1;
    unsigned bad = 0;
    for (uint32_t k = 1; k < 65536u; k++) {
        uint16_t pc = z->pc;
        if (step() != 4 || z->pc != (uint16_t)(pc + 1u) || z->idx != COGMASTER__Z80_IDX_IX) {
            bad++;
        }
    }
    CHECK_EQ_U64(bad, 0);
    CHECK_EQ_HEX(z->pc, 0x0100);
    CHECK_EQ_HEX(z->r, 0x80);
    CHECK_EQ_U64(z->iff1, 1);
}

/* R counts M1 cycles: DD CB d op has two, FD op two, so each adds 2. */
static void r_prefixes(void)
{
    static const uint8_t code[] = {0xDD, 0xCB, 0x01, 0x06, 0xFD, 0x23, 0xFD, 0xDD, 0x00};
    struct cogmaster__z80 *z = setup(code, sizeof code);
    z->r = 0xFE;
    step();
    step();
    CHECK_EQ_HEX(z->pc, 0x0104);
    CHECK_EQ_HEX(z->r, 0x80);
    step();
    step();
    CHECK_EQ_HEX(z->pc, 0x0106);
    CHECK_EQ_HEX(z->r, 0x82);
    step();
    step();
    step();
    CHECK_EQ_HEX(z->pc, 0x0109);
    CHECK_EQ_HEX(z->r, 0x85);
}

/* Every undefined ED opcode is an 8 T-state no-operation that adds 2 to R,
   changes nothing else and reaches the seam once with its second byte. */
static void holes(void)
{
    unsigned n = 0;
    for (unsigned op = 0; op < 256; op++) {
        unsigned x = op >> 6, y = op >> 3 & 7u, s = op & 7u;
        if (x == 1 || (x == 2 && y >= 4 && s <= 3)) {
            continue;
        }
        n++;
        const uint8_t code[] = {0xED, (uint8_t)op};
        struct cogmaster__z80 *z = setup(code, sizeof code);
        for (unsigned k = 0; k < COGMASTER__Z80_REGS; k++) {
            z->reg[k] = (uint8_t)(0x11u * k + 3u);
        }
        z->wz = 0x1234;
        z->i = 0x56;
        z->r = 0x7F;
        z->iff2 = 1;
        z->im = 2;
        struct cogmaster__z80 want = *z;
        want.pc = 0x0102;
        want.r = 0x01;
        CHECK_EQ_U64(step(), 8);
        CHECK_EQ_U64(ed_calls, 1);
        CHECK_EQ_HEX(ed_op, op);
        CHECK(memcmp(z, &want, sizeof want) == 0);
    }
    CHECK_EQ_U64(n, 176);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*run)(void);
    } cases[] = {{"im0", im0},
                 {"im1", im1},
                 {"im2", im2},
                 {"ei_delay", ei_delay},
                 {"di", di},
                 {"nmi", nmi},
                 {"retn", retn},
                 {"ld_a_i", ld_a_i},
                 {"halt_exit", halt_exit},
                 {"block_op", block_op},
                 {"prefix_run", prefix_run},
                 {"r_prefixes", r_prefixes},
                 {"holes", holes}};
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-z80int CASE\n");
        return 2;
    }
    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        if (strcmp(argv[1], cases[k].name) == 0) {
            cases[k].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-z80int: unknown case %s\n", argv[1]);
    return 2;
}
