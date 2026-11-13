/* The Korean, Korean no-bank, Hi-Com, Zemina, Nemesis and 4 PAK boards at register level: an image
   of exactly the size under test is installed on the board, every register value is written through
   the bus, and each bank and each read is compared with a model kept here from the writes alone:
   cogmaster-test-mapper-ext <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define SLOT 8192u

static const uint32_t sizes_kib[6] = {32u, 48u, 128u, 432u, 800u, 1024u};

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

/* A byte of the image: a multiplicative hash of its offset, so neighbours and
   the same offset in different banks all differ. */
static uint8_t rom_byte(uint32_t i)
{
    return (uint8_t)((i * 2654435761u) >> 24);
}

/* What the board should show, worked out from the writes alone. */
struct model {
    uint32_t board, pages;
    uint32_t bank[6];
    uint32_t raw[2];
};

static uint32_t reg_addr(uint32_t board)
{
    return board == COGMASTER__BOARD_HICOM ? 0xFFFFu : 0xA000u;
}

static int model_pow2(const struct model *g)
{
    return (g->pages & (g->pages - 1u)) == 0u;
}

static void model_reset(struct model *g)
{
    for (uint32_t i = 0; i < 6u; i++) {
        uint32_t window = g->board == COGMASTER__BOARD_HICOM && i >= 4u ? i - 4u : i;
        g->bank[i] = window % (2u * g->pages);
        if (g->board == COGMASTER__BOARD_4PAK) {
            g->bank[i] = i & 1u;
        }
    }
    if (g->board == COGMASTER__BOARD_NEMESIS) {
        g->bank[0] = 2u * g->pages - 1u;
    }
    g->raw[0] = 0u;
    g->raw[1] = 0u;
}

/* 4 PAK: the 16 KiB slot s shows page val modulo the page count. */
static int model_slot(struct model *g, uint32_t s, uint32_t val)
{
    g->bank[2u * s] = 2u * (val % g->pages);
    g->bank[2u * s + 1u] = 2u * (val % g->pages) + 1u;
    return val >= g->pages && !model_pow2(g);
}

/* A write of v to addr. Korean picks 16 KiB page v mod pages for $8000-$BFFF,
   Hi-Com 32 KiB bank v for $0000-$7FFF, Zemina and Nemesis one 8 KiB bank of
   four registers, the 4 PAK 16 KiB pages as its registers say; all modulo the
   image. Returns 1 when the write selects a bank past an image whose page
   count is not a power of two, which cannot wrap by dropping high bits. */
static int model_write(struct model *g, uint32_t addr, uint32_t v)
{
    if (g->board == COGMASTER__BOARD_KOREAN) {
        g->bank[4] = 2u * (v % g->pages);
        g->bank[5] = 2u * (v % g->pages) + 1u;
        return v >= g->pages && !model_pow2(g);
    }
    if (g->board == COGMASTER__BOARD_HICOM) {
        for (uint32_t i = 0; i < 4u; i++) {
            g->bank[i] = (4u * v + i) % (2u * g->pages);
        }
        return 4u * v + 3u >= 2u * g->pages && !model_pow2(g);
    }
    if (g->board == COGMASTER__BOARD_ZEMINA || g->board == COGMASTER__BOARD_NEMESIS) {
        static const uint32_t window[4] = {4u, 5u, 2u, 3u};
        g->bank[window[addr]] = v % (2u * g->pages);
        return v >= 2u * g->pages && !model_pow2(g);
    }
    if (addr == 0x3FFEu) {
        g->raw[0] = v;
        int a = model_slot(g, 0u, v);
        return model_slot(g, 2u, (g->raw[0] & 0x30u) + g->raw[1]) | a;
    }
    if (addr == 0x7FFFu) {
        return model_slot(g, 1u, v);
    }
    if (addr == 0xBFFFu) {
        g->raw[1] = v;
        return model_slot(g, 2u, (g->raw[0] & 0x30u) + g->raw[1]);
    }
    return addr >= 0xFFFDu ? model_slot(g, addr - 0xFFFDu, v) : 0;
}

