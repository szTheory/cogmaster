/* Host light-gun helper, outside the core's determinism boundary: nothing here
   is hashed or saved. The libretro adapter uses it to turn a pointer into the
   1-based field aim the core takes and to draw a crosshair. */
#ifndef COGMASTER_HOST_AIM_H
#define COGMASTER_HOST_AIM_H

#include <stdint.h>

/* The picture's rectangle in field dots: the last completed frame's x, y, w
   and h. Each field must be at most 65535; a rectangle with w or h of 0 holds
   no aim. */
struct cogmaster__aim_rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

/* A crosshair dot in 0-based field coordinates, inside the rectangle. The
   adapter subtracts the rectangle's x and y to index its own buffer. */
struct cogmaster__aim_pt {
    uint32_t x;
    uint32_t y;
};

#define COGMASTER_AIM_CURSOR_UNSET UINT32_MAX
#define COGMASTER_AIM_CROSS_MAX 17u

/* Maps pointer coordinates (-32767 to 32767, 0 the centre of the screen,
   -32768 out of bounds; libretro.h) through rect to a 1-based field aim: the
   first dot of the rectangle is 1 more than rect->x. The offscreen flag, either
   coordinate out of -32767 to 32767, or an empty rectangle, gives 0 0, never
   an edge dot. A NULL rect parks; a NULL output pointer does nothing. */
void cogmaster__aim_from_screen(const struct cogmaster__aim_rect *rect, int32_t sx, int32_t sy,
                                int offscreen, uint32_t *ax, uint32_t *ay);

/* Moves a 1-based cursor by dx and dy and keeps it inside rect. A cursor of
   COGMASTER_AIM_CURSOR_UNSET (or one outside rect) is first put at the centre
   or the nearest edge. Any int32 delta is safe. */
void cogmaster__aim_step(const struct cogmaster__aim_rect *rect, uint32_t *cx, uint32_t *cy,
                         int32_t dx, int32_t dy);

/* Writes the crosshair for a 1-based aim into pts, which has room for
   COGMASTER_AIM_CROSS_MAX points: the aim dot and four dots each way along x
   and y, those outside rect left out. Returns the count written; an aim of
   0 0 or outside rect gives 0. Nothing is written past the count. */
uint32_t cogmaster__aim_cross(const struct cogmaster__aim_rect *rect, uint32_t ax, uint32_t ay,
                              struct cogmaster__aim_pt *pts);

#endif /* COGMASTER_HOST_AIM_H */
