/* The Z80 step and the main opcode page to 0xBF; z80_x3.c runs 0xC0-0xFF.
   Instruction-granular, with every bus cycle stamped at its data T-state
   (z80_cycle.h). Opcode decoding and M-cycle layouts: Zilog UM0080;
   Cristian Dinu, "Decoding Z80 opcodes". WZ follows Sean Young, "The
   Undocumented Z80 Documented", section on MEMPTR. */
#include "z80_cycle.h"

enum {
    RB = COGMASTER__Z80_B,
    RC = COGMASTER__Z80_C,
    RD = COGMASTER__Z80_D,
    RE = COGMASTER__Z80_E,
    RH = COGMASTER__Z80_H,
    RL = COGMASTER__Z80_L,
    RF = COGMASTER__Z80_F,
    RA = COGMASTER__Z80_A,
    ALT = COGMASTER__Z80_ALT
};

/* Flags S, Z and P/V are kept by the rotates on A, SCF, CCF and CPL. */
#define KEEP_SZP (COGMASTER__Z80_SF | COGMASTER__Z80_ZF | COGMASTER__Z80_PF)

/* Register field r, with H and L remapped under a prefix. */
static unsigned r8(const struct cogmaster__z80 *z, unsigned r)
{
    return r == RH || r == RL ? cogmaster__z80_hx(z) + (r - RH) : r;
}

/* The address of a (HL) operand. Under a prefix it is (IX+d) or (IY+d):
   the displacement byte at PC, then n internal T-states, and WZ takes the
   address. */
static uint16_t mem_hl(cogmaster *m, uint32_t n)
{
    struct cogmaster__z80 *z = &m->z80;
    if (z->idx == COGMASTER__Z80_IDX_NONE) {
        return cogmaster__z80_pair(z, RH);
    }
    uint8_t d = cogmaster__z80_imm8(m);
    uint16_t a = (uint16_t)(cogmaster__z80_hl(z) + (uint16_t)(int8_t)d);
    cogmaster__z80_idle(m, (uint16_t)(z->pc - 1u), n);
    z->wz = a;
    return a;
}

/* Opcodes 0x40-0xBF by their fields x = op >> 6, y = op >> 3 & 7 and
   z = op & 7, where register field 6 means the byte at HL: x = 1 is
   LD r[y],r[z] with 0x76 = HALT in place of LD (HL),(HL); x = 2 is the
   accumulator operation y on r[z]. Under a prefix H and L remap, except
   beside (IX+d): LD H,(IX+d) and LD (IX+d),L use the true H and L. */
static void block(cogmaster *m, uint8_t op)
{
    struct cogmaster__z80 *z = &m->z80;
    unsigned y = op >> 3 & 7u, s = op & 7u;
    if (op == 0x76) {
        /* HALT: PC has moved past it; halted steps then repeat M1 cycles
           at PC (UM0080, "HALT") */
        z->halted = 1;
        return;
    }
    uint8_t v;
    if (s == 6) {
        v = cogmaster__z80_mread(m, mem_hl(m, 5));
    } else {
        v = z->reg[y == 6 && op < 0x80 ? s : r8(z, s)];
    }
    if (op >= 0x80) {
        cogmaster__z80_alu(z, y, v);
    } else if (y == 6) {
        cogmaster__z80_mwrite(m, mem_hl(m, 5), v);
    } else {
        z->reg[s == 6 ? y : r8(z, y)] = v;
    }
}

/* LD (rr),A and LD A,(rr) for BC and DE. A store sets WZ to A:(low + 1),
   a load to rr + 1. */
static void ld_ind_a(cogmaster *m, unsigned hi)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t a = cogmaster__z80_pair(z, hi);
    cogmaster__z80_mwrite(m, a, z->reg[RA]);
    z->wz = (uint16_t)(z->reg[RA] << 8 | (uint8_t)(a + 1u));
}

static void ld_a_ind(cogmaster *m, unsigned hi)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t a = cogmaster__z80_pair(z, hi);
    z->reg[RA] = cogmaster__z80_mread(m, a);
    z->wz = (uint16_t)(a + 1u);
}

/* INC rr and DEC rr: the opcode fetch runs 6 T-states. */
static void inc_rp(cogmaster *m, unsigned p, uint16_t delta)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 2);
    cogmaster__z80_set_rp(z, p, (uint16_t)(cogmaster__z80_rp(z, p) + delta));
}

