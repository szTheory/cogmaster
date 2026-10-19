/* cogmaster_run_frame and cogmaster_get_frame: every check first, then the
   work. With the test card loaded a frame redraws the card into the field. */
#include <string.h>

#include "internal.h"

cogmaster_status cogmaster_run_frame(cogmaster *m, cogmaster_run_result *out)
{
    if (m == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (out != NULL) {
        cogmaster_status st = cogmaster__check_out_size(out, (uint32_t)sizeof *out);
        if (st != COGMASTER_STATUS_OK) {
            return st;
        }
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    for (uint32_t y = 0; y < COGMASTER__FIELD_H; y++) {
        for (uint32_t x = 0; x < COGMASTER__FIELD_W; x++) {
            m->field[y][x] = cogmaster__testcard_pixel(x, y, m->machine);
        }
    }
    m->frame++;
    m->ticks += COGMASTER__TICKS_PER_FRAME_NTSC;
    if (out != NULL) {
        cogmaster_run_result r;
        memset(&r, 0, sizeof r);
        r.size = out->size;
        r.stop = COGMASTER_STOP_FRAME;
        r.overshoot_ticks = 0u;
        cogmaster__write_out(out, &r, (uint32_t)sizeof r);
    }
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster_get_frame(const cogmaster *m, cogmaster_frame *io)
{
    if (m == NULL || io == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_size(io, (uint32_t)sizeof *io, (uint32_t)sizeof *io);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (io->flags != 0u || io->pixels == NULL || io->pitch_bytes > COGMASTER__PITCH_MAX) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    /* The SMS crop is the whole 256-dot field (ARCHITECTURE section 5). */
    uint32_t x0 = 0u;
    uint32_t y0 = 0u;
    uint32_t w = COGMASTER__FIELD_W;
    uint32_t h = COGMASTER__FIELD_H;
    if (io->pitch_bytes < w * 2u) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    uint8_t *dst = (uint8_t *)io->pixels;
    for (uint32_t row = 0; row < h; row++) {
        memcpy(dst + (size_t)row * io->pitch_bytes, &m->field[y0 + row][x0], (size_t)w * 2u);
    }
    io->x = x0;
    io->y = y0;
    io->w = w;
    io->h = h;
    io->field_h = COGMASTER__FIELD_H;
    io->backdrop_rgb444 = 0u;
    return COGMASTER_STATUS_OK;
}
