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

uint16_t cogmaster__testcard_pixel(uint32_t x, uint32_t y, uint32_t machine)
{
    (void)machine;
    return sms_pixel(x, y);
}