struct rig {
    cogmaster *m;
    uint8_t *img;
    struct model g;
};

/* A Master System running a 32 KiB image, whose cartridge is then replaced by
   one of kib KiB on the given board. The image buffer is exactly its size, so
   a read past it is an address-sanitizer error. */
static void rig_open(struct rig *r, uint32_t board, uint32_t kib)
{
    static uint8_t boot[32u * 1024u];
    memset(boot, 0xFF, sizeof boot);
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = boot;
    media.image_size = sizeof boot;
    media.machine = COGMASTER_MACHINE_SMS2;
    r->m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &r->m), COGMASTER_STATUS_OK);
    if (r->m == NULL || cogmaster_load(r->m, &media) != COGMASTER_STATUS_OK) {
        exit(1);
    }
    uint32_t size = kib * 1024u;
    r->img = (uint8_t *)malloc(size);
    if (r->img == NULL) {
        exit(1);
    }
    for (uint32_t i = 0; i < size; i++) {
        r->img[i] = rom_byte(i);
    }
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.image_size = size;
    info.board = board;
    struct cogmaster__cart cart;
    CHECK_EQ_U64(cogmaster__cart_copy(r->m, r->img, size, &info, &cart), COGMASTER_STATUS_OK);
    cogmaster__cart_install(r->m, &cart);
    CHECK_EQ_U64(cogmaster_reset(r->m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
    r->g.board = board;
    r->g.pages = size / (2u * SLOT);
    model_reset(&r->g);
}

static void rig_close(struct rig *r)
{
    cogmaster_destroy(r->m);
    free(r->img);
}

static uint8_t want_byte(const struct rig *r, uint16_t addr)
{
    return r->img[r->g.bank[addr / SLOT] * SLOT + (addr % SLOT)];
}

/* Every bank is below twice the page count and equals the model. */
static void check_banks(const struct rig *r)
{
    for (uint32_t i = 0; i < 6u; i++) {
        CHECK(r->m->mapper.bank8[i] < 2u * r->g.pages);
        CHECK_EQ_U64(r->m->mapper.bank8[i], r->g.bank[i]);
    }
    if (r->g.board == COGMASTER__BOARD_4PAK) {
        CHECK_EQ_U64(r->m->mapper.raw[0], r->g.raw[0]);
        CHECK_EQ_U64(r->m->mapper.raw[1], r->g.raw[1]);
    }
}

/* The first and last byte of each 8 KiB window, through the bus. */
static void check_ends(const struct rig *r)
{
    uint32_t wrong = 0;
    for (uint32_t s = 0; s < 6u; s++) {
        uint16_t first = (uint16_t)(s * SLOT);
        uint16_t last = (uint16_t)(first + SLOT - 1u);
        wrong += cogmaster__bus_read(r->m, first) != want_byte(r, first);
        wrong += cogmaster__bus_read(r->m, last) != want_byte(r, last);
    }
    CHECK_EQ_U64(wrong, 0);
}

/* The low 13 address bits of every bank. */
static void sweep(const struct rig *r)
{
    uint32_t wrong = 0;
    for (uint32_t a = 0; a < 6u * SLOT; a++) {
        wrong += cogmaster__mapper_read(r->m, (uint16_t)a) != want_byte(r, (uint16_t)a);
    }
    CHECK_EQ_U64(wrong, 0);
}

static cogmaster_fault fault_of(const cogmaster *m)
{
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
    return f;
}

static void expect_fault(const cogmaster *m, uint32_t kind, uint32_t device, uint32_t addr)
{
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 1);
    CHECK_EQ_U64(f.kind, kind);
    CHECK_EQ_U64(f.device, device);
    CHECK_EQ_HEX(f.addr, addr);
}

/* Every value 0 to 255 written to the register (the no-bank board has none, so
   writes to the addresses the other boards use change nothing), with the banks
   and reads checked after each, then the writes that are not registers. */
