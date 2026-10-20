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

/* Palette index plus priority; zero retains its colour without priority. */
uint32_t cogmaster__vdp_background(const cogmaster *m, uint32_t x, uint32_t y)
{
    const struct cogmaster__vdp *v = &m->vdp;
    uint32_t scroll = (v->reg[0] & 0x40u) != 0u && y < 16u ? 0u : v->hscroll;
    if (!v->display || (v->reg[0] & 4u) == 0u || ((v->reg[0] & 0x20u) != 0u && x < 8u) ||
        x < (scroll & 7u)) {
        return 16u + (v->reg[7] & 15u);
    }
    uint32_t sx = (x + 256u - scroll) & 255u;
    uint32_t sy = (y + (((v->reg[0] & 0x80u) != 0u && sx >= 192u) ? 0u : v->vscroll)) % 224u;
    uint32_t name = ((uint32_t)(v->reg[2] & 14u) << 10) + (sy / 8u * 32u + sx / 8u) * 2u;
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
        index |= ((uint32_t)(v->vram[row + plane] >> (7u - tx)) & 1u) << plane;
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
