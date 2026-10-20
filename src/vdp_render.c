/* Mode 4 tiles and monotonic raster intervals. Hardware rules and the
   selected latch model are sourced in docs/hardware/video.md. */
#include <string.h>
#include "internal.h"

static uint16_t vdp_colour(const cogmaster *m, uint32_t index)
{
    if (cogmaster__native_gg(m))
        return (uint16_t)(m->vdp.cram[index * 2u] | ((uint16_t)m->vdp.cram[index * 2u + 1u] << 8));
    uint8_t c = m->vdp.cram[index];
    return (uint16_t)(((c & 3u) * 5u) | (((c >> 2) & 3u) * 5u << 4) | (((c >> 4) & 3u) * 5u << 8));
}

uint16_t cogmaster__vdp_backdrop(const cogmaster *m)
{
    return vdp_colour(m, 16u + (m->vdp.reg[7] & 15u));
}

/* SMS1 AND gates apply R3 to A13..A6 for planes 0/1, R4 to
   A13..A11 for planes 2/3 (SMS Power VDPRegisters R3/R4, video.md). */
uint32_t cogmaster__vdp_pattern_address(const cogmaster *m, uint32_t address)
{
    if (cogmaster__profile(m).vdp_masks) {
        uint32_t mask = (address & 2u) != 0u ? ((uint32_t)m->vdp.reg[4] << 11) | 0x07FFu
                                             : ((uint32_t)m->vdp.reg[3] << 6) | 0x003Fu;
        address &= mask;
    }
    return address & 0x3FFFu;
}

/* R5 bit 0 gates A7 only on SMS1; the upper bits locate the SAT. */
uint32_t cogmaster__vdp_sat_address(const cogmaster *m, uint32_t offset)
{
    uint32_t address = ((uint32_t)(m->vdp.reg[5] & 0x7Eu) << 7) | (offset & 255u);
    if (cogmaster__profile(m).vdp_masks && (m->vdp.reg[5] & 1u) == 0u) {
        address &= ~0x80u;
    }
    return address;
}

/* R6 bit 2 supplies A13; SMS1 bits 1/0 gate A12/A11 respectively. */
uint32_t cogmaster__vdp_sprite_address(const cogmaster *m, uint32_t tile, uint32_t row)
{
    uint32_t address =
        ((uint32_t)(m->vdp.reg[6] & 4u) << 11) | ((tile & 255u) << 5) | ((row & 7u) << 2);
    if (cogmaster__profile(m).vdp_masks) {
        address &= ((uint32_t)(m->vdp.reg[6] & 3u) << 11) | 0x27FFu;
    }
    return address;
}

/* Legacy and invalid text combinations have no renderer. Extended heights
   on the 315-5124 also select invalid text (SMS-VDP section 2.5). */
static int vdp_supported(const cogmaster *m)
{
    const uint8_t *r = m->vdp.reg;
    uint32_t mode =
        ((uint32_t)(r[0] & 4u) << 1) | ((r[1] & 8u) >> 1) | (r[0] & 2u) | ((r[1] & 16u) >> 4);
    if (cogmaster__profile(m).vdp_masks) {
        return mode == 8u || mode == 10u || mode == 12u;
    }
    return mode >= 8u && mode != 9u && mode != 13u;
}

