/* cogmaster_run_frame and cogmaster_get_frame: every check first, then the
   work. With the test card loaded a frame redraws the card into the field;
   with an image it runs the Z80 to the frame deadline. */
#include <string.h>

#include "internal.h"

cogmaster__rect cogmaster__crop(uint32_t machine)
{
    cogmaster__rect r;
    if (machine == COGMASTER_MACHINE_GG) {
        r.x = 48u;
        r.y = 24u;
        r.w = 160u;
        r.h = 144u;
    } else {
        r.x = 0u;
        r.y = 0u;
        r.w = COGMASTER__FIELD_W;
        r.h = COGMASTER__FIELD_H;
    }
    return r;
}

cogmaster_status cogmaster_run_frame(cogmaster *m, cogmaster_run_result *out)
{
    if (m == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (out != NULL) {
        cogmaster_status st = cogmaster__check_out_size(out, COGMASTER__FIRST_SIZE_RUN_RESULT);
        if (st != COGMASTER_STATUS_OK) {
            return st;
        }
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    uint32_t overshoot = 0u;
    /* Absolute host levels latch once at the frame boundary; only a Pause
       rising edge queues the console NMI (SMS-CARTRIDGES section 3). */
    if (m->pending_input.pause && !m->input.pause) {
        m->vdp.pause = 1u;
    }
    m->input = m->pending_input;
    m->input_read = 0u;
    if (m->cart.rom != NULL) {
        /* Beam events and stamped writes advance the production renderer. */
        overshoot = cogmaster__bus_run_frame(m);
    } else {
        for (uint32_t y = 0; y < COGMASTER__FIELD_H; y++) {
            for (uint32_t x = 0; x < COGMASTER__FIELD_W; x++) {
                m->field[y][x] = cogmaster__testcard_pixel(x, y, m->machine);
            }
        }
        /* The card is exactly one frame: the beam ends where it began. */
        m->frame++;
        uint32_t ticks = cogmaster__frame_ticks(m);
        m->ticks += ticks;
        m->bus.at += ticks;
        m->bus.deadline += ticks;
    }
    uint32_t lagged = m->input_read == 0u ? 1u : 0u;
    m->counters.lag_frames += lagged;
    if (out != NULL) {
        cogmaster_run_result r;
        memset(&r, 0, sizeof r);
        r.size = out->size;
        r.stop = COGMASTER_STOP_FRAME;
        r.overshoot_ticks = overshoot;
        r.lagged = lagged;
        cogmaster__write_out(out, &r, (uint32_t)sizeof r);
    }
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster_get_frame(const cogmaster *m, cogmaster_frame *io)
{
    if (m == NULL || io == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st =
        cogmaster__check_size(io, COGMASTER__FIRST_SIZE_FRAME, (uint32_t)sizeof *io);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    if ((io->flags & ~COGMASTER_FRAME_FULL_FIELD) != 0u || io->pixels == NULL ||
        io->pitch_bytes > COGMASTER__PITCH_MAX) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster__rect r;
    if ((io->flags & COGMASTER_FRAME_FULL_FIELD) != 0u) {
        r.x = 0u;
        r.y = 0u;
        r.w = COGMASTER__FIELD_W;
        r.h = COGMASTER__FIELD_H;
    } else {
        r = cogmaster__crop(m->machine);
    }
    if (io->pitch_bytes < r.w * 2u) {
        return COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL;
    }
    uint8_t *dst = (uint8_t *)io->pixels;
    for (uint32_t row = 0; row < r.h; row++) {
        memcpy(dst + (size_t)row * io->pitch_bytes, &m->field[r.y + row][r.x], (size_t)r.w * 2u);
    }
    io->x = r.x;
    io->y = r.y;
    io->w = r.w;
    io->h = r.h;
    io->field_h = COGMASTER__FIELD_H;
    io->backdrop_rgb444 = 0u;
    return COGMASTER_STATUS_OK;
}
