/* The built-in test card shown when no content is loaded. A pure integer
   function of (x, y, machine): no table, no font, no state, never drawn
   through the VDP. Its layout is fixed so its hash is pinned; ARCHITECTURE
   section 5 gives the field and crop it is drawn into, and docs/runner.md
   explains what each element reveals. First matching rule wins. */
#include "internal.h"

/* RGB444, 0000BBBBGGGGRRRR. */
#define WHITE 0x0FFFu
#define BLACK 0x0000u /* the backdrop */
#define RED 0x000Fu
#define GREEN 0x00F0u
#define BLUE 0x0F00u
#define MAGENTA 0x0F0Fu /* outside the Game Gear window only */

/* An SMS colour is 2 bits per channel, --BBGGRR. Each channel widens to 4
   bits as v | v << 2 (0, 5, 10, 15), exactly as a Game Gear in SMS mode
   expands it (ARCHITECTURE section 5). */
static uint16_t expand(uint32_t c)
{
    uint32_t r = c & 3u;
    uint32_t g = (c >> 2) & 3u;
    uint32_t b = (c >> 4) & 3u;
    return (uint16_t)(((b | b << 2) << 8) | ((g | g << 2) << 4) | (r | r << 2));
}

/* The SMS card fills the 256x192 crop. */
static uint16_t sms_pixel(uint32_t x, uint32_t y)
{
    /* A white block in the top-left corner only, so a mirror or flip shows. */
    if (x < 8u && y < 8u) {
        return WHITE;
    }
    /* A white one-dot frame on the crop edges, so cropping shows. */
    if (x == 0u || x == 255u || y == 0u || y == 191u) {
        return WHITE;
    }
    /* Red on top and blue on the right, so a red/blue swap looks different
       from a flip; green at the bottom. */
    if (y < 32u) {
        return RED;
    }
    if (y >= 161u) {
        return GREEN;
    }
    /* A 1x1 checker on the left, which shows filtering and scaling. */
    if (x < 64u) {
        return ((x ^ y) & 1u) != 0u ? WHITE : BLACK;
    }
    if (x >= 193u) {
        return BLUE;
    }
    /* The chart, x 64..192 and y 32..160: an 8x8 grid of 15x15 swatches on
       black one-dot lines, each of the 64 SMS colours once. */
    uint32_t lx = x - 64u;
    uint32_t ly = y - 32u;
    if (lx % 16u == 0u || ly % 16u == 0u) {
        return BLACK;
    }
    uint32_t col = lx >> 4;
    uint32_t row = ly >> 4;
    uint32_t r = col & 3u;
    uint32_t g = row & 3u;
    uint32_t b = ((row >> 2) << 1) | (col >> 2);
    return expand(b << 4 | g << 2 | r);
}

/* The Game Gear card fills the 160x144 window at (48, 24) of the field, in
   window coordinates u = x - 48, v = y - 24; the rest of the field is
   magenta, which is never hashed and names a wrong crop origin or pitch at
   once. */
static uint16_t gg_pixel(uint32_t x, uint32_t y)
{
    if (x < 48u || x >= 208u || y < 24u || y >= 168u) {
        return MAGENTA;
    }
    uint32_t u = x - 48u;
    uint32_t v = y - 24u;
    if (u < 8u && v < 8u) {
        return WHITE;
    }
    if (u == 0u || u == 159u || v == 0u || v == 143u) {
        return WHITE;
    }
    if (v < 8u) {
        return RED;
    }
    if (v >= 136u) {
        return GREEN;
    }
    if (u < 16u) {
        return ((u ^ v) & 1u) != 0u ? WHITE : BLACK;
    }
    if (u >= 144u) {
        return BLUE;
    }
    /* The chart, u 16..143 and v 8..135: 64x64 cells of 2x2 dots, each of the
       4,096 RGB444 values once. Red counts along a row of 16 cells, green
       down a column of 16, and blue picks one of the 4x4 blocks. */
    uint32_t cx = (u - 16u) >> 1;
    uint32_t cy = (v - 8u) >> 1;
    uint32_t r = cx & 15u;
    uint32_t g = cy & 15u;
    uint32_t b = ((cy >> 4) << 2) | (cx >> 4);
    return (uint16_t)(b << 8 | g << 4 | r);
}

uint16_t cogmaster__testcard_pixel(uint32_t x, uint32_t y, uint32_t machine)
{
    if (machine == COGMASTER_MACHINE_GG) {
        return gg_pixel(x, y);
    }
    return sms_pixel(x, y);
}
