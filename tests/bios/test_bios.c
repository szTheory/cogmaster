/* The boot ROM on the bus and in the load path, through the public API with the registers and the
   image also read through src/internal.h: cogmaster-test-bios <case>. Every image here is
   self-authored: 8 KiB of one byte attached by hand, or a pseudo-random body behind a short Z80
   program. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define ROM_BYTE 0x0Fu
#define BIOS_BYTE 0xF3u
#define RAM_BYTE 0x5Au
#define PROBES 4u
#define COMBOS 16u
#define MACHINES 2u
#define KINDS 4u
#define IMAGE_SIZE 16384u
#define STATE_BUF 393216u
#define BIG_SIZE 262144u
/* One byte more than the largest image the probe takes (a 1 MiB body and a copier header). */
#define HUGE_SIZE 1049089u

static const uint16_t probe_addr[PROBES] = {0x0000u, 0x1FFFu, 0x2000u, 0xC000u};
static const uint32_t machine_id[MACHINES] = {COGMASTER_MACHINE_SMS1, COGMASTER_MACHINE_SMS2};
static const char *const machine_name[MACHINES] = {"SMS1", "SMS2"};

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

/* A new instance of the machine running a 16 KiB image of ROM_BYTE, with the BIOS attached when
   asked: every byte BIOS_BYTE and the size 8192, the way the load will set it. */
static cogmaster *make(uint32_t machine, int with_bios)
{
    static uint8_t image[IMAGE_SIZE];
    memset(image, (int)ROM_BYTE, sizeof image);
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
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    if (with_bios) {
        memset(m->bios.rom, (int)BIOS_BYTE, sizeof m->bios.rom);
        m->bios.size = COGMASTER__BIOS_SIZE;
    }
    m->ram[0] = (uint8_t)RAM_BYTE;
    cogmaster__bus_decode(m);
    return m;
}

/* What the four probes should read, per machine, per kind of enables and per probe. The kind is
   2 * b6 + b3 of $3E (a set bit is off): 0 both on, 1 cartridge on and BIOS off, 2 cartridge off
   and BIOS on, 3 neither. An undriven read is the retained byte, which the probes set to $00
   (SMS1), or the pull-up $FF (SMS2). Typed by hand from the hardware tests, never from the core. */
struct arb_want {
    uint16_t val[MACHINES][KINDS][PROBES];
    uint8_t flt[MACHINES][COMBOS];
};

/* What the core did, per machine, per combination of $3E b7, b6, b5 and b3, and per probe. */
struct arb_got {
    uint16_t val[MACHINES][COMBOS][PROBES];
    uint8_t flt[MACHINES][COMBOS];
};

/* Index into the kind rows of the combination i = 8 * b7 + 4 * b6 + 2 * b5 + b3: b7 and b5 change
   no value, so a combination reads as its twin with both set. */
static uint32_t kind_of(uint32_t i)
{
    return 2u * ((i >> 2) & 1u) + (i & 1u);
}

static void note(char *msg, size_t n, uint32_t mach, uint32_t i, const char *what, uint32_t probe,
                 uint32_t got, uint32_t want)
{
    if (msg[0] == '\0') {
        (void)snprintf(msg, n, "%s b7=%u b6=%u b5=%u b3=%u %s at probe %u: got %u, want %u",
                       machine_name[mach], (i >> 3) & 1u, (i >> 2) & 1u, (i >> 1) & 1u, i & 1u,
                       what, probe, got, want);
    }
}

/* The number of entries of got that differ from want; the first one is described in msg. */
static uint32_t arb_compare(const struct arb_want *want, const struct arb_got *got, char *msg,
                            size_t n)
{
    uint32_t bad = 0;
    msg[0] = '\0';
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        for (uint32_t i = 0; i < COMBOS; i++) {
            for (uint32_t p = 0; p < PROBES; p++) {
                uint16_t w = want->val[mach][kind_of(i)][p];
                if (got->val[mach][i][p] != w) {
                    bad++;
                    note(msg, n, mach, i, "value", p, got->val[mach][i][p], w);
                }
            }
            if (got->flt[mach][i] != want->flt[mach][i]) {
                bad++;
                note(msg, n, mach, i, "fault count", PROBES, got->flt[mach][i], want->flt[mach][i]);
            }
        }
    }
    return bad;
}

