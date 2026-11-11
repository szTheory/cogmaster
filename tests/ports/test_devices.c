/* The Sports Pad and the Paddle through the internal API: cogmaster-test-ports-devices <case>.
   Writes to $3F at chosen ticks and reads of $DC and $DD are made on a loaded instance, the way a
   program's port accesses reach it, and every expected value is computed here from the scripted
   literals and from the protocol in SMS-CARTRIDGES-AND-PERIPHERALS section 3.4 and the two after it
   (a read is a high nibble at a TH fall and the low nibble at the next rise; the pad's byte is the
   negated host motion). The reference model in the profile case is written apart from src/ports.c
   and shares none of its code. Tick constants are named by the header, never repeated here. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "sha256.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define PAD_PINS 0x1Fu  /* D0-D3 and TL; TR is the console's to override */
#define KEY_BITS 0x30u  /* TL and TR in the held byte */
#define FIRST_WAIT 858u /* TH low before the first nibble: about 80 microseconds of ticks */
#define NEXT_WAIT 429u  /* and between the other nibbles: about 40 microseconds */
#define CTRL_BASE 0x05u /* TR as inputs, TH as outputs; the levels are b5 and b7 */
#define HELD_1                                                                                     \
    0x10u /* the script's held bytes: TL pressed on the first pad, only TR on the second */
#define HELD_2 0x20u

static uint8_t image[IMAGE_SIZE];
static uint32_t level[2]; /* the TH levels the helper has written, per port */

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

static void halt_image(void)
{
    memset(image, 0x76u, sizeof image);
}

static void media_for(cogmaster_media *md, uint32_t machine, uint32_t region)
{
    memset(md, 0, sizeof *md);
    md->size = (uint32_t)sizeof *md;
    md->image = image;
    md->image_size = sizeof image;
    md->machine = machine;
    md->region = region;
    md->tv = COGMASTER_TV_NTSC;
}

static void drive_th(cogmaster *m, uint64_t tick);

/* A new instance with both TH lines driven as outputs and high, which changes no wire. */
static cogmaster *load(uint32_t machine, uint32_t region)
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
    cogmaster_media md;
    media_for(&md, machine, region);
    CHECK_EQ_U64(cogmaster_load(m, &md), COGMASTER_STATUS_OK);
    level[0] = level[1] = 1u;
    drive_th(m, 0u);
    return m;
}

/* $3F at a tick, with the levels this helper last set. The beam is brought up to the tick first,
   as the bus does before any port access, so the state stays one a run could have saved. */
static void drive_th(cogmaster *m, uint64_t tick)
{
    m->ticks = tick;
    cogmaster__bus_sync(m);
    cogmaster__io_out(m, 0x3Fu, (uint8_t)(CTRL_BASE | (level[0] << 5) | (level[1] << 7)));
}

static void th(cogmaster *m, uint32_t port, uint32_t to, uint64_t tick)
{
    level[port] = to;
    drive_th(m, tick);
}

/* The five pins D0-D3 and TL of a port as the program reads them. */
static uint32_t pins_of(cogmaster *m, uint32_t port)
{
    uint32_t dc = cogmaster__io_in(m, 0xDCu);
    uint32_t dd = cogmaster__io_in(m, 0xDDu);
    return port == 0u ? dc & PAD_PINS : ((dc >> 6) & 3u) | ((dd & 7u) << 2);
}

static uint32_t tr_of(cogmaster *m, uint32_t port)
{
    return port == 0u ? (cogmaster__io_in(m, 0xDCu) >> 5) & 1u
                      : (cogmaster__io_in(m, 0xDDu) >> 3) & 1u;
}

static cogmaster_status send(cogmaster *m, uint32_t d1, uint32_t d2, uint32_t p1, uint32_t p2,
                             int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    cogmaster_input in;
    memset(&in, 0, sizeof in);
    in.size = (uint32_t)sizeof in;
    in.port1 = d1;
    in.port2 = d2;
    in.p1 = p1;
    in.p2 = p2;
    in.x1 = x1;
    in.y1 = y1;
    in.x2 = x2;
    in.y2 = y2;
    return cogmaster_set_input(m, &in);
}

/* One frame, taking its audio as a host does. */
static void frame(cogmaster *m)
{
    static int16_t pcm[COGMASTER__AUDIO_MAX_FRAMES * 2u];
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = pcm;
    a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
}

static uint8_t neg_byte(int32_t d)
{
    return (uint8_t)(0 - d);
}

static int32_t clamp_to(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

struct seq {
    uint8_t x, y;
    uint32_t keys; /* TL and TR as the pad drove them, from the last nibble */
};

/* The four-step read of the cited sequence on one port, starting at tick t; the pad's keys are
   what it drives on every step, and the bytes are joined from the nibbles seen. */
static struct seq read_seq(cogmaster *m, uint32_t port, uint64_t *t)
{
    uint32_t n[4];
    uint32_t keys = 0u;
    static const uint64_t wait[4] = {FIRST_WAIT, FIRST_WAIT, NEXT_WAIT, NEXT_WAIT};
    for (uint32_t i = 0u; i < 4u; i++) {
        *t += wait[i];
        th(m, port, i & 1u, *t);
        uint32_t p = pins_of(m, port);
        n[i] = p & 0xFu;
        keys = p & 0x10u;
    }
    struct seq s;
    s.x = (uint8_t)(n[0] << 4 | n[1]);
    s.y = (uint8_t)(n[2] << 4 | n[3]);
    s.keys = keys;
    return s;
}

/* A fall at the first step is how a read starts, so a rise always comes first here to end it. */
static void check_keys(uint32_t pins, uint32_t held)
{
    CHECK_EQ_U64(pins & 0x10u, ~held & 0x10u);
}

/* ---- sports_ticks ---- */

/* Between the X high read and the next fall, a pad idle for fewer ticks than the limit is still
   in the same read and shows the Y high nibble; idle for the limit it starts over and shows the X
   high nibble of whatever has gathered since. */
static void test_sports_ticks(void)
{
    for (uint32_t pass = 0u; pass < 2u; pass++) {
        uint64_t gap = pass == 0u ? SPORTS_IDLE_TICKS - 1u : SPORTS_IDLE_TICKS;
        cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
        CHECK_EQ_U64(
            send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, 53, -45, 0, 0),
            COGMASTER_STATUS_OK);
        frame(m);
        uint64_t t = m->ticks + 100u;
        th(m, 0u, 0u, t);
        CHECK_EQ_U64(pins_of(m, 0u) & 0xFu, neg_byte(53) >> 4);
        t += FIRST_WAIT;
        th(m, 0u, 1u, t);
        CHECK_EQ_U64(pins_of(m, 0u) & 0xFu, neg_byte(53) & 0xFu);
        m->ports[0].acc[0] = 17; /* more has gathered since the latch */
        t += gap;
        th(m, 0u, 0u, t);
        if (pass == 0u) {
            CHECK_EQ_U64(pins_of(m, 0u) & 0xFu, neg_byte(-45) >> 4);
            CHECK_EQ_U64(m->ports[0].acc[0], 17);
        } else {
            CHECK_EQ_U64(pins_of(m, 0u) & 0xFu, neg_byte(17) >> 4);
            CHECK_EQ_U64(m->ports[0].acc[0], 0);
        }
        cogmaster_destroy(m);
    }
}

