/* The Z80 ED page, decoded by x = op >> 6, y = op >> 3 & 7, z = op & 7
   (Cristian Dinu, "Decoding Z80 opcodes"). A DD or FD prefix before ED has
   no effect, so register pairs here never remap to IX or IY. Flags and WZ
   follow Sean Young, "The Undocumented Z80 Documented" sections 4 and 5 and
   the MEMPTR document (zx.pk.ru, 2006); M-cycle layouts follow Zilog UM0080
   and the SingleStepTests z80 cycle streams. */
#include "z80_cycle.h"

enum {
    RB = COGMASTER__Z80_B,
    RC = COGMASTER__Z80_C,
    RH = COGMASTER__Z80_H,
    RL = COGMASTER__Z80_L,
    RF = COGMASTER__Z80_F,
    RA = COGMASTER__Z80_A
};

enum {
    CF = COGMASTER__Z80_CF,
    NF = COGMASTER__Z80_NF,
    PF = COGMASTER__Z80_PF,
    HF = COGMASTER__Z80_HF,
    ZF = COGMASTER__Z80_ZF,
    SF = COGMASTER__Z80_SF
};

/* The refresh address on the bus during internal T-states after a fetch. */
static uint16_t ir(const struct cogmaster__z80 *z)
{
    return (uint16_t)(z->i << 8 | z->r);
}

/* Register pair p: BC, DE, HL, SP, never IX or IY. */
static uint16_t rp(const struct cogmaster__z80 *z, unsigned p)
{
    return p == 3 ? z->sp : cogmaster__z80_pair(z, 2u * p);
}

static void set_rp(struct cogmaster__z80 *z, unsigned p, uint16_t v)
{
    if (p == 3) {
        z->sp = v;
    } else {
        cogmaster__z80_set_pair(z, 2u * p, v);
    }
}

/* PF when v has an even number of 1 bits. */
static uint8_t parity(uint8_t v)
{
    return (uint8_t)(cogmaster__z80_sz53p(v) & PF);
}

/* IN r,(C): WZ = BC + 1; flags from the byte, C kept. y = 6 sets only the
   flags. */
static void in_c(cogmaster *m, unsigned y)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t bc = cogmaster__z80_pair(z, RB);
    uint8_t v = cogmaster__z80_ioread(m, bc);
    z->wz = (uint16_t)(bc + 1u);
    if (y != 6) {
        z->reg[y] = v;
    }
    cogmaster__z80_setf(z, (uint8_t)((z->reg[RF] & CF) | cogmaster__z80_sz53p(v)));
}

/* OUT (C),r: WZ = BC + 1. y = 6 is OUT (C),0, which writes 0 on NMOS
   parts (msx.org forum, "Turbo-R and out(c),0", 2022). */
static void out_c(cogmaster *m, unsigned y)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t bc = cogmaster__z80_pair(z, RB);
    cogmaster__z80_iowrite(m, bc, y == 6 ? 0u : z->reg[y]);
    z->wz = (uint16_t)(bc + 1u);
}

/* SBC HL,rr and ADC HL,rr: seven internal T-states, WZ = HL + 1. H from
   the carry out of bit 11, V signed overflow, S, Y and X from the high
   byte, Z from all 16 bits. */
static void adc_hl(cogmaster *m, unsigned p, int sub)
{
    struct cogmaster__z80 *z = &m->z80;
    uint32_t a = cogmaster__z80_pair(z, RH), b = rp(z, p), c = z->reg[RF] & CF;
    uint32_t r = sub ? a - b - c : a + b + c;
    uint32_t v = sub ? (a ^ b) & (a ^ r) : (a ^ r) & (b ^ r);
    cogmaster__z80_idle(m, ir(z), 7);
    z->wz = (uint16_t)(a + 1u);
    cogmaster__z80_set_pair(z, RH, (uint16_t)r);
    cogmaster__z80_setf(z, (uint8_t)(((r >> 8) & 0xA8u) | ((r & 0xFFFFu) ? 0u : ZF) |
                                     (((a ^ b ^ r) >> 8) & HF) | ((v >> 13) & PF) |
                                     (sub ? NF : 0u) | ((r >> 16) & CF)));
}

