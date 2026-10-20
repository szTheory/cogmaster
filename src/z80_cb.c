/* The Z80 CB page and its DD CB / FD CB forms, decoded by the fields
   x = op >> 6, y = op >> 3 & 7 and z = op & 7 (Cristian Dinu, "Decoding Z80
   opcodes"). Flags follow Sean Young, "The Undocumented Z80 Documented"
   sections 4 and 6; M-cycle layouts follow Zilog UM0080 and the
   SingleStepTests z80 cycle streams. */
#include "z80_cycle.h"

enum { RF = COGMASTER__Z80_F, RH = COGMASTER__Z80_H };

/* The rotate or shift y on v: RLC, RRC, RL, RR, SLA, SRA, SLL, SRL. SLL
   shifts a 1 into bit 0. C is the bit shifted out, H and N clear, the rest
   from the result. */
static uint8_t rot(struct cogmaster__z80 *z, unsigned y, uint8_t v)
{
    unsigned c = z->reg[RF] & COGMASTER__Z80_CF;
    unsigned in, out, r;
    if (y & 1u) {
        out = v & 1u;
        in = y == 1 ? out << 7 : y == 3 ? c << 7 : y == 5 ? (v & 0x80u) : 0u;
        r = (unsigned)v >> 1 | in;
    } else {
        out = (unsigned)v >> 7;
        in = y == 0 ? out : y == 2 ? c : y == 6 ? 1u : 0u;
        r = ((unsigned)v << 1 | in) & 0xFFu;
    }
    cogmaster__z80_setf(z, (uint8_t)(cogmaster__z80_sz53p((uint8_t)r) | out));
    return (uint8_t)r;
}

/* BIT y,v: Z and P/V when the bit is clear, S when bit 7 is set, H set,
   C kept. Y and X come from xy: the register itself, or WZ's high byte for
   the memory forms (Sean Young section 4.1). */
static void bit(struct cogmaster__z80 *z, unsigned y, uint8_t v, uint8_t xy)
{
    unsigned b = v & (1u << y);
    cogmaster__z80_setf(z, (uint8_t)((z->reg[RF] & COGMASTER__Z80_CF) | COGMASTER__Z80_HF |
                                     (xy & 0x28u) |
                                     (b ? (b & COGMASTER__Z80_SF) : (COGMASTER__Z80_ZF | COGMASTER__Z80_PF))));
}

/* Rotate (x = 0), RES (x = 2) or SET (x = 3) bit y of v. RES and SET leave
   F, and so Q, alone. */
static uint8_t apply(struct cogmaster__z80 *z, unsigned x, unsigned y, uint8_t v)
{
    if (x == 0) {
        return rot(z, y, v);
    }
    return x == 2 ? (uint8_t)(v & ~(1u << y)) : (uint8_t)(v | 1u << y);
}

/* DD CB d op and FD CB d op: d and op are plain memory reads, so R counts
   only the two opcode fetches; two internal T-states follow op. The
   operand is (IX+d) or (IY+d), which also goes to WZ; the result of a
   rotate, RES or SET is copied to register z unless z = 6, and BIT takes
   Y and X from WZ's high byte (Sean Young section 6). */
static void indexed(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    unsigned hi = z->idx == COGMASTER__Z80_IDX_IX ? COGMASTER__Z80_IXH : COGMASTER__Z80_IYH;
    uint8_t d = cogmaster__z80_mread(m, z->pc);
    z->pc = (uint16_t)(z->pc + 1u);
    uint8_t op = cogmaster__z80_mread(m, z->pc);
    cogmaster__z80_idle(m, z->pc, 2);
    z->pc = (uint16_t)(z->pc + 1u);
    uint16_t a = (uint16_t)(cogmaster__z80_pair(z, hi) + (uint16_t)(int8_t)d);
    unsigned x = op >> 6, y = op >> 3 & 7u, s = op & 7u;
    z->wz = a;
    uint8_t v = cogmaster__z80_mread(m, a);
    cogmaster__z80_idle(m, a, 1);
    if (x == 1) {
        bit(z, y, v, (uint8_t)(a >> 8));
        return;
    }
    v = apply(z, x, y, v);
    cogmaster__z80_mwrite(m, a, v);
    if (s != 6) {
        z->reg[s] = v;
    }
}

void cogmaster__z80_cb(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    if (z->idx != COGMASTER__Z80_IDX_NONE) {
        indexed(m);
        return;
    }
    uint8_t op = cogmaster__z80_m1(m);
    unsigned x = op >> 6, y = op >> 3 & 7u, s = op & 7u;
    if (s != 6) {
        uint8_t v = z->reg[s];
        if (x == 1) {
            bit(z, y, v, v);
        } else {
            z->reg[s] = apply(z, x, y, v);
        }
        return;
    }
    /* (HL): the read takes one extra T-state; BIT then stops */
    uint16_t a = cogmaster__z80_pair(z, RH);
    uint8_t v = cogmaster__z80_mread(m, a);
    cogmaster__z80_idle(m, a, 1);
    if (x == 1) {
        bit(z, y, v, (uint8_t)(z->wz >> 8));
        return;
    }
    cogmaster__z80_mwrite(m, a, apply(z, x, y, v));
}