/* ---- sports_walk ---- */

static void test_sports_walk(void)
{
    static const uint32_t machines[] = {COGMASTER_MACHINE_SMS1, COGMASTER_MACHINE_SMS2};
    static const uint32_t regions[] = {COGMASTER_REGION_EXPORT, COGMASTER_REGION_JAPAN};
    for (uint32_t mi = 0u; mi < 2u; mi++) {
        for (uint32_t ri = 0u; ri < 2u; ri++) {
            for (uint32_t port = 0u; port < 2u; port++) {
                /* The direction bits of the held byte are ignored; the keys show on TL. */
                for (uint32_t held = 0u; held < 64u; held += 7u) {
                    cogmaster *m = load(machines[mi], regions[ri]);
                    uint32_t d1 = port == 0u ? COGMASTER_DEVICE_SPORTS_PAD : COGMASTER_DEVICE_PAD;
                    uint32_t d2 = port == 1u ? COGMASTER_DEVICE_SPORTS_PAD : COGMASTER_DEVICE_PAD;
                    CHECK_EQ_U64(send(m, d1, d2, port == 0u ? held : 0u, port == 1u ? held : 0u,
                                      port == 0u ? 5 : 0, port == 0u ? -3 : 0, port == 1u ? 5 : 0,
                                      port == 1u ? -3 : 0),
                                 COGMASTER_STATUS_OK);
                    frame(m);
                    uint64_t t = m->ticks + 50u;
                    struct seq s = read_seq(m, port, &t);
                    CHECK_EQ_U64(s.x, neg_byte(5));
                    CHECK_EQ_U64(s.y, neg_byte(-3));
                    check_keys(s.keys, held);
                    /* Everything has been delivered: the next read is zero. */
                    s = read_seq(m, port, &t);
                    CHECK_EQ_U64(s.x, 0);
                    CHECK_EQ_U64(s.y, 0);
                    /* Idle between reads, the pad presents $F with its keys. */
                    CHECK_EQ_U64(m->ports[port].step, 4);
                    cogmaster_destroy(m);
                }
            }
        }
    }
    /* A fresh pad that has not been read presents $F. */
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0x10u, 0x20u, 0, 0, 0, 0),
        COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_U64(pins_of(m, 0u), 0x0Fu);
    CHECK_EQ_U64(pins_of(m, 1u), 0x1Fu);
    cogmaster_destroy(m);
}

/* ---- clamp ---- */

/* The motion a pad has not been asked for waits: 127 a read either way, the rest carried over,
   across reads and across frames, and no more than 1023 kept. Expected bytes come from a running
   total of what was sent. */
static void deliver_all(cogmaster *m, int32_t *rem, uint64_t *t, uint32_t reads)
{
    for (uint32_t i = 0u; i < reads; i++) {
        int32_t dx = clamp_to(rem[0], -127, 127);
        int32_t dy = clamp_to(rem[1], -127, 127);
        struct seq s = read_seq(m, 0u, t);
        CHECK_EQ_U64(s.x, neg_byte(dx));
        CHECK_EQ_U64(s.y, neg_byte(dy));
        rem[0] -= dx;
        rem[1] -= dy;
    }
}

static void test_clamp(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    int32_t rem[2] = {300, -200};
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, rem[0], rem[1], 0, 0),
        COGMASTER_STATUS_OK);
    frame(m);
    uint64_t t = m->ticks;
    deliver_all(m, rem, &t, 1u);
    CHECK_EQ_U64(m->ports[0].acc[0], 173);
    CHECK_EQ_U64((uint16_t)m->ports[0].acc[1], (uint16_t)-73);
    /* The carry meets the next frame's motion. */
    rem[0] += 100;
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, 100, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
    t = m->ticks;
    deliver_all(m, rem, &t, 4u);
    CHECK_EQ_U64(rem[0], 0);
    CHECK_EQ_U64(rem[1], 0);
    CHECK_EQ_U64(m->ports[0].acc[0], 0);
    CHECK_EQ_U64(m->ports[0].acc[1], 0);
    cogmaster_destroy(m);

    /* More than eight full bytes are not hoarded: two frames of 700 keep 1023, which nine reads
       deliver in full and a tenth finds empty. */
    m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    for (uint32_t i = 0u; i < 2u; i++) {
        CHECK_EQ_U64(
            send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, 700, -700, 0, 0),
            COGMASTER_STATUS_OK);
        frame(m);
    }
    CHECK_EQ_U64(m->ports[0].acc[0], 1023);
    CHECK_EQ_U64((uint16_t)m->ports[0].acc[1], (uint16_t)-1023);
    int32_t cap[2] = {1023, -1023};
    t = m->ticks;
    deliver_all(m, cap, &t, 10u);
    CHECK_EQ_U64(cap[0], 0);
    CHECK_EQ_U64(cap[1], 0);
    CHECK_EQ_U64(m->ports[0].acc[0], 0);
    cogmaster_destroy(m);
}

/* ---- pending ---- */

