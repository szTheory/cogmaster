/* The Z80 step and the main opcode page. Instruction-granular, with every
   bus cycle stamped at its data T-state (z80_cycle.h). Opcode decoding:
   Zilog UM0080; Cristian Dinu, "Decoding Z80 opcodes". WZ follows Sean
   Young, "The Undocumented Z80 Documented", section on MEMPTR. */
#include "z80_cycle.h"

enum {
    RB = COGMASTER__Z80_B,
    RC = COGMASTER__Z80_C,
    RH = COGMASTER__Z80_H,
    RL = COGMASTER__Z80_L,
    RF = COGMASTER__Z80_F,
    RA = COGMASTER__Z80_A
};

/* Opcodes 0x40-0xBF by their fields x = op >> 6, y = op >> 3 & 7 and
   z = op & 7, where register field 6 means the byte at HL: x = 1 is
   LD r[y],r[z] with 0x76 = HALT in place of LD (HL),(HL); x = 2 is the
   accumulator operation y on r[z]. */
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
    uint8_t v = s == 6 ? cogmaster__z80_mread(m, cogmaster__z80_pair(z, RH)) : z->reg[s];
    if (op >= 0x80) {
        cogmaster__z80_alu(z, y, v);
    } else if (y == 6) {
        cogmaster__z80_mwrite(m, cogmaster__z80_pair(z, RH), v);
    } else {
        z->reg[y] = v;
    }
}

void cogmaster__z80_step(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    /* Q and the LD A,I/R and EI marks describe only the step just run. */
    z->q = 0;
    z->p = 0;
    z->ei = 0;
    z->idx = COGMASTER__Z80_IDX_NONE;

    if (z->halted) {
        /* a halted CPU runs M1 cycles at PC without advancing it, so R
           still counts (UM0080, "HALT") */
        (void)cogmaster__z80_m1(m);
        z->pc = (uint16_t)(z->pc - 1u);
        return;
    }
    uint8_t op = cogmaster__z80_m1(m);
    if (op >= 0x40 && op < 0xC0) {
        block(m, op);
        return;
    }
    switch (op) {
    case 0x00: /* NOP */
        break;
    case 0x02: { /* LD (BC),A; WZ = A:(C+1) */
        uint8_t a = z->reg[RA];
        uint8_t c = z->reg[RC];
        cogmaster__z80_mwrite(m, (uint16_t)(z->reg[RB] << 8 | c), a);
        z->wz = (uint16_t)(a << 8 | (uint8_t)(c + 1u));
        break;
    }
    case 0xDD: /* a prefix byte is a step of its own, so a run of them stays
                  bounded per step */
        z->idx = COGMASTER__Z80_IDX_IX;
        break;
    case 0xFD:
        z->idx = COGMASTER__Z80_IDX_IY;
        break;
    default:
        break;
    }
}