/* INC r and DEC r; on (HL) the read takes one extra T-state. */
static void inc_r(cogmaster *m, unsigned r, int dec)
{
    struct cogmaster__z80 *z = &m->z80;
    if (r != 6) {
        r = r8(z, r);
        z->reg[r] = dec ? cogmaster__z80_dec8(z, z->reg[r]) : cogmaster__z80_inc8(z, z->reg[r]);
        return;
    }
    uint16_t a = mem_hl(m, 5);
    uint8_t v = cogmaster__z80_mread(m, a);
    cogmaster__z80_idle(m, a, 1);
    cogmaster__z80_mwrite(m, a, dec ? cogmaster__z80_dec8(z, v) : cogmaster__z80_inc8(z, v));
}

/* LD r,n; LD (IX+d),n reads d and n, then runs two internal T-states. */
static void ld_r_n(cogmaster *m, unsigned r)
{
    struct cogmaster__z80 *z = &m->z80;
    if (r != 6) {
        z->reg[r8(z, r)] = cogmaster__z80_imm8(m);
        return;
    }
    uint16_t a = mem_hl(m, 0);
    uint8_t n = cogmaster__z80_imm8(m);
    if (z->idx != COGMASTER__Z80_IDX_NONE) {
        cogmaster__z80_idle(m, (uint16_t)(z->pc - 1u), 2);
    }
    cogmaster__z80_mwrite(m, a, n);
}

/* RLCA, RRCA, RLA, RRA by y: C from the bit shifted out, H and N clear,
   Y and X from the new A. */
static void rot_a(struct cogmaster__z80 *z, unsigned y)
{
    uint8_t a = z->reg[RA], f = z->reg[RF];
    uint8_t out = y & 1u ? (uint8_t)(a & 1u) : (uint8_t)(a >> 7);
    uint8_t in = y < 2 ? out : (uint8_t)(f & COGMASTER__Z80_CF);
    a = y & 1u ? (uint8_t)(a >> 1 | in << 7) : (uint8_t)(a << 1 | in);
    z->reg[RA] = a;
    cogmaster__z80_setf(z, (uint8_t)((f & KEEP_SZP) | (a & 0x28u) | out));
}

/* ADD HL,rr: H from the carry out of bit 11, C from bit 15, Y and X from
   the new H; seven internal T-states. WZ = HL + 1. */
static void add_hl(cogmaster *m, unsigned p)
{
    struct cogmaster__z80 *z = &m->z80;
    uint32_t a = cogmaster__z80_hl(z), b = cogmaster__z80_rp(z, p), r = a + b;
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 7);
    z->wz = (uint16_t)(a + 1u);
    cogmaster__z80_set_pair(z, cogmaster__z80_hx(z), (uint16_t)r);
    cogmaster__z80_setf(z, (uint8_t)((z->reg[RF] & KEEP_SZP) | ((r >> 8) & 0x28u) |
                                     (((a ^ b ^ r) >> 8) & 0x10u) | (r >> 16)));
}

/* A taken relative jump: five internal T-states on the operand's address. */
static void jump_rel(cogmaster *m, uint8_t d)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, (uint16_t)(z->pc - 1u), 5);
    z->pc = (uint16_t)(z->pc + (uint16_t)(int8_t)d);
    z->wz = z->pc;
}

static void jr(cogmaster *m, int taken)
{
    uint8_t d = cogmaster__z80_imm8(m);
    if (taken) {
        jump_rel(m, d);
    }
}

/* DJNZ: the opcode fetch runs 5 T-states. */
static void djnz(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 1);
    uint8_t d = cogmaster__z80_imm8(m);
    z->reg[RB] = (uint8_t)(z->reg[RB] - 1u);
    if (z->reg[RB] != 0) {
        jump_rel(m, d);
    }
}

/* LD (nn),HL, LD HL,(nn), LD (nn),A and LD A,(nn). WZ = nn + 1, except
   that a store of A sets A:(low + 1). */