/* Palette index plus priority; zero retains its colour without priority. */
uint32_t cogmaster__vdp_background(const cogmaster *m, uint32_t x, uint32_t y)
{
    const struct cogmaster__vdp *v = &m->vdp;
    uint32_t scroll = (v->reg[0] & 0x40u) != 0u && y < 16u ? 0u : v->hscroll;
    if (!v->display || !vdp_supported(m) || ((v->reg[0] & 0x20u) != 0u && x < 8u) ||
        x < (scroll & 7u)) {
        return 16u + (v->reg[7] & 15u);
    }
    uint32_t sx = (x + 256u - scroll) & 255u;
    uint32_t sy = (y + (((v->reg[0] & 0x80u) != 0u && sx >= 192u) ? 0u : v->vscroll)) %
                  (v->height == 192u ? 224u : 256u);
    uint32_t base = v->height == 192u ? (uint32_t)(v->reg[2] & 14u) << 10
                                      : ((uint32_t)(v->reg[2] & 12u) << 10) | 0x700u;
    uint32_t name = base + (sy / 8u * 32u + sx / 8u) * 2u;
    if (cogmaster__profile(m).vdp_masks && (v->reg[2] & 1u) == 0u) {
        name &= ~0x400u;
    }
    uint32_t attr = v->vram[name & 0x3FFFu] | ((uint32_t)v->vram[(name + 1u) & 0x3FFFu] << 8);
    uint32_t tx = sx & 7u, ty = sy & 7u;
    if ((attr & 0x200u) != 0u) {
        tx = 7u - tx;
    }
    if ((attr & 0x400u) != 0u) {
        ty = 7u - ty;
    }
    uint32_t row = (attr & 0x1FFu) * 32u + ty * 4u;
    uint32_t index = 0u;
    for (uint32_t plane = 0u; plane < 4u; plane++) {
        uint32_t address = cogmaster__vdp_pattern_address(m, row + plane);
        index |= ((uint32_t)(v->vram[address] >> (7u - tx)) & 1u) << plane;
    }
    return index + ((attr & 0x800u) != 0u ? 16u : 0u) +
           ((attr & 0x1000u) != 0u && index != 0u ? COGMASTER__VDP_PRIORITY : 0u);
}

/* Y+1, negative F0..FF, D0 termination, eight retained slots and the
   ninth Y match: SMS-VDP section 3, docs/hardware/video.md. */
void cogmaster__vdp_sprite_scan(cogmaster *m, uint32_t line)
{
    struct cogmaster__vdp *v = &m->vdp;
    v->sprite_line = (uint8_t)line;
    v->sprite_count = 0u;
    v->sprite_scanned = 0u;
    v->sprite_overflow = 0u;
    v->sprite_zoom = v->reg[1] & 1u;
    v->sprite_shift = (uint8_t)((v->reg[0] >> 3) & 1u);
    uint32_t height = (v->reg[1] & 2u) != 0u ? 16u : 8u;
    for (uint32_t i = 0u; i < 64u; i++) {
        uint8_t y = v->vram[cogmaster__vdp_sat_address(m, i)];
        v->sprite_scanned++;
        if (y == 0xD0u && v->height == 192u) {
            break;
        }
        int32_t top = y >= 0xF0u ? (int32_t)y - 255 : (int32_t)y + 1;
        int32_t delta = (int32_t)line - top;
        if (delta < 0 || (uint32_t)delta >= (height << v->sprite_zoom)) {
            continue;
        }
        if (v->sprite_count == 8u) {
            v->sprite_overflow = 1u;
            break;
        }
        uint32_t slot = v->sprite_count++;
        uint32_t row = (uint32_t)delta >> v->sprite_zoom;
        uint32_t tile = v->vram[cogmaster__vdp_sat_address(m, 129u + 2u * i)];
        if (height == 16u) {
            tile = (tile & 254u) + (row >> 3);
        }
        v->sprite[slot].x = v->vram[cogmaster__vdp_sat_address(m, 128u + 2u * i)];
        v->sprite[slot].tile = (uint8_t)tile;
        v->sprite[slot].row = (uint8_t)(row & 7u);
    }
}

/* Earlier opaque SAT slots win. SMS1 horizontal doubling requires a
   populated slot i+4; vertical doubling applies to all selected slots
   (SMS-VDP 3.A, ARCHITECTURE section 13). */
static uint32_t vdp_sprite_pixel(cogmaster *m, uint32_t x)
{
    struct cogmaster__vdp *v = &m->vdp;
    uint32_t first = 0u;
    for (uint32_t i = 0u; i < v->sprite_count; i++) {
        int32_t left = (int32_t)v->sprite[i].x - (v->sprite_shift ? 8 : 0);
        uint32_t dx = (uint32_t)((int32_t)x - left);
        uint32_t wide =
            v->sprite_zoom && (!cogmaster__profile(m).vdp_masks || i + 4u < v->sprite_count);
        if (dx >= (8u << wide)) {
            continue;
        }
        uint32_t row = cogmaster__vdp_sprite_address(m, v->sprite[i].tile, v->sprite[i].row);
        uint32_t colour = 0u;
        for (uint32_t plane = 0u; plane < 4u; plane++) {
            colour |= ((uint32_t)(v->vram[row + plane] >> (7u - (dx >> wide))) & 1u) << plane;
        }
        if (colour != 0u) {
            if (first != 0u) {
                /* Shared opaque active dot, independent of background priority
                   (SMS-VDP section 3, docs/hardware/video.md). */
                v->status |= 0x20u;
                return first;
            }
            first = 16u + colour;
        }
    }
    return first;
}