/* LD (nn),rr and LD rr,(nn): WZ = nn + 1. */
static void ld_nn_rp(cogmaster *m, unsigned p, int load)
{
    struct cogmaster__z80 *z = &m->z80;
    uint8_t lo = cogmaster__z80_mread(m, z->pc);
    uint8_t hi = cogmaster__z80_mread(m, (uint16_t)(z->pc + 1u));
    uint16_t a = (uint16_t)(hi << 8 | lo);
    z->pc = (uint16_t)(z->pc + 2u);
    if (load) {
        uint8_t vl = cogmaster__z80_mread(m, a);
        uint8_t vh = cogmaster__z80_mread(m, (uint16_t)(a + 1u));
        set_rp(z, p, (uint16_t)(vh << 8 | vl));
    } else {
        uint16_t v = rp(z, p);
        cogmaster__z80_mwrite(m, a, (uint8_t)v);
        cogmaster__z80_mwrite(m, (uint16_t)(a + 1u), (uint8_t)(v >> 8));
    }
    z->wz = (uint16_t)(a + 1u);
}

/* RETN and RETI: IFF1 takes IFF2 (Sean Young section 5.3); PC and WZ take
   the popped address. */
static void retn(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    uint8_t lo = cogmaster__z80_mread(m, z->sp);
    uint8_t hi = cogmaster__z80_mread(m, (uint16_t)(z->sp + 1u));
    z->sp = (uint16_t)(z->sp + 2u);
    z->pc = (uint16_t)(hi << 8 | lo);
    z->wz = z->pc;
    z->iff1 = z->iff2;
}

/* LD A,I and LD A,R: one internal T-state; P/V from IFF2, H and N clear,
   C kept, and p marks the step so that an interrupt accepted next clears
   P/V (Sean Young section 5.3; MAME's Z80 notes on NMOS parts). */
static void ld_a_ir(cogmaster *m, uint8_t v)
{
    struct cogmaster__z80 *z = &m->z80;
    z->reg[RA] = v;
    cogmaster__z80_setf(z, (uint8_t)((z->reg[RF] & CF) | cogmaster__z80_sz53(v) |
                                     (z->iff2 ? PF : 0u)));
    z->p = 1;
}

/* RRD and RLD rotate a nibble between A and (HL): read, four internal
   T-states, write. WZ = HL + 1; flags from A, C kept. */
static void rxd(cogmaster *m, int left)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t a = cogmaster__z80_pair(z, RH);
    uint8_t v = cogmaster__z80_mread(m, a), acc = z->reg[RA];
    cogmaster__z80_idle(m, a, 4);
    uint8_t w;
    if (left) {
        w = (uint8_t)((unsigned)v << 4 | (acc & 0x0Fu));
        acc = (uint8_t)((acc & 0xF0u) | v >> 4);
    } else {
        w = (uint8_t)(acc << 4 | v >> 4);
        acc = (uint8_t)((acc & 0xF0u) | (v & 0x0Fu));
    }
    cogmaster__z80_mwrite(m, a, w);
    z->wz = (uint16_t)(a + 1u);
    z->reg[RA] = acc;
    cogmaster__z80_setf(z, (uint8_t)((z->reg[RF] & CF) | cogmaster__z80_sz53p(acc)));
}

/* A repeating block instruction's extra iteration: five internal T-states,
   PC back to the ED byte, WZ = PC + 1, and Y and X from PC's high byte
   (stardot "Z80 Protocol Decoder" thread, 2018; the MEMPTR document).
   Returns the new flags. */
static uint8_t repeat(cogmaster *m, uint16_t addr, uint8_t f)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, addr, 5);
    z->pc = (uint16_t)(z->pc - 2u);
    z->wz = (uint16_t)(z->pc + 1u);
    return (uint8_t)((f & 0xD7u) | ((z->pc >> 8) & 0x28u));
}

