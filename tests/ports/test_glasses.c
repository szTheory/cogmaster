/* The 3-D glasses eye, written to $FFF8-$FFFB and reported with each frame:
   cogmaster-test-ports-glasses <case>. A one-instruction program writes the
   eye at a chosen tick of the frame window (the instruction's write lands 18
   ticks after it starts: a 4 T-state fetch and the write's first two T-states,
   three ticks each, Zilog UM0080 "Z80 CPU Timing"), and the reported eye is
   compared with a closed form written here from line and tick numbers: the
   window starts at line 192 tick 607 and the picture it ends in is the one
   whose top 96 active lines count. The reference shares no code or constant
   expression with src/io.c, and the snapshot reference is the rule a naive
   host would use, kept only to show that the two disagree. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define LINE_TICKS 684u
#define WINDOW_LINE 192u /* the frame window starts at line 192 tick 607 (SMS-SYSTEM section 2) */
#define WINDOW_TICK 607u
#define TOP_LINES 96u /* the top active lines that count for the picture */
#define WRITE_AFTER 18u
#define OP_LD_HL_A 0x77u
#define OP_HALT 0x76u
#define OP_IN_A_N 0xDBu
#define EYE_NONE COGMASTER_GLASSES_NONE
#define EYE_RIGHT COGMASTER_GLASSES_RIGHT
#define EYE_LEFT COGMASTER_GLASSES_LEFT
#define MEM_DEFAULT 0xABu
#define MEM_CART_OFF 0x40u
#define MEM_RAM_OFF 0x10u

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

/* A new instance of the profile running the program held in image. */
static cogmaster *load(uint32_t machine, uint32_t region, uint32_t tv)
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
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    media.region = region;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

static void program(uint8_t op0, uint8_t op1)
{
    memset(image, OP_HALT, sizeof image);
    image[0] = op0;
    image[1] = op1;
}

static uint32_t run(cogmaster *m)
{
    cogmaster_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
    return r.glasses;
}

static uint32_t lines_of(uint32_t tv)
{
    return tv == COGMASTER_TV_PAL ? COGMASTER__LINES_PAL : COGMASTER__LINES_NTSC;
}

static uint64_t frame_of(uint32_t tv)
{
    return tv == COGMASTER_TV_PAL ? COGMASTER__TICKS_PER_FRAME_PAL
                                  : COGMASTER__TICKS_PER_FRAME_NTSC;
}

/* The window tick at which the beam reaches line `line` of the picture the
   window ends in, tick `tick` of that line. */
static uint64_t window_tick(uint32_t tv, uint32_t line, uint32_t tick)
{
    uint64_t absolute = ((uint64_t)lines_of(tv) + line) * LINE_TICKS + tick;
    return absolute - ((uint64_t)WINDOW_LINE * LINE_TICKS + WINDOW_TICK);
}

/* The window tick of the first dot that belongs to the next picture. */
static uint64_t cut_tick(uint32_t tv)
{
    return window_tick(tv, TOP_LINES, 0u);
}

/* The eye a write at window tick w makes this frame report, given the eye in
   force before it. */
static uint32_t rule_report(uint32_t tv, uint32_t before, uint32_t eye, uint64_t w)
{
    return w < cut_tick(tv) ? eye : before;
}

/* What a host that read the eye at the end of the window would report. */
static uint32_t snapshot_report(uint32_t before, uint32_t eye)
{
    (void)before;
    return eye;
}

static uint8_t bit0(uint32_t eye)
{
    return eye == EYE_LEFT ? 1u : 0u;
}

struct play {
    uint32_t first;  /* what the frame holding the write reports */
    uint32_t second; /* what the next frame reports */
    uint32_t held;   /* the eye in force after both */
    uint32_t height; /* the completed picture's height */
};

/* One frame in which a single store of `eye` to $FFF8 lands at window tick w,
   after an earlier eye `before` (EYE_NONE for none), then one quiet frame. A
   write at tick 0 is made through the bus before the frame starts, as no
   instruction can land there. */