/* Exclusive raster coordinate: stamped mutations cannot repaint earlier dots. */
void cogmaster__vdp_render_until(cogmaster *m, uint32_t line, uint32_t x)
{
    uint32_t end = line >= m->vdp.height ? m->vdp.height * COGMASTER__FIELD_W
                                         : line * COGMASTER__FIELD_W +
                                               (x > COGMASTER__FIELD_W ? COGMASTER__FIELD_W : x);
    uint32_t at = m->render_line * COGMASTER__FIELD_W + m->render_x;
    if (at < end && m->vdp.display && !vdp_supported(m)) {
        cogmaster__bus_fault(m, 0u, COGMASTER_FAULT_DEVICE_VDP,
                             COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE);
    }
    while (at < end) {
        if (m->vdp.sprite_line != m->render_line) {
            cogmaster__vdp_sprite_scan(m, m->render_line);
        }
        uint32_t colour = cogmaster__vdp_background(m, m->render_x, m->render_line);
        if (m->vdp.display && vdp_supported(m) &&
            !((m->vdp.reg[0] & 0x20u) != 0u && m->render_x < 8u)) {
            uint32_t sprite = vdp_sprite_pixel(m, m->render_x);
            if (sprite != 0u && (colour & COGMASTER__VDP_PRIORITY) == 0u) {
                colour = sprite;
            }
        }
        /* Selected model: a ninth Y match asserts at the line's first
           enabled active dot, regardless of opacity or X (video.md). */
        if (m->render_x == 0u && m->vdp.display && vdp_supported(m) && m->vdp.sprite_overflow) {
            m->vdp.status |= 0x40u;
        }
        m->field[m->render_line][m->render_x] = vdp_colour(m, colour & 31u);
        at++;
        if (++m->render_x == COGMASTER__FIELD_W) {
            m->render_x = 0u;
            m->render_line++;
        }
    }
}

void cogmaster__vdp_render_to(cogmaster *m, uint32_t end)
{
    cogmaster__vdp_render_until(m, end, 0u);
}

void cogmaster__vdp_finish_field(cogmaster *m)
{
    cogmaster__vdp_render_to(m, m->vdp.height);
    m->completed.height = m->vdp.height;
    m->completed.crop =
        cogmaster__crop(cogmaster__native_gg(m) ? COGMASTER_MACHINE_GG : COGMASTER_MACHINE_SMS2);
    if (!cogmaster__native_gg(m)) {
        m->completed.crop.h = m->vdp.height;
    } else if (m->vdp.height == 224u) {
        m->completed.crop.y = 40u;
    }
    m->completed.backdrop = cogmaster__vdp_backdrop(m);
    memcpy(m->completed.pixels, m->field, sizeof m->field);
    /* Selected LCD model: 240 lines freezes the last native picture until
       POWER reset; the diagnostic full field continues (SMS-VDP section 6). */
    if (cogmaster__native_gg(m)) {
        if (m->vdp.height == 240u)
            m->lcd.frozen = 1u;
        if (!m->lcd.frozen) {
            m->lcd.height = m->vdp.height;
            m->lcd.y = m->completed.crop.y;
            m->lcd.backdrop = m->completed.backdrop;
            for (uint32_t y = 0u; y < 144u; y++)
                memcpy(m->lcd.pixels[y], &m->field[m->lcd.y + y][48], sizeof m->lcd.pixels[y]);
        }
    }
    m->render_line = 0u;
    m->render_x = 0u;
    m->vdp.sprite_line = m->vdp.height;
}