static void test_pending(void)
{
    /* Two calls before a frame equal one call with the sum, inside the range. */
    cogmaster *a = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    cogmaster *b = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(
        send(a, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 3u, 9u, 100, -60, -7, 8),
        COGMASTER_STATUS_OK);
    CHECK_EQ_U64(
        send(a, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 5u, 11u, 27, 10, 40, -3),
        COGMASTER_STATUS_OK);
    CHECK_EQ_U64(
        send(b, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 5u, 11u, 127, -50, 33, 5),
        COGMASTER_STATUS_OK);
    CHECK(memcmp(&a->pending_input, &b->pending_input, sizeof a->pending_input) == 0);
    frame(a);
    frame(b);
    CHECK(memcmp(a->ports, b->ports, sizeof a->ports) == 0);
    CHECK_EQ_U64(a->ports[0].acc[0], 127);
    cogmaster_destroy(a);
    cogmaster_destroy(b);

    /* Two calls of 20000 give the top of the range, in either direction. */
    a = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    for (uint32_t i = 0u; i < 2u; i++) {
        CHECK_EQ_U64(send(a, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u,
                          20000, -20000, -20000, 20000),
                     COGMASTER_STATUS_OK);
    }
    CHECK_EQ_U64(a->pending_input.x1, 32767);
    CHECK_EQ_U64((uint32_t)a->pending_input.y1, (uint32_t)-32767);
    CHECK_EQ_U64((uint32_t)a->pending_input.x2, (uint32_t)-32767);
    CHECK_EQ_U64(a->pending_input.y2, 32767);

    /* A hundred thousand calls at the ends of the range never leave it; the sanitiser lanes
       catch a signed overflow. */
    uint32_t x = 1u;
    for (uint32_t i = 0u; i < 100000u; i++) {
        x = x * 1664525u + 1013904223u;
        int32_t d = (x >> 28) < 11u ? 32767 : -32767;
        int32_t e = (x >> 24 & 1u) != 0u ? 32767 : -32767;
        CHECK_EQ_U64(
            send(a, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, d, e, e, d),
            COGMASTER_STATUS_OK);
        CHECK(a->pending_input.x1 >= -32767 && a->pending_input.x1 <= 32767);
        CHECK(a->pending_input.y1 >= -32767 && a->pending_input.y1 <= 32767);
        CHECK(a->pending_input.x2 >= -32767 && a->pending_input.x2 <= 32767);
        CHECK(a->pending_input.y2 >= -32767 && a->pending_input.y2 <= 32767);
    }
    /* Whatever was pending stays within the saved state's bounds. */
    size_t size = cogmaster__state_size(a);
    uint8_t *buf = malloc(size);
    CHECK(buf != NULL);
    if (buf != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(a, buf, size), COGMASTER_STATUS_OK);
        cogmaster *c = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
        CHECK_EQ_U64(cogmaster__state_load(c, buf, size), COGMASTER_STATUS_OK);
        cogmaster_destroy(c);
        free(buf);
    }
    cogmaster_destroy(a);
}

/* ---- device_switch ---- */

static void test_device_switch(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN);
    /* A Paddle takes x and drops y. */
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PAD, 0u, 0u, 10, 9, 4, 4),
                 COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->pending_input.x1, 10);
    CHECK_EQ_U64(m->pending_input.y1, 0);
    CHECK_EQ_U64(m->pending_input.x2, 0);
    CHECK_EQ_U64(m->pending_input.y2, 0);
    /* A different device restarts the port's axes; the new one takes both. */
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_NONE, 0u, 0u, 3, 4, 5, 6),
                 COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->pending_input.x1, 3);
    CHECK_EQ_U64(m->pending_input.y1, 4);
    CHECK_EQ_U64(m->pending_input.x2, 0);
    /* The same device adds; a pad and nothing take no motion. */
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_NONE, 0u, 0u, 3, 4, 5, 6),
                 COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->pending_input.x1, 6);
    CHECK_EQ_U64(m->pending_input.y1, 8);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, 3, 4, 5, 6),
                 COGMASTER_STATUS_OK);
    CHECK_EQ_U64(
        m->pending_input.x1 | m->pending_input.y1 | m->pending_input.x2 | m->pending_input.y2, 0);

    /* The block follows the device at the next frame, on either port. */
    for (uint32_t port = 0u; port < 2u; port++) {
        uint32_t other = COGMASTER_DEVICE_PAD;
        uint32_t d[2];
        int32_t mx = 50;
        d[port] = COGMASTER_DEVICE_SPORTS_PAD;
        d[1u - port] = other;
        CHECK_EQ_U64(send(m, d[0], d[1], 0u, 0u, port == 0u ? mx : 0, 0, port == 1u ? mx : 0, 0),
                     COGMASTER_STATUS_OK);
        frame(m);
        CHECK_EQ_U64(m->ports[port].acc[0], 50);
        /* The same device keeps what is gathering. */
        CHECK_EQ_U64(send(m, d[0], d[1], 0u, 0u, port == 0u ? 7 : 0, 0, port == 1u ? 7 : 0, 0),
                     COGMASTER_STATUS_OK);
        frame(m);
        CHECK_EQ_U64(m->ports[port].acc[0], 57);
        /* Another device clears it at the frame that first uses it. */
        d[port] = COGMASTER_DEVICE_PAD;
        CHECK_EQ_U64(send(m, d[0], d[1], 0u, 0u, 9, 9, 9, 9), COGMASTER_STATUS_OK);
        frame(m);
        CHECK_EQ_U64(m->ports[port].acc[0] | m->ports[port].acc[1] | m->ports[port].step, 0);
        d[port] = COGMASTER_DEVICE_SPORTS_PAD;
        CHECK_EQ_U64(send(m, d[0], d[1], 0u, 0u, port == 0u ? 7 : 0, 0, port == 1u ? 7 : 0, 0),
                     COGMASTER_STATUS_OK);
        frame(m);
        CHECK_EQ_U64(m->ports[port].acc[0], 7);
    }
    cogmaster_destroy(m);
}

/* ---- resets ---- */

