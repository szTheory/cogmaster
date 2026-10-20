/* RGB444 to the host's pixel formats. A core dot is 0000BBBBGGGGRRRR
   (.planning/preparation/ARCHITECTURE.md section 5). Each 4-bit channel is
   widened by repeating its bits, so full scale stays full scale and the
   original value is the top bits of the result. */
#include "palette.h"

uint32_t cogmaster__rgb444_to_rgb888(uint16_t p)
{
    uint32_t r = p & 15u;
    uint32_t g = (p >> 4) & 15u;
    uint32_t b = (p >> 8) & 15u;
    return (r * 0x11u) << 16 | (g * 0x11u) << 8 | b * 0x11u;
}

uint16_t cogmaster__rgb444_to_rgb565(uint16_t p)
{
    uint32_t r = p & 15u;
    uint32_t g = (p >> 4) & 15u;
    uint32_t b = (p >> 8) & 15u;
    uint32_t r5 = r << 1 | r >> 3;
    uint32_t g6 = g << 2 | g >> 2;
    uint32_t b5 = b << 1 | b >> 3;
    return (uint16_t)(r5 << 11 | g6 << 5 | b5);
}
