/* The 192-line Mode 4 background, decoded from four bitplanes and the
   little-endian name table (SMS-VDP sections 2.3, 2.4 and 3.1). Lines
   already emitted are immutable until the next field. */
#include "internal.h"

static uint16_t vdp_colour(uint8_t c)
{
    return (uint16_t)(((c & 3u) * 5u << 8) | (((c >> 2) & 3u) * 5u << 4) |
                      (((c >> 4) & 3u) * 5u));
}

static void vdp_line(cogmaster *m, uint32_t y)
{
    const struct cogmaster__vdp *v = &m->vdp;
    uint16_t backdrop = vdp_colour(v->cram[16u + (v->reg[7] & 15u)]);
    for (uint32_t x = 0u; x < COGMASTER__FIELD_W; x++) {
        uint16_t pixel = backdrop;
        if ((v->reg[1] & 0x40u) != 0u && (v->reg[0] & 4u) != 0u) {
            uint32_t sx = (x + 256u - v->reg[8]) & 255u;
            uint32_t sy = (y + v->reg[9]) % 224u;
            uint32_t name = ((uint32_t)(v->reg[2] & 14u) << 10) +
                            (sy / 8u * 32u + sx / 8u) * 2u;
            uint32_t attr = v->vram[name & 0x3FFFu] |
                            ((uint32_t)v->vram[(name + 1u) & 0x3FFFu] << 8);
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
            pixel = vdp_colour(v->cram[index + ((attr & 0x800u) != 0u ? 16u : 0u)]);
        }
        m->field[y][x] = pixel;
    }
}

void cogmaster__vdp_render_to(cogmaster *m, uint32_t end)
{
    if (end > COGMASTER__FIELD_H) {
        end = COGMASTER__FIELD_H;
    }
    while (m->render_line < end) {
        vdp_line(m, m->render_line++);
    }
}

void cogmaster__vdp_finish_field(cogmaster *m)
{
    cogmaster__vdp_render_to(m, COGMASTER__FIELD_H);
    m->render_line = 0u;
}
