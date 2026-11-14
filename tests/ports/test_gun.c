/* The Light Phaser on the internal API: cogmaster-test-ports-gun <case>. The
   oracle below is a literal copy of the H-counter expression src/vdp.c held
   before it became a function of the tick. It is a copy on purpose: a
   refactor that moves the latch origin is the fault, so the expected value
   must not come from the code under test. The aim and pin cases carry their
   own written-again rules for the same reason: a field dot is 1 to 256 across
   and 1 to 240 down, anything else is the parked pair 0 0. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define LINE_TICKS 684u

static uint8_t image[IMAGE_SIZE];

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void load_into(cogmaster *m, uint32_t machine, uint32_t region, uint32_t tv)
{
    memset(image, 0x76, sizeof image);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    media.region = region;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
}

static cogmaster *load_tv(uint32_t machine, uint32_t region, uint32_t tv)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    load_into(m, machine, region, tv);
    return m;
}

static cogmaster *load(uint32_t machine, uint32_t region)
{
    return load_tv(machine, region, COGMASTER_TV_NTSC);
}

/* Copy of the pre-refactor counter, with the origin as a parameter so the
   control can move it by one. */
static uint8_t oracle_hcount(uint32_t tick, uint32_t origin)
{
    uint32_t t = (tick + origin) % LINE_TICKS;
    return (uint8_t)((t >= 592u ? t + 340u : t) >> 2);
}

/* The counter at every in-line tick equals the copy, and the latch holds the
   value of the tick it is given, not of the bus tick. */
static void test_latch_at(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    uint32_t ticks = 0u;
    uint32_t moved = 0u;
    for (uint32_t t = 0u; t < LINE_TICKS; t++) {
        uint8_t want = oracle_hcount(t, 32u);
        CHECK_EQ_HEX(cogmaster__vdp_hcount_at(m, t), want);
        /* The control: an origin one lower must disagree somewhere. */
        moved += oracle_hcount(t, 31u) != want ? 1u : 0u;
        m->bus.t = t;
        m->vdp.hheld = 0u;
        cogmaster__vdp_latch_h(m);
        CHECK_EQ_HEX(m->vdp.hcounter, want);
        CHECK_EQ_U64(m->vdp.hheld, 1u);
        ticks++;
    }
    CHECK_EQ_U64(ticks, LINE_TICKS);
    CHECK(moved > 0u);
    for (uint32_t k = 0u; k < 16u; k++) {
        uint32_t t1 = (k * 41u) % LINE_TICKS;
        uint32_t t2 = (k * 41u + 300u + k) % LINE_TICKS;
        m->bus.t = t1;
        m->vdp.hheld = 0u;
        cogmaster__vdp_latch_h_at(m, t2);
        CHECK_EQ_HEX(m->vdp.hcounter, oracle_hcount(t2, 32u));
        CHECK_EQ_U64(m->vdp.hheld, 1u);
    }
    cogmaster_destroy(m);
}

/* ---- the device: aim, pins and hostile input ---- */

#define PHASER COGMASTER_DEVICE_LIGHT_PHASER

static cogmaster_status send(cogmaster *m, uint32_t d1, uint32_t d2, uint32_t p1, uint32_t p2,
                             const int32_t axis[4])
{
    cogmaster_input in;
    memset(&in, 0, sizeof in);
    in.size = (uint32_t)sizeof in;
    in.port1 = d1;
    in.port2 = d2;
    in.p1 = p1;
    in.p2 = p2;
    in.x1 = axis[0];
    in.y1 = axis[1];
    in.x2 = axis[2];
    in.y2 = axis[3];
    return cogmaster_set_input(m, &in);
}

static void frame(cogmaster *m)
{
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
}

static void expect_pending(const cogmaster *m, uint32_t port, int32_t x, int32_t y)
{
    CHECK_EQ_U64(port == 0u ? m->pending_input.x1 : m->pending_input.x2, x);
    CHECK_EQ_U64(port == 0u ? m->pending_input.y1 : m->pending_input.y2, y);
}

static void expect_active(const cogmaster *m, uint32_t port, int32_t x, int32_t y)
{
    CHECK_EQ_U64(port == 0u ? m->input.x1 : m->input.x2, x);
    CHECK_EQ_U64(port == 0u ? m->input.y1 : m->input.y2, y);
}

/* The aim is absolute and the last call wins, it survives frames, a device change restarts it, a
   soft reset keeps it and a load parks it. */
static void test_aim(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    const int32_t first[4] = {10, 20, 0, 0};
    const int32_t second[4] = {11, 21, 0, 0};
    const int32_t both[4] = {11, 21, 256, 240};
    const int32_t none[4] = {0, 0, 0, 0};
    /* A fresh instance is parked. */
    expect_pending(m, 0u, 0, 0);
    expect_active(m, 0u, 0, 0);
    CHECK_EQ_U64(send(m, PHASER, COGMASTER_DEVICE_PAD, 0u, 0u, first), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 10, 20);
    CHECK_EQ_U64(send(m, PHASER, COGMASTER_DEVICE_PAD, 0u, 0u, second), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 11, 21); /* replaced, not summed to 21 and 41 */
    frame(m);
    expect_active(m, 0u, 11, 21);
    for (uint32_t i = 0u; i < 3u; i++) {
        frame(m); /* no call: the aim in force stays */
        expect_active(m, 0u, 11, 21);
        expect_pending(m, 0u, 11, 21);
    }
    /* One call sets both ports. */
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0u, 0u, both), COGMASTER_STATUS_OK);
    frame(m);
    expect_active(m, 0u, 11, 21);
    expect_active(m, 1u, 256, 240);
    /* A call with the phaser selected and no axes parks the aim, as a caller of the shorter
       structs and a script line without axes send. */
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0u, 0u, none), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 0, 0);
    expect_pending(m, 1u, 0, 0);
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0u, 0u, both), COGMASTER_STATUS_OK);
    frame(m);
    /* A frame leaves the gun's block alone while the device stays. */
    m->ports[0].g_ax = 11u;
    m->ports[0].g_ay = 21u;
    frame(m);
    CHECK_EQ_U64(m->ports[0].g_ax, 11u);
    CHECK_EQ_U64(m->ports[0].g_ay, 21u);
    /* A device change on port 1 restarts its axes and its block; port 2 keeps its aim. */
    const int32_t moved[4] = {5, 6, 256, 240};
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, PHASER, 0u, 0u, moved), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 5, 6);
    expect_pending(m, 1u, 256, 240);
    frame(m);
    CHECK_EQ_U64(m->ports[0].g_ax, 0u);
    CHECK_EQ_U64(m->ports[0].g_ay, 0u);
    expect_pending(m, 0u, 0, 0); /* the Sports Pad's motion was taken */
    expect_active(m, 1u, 256, 240);
    expect_pending(m, 1u, 256, 240);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PAD, PHASER, 0u, 0u, moved), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 0, 0); /* a pad takes no axes */
    frame(m);
    expect_active(m, 0u, 0, 0);
    expect_active(m, 1u, 256, 240);
    /* Back to the phaser from another device starts at the call's own aim. */
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0u, 0u, both), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 11, 21);
    /* A soft reset keeps the pending aim, as it keeps a Sports Pad's motion. */
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    expect_pending(m, 0u, 11, 21);
    expect_pending(m, 1u, 256, 240);
    frame(m);
    expect_active(m, 1u, 256, 240);
    /* A load parks it. */
    load_into(m, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
    expect_pending(m, 0u, 0, 0);
    expect_pending(m, 1u, 0, 0);
    expect_active(m, 0u, 0, 0);
    expect_active(m, 1u, 0, 0);
    cogmaster_destroy(m);
}

/* The phaser's pins: TL is the trigger, the rest read high, and nothing else the host or the
   program sends reaches them. The meaning of a held bit is the pad's, so a phaser at rest reads
   exactly as a pad with nothing pressed. */
static const uint8_t ctrl_writes[8] = {0x00u, 0x05u, 0x0Au, 0x0Fu, 0x55u, 0xAAu, 0xF0u, 0xFFu};

static void pins_profile(uint32_t machine, uint32_t region, uint32_t tv)
{
    cogmaster *m = load_tv(machine, region, tv);
    cogmaster *pad = load_tv(machine, region, tv);
    const int32_t aim[4] = {100, 100, 100, 100};
    const int32_t none[4] = {0, 0, 0, 0};
    for (uint32_t held = 0u; held < 64u; held++) {
        CHECK_EQ_U64(send(m, PHASER, PHASER, held, held, aim), COGMASTER_STATUS_OK);
        frame(m);
        for (uint32_t port = 0u; port < 2u; port++) {
            uint32_t want = 0x2Fu | (~held & 0x10u);
            CHECK_EQ_HEX(cogmaster__ports_pins(m, port, 0u), want);
            for (uint32_t c = 0u; c < 8u; c++) {
                cogmaster__io_out(m, 0x3Fu, ctrl_writes[c]);
                CHECK_EQ_HEX(cogmaster__ports_pins(m, port, 12345u + c), want);
            }
        }
    }
    /* At rest the gun and a pad with nothing pressed read alike on both ports. */
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0u, 0u, aim), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(send(pad, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, none),
                 COGMASTER_STATUS_OK);
    frame(m);
    frame(pad);
    CHECK_EQ_HEX(cogmaster__ports_pins(m, 0u, 0u), 0x3Fu);
    for (uint32_t port = 0u; port < 2u; port++) {
        CHECK_EQ_HEX(cogmaster__ports_pins(m, port, 0u), cogmaster__ports_pins(pad, port, 0u));
    }
    for (uint32_t c = 0u; c < 8u; c++) {
        cogmaster__io_out(m, 0x3Fu, ctrl_writes[c]);
        cogmaster__io_out(pad, 0x3Fu, ctrl_writes[c]);
        CHECK_EQ_HEX(cogmaster__io_in(m, 0xDCu), cogmaster__io_in(pad, 0xDCu));
        CHECK_EQ_HEX(cogmaster__io_in(m, 0xDDu), cogmaster__io_in(pad, 0xDDu));
    }
    /* The trigger on either port is visible on its own line of $DC and $DD and no other. */
    cogmaster__io_out(m, 0x3Fu, 0xFFu);
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0x10u, 0u, aim), COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_HEX(cogmaster__io_in(m, 0xDCu), 0xEFu);
    CHECK_EQ_HEX(cogmaster__io_in(m, 0xDDu) & 0x0Fu, 0x0Fu);
    CHECK_EQ_U64(send(m, PHASER, PHASER, 0u, 0x10u, aim), COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_HEX(cogmaster__io_in(m, 0xDCu), 0xFFu);
    CHECK_EQ_HEX(cogmaster__io_in(m, 0xDDu) & 0x0Fu, 0x0Bu);
    cogmaster_destroy(pad);
    cogmaster_destroy(m);
}