static void test_resets(void)
{
    for (uint32_t kind = 0u; kind < 2u; kind++) {
        cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
        CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 300,
                          20, 40, 50),
                     COGMASTER_STATUS_OK);
        frame(m);
        uint64_t t = m->ticks + 10u;
        th(m, 0u, 0u, t);
        t += FIRST_WAIT;
        th(m, 0u, 1u, t);
        CHECK_EQ_U64(m->ports[0].step, 2);
        CHECK(m->ports[0].edge != 0u);
        /* Motion sent just before a reset applies at the next frame, so a replay does not
           depend on whether the reset or the call came first. */
        CHECK_EQ_U64(
            send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 40, 0, 0, 0),
            COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_reset(m, kind == 0u ? COGMASTER_RESET_SOFT : COGMASTER_RESET_POWER),
                     COGMASTER_STATUS_OK);
        CHECK_EQ_U64(m->pending_input.x1, 40);
        for (uint32_t i = 0u; i < 2u; i++) {
            CHECK_EQ_U64(m->ports[i].edge | (uint64_t)(uint16_t)m->ports[i].acc[0] |
                             (uint64_t)(uint16_t)m->ports[i].acc[1] | m->ports[i].step,
                         0);
            CHECK_EQ_U64((uint8_t)m->ports[i].val[0] | (uint8_t)m->ports[i].val[1], 0);
        }
        /* The tick count restarted, so a stale edge would be in the future: a rise and the fall
           that follows it close behind it keep the phase, as they would on a fresh instance. */
        level[0] = level[1] = 1u;
        th(m, 0u, 0u, 10u);
        CHECK_EQ_U64(m->ports[0].step, 1);
        th(m, 0u, 1u, 20u);
        CHECK_EQ_U64(m->ports[0].step, 2);
        th(m, 0u, 0u, 30u);
        CHECK_EQ_U64(m->ports[0].step, 3);
        frame(m);
        CHECK_EQ_U64(m->ports[0].acc[0], 40);
        cogmaster_destroy(m);
    }

    /* A load zeroes the pending motion and the blocks. */
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 300, 20, 40, 50),
        COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 5, 5, 5, 5),
        COGMASTER_STATUS_OK);
    th(m, 0u, 0u, m->ticks + 10u);
    cogmaster_media md;
    media_for(&md, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(cogmaster_load(m, &md), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(
        m->pending_input.x1 | m->pending_input.y1 | m->pending_input.x2 | m->pending_input.y2, 0);
    uint8_t zero[sizeof m->ports];
    memset(zero, 0, sizeof zero);
    CHECK(memcmp(m->ports, zero, sizeof zero) == 0);
    cogmaster_destroy(m);
}

/* ---- paddle ---- */

static uint32_t paddle_position(cogmaster *m, uint32_t port)
{
    /* TR high shows the high nibble and TR low the low one; the clock is the tick's. */
    uint64_t lo_tick = (uint64_t)(m->ticks / PADDLE_HALF_TICKS / 2u) * 2u * PADDLE_HALF_TICKS + 1u;
    uint64_t save = m->ticks;
    m->ticks = lo_tick;
    uint32_t lo = pins_of(m, port) & 0xFu;
    CHECK_EQ_U64(tr_of(m, port), 0);
    m->ticks = lo_tick + PADDLE_HALF_TICKS;
    uint32_t hi = pins_of(m, port) & 0xFu;
    CHECK_EQ_U64(tr_of(m, port), 1);
    m->ticks = save;
    return hi << 4 | lo;
}

static void test_paddle(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PADDLE, 0u, 0u, 0, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
    /* The centre after a load is 128 on both ports. */
    CHECK_EQ_U64(paddle_position(m, 0u), 128);
    CHECK_EQ_U64(paddle_position(m, 1u), 128);

    /* The clock alternates each half period, on both ports, with no state: the level is the
       parity of the half period the tick is in, on both sides of every boundary. */
    uint64_t now = m->ticks;
    for (uint64_t k = 0u; k < 12u; k++) {
        m->ticks = k * PADDLE_HALF_TICKS;
        CHECK_EQ_U64(tr_of(m, 0u), k & 1u);
        CHECK_EQ_U64(tr_of(m, 1u), k & 1u);
        m->ticks = (k + 1u) * PADDLE_HALF_TICKS - 1u;
        CHECK_EQ_U64(tr_of(m, 0u), k & 1u);
        CHECK_EQ_U64(tr_of(m, 1u), k & 1u);
    }
    m->ticks = now;

    /* Motion moves the position with +x larger, saturates at both ends, and y is ignored. */
    static const struct {
        int32_t dx, dy;
        uint32_t want;
    } steps[] = {{5, 400, 133}, {-8, -9, 125}, {127, 0, 252}, {1000, 0, 255},
                 {-300, 0, 0},  {-5, 0, 0},    {200, 0, 200}, {0, 0, 200}};
    for (size_t i = 0; i < sizeof steps / sizeof steps[0]; i++) {
        CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PADDLE, 0u, 0u, steps[i].dx,
                          steps[i].dy, steps[i].dx, steps[i].dy),
                     COGMASTER_STATUS_OK);
        frame(m);
        CHECK_EQ_U64(paddle_position(m, 0u), steps[i].want);
        CHECK_EQ_U64(paddle_position(m, 1u), steps[i].want);
    }
    /* The button is TL, active low, and the second paddle's own. */
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PADDLE, 0x10u, 0u, 0, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_U64(pins_of(m, 0u) & 0x10u, 0);
    CHECK_EQ_U64(pins_of(m, 1u) & 0x10u, 0x10u);
    /* A console TR output still replaces the clock's level on its pin (a Japanese console reads
       an output as low), and the nibble on D0-D3 still follows the clock. */
    uint64_t keep = m->ticks;
    m->ticks = PADDLE_HALF_TICKS;
    CHECK_EQ_U64(tr_of(m, 0u), 1);
    cogmaster__io_out(m, 0x3Fu, (uint8_t)(CTRL_BASE & ~1u) | 0xB0u);
    CHECK_EQ_U64(tr_of(m, 0u), 0);
    CHECK_EQ_U64(pins_of(m, 0u) & 0xFu, steps[sizeof steps / sizeof steps[0] - 1u].want >> 4);
    m->ticks = keep;
    cogmaster_destroy(m);
}

/* ---- profiles ---- */

/* The reference model of one pad on one port: the cited sequence, written from the protocol. */
struct ref {
    uint32_t step, wire;
    uint64_t edge;
    int32_t acc[2], val[2];
};

static void ref_wire(struct ref *r, uint32_t wire, uint64_t tick)
{
    if (wire == r->wire) {
        return;
    }
    r->wire = wire;
    if (wire == 0u) {
        if (r->step == 2u && tick - r->edge < SPORTS_IDLE_TICKS) {
            r->step = 3u;
            return;
        }
        for (uint32_t i = 0u; i < 2u; i++) {
            int32_t d = clamp_to(r->acc[i], -127, 127);
            r->acc[i] -= d;
            r->val[i] = -d;
        }
        r->step = 1u;
        return;
    }
    r->step += r->step == 1u || r->step == 3u ? 1u : 0u;
    r->edge = tick;
}

static uint32_t ref_nibble(const struct ref *r)
{
    static const uint32_t shift[5] = {0u, 4u, 0u, 4u, 0u};
    if (r->step == 0u) {
        return 0xFu;
    }
    return ((uint32_t)(uint8_t)r->val[r->step > 2u ? 1u : 0u] >> shift[r->step]) & 0xFu;
}

