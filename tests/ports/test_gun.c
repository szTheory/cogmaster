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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"latch_at", test_latch_at},
    {"aim", test_aim},
    {"pins", test_pins},
    {"hostile_aim", test_hostile_aim},
};

int main(int argc, char **argv)
{
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