static void ld_nn_hl(cogmaster *m, int load)
{
    struct cogmaster__z80 *z = &m->z80;
    unsigned h = cogmaster__z80_hx(z);
    uint16_t a = cogmaster__z80_imm16(m);
    if (load) {
        z->reg[h + 1u] = cogmaster__z80_mread(m, a);
        z->reg[h] = cogmaster__z80_mread(m, (uint16_t)(a + 1u));
    } else {
        cogmaster__z80_mwrite(m, a, z->reg[h + 1u]);
        cogmaster__z80_mwrite(m, (uint16_t)(a + 1u), z->reg[h]);
    }
    z->wz = (uint16_t)(a + 1u);
}

static void ld_nn_a(cogmaster *m, int load)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t a = cogmaster__z80_imm16(m);
    if (load) {
        z->reg[RA] = cogmaster__z80_mread(m, a);
        z->wz = (uint16_t)(a + 1u);
    } else {
        cogmaster__z80_mwrite(m, a, z->reg[RA]);
        z->wz = (uint16_t)(z->reg[RA] << 8 | (uint8_t)(a + 1u));
    }
}

/* DAA: adds or subtracts 6 per nibble that is past 9 or carried (Sean
   Young section 4.7). */
static void daa(struct cogmaster__z80 *z)
{
    uint8_t a = z->reg[RA], f = z->reg[RF];
    uint8_t diff = 0, c = f & COGMASTER__Z80_CF;
    if ((f & COGMASTER__Z80_HF) || (a & 0x0Fu) > 9) {
        diff = 0x06;
    }
    if (c || a > 0x99) {
        diff |= 0x60;
        c = COGMASTER__Z80_CF;
    }
    uint8_t h;
    if (f & COGMASTER__Z80_NF) {
        h = (f & COGMASTER__Z80_HF) && (a & 0x0Fu) < 6 ? COGMASTER__Z80_HF : 0;
        a = (uint8_t)(a - diff);
    } else {
        h = (a & 0x0Fu) > 9 ? COGMASTER__Z80_HF : 0;
        a = (uint8_t)(a + diff);
    }
    z->reg[RA] = a;
    cogmaster__z80_setf(z, (uint8_t)(cogmaster__z80_sz53p(a) | (f & COGMASTER__Z80_NF) | h | c));
}

static void cpl(struct cogmaster__z80 *z)
{
    uint8_t a = (uint8_t)~z->reg[RA];
    z->reg[RA] = a;
    cogmaster__z80_setf(z, (uint8_t)((z->reg[RF] & (KEEP_SZP | COGMASTER__Z80_CF)) | (a & 0x28u) |
                                     COGMASTER__Z80_HF | COGMASTER__Z80_NF));
}

/* SCF and CCF. Y and X are ((Q ^ F) | A) & 0x28 with Q from the step
   before: A's bits when the previous instruction wrote F, else A's or F's
   (Patrik Rak, "SCF/CCF undocumented flags"). CCF moves the old C to H. */
static void scf_ccf(struct cogmaster__z80 *z, uint8_t q_prev, int ccf)
{
    uint8_t f = z->reg[RF];
    uint8_t xy = (uint8_t)(((q_prev ^ f) | z->reg[RA]) & 0x28u);
    uint8_t c = f & COGMASTER__Z80_CF;
    uint8_t nf = ccf ? (uint8_t)((c ? COGMASTER__Z80_HF : 0) | (c ^ 1u)) : COGMASTER__Z80_CF;
    cogmaster__z80_setf(z, (uint8_t)((f & KEEP_SZP) | xy | nf));
}

/* EX AF,AF' swaps without writing Q. */
static void ex_af(struct cogmaster__z80 *z)
{
    for (unsigned k = RF; k <= RA; k++) {
        uint8_t t = z->reg[k];
        z->reg[k] = z->reg[ALT + k];
        z->reg[ALT + k] = t;
    }
}

