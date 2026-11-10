/* The cartridge boards outside the three arms of mapper.c: one switch arm per
   board, reached from the default arm of each mapper.c switch, so the Sega,
   Codemasters and ROM-only paths never see them. The dispatch is a switch and
   not a table of function pointers because a pointer table built at load time
   in a position-independent library lands in a relocated read-only data
   section, which the symbol check on the built library rejects. */
#include "internal.h"

uint8_t cogmaster__mapext_read(const cogmaster *m, uint16_t addr)
{
    (void)addr;
    switch (m->cart.board) {
    default:
        return 0xFFu;
    }
}

void cogmaster__mapext_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    (void)v;
    switch (m->cart.board) {
    default:
        /* Writes to ROM change nothing. $FFFC-$FFFF are work RAM only. */
        if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    }
}

void cogmaster__mapext_reset(cogmaster *m)
{
    switch (m->cart.board) {
    default:
        /* The caller cleared every bank. */
        break;
    }
}
