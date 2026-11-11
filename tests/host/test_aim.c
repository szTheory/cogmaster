/* The pointer-to-aim helper. The case is named on the command line. Each
   oracle is written here from the definition (a pointer position as a fraction
   of the picture, rounded down), not by calling the helper. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "aim.h"
#include "../check.h"

#define NRECT 5u

/* Master System 256 wide at 192, 224 and 240 lines; Game Gear window in the
   same field at its two vertical offsets. */
static const struct cogmaster__aim_rect rects[NRECT] = {
    {0u, 0u, 256u, 192u},   {0u, 0u, 256u, 224u},   {0u, 0u, 256u, 240u},
    {48u, 24u, 160u, 144u}, {48u, 40u, 160u, 144u},
};

/* floor((s + 32768) * size / 65536), 64-bit so it cannot share a wrap with the helper. */
static uint32_t col_of(int32_t s, uint32_t size)
{
    return (uint32_t)(((uint64_t)(uint32_t)(s + 32768) * size) >> 16);
}

/* The oracle with the crop offset forgotten. */
static uint32_t wrong_aim(int32_t s, uint32_t size)
{
    return col_of(s, size) + 1u;
}

static void map_axis(uint32_t size)
{
    uint32_t seen[257];
    uint32_t prev = 0;
    uint32_t bad = 0;
    struct cogmaster__aim_rect r = {0u, 0u, size, size};
    memset(seen, 0, sizeof seen);
    for (int32_t s = -32767; s <= 32767; s++) {
        uint32_t ax = 99u, ay = 99u;
        cogmaster__aim_from_screen(&r, s, s, 0, &ax, &ay);
        uint32_t col = ax - 1u;
        bad += (uint32_t)(ax < 1u || ax > size || ax != ay || ax != col_of(s, size) + 1u);
        bad += (uint32_t)(col < prev);
        if (col < size) {
            seen[col] = 1u;
        }
        prev = col;
    }
    uint32_t reached = 0;
    for (uint32_t c = 0; c < size; c++) {
        reached += seen[c];
    }
    CHECK_EQ_U64(bad, 0u);
    CHECK_EQ_U64(reached, size);
    uint32_t ax = 0u, ay = 0u;
    cogmaster__aim_from_screen(&r, 0, 0, 0, &ax, &ay);
    CHECK_EQ_U64(ax - 1u, size / 2u);
    cogmaster__aim_from_screen(&r, -32767, 32767, 0, &ax, &ay);
    CHECK_EQ_U64(ax - 1u, 0u);
    CHECK_EQ_U64(ay - 1u, size - 1u);
    cogmaster__aim_from_screen(&r, 32767, -32767, 0, &ax, &ay);
    CHECK_EQ_U64(ax - 1u, size - 1u);
    CHECK_EQ_U64(ay - 1u, 0u);
}

/* Exhaustive over -32767..32767 for every width the picture can have. */
static void mapping(void)
{
    static const uint32_t widths[4] = {160u, 224u, 240u, 256u};
    for (uint32_t i = 0; i < 4u; i++) {
        map_axis(widths[i]);
    }
    /* Every rectangle: every pointer value lands inside it, at the oracle's dot. */
    for (uint32_t k = 0; k < NRECT; k++) {
        uint32_t bad = 0;
        for (int32_t s = -32767; s <= 32767; s++) {
            uint32_t ax, ay;
            cogmaster__aim_from_screen(&rects[k], s, -s, 0, &ax, &ay);
            bad += (uint32_t)(ax != rects[k].x + col_of(s, rects[k].w) + 1u);
            bad += (uint32_t)(ay != rects[k].y + col_of(-s, rects[k].h) + 1u);
            bad += (uint32_t)(ax <= rects[k].x || ax > rects[k].x + rects[k].w);
            bad += (uint32_t)(ay <= rects[k].y || ay > rects[k].y + rects[k].h);
        }
        CHECK_EQ_U64(bad, 0u);
    }
}

