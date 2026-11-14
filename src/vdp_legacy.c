/* Graphics I and Graphics II backgrounds: the mode select nibbles 0000 and 0010 (SMS-VDP section 7,
   docs/hardware/video.md). The table addresses are those of the TI TMS9918A data manual, section
   2.4.1 and table 3-2, which the Master System video chips keep. Both modes are 192 lines of 32 by
   24 cells with no scroll and no left-column mask, so R0 bits 5 to 7, R8 and R9 change nothing.
   Every address is a 32-bit value masked to 14 bits here; the mode 4 address helpers are never
   used, because their 315-5124 masks apply register bits that mean something else in these modes.
 */
#include "internal.h"

#define LEGACY_LINES 192u
#define LEGACY_VRAM_MASK 0x3FFFu

/* The Master System fixed palette of the legacy modes, as six-bit BBGGRR values (SMS Power palette
   page; ares ms/vdp/dac.cpp gives the same sixteen). Model: no source measures the Sega chip's
   legacy palette against its colour table. */
static const uint8_t legacy_palette[16] = {0x00, 0x00, 0x08, 0x0C, 0x10, 0x30, 0x01, 0x3C,
                                           0x02, 0x03, 0x05, 0x0F, 0x04, 0x33, 0x15, 0x3F};

/* Palette entry nibble 0 to 15. The Game Gear in Master System video mode reads colour RAM entry
   16 + n instead of the fixed table (msvdp section 14, SMS Power Game Gear notes). Model: the
   sources disagree, ares uses the fixed table there. */
uint16_t cogmaster__legacy_colour(const cogmaster *m, uint32_t nibble)
{
    nibble &= 15u;
    if (m->machine == COGMASTER_MACHINE_GG && !cogmaster__native_gg(m)) {
        return cogmaster__vdp_rgb444(m->vdp.cram[16u + nibble]);
    }
    return cogmaster__vdp_rgb444(legacy_palette[nibble]);
}

/* The pattern byte and the colour byte of the cell that holds dot x on this line.
   Graphics I: the name selects eight pattern rows and one colour byte for eight names.
   Graphics II: the name and the third of the screen, line / 64, form a 768 entry index into
   pattern and colour tables that hold one byte per entry and line. Model: the Master System 1
   limits that index with the low bits of R3 and R4, as MAME's 315_5124 file does; the Master System
   2 and the Game Gear in Master System video mode do not (msvdp). R3 and R4 values with all those
   bits set, the ones games use, behave the same either way. */
static void legacy_cell(const cogmaster *m, int graphics2, uint32_t x, uint32_t line,
                        uint32_t *pattern, uint32_t *colour)
{
    const struct cogmaster__vdp *v = &m->vdp;
    uint32_t cell = ((uint32_t)(v->reg[2] & 15u) << 10) | ((line >> 3) << 5) | (x >> 3);
    uint32_t name = v->vram[cell & LEGACY_VRAM_MASK];
    uint32_t row = line & 7u;
    if (graphics2) {
        uint32_t index = ((line >> 6) << 8) | name;
        uint32_t pattern_index = index, colour_index = index;
        if (cogmaster__profile(m).vdp_masks) {
            pattern_index &= (((uint32_t)v->reg[4] & 3u) << 8) | 0xFFu;
            colour_index &= (((uint32_t)v->reg[3] & 0x7Fu) << 3) | 7u;
        }
        uint32_t pattern_at = (((uint32_t)v->reg[4] & 4u) << 11) | (pattern_index << 3) | row;
        uint32_t colour_at = (((uint32_t)v->reg[3] & 0x80u) << 6) | (colour_index << 3) | row;
        *pattern = v->vram[pattern_at & LEGACY_VRAM_MASK];
        *colour = v->vram[colour_at & LEGACY_VRAM_MASK];
    } else {
        uint32_t pattern_at = (((uint32_t)v->reg[4] & 7u) << 11) | (name << 3) | row;
        uint32_t colour_at = (((uint32_t)v->reg[3] << 6) & LEGACY_VRAM_MASK) | (name >> 3);
        *pattern = v->vram[pattern_at & LEGACY_VRAM_MASK];
        *colour = v->vram[colour_at & LEGACY_VRAM_MASK];
    }
}

/* Dots [x0, x1) of one line, x1 at most the field width. A set pattern bit shows the high nibble of
   the colour byte, a clear one the low nibble, and nibble 0 shows the backdrop, the low nibble of
   R7. Lines from 192 on show the backdrop alone: a mode 4 picture that latched 224 or 240 lines can
   hand over to these modes in mid-frame. Model: the backdrop of a legacy picture is a palette
   entry of the legacy palette while the display is on, and the colour RAM backdrop while it is
   off (cogmaster__vdp_backdrop). */
void cogmaster__legacy_span(cogmaster *m, uint32_t line, uint32_t x0, uint32_t x1)
{
    const struct cogmaster__vdp *v = &m->vdp;
    uint16_t *dots = m->field[line];
    if (line >= LEGACY_LINES || !v->display) {
        uint16_t backdrop = cogmaster__vdp_backdrop(m);
        for (uint32_t x = x0; x < x1; x++) {
            dots[x] = backdrop;
        }
        return;
    }
    uint16_t colours[16];
    for (uint32_t n = 0u; n < 16u; n++) {
        colours[n] = cogmaster__legacy_colour(m, n);
    }
    colours[0] = colours[v->reg[7] & 15u];
    int graphics2 = (v->reg[0] & 2u) != 0u;
    uint32_t pattern = 0u, colour = 0u;
    for (uint32_t x = x0; x < x1; x++) {
        if (x == x0 || (x & 7u) == 0u) {
            legacy_cell(m, graphics2, x, line, &pattern, &colour);
        }
        uint32_t set = (pattern >> (7u - (x & 7u))) & 1u;
        dots[x] = colours[set != 0u ? colour >> 4 : colour & 15u];
    }
}