/* Runs the four probes at every combination on both machines. A read and a peek must agree, and a
   read records a fault exactly when it counts open bus, so the fault column counts both. */
static void probe_all(struct arb_got *got, int with_bios)
{
    memset(got, 0, sizeof *got);
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        cogmaster *m = make(machine_id[mach], with_bios);
        for (uint32_t i = 0; i < COMBOS; i++) {
            m->io.mem_ctrl = (uint8_t)(((i >> 3) << 7) | (((i >> 2) & 1u) << 6) |
                                       (((i >> 1) & 1u) << 5) | ((i & 1u) << 3));
            cogmaster__bus_decode(m);
            for (uint32_t p = 0; p < PROBES; p++) {
                m->fault_present = 0u;
                m->counters.open_bus_reads = 0u;
                m->bus.data = 0u;
                uint32_t peeked = cogmaster_peek(m, COGMASTER_SPACE_BUS, probe_addr[p]);
                m->bus.data = 0u;
                uint8_t read = cogmaster__bus_read(m, probe_addr[p]);
                CHECK_EQ_U64(peeked, read);
                CHECK_EQ_U64(m->counters.open_bus_reads, m->fault_present);
                got->val[mach][i][p] = read;
                got->flt[mach][i] = (uint8_t)(got->flt[mach][i] + m->fault_present);
            }
        }
        cogmaster_destroy(m);
    }
}

/* With the BIOS attached: the fault count of a combination is the number of undriven probes when
   b7 and b5 are both set and 0 otherwise (a model row for an enabled but empty card or expansion
   slot). Only the combinations 14 and 15 have both set and an undriven probe: 1 and 3 of them. */
static const struct arb_want with_bios = {
    .val = {{{0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0xF3u, 0xF3u, 0x00u, 0x5Au},
             {0x00u, 0x00u, 0x00u, 0x5Au}},
            {{0x03u, 0x03u, 0x0Fu, 0x5Au},
             {0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0xF3u, 0xF3u, 0xFFu, 0x5Au},
             {0xFFu, 0xFFu, 0xFFu, 0x5Au}}},
    .flt = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3}},
};

/* With no BIOS: the cartridge or nothing, and every undriven slot read is a fault whatever b7 and
   b5 say. */
static const struct arb_want no_bios = {
    .val = {{{0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0x00u, 0x00u, 0x00u, 0x5Au},
             {0x00u, 0x00u, 0x00u, 0x5Au}},
            {{0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0x0Fu, 0x0Fu, 0x0Fu, 0x5Au},
             {0xFFu, 0xFFu, 0xFFu, 0x5Au},
             {0xFFu, 0xFFu, 0xFFu, 0x5Au}}},
    .flt = {{0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 3, 3, 3, 3},
            {0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 3, 3, 3, 3}},
};

/* The comparator must fail on want, and its message must name a machine. */
static void must_fail(const struct arb_want *want, const struct arb_got *got, const char *what)
{
    char msg[160];
    uint32_t bad = arb_compare(want, got, msg, sizeof msg);
    if (bad == 0u || strstr(msg, "SMS") == NULL) {
        fprintf(stderr, "control passed where it must fail: %s\n", what);
        check_failures++;
    }
}