static void test_pins(void)
{
    pins_profile(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
    pins_profile(COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
    pins_profile(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC);
    pins_profile(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL);
    /* The Game Gear refuses the gun on either port, whole, and keeps what it held. */
    cogmaster *gg = load(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT);
    const int32_t aim[4] = {10, 20, 30, 40};
    const int32_t ok[4] = {0, 0, 0, 0};
    CHECK_EQ_U64(send(gg, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_NONE, 5u, 6u, ok),
                 COGMASTER_STATUS_OK);
    cogmaster_input before = gg->pending_input;
    CHECK_EQ_U64(send(gg, PHASER, COGMASTER_DEVICE_NONE, 1u, 2u, aim),
                 COGMASTER_STATUS_ERR_PROFILE_REJECTED);
    CHECK_EQ_U64(send(gg, COGMASTER_DEVICE_PAD, PHASER, 1u, 2u, aim),
                 COGMASTER_STATUS_ERR_PROFILE_REJECTED);
    CHECK_EQ_U64(send(gg, PHASER, PHASER, 1u, 2u, aim), COGMASTER_STATUS_ERR_PROFILE_REJECTED);
    CHECK(memcmp(&before, &gg->pending_input, sizeof before) == 0);
    cogmaster_destroy(gg);
}

/* The oracle for a stored aim, written again here: a field dot is 1 to 256 across and 1 to 240
   down, and anything else is the parked pair. */
static void canon(int32_t x, int32_t y, int32_t out[2])
{
    int on = x >= 1 && x <= 256 && y >= 1 && y <= 240;
    out[0] = on ? x : 0;
    out[1] = on ? y : 0;
}

static const int32_t axis_values[10] = {0, 1, -1, 255, 256, 257, 240, 241, 32767, -32767};

static const struct {
    uint32_t machine, region, tv;
} profiles[4] = {
    {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL},
};

/* Which of the two ports hold a phaser in each of the three variants. */
static const uint32_t variant_devices[3][2] = {
    {PHASER, PHASER}, {PHASER, COGMASTER_DEVICE_PAD}, {COGMASTER_DEVICE_PAD, PHASER}};

/* Every axis value at every position of the aim on both ports and four profiles, three calls a
   frame: each call leaves the aim the oracle says, the last one wins, a value outside the
   call's range is refused with nothing changed, and a frame keeps what the last call left. */
static void test_hostile_aim(void)
{
    static const int32_t refused[6] = {32768, -32768, INT32_MAX, INT32_MIN, 65536, -65536};
    for (uint32_t pr = 0u; pr < 4u; pr++) {
        cogmaster *m = load_tv(profiles[pr].machine, profiles[pr].region, profiles[pr].tv);
        for (uint32_t v = 0u; v < 3u; v++) {
            const uint32_t d1 = variant_devices[v][0];
            const uint32_t d2 = variant_devices[v][1];
            for (uint32_t i = 0u; i < 10u; i++) {
                for (uint32_t j = 0u; j < 10u; j++) {
                    int32_t last[4] = {0, 0, 0, 0};
                    for (uint32_t call = 0u; call < 3u; call++) {
                        /* Two calls with other values, then the pair under test. */
                        int32_t a = axis_values[(i + 3u * (2u - call)) % 10u];
                        int32_t b = axis_values[(j + 7u * (2u - call)) % 10u];
                        int32_t axis[4] = {a, b, b, a};
                        CHECK_EQ_U64(send(m, d1, d2, 0u, 0u, axis), COGMASTER_STATUS_OK);
                        int32_t want[2];
                        canon(a, b, want);
                        if (d1 == PHASER) {
                            expect_pending(m, 0u, want[0], want[1]);
                        } else {
                            expect_pending(m, 0u, 0, 0);
                        }
                        canon(b, a, want);
                        if (d2 == PHASER) {
                            expect_pending(m, 1u, want[0], want[1]);
                        } else {
                            expect_pending(m, 1u, 0, 0);
                        }
                        memcpy(last, axis, sizeof last);
                    }
                    frame(m);
                    int32_t want[2];
                    canon(last[0], last[1], want);
                    expect_active(m, 0u, d1 == PHASER ? want[0] : 0, d1 == PHASER ? want[1] : 0);
                    canon(last[2], last[3], want);
                    expect_active(m, 1u, d2 == PHASER ? want[0] : 0, d2 == PHASER ? want[1] : 0);
                    /* Nothing the sensor squares can overflow: a stored aim is within 256. */
                    CHECK(m->input.x1 >= 0 && m->input.x1 <= 256 && m->input.y1 >= 0 &&
                          m->input.y1 <= 240);
                    CHECK(m->input.x2 >= 0 && m->input.x2 <= 256 && m->input.y2 >= 0 &&
                          m->input.y2 <= 240);
                }
            }
            /* A value outside the call's range is the one error and changes nothing. */
            for (uint32_t r = 0u; r < 6u; r++) {
                for (uint32_t axis_at = 0u; axis_at < 4u; axis_at++) {
                    const int32_t good[4] = {20, 30, 40, 50};
                    CHECK_EQ_U64(send(m, d1, d2, 3u, 4u, good), COGMASTER_STATUS_OK);
                    cogmaster_input pend = m->pending_input;
                    cogmaster_input act = m->input;
                    uint8_t blocks[sizeof m->ports];
                    memcpy(blocks, m->ports, sizeof blocks);
                    int32_t bad[4] = {20, 30, 40, 50};
                    bad[axis_at] = refused[r];
                    CHECK_EQ_U64(send(m, d1, d2, 9u, 9u, bad), COGMASTER_STATUS_ERR_ARGUMENT);
                    CHECK(memcmp(&pend, &m->pending_input, sizeof pend) == 0);
                    CHECK(memcmp(&act, &m->input, sizeof act) == 0);
                    CHECK(memcmp(blocks, m->ports, sizeof blocks) == 0);
                }
            }
        }
        cogmaster_destroy(m);
    }
}

/* ---- the sensor: pictures, a beam walker and an independent oracle ---- */

/* The picture: a black field with square tiles of a few colours, drawn by writing the VDP's own
   memories. Tile t is solid in colour index t, and the colours are named by their three 2-bit
   levels so the test's oracle can judge them without the core's code. */
#define NTILE_COLOURS 4u
static const uint8_t colour_levels[NTILE_COLOURS][3] = {
    {0u, 0u, 0u}, {3u, 3u, 3u}, {2u, 2u, 2u}, {1u, 1u, 1u}};

#define SQUARE_SIZE 16u
static const struct {
    uint32_t x, y, colour;
} squares[] = {
    {32u, 24u, 1u}, {120u, 96u, 2u}, {232u, 176u, 1u}, {72u, 56u, 3u}, {160u, 208u, 1u},
};
#define NSQUARES (sizeof squares / sizeof squares[0])

/* The oracle's own judgement of brightness, from 2-bit levels. A level n is the 4-bit value 5n, and
   its 8-bit expansion is 17 x 5n = 85n; the weights and the threshold are the named model values.
 */
static uint32_t oracle_luma(uint32_t r, uint32_t g, uint32_t b)
{
    return (GUN_WEIGHT_R * 85u * r + GUN_WEIGHT_G * 85u * g + GUN_WEIGHT_B * 85u * b) >> 8;
}

static int oracle_lit(uint32_t r, uint32_t g, uint32_t b)
{
    return oracle_luma(r, g, b) >= GUN_LUMA;
}

/* The colour index of the picture's dot, by the squares and nothing else. */
static uint32_t picture_colour(uint32_t x, uint32_t y)
{
    for (size_t i = 0; i < NSQUARES; i++) {
        if (x >= squares[i].x && x < squares[i].x + SQUARE_SIZE && y >= squares[i].y &&
            y < squares[i].y + SQUARE_SIZE) {
            return squares[i].colour;
        }
    }
    return 0u;
}

static int picture_lit(uint32_t x, uint32_t y)
{
    const uint8_t *l = colour_levels[picture_colour(x, y)];
    return oracle_lit(l[0], l[1], l[2]);
}

static uint32_t cram_byte(uint32_t colour)
{
    const uint8_t *l = colour_levels[colour];
    return (uint32_t)l[0] | ((uint32_t)l[1] << 2) | ((uint32_t)l[2] << 4);
}

/* The mode-4 name table for a picture height, and the video mode registers that give it. */
static uint32_t name_base(uint32_t height)
{
    return height == 192u ? 0x3800u : 0x3700u;
}

/* The video mode registers of a picture height, as the picture helper writes them. */
static void height_mode(cogmaster *m, uint32_t height)
{
    m->vdp.reg[0] = height == 192u ? 0x04u : 0x06u;
    m->vdp.reg[1] = (uint8_t)(0x40u | (height == 224u ? 0x10u : height == 240u ? 0x08u : 0u));
}

static void draw(cogmaster *m, uint32_t height)
{
    struct cogmaster__vdp *v = &m->vdp;
    height_mode(m, height);
    v->reg[2] = 0x0Eu;
    v->reg[5] = 0x7Eu;
    v->reg[7] = 0u;
    for (uint32_t c = 1u; c < NTILE_COLOURS; c++) {
        v->cram[c] = (uint8_t)cram_byte(c);
        for (uint32_t row = 0u; row < 8u; row++) {
            for (uint32_t plane = 0u; plane < 4u; plane++) {
                v->vram[c * 32u + row * 4u + plane] = (c & (1u << plane)) != 0u ? 0xFFu : 0u;
            }
        }
    }
    for (uint32_t i = 0u; i < 64u; i++) {
        v->vram[0x3F00u + i] = 0xD0u;
    }
    for (size_t i = 0; i < NSQUARES; i++) {
        for (uint32_t ty = 0u; ty < SQUARE_SIZE / 8u; ty++) {
            for (uint32_t tx = 0u; tx < SQUARE_SIZE / 8u; tx++) {
                uint32_t col = squares[i].x / 8u + tx;
                uint32_t row = squares[i].y / 8u + ty;
                v->vram[name_base(height) + (row * 32u + col) * 2u] = (uint8_t)squares[i].colour;
            }
        }
    }
}

static cogmaster *picture(uint32_t height, uint32_t region)
{
    cogmaster *m = load_tv(COGMASTER_MACHINE_SMS2, region, COGMASTER_TV_NTSC);
    draw(m, height);
    return m;
}

/* Aims the gun on one port, or both, and runs a frame so the aim is in force and the picture that
   follows the next wrap is scanned. */
static void aim_ports(cogmaster *m, uint32_t d1, int32_t x1, int32_t y1, uint32_t d2, int32_t x2,
                      int32_t y2)
{
    const int32_t axis[4] = {x1, y1, x2, y2};
    CHECK_EQ_U64(send(m, d1, d2, 0u, 0u, axis), COGMASTER_STATUS_OK);
    frame(m);
}

static void aim_one(cogmaster *m, uint32_t port, int32_t ax, int32_t ay)
{
    if (port == 0u) {
        aim_ports(m, PHASER, ax, ay, COGMASTER_DEVICE_PAD, 0, 0);
    } else {
        aim_ports(m, COGMASTER_DEVICE_PAD, 0, 0, PHASER, ax, ay);
    }
}

/* The beam, moved forward to (line, t) without running the CPU. */
static void walk_to(cogmaster *m, uint32_t line, uint32_t t)
{
    uint32_t total = cogmaster__frame_lines(m) * LINE_TICKS;
    uint32_t want = line * LINE_TICKS + t;
    uint32_t cur = m->bus.line * LINE_TICKS + m->bus.t;
    m->ticks += (want + total - cur) % total;
    cogmaster__bus_sync(m);
}

/* One tick past the wrap of the picture the gun armed for, where the next picture has not begun. */
static void walk_past_wrap(cogmaster *m)
{
    walk_to(m, cogmaster__frame_lines(m) - 1u, COGMASTER__VDP_WRAP_TICK + 1u);
}

/* $DD b6 or b7 of the port is low: the sensor's level, read twice so the pulse bit the first read
   takes is gone. */
static uint32_t th_low(cogmaster *m, uint32_t port)
{
    (void)cogmaster__io_in(m, 0xDDu);
    return (cogmaster__io_in(m, 0xDDu) & (0x40u << port)) == 0u ? 1u : 0u;
}

/* The runs of lit dots of one row for an aim, from a plain distance test per dot (not the core's
   chord arithmetic), as [on, off) in dots. */
#define MAX_RUNS 8u
struct runs {
    uint32_t n, on[MAX_RUNS], off[MAX_RUNS];
};

static void oracle_runs(uint32_t ax, uint32_t ay, uint32_t y, struct runs *out)
{
    int64_t dy = (int64_t)y - ((int64_t)ay - 1);
    int in = 0;
    out->n = 0u;
    for (uint32_t x = 0u; x <= COGMASTER__FIELD_W; x++) {
        int64_t dx = (int64_t)x - ((int64_t)ax - 1);
        int on = x < COGMASTER__FIELD_W && dx * dx + dy * dy <= (int64_t)GUN_RADIUS * GUN_RADIUS &&
                 picture_lit(x, y);
        if (on && !in) {
            out->on[out->n] = x;
        }
        if (!on && in) {
            out->off[out->n++] = x;
        }
        in = on;
    }
}

/* The sensor's level at an in-line tick: low from the tick it sees the first lit dot of a run until
   the tick it sees the first dot after it, each dot being seen a fixed delay after its own tick. */
static uint32_t oracle_low(const struct runs *r, uint32_t t, uint32_t delay)
{
    for (uint32_t i = 0u; i < r->n; i++) {
        if (2u * r->on[i] + delay <= t && t < 2u * r->off[i] + delay) {
            return 1u;
        }
    }
    return 0u;
}

static uint8_t oracle_h(uint32_t tick)
{
    return oracle_hcount(tick, 32u);
}

static int near_aim(uint32_t y, uint32_t ay)
{
    uint32_t aim_row = ay - 1u;
    return (y > aim_row ? y - aim_row : aim_row - y) <= GUN_RADIUS;
}

/* The first and the last aims and rows of a grid, per picture height. */
static const uint32_t model_heights[3] = {192u, 224u, 240u};
static const uint32_t model_ay[3][6] = {
    {1u, 30u, 100u, 150u, 190u, 192u},
    {1u, 30u, 100u, 150u, 215u, 224u},
    {1u, 30u, 100u, 150u, 225u, 240u},
};

/* One aim on one port: the level at every tick of every row the disc meets, the level at the end
   of every other row, and the latched H counter after every row against the oracle's, for a sensor
   delay the caller names. Returns how many comparisons disagreed. */
static uint32_t model_aim(uint32_t height, uint32_t region, uint32_t port, uint32_t ax, uint32_t ay,
                          uint32_t delay)
{
    uint32_t bad = 0u;
    cogmaster *m = picture(height, region);
    aim_one(m, port, (int32_t)ax, (int32_t)ay);
    walk_past_wrap(m);
    m->vdp.hheld = 0u;
    m->vdp.hcounter = 0u;
    uint32_t held = 0u;
    uint8_t expect_h = 0u;
    for (uint32_t y = 0u; y < height; y++) {
        struct runs r;
        oracle_runs(ax, ay, y, &r);
        if (near_aim(y, ay)) {
            for (uint32_t t = 0u; t < LINE_TICKS; t++) {
                walk_to(m, y, t);
                bad += th_low(m, port) != oracle_low(&r, t, delay) ? 1u : 0u;
            }
        } else {
            walk_to(m, y, LINE_TICKS - 1u);
            bad += th_low(m, port) != 0u ? 1u : 0u;
        }
        if (r.n != 0u) {
            held = 1u;
            expect_h = oracle_h(2u * r.off[r.n - 1u] + delay);
        }
        (void)cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT);
        bad += m->vdp.hheld != held ? 1u : 0u;
        bad += held != 0u && m->vdp.hcounter != expect_h ? 1u : 0u;
    }
    walk_to(m, height + 1u, 0u);
    (void)cogmaster__io_in(m, 0xDDu);
    bad += m->ports[port].g_scan != 0u ? 1u : 0u;
    bad += (m->ports[port].g_sense & 1u) != 0u ? 1u : 0u;
    cogmaster_destroy(m);
    return bad;
}

static void test_model(void)
{
    uint32_t pictures = 0u;
    for (uint32_t hi = 0u; hi < 3u; hi++) {
        for (uint32_t k = 0u; k < 16u; k++) {
            for (uint32_t j = 0u; j < 6u; j++) {
                uint32_t ax = 1u + 17u * k;
                /* Export on both ports; the Japanese console on every fourth aim, the other port.
                 */
                CHECK_EQ_U64(model_aim(model_heights[hi], COGMASTER_REGION_EXPORT, (k + j) & 1u, ax,
                                       model_ay[hi][j], GUN_DELAY_TICKS),
                             0u);
                pictures++;
                if (((k + j) & 3u) == 0u) {
                    CHECK_EQ_U64(model_aim(model_heights[hi], COGMASTER_REGION_JAPAN,
                                           (k + j + 1u) & 1u, ax, model_ay[hi][j], GUN_DELAY_TICKS),
                                 0u);
                    pictures++;
                }
            }
        }
    }
    CHECK(pictures > 300u);
}

/* The release edge of a square's vertical right edge: whatever tick the first read after it comes
   at, $7F is the counter at the edge's own tick, constant for a whole line, and the next row's
   identical edge comes exactly one line later. A latch taken at the read would be the counter at
   that first read instead. */
static void edge_after(uint32_t first_read)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    uint32_t x_off = squares[0].x + SQUARE_SIZE; /* the first dot past the right edge */
    uint32_t ax = x_off; /* the aim is one-based: the dot of the right edge is x_off - 1 */
    uint32_t ay = squares[0].y + SQUARE_SIZE / 2u + 1u;
    aim_one(m, 0u, (int32_t)ax, (int32_t)ay);
    walk_past_wrap(m);
    m->vdp.hheld = 0u;
    uint32_t first_row = ay - 1u - GUN_RADIUS;
    uint32_t release = first_row * LINE_TICKS + 2u * x_off + GUN_DELAY_TICKS;
    /* The closed form of the plan: a tick of 2 x_off + delay, 32 ticks of origin, 4 ticks a count.
     */
    uint8_t want = (uint8_t)((2u * x_off + GUN_DELAY_TICKS + 32u) >> 2);
    CHECK_EQ_HEX(want, oracle_h(2u * x_off + GUN_DELAY_TICKS));
    walk_to(m, (release - 1u) / LINE_TICKS, (release - 1u) % LINE_TICKS);
    (void)cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT);
    CHECK_EQ_U64(m->vdp.hheld, 0u);
    for (uint32_t k = first_read; k < LINE_TICKS; k++) {
        uint32_t at = release + k;
        walk_to(m, at / LINE_TICKS, at % LINE_TICKS);
        CHECK_EQ_HEX(cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT), want);
    }
    /* A line later the next row releases at the same dot and the value is the same. */
    uint32_t next = release + LINE_TICKS;
    walk_to(m, next / LINE_TICKS, next % LINE_TICKS);
    CHECK_EQ_HEX(cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT), want);
    cogmaster_destroy(m);
}