static void run_board(uint32_t board)
{
    for (uint32_t k = 0; k < 6u; k++) {
        struct rig r;
        rig_open(&r, board, sizes_kib[k]);
        int full = sizes_kib[k] == 128u || sizes_kib[k] == 432u;
        check_banks(&r);
        check_ends(&r);
        if (full) {
            sweep(&r);
        }
        int seen_over = 0;
        for (uint32_t v = 0; v < 256u; v++) {
            if (board == COGMASTER__BOARD_KOREAN_NB) {
                cogmaster__bus_write(r.m, 0xA000u, (uint8_t)v);
                cogmaster__bus_write(r.m, 0xFFFFu, (uint8_t)v);
            } else {
                cogmaster__bus_write(r.m, (uint16_t)reg_addr(board), (uint8_t)v);
                seen_over |= model_write(&r.g, reg_addr(board), v);
            }
            if (seen_over) {
                expect_fault(r.m, COGMASTER__FAULT_MAPPER_RANGE, COGMASTER__DEVICE_MAPPER,
                             reg_addr(board));
            } else {
                CHECK_EQ_U64(fault_of(r.m).present, 0);
            }
            check_banks(&r);
            check_ends(&r);
            if (full) {
                sweep(&r);
            }
        }

        /* A power reset clears the fault and the banks. */
        CHECK_EQ_U64(cogmaster_reset(r.m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(fault_of(r.m).present, 0);
        model_reset(&r.g);
        check_banks(&r);

        /* $FFFC-$FFFF are work RAM only, apart from Hi-Com's $FFFF. */
        for (uint32_t a = 0xFFFCu; a <= 0xFFFFu; a++) {
            if (board != COGMASTER__BOARD_HICOM || a != 0xFFFFu) {
                cogmaster__bus_write(r.m, (uint16_t)a, 3u);
            }
        }
        check_banks(&r);
        CHECK_EQ_U64(fault_of(r.m).present, 0);

        /* Writes to ROM that are not a register change no bank and record no
           fault. */
        static const uint16_t other[6] = {0x1234u, 0x0000u, 0x4000u, 0x8000u, 0xA001u, 0xBFFFu};
        for (uint32_t i = 0; i < 6u; i++) {
            cogmaster__bus_write(r.m, other[i], 3u);
            check_banks(&r);
        }
        CHECK_EQ_U64(fault_of(r.m).present, 0);
        if (board == COGMASTER__BOARD_KOREAN) {
            cogmaster__bus_write(r.m, 0x9FFFu, 3u);
            check_banks(&r);
        }
        if (board == COGMASTER__BOARD_HICOM) {
            cogmaster__bus_write(r.m, 0xA000u, 3u);
            check_banks(&r);
        }
        rig_close(&r);
    }
}

static void test_korean(void)
{
    run_board(COGMASTER__BOARD_KOREAN);
}

static void test_korean_nb(void)
{
    run_board(COGMASTER__BOARD_KOREAN_NB);
}

static void test_hicom(void)
{
    run_board(COGMASTER__BOARD_HICOM);
}

/* One write through the bus and the same write in the model; the first write
   that selects a bank past the image fixes the address the fault must name. */
static void write_reg(struct rig *r, uint32_t addr, uint32_t v, uint32_t *first_over)
{
    cogmaster__bus_write(r->m, (uint16_t)addr, (uint8_t)v);
    if (model_write(&r->g, addr, v) && *first_over == 0xFFFFFFFFu) {
        *first_over = addr;
    }
}

/* Every value 0 to 255 written to each register of a board with several, with
   the banks, the reads and the fault record checked after each write. The 4
   PAK slot 2 depends on both $3FFE and $BFFF, so $BFFF is swept under each of
   the four settings of the $3FFE bits 5 and 4. The write is a power-on away
   from the previous register's, so a fault from one never hides another. */
static void run_regs(uint32_t board)
{
    static const uint16_t zem[4] = {0x0000u, 0x0001u, 0x0002u, 0x0003u};
    static const uint16_t pak[7] = {0x3FFEu, 0x7FFFu, 0xBFFFu, 0xFFFCu, 0xFFFDu, 0xFFFEu, 0xFFFFu};
    int is_pak = board == COGMASTER__BOARD_4PAK;
    const uint16_t *regs = is_pak ? pak : zem;
    uint32_t nregs = is_pak ? 7u : 4u;
    for (uint32_t k = 0; k < 6u; k++) {
        struct rig r;
        rig_open(&r, board, sizes_kib[k]);
        int full = sizes_kib[k] == 128u || sizes_kib[k] == 432u;
        uint32_t last = 2u * r.g.pages - 1u;
        CHECK_EQ_U64(r.m->mapper.bank8[0], board == COGMASTER__BOARD_NEMESIS ? last : 0u);
        check_banks(&r);
        check_ends(&r);
        if (full) {
            sweep(&r);
        }
        for (uint32_t i = 0; i < nregs; i++) {
            uint32_t settings = regs[i] == 0xBFFFu ? 4u : 1u;
            for (uint32_t st = 0; st < settings; st++) {
                CHECK_EQ_U64(cogmaster_reset(r.m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
                model_reset(&r.g);
                uint32_t first_over = 0xFFFFFFFFu;
                if (settings == 4u) {
                    write_reg(&r, 0x3FFEu, st << 4, &first_over);
                }
                for (uint32_t v = 0; v < 256u; v++) {
                    write_reg(&r, regs[i], v, &first_over);
                    if (first_over != 0xFFFFFFFFu) {
                        expect_fault(r.m, COGMASTER__FAULT_MAPPER_RANGE, COGMASTER__DEVICE_MAPPER,
                                     first_over);
                    } else {
                        CHECK_EQ_U64(fault_of(r.m).present, 0);
                    }
                    check_banks(&r);
                    check_ends(&r);
                    if (full) {
                        sweep(&r);
                    }
                }
            }
        }

        /* A power reset clears the fault and the banks. */
        CHECK_EQ_U64(cogmaster_reset(r.m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(fault_of(r.m).present, 0);
        model_reset(&r.g);
        check_banks(&r);

        /* Writes to ROM that are not a register change no bank; the first of
           them, $1234, is the one the fault record names. Writes to $FFFC
           (and on the Zemina boards $FFFD-$FFFF) are work RAM only. */
        for (uint32_t a = 0xFFFCu; a <= 0xFFFFu; a++) {
            if (!is_pak || a == 0xFFFCu) {
                cogmaster__bus_write(r.m, (uint16_t)a, 3u);
            }
        }
        check_banks(&r);
        CHECK_EQ_U64(fault_of(r.m).present, 0);
        static const uint16_t other[9] = {0x1234u, 0x0000u, 0x4000u, 0x8000u, 0xA000u,
                                          0xBFFEu, 0x3FFFu, 0x7FFEu, 0x0004u};
        for (uint32_t i = 0; i < 9u; i++) {
            if (is_pak || other[i] != 0x0000u) {
                cogmaster__bus_write(r.m, other[i], 3u);
                check_banks(&r);
            }
        }
        CHECK_EQ_U64(fault_of(r.m).present, 0);
        rig_close(&r);
    }
}

static void test_zemina(void)
{
    run_regs(COGMASTER__BOARD_ZEMINA);
}

static void test_nemesis(void)
{
    run_regs(COGMASTER__BOARD_NEMESIS);
}

/* The 4 PAK registers swept as above, then the $FFFD-$FFFF reading kept
   apart: a write there changes no raw register, and a later $3FFE or $BFFF
   write works slot 2 out from raw as usual. */
static void test_hes_4pak(void)
{
    run_regs(COGMASTER__BOARD_4PAK);

    struct rig r;
    rig_open(&r, COGMASTER__BOARD_4PAK, 128u);
    uint32_t none = 0xFFFFFFFFu;
    write_reg(&r, 0xBFFFu, 5u, &none);
    write_reg(&r, 0x3FFEu, 0x10u, &none);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 10);
    CHECK_EQ_U64(r.m->mapper.bank8[5], 11);
    write_reg(&r, 0xFFFFu, 2u, &none);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 4);
    CHECK_EQ_U64(r.m->mapper.raw[0], 0x10);
    CHECK_EQ_U64(r.m->mapper.raw[1], 5);
    write_reg(&r, 0x3FFEu, 0x10u, &none);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 10);
    CHECK_EQ_HEX(cogmaster__bus_read(r.m, 0x8000u), r.img[10u * SLOT]);
    CHECK_EQ_U64(fault_of(r.m).present, 0);

    /* The state round-trips raw: after a reload, slot 2 follows the saved
       $3FFE and $BFFF values. */
    size_t n = cogmaster__state_size(r.m);
    uint8_t *buf = (uint8_t *)malloc(n);
    if (n == 0u || buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(r.m, buf, n), COGMASTER_STATUS_OK);
    write_reg(&r, 0x3FFEu, 0x30u, &none);
    write_reg(&r, 0xBFFFu, 1u, &none);
    CHECK_EQ_U64(cogmaster__state_load(r.m, buf, n), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(r.m->mapper.raw[0], 0x10);
    CHECK_EQ_U64(r.m->mapper.raw[1], 5);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 10);
    write_reg(&r, 0x7FFFu, 0u, &none);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 10);
    cogmaster__bus_write(r.m, 0x3FFEu, 0x10u);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 10);
    free(buf);
    rig_close(&r);
}

/* The bank state round-trips, and a state whose bank is past the image is
   refused. */
static void test_state(void)
{
    struct rig r;
    rig_open(&r, COGMASTER__BOARD_KOREAN, 432u);
    size_t n = cogmaster__state_size(r.m);
    uint8_t *buf = (uint8_t *)malloc(n);
    if (n == 0u || buf == NULL) {
        exit(1);
    }
    cogmaster__bus_write(r.m, 0xA000u, 7u);
    CHECK_EQ_U64(cogmaster__state_save(r.m, buf, n), COGMASTER_STATUS_OK);
    cogmaster__bus_write(r.m, 0xA000u, 3u);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 6);
    CHECK_EQ_U64(cogmaster__state_load(r.m, buf, n), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 14);
    CHECK_EQ_U64(r.m->mapper.bank8[5], 15);
    CHECK_EQ_HEX(cogmaster__bus_read(r.m, 0x8000u), r.img[14u * SLOT]);

    size_t fields = cogmaster__state_record(r.m, NULL, 0u);
    struct cogmaster__state_field *rec =
        (struct cogmaster__state_field *)malloc(fields * sizeof *rec);
    if (rec == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_record(r.m, rec, fields), fields);
    uint32_t found = 0;
    for (size_t i = 0; i < fields; i++) {
        if (strcmp(rec[i].name, "mapper_bank8") == 0 && found == 0u) {
            found = 1u;
            CHECK_EQ_U64(rec[i].max, 53);
            buf[rec[i].offset] = 255u;
            cogmaster__state_seal(buf, n);
            CHECK_EQ_U64(cogmaster__state_load(r.m, buf, n), COGMASTER_STATUS_ERR_ARGUMENT);
        }
    }
    CHECK_EQ_U64(found, 1);
    CHECK_EQ_U64(r.m->mapper.bank8[4], 14);
    free(rec);
    free(buf);
    rig_close(&r);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"hes_4pak", test_hes_4pak},   {"hicom", test_hicom},     {"korean", test_korean},
    {"korean_nb", test_korean_nb}, {"nemesis", test_nemesis}, {"state", test_state},
    {"zemina", test_zemina},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-mapper-ext <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-mapper-ext: unknown case %s\n", argv[1]);
    return 2;
}