static void test_arbitration(void)
{
    struct arb_got got;
    char msg[160];
    probe_all(&got, 1);
    if (arb_compare(&with_bios, &got, msg, sizeof msg) != 0u) {
        fprintf(stderr, "arbitration: %s\n", msg);
        check_failures++;
    }
    /* Every one-entry flip of the 32 values and the 32 fault counts is caught. */
    struct arb_want w;
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        for (uint32_t k = 0; k < KINDS; k++) {
            for (uint32_t p = 0; p < PROBES; p++) {
                w = with_bios;
                w.val[mach][k][p] = (uint16_t)(w.val[mach][k][p] ^ 0x01u);
                must_fail(&w, &got, "value flip");
            }
        }
        for (uint32_t i = 0; i < COMBOS; i++) {
            w = with_bios;
            w.flt[mach][i] = (uint8_t)(w.flt[mach][i] + 1u);
            must_fail(&w, &got, "fault flip");
        }
    }
    /* The three wrong whole tables: the machines' columns swapped, the AND on both machines, and
       the BIOS always winning. */
    w = with_bios;
    memcpy(w.val[0], with_bios.val[1], sizeof w.val[0]);
    memcpy(w.val[1], with_bios.val[0], sizeof w.val[1]);
    must_fail(&w, &got, "columns swapped");
    w = with_bios;
    memcpy(w.val[0][0], with_bios.val[1][0], sizeof w.val[0][0]);
    must_fail(&w, &got, "AND on both machines");
    w = with_bios;
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        w.val[mach][0][0] = BIOS_BYTE;
        w.val[mach][0][1] = BIOS_BYTE;
    }
    must_fail(&w, &got, "BIOS always wins");
    /* With no BIOS attached every undriven slot read is the fault, b7 and b5 or not. */
    probe_all(&got, 0);
    if (arb_compare(&no_bios, &got, msg, sizeof msg) != 0u) {
        fprintf(stderr, "no BIOS: %s\n", msg);
        check_failures++;
    }
    must_fail(&with_bios, &got, "BIOS table against no BIOS");
}

/* Whether the flag matches what $3E and the BIOS size say, restated in words: the flag is set when
   b6 (cartridge off) or b4 (work RAM off) is set, or a BIOS is attached and b3 is clear. */
static int flag_matches(const cogmaster *m)
{
    uint8_t c = m->io.mem_ctrl;
    int cart_off = (c & 0x40u) != 0u;
    int ram_off = (c & 0x10u) != 0u;
    int bios_answers = m->bios.size != 0u && (c & 0x08u) == 0u;
    return m->mem_slow == (uint8_t)(cart_off || ram_off || bios_answers ? 1u : 0u);
}

static void flag_leg(uint32_t machine, int with_bios_attached)
{
    cogmaster *m = make(machine, with_bios_attached);
    uint32_t s = 0x2545F491u;
    for (uint32_t n = 0; n < 4096u; n++) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        cogmaster__io_out(m, 0x3Eu, (uint8_t)(s >> 8));
        if (!flag_matches(m)) {
            fprintf(stderr, "flag stale after a $3E write of %u\n", (s >> 8) & 0xFFu);
            check_failures++;
            break;
        }
    }
    /* A reset puts $3E back to its power-on value and the flag with it. */
    cogmaster__io_out(m, 0x3Eu, 0x50u);
    CHECK(flag_matches(m));
    cogmaster__io_reset(m);
    CHECK(flag_matches(m));
    /* A flag set by hand to the wrong value must fail the comparator. */
    cogmaster__io_out(m, 0x3Eu, 0x50u);
    m->mem_slow = 0u;
    CHECK(!flag_matches(m));
    cogmaster_destroy(m);
}

static void test_slow_flag(void)
{
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        flag_leg(machine_id[mach], 0);
        flag_leg(machine_id[mach], 1);
    }
}

/* ---- load: the validation order, ownership, power-on state and reset ---- */

static uint8_t bios_img[COGMASTER__BIOS_SIZE];
static uint8_t cart64[65536];
static uint8_t state_a[STATE_BUF];
static uint8_t state_b[STATE_BUF];

static uint32_t xorshift(uint32_t *s)
{
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return *s;
}

/* A self-authored 8 KiB image: di, then a jump to itself, then a pseudo-random body seeded by seed
   (no byte of any real firmware is involved). */
static void bios_fill(uint8_t *img, uint32_t seed)
{
    uint32_t s = seed;
    for (uint32_t i = 0; i < COGMASTER__BIOS_SIZE; i++) {
        img[i] = (uint8_t)(xorshift(&s) >> 11);
    }
    img[0] = 0xF3u;
    img[1] = 0x18u;
    img[2] = 0xFEu;
}

