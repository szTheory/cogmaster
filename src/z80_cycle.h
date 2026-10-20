/* Z80 bus cycle primitives. Each advances m->ticks by 3 per T-state (one
   T-state is 3 master clock ticks on the SMS, SMS-SYSTEM section 1) and calls
   the bus once, at the cycle's data T-state: T3 of an M1 and of a memory
   read or write, T4 of an I/O cycle, which includes the automatic wait state.
   T-state layouts: Zilog UM0080 "Z80 CPU Timing"; SingleStepTests z80 cycle
   streams. Only z80*.c include this file. */
#ifndef COGMASTER_Z80_CYCLE_H
#define COGMASTER_Z80_CYCLE_H

#include "internal.h"

/* Opcode fetch, 4 T-states. The refresh address I<<8|R goes out on T3 with
   R as it was before this fetch; then R's low 7 bits count up and bit 7
   stays (UM0080, "Memory Refresh Register"). */
static inline uint8_t cogmaster__z80_m1(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t refresh = (uint16_t)(z->i << 8 | z->r);
    m->ticks += 3u * 2u;
    uint8_t op = cogmaster__bus_fetch(m, z->pc, refresh);
    m->ticks += 3u * 2u;
    z->pc = (uint16_t)(z->pc + 1u);
    z->r = (uint8_t)((z->r & 0x80u) | ((z->r + 1u) & 0x7Fu));
    return op;
}

/* Memory read, 3 T-states. */
static inline uint8_t cogmaster__z80_mread(cogmaster *m, uint16_t addr)
{
    m->ticks += 3u * 2u;
    uint8_t v = cogmaster__bus_read(m, addr);
    m->ticks += 3u;
    return v;
}

/* Memory write, 3 T-states. The data is on the bus from T2; the write is
   stamped at T3 like a read. */
static inline void cogmaster__z80_mwrite(cogmaster *m, uint16_t addr, uint8_t v)
{
    m->ticks += 3u * 2u;
    cogmaster__bus_write(m, addr, v);
    m->ticks += 3u;
}

/* I/O read, 4 T-states with the automatic wait state. */
static inline uint8_t cogmaster__z80_ioread(cogmaster *m, uint16_t port)
{
    m->ticks += 3u * 3u;
    uint8_t v = cogmaster__bus_in(m, port);
    m->ticks += 3u;
    return v;
}

/* I/O write, 4 T-states with the automatic wait state. */
static inline void cogmaster__z80_iowrite(cogmaster *m, uint16_t port, uint8_t v)
{
    m->ticks += 3u * 3u;
    cogmaster__bus_out(m, port, v);
    m->ticks += 3u;
}

/* n internal T-states with no bus cycle; addr stays on the address bus. */
static inline void cogmaster__z80_idle(cogmaster *m, uint16_t addr, uint32_t n)
{
    (void)addr;
    m->ticks += 3u * n;
}

/* Writes F, and Q with it: Q holds the F an instruction wrote, even when the
   value is unchanged (SingleStepTests z80 README, "q"). */
static inline void cogmaster__z80_setf(struct cogmaster__z80 *z, uint8_t f)
{
    z->reg[COGMASTER__Z80_F] = f;
    z->q = f;
}

/* F bits (UM0080, "Flag register"; Sean Young, "The Undocumented Z80
   Documented" section 4): Y and X are copies of result bits 5 and 3. */
enum {
    COGMASTER__Z80_CF = 0x01,
    COGMASTER__Z80_NF = 0x02,
    COGMASTER__Z80_PF = 0x04,
    COGMASTER__Z80_XF = 0x08,
    COGMASTER__Z80_HF = 0x10,
    COGMASTER__Z80_YF = 0x20,
    COGMASTER__Z80_ZF = 0x40,
    COGMASTER__Z80_SF = 0x80
};

/* S, Y and X from bits 7, 5 and 3 of v, and Z when v is zero. */
static inline uint8_t cogmaster__z80_sz53(uint8_t v)
{
    return (uint8_t)((v & 0xA8u) | (v ? 0u : COGMASTER__Z80_ZF));
}