#define PORTS_BYTES (2u * sizeof(struct cogmaster__port))

static void run_script(uint32_t machine, uint32_t region, uint8_t *ports_out)
{
    static const uint64_t gaps[] = {0u,
                                    1u,
                                    100u,
                                    NEXT_WAIT,
                                    FIRST_WAIT,
                                    SPORTS_IDLE_TICKS - 1u,
                                    SPORTS_IDLE_TICKS,
                                    SPORTS_IDLE_TICKS + 1u,
                                    5000u};
    cogmaster *m = load(machine, region);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, HELD_1, HELD_2,
                      300, -300, -77, 190),
                 COGMASTER_STATUS_OK);
    frame(m);
    struct ref r[2];
    memset(r, 0, sizeof r);
    r[0].wire = r[1].wire = 1u;
    r[0].acc[0] = 300;
    r[0].acc[1] = -300;
    r[1].acc[0] = -77;
    r[1].acc[1] = 190;
    uint64_t t = m->ticks;
    uint32_t x = 7u;
    for (uint32_t i = 0u; i < 400u; i++) {
        x = x * 1664525u + 1013904223u;
        uint32_t ctrl = x >> 24;
        t += gaps[(x >> 8) % (sizeof gaps / sizeof gaps[0])];
        m->ticks = t;
        cogmaster__io_out(m, 0x3Fu, (uint8_t)ctrl);
        /* The wire is the level written, or high when the pin is an input, on every region. */
        ref_wire(&r[0], (ctrl & 2u) != 0u ? 1u : (ctrl >> 5) & 1u, t);
        ref_wire(&r[1], (ctrl & 8u) != 0u ? 1u : (ctrl >> 7) & 1u, t);
        CHECK_EQ_U64(pins_of(m, 0u), ref_nibble(&r[0]) | (~HELD_1 & 0x10u));
        CHECK_EQ_U64(pins_of(m, 1u), ref_nibble(&r[1]) | (~HELD_2 & 0x10u));
        CHECK_EQ_U64(m->ports[0].step, r[0].step);
        CHECK_EQ_U64(m->ports[1].step, r[1].step);
    }
    memcpy(ports_out, m->ports, sizeof m->ports);
    cogmaster_destroy(m);
}

static void test_profiles(void)
{
    /* The same wire gives the same bytes on a Japanese console, whose readback of an output TH is
       low, and an export one: the pad is driven by the wire, not by the echo. */
    uint8_t blocks[4][PORTS_BYTES];
    static const uint32_t machines[] = {COGMASTER_MACHINE_SMS1, COGMASTER_MACHINE_SMS2};
    static const uint32_t regions[] = {COGMASTER_REGION_EXPORT, COGMASTER_REGION_JAPAN};
    for (uint32_t mi = 0u; mi < 2u; mi++) {
        for (uint32_t ri = 0u; ri < 2u; ri++) {
            run_script(machines[mi], regions[ri], blocks[mi * 2u + ri]);
        }
    }
    for (uint32_t i = 1u; i < 4u; i++) {
        CHECK(memcmp(blocks[0], blocks[i], sizeof blocks[0]) == 0);
    }

    /* A device the profile cannot take is refused whole, on either port, with nothing changed. */
    static const struct {
        uint32_t machine, region, device;
        int refused;
    } cases[] = {
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_DEVICE_PADDLE, 1},
        {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT, COGMASTER_DEVICE_PADDLE, 1},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_JAPAN, COGMASTER_DEVICE_PADDLE, 1},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_DEVICE_SPORTS_PAD, 1},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_JAPAN, COGMASTER_DEVICE_SPORTS_PAD, 1},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_DEVICE_PADDLE, 0},
        {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_JAPAN, COGMASTER_DEVICE_PADDLE, 0},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_DEVICE_SPORTS_PAD, 0},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_DEVICE_NONE, 0},
    };
    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        for (uint32_t port = 0u; port < 2u; port++) {
            cogmaster *m = load(cases[c].machine, cases[c].region);
            uint8_t *snap = malloc(sizeof *m);
            CHECK(snap != NULL);
            if (snap == NULL) {
                exit(1);
            }
            memcpy(snap, m, sizeof *m);
            uint32_t d1 = port == 0u ? cases[c].device : COGMASTER_DEVICE_PAD;
            uint32_t d2 = port == 1u ? cases[c].device : COGMASTER_DEVICE_PAD;
            cogmaster_status st = send(m, d1, d2, 0x15u, 0x2Au, 9, 9, 9, 9);
            if (cases[c].refused) {
                CHECK_EQ_U64(st, COGMASTER_STATUS_ERR_PROFILE_REJECTED);
                CHECK(memcmp(snap, m, sizeof *m) == 0);
            } else {
                CHECK_EQ_U64(st, COGMASTER_STATUS_OK);
            }
            free(snap);
            cogmaster_destroy(m);
        }
    }
}

/* ---- hammer ---- */

static void test_hammer(void)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 500, 500,
                      -500, -500),
                 COGMASTER_STATUS_OK);
    frame(m);
    uint64_t t = m->ticks + 10u;
    th(m, 0u, 0u, t);
    th(m, 0u, 1u, t + FIRST_WAIT);
    struct cogmaster__port before[2];
    memcpy(before, m->ports, sizeof before);
    /* Reads, and a write that changes no wire, at one tick: nothing moves. */
    for (uint32_t i = 0u; i < 1000000u; i++) {
        (void)cogmaster__io_in(m, 0xDCu);
        (void)cogmaster__io_in(m, 0xDDu);
        if ((i & 255u) == 0u) {
            drive_th(m, m->ticks);
        }
    }
    CHECK(memcmp(before, m->ports, sizeof before) == 0);

    /* A million toggles of both lines at one tick stay inside the model's bounds. */
    for (uint32_t i = 0u; i < 1000000u; i++) {
        level[0] = (i & 1u) != 0u ? 1u : 0u;
        level[1] = (i & 2u) != 0u ? 0u : 1u;
        drive_th(m, m->ticks);
    }
    for (uint32_t p = 0u; p < 2u; p++) {
        CHECK(m->ports[p].step <= 4u);
        CHECK(m->ports[p].acc[0] >= -1023 && m->ports[p].acc[0] <= 1023);
        CHECK(m->ports[p].acc[1] >= -1023 && m->ports[p].acc[1] <= 1023);
        CHECK(m->ports[p].val[0] >= -127 && m->ports[p].val[1] >= -127);
    }
    size_t size = cogmaster__state_size(m);
    uint8_t *buf = malloc(size);
    CHECK(buf != NULL);
    if (buf != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(m, buf, size), COGMASTER_STATUS_OK);
        cogmaster *c = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
        CHECK_EQ_U64(cogmaster__state_load(c, buf, size), COGMASTER_STATUS_OK);
        cogmaster_destroy(c);
        free(buf);
    }
    cogmaster_destroy(m);
}

