/* The boot ROM arbitration on the bus and the derived flag that guards it, through the public API
   with the register and the image set through src/internal.h: cogmaster-test-bios <case>. The
   image is self-authored: 8 KiB of one byte, attached by hand. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define ROM_BYTE 0x0Fu
#define BIOS_BYTE 0xF3u
#define RAM_BYTE 0x5Au
#define PROBES 4u
#define COMBOS 16u
#define MACHINES 2u
#define KINDS 4u
#define IMAGE_SIZE 16384u

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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"arbitration", test_arbitration},
    {"slow_flag", test_slow_flag},
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