/* One opcode after its fetch; idx holds the prefix of the step before. */
static void exec(cogmaster *m, uint8_t op, uint8_t q_prev)
{
    struct cogmaster__z80 *z = &m->z80;
    if (op >= 0x40 && op < 0xC0) {
        block(m, op);
        return;
    }
    if (op >= 0xC0) {
        cogmaster__z80_x3(m, op);
        return;
    }
    switch (op) {
    case 0x00:
        break; /* NOP */
    case 0x01:
        cogmaster__z80_set_rp(z, 0, cogmaster__z80_imm16(m));
        break;
    case 0x02:
        ld_ind_a(m, RB);
        break;
    case 0x03:
        inc_rp(m, 0, 1);
        break;
    case 0x04:
        inc_r(m, RB, 0);
        break;
    case 0x05:
        inc_r(m, RB, 1);
        break;
    case 0x06:
        ld_r_n(m, RB);
        break;
    case 0x07:
        rot_a(z, 0);
        break;
    case 0x08:
        ex_af(z);
        break;
    case 0x09:
        add_hl(m, 0);
        break;
    case 0x0A:
        ld_a_ind(m, RB);
        break;
    case 0x0B:
        inc_rp(m, 0, 0xFFFF);
        break;
    case 0x0C:
        inc_r(m, RC, 0);
        break;
    case 0x0D:
        inc_r(m, RC, 1);
        break;
    case 0x0E:
        ld_r_n(m, RC);
        break;
    case 0x0F:
        rot_a(z, 1);
        break;
    case 0x10:
        djnz(m);
        break;
    case 0x11:
        cogmaster__z80_set_rp(z, 1, cogmaster__z80_imm16(m));
        break;
    case 0x12:
        ld_ind_a(m, RD);
        break;
    case 0x13:
        inc_rp(m, 1, 1);
        break;
    case 0x14:
        inc_r(m, RD, 0);
        break;
    case 0x15:
        inc_r(m, RD, 1);
        break;
    case 0x16:
        ld_r_n(m, RD);
        break;
    case 0x17:
        rot_a(z, 2);
        break;
    case 0x18:
        jr(m, 1);
        break;
    case 0x19:
        add_hl(m, 1);
        break;
    case 0x1A:
        ld_a_ind(m, RD);
        break;
    case 0x1B:
        inc_rp(m, 1, 0xFFFF);
        break;
    case 0x1C:
        inc_r(m, RE, 0);
        break;
    case 0x1D:
        inc_r(m, RE, 1);
        break;
    case 0x1E:
        ld_r_n(m, RE);
        break;
    case 0x1F:
        rot_a(z, 3);
        break;
    case 0x20:
        jr(m, cogmaster__z80_cc(z, 0));
        break;
    case 0x21:
        cogmaster__z80_set_rp(z, 2, cogmaster__z80_imm16(m));
        break;
    case 0x22:
        ld_nn_hl(m, 0);
        break;
    case 0x23:
        inc_rp(m, 2, 1);
        break;
    case 0x24:
        inc_r(m, RH, 0);
        break;
    case 0x25:
        inc_r(m, RH, 1);
        break;
    case 0x26:
        ld_r_n(m, RH);
        break;
    case 0x27:
        daa(z);
        break;
    case 0x28:
        jr(m, cogmaster__z80_cc(z, 1));
        break;
    case 0x29:
        add_hl(m, 2);
        break;
    case 0x2A:
        ld_nn_hl(m, 1);
        break;
    case 0x2B:
        inc_rp(m, 2, 0xFFFF);
        break;
    case 0x2C:
        inc_r(m, RL, 0);
        break;
    case 0x2D:
        inc_r(m, RL, 1);
        break;
    case 0x2E:
        ld_r_n(m, RL);
        break;
    case 0x2F:
        cpl(z);
        break;
    case 0x30:
        jr(m, cogmaster__z80_cc(z, 2));
        break;
    case 0x31:
        cogmaster__z80_set_rp(z, 3, cogmaster__z80_imm16(m));
        break;
    case 0x32:
        ld_nn_a(m, 0);
        break;
    case 0x33:
        inc_rp(m, 3, 1);
        break;
    case 0x34:
        inc_r(m, 6, 0);
        break;
    case 0x35:
        inc_r(m, 6, 1);
        break;
    case 0x36:
        ld_r_n(m, 6);
        break;
    case 0x37:
        scf_ccf(z, q_prev, 0);
        break;
    case 0x38:
        jr(m, cogmaster__z80_cc(z, 3));
        break;
    case 0x39:
        add_hl(m, 3);
        break;
    case 0x3A:
        ld_nn_a(m, 1);
        break;
    case 0x3B:
        inc_rp(m, 3, 0xFFFF);
        break;
    case 0x3C:
        inc_r(m, RA, 0);
        break;
    case 0x3D:
        inc_r(m, RA, 1);
        break;
    case 0x3E:
        ld_r_n(m, RA);
        break;
    case 0x3F:
        scf_ccf(z, q_prev, 1);
        break;
    }
}

