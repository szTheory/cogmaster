/* Fast boot: the state the BIOS hands a cartridge, set directly and with no
   BIOS code (SMS-SYSTEM C7; ARCHITECTURE section 9). */
#include <string.h>

#include "internal.h"

void cogmaster__boot_handoff(cogmaster *m, int power)
{
    struct cogmaster__z80 *z = &m->z80;
    /* The Z80 after /RESET: PC, I, R, IFF1, IFF2 and IM 0; AF $FFFF; the
       other pairs are left 0 by convention. The BIOS leaves SP at $DFF0. */
    memset(z, 0, sizeof *z);
    z->reg[COGMASTER__Z80_A] = 0xFFu;
    z->reg[COGMASTER__Z80_F] = 0xFFu;
    z->sp = 0xDFF0u;
    if (power) {
        memset(m->ram, 0, sizeof m->ram);
    }
    /* The Master System BIOS stores its last port $3E value, $AB, at $C000;
       whether the Game Gear BIOS does is unverified, so it is left alone. */
    if (m->machine != COGMASTER_MACHINE_GG) {
        m->ram[0] = 0xABu;
    }
    /* The BIOS leaves the Sega mapper paged 0, 0, 1, 2 ($FFFC to $FFFF);
       the values it wrote to the RAM mirror are not documented, so the
       mirror is left alone (SMS-SYSTEM C7). */
    cogmaster__mapper_reset(m);
    /* $3E as the BIOS leaves it ($AB, or $A8 on the Game Gear), $3F $FF and
       the Game Gear's own ports at their power-on values (C7). */
    cogmaster__io_reset(m);
}

/* Power-on with a BIOS attached: the Z80 starts at address 0 with the BIOS visible and the BIOS
   does the hand-off itself (SMS-SYSTEM section 2 and section 3). Nothing the fast boot sets is
   set here: no $AB at the top of work RAM (the BIOS writes it), no cartridge-visible $3E.
   The Z80 after /RESET has PC, I, R, IM 0 and both flip-flops clear, and AF and SP $FFFF (the
   section marks this unverified). Work RAM clears on power and is kept on a soft reset. The
   mapper is at its power-on banks 0, 0, 1, 2: exact for a cartridge up to 48 KiB and an
   approximation after the BIOS checksums a larger one. $3E is $E0 (expansion, cartridge and card
   off; RAM, BIOS and I/O on): a derived constant, the value that BIOS visible and cartridge
   hidden force, which MAME builds the same way and ares approximates with only the BIOS enabled;
   it is not a value measured from a BIOS. The VDP reset is the one the fast boot already uses. */
void cogmaster__boot_power_on(cogmaster *m, int power)
{
    struct cogmaster__z80 *z = &m->z80;
    memset(z, 0, sizeof *z);
    z->reg[COGMASTER__Z80_A] = 0xFFu;
    z->reg[COGMASTER__Z80_F] = 0xFFu;
    z->sp = 0xFFFFu;
    if (power) {
        memset(m->ram, 0, sizeof m->ram);
    }
    cogmaster__mapper_reset(m);
    cogmaster__io_reset(m);
    m->io.mem_ctrl = 0xE0u;
    cogmaster__bus_decode(m);
}