/* A parked pointer is 0 0 and never an edge dot. */
static void offscreen(void)
{
    for (uint32_t k = 0; k < NRECT; k++) {
        uint32_t ax = 7u, ay = 7u;
        cogmaster__aim_from_screen(&rects[k], 0, 0, 1, &ax, &ay);
        CHECK(ax == 0u && ay == 0u);
        ax = ay = 7u;
        cogmaster__aim_from_screen(&rects[k], -32768, 0, 0, &ax, &ay);
        CHECK(ax == 0u && ay == 0u);
        ax = ay = 7u;
        cogmaster__aim_from_screen(&rects[k], 0, -32768, 0, &ax, &ay);
        CHECK(ax == 0u && ay == 0u);
        ax = ay = 7u;
        cogmaster__aim_from_screen(&rects[k], -32768, -32768, 0, &ax, &ay);
        CHECK(ax == 0u && ay == 0u);
        /* Outside the documented range is treated as parked too. */
        ax = ay = 7u;
        cogmaster__aim_from_screen(&rects[k], 32768, 0, 0, &ax, &ay);
        CHECK(ax == 0u && ay == 0u);
        ax = ay = 7u;
        cogmaster__aim_from_screen(&rects[k], 0, INT32_MIN, 0, &ax, &ay);
        CHECK(ax == 0u && ay == 0u);
    }
    struct cogmaster__aim_rect empty = {0u, 0u, 0u, 192u};
    uint32_t ax = 7u, ay = 7u;
    cogmaster__aim_from_screen(&empty, 0, 0, 0, &ax, &ay);
    CHECK(ax == 0u && ay == 0u);
    ax = ay = 7u;
    cogmaster__aim_from_screen(NULL, 0, 0, 0, &ax, &ay);
    CHECK(ax == 0u && ay == 0u);
    ay = 7u;
    cogmaster__aim_from_screen(&rects[0], 0, 0, 0, NULL, &ay);
    CHECK(ay == 7u);
}

/* Corners and centre of each rectangle, against hand-written 1-based dots. */
static void corners(void)
{
    struct {
        uint32_t k, ax, ay;
        int32_t sx, sy;
    } const t[] = {
        {0u, 1u, 1u, -32767, -32767},   {0u, 256u, 192u, 32767, 32767},
        {0u, 129u, 97u, 0, 0},          {1u, 256u, 224u, 32767, 32767},
        {1u, 129u, 113u, 0, 0},         {2u, 256u, 240u, 32767, 32767},
        {2u, 129u, 121u, 0, 0},         {3u, 49u, 25u, -32767, -32767},
        {3u, 208u, 168u, 32767, 32767}, {3u, 129u, 97u, 0, 0},
        {4u, 49u, 41u, -32767, -32767}, {4u, 208u, 184u, 32767, 32767},
        {4u, 129u, 113u, 0, 0},
    };
    for (uint32_t i = 0; i < sizeof t / sizeof t[0]; i++) {
        uint32_t ax, ay;
        cogmaster__aim_from_screen(&rects[t[i].k], t[i].sx, t[i].sy, 0, &ax, &ay);
        CHECK_EQ_U64(ax, t[i].ax);
        CHECK_EQ_U64(ay, t[i].ay);
    }
}

/* The crop offset must matter: an oracle that ignores x and y has to disagree
   on both Game Gear rectangles (at the centre and at both corners) and agree
   on the Master System ones, where the offset is zero. */
static void control(void)
{
    static const int32_t pts[3] = {-32767, 0, 32767};
    for (uint32_t k = 0; k < NRECT; k++) {
        uint32_t disagree = 0;
        for (uint32_t i = 0; i < 3u; i++) {
            uint32_t ax, ay;
            cogmaster__aim_from_screen(&rects[k], pts[i], pts[i], 0, &ax, &ay);
            disagree += (uint32_t)(ax != wrong_aim(pts[i], rects[k].w)) +
                        (uint32_t)(ay != wrong_aim(pts[i], rects[k].h));
        }
        if (rects[k].x == 0u && rects[k].y == 0u) {
            CHECK_EQ_U64(disagree, 0u);
        } else {
            CHECK_EQ_U64(disagree, 6u);
        }
    }
}