/* ---- roundtrip ---- */

static void round_trip(cogmaster *m, uint32_t machine, uint32_t region)
{
    size_t size = cogmaster__state_size(m);
    uint8_t *a = malloc(size);
    uint8_t *b = malloc(size);
    CHECK(a != NULL && b != NULL);
    if (a == NULL || b == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(m, a, size), COGMASTER_STATUS_OK);
    cogmaster *other = load(machine, region);
    CHECK_EQ_U64(cogmaster__state_load(other, a, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_save(other, b, size), COGMASTER_STATUS_OK);
    CHECK(memcmp(a, b, size) == 0);
    CHECK(memcmp(m->ports, other->ports, sizeof m->ports) == 0);
    /* Both go on the same way. */
    level[0] = level[1] = 1u;
    for (uint32_t i = 0u; i < 3u; i++) {
        uint64_t t = m->ticks + 100u;
        for (uint32_t k = 0u; k < 2u; k++) {
            level[0] = (i + k) & 1u;
            drive_th(m, t + k * NEXT_WAIT);
            drive_th(other, t + k * NEXT_WAIT);
            CHECK_EQ_U64(pins_of(m, 0u), pins_of(other, 0u));
        }
    }
    CHECK_EQ_U64(cogmaster__state_save(m, a, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_save(other, b, size), COGMASTER_STATUS_OK);
    CHECK(memcmp(a, b, size) == 0);
    cogmaster_destroy(other);
    free(a);
    free(b);
}

static void test_roundtrip(void)
{
    /* Step 1 right after the first fall of a reset instance: the stored edge is 0. */
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 20, -9, 0, 0),
        COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    th(m, 0u, 0u, 0u);
    CHECK_EQ_U64(m->ports[0].step, 1);
    CHECK_EQ_U64(m->ports[0].edge, 0);
    round_trip(m, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    cogmaster_destroy(m);

    /* Step 2 with motion left over, a latched byte and motion still pending. */
    m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0x13u, 0x25u,
                      300, -250, 40, 50),
                 COGMASTER_STATUS_OK);
    frame(m);
    uint64_t t = m->ticks + 20u;
    th(m, 0u, 0u, t);
    th(m, 0u, 1u, t + FIRST_WAIT);
    CHECK_EQ_U64(m->ports[0].step, 2);
    CHECK(m->ports[0].acc[0] != 0 && m->ports[0].val[0] != 0);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0x13u, 0x25u, 7,
                      -8, 9, 10),
                 COGMASTER_STATUS_OK);
    CHECK(m->pending_input.x1 != 0);
    round_trip(m, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    cogmaster_destroy(m);

    /* A Paddle off centre, with motion pending. */
    m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PADDLE, 0u, 0u, 70, 0, -30, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PADDLE, 0u, 0u, 5, 0, 6, 0),
                 COGMASTER_STATUS_OK);
    round_trip(m, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN);
    cogmaster_destroy(m);
}

/* ---- replay ---- */

/* An interrupt handler that, once a frame, reads the pad through the whole sequence, folds its
   four nibbles into a colour and writes it to the backdrop's palette entry, so the picture of
   each frame depends on the motion the frame delivered. */
static void pad_image(void)
{
    static const uint8_t boot[] = {
        0xF3u,                      /* di */
        0xEDu, 0x56u,               /* im 1 */
        0x31u, 0xF0u, 0xDFu,        /* ld sp,$DFF0 */
        0x3Eu, 0x20u, 0xD3u, 0xBFu, /* ld a,$20 : out ($BF),a */
        0x3Eu, 0x81u, 0xD3u, 0xBFu, /* ld a,$81 : out ($BF),a  (frame interrupt on) */
        0xFBu,                      /* ei */
        0x76u,                      /* halt */
        0x18u, 0xFDu,               /* jr halt */
    };
    static const uint8_t handler[] = {
        0xF5u, 0xC5u, 0xD5u,               /* push af, bc, de */
        0xDBu, 0xBFu,                      /* in a,($BF): acknowledge */
        0x3Eu, 0x05u, 0xD3u, 0x3Fu,        /* TH low */
        0xDBu, 0xDCu, 0xE6u, 0x0Fu, 0x47u, /* b = X high nibble */
        0x3Eu, 0x25u, 0xD3u, 0x3Fu,        /* TH high */
        0xDBu, 0xDCu, 0xE6u, 0x0Fu, 0x4Fu, /* c = X low nibble */
        0x3Eu, 0x05u, 0xD3u, 0x3Fu,        /* TH low */
        0xDBu, 0xDCu, 0xE6u, 0x0Fu, 0x57u, /* d = Y high nibble */
        0x3Eu, 0x25u, 0xD3u, 0x3Fu,        /* TH high */
        0xDBu, 0xDCu, 0xE6u, 0x0Fu,        /* a = Y low nibble */
        0x5Fu,                             /* e = a */
        0x78u, 0x87u, 0x87u, 0xA9u,        /* a = (b * 4) xor c */
        0x83u,                             /* add a,e */
        0x5Fu, 0x7Au, 0x87u, 0x83u,        /* a = d * 2 + a */
        0xE6u, 0x3Fu,                      /* and $3F */
        0x5Fu,                             /* e = colour */
        0x3Eu, 0x10u, 0xD3u, 0xBFu,        /* CRAM address 16 ... */
        0x3Eu, 0xC0u, 0xD3u, 0xBFu,        /* ... for writing */
        0x7Bu, 0xD3u, 0xBEu,               /* out ($BE),colour */
        0xD1u, 0xC1u, 0xF1u, 0xFBu,        /* pop de, bc, af : ei */
        0xEDu, 0x4Du,                      /* reti */
    };
    memset(image, 0, sizeof image);
    memcpy(image, boot, sizeof boot);
    memcpy(image + 0x38, handler, sizeof handler);
}

#define REPLAY_FRAMES 120u

struct call {
    int32_t x, y;
};