/* A 64 KiB cartridge of NOPs with load-to-register-store sites, so the probe picks the Sega mapper
   and the power-on banks are 0, 1, 2. */
static void cart_fill(void)
{
    memset(cart64, 0, sizeof cart64);
    for (uint32_t i = 0; i < 16u; i++) {
        cart64[0x100u + 3u * i] = 0x32u;
        cart64[0x101u + 3u * i] = 0xFFu;
        cart64[0x102u + 3u * i] = 0xFFu;
    }
}

static cogmaster *blank(void)
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
    return m;
}

static cogmaster_media media_of(const void *image, size_t image_size, uint32_t machine,
                                const void *bios, size_t bios_size)
{
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = image_size;
    media.machine = machine;
    media.bios = bios;
    media.bios_size = bios_size;
    return media;
}

static size_t save_to(const cogmaster *m, uint8_t *buf)
{
    size_t n = cogmaster__state_size(m);
    CHECK(n <= STATE_BUF);
    CHECK_EQ_U64(cogmaster__state_save(m, buf, STATE_BUF), COGMASTER_STATUS_OK);
    return n;
}

static void run_frames(cogmaster *m, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
}

/* A load that must be refused with want and must change nothing: the state bytes, the BIOS copy
   and the derived flag are the same before and after. */
static void refused(cogmaster *m, const cogmaster_media *media, cogmaster_status want,
                    const char *what)
{
    static struct cogmaster__bios before;
    size_t n = save_to(m, state_a);
    before = m->bios;
    uint8_t flag = m->mem_slow;
    cogmaster_status got = cogmaster_load(m, media);
    if (got != want) {
        fprintf(stderr, "load %s: status %d, want %d\n", what, (int)got, (int)want);
        check_failures++;
    }
    CHECK_EQ_U64(save_to(m, state_b), n);
    CHECK(memcmp(state_a, state_b, n) == 0);
    CHECK(memcmp(&before, &m->bios, sizeof before) == 0);
    CHECK_EQ_U64(m->mem_slow, flag);
}

/* Whether the flag equals its restatement in words (see flag_matches below). */
static int flag_matches(const cogmaster *m);

static void test_load_refusals(void)
{
    static uint8_t big[BIG_SIZE];
    bios_fill(big, 7u);
    cogmaster *m = blank();
    /* First a BIOS loaded for real, so a refusal has a BIOS to leave alone. */
    cogmaster_media good =
        media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, bios_img, COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(m, &good), COGMASTER_STATUS_OK);
    run_frames(m, 2u);
    static const size_t bad_sizes[] = {1u, 1023u, 1024u, 8191u, 8193u, 16384u, 131072u, BIG_SIZE};
    for (size_t i = 0; i < sizeof bad_sizes / sizeof bad_sizes[0]; i++) {
        cogmaster_media media =
            media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, big, bad_sizes[i]);
        refused(m, &media, COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE, "size");
    }
    /* The pair rule comes first: a size of 0 with a pointer, a size with NULL. */
    cogmaster_media media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, big, 0u);
    refused(m, &media, COGMASTER_STATUS_ERR_ARGUMENT, "pointer without size");
    media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, NULL, COGMASTER__BIOS_SIZE);
    refused(m, &media, COGMASTER_STATUS_ERR_ARGUMENT, "size without pointer");
    /* The test card runs no CPU: a BIOS on it is no content, a wrong size still reports itself. */
    media = media_of(NULL, 0u, COGMASTER_MACHINE_SMS2, bios_img, COGMASTER__BIOS_SIZE);
    refused(m, &media, COGMASTER_STATUS_ERR_NO_CONTENT, "test card");
    media = media_of(NULL, 0u, COGMASTER_MACHINE_SMS2, big, 8191u);
    refused(m, &media, COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE, "test card, wrong size");
    /* A Game Gear is refused after the cartridge probe resolved it. */
    media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_GG, bios_img, COGMASTER__BIOS_SIZE);
    refused(m, &media, COGMASTER_STATUS_ERR_PROFILE_REJECTED, "game gear");
    media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_GG, big, 8191u);
    refused(m, &media, COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE, "game gear, wrong size");
    /* An unprobeable cartridge keeps its own status under a valid BIOS, and a bad BIOS pair or size
       is reported before the probe runs. */
    static uint8_t huge[HUGE_SIZE];
    static const size_t bad_carts[] = {1u, 1000u, HUGE_SIZE};
    for (size_t i = 0; i < sizeof bad_carts / sizeof bad_carts[0]; i++) {
        cogmaster_media plain = media_of(huge, bad_carts[i], COGMASTER_MACHINE_SMS2, NULL, 0u);
        cogmaster *probe = blank();
        cogmaster_status own = cogmaster_load(probe, &plain);
        cogmaster_destroy(probe);
        CHECK(own != COGMASTER_STATUS_OK);
        media =
            media_of(huge, bad_carts[i], COGMASTER_MACHINE_SMS2, bios_img, COGMASTER__BIOS_SIZE);
        refused(m, &media, own, "unprobeable cartridge");
        media = media_of(huge, bad_carts[i], COGMASTER_MACHINE_SMS2, NULL, COGMASTER__BIOS_SIZE);
        refused(m, &media, COGMASTER_STATUS_ERR_ARGUMENT, "bad pair, unprobeable cartridge");
    }
    media = media_of(NULL, 5u, COGMASTER_MACHINE_SMS2, bios_img, COGMASTER__BIOS_SIZE);
    refused(m, &media, COGMASTER_STATUS_ERR_ARGUMENT, "cartridge size without pointer");
    CHECK(flag_matches(m));
    cogmaster_destroy(m);
    /* The same refusals on an instance that never had a BIOS leave none. */
    m = blank();
    media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, big, 8191u);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    CHECK_EQ_U64(m->bios.size, 0);
    CHECK_EQ_U64(m->loaded, 0);
    cogmaster_destroy(m);
}