static void test_edge(void)
{
    /* The first read right at the edge, one tick after it, and at the distances of a short poll
     * loop. */
    static const uint32_t first_reads[4] = {0u, 1u, 16u, 66u};
    for (size_t i = 0; i < sizeof first_reads / sizeof first_reads[0]; i++) {
        edge_after(first_reads[i]);
    }
}

/* Every Master System colour against the oracle's judgement, through a lit square: the sensor
   sees light exactly for the bright ones, and the colours either side of the threshold are named.
 */
static void test_luma(void)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    aim_one(m, 0u, (int32_t)(squares[0].x + SQUARE_SIZE / 2u + 1u),
            (int32_t)(squares[0].y + SQUARE_SIZE / 2u + 1u));
    uint32_t lit_count = 0u;
    for (uint32_t c = 0u; c < 64u; c++) {
        uint32_t r = c & 3u;
        uint32_t g = (c >> 2) & 3u;
        uint32_t b = (c >> 4) & 3u;
        m->vdp.cram[1] = (uint8_t)c;
        uint32_t before = m->gun_stats.edges;
        frame(m);
        uint32_t sensed = m->gun_stats.edges != before ? 1u : 0u;
        CHECK_EQ_U64(sensed, oracle_lit(r, g, b) ? 1u : 0u);
        lit_count += sensed;
    }
    CHECK_EQ_U64(lit_count, 32u);
    /* The four colours nearest the threshold, as (red, green, blue) levels. */
    CHECK_EQ_U64(oracle_luma(1u, 2u, 0u), 125u);
    CHECK(!oracle_lit(1u, 2u, 0u));
    CHECK_EQ_U64(oracle_luma(3u, 1u, 0u), 126u);
    CHECK(!oracle_lit(3u, 1u, 0u));
    CHECK_EQ_U64(oracle_luma(0u, 2u, 3u), 128u);
    CHECK(oracle_lit(0u, 2u, 3u));
    CHECK_EQ_U64(oracle_luma(2u, 1u, 3u), 129u);
    CHECK(oracle_lit(2u, 1u, 3u));
    CHECK(oracle_lit(3u, 3u, 3u));
    CHECK(!oracle_lit(0u, 0u, 0u));
    cogmaster_destroy(m);
}