static uint64_t frame_hash(const cogmaster *m)
{
    static uint16_t pixels[COGMASTER__FIELD_W * COGMASTER__FIELD_MAX_H];
    cogmaster_frame f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    f.pixels = pixels;
    f.pitch_bytes = COGMASTER__FIELD_W * 2u;
    f.flags = COGMASTER_FRAME_FULL_FIELD;
    CHECK_EQ_U64(cogmaster_get_frame(m, &f), COGMASTER_STATUS_OK);
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < (size_t)f.w * f.h; i++) {
        h = (h ^ (pixels[i] & 0xFFu)) * 1099511628211ull;
        h = (h ^ (pixels[i] >> 8)) * 1099511628211ull;
    }
    return h;
}

/* Frames from..to-1 of a script, each with its own call; folds each frame's hash into h. */
static uint64_t play(cogmaster *m, const struct call *c, uint32_t from, uint32_t to, uint64_t h)
{
    for (uint32_t i = from; i < to; i++) {
        CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_PAD, 0u, 0u, c[i].x,
                          c[i].y, 0, 0),
                     COGMASTER_STATUS_OK);
        frame(m);
        h = (h ^ frame_hash(m)) * 1099511628211ull;
    }
    return h;
}

struct run {
    uint64_t hash;
    uint8_t *state;
    size_t size;
};

static struct run finish(cogmaster *m, uint64_t h)
{
    struct run r;
    r.hash = h;
    r.size = cogmaster__state_size(m);
    r.state = malloc(r.size);
    CHECK(r.state != NULL);
    if (r.state == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(m, r.state, r.size), COGMASTER_STATUS_OK);
    return r;
}

static struct run run_all(const struct call *c, uint32_t restore_at)
{
    cogmaster *m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
    uint64_t h = 14695981039346656037ull;
    if (restore_at == 0u) {
        h = play(m, c, 0u, REPLAY_FRAMES, h);
    } else {
        h = play(m, c, 0u, restore_at, h);
        size_t size = cogmaster__state_size(m);
        uint8_t *buf = malloc(size);
        CHECK(buf != NULL);
        if (buf == NULL) {
            exit(1);
        }
        CHECK_EQ_U64(cogmaster__state_save(m, buf, size), COGMASTER_STATUS_OK);
        cogmaster_destroy(m);
        m = load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT);
        CHECK_EQ_U64(cogmaster__state_load(m, buf, size), COGMASTER_STATUS_OK);
        free(buf);
        h = play(m, c, restore_at, REPLAY_FRAMES, h);
    }
    struct run r = finish(m, h);
    cogmaster_destroy(m);
    return r;
}

static void test_replay(void)
{
    pad_image();
    struct call script[REPLAY_FRAMES];
    struct call recorded[REPLAY_FRAMES];
    struct call chunked[REPLAY_FRAMES];
    struct call still[REPLAY_FRAMES];
    uint32_t x = 99u;
    for (uint32_t i = 0u; i < REPLAY_FRAMES; i++) {
        x = x * 1664525u + 1013904223u;
        script[i].x = (int32_t)((x >> 20) % 401u) - 200;
        script[i].y = (int32_t)((x >> 8) % 201u) - 100;
        still[i].x = still[i].y = 0;
    }
    /* The same total motion, cut into other per-frame pieces: a frame takes its neighbour's. */
    for (uint32_t i = 0u; i < REPLAY_FRAMES; i += 2u) {
        chunked[i].x = script[i].x + script[i + 1u].x;
        chunked[i].y = script[i].y + script[i + 1u].y;
        chunked[i + 1u].x = chunked[i + 1u].y = 0;
    }

    struct run first = run_all(script, 0u);
    /* The per-call deltas of that run, as a recording holds them, replayed. */
    memcpy(recorded, script, sizeof recorded);
    struct run again = run_all(recorded, 0u);
    struct run resumed = run_all(recorded, REPLAY_FRAMES / 2u);
    CHECK_EQ_U64(first.hash, again.hash);
    CHECK_EQ_U64(first.hash, resumed.hash);
    CHECK_EQ_U64(first.size, again.size);
    CHECK_EQ_U64(first.size, resumed.size);
    CHECK(first.size == again.size && memcmp(first.state, again.state, first.size) == 0);
    CHECK(first.size == resumed.size && memcmp(first.state, resumed.state, first.size) == 0);

    /* The motion reaches the picture, and how it is cut matters. */
    struct run quiet = run_all(still, 0u);
    struct run cut = run_all(chunked, 0u);
    CHECK(first.hash != quiet.hash);
    CHECK(first.hash != cut.hash);
    CHECK(quiet.hash != cut.hash);
    free(first.state);
    free(again.state);
    free(resumed.state);
    free(quiet.state);
    free(cut.state);
}

/* ---- record ---- */

/* cogmaster-test-ports-devices record IMAGE DEVICE REGION SCRIPT FRAMES: the library's side of a
   replay. It runs the image on the public API with DEVICE on port 1 and REGION, sends a fixed
   pseudo-random motion each frame, writes each call's axes as the runner's 75-byte script line, and
   prints the hash line the runner prints for the last frame. The motion stays within 100 counts a
   frame, which the pad delivers whole, so a change of one count in the last frame is a change in
   the picture. Half way through, the state is saved and loaded into a second instance; both finish
   the script and must agree in picture and in state bytes. */
#define RECORD_IMAGE_MAX (1u << 20)
#define RECORD_SPAN 100u

