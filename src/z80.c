/* The Z80 step and the main opcode page. Instruction-granular, with every
   bus cycle stamped at its data T-state (z80_cycle.h). Opcode decoding:
   Zilog UM0080; Cristian Dinu, "Decoding Z80 opcodes". */
#include "z80_cycle.h"

void cogmaster__z80_step(cogmaster *m)
{
    struct cogmaster__z80 *z = &m->z80;
    /* Q and the LD A,I/R and EI marks describe only the step just run. */
    z->q = 0;
    z->p = 0;
    z->ei = 0;
    z->idx = COGMASTER__Z80_IDX_NONE;

    uint8_t op = cogmaster__z80_m1(m);
    switch (op) {
    case 0x00: /* NOP */
        break;
    case 0x02: { /* LD (BC),A; WZ = A:(C+1) (Sean Young, "The Undocumented Z80
                    Documented", MEMPTR) */
        uint8_t a = z->reg[COGMASTER__Z80_A];
        uint8_t c = z->reg[COGMASTER__Z80_C];
        cogmaster__z80_mwrite(m, (uint16_t)(z->reg[COGMASTER__Z80_B] << 8 | c), a);
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