/* ---- a polling program's view: controls that must disagree with the model ---- */

/* The aims the controls use: on a square of the picture, so there is light to see. */
static const uint32_t control_aims[3][2] = {{40u, 33u}, {129u, 105u}, {240u, 185u}};
#define POLL_PERIOD 75u /* a poll loop of 25 Z80 cycles, the shortest a program has */

struct polled {
    uint32_t low;        /* polls that read TH low */
    uint32_t held;       /* the H counter is latched at the end */
    uint8_t h;           /* and its value */
    uint32_t rel_abs;    /* the tick the oracle's last release happens at, in ticks from line 0 */
    uint32_t on_abs;     /* and the first lit dot of that run */
    uint32_t level_low;  /* polls an oracle without the pulse bit reads low */
    uint32_t sticky_low; /* polls the oracle with the pulse bit reads low */
};

/* A program that reads $DD every POLL_PERIOD ticks through one picture, against the oracle with
   and without the pulse bit. */
static void poll_picture(uint32_t ax, uint32_t ay, struct polled *out)
{
    const uint32_t height = 192u;
    cogmaster *m = picture(height, COGMASTER_REGION_EXPORT);
    memset(out, 0, sizeof *out);
    aim_one(m, 0u, (int32_t)ax, (int32_t)ay);
    walk_past_wrap(m);
    m->vdp.hheld = 0u;
    uint32_t prev = 0u;
    for (uint32_t at = POLL_PERIOD; at < height * LINE_TICKS; at += POLL_PERIOD) {
        walk_to(m, at / LINE_TICKS, at % LINE_TICKS);
        out->low += (cogmaster__io_in(m, 0xDDu) & 0x40u) == 0u ? 1u : 0u;
        uint32_t row = at / LINE_TICKS;
        struct runs r;
        oracle_runs(ax, ay, row, &r);
        uint32_t level = oracle_low(&r, at % LINE_TICKS, GUN_DELAY_TICKS);
        out->level_low += level;
        /* A fall since the last poll, at any row, sets the pulse bit. */
        uint32_t fell = 0u;
        for (uint32_t y = prev / LINE_TICKS; y <= row; y++) {
            struct runs q;
            oracle_runs(ax, ay, y, &q);
            for (uint32_t i = 0u; i < q.n; i++) {
                uint32_t on = y * LINE_TICKS + 2u * q.on[i] + GUN_DELAY_TICKS;
                fell |= on > prev && on <= at ? 1u : 0u;
            }
        }
        out->sticky_low += (level | fell) != 0u ? 1u : 0u;
        prev = at;
    }
    for (uint32_t y = 0u; y < height; y++) {
        struct runs r;
        oracle_runs(ax, ay, y, &r);
        if (r.n != 0u) {
            out->rel_abs = y * LINE_TICKS + 2u * r.off[r.n - 1u] + GUN_DELAY_TICKS;
            out->on_abs = y * LINE_TICKS + 2u * r.on[r.n - 1u] + GUN_DELAY_TICKS;
        }
    }
    walk_to(m, height + 1u, 0u);
    out->held = m->vdp.hheld;
    out->h = (uint8_t)cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT);
    cogmaster_destroy(m);
}

/* The model with a sensor that is late, one that latches at a read, one that latches at the leading
   edge and one with no pulse bit each disagree with the core somewhere on the aims above. */
static void test_control(void)
{
    uint32_t late = 0u;
    uint32_t read_time = 0u;
    uint32_t leading = 0u;
    uint32_t no_pulse_bit = 0u;
    for (uint32_t i = 0u; i < 3u; i++) {
        uint32_t ax = control_aims[i][0];
        uint32_t ay = control_aims[i][1];
        CHECK_EQ_U64(model_aim(192u, COGMASTER_REGION_EXPORT, 0u, ax, ay, GUN_DELAY_TICKS), 0u);
        late += model_aim(192u, COGMASTER_REGION_EXPORT, 0u, ax, ay, GUN_DELAY_TICKS + 16u) != 0u
                    ? 1u
                    : 0u;
        struct polled p;
        poll_picture(ax, ay, &p);
        /* The core agrees with the oracle that has the pulse bit and the right edge. */
        CHECK_EQ_U64(p.held, 1u);
        CHECK_EQ_HEX(p.h, oracle_h(p.rel_abs % LINE_TICKS));
        CHECK_EQ_U64(p.low, p.sticky_low);
        /* A latch at the first poll after the release is another value. */
        uint32_t poll_at = (p.rel_abs / POLL_PERIOD + 1u) * POLL_PERIOD;
        read_time += oracle_h(poll_at % LINE_TICKS) != p.h ? 1u : 0u;
        leading += oracle_h(p.on_abs % LINE_TICKS) != p.h ? 1u : 0u;
        no_pulse_bit += p.level_low != p.low ? 1u : 0u;
    }
    CHECK(late > 0u);
    CHECK(read_time > 0u);
    CHECK(leading > 0u);
    CHECK(no_pulse_bit > 0u);
}

/* ---- neutrality: the same picture with and without the gun ---- */

static void vdp_control(cogmaster *m, uint8_t first, uint8_t second)
{
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, first);
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, second);
}

/* A write of CRAM entry 2 to black at the tick a poll lands on, in the middle of a lit square: the
   dot at that tick takes the new colour, with the gun or without. */