static uint16_t step_pair(struct cogmaster__z80 *z, unsigned hi, uint16_t d)
{
    uint16_t v = cogmaster__z80_pair(z, hi);
    cogmaster__z80_set_pair(z, hi, (uint16_t)(v + d));
    return v;
}

/* LDI, LDD, LDIR, LDDR: (DE) = (HL), two internal T-states on DE. Y and X
   from bits 3 and 1 of the byte plus A; P/V while BC is not zero. */
static void ldx(cogmaster *m, uint16_t d, int rep)
{
    struct cogmaster__z80 *z = &m->z80;
    uint8_t v = cogmaster__z80_mread(m, step_pair(z, RH, d));
    uint16_t de = step_pair(z, COGMASTER__Z80_D, d);
    cogmaster__z80_mwrite(m, de, v);
    cogmaster__z80_idle(m, de, 2);
    uint16_t bc = (uint16_t)(step_pair(z, RB, 0xFFFF) - 1u);
    uint8_t n = (uint8_t)(v + z->reg[RA]);
    uint8_t f = (uint8_t)((z->reg[RF] & (SF | ZF | CF)) | (bc ? PF : 0u) | (n & 0x08u) |
                          ((n & 0x02u) << 4));
    if (rep && bc) {
        f = repeat(m, de, f);
    }
    cogmaster__z80_setf(z, f);
}

/* CPI, CPD, CPIR, CPDR: compare A with (HL), five internal T-states on
   HL. Y and X from A - (HL) - H; WZ steps with HL. A repeat stops on a
   match or when BC reaches zero. */
static void cpx(cogmaster *m, uint16_t d, int rep)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t a = step_pair(z, RH, d);
    uint8_t v = cogmaster__z80_mread(m, a);
    cogmaster__z80_idle(m, a, 5);
    z->wz = (uint16_t)(z->wz + d);
    uint16_t bc = (uint16_t)(step_pair(z, RB, 0xFFFF) - 1u);
    uint8_t acc = z->reg[RA], r = (uint8_t)(acc - v);
    uint8_t h = (uint8_t)((acc ^ v ^ r) & HF);
    uint8_t n = (uint8_t)(r - (h ? 1u : 0u));
    uint8_t f = (uint8_t)((z->reg[RF] & CF) | (r & SF) | (r ? 0u : ZF) | h | (bc ? PF : 0u) | NF |
                          (n & 0x08u) | ((n & 0x02u) << 4));
    if (rep && bc && r) {
        f = repeat(m, a, f);
    }
    cogmaster__z80_setf(z, f);
}

/* The flags of INI, IND, OUTI, OUTD and their repeats, from the new B,
   the byte v and k = v plus C +/- 1 (IN) or the new L (OUT): N from v's
   bit 7, H and C from k's carry, P/V from the parity of (k & 7) ^ B. A
   repeat also adjusts P/V and H (stardot "Z80 Protocol Decoder" thread). */
static void io_flags(cogmaster *m, uint8_t v, unsigned k, int rep, uint16_t addr)
{
    struct cogmaster__z80 *z = &m->z80;
    uint8_t b = z->reg[RB];
    uint8_t f = (uint8_t)(cogmaster__z80_sz53(b) | (v & 0x80u ? NF : 0u) |
                          (k > 0xFFu ? (HF | CF) : 0u) | parity((uint8_t)((k & 7u) ^ b)));
    if (rep && b) {
        f = repeat(m, addr, f);
        uint8_t pb = b;
        if (f & CF) {
            pb = (uint8_t)(v & 0x80u ? b - 1u : b + 1u);
            uint8_t lo = (uint8_t)(b & 0x0Fu);
            f = (uint8_t)((f & ~HF) | ((v & 0x80u ? lo == 0x00u : lo == 0x0Fu) ? HF : 0u));
        }
        f = (uint8_t)(f ^ parity((uint8_t)(pb & 7u)) ^ PF);
    }
    cogmaster__z80_setf(z, f);
}