/* As sz53, with P/V set when v has an even number of 1 bits. */
static inline uint8_t cogmaster__z80_sz53p(uint8_t v)
{
    uint8_t x = (uint8_t)(v ^ v >> 4);
    x = (uint8_t)(x ^ x >> 2);
    x = (uint8_t)(x ^ x >> 1);
    return (uint8_t)(cogmaster__z80_sz53(v) | ((~(unsigned)x & 1u) << 2));
}

/* The eight accumulator operations by the opcode's y field: ADD, ADC, SUB,
   SBC, AND, XOR, OR, CP. H is the carry out of bit 3, found as bit 4 of
   a ^ b ^ result; V is signed overflow. CP leaves A and takes Y and X from
   the operand (Sean Young section 4.3). */
static inline void cogmaster__z80_alu(struct cogmaster__z80 *z, unsigned op, uint8_t b)
{
    uint8_t a = z->reg[COGMASTER__Z80_A];
    unsigned c = (op == 1 || op == 3) ? (z->reg[COGMASTER__Z80_F] & 1u) : 0u;
    unsigned r;
    uint8_t f;
    switch (op) {
    case 0:
    case 1:
        r = a + b + c;
        f = (uint8_t)(cogmaster__z80_sz53((uint8_t)r) | ((a ^ b ^ r) & 0x10u) |
                      (((a ^ r) & (b ^ r) & 0x80u) >> 5) | (r >> 8));
        break;
    case 4:
        r = a & b;
        f = (uint8_t)(cogmaster__z80_sz53p((uint8_t)r) | COGMASTER__Z80_HF);
        break;
    case 5:
        r = a ^ b;
        f = cogmaster__z80_sz53p((uint8_t)r);
        break;
    case 6:
        r = a | b;
        f = cogmaster__z80_sz53p((uint8_t)r);
        break;
    default:
        r = a - b - c;
        f = (uint8_t)(cogmaster__z80_sz53((uint8_t)r) | ((a ^ b ^ r) & 0x10u) |
                      (((a ^ b) & (a ^ r) & 0x80u) >> 5) | COGMASTER__Z80_NF | ((r >> 8) & 1u));
        if (op == 7) {
            f = (uint8_t)((f & 0xD7u) | (b & 0x28u));
            r = a;
        }
        break;
    }
    z->reg[COGMASTER__Z80_A] = (uint8_t)r;
    cogmaster__z80_setf(z, f);
}

/* INC and DEC of an 8-bit value; C is kept. */
static inline uint8_t cogmaster__z80_inc8(struct cogmaster__z80 *z, uint8_t v)
{
    uint8_t r = (uint8_t)(v + 1u);
    cogmaster__z80_setf(z,
                        (uint8_t)((z->reg[COGMASTER__Z80_F] & COGMASTER__Z80_CF) |
                                  cogmaster__z80_sz53(r) | ((r & 0x0Fu) ? 0u : COGMASTER__Z80_HF) |
                                  (r == 0x80u ? COGMASTER__Z80_PF : 0u)));
    return r;
}

static inline uint8_t cogmaster__z80_dec8(struct cogmaster__z80 *z, uint8_t v)
{
    uint8_t r = (uint8_t)(v - 1u);
    cogmaster__z80_setf(z, (uint8_t)((z->reg[COGMASTER__Z80_F] & COGMASTER__Z80_CF) |
                                     cogmaster__z80_sz53(r) | COGMASTER__Z80_NF |
                                     ((r & 0x0Fu) == 0x0Fu ? COGMASTER__Z80_HF : 0u) |
                                     (r == 0x7Fu ? COGMASTER__Z80_PF : 0u)));
    return r;
}

/* A register pair from its high byte's index; the low byte follows it. */
static inline uint16_t cogmaster__z80_pair(const struct cogmaster__z80 *z, unsigned hi)
{
    return (uint16_t)(z->reg[hi] << 8 | z->reg[hi + 1u]);
}

static inline void cogmaster__z80_set_pair(struct cogmaster__z80 *z, unsigned hi, uint16_t v)
{
    z->reg[hi] = (uint8_t)(v >> 8);
    z->reg[hi + 1u] = (uint8_t)v;
}

#endif /* COGMASTER_Z80_CYCLE_H */