static cogmaster *order_run(int gun)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    uint32_t write_dot = squares[1].x + 8u; /* inside the second square */
    uint32_t row = squares[1].y + 4u;
    if (gun) {
        /* The aim is 19 dots left of the write, so its chord has dots the sensor sees by then. */
        aim_one(m, 0u, (int32_t)(write_dot - 16u + 1u), (int32_t)(row + 1u));
    } else {
        aim_ports(m, COGMASTER_DEVICE_PAD, 0, 0, COGMASTER_DEVICE_PAD, 0, 0);
    }
    walk_past_wrap(m);
    walk_to(m, row, 2u * write_dot);
    if (gun) {
        (void)cogmaster__io_in(m, 0xDDu);
    }
    vdp_control(m, 2u, 0xC0u);
    cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 0u);
    walk_to(m, 192u + 1u, 0u);
    return m;
}

static void test_order(void)
{
    cogmaster *g = order_run(1);
    cogmaster *p = order_run(0);
    uint32_t row = squares[1].y + 4u;
    uint32_t dot = squares[1].x + 8u;
    CHECK(memcmp(g->field, p->field, sizeof g->field) == 0);
    /* The write is not vacuous: the square's colour before the write dot, black from it on. */
    CHECK(g->field[row][dot - 1u] != 0u);
    CHECK_EQ_HEX(g->field[row][dot], 0u);
    CHECK_EQ_HEX(g->completed.pixels[row][dot], 0u);
    cogmaster_destroy(g);
    cogmaster_destroy(p);
}

/* A video mode the renderer rejects, with the display on: the first fault is recorded where the
   renderer's own schedule puts it, with a gun aimed and polled and without. Text is mode select
   nibble 1: R0 bits 1 and 2 clear and R1 bit 4 set, which no profile draws. */
static cogmaster *fault_run(int gun)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    m->vdp.reg[0] = 0u;
    m->vdp.reg[1] = (uint8_t)((m->vdp.reg[1] & ~0x08u) | 0x10u);
    m->vdp.display = 1u;
    const int32_t axis[4] = {100, 3, 0, 0};
    CHECK_EQ_U64(send(m, gun ? PHASER : COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, axis),
                 COGMASTER_STATUS_OK);
    m->input = m->pending_input;
    walk_past_wrap(m);
    for (uint32_t at = 7u; at < 20u * LINE_TICKS; at += 37u) {
        walk_to(m, at / LINE_TICKS, at % LINE_TICKS);
        if (gun) {
            (void)cogmaster__io_in(m, 0xDDu);
            (void)cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT);
        }
    }
    walk_to(m, 30u, 0u);
    return m;
}

static void test_fault_neutral(void)
{
    cogmaster *g = fault_run(1);
    cogmaster *p = fault_run(0);
    CHECK_EQ_U64(p->fault_present, 1u);
    CHECK_EQ_U64(g->fault_present, 1u);
    CHECK_EQ_U64(p->fault.kind, COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE);
    CHECK(memcmp(&g->fault, &p->fault, sizeof g->fault) == 0);
    /* Where the renderer's own schedule puts it: the end of the first line's active period. */
    CHECK_EQ_U64(p->fault.line, 0u);
    CHECK_EQ_U64(p->fault.dot, COGMASTER__FIELD_W);
    CHECK(g->gun_stats.syncs > 0u);
    cogmaster_destroy(g);
    cogmaster_destroy(p);
}

/* ---- programs: assembled here, run by the CPU ---- */

enum { PROG_READ_7F, PROG_READ_DD, PROG_POLL, PROG_COUNT };

/* in a,($7F) and in a,($DD) forever. */
static const uint8_t prog_read_7f[] = {0xDB, 0x7F, 0xC3, 0x00, 0x00};
static const uint8_t prog_read_dd[] = {0xDB, 0xDD, 0xC3, 0x00, 0x00};
/* Every port a game reads and writes: $DC into RAM and, shifted down, into the backdrop register;
   $DD, $7E and $7F into RAM; $3F written with every TH an input and then an output. */
static const uint8_t prog_poll[] = {
    0xDB, 0xDC,             /* in a,($DC) */
    0x32, 0x00, 0xC0,       /* ld ($C000),a */
    0x0F, 0x0F, 0x0F, 0x0F, /* rrca, four times */
    0xE6, 0x0F,             /* and $0F */
    0xD3, 0xBF,             /* out ($BF),a: the value for backdrop register 7 */
    0x3E, 0x87,             /* ld a,$87 */
    0xD3, 0xBF,             /* out ($BF),a: select register 7 */
    0xDB, 0xDD,             /* in a,($DD) */
    0x32, 0x01, 0xC0,       /* ld ($C001),a */
    0xDB, 0x7E,             /* in a,($7E) */
    0xDB, 0x7F,             /* in a,($7F) */
    0x32, 0x02, 0xC0,       /* ld ($C002),a */
    0x3E, 0xFF,             /* ld a,$FF */
    0xD3, 0x3F,             /* out ($3F),a: every pin an input */
    0x3E, 0xF5,             /* ld a,$F5 */
    0xD3, 0x3F,             /* out ($3F),a: both TH lines outputs */
    0xC3, 0x00, 0x00,       /* jp 0 */
};

static const struct {
    const uint8_t *code;
    size_t size;
} programs[PROG_COUNT] = {
    {prog_read_7f, sizeof prog_read_7f},
    {prog_read_dd, sizeof prog_read_dd},
    {prog_poll, sizeof prog_poll},
};

static uint8_t program_images[PROG_COUNT][IMAGE_SIZE];

/* An instance running a program, with the square picture drawn and distinct backdrop colours. */
static cogmaster *machine_running(uint32_t which, uint32_t machine, uint32_t region, uint32_t tv)
{
    memset(program_images[which], 0x76, IMAGE_SIZE);
    memcpy(program_images[which], programs[which].code, programs[which].size);
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = program_images[which];
    media.image_size = IMAGE_SIZE;
    media.machine = machine;
    media.region = region;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    draw(m, 192u);
    m->vdp.cram[16u + 14u] = 0x30u;
    m->vdp.cram[16u + 15u] = 0x0Cu;
    return m;
}

/* An aim over black dots, clear of every square of the picture. */
#define DARK_X 190
#define DARK_Y 140

/* A frame that only reads the H counter and the V counter is lagged as it is without a gun, one
   that reads $DD is not, nothing the sensor does reaches the open-bus count or the bus byte, and a
   console with no gun does no sensor work at all. */
static void test_lag(void)
{
    static const uint32_t which[2] = {PROG_READ_7F, PROG_READ_DD};
    for (uint32_t w = 0u; w < 2u; w++) {
        cogmaster *g = machine_running(which[w], COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT,
                                       COGMASTER_TV_NTSC);
        cogmaster *p = machine_running(which[w], COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT,
                                       COGMASTER_TV_NTSC);
        /* The gun looks at black dots: a latched H counter would be a different $7F, which is the
           program's own business, not the sensor's. */
        aim_one(g, 0u, DARK_X, DARK_Y);
        aim_ports(p, COGMASTER_DEVICE_PAD, 0, 0, COGMASTER_DEVICE_PAD, 0, 0);
        for (uint32_t f = 0u; f < 3u; f++) {
            frame(g);
            frame(p);
        }
        CHECK_EQ_U64(g->counters.lag_frames, p->counters.lag_frames);
        CHECK_EQ_U64(g->counters.open_bus_reads, p->counters.open_bus_reads);
        CHECK_EQ_HEX(g->bus.data, p->bus.data);
        CHECK_EQ_U64(g->counters.lag_frames, w == 0u ? 4u : 0u);
        cogmaster_destroy(g);
        cogmaster_destroy(p);
    }
    /* No gun: every counter stays zero over a program that touches every port. */
    cogmaster *p = machine_running(PROG_POLL, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT,
                                   COGMASTER_TV_NTSC);
    aim_ports(p, COGMASTER_DEVICE_PAD, 0, 0, COGMASTER_DEVICE_PAD, 0, 0);
    for (uint32_t f = 0u; f < 3u; f++) {
        frame(p);
    }
    CHECK_EQ_U64(p->gun_stats.samples, 0u);
    CHECK_EQ_U64(p->gun_stats.syncs, 0u);
    CHECK_EQ_U64(p->gun_stats.edges, 0u);
    cogmaster_destroy(p);
    /* The control: the same program with a gun on a lit square does work. */
    cogmaster *g = machine_running(PROG_POLL, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT,
                                   COGMASTER_TV_NTSC);
    aim_one(g, 0u, (int32_t)(squares[0].x + 9u), (int32_t)(squares[0].y + 9u));
    for (uint32_t f = 0u; f < 3u; f++) {
        frame(g);
    }
    CHECK(g->gun_stats.syncs > 0u);
    CHECK(g->gun_stats.samples > 0u);
    CHECK(g->gun_stats.edges > 0u);
    cogmaster_destroy(g);
}

/* ---- the pulse bit, observation, the no-shot invariant, bounds ---- */

/* A gun aimed at the right edge of the first square, past the wrap of the picture it scans. */
static cogmaster *pulse_machine(void)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    aim_one(m, 0u, (int32_t)(squares[0].x + SQUARE_SIZE),
            (int32_t)(squares[0].y + SQUARE_SIZE / 2u + 1u));
    walk_past_wrap(m);
    m->vdp.hheld = 0u;
    return m;
}

static uint32_t th1_high(cogmaster *m)
{
    return (cogmaster__io_in(m, 0xDDu) & 0x40u) != 0u ? 1u : 0u;
}

/* A light pulse is seen by a read that comes after it, once, and never by a read in a later
   picture; an output TH never takes the bit. */