static struct play play(uint32_t machine, uint32_t tv, int tall, uint32_t before, uint32_t eye,
                        uint64_t w)
{
    program(OP_LD_HL_A, OP_HALT);
    cogmaster *m = load(machine, COGMASTER_REGION_EXPORT, tv);
    if (tall) {
        /* Mode 4 with M2 and M1: the 224-line picture. */
        cogmaster__bus_out(m, 0xBFu, 0x06u);
        cogmaster__bus_out(m, 0xBFu, 0x80u);
        cogmaster__bus_out(m, 0xBFu, 0x10u);
        cogmaster__bus_out(m, 0xBFu, 0x81u);
    }
    if (before != EYE_NONE) {
        cogmaster__bus_write(m, 0xFFF8u, bit0(before));
    }
    m->z80.reg[COGMASTER__Z80_H] = 0xFFu;
    m->z80.reg[COGMASTER__Z80_L] = 0xF8u;
    m->z80.reg[COGMASTER__Z80_A] = bit0(eye);
    if (w >= WRITE_AFTER) {
        m->ticks = w - WRITE_AFTER;
    } else {
        CHECK_EQ_U64(w, 0);
        cogmaster__bus_write(m, 0xFFF8u, bit0(eye));
        CHECK_EQ_U64(m->io.glasses_frame, eye);
        m->z80.pc = 1u;
    }
    struct play p;
    p.first = run(m);
    p.second = run(m);
    p.held = m->io.glasses;
    p.height = m->completed.height;
    CHECK_EQ_U64(m->frame, 2);
    cogmaster_destroy(m);
    return p;
}

struct setup {
    uint32_t machine;
    uint32_t tv;
    int tall;
};

static const struct setup setups[] = {
    {COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, 0},
    {COGMASTER_MACHINE_SMS2, COGMASTER_TV_PAL, 0},
    {COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC, 1},
    {COGMASTER_MACHINE_SMS1, COGMASTER_TV_NTSC, 0},
};
#define SETUPS (sizeof setups / sizeof setups[0])

static const uint32_t eyes[] = {EYE_RIGHT, EYE_LEFT};

/* The window ticks of the writes: the window start, active lines 20 and 150,
   the last tick before the cut and the cut, and line 192 tick 606, the last
   tick of the window. */
static size_t writes_for(uint32_t tv, uint64_t out[8])
{
    size_t n = 0;
    out[n++] = 0u;
    out[n++] = window_tick(tv, 20u, 100u);
    out[n++] = window_tick(tv, 150u, 100u);
    out[n++] = cut_tick(tv) - 1u;
    out[n++] = cut_tick(tv);
    out[n++] = window_tick(tv, WINDOW_LINE, WINDOW_TICK - 1u);
    return n;
}

/* The closed form agrees with the window it describes: line 192 tick 606 is
   the last tick of a frame, and the cut sits where the macro says it does. */
static void check_geometry(uint32_t tv)
{
    CHECK_EQ_U64(window_tick(tv, WINDOW_LINE, WINDOW_TICK - 1u), frame_of(tv) - 1u);
    CHECK_EQ_U64(cut_tick(tv), COGMASTER__GLASSES_CUT_TICKS(lines_of(tv)));
}

/* The reported eye is the rule's for every write position, eye pair, profile
   timing and picture height; the following frame reports the eye in force. */
static void test_rule(void)
{
    uint64_t at[8];
    for (size_t s = 0; s < SETUPS; s++) {
        check_geometry(setups[s].tv);
        size_t n = writes_for(setups[s].tv, at);
        for (size_t i = 0; i < n; i++) {
            for (size_t b = 0; b < 3u; b++) {
                uint32_t before = b == 0u ? EYE_NONE : eyes[b - 1u];
                for (size_t e = 0; e < 2u; e++) {
                    struct play p = play(setups[s].machine, setups[s].tv, setups[s].tall, before,
                                         eyes[e], at[i]);
                    CHECK_EQ_U64(p.first, rule_report(setups[s].tv, before, eyes[e], at[i]));
                    CHECK_EQ_U64(p.second, eyes[e]);
                    CHECK_EQ_U64(p.held, eyes[e]);
                    CHECK_EQ_U64(p.height, setups[s].tall ? 224u : 192u);
                }
            }
        }
    }
}

/* The control: an end-of-window snapshot reports the late writes in this
   frame, so it disagrees with the core on exactly those, and the core defers
   them to the next picture. A rule that is secretly a snapshot fails here. */