/* A load without a BIOS clears the last one; the instance owns its copy; the size tag decides
   whether the two fields are read at all. */
static void test_load_ownership(void)
{
    cogmaster *m = blank();
    uint8_t *copy = (uint8_t *)malloc(COGMASTER__BIOS_SIZE);
    if (copy == NULL) {
        exit(1);
    }
    memcpy(copy, bios_img, COGMASTER__BIOS_SIZE);
    cogmaster_media media =
        media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS1, copy, COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->bios.size, COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(m->bios.crc32, cogmaster__crc32(0u, bios_img, COGMASTER__BIOS_SIZE));
    /* The caller's buffer goes at once: the sanitiser sees any later read of it. */
    free(copy);
    run_frames(m, 3u);
    CHECK(memcmp(m->bios.rom, bios_img, COGMASTER__BIOS_SIZE) == 0);
    /* No BIOS in the next load: fast boot, the BIOS gone. */
    media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS1, NULL, 0u);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    static const uint8_t zero[COGMASTER__BIOS_SIZE];
    CHECK_EQ_U64(m->bios.size, 0);
    CHECK_EQ_U64(m->bios.crc32, 0);
    CHECK(memcmp(m->bios.rom, zero, sizeof zero) == 0);
    CHECK(flag_matches(m));
    CHECK_EQ_U64(m->z80.sp, 0xDFF0);
    CHECK_EQ_U64(m->ram[0], 0xAB);
    /* A size tag that does not reach the end of bios_size reads both fields as absent, even with
       a good pair over the old tail padding; the whole struct reads them. */
    const uint32_t tag_old = sizeof(void *) == 8u ? 56u : 32u;
    const uint32_t tag_full = (uint32_t)sizeof(cogmaster_media);
    const uint32_t tag_ptr = (uint32_t)offsetof(cogmaster_media, bios_size);
    const uint32_t tags[] = {tag_old, tag_ptr, tag_full - 1u, tag_full};
    for (size_t i = 0; i < sizeof tags / sizeof tags[0]; i++) {
        media =
            media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, bios_img, COGMASTER__BIOS_SIZE);
        media.size = tags[i];
        CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(m->bios.size, tags[i] == tag_full ? COGMASTER__BIOS_SIZE : 0u);
        CHECK(flag_matches(m));
    }
    /* A pair that would be refused is not even looked at below the tag. */
    media = media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, NULL, COGMASTER__BIOS_SIZE);
    media.size = tag_old;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->bios.size, 0);
    cogmaster_destroy(m);
}

