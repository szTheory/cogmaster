/* The Z80 main page's last quarter, opcodes 0xC0-0xFF (x = 3 in Cristian
   Dinu, "Decoding Z80 opcodes"): returns, jumps, calls, the stack, port I/O,
   the exchanges, immediate ALU operations and the CB and ED pages. M-cycle
   layouts follow Zilog UM0080; WZ follows Sean Young, "The Undocumented Z80
   Documented", section on MEMPTR. */
#include "z80_cycle.h"

enum {
    RB = COGMASTER__Z80_B,
    RD = COGMASTER__Z80_D,
    RH = COGMASTER__Z80_H,
    RL = COGMASTER__Z80_L,
    RF = COGMASTER__Z80_F,
    RA = COGMASTER__Z80_A,
    ALT = COGMASTER__Z80_ALT
};

static uint16_t pop(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    uint8_t lo = cogmaster__z80_mread(m, z->sp);
    z->sp = (uint16_t)(z->sp + 1u);
    uint8_t hi = cogmaster__z80_mread(m, z->sp);
    z->sp = (uint16_t)(z->sp + 1u);
    return (uint16_t)(hi << 8 | lo);
}

/* EXX and EX DE,HL swap without writing Q and keep the true HL under a
   prefix. */
static void exx(struct cogmaster__z80 *z)
{
    for (unsigned k = RB; k <= RL; k++) {
        uint8_t t = z->reg[k];
        z->reg[k] = z->reg[ALT + k];
        z->reg[ALT + k] = t;
    }
}

static void ex_de_hl(struct cogmaster__z80 *z)
{
    uint16_t t = cogmaster__z80_pair(z, RD);
    cogmaster__z80_set_pair(z, RD, cogmaster__z80_pair(z, RH));
    cogmaster__z80_set_pair(z, RH, t);
}

/* EX (SP),HL: read low then high (one extra T-state), write high then low
   (two extra T-states). WZ = the new HL. */
static void ex_sp_hl(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    unsigned h = cogmaster__z80_hx(z);
    uint16_t s1 = (uint16_t)(z->sp + 1u);
    uint8_t lo = cogmaster__z80_mread(m, z->sp);
    uint8_t hi = cogmaster__z80_mread(m, s1);
    cogmaster__z80_idle(m, s1, 1);
    cogmaster__z80_mwrite(m, s1, z->reg[h]);
    cogmaster__z80_mwrite(m, z->sp, z->reg[h + 1u]);
    cogmaster__z80_idle(m, z->sp, 2);
    z->reg[h] = hi;
    z->reg[h + 1u] = lo;
    z->wz = cogmaster__z80_hl(z);
}

/* RET cc: the opcode fetch runs 5 T-states. RET and a taken RET cc set
   PC and WZ to the popped address. */
static void ret(cogmaster *m, int taken)
{
    struct cogmaster__z80 *z = &m->z80;
    if (taken) {
        z->pc = pop(m);
        z->wz = z->pc;
    }
}

static void ret_cc(cogmaster *m, unsigned y)
{
    cogmaster__z80_idle(m, cogmaster__z80_ir(&m->z80), 1);
    ret(m, cogmaster__z80_cc(&m->z80, y));
}

/* POP rr and PUSH rr, p = 3 meaning AF. POP AF writes F without Q. PUSH's
   opcode fetch runs 5 T-states. */
static void pop_rp(cogmaster *m, unsigned p)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t v = pop(m);
    if (p == 3) {
        z->reg[RA] = (uint8_t)(v >> 8);
        z->reg[RF] = (uint8_t)v;
    } else {
        cogmaster__z80_set_rp(z, p, v);
    }
}

static void push_rp(cogmaster *m, unsigned p)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 1);
    cogmaster__z80_push(m, p == 3 ? (uint16_t)(z->reg[RA] << 8 | z->reg[RF])
                                  : cogmaster__z80_rp(z, p));
}

/* JP nn and JP cc,nn: WZ = nn whether or not the jump is taken. */
static void jp(cogmaster *m, int taken)
{
    struct cogmaster__z80 *z = &m->z80;
    z->wz = cogmaster__z80_imm16(m);
    if (taken) {
        z->pc = z->wz;
    }
}

/* CALL nn and CALL cc,nn: WZ = nn; a taken call reads the high byte in 4
   T-states and pushes PC. */
static void call(cogmaster *m, int taken)
{
    struct cogmaster__z80 *z = &m->z80;
    z->wz = cogmaster__z80_imm16(m);
    if (taken) {
        cogmaster__z80_idle(m, (uint16_t)(z->pc - 1u), 1);
        cogmaster__z80_push(m, z->pc);
        z->pc = z->wz;
    }
}

/* RST p: the opcode fetch runs 5 T-states. */
static void rst(cogmaster *m, uint16_t addr)
{
    struct cogmaster__z80 *z = &m->z80;
    cogmaster__z80_idle(m, cogmaster__z80_ir(z), 1);
    cogmaster__z80_push(m, z->pc);
    z->pc = addr;
    z->wz = addr;
}

/* OUT (n),A and IN A,(n) address port A:n. OUT sets WZ to A:(n + 1)
   with no carry into A, IN to A:n + 1. Flags are unaffected. */
static void out_n(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    uint8_t n = cogmaster__z80_imm8(m), a = z->reg[RA];
    cogmaster__z80_iowrite(m, (uint16_t)(a << 8 | n), a);
    z->wz = (uint16_t)(a << 8 | (uint8_t)(n + 1u));
}