/* INI, IND, INIR, INDR: one internal T-state after the fetch, port BC
   into (HL), then B decrements. WZ = BC +/- 1 with the old B. */
static void inx(cogmaster *m, uint16_t d, int rep)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, ir(z), 1);
    uint16_t bc = cogmaster__z80_pair(z, RB);
    uint8_t v = cogmaster__z80_ioread(m, bc);
    z->wz = (uint16_t)(bc + d);
    z->reg[RB] = (uint8_t)(z->reg[RB] - 1u);
    uint16_t a = step_pair(z, RH, d);
    cogmaster__z80_mwrite(m, a, v);
    io_flags(m, v, v + (unsigned)(uint8_t)(z->reg[RC] + d), rep, a);
}

/* OUTI, OUTD, OTIR, OTDR: one internal T-state after the fetch, (HL) read,
   B decrements, then the byte goes out on port BC. WZ = BC +/- 1 with the
   new B. */
static void outx(cogmaster *m, uint16_t d, int rep)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, ir(z), 1);
    uint8_t v = cogmaster__z80_mread(m, step_pair(z, RH, d));
    z->reg[RB] = (uint8_t)(z->reg[RB] - 1u);
    uint16_t bc = cogmaster__z80_pair(z, RB);
    cogmaster__z80_iowrite(m, bc, v);
    z->wz = (uint16_t)(bc + d);
    io_flags(m, v, v + (unsigned)z->reg[RL], rep, bc);
}

/* The block instructions A0-A3, A8-AB, B0-B3, B8-BB by y (4 increment,
   5 decrement, 6 and 7 repeat) and z. */
static void block(cogmaster *m, unsigned y, unsigned s)
{
    uint16_t d = y & 1u ? 0xFFFFu : 1u;
    int rep = y >= 6;
    switch (s) {
    case 0: ldx(m, d, rep); break;
    case 1: cpx(m, d, rep); break;
    case 2: inx(m, d, rep); break;
    default: outx(m, d, rep); break;
    }
}

/* The 0x40-0x7F quarter by z and y. */
static void x1(cogmaster *m, unsigned y, unsigned s)
{
    struct cogmaster__z80 *z = &m->z80;
    static const uint8_t im[8] = {0, 0, 1, 2, 0, 0, 1, 2};
    switch (s) {
    case 0: in_c(m, y); break;
    case 1: out_c(m, y); break;
    case 2: adc_hl(m, y >> 1, !(y & 1u)); break;
    case 3: ld_nn_rp(m, y >> 1, (int)(y & 1u)); break;
    case 4: /* NEG */
    {
        uint8_t a = z->reg[RA];
        z->reg[RA] = 0;
        cogmaster__z80_alu(z, 2, a);
        break;
    }
    case 5: retn(m); break;
    case 6: z->im = im[y]; break;
    default:
        switch (y) {
        case 0: cogmaster__z80_idle(m, ir(z), 1); z->i = z->reg[RA]; break;
        case 1: cogmaster__z80_idle(m, ir(z), 1); z->r = z->reg[RA]; break;
        case 2: cogmaster__z80_idle(m, ir(z), 1); ld_a_ir(m, z->i); break;
        case 3: cogmaster__z80_idle(m, ir(z), 1); ld_a_ir(m, z->r); break;
        case 4: rxd(m, 0); break;
        case 5: rxd(m, 1); break;
        default: break; /* 0x77 and 0x7F: 8 T-state NOPs */
        }
        break;
    }
}

void cogmaster__z80_ed(cogmaster *m)
{
    uint8_t op = cogmaster__z80_m1(m);
    unsigned x = op >> 6, y = op >> 3 & 7u, s = op & 7u;
    if (x == 1) {
        x1(m, y, s);
    } else if (x == 2 && y >= 4 && s <= 3) {
        block(m, y, s);
    } else {
        /* A hole: the second fetch ends an 8 T-state no-operation. */
        cogmaster__bus_ed_undefined(m, op);
    }
}