static int32_t clampi(int64_t v, int64_t lo, int64_t hi)
{
    return (int32_t)(v < lo ? lo : (v > hi ? hi : v));
}

static void step(void)
{
    static const int32_t deltas[] = {INT32_MIN, -100000, -257, -5,     -1,       0,
                                     1,         5,       257,  100000, INT32_MAX};
    for (uint32_t k = 0; k < NRECT; k++) {
        const struct cogmaster__aim_rect *r = &rects[k];
        uint32_t cx = COGMASTER_AIM_CURSOR_UNSET, cy = COGMASTER_AIM_CURSOR_UNSET;
        cogmaster__aim_step(r, &cx, &cy, 0, 0);
        CHECK_EQ_U64(cx, r->x + r->w / 2u + 1u);
        CHECK_EQ_U64(cy, r->y + r->h / 2u + 1u);
        /* Every delta pair from a set of starting points stays inside and matches a clamped add. */
        for (uint32_t a = 0; a < sizeof deltas / sizeof deltas[0]; a++) {
            for (uint32_t b = 0; b < sizeof deltas / sizeof deltas[0]; b++) {
                uint32_t sx[3] = {r->x + 1u, r->x + r->w, r->x + 5u};
                uint32_t sy[3] = {r->y + r->h, r->y + 5u, r->y + 1u};
                for (uint32_t s = 0; s < 3u; s++) {
                    uint32_t px = sx[s], py = sy[s];
                    cogmaster__aim_step(r, &px, &py, deltas[a], deltas[b]);
                    CHECK_EQ_U64(px, clampi((int64_t)sx[s] + deltas[a], (int64_t)r->x + 1,
                                            (int64_t)r->x + r->w));
                    CHECK_EQ_U64(py, clampi((int64_t)sy[s] + deltas[b], (int64_t)r->y + 1,
                                            (int64_t)r->y + r->h));
                }
            }
        }
        /* The extremes land on the edges. */
        cx = cy = COGMASTER_AIM_CURSOR_UNSET;
        cogmaster__aim_step(r, &cx, &cy, INT32_MIN, INT32_MIN);
        CHECK(cx == r->x + 1u && cy == r->y + 1u);
        cx = cy = COGMASTER_AIM_CURSOR_UNSET;
        cogmaster__aim_step(r, &cx, &cy, INT32_MAX, INT32_MAX);
        CHECK(cx == r->x + r->w && cy == r->y + r->h);
        /* A cursor left outside by a smaller picture comes back inside. */
        cx = 100000u;
        cy = 0u;
        cogmaster__aim_step(r, &cx, &cy, 0, 0);
        CHECK(cx == r->x + r->w && cy == r->y + 1u);
    }
    uint32_t cx = 5u, cy = 5u;
    cogmaster__aim_step(NULL, &cx, &cy, 1, 1);
    cogmaster__aim_step(&rects[0], NULL, &cy, 1, 1);
    CHECK(cx == 5u && cy == 5u);
}

/* The number of crosshair dots for a 1-based aim, counted from the shape. */
static uint32_t expect_count(const struct cogmaster__aim_rect *r, uint32_t ax, uint32_t ay)
{
    int64_t x = (int64_t)ax - 1, y = (int64_t)ay - 1;
    uint32_t n = 1u;
    for (int64_t k = 1; k <= 4; k++) {
        n += (uint32_t)(x + k < (int64_t)(r->x + r->w)) + (uint32_t)(x - k >= (int64_t)r->x);
        n += (uint32_t)(y + k < (int64_t)(r->y + r->h)) + (uint32_t)(y - k >= (int64_t)r->y);
    }
    return n;
}

