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
    default:
        break;
    }
}