static void test_control(void)
{
    uint64_t at[8];
    uint32_t disagree = 0;
    uint32_t late = 0;
    for (size_t s = 0; s < SETUPS; s++) {
        size_t n = writes_for(setups[s].tv, at);
        for (size_t i = 0; i < n; i++) {
            struct play p =
                play(setups[s].machine, setups[s].tv, setups[s].tall, EYE_RIGHT, EYE_LEFT, at[i]);
            uint32_t snap = snapshot_report(EYE_RIGHT, EYE_LEFT);
            if (at[i] >= cut_tick(setups[s].tv)) {
                late++;
                CHECK_EQ_U64(p.first, EYE_RIGHT);
                CHECK(p.first != snap);
                disagree += p.first != snap;
            } else {
                CHECK_EQ_U64(p.first, snap);
            }
        }
    }
    CHECK(late != 0u);
    CHECK_EQ_U64(disagree, late);
}

/* Whether a store to addr faulted on the bus before the glasses existed: with
   work RAM off every address faulted except $FFFC and up when the cartridge was
   on, where the cartridge's mapper took it. */
static int old_faults(uint16_t addr, int cart_on, int ram_on)
{
    if (addr >= 0xFFFCu && cart_on) {
        return 0;
    }
    return !ram_on;
}

/* $3E, cartridge or RAM off, changes neither the eye nor the fault. */
static void test_gate(void)
{
    static const uint8_t mems[] = {MEM_DEFAULT, MEM_DEFAULT | MEM_CART_OFF,
                                   MEM_DEFAULT | MEM_RAM_OFF,
                                   MEM_DEFAULT | MEM_CART_OFF | MEM_RAM_OFF};
    static const uint16_t addrs[] = {0xC000u, 0xE123u, 0xFFF7u, 0xFFF8u, 0xFFF9u,
                                     0xFFFAu, 0xFFFBu, 0xFFFCu, 0xFFFDu, 0xFFFFu};
    static const uint32_t machines[] = {COGMASTER_MACHINE_SMS1, COGMASTER_MACHINE_SMS2,
                                        COGMASTER_MACHINE_GG};
    program(OP_HALT, OP_HALT);
    for (size_t mi = 0; mi < 3u; mi++) {
        cogmaster *m = load(machines[mi], COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
        for (size_t k = 0; k < sizeof mems; k++) {
            for (size_t a = 0; a < sizeof addrs / sizeof addrs[0]; a++) {
                for (uint8_t v = 0u; v < 2u; v++) {
                    uint16_t addr = addrs[a];
                    int cart_on = (mems[k] & MEM_CART_OFF) == 0u;
                    int ram_on = (mems[k] & MEM_RAM_OFF) == 0u;
                    int eye_addr = addr >= 0xFFF8u && addr <= 0xFFFBu;
                    m->io.mem_ctrl = mems[k];
                    m->io.glasses = EYE_NONE;
                    m->io.glasses_frame = EYE_NONE;
                    m->fault_present = 0u;
                    m->ram[addr & (COGMASTER__RAM_SIZE - 1u)] = 0xA5u;
                    cogmaster__bus_write(m, addr, v);
                    int fault = old_faults(addr, cart_on, ram_on);
                    CHECK_EQ_U64(m->fault_present, fault);
                    if (fault) {
                        CHECK_EQ_U64(m->fault.addr, addr);
                        CHECK_EQ_U64(m->fault.device, COGMASTER__DEVICE_RAM);
                        CHECK_EQ_U64(m->fault.kind, COGMASTER__FAULT_UNMAPPED_WRITE);
                    }
                    CHECK_EQ_U64(m->ram[addr & (COGMASTER__RAM_SIZE - 1u)], ram_on ? v : 0xA5u);
                    uint32_t want = EYE_NONE;
                    if (eye_addr && machines[mi] != COGMASTER_MACHINE_GG) {
                        want = v != 0u ? EYE_LEFT : EYE_RIGHT;
                    }
                    CHECK_EQ_U64(m->io.glasses, want);
                    CHECK_EQ_U64(m->io.glasses_frame, want);
                }
            }
        }
        cogmaster_destroy(m);
    }
}

static const struct {
    uint32_t machine;
    uint32_t region;
} profiles[] = {
    {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_JAPAN},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN},
    {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_GG, COGMASTER_REGION_JAPAN},
};
#define PROFILES (sizeof profiles / sizeof profiles[0])

/* A hundred thousand writes with mixed bit 0: a Master System ends on the last
   value written and a Game Gear never records one. */