static void test_seen_expiry(void)
{
    /* No read until the next picture is armed: the pulse is gone. */
    cogmaster *m = pulse_machine();
    walk_to(m, 150u, 0u);
    walk_past_wrap(m);
    CHECK(m->gun_stats.edges > 0u);
    CHECK_EQ_U64(m->vdp.hheld, 1u);
    CHECK_EQ_U64(th1_high(m), 1u);
    cogmaster_destroy(m);
    /* The same pulse with a read before that wrap: low once, then high, and high after the wrap. */
    m = pulse_machine();
    walk_to(m, 100u, 0u);
    CHECK_EQ_U64(th1_high(m), 0u);
    CHECK_EQ_U64(th1_high(m), 1u);
    walk_past_wrap(m);
    CHECK_EQ_U64(th1_high(m), 1u);
    cogmaster_destroy(m);
    /* A pulse that happens while TH is an output leaves no bit and no latch. */
    m = pulse_machine();
    cogmaster__io_out(m, 0x3Fu, 0xFDu);
    walk_to(m, 100u, 0u);
    CHECK_EQ_U64(th1_high(m), 1u);
    CHECK(m->gun_stats.edges > 0u);
    CHECK_EQ_U64(m->ports[0].g_sense & 2u, 0u);
    CHECK_EQ_U64(m->vdp.hheld, 0u);
    CHECK_EQ_U64(th1_high(m), 1u);
    cogmaster_destroy(m);
}

/* Looking at a lit, scanned instance changes nothing, and a second $DD read changes nothing that
   the first did not. */
static void test_peek(void)
{
    cogmaster *m = pulse_machine();
    walk_to(m, 30u, 300u);
    (void)cogmaster__io_in(m, 0xDDu);
    CHECK(m->gun_stats.samples > 0u);
    cogmaster *snap = malloc(sizeof *snap);
    CHECK(snap != NULL);
    if (snap == NULL) {
        exit(1);
    }
    memcpy(snap, m, sizeof *snap);
    for (uint32_t addr = 0u; addr < 0x10000u; addr += 251u) {
        (void)cogmaster_peek(m, COGMASTER_SPACE_BUS, addr);
    }
    for (uint32_t port = 0u; port < 2u; port++) {
        (void)cogmaster__ports_pins(m, port, m->ticks);
    }
    CHECK(memcmp(snap, m, sizeof *snap) == 0);
    /* The second read of one tick: only the count of syncs moves. */
    (void)cogmaster__io_in(m, 0xDDu);
    memcpy(snap, m, sizeof *snap);
    (void)cogmaster__io_in(m, 0xDDu);
    snap->gun_stats.syncs = m->gun_stats.syncs;
    CHECK(memcmp(snap, m, sizeof *snap) == 0);
    free(snap);
    cogmaster_destroy(m);
}

/* ---- the invariant: no shot, no light ---- */

static const char *const noshot_masked[] = {
    "input_port1", "input_port2",   "pending_x1",     "pending_y1",
    "pending_x2",  "pending_y2",    "active_x1",      "active_y1",
    "active_x2",   "active_y2",     "port_gun_ax",    "port_gun_dot",
    "port_gun_ay", "port_gun_line", "port_gun_sense", "port_gun_scan",
};

