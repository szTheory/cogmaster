/* Pointer to light-gun aim. libretro.h gives a pointer as -32767 to 32767 across
   the game's screen, 0 at the centre, and -32768 for "out of bounds". The gun
   takes a 1-based field aim with 0 0 meaning "no aim" (the core's rule), so a
   pointer that is off screen is the parked state and is never clamped onto an
   edge dot: a parked gun sees nothing, an edge dot would see the picture's
   border. All arithmetic is 32-bit: (s + 32768) * w is at most 65535 * 65535. */
#include "aim.h"

#include <stddef.h>

#define AIM_FIELD_MAX 0xFFFFu
#define AIM_SCREEN_MAX 32767

static int aim_rect_ok(const struct cogmaster__aim_rect *r)
{
    return r != NULL && r->w != 0u && r->h != 0u && r->x <= AIM_FIELD_MAX &&
           r->y <= AIM_FIELD_MAX && r->w <= AIM_FIELD_MAX && r->h <= AIM_FIELD_MAX;
}

/* One axis: the 1-based dot for screen coordinate s over origin and size. */
static uint32_t aim_axis(uint32_t origin, uint32_t size, int32_t s)
{
    uint32_t u = (uint32_t)(s + 32768);
    return origin + ((u * size) >> 16) + 1u;
}

void cogmaster__aim_from_screen(const struct cogmaster__aim_rect *rect, int32_t sx, int32_t sy,
                                int offscreen, uint32_t *ax, uint32_t *ay)
{
    if (ax == NULL || ay == NULL) {
        return;
    }
    *ax = 0u;
    *ay = 0u;
    if (offscreen != 0 || !aim_rect_ok(rect) || sx < -AIM_SCREEN_MAX || sx > AIM_SCREEN_MAX ||
        sy < -AIM_SCREEN_MAX || sy > AIM_SCREEN_MAX) {
        return;
    }
    *ax = aim_axis(rect->x, rect->w, sx);
    *ay = aim_axis(rect->y, rect->h, sy);
}

/* One axis of the cursor: centre when unset, add the delta clamped to the
   size (so the sum cannot overflow), then clamp into the rectangle's 1-based range. */
static uint32_t aim_move(uint32_t first, uint32_t size, uint32_t cur, int32_t d)
{
    int32_t lo = (int32_t)first + 1;
    int32_t hi = (int32_t)(first + size);
    int32_t span = (int32_t)size;
    int32_t v;
    if (cur == COGMASTER_AIM_CURSOR_UNSET) {
        v = (int32_t)(first + size / 2u) + 1;
    } else if (cur > (uint32_t)hi) {
        v = hi;
    } else {
        v = (int32_t)cur;
    }
    if (d > span) {
        d = span;
    } else if (d < -span) {
        d = -span;
    }
    v += d;
    if (v < lo) {
        v = lo;
    } else if (v > hi) {
        v = hi;
    }
    return (uint32_t)v;
}

void cogmaster__aim_step(const struct cogmaster__aim_rect *rect, uint32_t *cx, uint32_t *cy,
                         int32_t dx, int32_t dy)
{
    if (cx == NULL || cy == NULL || !aim_rect_ok(rect)) {
        return;
    }
    *cx = aim_move(rect->x, rect->w, *cx, dx);
    *cy = aim_move(rect->y, rect->h, *cy, dy);
}

static int aim_inside(const struct cogmaster__aim_rect *r, int32_t x, int32_t y)
{
    return x >= (int32_t)r->x && x < (int32_t)(r->x + r->w) && y >= (int32_t)r->y &&
           y < (int32_t)(r->y + r->h);
}

static uint32_t aim_put(const struct cogmaster__aim_rect *r, struct cogmaster__aim_pt *pts,
                        uint32_t n, int32_t x, int32_t y)
{
    if (aim_inside(r, x, y)) {
        pts[n].x = (uint32_t)x;
        pts[n].y = (uint32_t)y;
        return n + 1u;
    }
    return n;
}

uint32_t cogmaster__aim_cross(const struct cogmaster__aim_rect *rect, uint32_t ax, uint32_t ay,
                              struct cogmaster__aim_pt *pts)
{
    if (pts == NULL || !aim_rect_ok(rect) || ax == 0u || ay == 0u || ax > AIM_FIELD_MAX + 1u ||
        ay > AIM_FIELD_MAX + 1u) {
        return 0u;
    }
    int32_t x = (int32_t)ax - 1;
    int32_t y = (int32_t)ay - 1;
    if (!aim_inside(rect, x, y)) {
        return 0u;
    }
    uint32_t n = aim_put(rect, pts, 0u, x, y);
    for (int32_t k = 1; k <= 4; k++) {
        n = aim_put(rect, pts, n, x + k, y);
        n = aim_put(rect, pts, n, x - k, y);
        n = aim_put(rect, pts, n, x, y + k);
        n = aim_put(rect, pts, n, x, y - k);
    }
    return n;
}
