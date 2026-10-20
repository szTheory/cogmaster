/* The I/O chip's ports: every port the VDP does not answer (SMS-SYSTEM
   section 4). */
#include "internal.h"

/* An undriven read: the SMS2 and the Game Gear pull the data bus up to $FF
   (SMS-SYSTEM C5). Shipped software makes such reads, so they are counted,
   not faults. */
uint8_t cogmaster__io_open_bus(cogmaster *m)
{
    m->counters.open_bus_reads++;
    return 0xFFu;
}

void cogmaster__io_reset(cogmaster *m)
{
    (void)m;
}

uint8_t cogmaster__io_in(cogmaster *m, uint16_t port)
{
    uint32_t p = port & 0xFFu;
    if (p < 0x40u) {
        /* $3E and $3F are write-only; nothing drives a read there. */
        cogmaster__bus_fault(m, port, COGMASTER__DEVICE_CTRL, COGMASTER__FAULT_UNMAPPED_READ);
    }
    return cogmaster__io_open_bus(m);
}

void cogmaster__io_out(cogmaster *m, uint16_t port, uint8_t v)
{
    (void)m;
    (void)port;
    (void)v;
}
