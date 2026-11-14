/* Graphics I and Graphics II: the mode select nibbles 0000 and 0010 (SMS-VDP section 7,
   docs/hardware/video.md), their backgrounds and their sprites. The table addresses are those of
   the TI TMS9918A data manual, section 2.4.1 and table 3-2, which the Master System video chips
   keep. Both modes are 192 lines of 32 by 24 cells with no scroll and no left-column mask, so R0
   bits 5 to 7, R8 and R9 change nothing. Every address is a 32-bit value masked to 14 bits here;
   the mode 4 address helpers are never used, because their 315-5124 masks apply register bits
   that mean something else in these modes. */
#include "internal.h"

#define LEGACY_LINES 192u
#define LEGACY_VRAM_MASK 0x3FFFu
#define LEGACY_SPRITES 32u
#define LEGACY_SLOTS 4u
#define LEGACY_STATUS_COINCIDENCE 0x20u
#define LEGACY_STATUS_FIFTH 0x40u

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

/* The colour a colour byte nibble shows: nibble 0 is the backdrop, the low nibble of R7. */
static uint16_t legacy_dot(const cogmaster *m, uint32_t nibble)
{
    return cogmaster__legacy_colour(m, nibble != 0u ? nibble : (uint32_t)(m->vdp.reg[7] & 15u));
}

/* The sprites of one line, scanned in index order from the attribute table at (R5 & 127) << 7: 32
   entries of Y, X, name and colour byte (TI TMS9918A manual 2.4.5, Sean Young's TMS9918A notes
   section 4). A Y of $D0 ends the list before any other test, wherever it stands (every source
   agrees). A sprite is on the line when the line is within its 8 or 16 lines, doubled by R1 bit 0,
   below its top line, which is Y + 1. The first four fill the slots; the fifth match ends the scan.
   Its attribute table index plus 1 stays in sprite_overflow, because the fifth sprite flag asserts
   at the first dot of the line, up to 84 ticks after this scan, and a status read in between
   clears the latched index but must not lose the candidate. Every Y match counts, a transparent
   or off-screen sprite included. The slots keep the colour byte under the mask $8F (early clock and
   colour): the bits between are ignored by the chip (Young), and a state with them set would not
   load. Models, where the sources disagree: a Y of $E0 or more is negative (ares; MAME's
   tms9928a.cpp tests above $E0 and its 315_5124.cpp $F0 or more, which differ from this only for
   a 32 line sprite with Y in $E0 to $EF); the early clock is attribute bit 7 alone and R0 bit 3
   does nothing (MAME and ares; msvdp says it shifts by 8); the fifth sprite is the fifth match
   whether or not it would draw. */
void cogmaster__legacy_sprite_scan(cogmaster *m, uint32_t line)
{
    struct cogmaster__vdp *v = &m->vdp;
    uint32_t base = (uint32_t)(v->reg[5] & 0x7Fu) << 7;
    uint32_t zoom = v->reg[1] & 1u, size = (v->reg[1] >> 1) & 1u;
    uint32_t height = (size != 0u ? 16u : 8u) << zoom;
    v->sprite_line = (uint8_t)line;
    v->sprite_count = 0u;
    v->sprite_scanned = 0u;
    v->sprite_overflow = 0u;
    v->sprite_legacy = 1u;
    v->sprite_zoom = (uint8_t)zoom;
    v->sprite_size = (uint8_t)size;
    v->sprite_shift = 0u;
    for (uint32_t i = 0u; i < LEGACY_SPRITES; i++) {
        uint32_t entry = base + 4u * i;
        uint32_t y = v->vram[entry & LEGACY_VRAM_MASK];
        v->sprite_scanned++;
        if (y == 0xD0u) {
            break;
        }
        int32_t top = y >= 0xE0u ? (int32_t)y - 255 : (int32_t)y + 1;
        int32_t delta = (int32_t)line - top;
        if (delta < 0 || (uint32_t)delta >= height) {
            continue;
        }
        if (v->sprite_count == LEGACY_SLOTS) {
            v->sprite_overflow = (uint8_t)(1u + i);
            break;
        }
        uint32_t slot = v->sprite_count++;
        uint32_t name = v->vram[(entry + 2u) & LEGACY_VRAM_MASK];
        uint32_t colour = v->vram[(entry + 3u) & LEGACY_VRAM_MASK];
        v->sprite[slot].x = v->vram[(entry + 1u) & LEGACY_VRAM_MASK];
        v->sprite[slot].tile = (uint8_t)(size != 0u ? name & 0xFCu : name);
        v->sprite[slot].row = (uint8_t)((uint32_t)delta >> zoom);
        v->sprite[slot].attr = (uint8_t)(colour & 0x8Fu);
    }
}

