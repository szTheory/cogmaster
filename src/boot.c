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
}