static void test_profiles(void)
{
    program(OP_HALT, OP_HALT);
    for (size_t p = 0; p < PROFILES; p++) {
        cogmaster *m = load(profiles[p].machine, profiles[p].region, COGMASTER_TV_NTSC);
        int gg = profiles[p].machine == COGMASTER_MACHINE_GG;
        uint32_t x = 12345u;
        uint32_t last = EYE_NONE;
        for (uint32_t i = 0; i < 100000u; i++) {
            x = x * 1664525u + 1013904223u;
            uint8_t v = (uint8_t)(x >> 24);
            cogmaster__bus_write(m, (uint16_t)(0xFFF8u + (i & 3u)), v);
            last = (v & 1u) != 0u ? EYE_LEFT : EYE_RIGHT;
            if (gg) {
                CHECK_EQ_U64(m->io.glasses | m->io.glasses_frame, 0);
            }
        }
        CHECK_EQ_U64(m->io.glasses, gg ? EYE_NONE : last);
        CHECK_EQ_U64(m->io.glasses_frame, gg ? EYE_NONE : last);
        CHECK_EQ_U64(m->input_read, 0);
        CHECK_EQ_U64(run(m), gg ? EYE_NONE : last);
        cogmaster_destroy(m);
    }
}

/* The FNV-1a hash of the full field a frame left. */
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

/* A store to the glasses address is no read of the controller ports, so a
   frame holding one is still a lag frame; a read of $DC is the control. The
   picture is the same with the eye write and with the same store aimed at
   plain work RAM, because the core never changes a pixel. */
static void test_nolag(void)
{
    for (size_t p = 0; p < PROFILES; p++) {
        uint64_t hash[2];
        for (uint32_t aim = 0; aim < 2u; aim++) {
            program(OP_LD_HL_A, OP_HALT);
            cogmaster *m = load(profiles[p].machine, profiles[p].region, COGMASTER_TV_NTSC);
            m->z80.reg[COGMASTER__Z80_H] = aim == 0u ? 0xFFu : 0xC1u;
            m->z80.reg[COGMASTER__Z80_L] = aim == 0u ? 0xF8u : 0x00u;
            m->z80.reg[COGMASTER__Z80_A] = 1u;
            m->ticks = 1000u;
            cogmaster_run_result r;
            memset(&r, 0, sizeof r);
            r.size = (uint32_t)sizeof r;
            CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(m->input_read, 0);
            CHECK_EQ_U64(r.lagged, 1);
            int eye = aim == 0u && profiles[p].machine != COGMASTER_MACHINE_GG;
            CHECK_EQ_U64(r.glasses, eye ? EYE_LEFT : EYE_NONE);
            hash[aim] = frame_hash(m);
            cogmaster_destroy(m);
        }
        CHECK_EQ_U64(hash[0], hash[1]);

        program(OP_IN_A_N, 0xDCu);
        cogmaster *m = load(profiles[p].machine, profiles[p].region, COGMASTER_TV_NTSC);
        cogmaster_run_result r;
        memset(&r, 0, sizeof r);
        r.size = (uint32_t)sizeof r;
        CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(r.lagged, 0);
        cogmaster_destroy(m);
    }
}

/* The saved state is the byte range of one record entry, found by name. */
static int find_field(const cogmaster *m, const char *name, struct cogmaster__state_field *out)
{
    size_t count = cogmaster__state_record(m, NULL, 0);
    struct cogmaster__state_field *all = malloc(count * sizeof *all);
    if (all == NULL) {
        exit(1);
    }
    cogmaster__state_record(m, all, count);
    int found = 0;
    for (size_t i = 0; i < count && !found; i++) {
        if (strcmp(all[i].name, name) == 0) {
            *out = all[i];
            found = 1;
        }
    }
    free(all);
    return found;
}

/* The eye pair round-trips, and a state whose pair no run can make is refused
   with the instance left as it was. */