static int is_masked(const char *name)
{
    for (size_t i = 0; i < sizeof noshot_masked / sizeof noshot_masked[0]; i++) {
        if (strcmp(name, noshot_masked[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

#define MAX_FIELDS 240000u
static struct cogmaster__state_field fields[MAX_FIELDS];

/* The name of the first saved field the two instances differ in, apart from the masked ones, or
   NULL when there is none. */
static const char *first_difference(cogmaster *a, cogmaster *b)
{
    size_t size = cogmaster__state_size(a);
    uint8_t *sa = malloc(size);
    uint8_t *sb = malloc(size);
    const char *found = NULL;
    CHECK_EQ_U64(cogmaster__state_size(b), size);
    if (sa == NULL || sb == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(a, sa, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_save(b, sb, size), COGMASTER_STATUS_OK);
    size_t n = cogmaster__state_record(a, fields, MAX_FIELDS);
    CHECK(n <= MAX_FIELDS);
    for (size_t i = 0; i < n && i < MAX_FIELDS && found == NULL; i++) {
        /* The body's checksum is derived from every field, the masked ones included, so it is the
           one name besides them that is not compared. */
        if (!is_masked(fields[i].name) && strcmp(fields[i].name, "body_crc32") != 0 &&
            memcmp(sa + fields[i].offset, sb + fields[i].offset, fields[i].width) != 0) {
            found = fields[i].name;
        }
    }
    free(sa);
    free(sb);
    return found;
}

static int16_t audio_a[2u * COGMASTER__AUDIO_MAX_FRAMES];
static int16_t audio_b[2u * COGMASTER__AUDIO_MAX_FRAMES];

/* The audio a frame produced, in frames written. */
static uint32_t drain(cogmaster *m, int16_t *buf)
{
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = buf;
    a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    return a.written;
}

/* The reference run's frame: its sound is taken once, and every other run's frame is compared with
 * it. */
static cogmaster *reference;
static uint32_t reference_frames;

static void take_reference(cogmaster *m)
{
    reference = m;
    reference_frames = drain(m, audio_b);
}

/* The picture and the sound a frame produced equal the reference's. */
static int same_as_reference(cogmaster *a)
{
    uint32_t na = drain(a, audio_a);
    return na == reference_frames &&
           memcmp(audio_a, audio_b, (size_t)na * 2u * sizeof audio_a[0]) == 0 &&
           memcmp(&a->completed, &reference->completed, sizeof a->completed) == 0;
}

/* The polling loop's phase against the frame, which is what moves a poll across the disc, repeats
   within 150 frames on every profile (the last new (pc, overshoot) pair appears at frame 149 on
   PAL and 127 on the others), so 160 frames visit every alignment and more only revisit them. The
   controls need a frame or two to differ. */
#define NOSHOT_FRAMES 160u
#define NOSHOT_SHORT 8u
#define TRIGGER 0x10u

static cogmaster *noshot_machine(uint32_t pr, uint32_t d1, int32_t ax, int32_t ay, uint32_t held)
{
    cogmaster *m =
        machine_running(PROG_POLL, profiles[pr].machine, profiles[pr].region, profiles[pr].tv);
    const int32_t axis[4] = {ax, ay, 0, 0};
    CHECK_EQ_U64(send(m, d1, COGMASTER_DEVICE_PAD, held, 0u, axis), COGMASTER_STATUS_OK);
    return m;
}

/* A game that polls every port, with the gun plugged in, the trigger never pulled and nothing lit
   in the sensor's view, plays exactly as with a pad and nothing pressed: the picture and sound of
   every frame and the whole saved state, apart from the gun's own fields. The control: aimed at a
   lit square it does not, and a pulled trigger changes the picture. */
static void test_noshot(void)
{
    /* Over black dots, with the lit squares elsewhere in the picture; and over the second square.
     */
    const int32_t dark_x = DARK_X;
    const int32_t dark_y = DARK_Y;
    const int32_t lit_x = (int32_t)(squares[1].x + SQUARE_SIZE / 2u + 1u);
    const int32_t lit_y = (int32_t)(squares[1].y + SQUARE_SIZE / 2u + 1u);
    for (uint32_t pr = 0u; pr < 4u; pr++) {
        cogmaster *pad = noshot_machine(pr, COGMASTER_DEVICE_PAD, 0, 0, 0u);
        cogmaster *dark = noshot_machine(pr, PHASER, dark_x, dark_y, 0u);
        cogmaster *parked = noshot_machine(pr, PHASER, 0, 0, 0u);
        cogmaster *lit = noshot_machine(pr, PHASER, lit_x, lit_y, 0u);
        cogmaster *shot = noshot_machine(pr, PHASER, dark_x, dark_y, TRIGGER);
        uint32_t shot_differs = 0u;
        for (uint32_t f = 0u; f < NOSHOT_FRAMES; f++) {
            frame(pad);
            frame(dark);
            frame(parked);
            take_reference(pad);
            CHECK(same_as_reference(dark));
            CHECK(same_as_reference(parked));
            if (f < NOSHOT_SHORT) {
                frame(lit);
                frame(shot);
                if (memcmp(&shot->completed, &pad->completed, sizeof pad->completed) != 0) {
                    shot_differs = 1u;
                }
            }
        }
        const char *diff = first_difference(dark, pad);
        if (diff != NULL) {
            fprintf(stderr, "profile %u: dark aim differs in %s\n", (unsigned)pr, diff);
        }
        CHECK(diff == NULL);
        diff = first_difference(parked, pad);
        if (diff != NULL) {
            fprintf(stderr, "profile %u: parked aim differs in %s\n", (unsigned)pr, diff);
        }
        CHECK(diff == NULL);
        /* The controls: the lit aim changes the state the program can see, the trigger the image.
         */
        CHECK(first_difference(lit, pad) != NULL);
        CHECK(shot_differs != 0u);
        /* The scanner ran over the whole disc in the dark run, so the pass above is not parked. */
        CHECK(dark->gun_stats.samples > 100u);
        CHECK_EQ_U64(dark->gun_stats.edges, 0u);
        CHECK_EQ_U64(parked->gun_stats.samples, 0u);
        cogmaster_destroy(pad);
        cogmaster_destroy(dark);
        cogmaster_destroy(parked);
        cogmaster_destroy(lit);
        cogmaster_destroy(shot);
    }
}

/* ---- bounds ---- */

static void gun_hammer_tick(cogmaster *m)
{
    (void)cogmaster__io_in(m, 0xDDu);
    (void)cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT);
    cogmaster__io_out(m, 0x3Fu, 0xFFu);
}

#define HAMMER_ACCESSES 1000000u
/* The dots of the disc by a plain count, one chord of at most 2 R + 1 dots on each of 2 R + 1 rows,
   and the closing dot each row adds. */
static uint32_t disc_dots(void)
{
    uint32_t n = 0u;
    for (int32_t dy = -(int32_t)GUN_RADIUS; dy <= (int32_t)GUN_RADIUS; dy++) {
        for (int32_t dx = -(int32_t)GUN_RADIUS; dx <= (int32_t)GUN_RADIUS; dx++) {
            n += dx * dx + dy * dy <= (int32_t)(GUN_RADIUS * GUN_RADIUS) ? 1u : 0u;
        }
    }
    return n;
}
#define MAX_SAMPLES_ROW (2u * GUN_RADIUS + 2u)
#define MAX_SAMPLES_PICTURE (disc_dots() + 2u * GUN_RADIUS + 1u)

/* A million of each access at one tick leave the sensor where its first call put it, and a
   hammer at every tick of a picture still looks at each dot once. */
static void test_hammer(void)
{
    cogmaster *m = pulse_machine();
    walk_to(m, 30u, 200u);
    gun_hammer_tick(m);
    struct cogmaster__port block[2];
    memcpy(block, m->ports, sizeof block);
    uint8_t h = m->vdp.hcounter;
    uint32_t held = m->vdp.hheld;
    uint32_t line = m->render_line;
    uint32_t dot = m->render_x;
    uint32_t samples = m->gun_stats.samples;
    uint32_t edges = m->gun_stats.edges;
    uint32_t syncs = m->gun_stats.syncs;
    for (uint32_t i = 0u; i < HAMMER_ACCESSES; i++) {
        gun_hammer_tick(m);
    }
    CHECK(memcmp(block, m->ports, sizeof block) == 0);
    CHECK_EQ_HEX(m->vdp.hcounter, h);
    CHECK_EQ_U64(m->vdp.hheld, held);
    CHECK_EQ_U64(m->render_line, line);
    CHECK_EQ_U64(m->render_x, dot);
    CHECK_EQ_U64(m->gun_stats.samples, samples);
    CHECK_EQ_U64(m->gun_stats.edges, edges);
    CHECK_EQ_U64(m->gun_stats.syncs, syncs + 3u * HAMMER_ACCESSES);
    cogmaster_destroy(m);

    /* Every tick of a picture, three accesses each. */
    m = pulse_machine();
    uint32_t base = m->gun_stats.samples;
    uint32_t row_start = base;
    for (uint32_t y = 0u; y < 192u; y++) {
        for (uint32_t t = 0u; t < LINE_TICKS; t++) {
            walk_to(m, y, t);
            gun_hammer_tick(m);
        }
        CHECK(m->gun_stats.samples - row_start <= MAX_SAMPLES_ROW);
        row_start = m->gun_stats.samples;
    }
    CHECK(m->gun_stats.samples > base);
    CHECK(m->gun_stats.samples - base <= MAX_SAMPLES_PICTURE);
    cogmaster_destroy(m);
}

/* ---- schedule independence ---- */

/* A white band across the picture, one tile row tall, so two discs aimed at different columns meet
   light on the same rows. Aims are one-based dots. */
#define BAND_TILE_ROW 15u
#define BAND_AIM_Y 124
#define BAND_X1 150
#define BAND_X2 100

static void band(cogmaster *m, uint32_t height, uint32_t tile_row)
{
    for (uint32_t col = 0u; col < 32u; col++) {
        m->vdp.vram[name_base(height) + (tile_row * 32u + col) * 2u] = 1u;
    }
}

/* The state a poll schedule must not change: both blocks but the pulse bit (a read takes it), the
   H counter and its hold, and the picture. The render cursor is not in it: the sensor renders only
   as far as a dot it needs, so a polled run's cursor is where its last need put it. */
static int same_sensor_state(const cogmaster *a, const cogmaster *b)
{
    int same = a->vdp.hheld == b->vdp.hheld && a->vdp.hcounter == b->vdp.hcounter &&
               memcmp(a->field, b->field, sizeof a->field) == 0;
    for (uint32_t port = 0u; port < 2u; port++) {
        struct cogmaster__port pa = a->ports[port];
        struct cogmaster__port pb = b->ports[port];
        pa.g_sense &= 1u;
        pb.g_sense &= 1u;
        same = same && memcmp(&pa, &pb, sizeof pa) == 0;
    }
    return same;
}

static cogmaster *walk_machine(int two)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    band(m, 192u, BAND_TILE_ROW);
    if (two) {
        aim_ports(m, PHASER, BAND_X1, BAND_AIM_Y, PHASER, BAND_X2, BAND_AIM_Y);
    } else {
        aim_ports(m, PHASER, (int32_t)(squares[0].x + SQUARE_SIZE),
                  (int32_t)(squares[0].y + SQUARE_SIZE / 2u + 1u), COGMASTER_DEVICE_PAD, 0, 0);
    }
    walk_past_wrap(m);
    return m;
}

static uint32_t ticks_to(const cogmaster *m, uint32_t line, uint32_t t)
{
    uint32_t total = cogmaster__frame_lines(m) * LINE_TICKS;
    return (line * LINE_TICKS + t + total - (m->bus.line * LINE_TICKS + m->bus.t)) % total;
}

/* Moves the beam n ticks: one tick at a time with a $DD read at each, or in steps to the next line
   start with no access at all. */
static void walk_ticks(cogmaster *m, uint32_t n, int polled)
{
    while (n > 0u) {
        uint32_t step = polled ? 1u : LINE_TICKS - m->bus.t;
        step = step > n ? n : step;
        m->ticks += step;
        cogmaster__bus_sync(m);
        if (polled) {
            (void)cogmaster__io_in(m, 0xDDu);
        }
        n -= step;
    }
}

/* The half width of a row's chord, found by growing it, not by the core's shrinking loop. */
static uint32_t chord_half(uint32_t dy)
{
    uint32_t h = 0u;
    while ((h + 1u) * (h + 1u) + dy * dy <= GUN_RADIUS * GUN_RADIUS) {
        h++;
    }
    return h;
}

/* The tick the sensor releases a band row at, for an aim: one dot past the chord, a delay later. */
static uint32_t band_release(uint32_t ax, uint32_t row)
{
    uint32_t aim_row = (uint32_t)BAND_AIM_Y - 1u;
    uint32_t dy = row > aim_row ? row - aim_row : aim_row - row;
    return 2u * (ax - 1u + chord_half(dy) + 1u) + GUN_DELAY_TICKS;
}

/* A read at every tick for two frames equals steps of whole lines with no read, at the frame
   boundary the library syncs at and at the end, for one gun and for two. */
static void walk_pair(int two)
{
    cogmaster *x = walk_machine(two);
    cogmaster *y = walk_machine(two);
    uint32_t frame_ticks = cogmaster__frame_lines(x) * LINE_TICKS;
    /* The checkpoints: inside the first band row, at the end of the band, the frame boundary. */
    static const uint32_t at[3][2] = {{120u, 500u}, {127u, 683u}, {192u, 610u}};
    for (uint32_t c = 0u; c < 3u; c++) {
        uint32_t nx = ticks_to(x, at[c][0], at[c][1]);
        walk_ticks(x, nx, 1);
        walk_ticks(y, ticks_to(y, at[c][0], at[c][1]), 0);
        cogmaster__gun_sync(y);
        CHECK(same_sensor_state(x, y));
        CHECK_EQ_U64(x->vdp.hheld, 1u);
        if (two && c < 2u) {
            /* Port 1 releases later than port 2 on every band row, so its edge is the latch. */
            uint32_t row = at[c][0] == 120u ? 120u : 127u;
            CHECK(band_release(BAND_X1, row) > band_release(BAND_X2, row));
            CHECK_EQ_HEX(x->vdp.hcounter, oracle_h(band_release(BAND_X1, row)));
            CHECK(oracle_h(band_release(BAND_X1, row)) != oracle_h(band_release(BAND_X2, row)));
        }
    }
    walk_ticks(
        x, ticks_to(x, frame_ticks / LINE_TICKS - 1u, COGMASTER__VDP_WRAP_TICK + 1u) + frame_ticks,
        1);
    walk_ticks(
        y, ticks_to(y, frame_ticks / LINE_TICKS - 1u, COGMASTER__VDP_WRAP_TICK + 1u) + frame_ticks,
        0);
    CHECK(same_sensor_state(x, y));
    cogmaster_destroy(x);
    cogmaster_destroy(y);
}

/* A picture of height `from` with white tile rows across the bottom of its disc, a gun aimed at
   `ay`, and rows from `from` down bright in the field: the leftovers of an earlier, taller
   picture, which a sensor still scanning past the old picture's height would read. The beam is one
   tick past the wrap that armed the gun for the picture of height `from`. */
static cogmaster *height_machine(uint32_t from, int32_t ay)
{
    cogmaster *m = picture(from, COGMASTER_REGION_EXPORT);
    for (uint32_t row = ((uint32_t)ay - 1u) / 8u - 1u; row < from / 8u; row++) {
        band(m, from, row);
    }
    aim_one(m, 0u, BAND_X2, ay);
    walk_past_wrap(m);
    for (uint32_t y = from; y < 240u; y++) {
        for (uint32_t x = 0u; x < 256u; x++) {
            m->field[y][x] = 0x0FFFu;
        }
    }
    return m;
}

/* A height change between two pictures, with the aim on the old picture's last rows, so the disc is
   cut by the old bottom and its first and last rows differ in chord. One twin is
   read at every tick from the frame's end-of-window catch-up (line 192) to the wrap, the other not
   at all. The mode registers change after the old picture is drawn, line 230 being past the bottom
   of every old height here. The two must hold the same sensor state, and the H latch must be that
   of the last row the old picture has. */
static void walk_height(uint32_t from, uint32_t to, int32_t ay)
{
    cogmaster *twin[2] = {height_machine(from, ay), height_machine(from, ay)};
    for (uint32_t i = 0u; i < 2u; i++) {
        cogmaster *m = twin[i];
        uint32_t last = cogmaster__frame_lines(m) - 1u;
        walk_ticks(m, ticks_to(m, 192u, 0u), 0);
        cogmaster__gun_sync(m);
        walk_ticks(m, ticks_to(m, 230u, 0u), (int)(i == 0u));
        height_mode(m, to);
        walk_ticks(m, ticks_to(m, last, COGMASTER__VDP_WRAP_TICK + 1u), (int)(i == 0u));
        CHECK_EQ_U64(m->vdp.height, to);
    }
    CHECK(same_sensor_state(twin[0], twin[1]));
    /* The last disc row inside the old picture: its release is the last rising TH. */
    uint32_t aim_row = (uint32_t)ay - 1u;
    uint32_t last = aim_row + GUN_RADIUS < from ? aim_row + GUN_RADIUS : from - 1u;
    uint32_t dy = last > aim_row ? last - aim_row : aim_row - last;
    uint32_t release = 2u * ((uint32_t)BAND_X2 - 1u + chord_half(dy) + 1u) + GUN_DELAY_TICKS;
    for (uint32_t i = 0u; i < 2u; i++) {
        CHECK_EQ_U64(twin[i]->vdp.hheld, 1u);
        CHECK_EQ_HEX(twin[i]->vdp.hcounter, oracle_h(release));
        cogmaster_destroy(twin[i]);
    }
}

static void test_walk(void)
{
    walk_pair(0);
    walk_pair(1);
    walk_height(224u, 192u, 221);
    walk_height(224u, 240u, 223);
    walk_height(192u, 240u, 190);
    walk_height(192u, 224u, 190);
}

/* ---- TH written while the sensor sees light ---- */

/* The effective TH pins of the export console from $3F and the sensor's level, written again
   here: an input TH reads the sensor, an output one its level bit, and port 2 has no gun. */
static uint8_t model_th(uint8_t ctrl, int lit)
{
    uint8_t th1 = (uint8_t)((ctrl & 2u) != 0u ? (lit ? 0u : 1u) : ((ctrl >> 5) & 1u));
    uint8_t th2 = (ctrl & 8u) != 0u ? 1u : (uint8_t)((ctrl >> 7) & 1u);
    return (uint8_t)((th1 << 6) | (th2 << 7));
}

/* A million $3F writes at one tick while the sensor sees light: the H counter is latched exactly
   when the model's effective TH rises, so the number of latches is the model's edge count. */
static void test_th_toggle(void)
{
    cogmaster *m = picture(192u, COGMASTER_REGION_EXPORT);
    band(m, 192u, BAND_TILE_ROW);
    aim_one(m, 0u, 129, BAND_AIM_Y);
    walk_past_wrap(m);
    walk_to(m, BAND_AIM_Y - 1, 2u * 128u + GUN_DELAY_TICKS);
    CHECK_EQ_U64(th_low(m, 0u), 1u);
    struct cogmaster__port block[2];
    memcpy(block, m->ports, sizeof block);
    uint8_t ctrl = 0xFFu;
    uint32_t seed = 12345u;
    uint32_t latches = 0u;
    uint32_t model = 0u;
    uint32_t wrong = 0u;
    for (uint32_t i = 0u; i < HAMMER_ACCESSES; i++) {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        uint8_t next = (uint8_t)(seed >> 8);
        uint32_t rises = ((uint8_t)~model_th(ctrl, 1) & model_th(next, 1) & 0xC0u) != 0u ? 1u : 0u;
        m->vdp.hheld = 0u;
        cogmaster__io_out(m, 0x3Fu, next);
        wrong += m->vdp.hheld != rises ? 1u : 0u;
        latches += m->vdp.hheld;
        model += rises;
        ctrl = next;
    }
    CHECK_EQ_U64(wrong, 0u);
    CHECK_EQ_U64(latches, model);
    CHECK(latches > 0u);
    /* Nothing grew: the sensor's cursor is where the one sync left it. */
    CHECK(memcmp(block, m->ports, sizeof block) == 0);
    cogmaster_destroy(m);
}

/* ---- the one delay does not read the header ---- */

#define FIXTURE_SIZE 32768u
#define MAILBOX_AT 0x100u
#define MAILBOX_LEN 0x100u
#define MAILBOX_STATUS 0x105u
#define IDENTITY_CAP 600u

static uint8_t fixture_image[FIXTURE_SIZE];
static uint8_t fixture_flipped[FIXTURE_SIZE];

/* An instance of the fixture with the phaser on port 1 at a fixed aim, run until the mailbox says
   done (magic written, status 0) or the cap. The frames it took are returned. */
static cogmaster *identity_run(const uint8_t *img, uint32_t pr, int32_t ax, int32_t ay,
                               uint32_t *frames)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = img;
    media.image_size = FIXTURE_SIZE;
    media.machine = profiles[pr].machine;
    media.region = profiles[pr].region;
    media.tv = profiles[pr].tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    const int32_t axis[4] = {ax, ay, 0, 0};
    CHECK_EQ_U64(send(m, PHASER, COGMASTER_DEVICE_PAD, 0u, 0u, axis), COGMASTER_STATUS_OK);
    uint32_t f = 0u;
    while (f < IDENTITY_CAP &&
           !(memcmp(&m->ram[MAILBOX_AT], "CMDG", 4u) == 0 && m->ram[MAILBOX_STATUS] == 0u)) {
        frame(m);
        f++;
    }
    CHECK(f < IDENTITY_CAP);
    *frames = f;
    return m;
}

/* The fixture with the product code, the checksum and the region nibble of its header flipped, and
   the same aim, gives the same mailbox, the same picture and the same frame count on every profile,
   and a run with the aim parked does not, so the equality compares a run that senses. This catches
   a branch keyed on the header of the owned image only; the static check over the sensor's files is
   what covers the rest. The flipped bytes are in the test's copy; nothing is written to disk. */
static void test_identity(const char *path, int32_t ax, int32_t ay)
{
    FILE *in = fopen(path, "rb");
    if (in == NULL || fread(fixture_image, 1u, FIXTURE_SIZE, in) != FIXTURE_SIZE ||
        fgetc(in) != EOF) {
        fprintf(stderr, "cogmaster-test-ports-gun: %s is not a 32768-byte image\n", path);
        exit(1);
    }
    fclose(in);
    memcpy(fixture_flipped, fixture_image, FIXTURE_SIZE);
    for (uint32_t i = 0x7FFAu; i < 0x7FFFu; i++) {
        fixture_flipped[i] ^= 0xFFu;
    }
    fixture_flipped[0x7FFFu] ^= 0x70u;
    CHECK(memcmp(fixture_flipped, fixture_image, FIXTURE_SIZE) != 0);
    for (uint32_t pr = 0u; pr < 4u; pr++) {
        uint32_t fa, fb, fp;
        cogmaster *a = identity_run(fixture_image, pr, ax, ay, &fa);
        cogmaster *b = identity_run(fixture_flipped, pr, ax, ay, &fb);
        cogmaster *p = identity_run(fixture_image, pr, 0, 0, &fp);
        CHECK_EQ_U64(fa, fb);
        CHECK(memcmp(&a->ram[MAILBOX_AT], &b->ram[MAILBOX_AT], MAILBOX_LEN) == 0);
        CHECK(memcmp(&a->completed, &b->completed, sizeof a->completed) == 0);
        CHECK(memcmp(&a->ram[MAILBOX_AT], &p->ram[MAILBOX_AT], MAILBOX_LEN) != 0);
        cogmaster_destroy(a);
        cogmaster_destroy(b);
        cogmaster_destroy(p);
    }
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"latch_at", test_latch_at},
    {"aim", test_aim},
    {"pins", test_pins},
    {"hostile_aim", test_hostile_aim},
    {"model", test_model},
    {"edge", test_edge},
    {"luma", test_luma},
    {"control", test_control},
    {"order", test_order},
    {"fault_neutral", test_fault_neutral},
    {"lag", test_lag},
    {"seen_expiry", test_seen_expiry},
    {"peek", test_peek},
    {"noshot", test_noshot},
    {"hammer", test_hammer},
    {"walk", test_walk},
    {"th_toggle", test_th_toggle},
};

int main(int argc, char **argv)
{
    if (argc == 5 && strcmp(argv[1], "identity") == 0) {
        test_identity(argv[2], (int32_t)strtol(argv[3], NULL, 10),
                      (int32_t)strtol(argv[4], NULL, 10));
        CHECK_DONE();
    }
    if (argc == 2) {
        for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            if (strcmp(argv[1], cases[i].name) == 0) {
                cases[i].run();
                CHECK_DONE();
            }
        }
    }
    fprintf(stderr, "cogmaster-test-ports-gun: unknown case\n");
    return 2;
}