/* The start of an acceptance: IFF1 clears, a halted CPU resumes with PC
   already past HALT, and an NMOS part accepting right after LD A,I or
   LD A,R leaves P/V clear in F (Sean Young section 5.3 and section 5.4; MAME's
   Z80 notes on NMOS parts). */
static void accept(struct cogmaster__z80 *z, uint8_t p_prev)
{
    z->iff1 = 0;
    z->halted = 0;
    if (p_prev) {
        z->reg[RF] = (uint8_t)(z->reg[RF] & 0xFBu);
    }
}

/* NMI acceptance, 11 T-states: a 5 T-state opcode fetch whose byte is
   ignored, then PC pushed; IFF2 keeps the old IFF1 (Sean Young
   section 5.1 and section 5.3). */
static void nmi(cogmaster *m, uint8_t p_prev)
{
    struct cogmaster__z80 *z = &m->z80;
    z->nmi_pending = 0;
    accept(z, p_prev);
    (void)cogmaster__z80_m1(m);
    z->pc = (uint16_t)(z->pc - 1u);
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 1);
    cogmaster__z80_push(m, z->pc);
    z->pc = 0x0066;
    z->wz = z->pc;
}

/* /INT acceptance: an acknowledge cycle of 6 T-states (an M1 with two
   automatic wait states) that counts in R, one internal T-state, then PC
   pushed. Nothing on the SMS drives the data bus during the acknowledge,
   so the byte read is $FF (Sean Young section 5.2; Charles MacDonald,
   smstech-20021112.txt). IM 0 runs it as RST $38 like IM 1, 13 T-states;
   IM 2 reads the handler address at I<<8 | $FF, 19 T-states (Sean Young
   chapter 5). */
static void irq(cogmaster *m, uint8_t p_prev)
{
    struct cogmaster__z80 *z = &m->z80;
    accept(z, p_prev);
    z->iff2 = 0;
    m->ticks += 3u * 6u;
    z->r = (uint8_t)((z->r & 0x80u) | ((z->r + 1u) & 0x7Fu));
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 1);
    cogmaster__z80_push(m, z->pc);
    if (z->im == 2) {
        uint16_t v = (uint16_t)((unsigned)z->i << 8 | 0xFFu);
        uint8_t lo = cogmaster__z80_mread(m, v);
        z->pc = (uint16_t)(cogmaster__z80_mread(m, (uint16_t)(v + 1u)) << 8 | lo);
    } else {
        z->pc = 0x0038;
    }
    z->wz = z->pc;
}

void cogmaster__z80_step(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    /* Q and the LD A,I/R and EI marks describe only the step just run. */
    uint8_t q_prev = z->q, p_prev = z->p, ei_prev = z->ei;
    z->q = 0;
    z->p = 0;
    z->ei = 0;

    /* Acceptance happens only between instructions: never after a prefix
       byte. /INT also waits one instruction after EI; an NMI does not
       (UM0080, "EI"; Sean Young section 5.5; hoglet67 Z80Decoder wiki,
       "NMI during EI Anomaly"). */
    if (z->idx == COGMASTER__Z80_IDX_NONE) {
        if (z->nmi_pending) {
            nmi(m, p_prev);
            return;
        }
        if (z->int_line && z->iff1 && !ei_prev) {
            irq(m, p_prev);
            return;
        }
    }

    if (z->halted) {
        /* a halted CPU runs M1 cycles at PC without advancing it, so R
           still counts (UM0080, "HALT") */
        (void)cogmaster__z80_m1(m);
        z->pc = (uint16_t)(z->pc - 1u);
        return;
    }
    /* The fault record names the instruction: its address, the first
       prefix's when one is pending, and its first byte after the prefixes. */
    if (z->idx == COGMASTER__Z80_IDX_NONE) {
        m->bus.op_pc = z->pc;
    }
    uint8_t op = cogmaster__z80_m1(m);
    m->bus.op = op;
    /* A DD or FD byte is a step of its own: a 4 T-state M1 that only sets
       the prefix for the next step and leaves Q at 0. A second prefix
       replaces the first; any other opcode consumes it. */
    if (op == 0xDD || op == 0xFD) {
        z->idx = op == 0xDD ? COGMASTER__Z80_IDX_IX : COGMASTER__Z80_IDX_IY;
        return;
    }
    exec(m, op, q_prev);
    z->idx = COGMASTER__Z80_IDX_NONE;
}
