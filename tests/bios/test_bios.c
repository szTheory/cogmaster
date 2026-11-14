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

/* With a BIOS attached and work RAM off (b4 set), a read at $C000 or $FFFF is undriven and is a
   fault whatever b7, b6, b5 and b3 say: the silent empty-slot answer belongs to the slot area only.
   The value is the retained byte (SMS1, $00 here) or the pull-up (SMS2, $FF). */
static void test_ram_off(void)
{
    static const uint16_t high[2] = {0xC000u, 0xFFFFu};
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        cogmaster *m = make(machine_id[mach], 1);
        for (uint32_t i = 0; i < COMBOS; i++) {
            m->io.mem_ctrl = (uint8_t)(0x10u | ((i >> 3) << 7) | (((i >> 2) & 1u) << 6) |
                                       (((i >> 1) & 1u) << 5) | ((i & 1u) << 3));
            cogmaster__bus_decode(m);
            for (uint32_t p = 0; p < 2u; p++) {
                m->fault_present = 0u;
                m->counters.open_bus_reads = 0u;
                m->bus.data = 0u;
                uint32_t peeked = cogmaster_peek(m, COGMASTER_SPACE_BUS, high[p]);
                m->bus.data = 0u;
                uint8_t read = cogmaster__bus_read(m, high[p]);
                CHECK_EQ_U64(read, mach == 1u ? 0xFFu : 0x00u);
                CHECK_EQ_U64(peeked, read);
                CHECK_EQ_U64(m->fault_present, 1u);
                CHECK_EQ_U64(m->counters.open_bus_reads, 1u);
            }
        }
        cogmaster_destroy(m);
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
    test_ram_off();
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
    static uint8_t zero[COGMASTER__BIOS_SIZE];
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

/* ---- the state: binding to the boot mode, the BIOS window detector, the flag after a load ---- */

static uint8_t state_c[STATE_BUF];
static uint8_t state_d[STATE_BUF];

/* An image whose program copies COPY_LEN bytes of its own body (from address 0x100) into work
   RAM at 0xC100 and then waits: the control for the window detector. */
#define COPY_LEN 64u
static void bios_copier(uint8_t *img, uint32_t seed)
{
    static const uint8_t program[] = {
        0xF3u, /* di */
        0x21u, 0x00u,
        0x01u, /* ld hl, 0x0100 */
        0x11u, 0x00u,
        0xC1u, /* ld de, 0xC100 */
        0x01u, (uint8_t)COPY_LEN,
        0x00u,        /* ld bc, COPY_LEN */
        0xEDu, 0xB0u, /* ldir */
        0x18u, 0xFEu, /* jr $ */
    };
    bios_fill(img, seed);
    memcpy(img, program, sizeof program);
}

static cogmaster *boot_with(uint32_t machine, const uint8_t *img)
{
    cogmaster *m = blank();
    cogmaster_media media =
        media_of(cart64, sizeof cart64, machine, img, img != NULL ? COGMASTER__BIOS_SIZE : 0u);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

#define WINDOW 32u
#define TABLE_BITS 14u

static uint32_t get4(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* The number of 32-byte windows of the image, at every offset, that occur anywhere in the state.
   The windows are indexed by their first four bytes in an open-addressed table, so the scan costs
   one lookup per state byte whatever the image. */
static uint32_t windows_in(const uint8_t *img, const uint8_t *st, size_t n)
{
    static uint16_t table[1u << TABLE_BITS];
    memset(table, 0, sizeof table);
    for (uint32_t off = 0; off + WINDOW <= COGMASTER__BIOS_SIZE; off++) {
        uint32_t h = (get4(img + off) * 2654435761u) >> (32u - TABLE_BITS);
        while (table[h] != 0u) {
            h = (h + 1u) & ((1u << TABLE_BITS) - 1u);
        }
        table[h] = (uint16_t)(off + 1u);
    }
    uint32_t found = 0;
    for (size_t i = 0; i + WINDOW <= n; i++) {
        uint32_t h = (get4(st + i) * 2654435761u) >> (32u - TABLE_BITS);
        while (table[h] != 0u) {
            if (memcmp(img + (table[h] - 1u), st + i, WINDOW) == 0) {
                found++;
            }
            h = (h + 1u) & ((1u << TABLE_BITS) - 1u);
        }
    }
    return found;
}

/* A state that must be refused with ERR_ARGUMENT and change nothing. */
static void state_refused(cogmaster *target, const uint8_t *buf, size_t n, const char *what)
{
    size_t before = save_to(target, state_c);
    cogmaster_status got = cogmaster__state_load(target, buf, n);
    if (got != COGMASTER_STATUS_ERR_ARGUMENT) {
        fprintf(stderr, "state %s: status %d, want ERR_ARGUMENT\n", what, (int)got);
        check_failures++;
    }
    CHECK_EQ_U64(save_to(target, state_d), before);
    CHECK(memcmp(state_c, state_d, before) == 0);
}

/* A state binds to the firmware it ran under: fast boot, or one BIOS by its CRC-32. */
static void state_binding_leg(uint32_t machine)
{
    static uint8_t img_a[COGMASTER__BIOS_SIZE];
    static uint8_t img_b[COGMASTER__BIOS_SIZE];
    bios_fill(img_a, 21u);
    bios_fill(img_b, 22u);
    cogmaster *fast = boot_with(machine, NULL);
    cogmaster *fast2 = boot_with(machine, NULL);
    cogmaster *a = boot_with(machine, img_a);
    cogmaster *a2 = boot_with(machine, img_a);
    cogmaster *b = boot_with(machine, img_b);
    run_frames(a, 2u);
    run_frames(fast, 2u);
    size_t nf = save_to(fast, state_a);
    size_t na = save_to(a, state_b);
    CHECK_EQ_U64(nf, na);
    /* Header: the boot byte at 53, the BIOS CRC-32 at 54 to 57, six reserved zero bytes after. */
    CHECK_EQ_U64(state_a[53], 0);
    CHECK_EQ_U64(get4(state_a + 54), 0);
    CHECK_EQ_U64(state_b[53], 1);
    CHECK(a->bios.crc32 != 0u && a->bios.crc32 != b->bios.crc32);
    CHECK_EQ_U64(get4(state_b + 54), a->bios.crc32);
    for (uint32_t i = 58; i < 64u; i++) {
        CHECK_EQ_U64(state_a[i], 0);
        CHECK_EQ_U64(state_b[i], 0);
    }
    CHECK_EQ_U64(cogmaster__state_load(fast2, state_a, nf), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(a2, state_b, na), COGMASTER_STATUS_OK);
    state_refused(a2, state_a, nf, "fast boot state into a BIOS instance");
    state_refused(fast2, state_b, na, "BIOS state into a fast boot instance");
    state_refused(b, state_b, na, "state of one BIOS into another");
    cogmaster_destroy(fast);
    cogmaster_destroy(fast2);
    cogmaster_destroy(a);
    cogmaster_destroy(a2);
    cogmaster_destroy(b);
}

/* Fault prevented: a state from one boot mode or BIOS loaded into another (a machine that
   continues under the wrong firmware), and the false belief that a state never holds bytes
   derived from the BIOS. The claim "no BIOS byte in a state" is FALSE for a real run: a real BIOS
   copies itself into work RAM and its logo reaches VRAM, which is why a real run's states, saves,
   frames and logs never leave the machine (a policy, not a test). What is tested: a self-authored
   image that never copies itself to work RAM leaves no 32-byte window of itself in the state after
   three frames, and a control image that copies 64 of its own bytes MUST show the window, so the
   detector is proven before its silence means anything. */
static void test_state(void)
{
    static uint8_t clean[COGMASTER__BIOS_SIZE];
    static uint8_t copier[COGMASTER__BIOS_SIZE];
    bios_fill(clean, 11u);
    bios_copier(copier, 12u);
    bios_fill(bios_img, 0x9E3779B9u);
    cart_fill();
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        cogmaster *m = boot_with(machine_id[mach], clean);
        run_frames(m, 3u);
        size_t n = save_to(m, state_a);
        uint32_t seen = windows_in(clean, state_a, n);
        if (seen != 0u) {
            fprintf(stderr, "state: %u windows of the clean image occur in the state\n", seen);
            check_failures++;
        }
        cogmaster_destroy(m);
        m = boot_with(machine_id[mach], copier);
        run_frames(m, 3u);
        CHECK(memcmp(m->ram + 0x100, copier + 0x100, COPY_LEN) == 0);
        n = save_to(m, state_a);
        seen = windows_in(copier, state_a, n);
        if (seen < COPY_LEN - WINDOW + 1u) {
            fprintf(stderr,
                    "state: the detector found %u windows of the copying image, wanted %u\n", seen,
                    COPY_LEN - WINDOW + 1u);
            check_failures++;
        }
        cogmaster_destroy(m);
        state_binding_leg(machine_id[mach]);
    }
}

/* The flag after each way into a state: a load, both resets and a state load. */
static void flag_after(cogmaster *m, const char *what)
{
    if (!flag_matches(m)) {
        fprintf(stderr, "flag stale after %s\n", what);
        check_failures++;
    }
}

static void flag_load_legs(uint32_t machine, int with_bios_attached)
{
    cogmaster *m = boot_with(machine, with_bios_attached ? bios_img : NULL);
    flag_after(m, "a load");
    /* A load over a loaded instance, with and without the BIOS, and a power-on reset after $3E
     * moved. */
    cogmaster__io_out(m, 0x3Eu, 0xFBu);
    cogmaster_media media =
        media_of(cart64, sizeof cart64, machine, with_bios_attached ? NULL : bios_img,
                 with_bios_attached ? 0u : COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    flag_after(m, "a load that flips the BIOS");
    for (uint32_t kind = 0; kind < 2u; kind++) {
        cogmaster__io_out(m, 0x3Eu, 0xFBu);
        flag_after(m, "a $3E write");
        CHECK_EQ_U64(cogmaster_reset(m, kind == 0u ? COGMASTER_RESET_SOFT : COGMASTER_RESET_POWER),
                     COGMASTER_STATUS_OK);
        flag_after(m, kind == 0u ? "a soft reset" : "a power reset");
        CHECK_EQ_U64(m->io.mem_ctrl, m->bios.size != 0u ? 0xE0u : 0xABu);
    }
    cogmaster_destroy(m);
}

/* A state saved with one $3E loaded into an instance holding another: the flag follows the loaded
   register, whether the saved value turns the cartridge off (b6), the BIOS on (b3 clear) or
   neither, and in both directions. */
static void flag_state_leg(uint32_t machine, int with_bios_attached)
{
    static const uint8_t values[] = {0xABu, 0xEBu, 0xA0u, 0xB0u, 0xE0u, 0xA3u};
    const uint8_t *img = with_bios_attached ? bios_img : NULL;
    for (size_t i = 0; i < sizeof values; i++) {
        cogmaster *src = boot_with(machine, img);
        cogmaster *dst = boot_with(machine, img);
        cogmaster__io_out(src, 0x3Eu, values[i]);
        size_t n = save_to(src, state_a);
        cogmaster__io_out(dst, 0x3Eu, values[i] == 0xABu ? 0xE0u : 0xABu);
        flag_after(dst, "a $3E write before a state load");
        CHECK_EQ_U64(cogmaster__state_load(dst, state_a, n), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(dst->io.mem_ctrl, values[i]);
        flag_after(dst, "a state load");
        cogmaster_destroy(src);
        cogmaster_destroy(dst);
    }
}

static void test_slow_flag(void)
{
    bios_fill(bios_img, 0x9E3779B9u);
    cart_fill();
    for (uint32_t mach = 0; mach < MACHINES; mach++) {
        flag_leg(machine_id[mach], 0);
        flag_leg(machine_id[mach], 1);
        flag_load_legs(machine_id[mach], 0);
        flag_load_legs(machine_id[mach], 1);
        flag_state_leg(machine_id[mach], 0);
        flag_state_leg(machine_id[mach], 1);
    }
}

/* ---- hostile and wrong-sized images ---- */

/* The most a frame may overshoot its deadline: one instruction (at most 68 ticks). */
#define OVERSHOOT_MAX 68u

static const uint8_t code_halt[] = {0xF3u, 0x76u};
static const uint8_t code_jr_self[] = {0xF3u, 0x18u, 0xFEu};
/* ld a,$FF; out ($3E),a; xor a; out ($3E),a; jr back: both slots off, then both on, again and
   again, and the fetch after the first write comes off an undriven bus. */
static const uint8_t code_alternate[] = {0xF3u, 0x3Eu, 0xFFu, 0xD3u, 0x3Eu,
                                         0xAFu, 0xD3u, 0x3Eu, 0x18u, 0xF7u};
/* xor a; out ($3E),a; cpl; jr back to the out: a write at every other instruction. */
static const uint8_t code_toggle[] = {0xF3u, 0xAFu, 0xD3u, 0x3Eu, 0x2Fu, 0x18u, 0xFBu};
/* ld sp,0; push af; jr back: the stack walks down through work RAM and the mapper registers. */
static const uint8_t code_stack[] = {0xF3u, 0x31u, 0x00u, 0x00u, 0xF5u, 0x18u, 0xFDu};

static const struct {
    const char *name;
    const uint8_t *code;
    size_t n;
    uint8_t fill;
    int fault;
} hostile[] = {
    {"di halt", code_halt, sizeof code_halt, 0x00u, 0},
    {"jr self", code_jr_self, sizeof code_jr_self, 0x00u, 0},
    {"alternate 3E", code_alternate, sizeof code_alternate, 0x00u, 1},
    {"toggle 3E", code_toggle, sizeof code_toggle, 0x00u, 0},
    {"wild stack", code_stack, sizeof code_stack, 0x00u, 0},
    {"all FF", NULL, 0u, 0xFFu, 0},
    {"all 00", NULL, 0u, 0x00u, 0},
};

static void hostile_leg(uint32_t machine, size_t which)
{
    static uint8_t cart[IMAGE_SIZE];
    memset(cart, 0, sizeof cart);
    cart[0] = 0xF3u;
    cart[1] = 0x76u;
    memset(bios_img, hostile[which].fill, sizeof bios_img);
    for (size_t i = 0; i < hostile[which].n; i++) {
        bios_img[i] = hostile[which].code[i];
    }
    cogmaster *m = blank();
    cogmaster_media media = media_of(cart, sizeof cart, machine, bios_img, COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (uint32_t f = 0; f < 3u; f++) {
        uint64_t before = m->frame;
        cogmaster_run_result r;
        memset(&r, 0, sizeof r);
        r.size = (uint32_t)sizeof r;
        CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
        if (r.overshoot_ticks > OVERSHOOT_MAX) {
            fprintf(stderr, "hostile %s: overshoot %u ticks\n", hostile[which].name,
                    (unsigned)r.overshoot_ticks);
            check_failures++;
        }
        CHECK_EQ_U64(m->frame, before + 1u);
    }
    cogmaster_fault fault;
    memset(&fault, 0, sizeof fault);
    fault.size = (uint32_t)sizeof fault;
    CHECK_EQ_U64(cogmaster_get_fault(m, &fault), COGMASTER_STATUS_OK);
    if (hostile[which].fault) {
        CHECK_EQ_U64(fault.present, 1);
    }
    cogmaster_destroy(m);
}

/* Every refused size, with a buffer of exactly that many bytes (one for a size beyond any
   allocation, which is only compared) so a read past the pointer is a sanitiser report. */
static void sweep_sizes(void)
{
    bios_fill(bios_img, 0x51ED270Bu);
    cart_fill();
    cogmaster *m = blank();
    cogmaster_media good =
        media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, bios_img, COGMASTER__BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(m, &good), COGMASTER_STATUS_OK);
    run_frames(m, 2u);
    static const size_t sizes[] = {0u, 1u, 1023u, 1025u, 8191u, 8193u, 262145u, SIZE_MAX};
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        size_t have = sizes[i] == SIZE_MAX ? 1u : sizes[i] > 0u ? sizes[i] : 1u;
        uint8_t *buf = malloc(have);
        CHECK(buf != NULL);
        if (buf == NULL) {
            exit(1);
        }
        memset(buf, 0x6Cu, have);
        cogmaster_media media =
            media_of(cart64, sizeof cart64, COGMASTER_MACHINE_SMS2, buf, sizes[i]);
        refused(m, &media,
                sizes[i] == 0u ? COGMASTER_STATUS_ERR_ARGUMENT
                               : COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE,
                "sweep");
        free(buf);
    }
    cogmaster_destroy(m);
}

/* Fault prevented: a hostile BIOS image (a jam, a stack overwrite, a $3E toggle that puts both
   slots on and off at every instruction) that holds the host past its frame time or corrupts the
   instance, and a BIOS-size input that crashes the loader. */
static void test_hostile(void)
{
    cart_fill();
    for (size_t i = 0; i < sizeof hostile / sizeof hostile[0]; i++) {
        hostile_leg(COGMASTER_MACHINE_SMS1, i);
        hostile_leg(COGMASTER_MACHINE_SMS2, i);
    }
    sweep_sizes();
}

/* ---- equivalence: the fast-boot hand-off against the state an owned BIOS-shaped image reaches
   ----

   Instance A is fast boot over the probe cartridge (a CPU frozen at a halt); instance B boots the
   same cartridge through bios_shape.bin and is stepped to the instant it jumps to address 0 with
   port $3E at $AB, then both are walked field by field with cogmaster__state_record. The image
   was written from SMS-SYSTEM section 3, section 6 and the hand-off table in C7, never from
   src/boot.c, and the literals below are typed from those places, so the image and boot.c cannot
   both be wrong the same way unseen.

   The honest limit: the image writes the constants fast boot sets, so this proves the mechanism
   (the BIOS window, the $3E walk, the hand-off through the mapper) and the single source of the
   constants in boot.c. It does not prove what Sega's BIOS leaves behind; that is checked only on
   a dump its owner holds, in the local library preset, as predicates. */

#define EQ_MAX_FIELDS 220000u
#define EQ_STEP_BOUND 40000u
#define EQ_SHORT_BOUND 1000u
#define EQ_COMPARED_NAMES 88u
#define EQ_EXCLUDED_NAMES 73u
#define EQ_REQUIRED_NAMES 16u
#define EQ_WINDOW_FIRST 0x100u
#define EQ_WINDOW_LAST 0x1FFu
#define EQ_RESULTS 0x1D0u
#define EQ_RESULT_BYTES 18u
#define EQ_NAMES_MAX 8u
#define EQ_PROBE_SIZE 65536u
#define EQ_BIOS_SIZE 8192u

static struct cogmaster__state_field eq_fields[EQ_MAX_FIELDS];
static uint8_t eq_probe[EQ_PROBE_SIZE + 1u];
static uint8_t eq_bios[EQ_BIOS_SIZE + 1u];
static const char *eq_probe_path;
static const char *eq_bios_path;

/* A field that is not compared, with the reason it is not. */
struct eq_exclusion {
    const char *name;
    const char *why;
};

/* The two lists a record name must be in one of. */
struct eq_rules {
    const char *const *compared;
    size_t n_compared;
    const struct eq_exclusion *excluded;
    size_t n_excluded;
};

static const char why_time[] =
    "the master clock and frame count: the image ran frames, fast boot none";
static const char why_bus[] =
    "the bus stamps and the record of the last instruction, which follow the instructions run";
static const char why_derived[] = "the body checksum, derived from every field";
static const char why_source[] =
    "the boot source and the BIOS digest header fields, which differ by construction";
static const char why_count[] = "a diagnostic counter of the frames the image ran";
static const char why_beam[] = "beam position and interrupt flags, which follow time";
static const char why_port[] =
    "guest-visible VDP port state: the image's own VDP programming can leave a different address, "
    "code, pending command byte and read buffer than fast boot";
static const char why_cpu[] = "hidden Z80 state: the M1 count (R) and the internal registers";
static const char why_out[] =
    "the pictures, samples and renderer scratch of the frames the image ran, not machine state";

/* Every field name a state record holds is in exactly one of these two lists: a name in neither
   fails the comparison naming it, so a field added later cannot be ignored without a decision.
   The lists were derived from a first run and are pinned by EQ_COMPARED_NAMES and
   EQ_EXCLUDED_NAMES. */
static const char *const eq_compared[] = {
    "active_x1",
    "active_x2",
    "active_y1",
    "active_y2",
    "api_version",
    "battery_generation",
    "behaviour_revision",
    "cart_ram",
    "cram_latch",
    "fault_addr",
    "fault_device",
    "fault_dot",
    "fault_frame",
    "fault_kind",
    "fault_line",
    "fault_opcode",
    "fault_pc",
    "fault_present",
    "fault_width",
    "fm_unit",
    "format_version",
    "header_size",
    "input_p1",
    "input_p2",
    "input_pause",
    "input_port1",
    "input_port2",
    "input_read",
    "input_reset",
    "input_start",
    "io_audio_ctrl",
    "io_ctrl",
    "io_gg",
    "io_glasses",
    "io_glasses_frame",
    "io_mem_ctrl",
    "machine",
    "magic",
    "mapper_bank",
    "mapper_board",
    "mapper_ctrl",
    "pending_x1",
    "pending_x2",
    "pending_y1",
    "pending_y2",
    "port_acc",
    "port_edge",
    "port_gun_ax",
    "port_gun_ay",
    "port_gun_dot",
    "port_gun_line",
    "port_gun_scan",
    "port_gun_sense",
    "port_pos",
    "port_step",
    "port_val",
    "psg_att",
    "psg_latch",
    "psg_noise",
    "psg_period",
    "ram",
    "region",
    "reserved",
    "rom_crc32",
    "total_size",
    "tv",
    "vdp_cram",
    "vdp_display",
    "vdp_height",
    "vdp_hscroll",
    "vdp_latch",
    "vdp_pause",
    "vdp_reg",
    "vdp_vram",
    "vdp_vscroll",
    "video_mode",
    "z80_ei",
    "z80_halted",
    "z80_i",
    "z80_idx",
    "z80_iff1",
    "z80_iff2",
    "z80_im",
    "z80_int_line",
    "z80_nmi_pending",
    "z80_pc",
    "z80_reg",
    "z80_sp",
};

static const struct eq_exclusion eq_excluded[] = {
    {"ticks", why_time},
    {"frame", why_time},
    {"bus_deadline", why_bus},
    {"bus_at", why_bus},
    {"bus_vdp_at", why_bus},
    {"bus_line", why_bus},
    {"bus_t", why_bus},
    {"bus_op_pc", why_bus},
    {"bus_op", why_bus},
    {"bus_data", why_bus},
    {"body_crc32", why_derived},
    {"boot", why_source},
    {"bios_crc32", why_source},
    {"lag_frames", why_count},
    {"frame_ints", why_count},
    {"line_ints", why_count},
    {"nmis", why_count},
    {"open_bus_reads", why_count},
    {"audio_dropped", why_count},
    {"vdp_writes_active", why_count},
    {"vdp_fast_accesses", why_count},
    {"sprite_overflow_lines", why_count},
    {"collision_frames", why_count},
    {"vdp_vline", why_beam},
    {"vdp_hcounter", why_beam},
    {"vdp_hheld", why_beam},
    {"vdp_line_counter", why_beam},
    {"vdp_line_pending", why_beam},
    {"vdp_status", why_beam},
    {"vdp_fifth_index", why_beam},
    {"vdp_addr", why_port},
    {"vdp_code", why_port},
    {"vdp_first", why_port},
    {"vdp_buffer", why_port},
    {"z80_r", why_cpu},
    {"z80_wz", why_cpu},
    {"z80_p", why_cpu},
    {"z80_q", why_cpu},
    {"render_line", why_out},
    {"render_x", why_out},
    {"sprite_attr", why_out},
    {"sprite_count", why_out},
    {"sprite_legacy", why_out},
    {"sprite_line", why_out},
    {"sprite_overflow", why_out},
    {"sprite_row", why_out},
    {"sprite_scanned", why_out},
    {"sprite_shift", why_out},
    {"sprite_size", why_out},
    {"sprite_tile", why_out},
    {"sprite_x", why_out},
    {"sprite_zoom", why_out},
    {"synth_dc", why_out},
    {"synth_gained", why_out},
    {"synth_head", why_out},
    {"synth_integ", why_out},
    {"synth_level", why_out},
    {"synth_phase", why_out},
    {"synth_ring", why_out},
    {"completed_backdrop", why_out},
    {"completed_height", why_out},
    {"completed_pixel", why_out},
    {"field_pixel", why_out},
    {"lcd_backdrop", why_out},
    {"lcd_frozen", why_out},
    {"lcd_height", why_out},
    {"lcd_pixel", why_out},
    {"lcd_y", why_out},
    {"psg_at", why_out},
    {"psg_counter", why_out},
    {"psg_lfsr", why_out},
    {"psg_noise_div", why_out},
    {"psg_out", why_out},
};

/* The fields the comparison exists for: each must be compared, never excluded. The work RAM is
   compared everywhere except the named scratch window, work RAM bytes EQ_WINDOW_FIRST to
   EQ_WINDOW_LAST, where the image keeps its code, its results and its stack. */
static const char *const eq_required[EQ_REQUIRED_NAMES] = {
    "z80_reg",  "z80_idx",  "z80_sp",      "z80_pc",   "z80_i",       "z80_im",
    "z80_iff1", "z80_iff2", "io_mem_ctrl", "io_ctrl",  "mapper_ctrl", "mapper_bank",
    "ram",      "vdp_reg",  "vdp_cram",    "vdp_vram",
};

static const struct eq_rules eq_good = {
    eq_compared,
    sizeof eq_compared / sizeof eq_compared[0],
    eq_excluded,
    sizeof eq_excluded / sizeof eq_excluded[0],
};

/* How a rule set can be wrong, as bits. */
#define EQ_PIN_COMPARED 1u
#define EQ_PIN_EXCLUDED 2u
#define EQ_PIN_REQUIRED 4u
#define EQ_PIN_REASON 8u
#define EQ_PIN_BOTH 16u

static int eq_in(const char *name, const char *const *list, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(name, list[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int eq_in_excluded(const struct eq_rules *rules, const char *name)
{
    for (size_t i = 0; i < rules->n_excluded; i++) {
        if (strcmp(name, rules->excluded[i].name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* The counts are literals, every required name is compared and none is excluded, no name is in
   both lists and every excluded name has a reason. */
static uint32_t eq_pins(const struct eq_rules *rules)
{
    uint32_t bad = 0;
    if (rules->n_compared != EQ_COMPARED_NAMES) {
        bad |= EQ_PIN_COMPARED;
    }
    if (rules->n_excluded != EQ_EXCLUDED_NAMES) {
        bad |= EQ_PIN_EXCLUDED;
    }
    for (size_t i = 0; i < EQ_REQUIRED_NAMES; i++) {
        if (!eq_in(eq_required[i], rules->compared, rules->n_compared)) {
            bad |= EQ_PIN_REQUIRED;
        }
    }
    for (size_t i = 0; i < rules->n_excluded; i++) {
        if (rules->excluded[i].why == NULL || rules->excluded[i].why[0] == '\0') {
            bad |= EQ_PIN_REASON;
        }
        if (eq_in(rules->excluded[i].name, rules->compared, rules->n_compared)) {
            bad |= EQ_PIN_BOTH;
        }
    }
    return bad;
}

/* 1 compared, 0 excluded, -1 in neither list. */
static int eq_class(const struct eq_rules *rules, const char *name)
{
    if (eq_in(name, rules->compared, rules->n_compared)) {
        return 1;
    }
    return eq_in_excluded(rules, name) ? 0 : -1;
}

struct eq_result {
    const char *names[EQ_NAMES_MAX]; /* the compared fields that differ, each once */
    size_t count;
    const char *unclassified; /* the first record name in neither list, or NULL */
};

static void eq_note(struct eq_result *out, const char *name)
{
    for (size_t i = 0; i < out->count && i < EQ_NAMES_MAX; i++) {
        if (strcmp(out->names[i], name) == 0) {
            return;
        }
    }
    if (out->count < EQ_NAMES_MAX) {
        out->names[out->count] = name;
    }
    out->count++;
}

/* Walks n records and compares the two saved states by field name. */
static void eq_compare(const struct eq_rules *rules, size_t n, const uint8_t *sa, const uint8_t *sb,
                       struct eq_result *out)
{
    const char *last = "";
    int cls = 0;
    int is_ram = 0;
    int have_ram = 0;
    uint32_t ram_base = 0;
    memset(out, 0, sizeof *out);
    for (size_t i = 0; i < n; i++) {
        const struct cogmaster__state_field *f = &eq_fields[i];
        if (strcmp(f->name, last) != 0) {
            last = f->name;
            cls = eq_class(rules, last);
            is_ram = strcmp(last, "ram") == 0;
        }
        if (is_ram && !have_ram) {
            have_ram = 1;
            ram_base = f->offset;
        }
        if (cls < 0) {
            if (out->unclassified == NULL) {
                out->unclassified = f->name;
            }
            continue;
        }
        if (cls == 0 || memcmp(sa + f->offset, sb + f->offset, f->width) == 0) {
            continue;
        }
        if (is_ram && f->offset - ram_base >= EQ_WINDOW_FIRST &&
            f->offset - ram_base <= EQ_WINDOW_LAST) {
            continue;
        }
        eq_note(out, f->name);
    }
}

/* Prints a result in the one format the bad-image controls anchor to; true when it is clean. */
static int eq_report(const struct eq_result *res)
{
    if (res->unclassified != NULL) {
        fprintf(stderr, "equivalence: unclassified field %s\n", res->unclassified);
        check_failures++;
    }
    if (res->count != 0) {
        fprintf(stderr, "equivalence: differing fields [");
        for (size_t i = 0; i < res->count && i < EQ_NAMES_MAX; i++) {
            fprintf(stderr, "%s%s", i == 0 ? "" : ",", res->names[i]);
        }
        fprintf(stderr, "]\n");
        check_failures++;
    }
    return res->count == 0 && res->unclassified == NULL;
}

/* Whether the result is exactly the one differing field name, and nothing unclassified. */
static int eq_is_only(const struct eq_result *res, const char *name)
{
    return res->unclassified == NULL && res->count == 1 && strcmp(res->names[0], name) == 0;
}

static int eq_reached(const cogmaster *m)
{
    return m->z80.pc == 0u && m->io.mem_ctrl == 0xABu;
}

/* Steps instance m with the Z80 until it is at address 0 with $3E at $AB, closing each frame the
   way cogmaster_run_frame does (a frame already past its deadline runs no instruction), so every
   field stays what a host could have saved. 1 when reached, 0 after bound steps. */
static int eq_run(cogmaster *m, uint32_t bound)
{
    uint32_t steps = 0;
    for (;;) {
        while (!eq_reached(m) && m->ticks < m->bus.deadline) {
            if (steps++ >= bound) {
                return 0;
            }
            cogmaster__z80_step(m);
        }
        if (eq_reached(m)) {
            return 1;
        }
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
}

static size_t eq_read(const char *path, uint8_t *buf, size_t want)
{
    FILE *f = path == NULL ? NULL : fopen(path, "rb");
    size_t n = 0;
    if (f != NULL) {
        n = fread(buf, 1u, want + 1u, f);
        fclose(f);
    }
    return n;
}

/* Typed from the hand-off table of SMS-SYSTEM C7: $3E $AB written to the port and stored at the
   top of work RAM, $3F $FF, SP $DFF0, AF $FFFF, the mapper at 0, 0, 1, 2 and PC 0. */
static void eq_literals(const cogmaster *m, const char *which)
{
    int bad = check_failures;
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xABu);
    CHECK_EQ_HEX(m->ram[0], 0xABu);
    CHECK_EQ_HEX(m->io.io_ctrl, 0xFFu);
    CHECK_EQ_HEX(m->z80.sp, 0xDFF0u);
    CHECK_EQ_HEX(m->z80.reg[COGMASTER__Z80_A], 0xFFu);
    CHECK_EQ_HEX(m->z80.reg[COGMASTER__Z80_F], 0xFFu);
    CHECK_EQ_HEX(m->mapper.ctrl, 0u);
    CHECK_EQ_HEX(m->mapper.bank[0], 0u);
    CHECK_EQ_HEX(m->mapper.bank[1], 1u);
    CHECK_EQ_HEX(m->mapper.bank[2], 2u);
    CHECK_EQ_HEX(m->z80.pc, 0u);
    if (check_failures != bad) {
        fprintf(stderr, "equivalence: the %s instance is not the hand-off of SMS-SYSTEM C7\n",
                which);
    }
}

/* What the image recorded while it walked the enables, typed from SMS-SYSTEM section 3: BIOS on
   and cartridge off reads the BIOS's own bytes (the image's second byte is $C3, the probe's is
   $76, so the two sources differ); neither reads open bus, the retained byte $00 on an SMS and
   the pull-up $FF on an SMS 2 (C5); cartridge on and BIOS off reads the probe's bytes; and each
   write then read of $FFFC to $FFFF gives back $20 to $23, as work RAM answers the mirror. */
static void eq_walk_results(const cogmaster *b, uint32_t machine, const char *name)
{
    uint8_t open = machine == COGMASTER_MACHINE_SMS1 ? 0x00u : 0xFFu;
    const uint8_t want[EQ_RESULT_BYTES] = {0xF3u, 0xC3u, 0x20u, 0x21u, 0x22u, 0x23u,
                                           open,  open,  0x20u, 0x21u, 0x22u, 0x23u,
                                           0xF3u, 0x76u, 0x20u, 0x21u, 0x22u, 0x23u};
    for (uint32_t i = 0; i < EQ_RESULT_BYTES; i++) {
        if (b->ram[EQ_RESULTS + i] != want[i]) {
            fprintf(stderr, "equivalence: %s walk result %u is %02x, want %02x\n", name, i,
                    (unsigned)b->ram[EQ_RESULTS + i], (unsigned)want[i]);
            check_failures++;
        }
    }
}

/* One mutation of the fast-boot instance, which the comparator must name exactly. */
static void eq_mutation(cogmaster *a, size_t n, const uint8_t *sb, uint8_t *target, uint8_t bit,
                        const char *name)
{
    struct eq_result res;
    *target ^= bit;
    CHECK_EQ_U64(cogmaster__state_save(a, state_a, STATE_BUF), COGMASTER_STATUS_OK);
    eq_compare(&eq_good, n, state_a, sb, &res);
    if (name == NULL ? res.count != 0 || res.unclassified != NULL : !eq_is_only(&res, name)) {
        fprintf(stderr, "equivalence control: a change in %s was not reported as exactly that\n",
                name == NULL ? "an excluded field" : name);
        check_failures++;
    }
    *target ^= bit;
}

static void eq_controls(cogmaster *a, size_t n, const uint8_t *sb)
{
    /* Each required field changed in the fast-boot instance is named, and only it. */
    eq_mutation(a, n, sb, &a->ram[0], 1u, "ram");
    eq_mutation(a, n, sb, &a->mapper.bank[1], 1u, "mapper_bank");
    eq_mutation(a, n, sb, &a->mapper.ctrl, 1u, "mapper_ctrl");
    eq_mutation(a, n, sb, &a->io.mem_ctrl, 1u, "io_mem_ctrl");
    eq_mutation(a, n, sb, &a->io.io_ctrl, 1u, "io_ctrl");
    eq_mutation(a, n, sb, &a->vdp.reg[1], 1u, "vdp_reg");
    eq_mutation(a, n, sb, &a->vdp.vram[5], 1u, "vdp_vram");
    eq_mutation(a, n, sb, &a->vdp.cram[3], 1u, "vdp_cram");
    eq_mutation(a, n, sb, &a->z80.i, 1u, "z80_i");
    eq_mutation(a, n, sb, &a->z80.im, 1u, "z80_im");
    eq_mutation(a, n, sb, &a->z80.iff1, 1u, "z80_iff1");
    eq_mutation(a, n, sb, &a->z80.reg[COGMASTER__Z80_B], 1u, "z80_reg");
    eq_mutation(a, n, sb, &a->z80.idx, 1u, "z80_idx");
    {
        uint16_t sp = a->z80.sp;
        a->z80.sp = (uint16_t)(sp ^ 1u);
        struct eq_result res;
        CHECK_EQ_U64(cogmaster__state_save(a, state_a, STATE_BUF), COGMASTER_STATUS_OK);
        eq_compare(&eq_good, n, state_a, sb, &res);
        CHECK(eq_is_only(&res, "z80_sp"));
        a->z80.sp = sp;
        uint16_t pc = a->z80.pc;
        a->z80.pc = (uint16_t)(pc ^ 1u);
        CHECK_EQ_U64(cogmaster__state_save(a, state_a, STATE_BUF), COGMASTER_STATUS_OK);
        eq_compare(&eq_good, n, state_a, sb, &res);
        CHECK(eq_is_only(&res, "z80_pc"));
        a->z80.pc = pc;
    }
    /* The scratch window edges: the byte before and after the window are compared, the first and
       last byte inside it are not. */
    eq_mutation(a, n, sb, &a->ram[EQ_WINDOW_FIRST - 1u], 1u, "ram");
    eq_mutation(a, n, sb, &a->ram[EQ_WINDOW_LAST + 1u], 1u, "ram");
    eq_mutation(a, n, sb, &a->ram[EQ_WINDOW_FIRST], 1u, NULL);
    eq_mutation(a, n, sb, &a->ram[EQ_WINDOW_LAST], 1u, NULL);
    /* The named exceptions are never reported. */
    eq_mutation(a, n, sb, &a->z80.r, 1u, NULL);
    eq_mutation(a, n, sb, &a->vdp.buffer, 1u, NULL);
    eq_mutation(a, n, sb, &a->vdp.code, 1u, NULL);
    eq_mutation(a, n, sb, &a->vdp.first, 1u, NULL);
}

/* The rules themselves: a name in neither list is named, and a rule set that moves or adds a
   required name fails a pin. */
static void eq_rule_controls(size_t n, const uint8_t *sa, const uint8_t *sb)
{
    struct eq_result res;
    struct eq_rules rules = eq_good;
    CHECK_EQ_U64(eq_pins(&eq_good), 0u);
    /* A compared list without vdp_reg leaves it unclassified, named. */
    static const char *shorter[EQ_COMPARED_NAMES];
    size_t kept = 0;
    for (size_t i = 0; i < eq_good.n_compared; i++) {
        if (strcmp(eq_good.compared[i], "vdp_reg") != 0) {
            shorter[kept++] = eq_good.compared[i];
        }
    }
    rules.compared = shorter;
    rules.n_compared = kept;
    eq_compare(&rules, n, sa, sb, &res);
    CHECK(res.unclassified != NULL && strcmp(res.unclassified, "vdp_reg") == 0);
    CHECK_EQ_U64(eq_pins(&rules), EQ_PIN_COMPARED | EQ_PIN_REQUIRED);
    /* An excluded list extended by one required name fails the pinned count (and the overlap). */
    static struct eq_exclusion longer[EQ_EXCLUDED_NAMES + 1u];
    memcpy(longer, eq_excluded, sizeof eq_excluded);
    longer[EQ_EXCLUDED_NAMES].name = "z80_sp";
    longer[EQ_EXCLUDED_NAMES].why = why_out;
    rules = eq_good;
    rules.excluded = longer;
    rules.n_excluded = EQ_EXCLUDED_NAMES + 1u;
    CHECK_EQ_U64(eq_pins(&rules), EQ_PIN_EXCLUDED | EQ_PIN_BOTH);
    /* A required name moved from compared to excluded fails three pins at once. */
    static const char *moved[EQ_COMPARED_NAMES];
    kept = 0;
    for (size_t i = 0; i < eq_good.n_compared; i++) {
        if (strcmp(eq_good.compared[i], "z80_sp") != 0) {
            moved[kept++] = eq_good.compared[i];
        }
    }
    rules.compared = moved;
    rules.n_compared = kept;
    CHECK_EQ_U64(eq_pins(&rules), EQ_PIN_COMPARED | EQ_PIN_EXCLUDED | EQ_PIN_REQUIRED);
    /* An excluded entry with no reason fails. */
    longer[0].why = "";
    rules = eq_good;
    rules.excluded = longer;
    CHECK_EQ_U64(eq_pins(&rules), EQ_PIN_REASON);
}

static void eq_leg(uint32_t machine)
{
    cogmaster *a = blank();
    cogmaster *b = blank();
    cogmaster *c = blank();
    cogmaster *d = blank();
    cogmaster_media ma = media_of(eq_probe, EQ_PROBE_SIZE, machine, NULL, 0u);
    cogmaster_media mb = media_of(eq_probe, EQ_PROBE_SIZE, machine, eq_bios, EQ_BIOS_SIZE);
    CHECK_EQ_U64(cogmaster_load(a, &ma), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_load(b, &mb), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_load(c, &mb), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_load(d, &mb), COGMASTER_STATUS_OK);
    if (!eq_run(b, EQ_STEP_BOUND)) {
        fprintf(stderr,
                "equivalence: the image never reached address 0 with $3E at AB in %u steps\n",
                EQ_STEP_BOUND);
        check_failures++;
    } else {
        /* The stepped state is one a host could have saved: a third instance accepts it. */
        size_t size = save_to(b, state_b);
        CHECK_EQ_U64(cogmaster__state_load(c, state_b, size), COGMASTER_STATUS_OK);
        /* The image ran its two frames of backdrop before it handed off. */
        CHECK(b->frame >= 2u);
        eq_literals(a, "fast boot");
        eq_literals(b, "BIOS");
        eq_walk_results(b, machine, "BIOS");
        size_t n = cogmaster__state_record(a, eq_fields, EQ_MAX_FIELDS);
        CHECK(n != 0u && n <= EQ_MAX_FIELDS);
        CHECK_EQ_U64(save_to(a, state_a), size);
        uint32_t pins = eq_pins(&eq_good);
        if (pins != 0u) {
            fprintf(stderr,
                    "equivalence: the pinned lists are wrong (code %u): %zu compared names, want "
                    "%u, and %zu excluded, want %u\n",
                    pins, eq_good.n_compared, EQ_COMPARED_NAMES, eq_good.n_excluded,
                    EQ_EXCLUDED_NAMES);
            check_failures++;
        }
        struct eq_result res;
        eq_compare(&eq_good, n, state_a, state_b, &res);
        /* The controls expect a clean baseline, so a bad image stops at its own report. */
        if (eq_report(&res)) {
            eq_controls(a, n, state_b);
            eq_rule_controls(n, state_a, state_b);
        }
        /* A bound too short for the image is a named failure, not a hang. */
        CHECK_EQ_U64(eq_run(d, EQ_SHORT_BOUND), 0u);
    }
    cogmaster_destroy(a);
    cogmaster_destroy(b);
    cogmaster_destroy(c);
    cogmaster_destroy(d);
}

/* Fault prevented: a fast-boot hand-off that drifts from the state the hand-off facts describe, a
   comparator that ignores a field silently, and a vacuous equivalence that passes because both
   sides came from the same constants. */
static void test_equivalence(void)
{
    if (eq_read(eq_probe_path, eq_probe, EQ_PROBE_SIZE) != EQ_PROBE_SIZE ||
        eq_read(eq_bios_path, eq_bios, EQ_BIOS_SIZE) != EQ_BIOS_SIZE) {
        fprintf(stderr, "equivalence: the probe and the image must be %u and %u bytes\n",
                EQ_PROBE_SIZE, EQ_BIOS_SIZE);
        check_failures++;
        return;
    }
    eq_leg(COGMASTER_MACHINE_SMS1);
    eq_leg(COGMASTER_MACHINE_SMS2);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"arbitration", test_arbitration},
    {"slow_flag", test_slow_flag},
    {"load", test_load},
    {"state", test_state},
    {"hostile", test_hostile},
    {"equivalence", test_equivalence},
};

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "usage: cogmaster-test-bios <case> [<probe cartridge> <BIOS image>]\n");
        return 2;
    }
    eq_probe_path = argc == 4 ? argv[2] : NULL;
    eq_bios_path = argc == 4 ? argv[3] : NULL;
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-bios: unknown case %s\n", argv[1]);
    return 2;
}
