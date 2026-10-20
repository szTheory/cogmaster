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

/* Writes F, and Q with it: Q holds the F an instruction wrote. */
static inline void cogmaster__z80_setf(struct cogmaster__z80 *z, uint8_t f)
{
    z->reg[COGMASTER__Z80_F] = f;
    z->q = f;
}

#endif /* COGMASTER_Z80_CYCLE_H */
