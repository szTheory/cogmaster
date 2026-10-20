/* Mode 4 tiles and monotonic raster intervals. Hardware rules and the
   selected latch model are sourced in docs/hardware/video.md. */
#include "internal.h"

static uint16_t vdp_colour(uint8_t c)
{
    return (uint16_t)(((c & 3u) * 5u) | (((c >> 2) & 3u) * 5u << 4) | (((c >> 4) & 3u) * 5u << 8));
}

uint16_t cogmaster__vdp_backdrop(const cogmaster *m)
{
    return vdp_colour(m->vdp.cram[16u + (m->vdp.reg[7] & 15u)]);
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
    uint32_t sy = (y + (((v->reg[0] & 0x80u) != 0u && sx >= 192u) ? 0u : v->vscroll)) % 224u;
    uint32_t name = ((uint32_t)(v->reg[2] & 14u) << 10) + (sy / 8u * 32u + sx / 8u) * 2u;
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

/* Exclusive raster coordinate: stamped mutations cannot repaint earlier dots. */
void cogmaster__vdp_render_until(cogmaster *m, uint32_t line, uint32_t x)
{
    uint32_t end =
        line >= COGMASTER__FIELD_H
            ? COGMASTER__FIELD_H * COGMASTER__FIELD_W
            : line * COGMASTER__FIELD_W + (x > COGMASTER__FIELD_W ? COGMASTER__FIELD_W : x);
    uint32_t at = m->render_line * COGMASTER__FIELD_W + m->render_x;
    if (at < end && m->vdp.display && !vdp_supported(m)) {
        cogmaster__bus_fault(m, 0u, COGMASTER_FAULT_DEVICE_VDP,
                             COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE);
    }
    while (at < end) {
        uint32_t colour = cogmaster__vdp_background(m, m->render_x, m->render_line);
        m->field[m->render_line][m->render_x] = vdp_colour(m->vdp.cram[colour & 31u]);
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
    cogmaster__vdp_render_to(m, COGMASTER__FIELD_H);
    m->render_line = 0u;
    m->render_x = 0u;
}