static void in_n(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    uint16_t port = (uint16_t)(z->reg[RA] << 8 | cogmaster__z80_imm8(m));
    z->reg[RA] = cogmaster__z80_ioread(m, port);
    z->wz = (uint16_t)(port + 1u);
}

/* DI and EI set both flip-flops. EI marks the step so that acceptance waits
   one more instruction (UM0080, "EI"). */
static void ei_di(struct cogmaster__z80 *z, uint8_t on)
{
    z->iff1 = on;
    z->iff2 = on;
    z->ei = on;
}

void cogmaster__z80_x3(cogmaster *m, uint8_t op)
{
    struct cogmaster__z80 *z = &m->z80;
    switch (op) {
    case 0xC0:
        ret_cc(m, 0);
        break;
    case 0xC1:
        pop_rp(m, 0);
        break;
    case 0xC2:
        jp(m, cogmaster__z80_cc(z, 0));
        break;
    case 0xC3:
        jp(m, 1);
        break;
    case 0xC4:
        call(m, cogmaster__z80_cc(z, 0));
        break;
    case 0xC5:
        push_rp(m, 0);
        break;
    case 0xC6:
        cogmaster__z80_alu(z, 0, cogmaster__z80_imm8(m));
        break;
    case 0xC7:
        rst(m, 0x00);
        break;
    case 0xC8:
        ret_cc(m, 1);
        break;
    case 0xC9:
        ret(m, 1);
        break;
    case 0xCA:
        jp(m, cogmaster__z80_cc(z, 1));
        break;
    case 0xCB:
        cogmaster__z80_cb(m);
        break;
    case 0xCC:
        call(m, cogmaster__z80_cc(z, 1));
        break;
    case 0xCD:
        call(m, 1);
        break;
    case 0xCE:
        cogmaster__z80_alu(z, 1, cogmaster__z80_imm8(m));
        break;
    case 0xCF:
        rst(m, 0x08);
        break;
    case 0xD0:
        ret_cc(m, 2);
        break;
    case 0xD1:
        pop_rp(m, 1);
        break;
    case 0xD2:
        jp(m, cogmaster__z80_cc(z, 2));
        break;
    case 0xD3:
        out_n(m);
        break;
    case 0xD4:
        call(m, cogmaster__z80_cc(z, 2));
        break;
    case 0xD5:
        push_rp(m, 1);
        break;
    case 0xD6:
        cogmaster__z80_alu(z, 2, cogmaster__z80_imm8(m));
        break;
    case 0xD7:
        rst(m, 0x10);
        break;
    case 0xD8:
        ret_cc(m, 3);
        break;
    case 0xD9:
        exx(z);
        break;
    case 0xDA:
        jp(m, cogmaster__z80_cc(z, 3));
        break;
    case 0xDB:
        in_n(m);
        break;
    case 0xDC:
        call(m, cogmaster__z80_cc(z, 3));
        break;
    case 0xDE:
        cogmaster__z80_alu(z, 3, cogmaster__z80_imm8(m));
        break;
    case 0xDF:
        rst(m, 0x18);
        break;
    case 0xE0:
        ret_cc(m, 4);
        break;
    case 0xE1:
        pop_rp(m, 2);
        break;
    case 0xE2:
        jp(m, cogmaster__z80_cc(z, 4));
        break;
    case 0xE3:
        ex_sp_hl(m);
        break;
    case 0xE4:
        call(m, cogmaster__z80_cc(z, 4));
        break;
    case 0xE5:
        push_rp(m, 2);
        break;
    case 0xE6:
        cogmaster__z80_alu(z, 4, cogmaster__z80_imm8(m));
        break;
    case 0xE7:
        rst(m, 0x20);
        break;
    case 0xE8:
        ret_cc(m, 5);
        break;
    case 0xE9:
        z->pc = cogmaster__z80_hl(z);
        break; /* JP (HL) or (IX); WZ unchanged */
    case 0xEA:
        jp(m, cogmaster__z80_cc(z, 5));
        break;
    case 0xEB:
        ex_de_hl(z);
        break;
    case 0xEC:
        call(m, cogmaster__z80_cc(z, 5));
        break;
    case 0xED:
        cogmaster__z80_ed(m);
        break;
    case 0xEE:
        cogmaster__z80_alu(z, 5, cogmaster__z80_imm8(m));
        break;
    case 0xEF:
        rst(m, 0x28);
        break;
    case 0xF0:
        ret_cc(m, 6);
        break;
    case 0xF1:
        pop_rp(m, 3);
        break;
    case 0xF2:
        jp(m, cogmaster__z80_cc(z, 6));
        break;
    case 0xF3:
        ei_di(z, 0);
        break;
    case 0xF4:
        call(m, cogmaster__z80_cc(z, 6));
        break;
    case 0xF5:
        push_rp(m, 3);
        break;
    case 0xF6:
        cogmaster__z80_alu(z, 6, cogmaster__z80_imm8(m));
        break;
    case 0xF7:
        rst(m, 0x30);
        break;
    case 0xF8:
        ret_cc(m, 7);
        break;
    case 0xF9:
        cogmaster__z80_idle(m, cogmaster__z80_ir(z), 2);
        z->sp = cogmaster__z80_hl(z);
        break; /* LD SP,HL */
    case 0xFA:
        jp(m, cogmaster__z80_cc(z, 7));
        break;
    case 0xFB:
        ei_di(z, 1);
        break;
    case 0xFC:
        call(m, cogmaster__z80_cc(z, 7));
        break;
    case 0xFE:
        cogmaster__z80_alu(z, 7, cogmaster__z80_imm8(m));
        break;
    case 0xFF:
        rst(m, 0x38);
        break;
    default:
        break; /* DD and FD never reach here */
    }
}
