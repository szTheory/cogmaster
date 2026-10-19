/* Host colour helpers, outside the core's determinism boundary: nothing here
   is hashed. The runner's PNG and the libretro adapter use them. */
#ifndef COGMASTER_HOST_PALETTE_H
#define COGMASTER_HOST_PALETTE_H

#include <stdint.h>

/* An RGB444 dot, 0000BBBBGGGGRRRR, as 0x00RRGGBB with each 4-bit channel
   times 0x11, so c >> 4 gives the channel back. */
uint32_t cogmaster__rgb444_to_rgb888(uint16_t p);

/* An RGB444 dot as RGB565, each channel widened by repeating its high bits. */
uint16_t cogmaster__rgb444_to_rgb565(uint16_t p);

#endif /* COGMASTER_HOST_PALETTE_H */