static void check_cross(const struct cogmaster__aim_rect *r, uint32_t ax, uint32_t ay)
{
    struct cogmaster__aim_pt buf[COGMASTER_AIM_CROSS_MAX + 3u];
    unsigned char *raw = (unsigned char *)buf;
    uint32_t n;
    uint32_t dot_x = ax - 1u, dot_y = ay - 1u;
    memset(buf, 0xA5, sizeof buf);
    n = cogmaster__aim_cross(r, ax, ay, buf);
    CHECK_EQ_U64(n, expect_count(r, ax, ay));
    CHECK(n >= 1u && n <= COGMASTER_AIM_CROSS_MAX);
    CHECK(buf[0].x == dot_x && buf[0].y == dot_y);
    for (uint32_t i = 0; i < n && i < COGMASTER_AIM_CROSS_MAX; i++) {
        uint32_t dx = buf[i].x > dot_x ? buf[i].x - dot_x : dot_x - buf[i].x;
        uint32_t dy = buf[i].y > dot_y ? buf[i].y - dot_y : dot_y - buf[i].y;
        CHECK(buf[i].x >= r->x && buf[i].x < r->x + r->w);
        CHECK(buf[i].y >= r->y && buf[i].y < r->y + r->h);
        CHECK((dx == 0u && dy <= 4u) || (dy == 0u && dx <= 4u));
        for (uint32_t j = 0; j < i; j++) {
            CHECK(buf[j].x != buf[i].x || buf[j].y != buf[i].y);
        }
    }
    for (size_t i = n * sizeof buf[0]; i < sizeof buf; i++) {
        CHECK_EQ_HEX(raw[i], 0xA5u);
    }
}

static void cross(void)
{
    for (uint32_t k = 0; k < NRECT; k++) {
        const struct cogmaster__aim_rect *r = &rects[k];
        uint32_t x0 = r->x + 1u, x1 = r->x + r->w, xc = r->x + r->w / 2u + 1u;
        uint32_t y0 = r->y + 1u, y1 = r->y + r->h, yc = r->y + r->h / 2u + 1u;
        uint32_t xs[3] = {x0, xc, x1}, ys[3] = {y0, yc, y1};
        struct cogmaster__aim_pt buf[COGMASTER_AIM_CROSS_MAX];
        for (uint32_t i = 0; i < 3u; i++) {
            for (uint32_t j = 0; j < 3u; j++) {
                check_cross(r, xs[i], ys[j]);
            }
        }
        /* The centre is whole; a corner keeps 1 + 4 + 4 dots. */
        CHECK_EQ_U64(cogmaster__aim_cross(r, xc, yc, buf), COGMASTER_AIM_CROSS_MAX);
        CHECK_EQ_U64(cogmaster__aim_cross(r, x0, y0, buf), 9u);
        /* No aim, an aim outside the rectangle, and NULL give nothing. */
        memset(buf, 0xA5, sizeof buf);
        CHECK_EQ_U64(cogmaster__aim_cross(r, 0u, 0u, buf), 0u);
        CHECK_EQ_U64(cogmaster__aim_cross(r, xc, 0u, buf), 0u);
        CHECK_EQ_U64(cogmaster__aim_cross(r, x1 + 1u, yc, buf), 0u);
        CHECK_EQ_U64(cogmaster__aim_cross(r, xc, y1 + 1u, buf), 0u);
        CHECK_EQ_U64(cogmaster__aim_cross(r, 0xFFFFFFFFu, yc, buf), 0u);
        CHECK_EQ_U64(cogmaster__aim_cross(r, xc, yc, NULL), 0u);
        CHECK_EQ_HEX(((unsigned char *)buf)[0], 0xA5u);
        CHECK_EQ_HEX(((unsigned char *)buf)[sizeof buf - 1u], 0xA5u);
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "aim") == 0) {
        mapping();
        offscreen();
        corners();
        control();
        step();
        cross();
    } else {
        fprintf(stderr, "usage: cogmaster-test-aim aim\n");
        return 2;
    }
    CHECK_DONE();
}