static uint64_t record_next(uint64_t *seed)
{
    uint64_t z = (*seed += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint32_t record_device(const char *name)
{
    if (strcmp(name, "sports") == 0) {
        return COGMASTER_DEVICE_SPORTS_PAD;
    }
    if (strcmp(name, "paddle") == 0) {
        return COGMASTER_DEVICE_PADDLE;
    }
    if (strcmp(name, "pad") != 0) {
        exit(2);
    }
    return COGMASTER_DEVICE_PAD;
}

/* The region words of the runner's --region, which sets the TV standard with the region. */
static void record_region(const char *name, cogmaster_media *md)
{
    md->tv = COGMASTER_TV_NTSC;
    md->region = COGMASTER_REGION_EXPORT;
    if (strcmp(name, "ntsc-j") == 0) {
        md->region = COGMASTER_REGION_JAPAN;
    } else if (strcmp(name, "pal") == 0) {
        md->tv = COGMASTER_TV_PAL;
    } else if (strcmp(name, "ntsc-u") != 0) {
        exit(2);
    }
}

static cogmaster *record_instance(const cogmaster_media *md)
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
    CHECK_EQ_U64(cogmaster_load(m, md), COGMASTER_STATUS_OK);
    return m;
}

/* The runner's hash of the frame just run: the crop, each pixel low byte first. */
static void record_hash(const cogmaster *m, char hex[65])
{
    static uint16_t pixels[COGMASTER__FIELD_W * COGMASTER__FIELD_MAX_H];
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    cogmaster_frame f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    f.pixels = pixels;
    f.pitch_bytes = info.crop_w * 2u;
    CHECK_EQ_U64(cogmaster_get_frame(m, &f), COGMASTER_STATUS_OK);
    cogmaster__sha256 sha;
    cogmaster__sha256_init(&sha);
    for (uint32_t y = 0u; y < f.h; y++) {
        uint8_t row[2u * COGMASTER__FIELD_W];
        for (uint32_t x = 0u; x < f.w; x++) {
            uint16_t px = pixels[(size_t)y * info.crop_w + x];
            row[2u * x] = (uint8_t)(px & 0xFFu);
            row[2u * x + 1u] = (uint8_t)(px >> 8);
        }
        cogmaster__sha256_update(&sha, row, (size_t)f.w * 2u);
    }
    uint8_t digest[32];
    cogmaster__sha256_final(&sha, digest);
    cogmaster__sha256_hex(digest, hex);
}

static void record_step(cogmaster *m, uint32_t device, int32_t x, int32_t y)
{
    CHECK_EQ_U64(send(m, device, COGMASTER_DEVICE_PAD, 0u, 0u, x, y, 0, 0), COGMASTER_STATUS_OK);
    frame(m);
}

static void test_record(char **argv)
{
    static uint8_t loaded[RECORD_IMAGE_MAX];
    FILE *in = fopen(argv[2], "rb");
    if (in == NULL) {
        exit(1);
    }
    size_t size = fread(loaded, 1u, sizeof loaded, in);
    CHECK(fclose(in) == 0);
    uint32_t device = record_device(argv[3]);
    uint32_t frames = (uint32_t)strtoul(argv[6], NULL, 10);
    if (size == 0u || size == sizeof loaded || frames < 2u || frames > 10000u) {
        exit(2);
    }
    cogmaster_media md;
    memset(&md, 0, sizeof md);
    md.size = (uint32_t)sizeof md;
    md.image = loaded;
    md.image_size = size;
    record_region(argv[4], &md);
    cogmaster_cart_info cart;
    memset(&cart, 0, sizeof cart);
    cart.size = (uint32_t)sizeof cart;
    (void)cogmaster_probe(loaded, size, &cart);
    md.machine =
        cart.machine_guess != COGMASTER_MACHINE_AUTO ? cart.machine_guess : COGMASTER_MACHINE_SMS2;
    FILE *out = fopen(argv[5], "wb");
    if (out == NULL) {
        exit(1);
    }
    cogmaster *m = record_instance(&md);
    cogmaster *second = NULL;
    uint64_t seed = 0x5EEDull;
    for (uint32_t i = 0u; i < frames; i++) {
        uint64_t r = record_next(&seed);
        int32_t x =
            device == COGMASTER_DEVICE_PAD ? 0 : (int32_t)(r % (2u * RECORD_SPAN + 1u)) - 100;
        int32_t y = device == COGMASTER_DEVICE_PAD
                        ? 0
                        : (int32_t)((r >> 32) % (2u * RECORD_SPAN + 1u)) - 100;
        fprintf(out, "p1=00 p2=00 pause=0 reset=0 start=0 x1=%+06d y1=%+06d x2=+00000 y2=+00000\n",
                (int)x, (int)y);
        if (i == frames / 2u) {
            size_t state_size = cogmaster__state_size(m);
            uint8_t *state = malloc(state_size);
            CHECK(state != NULL);
            if (state == NULL) {
                exit(1);
            }
            CHECK_EQ_U64(cogmaster__state_save(m, state, state_size), COGMASTER_STATUS_OK);
            second = record_instance(&md);
            CHECK_EQ_U64(cogmaster__state_load(second, state, state_size), COGMASTER_STATUS_OK);
            free(state);
        }
        record_step(m, device, x, y);
        if (second != NULL) {
            record_step(second, device, x, y);
        }
    }
    CHECK(fclose(out) == 0);
    if (second == NULL) {
        exit(1);
    }
    char hex[65];
    char hex_second[65];
    record_hash(m, hex);
    record_hash(second, hex_second);
    CHECK(strcmp(hex, hex_second) == 0);
    size_t state_size = cogmaster__state_size(m);
    uint8_t *a = malloc(state_size);
    uint8_t *b = malloc(state_size);
    CHECK(a != NULL && b != NULL);
    if (a == NULL || b == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(m, a, state_size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_size(second), state_size);
    CHECK_EQ_U64(cogmaster__state_save(second, b, state_size), COGMASTER_STATUS_OK);
    CHECK(memcmp(a, b, state_size) == 0);
    free(a);
    free(b);
    cogmaster_destroy(second);
    cogmaster_destroy(m);
    printf("frame %u sha256 %s\n", (unsigned)frames, hex);
}

int main(int argc, char **argv)
{
    if (argc == 7 && strcmp(argv[1], "record") == 0) {
        test_record(argv);
        CHECK_DONE();
    }
    if (argc != 2) {
        return 2;
    }
    halt_image();
    if (strcmp(argv[1], "sports_ticks") == 0) {
        test_sports_ticks();
    } else if (strcmp(argv[1], "sports_walk") == 0) {
        test_sports_walk();
    } else if (strcmp(argv[1], "clamp") == 0) {
        test_clamp();
    } else if (strcmp(argv[1], "pending") == 0) {
        test_pending();
    } else if (strcmp(argv[1], "device_switch") == 0) {
        test_device_switch();
    } else if (strcmp(argv[1], "resets") == 0) {
        test_resets();
    } else if (strcmp(argv[1], "paddle") == 0) {
        test_paddle();
    } else if (strcmp(argv[1], "profiles") == 0) {
        test_profiles();
    } else if (strcmp(argv[1], "hammer") == 0) {
        test_hammer();
    } else if (strcmp(argv[1], "roundtrip") == 0) {
        test_roundtrip();
    } else if (strcmp(argv[1], "replay") == 0) {
        test_replay();
    } else {
        return 2;
    }
    CHECK_DONE();
}