static void test_state(void)
{
    program(OP_LD_HL_A, OP_HALT);
    static const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t mi = 0; mi < 2u; mi++) {
        uint32_t machine = machines[mi];
        cogmaster *m = load(machine, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
        int gg = machine == COGMASTER_MACHINE_GG;
        /* The right eye is written before the frame, then the frame's own store of
           the left eye lands after the cut: the held eye is the left one and the
           eye the picture reports is the right one. */
        cogmaster__bus_write(m, 0xFFF8u, 0u);
        m->z80.reg[COGMASTER__Z80_H] = 0xFFu;
        m->z80.reg[COGMASTER__Z80_L] = 0xF8u;
        m->z80.reg[COGMASTER__Z80_A] = 1u;
        m->ticks = cut_tick(COGMASTER_TV_NTSC) + 100u;
        run(m);
        CHECK_EQ_U64(m->io.glasses, gg ? EYE_NONE : EYE_LEFT);
        CHECK_EQ_U64(m->io.glasses_frame, gg ? EYE_NONE : EYE_RIGHT);

        size_t size = cogmaster__state_size(m);
        uint8_t *good = malloc(size);
        uint8_t *again = malloc(size);
        uint8_t *bad = malloc(size);
        uint8_t *snap = malloc(sizeof *m);
        CHECK(good != NULL && again != NULL && bad != NULL && snap != NULL);
        if (good == NULL || again == NULL || bad == NULL || snap == NULL) {
            exit(1);
        }
        CHECK_EQ_U64(cogmaster__state_save(m, good, size), COGMASTER_STATUS_OK);
        cogmaster *other = load(machine, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
        CHECK_EQ_U64(cogmaster__state_load(other, good, size), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(other->io.glasses, m->io.glasses);
        CHECK_EQ_U64(other->io.glasses_frame, m->io.glasses_frame);
        CHECK_EQ_U64(cogmaster__state_save(other, again, size), COGMASTER_STATUS_OK);
        CHECK(memcmp(good, again, size) == 0);

        struct cogmaster__state_field fe;
        struct cogmaster__state_field ff;
        CHECK(find_field(m, "io_glasses", &fe));
        CHECK(find_field(m, "io_glasses_frame", &ff));
        CHECK_EQ_U64(fe.width, 1);
        CHECK_EQ_U64(ff.width, 1);
        CHECK_EQ_U64(fe.max, 2);
        CHECK_EQ_U64(ff.max, 2);
        /* eye, eye in the picture: 3 and anything for either is out of bounds,
           a picture eye with no held eye and a Game Gear eye break the
           cross-checks. */
        static const uint8_t pairs[][2] = {{3u, 0u}, {0u, 3u}, {0u, 1u}, {0u, 2u}, {1u, 3u},
                                           {1u, 1u}, {2u, 1u}, {1u, 2u}, {2u, 2u}, {0u, 0u}};
        for (size_t i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
            memcpy(bad, good, size);
            bad[fe.offset] = pairs[i][0];
            bad[ff.offset] = pairs[i][1];
            cogmaster__state_seal(bad, size);
            memcpy(snap, other, sizeof *other);
            cogmaster_status st = cogmaster__state_load(other, bad, size);
            int in_bounds = pairs[i][0] <= 2u && pairs[i][1] <= 2u;
            int paired = pairs[i][1] == 0u || pairs[i][0] != 0u;
            int none_on_gg = !gg || (pairs[i][0] == 0u && pairs[i][1] == 0u);
            int legal = in_bounds && paired && none_on_gg;
            if (legal) {
                CHECK_EQ_U64(st, COGMASTER_STATUS_OK);
                CHECK_EQ_U64(other->io.glasses, pairs[i][0]);
                CHECK_EQ_U64(other->io.glasses_frame, pairs[i][1]);
                CHECK_EQ_U64(cogmaster__state_save(other, again, size), COGMASTER_STATUS_OK);
                CHECK(memcmp(bad, again, size) == 0);
            } else {
                CHECK_EQ_U64(st, COGMASTER_STATUS_ERR_ARGUMENT);
                CHECK(memcmp(snap, other, sizeof *other) == 0);
            }
            CHECK_EQ_U64(cogmaster__state_load(other, good, size), COGMASTER_STATUS_OK);
        }
        free(good);
        free(again);
        free(bad);
        free(snap);
        cogmaster_destroy(other);
        cogmaster_destroy(m);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "rule") == 0) {
        test_rule();
    } else if (strcmp(argv[1], "control") == 0) {
        test_control();
    } else if (strcmp(argv[1], "gate") == 0) {
        test_gate();
    } else if (strcmp(argv[1], "profiles") == 0) {
        test_profiles();
    } else if (strcmp(argv[1], "nolag") == 0) {
        test_nolag();
    } else if (strcmp(argv[1], "state") == 0) {
        test_state();
    } else {
        return 2;
    }
    CHECK_DONE();
}