/* The sprite at dot x: the colour of the first selected slot whose pattern bit is set and whose
   colour is not 0, or 0 for none, and in *hits the number of slots with a pattern bit set there. A
   16 by 16 sprite is four 8 by 8 patterns, name + 0 top left, +1 bottom left, +2 top right, +3
   bottom right, the name having its two low bits cleared at the scan (TI 2.4.5). The pattern
   generator is at (R6 & 7) << 11. Magnification doubles every pattern dot both ways, with no slot
   rule: the 315-5124 slot i + 4 rule of mode 4 does not apply here (model; MAME's 315_5124.cpp
   applies it in every mode). X does not wrap: a sprite is clipped at the right edge, and the early
   clock can put its left part off the left edge. */
static uint32_t legacy_sprite_dot(const cogmaster *m, uint32_t x, uint32_t *hits)
{
    const struct cogmaster__vdp *v = &m->vdp;
    uint32_t zoom = v->sprite_zoom, size = v->sprite_size;
    uint32_t width = (size != 0u ? 16u : 8u) << zoom;
    uint32_t first = 0u;
    *hits = 0u;
    for (uint32_t i = 0u; i < v->sprite_count; i++) {
        int32_t left = (int32_t)v->sprite[i].x - ((v->sprite[i].attr & 0x80u) != 0u ? 32 : 0);
        uint32_t dx = (uint32_t)((int32_t)x - left);
        if (dx >= width) {
            continue;
        }
        uint32_t column = dx >> zoom, row = v->sprite[i].row, name = v->sprite[i].tile;
        if (size != 0u) {
            name += 2u * (column >> 3) + (row >> 3);
        }
        uint32_t at = ((uint32_t)(v->reg[6] & 7u) << 11) | (name << 3) | (row & 7u);
        if (((v->vram[at & LEGACY_VRAM_MASK] >> (7u - (column & 7u))) & 1u) != 0u) {
            (*hits)++;
            if (first == 0u) {
                first = v->sprite[i].attr & 15u;
            }
        }
    }
    return first;
}

/* Dots [x0, x1) of one line, x1 at most the field width. A set pattern bit shows the high nibble of
   the colour byte, a clear one the low nibble; a sprite dot of colour 1 to 15 replaces it, always:
   there is no priority bit here. Lines from 192 on show the backdrop alone: a mode 4 picture that
   latched 224 or 240 lines can hand over to these modes in mid-frame. Status, each at its dot:
   coincidence (bit 5) when two slots have a pattern bit set on one dot of 0 to 255, and the fifth
   sprite flag (bit 6) at the first dot of the line, with the index of the first fifth sprite since
   the last status read in bits 4 to 0 (TI 2.3.2 and 2.3.3). Models: the backdrop of a legacy
   picture is a palette entry of the legacy palette while the display is on, and the colour RAM
   backdrop while it is off (cogmaster__vdp_backdrop); coincidence is decided by the pattern bits,
   so a sprite of colour 0 collides (TI and Young; MAME's 315_5124.cpp counts opaque dots only and
   ares only a dot over an earlier opaque sprite); the index of the first fifth sprite stays until
   a read (Young; MAME's tms9928a.cpp keeps the first too), and the flag is not held back by the
   frame flag (the manual sets it only while the frame flag is clear). */
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
    if (x0 == 0u && v->sprite_overflow != 0u && (v->status & LEGACY_STATUS_FIFTH) == 0u) {
        m->vdp.fifth_index = (uint8_t)(v->sprite_overflow - 1u);
        m->vdp.status |= LEGACY_STATUS_FIFTH;
    }
    int graphics2 = (v->reg[0] & 2u) != 0u;
    uint32_t pattern = 0u, colour = 0u;
    uint16_t set = 0u, clear = 0u;
    for (uint32_t x = x0; x < x1; x++) {
        if (x == x0 || (x & 7u) == 0u) {
            legacy_cell(m, graphics2, x, line, &pattern, &colour);
            set = legacy_dot(m, colour >> 4);
            clear = legacy_dot(m, colour & 15u);
        }
        dots[x] = ((pattern >> (7u - (x & 7u))) & 1u) != 0u ? set : clear;
        if (v->sprite_count != 0u) {
            uint32_t hits = 0u, sprite = legacy_sprite_dot(m, x, &hits);
            if (hits > 1u) {
                m->vdp.status |= LEGACY_STATUS_COINCIDENCE;
            }
            if (sprite != 0u) {
                dots[x] = cogmaster__legacy_colour(m, sprite);
            }
        }
    }
}