/* The state a BIOS starts from, and what each reset keeps. */
static void power_on_leg(uint32_t machine)
{
    cogmaster *m = blank();
    cogmaster_media media =
        media_of(cart64, sizeof cart64, machine, bios_img, COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (int pass = 0; pass < 3; pass++) {
        if (pass > 0) {
            /* pass 1 a soft reset, pass 2 a power reset, each after the machine moved on */
            m->vdp.vram[5] = 0x33u;
            m->vdp.cram[1] = 0x07u;
            m->ram[100] = 0x77u;
            run_frames(m, 2u);
            m->io.mem_ctrl = 0xABu;
            cogmaster__bus_decode(m);
            CHECK_EQ_U64(
                cogmaster_reset(m, pass == 1 ? COGMASTER_RESET_SOFT : COGMASTER_RESET_POWER),
                COGMASTER_STATUS_OK);
            CHECK_EQ_U64(m->ram[100], pass == 1 ? 0x77u : 0u);
            CHECK_EQ_U64(m->vdp.vram[5], pass == 1 ? 0x33u : 0u);
            CHECK_EQ_U64(m->vdp.cram[1], pass == 1 ? 0x07u : 0u);
        }
        const struct cogmaster__z80 *z = &m->z80;
        CHECK_EQ_U64(z->pc, 0);
        CHECK_EQ_U64(z->reg[COGMASTER__Z80_A], 0xFF);
        CHECK_EQ_U64(z->reg[COGMASTER__Z80_F], 0xFF);
        CHECK_EQ_U64(z->sp, 0xFFFF);
        CHECK_EQ_U64(z->im, 0);
        CHECK_EQ_U64(z->iff1, 0);
        CHECK_EQ_U64(z->iff2, 0);
        CHECK_EQ_U64(z->halted, 0);
        for (uint32_t r = 0; r < COGMASTER__Z80_REGS; r++) {
            if (r != COGMASTER__Z80_A && r != COGMASTER__Z80_F) {
                CHECK_EQ_U64(z->reg[r], 0);
            }
        }
        CHECK_EQ_U64(m->io.mem_ctrl, 0xE0);
        CHECK_EQ_U64(m->mapper.ctrl, 0);
        CHECK_EQ_U64(m->mapper.bank[0], 0);
        CHECK_EQ_U64(m->mapper.bank[1], 1);
        CHECK_EQ_U64(m->mapper.bank[2], 2);
        CHECK_EQ_U64(m->mem_slow, 1);
        CHECK(flag_matches(m));
        /* The hand-off byte is the BIOS's to write. */
        CHECK(m->ram[0] != 0xABu);
        if (pass != 1) {
            for (uint32_t i = 0; i < COGMASTER__RAM_SIZE; i++) {
                if (m->ram[i] != 0u) {
                    CHECK_EQ_U64(m->ram[i], 0);
                    break;
                }
            }
        }
        /* The instruction at address 0 is the BIOS's, not the cartridge's. */
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0u), 0xF3u);
    }
    cogmaster_destroy(m);
}

static void test_load_power_on(void)
{
    power_on_leg(COGMASTER_MACHINE_SMS1);
    power_on_leg(COGMASTER_MACHINE_SMS2);
}

/* Fault prevented: a BIOS load that accepts a hostile size or pointer pair, holds a pointer into
   caller memory, or changes the instance before it can still refuse; and a BIOS boot that leaks
   the fast-boot constants into the power-on state. */
static void test_load(void)
{
    bios_fill(bios_img, 0x9E3779B9u);
    cart_fill();
    test_load_refusals();
    test_load_ownership();
    test_load_power_on();
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"arbitration", test_arbitration},
    {"slow_flag", test_slow_flag},
    {"load", test_load},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-bios <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-bios: unknown case %s\n", argv[1]);
    return 2;
}
