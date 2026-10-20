/* The Z80 state and the bus seam it calls. Not installed. */
#ifndef COGMASTER_Z80_H
#define COGMASTER_Z80_H

#include <stdint.h>

#include "cogmaster.h"

/* Register file indices. B to A follow the opcode's 3-bit register field,
   with F at 6 where the field means (HL), so code tests that case before
   indexing. The index halves and the alternate set follow. */
enum {
    COGMASTER__Z80_B = 0,
    COGMASTER__Z80_C = 1,
    COGMASTER__Z80_D = 2,
    COGMASTER__Z80_E = 3,
    COGMASTER__Z80_H = 4,
    COGMASTER__Z80_L = 5,
    COGMASTER__Z80_F = 6,
    COGMASTER__Z80_A = 7,
    COGMASTER__Z80_IXH = 8,
    COGMASTER__Z80_IXL = 9,
    COGMASTER__Z80_IYH = 10,
    COGMASTER__Z80_IYL = 11,
    COGMASTER__Z80_ALT = 12, /* B' to A' at ALT + B to ALT + A */
    COGMASTER__Z80_REGS = 20
};

/* The pending prefix after a DD or FD byte. */
enum { COGMASTER__Z80_IDX_NONE = 0, COGMASTER__Z80_IDX_IX = 1, COGMASTER__Z80_IDX_IY = 2 };

struct cogmaster__z80 {
    uint8_t reg[COGMASTER__Z80_REGS];
    uint16_t pc, sp;
    uint16_t wz;         /* the internal address latch (MEMPTR) */
    uint8_t i;           /* interrupt vector base */
    uint8_t r;           /* refresh: low 7 bits count M1 cycles, bit 7 is kept */
    uint8_t iff1, iff2;  /* interrupt enable flip-flops */
    uint8_t im;          /* interrupt mode 0, 1 or 2 */
    uint8_t q;           /* F as written by the last instruction, else 0 */
    uint8_t p;           /* 1 after LD A,I or LD A,R, until the next step */
    uint8_t ei;          /* 1 after EI, until the next step: blocks acceptance */
    uint8_t idx;         /* the pending prefix, COGMASTER__Z80_IDX_* */
    uint8_t halted;      /* 1 between HALT and an accepted interrupt */
    uint8_t int_line;    /* /INT as the bus drives it */
    uint8_t nmi_pending; /* a /NMI edge not yet accepted */
};

/* Runs one bounded step: an instruction, one prefix byte, one halted M1 or
   one interrupt acceptance. */
void cogmaster__z80_step(cogmaster *m);

/* The bus seam. Each is called once per bus cycle, with m->ticks at the
   cycle's data T-state (see z80_cycle.h). The library defines them in its
   bus; the vector harness defines its own. */
uint8_t cogmaster__bus_fetch(cogmaster *m, uint16_t addr, uint16_t refresh);
uint8_t cogmaster__bus_read(cogmaster *m, uint16_t addr);
void cogmaster__bus_write(cogmaster *m, uint16_t addr, uint8_t v);
uint8_t cogmaster__bus_in(cogmaster *m, uint16_t port);
void cogmaster__bus_out(cogmaster *m, uint16_t port, uint8_t v);
/* An undefined ED opcode op ran, as an 8 T-state no-operation. */
void cogmaster__bus_ed_undefined(cogmaster *m, uint8_t op);

#endif /* COGMASTER_Z80_H */
