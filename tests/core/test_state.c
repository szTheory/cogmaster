/* The internal state visitor, the first-fault record and reset over the
   test card and over a loaded image: cogmaster-test-state <case>. The tests
   call the visitor and the fault recorder directly; the library has no
   public state API yet. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define BUF 65536u        /* bytes in each state buffer; up to about 57 KiB with cartridge RAM */
#define MAX_FIELDS 60000u /* 8,192 work RAM, 16,384 VRAM and 32,768 cartridge RAM bytes */
#define IMAGE_SIZE 1024u
#define TICKS_PER_FRAME 179208u
#define POISON 0xA5u
#define TAG_BUF 8192u

static uint8_t good[BUF];
static uint8_t bad[BUF];
static uint8_t before[BUF];
static uint8_t after[BUF];
static struct cogmaster__state_field fields[MAX_FIELDS];
static uint8_t tag_buf[TAG_BUF];
static uint8_t tag_before[TAG_BUF];
static uint16_t pixels[256u * 240u];
static uint8_t snapshot[sizeof(struct cogmaster)];
static uint8_t img_loop[IMAGE_SIZE];
static uint8_t img_halt[IMAGE_SIZE];

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

static cogmaster_config make_config(void)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    return cfg;
}

/* A new instance of the profile running the 1 KiB image img, or the test
   card when img is NULL. */
static cogmaster *make_with(const uint8_t *img, uint32_t machine, uint32_t region, uint32_t tv)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = img;
    media.image_size = img != NULL ? IMAGE_SIZE : 0u;
    media.machine = machine;
    media.region = region;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

/* A new instance with the test card of machine loaded. */
static cogmaster *make(uint32_t machine)
{
    return make_with(NULL, machine, COGMASTER_REGION_AUTO, COGMASTER_TV_AUTO);
}

/* A new Master System instance running the 1 KiB image img. */
static cogmaster *make_image(const uint8_t *img)
{
    return make_with(img, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_AUTO, COGMASTER_TV_AUTO);
}

/* An image that keeps changing RAM, registers and flags: a loop that
   increments and stores a counter, so a resumed run that misses any field
   drifts. */
static void fill_images(void)
{
    static const uint8_t prog[] = {
        0xF3u,               /* DI */
        0x21u, 0x00u, 0xC1u, /* LD HL,$C100 */
        0x34u,               /* INC (HL) */
        0x7Eu,               /* LD A,(HL) */
        0x23u,               /* INC HL */
        0x77u,               /* LD (HL),A */
        0xD9u,               /* EXX */
        0x08u,               /* EX AF,AF' */
        0x18u, 0xF8u,        /* JR -8, back to INC (HL) */
    };
    memset(img_loop, 0, sizeof img_loop);
    memcpy(img_loop, prog, sizeof prog);
    memset(img_halt, 0, sizeof img_halt);
    img_halt[0] = 0xF3u; /* DI; HALT */
    img_halt[1] = 0x76u;
}

static void run_frames(cogmaster *m, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
}

/* Saves m into buf and returns the state size, or 0 after a failed check. */
static size_t save(const cogmaster *m, uint8_t *buf)
{
    size_t size = cogmaster__state_size(m);
    CHECK(size > 64u && size <= BUF);
    if (size <= 64u || size > BUF) {
        return 0u;
    }
    CHECK_EQ_U64(cogmaster__state_save(m, buf, size), COGMASTER_STATUS_OK);
    return size;
}

static uint64_t get_le(const uint8_t *p, uint32_t width)
{
    uint64_t v = 0u;
    for (uint32_t i = 0; i < width; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

/* The fields of m's state; fails the test when there are none. */
static size_t record(const cogmaster *m)
{
    size_t n = cogmaster__state_record(m, fields, MAX_FIELDS);
    CHECK(n > 0u && n <= MAX_FIELDS);
    return n <= MAX_FIELDS ? n : 0u;
}

/* The first field named name, or NULL after a failed check. */
static const struct cogmaster__state_field *field(size_t n, const char *name)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            return &fields[i];
        }
    }
    fprintf(stderr, "no state field named %s\n", name);
    check_failures++;
    return NULL;
}

/* Loads the size bytes of bad into target, which must refuse them and still
   save the bytes it saved before. */
static void expect_refused(const char *what, cogmaster *target, size_t size, size_t target_size)
{
    cogmaster_status st = cogmaster__state_load(target, bad, size);
    if (st == COGMASTER_STATUS_OK) {
        fprintf(stderr, "%s: accepted\n", what);
        check_failures++;
    }
    if (save(target, after) != target_size || memcmp(before, after, target_size) != 0) {
        fprintf(stderr, "%s: the refused load changed the instance\n", what);
        check_failures++;
    }
}

/* A state taken mid-frame from a running image, loaded into a fresh
   instance of the same image, resumes exactly: five more frames of each
   save the same bytes. */
static void round_trip_image(uint32_t region, uint32_t tv)
{
    fill_images();
    cogmaster *a = make_with(img_loop, COGMASTER_MACHINE_SMS2, region, tv);
    run_frames(a, 2u);
    for (uint32_t i = 0; i < 1000u; i++) {
        cogmaster__z80_step(a);
    }
    CHECK(a->ticks < a->bus.deadline);
    size_t size = save(a, good);
    cogmaster *b = make_with(img_loop, COGMASTER_MACHINE_SMS2, region, tv);
    run_frames(b, 1u);
    if (size != 0u) {
        CHECK_EQ_U64(cogmaster__state_size(b), size);
        CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(save(b, after), size);
        CHECK(memcmp(good, after, size) == 0);
        run_frames(a, 5u);
        run_frames(b, 5u);
        CHECK_EQ_U64(save(a, good), size);
        CHECK_EQ_U64(save(b, after), size);
        CHECK(memcmp(good, after, size) == 0);
        /* The instances agree beyond the bytes they save. */
        CHECK(memcmp(a->ram, b->ram, sizeof a->ram) == 0);
        CHECK(memcmp(&a->z80, &b->z80, sizeof a->z80) == 0);
        CHECK_EQ_U64(a->ticks, b->ticks);
        CHECK(a->ram[0x100] != 0u);
    }
    cogmaster_destroy(a);
    cogmaster_destroy(b);
}

static void sweep(cogmaster *src, cogmaster *target)
{
    size_t size = save(src, good);
    size_t target_size = save(target, before);
    size_t n = record(src);
    CHECK(n >= 14u);
    size_t swept = 0;
    size_t ram = 0;
    for (size_t i = 0; i < n && size != 0u; i++) {
        const struct cogmaster__state_field *fd = &fields[i];
        CHECK(fd->offset + fd->width <= size);
        if (strcmp(fd->name, "ram") == 0) {
            ram++;
        }
        uint64_t type_max = fd->width == 8u ? UINT64_MAX : (UINT64_C(1) << (8u * fd->width)) - 1u;
        const uint64_t outside[2] = {fd->max + 1u, fd->min - 1u};
        const int sweep_side[2] = {fd->max < type_max, fd->min > 0u};
        for (size_t j = 0; j < 2u; j++) {
            if (!sweep_side[j]) {
                continue;
            }
            memcpy(bad, good, size);
            put_le(bad + fd->offset, outside[j], fd->width);
            cogmaster__state_seal(bad, size);
            expect_refused(fd->name, target, size, target_size);
            swept++;
        }
    }
    printf("state fields: %lu recorded, %lu out-of-range values refused\n", (unsigned long)n,
           (unsigned long)swept);
    CHECK(swept >= 30u);
    CHECK_EQ_U64(ram, 8192);
    static const char *const named[] = {
        "z80_pc",     "z80_sp",       "z80_wz",          "z80_im",       "z80_idx",
        "z80_halted", "z80_int_line", "z80_nmi_pending", "bus_deadline", "bus_at",
        "bus_line",   "bus_t",        "bus_vdp_at",      "vdp_reg",      "vdp_status",
        "vdp_latch",  "vdp_code",     "vdp_addr",        "vdp_buffer",   "vdp_pause",
        "vdp_cram",   "vdp_vram",     "machine",         "region",       "tv"};
    for (size_t i = 0; i < sizeof named / sizeof named[0]; i++) {
        (void)field(n, named[i]);
    }
    /* The profile is resolved: no field of it is AUTO or outside its set. */
    static const char *const profile[] = {"machine", "region", "tv"};
    for (size_t i = 0; i < 3u; i++) {
        const struct cogmaster__state_field *fd = field(n, profile[i]);
        if (fd != NULL) {
            CHECK(fd->min >= 1u && fd->max <= 2u);
        }
    }
    /* The control: the unchanged state, resealed, loads. */
    if (size != 0u) {
        memcpy(bad, good, size);
        cogmaster__state_seal(bad, size);
        CHECK(memcmp(bad, good, size) == 0);
        CHECK_EQ_U64(cogmaster__state_load(target, bad, size), COGMASTER_STATUS_OK);
    }
}

/* A state of one profile is refused by an instance of another, and loads
   into its own. */
static void reject_other_profile(void)
{
    static const uint32_t pairs[][2] = {
        {COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL},
        {COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC},
    };
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *src = make_with(NULL, COGMASTER_MACHINE_SMS2, pairs[k][0], pairs[k][1]);
        run_frames(src, 3u);
        cogmaster *target = make(COGMASTER_MACHINE_SMS2);
        run_frames(target, 3u);
        size_t size = save(src, bad);
        size_t target_size = save(target, before);
        CHECK_EQ_U64(size, target_size);
        if (size != 0u && size == target_size) {
            expect_refused("state of another profile", target, size, target_size);
            cogmaster *same = make_with(NULL, COGMASTER_MACHINE_SMS2, pairs[k][0], pairs[k][1]);
            CHECK_EQ_U64(cogmaster__state_load(same, bad, size), COGMASTER_STATUS_OK);
            cogmaster_destroy(same);
        }
        cogmaster_destroy(src);
        cogmaster_destroy(target);
    }
}

static uint8_t img_board[512u * 1024u];

/* A new Master System instance running the first n bytes of img_board. */
static cogmaster *make_board(size_t n)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = img_board;
    media.image_size = n;
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

/* Pages, maps cartridge RAM and fills some of it, runs, then saves, loads
   into a fresh instance and saves again: equal states, equal registers and
   RAM. Every field one step out of range is refused, the registers and each
   cartridge RAM byte are recorded, and a bank at or past the page count is
   refused. */
static void mapper_round_trip(size_t n, uint32_t board, uint32_t ram_size, const uint16_t *writes,
                              size_t n_writes)
{
    cogmaster *a = make_board(n);
    CHECK_EQ_U64(a->cart.board, board);
    for (size_t i = 0; i < n_writes; i += 2u) {
        cogmaster__bus_write(a, writes[i], (uint8_t)writes[i + 1u]);
    }
    for (uint32_t i = 0; i < 0x2000u; i++) {
        cogmaster__bus_write(a, (uint16_t)(0xA000u + i), (uint8_t)(i * 7u + 1u));
    }
    run_frames(a, 2u);
    cogmaster *b = make_board(n);
    run_frames(b, 1u);
    size_t size = save(a, good);
    if (size != 0u) {
        CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(save(b, after), size);
        CHECK(memcmp(good, after, size) == 0);
        CHECK(memcmp(&a->mapper, &b->mapper, sizeof a->mapper) == 0);
        CHECK(a->cart.ram_size == b->cart.ram_size &&
              memcmp(a->cart.ram, b->cart.ram, a->cart.ram_size) == 0);
        uint32_t set = 0;
        for (uint32_t i = 0; i < b->cart.ram_size; i++) {
            set += b->cart.ram[i] != 0u;
        }
        CHECK(set >= 0x1000u);
    }
    size_t fields_n = record(a);
    size_t cart_ram = 0, banks = 0;
    for (size_t i = 0; i < fields_n; i++) {
        cart_ram += strcmp(fields[i].name, "cart_ram") == 0;
        if (strcmp(fields[i].name, "mapper_bank") == 0) {
            banks++;
            CHECK_EQ_U64(fields[i].max, a->cart.pages - 1u);
        }
    }
    CHECK_EQ_U64(cart_ram, ram_size);
    CHECK_EQ_U64(banks, 3);
    (void)field(fields_n, "mapper_board");
    (void)field(fields_n, "mapper_ctrl");
    cogmaster *target = make_board(n);
    run_frames(target, 1u);
    sweep(a, target);
    cogmaster_destroy(a);
    cogmaster_destroy(b);
    cogmaster_destroy(target);
}

/* A 512 KiB Sega image with cartridge RAM bank 1 mapped and banks paged,
   and a 128 KiB Codemasters image with its RAM mapped. */
static void test_state_mapper(void)
{
    memset(img_board, 0, sizeof img_board);
    static const uint16_t sega[] = {0xFFFCu, 0x0Cu, 0xFFFDu, 0x05u, 0xFFFEu, 0x1Fu, 0xFFFFu, 0x02u};
    mapper_round_trip(512u * 1024u, COGMASTER__BOARD_SEGA, 32768u, sega, 8u);
    /* The Codemasters header at $7FE0: 8 pages, 15 June 1994 12:30, checksum
       $1234 and its complement, $7FEF zero. */
    static const uint8_t header[16] = {0x08u, 0x15u, 0x06u, 0x94u, 0x12u, 0x30u, 0x34u, 0x12u,
                                       0xCCu, 0xEDu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
    memcpy(img_board + 0x7FE0u, header, sizeof header);
    static const uint16_t cm[] = {0x4000u, 0x83u, 0x8000u, 0x06u};
    mapper_round_trip(128u * 1024u, COGMASTER__BOARD_CODEMASTERS, 8192u, cm, 4u);
}

/* save, load into a fresh instance of the same profile, save again: the two
   states are equal byte for byte, on NTSC and PAL and in both regions. */
static void test_round_trip(void)
{
    round_trip_image(COGMASTER_REGION_AUTO, COGMASTER_TV_AUTO);
    round_trip_image(COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL);
    round_trip_image(COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC);
    reject_other_profile();
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    const uint32_t frames[] = {0u, 1u, 60u};
    for (size_t k = 0; k < 2u; k++) {
        for (size_t f = 0; f < 3u; f++) {
            cogmaster *a = make(machines[k]);
            run_frames(a, frames[f]);
            size_t size = save(a, good);
            cogmaster *b = make(machines[k]);
            run_frames(b, 3u);
            if (size != 0u) {
                CHECK_EQ_U64(cogmaster__state_size(b), size);
                CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
                CHECK_EQ_U64(save(b, after), size);
                CHECK(memcmp(good, after, size) == 0);
                CHECK_EQ_U64(cogmaster__state_save(a, bad, size - 1u),
                             COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL);
            }
            cogmaster_destroy(a);
            cogmaster_destroy(b);
        }
    }
}

/* Every field set one step outside its bounds is refused, and the refusal
   changes nothing, for both test cards and for an image. A field the
   visitor adds later is swept with no new test code. */
static void test_record_sweep(void)
{
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *src = make(machines[k]);
        run_frames(src, 60u);
        cogmaster *target = make(machines[k]);
        run_frames(target, 5u);
        sweep(src, target);
        cogmaster_destroy(src);
        cogmaster_destroy(target);
    }
    fill_images();
    const uint32_t tvs[] = {COGMASTER_TV_NTSC, COGMASTER_TV_PAL};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *src = make_with(img_loop, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_AUTO, tvs[k]);
        run_frames(src, 3u);
        cogmaster *target =
            make_with(img_loop, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_AUTO, tvs[k]);
        run_frames(target, 1u);
        sweep(src, target);
        cogmaster_destroy(src);
        cogmaster_destroy(target);
    }
    /* A Japanese PAL test card cannot exist; a PAL export one can. */
    cogmaster *src =
        make_with(NULL, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL);
    run_frames(src, 60u);
    cogmaster *target =
        make_with(NULL, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL);
    run_frames(target, 5u);
    sweep(src, target);
    cogmaster_destroy(src);
    cogmaster_destroy(target);
}

/* Sets the field named name in a copy of good and requires target to refuse
   it once resealed. */
static void refuse_with(const char *name, size_t n, uint64_t v, cogmaster *target, size_t size,
                        size_t target_size)
{
    const struct cogmaster__state_field *fd = field(n, name);
    if (fd == NULL) {
        return;
    }
    memcpy(bad, good, size);
    put_le(bad + fd->offset, v, fd->width);
    cogmaster__state_seal(bad, size);
    expect_refused(name, target, size, target_size);
}

/* An image's state is refused by an instance running another image, and
   when its frame deadline, ticks and beam disagree, each value inside its
   own bounds. */
static void reject_image_states(void)
{
    fill_images();
    cogmaster *src = make_image(img_loop);
    run_frames(src, 4u);
    cogmaster__z80_step(src);
    cogmaster *target = make_image(img_loop);
    run_frames(target, 2u);
    size_t size = save(src, good);
    size_t target_size = save(target, before);
    size_t n = record(src);
    const struct cogmaster__state_field *ticks = field(n, "ticks");
    if (size == 0u || ticks == NULL) {
        cogmaster_destroy(src);
        cogmaster_destroy(target);
        return;
    }
    uint64_t t = get_le(good + ticks->offset, 8u);
    refuse_with("bus_deadline", n, 6u * TICKS_PER_FRAME, target, size, target_size);
    refuse_with("ticks", n, 4u * TICKS_PER_FRAME - 1u, target, size, target_size);
    refuse_with("ticks", n, 5u * TICKS_PER_FRAME, target, size, target_size);
    refuse_with("bus_at", n, t + 1u, target, size, target_size);
    refuse_with("bus_at", n, t - 70u, target, size, target_size);
    refuse_with("bus_vdp_at", n, t + 1u, target, size, target_size);
    /* /INT must be the VDP's frame flag AND IE0; this image leaves both
       clear, so a raised /INT is refused. */
    refuse_with("z80_int_line", n, 1u, target, size, target_size);
    cogmaster *other = make_image(img_halt);
    if (save(other, bad) == size) {
        memcpy(bad, good, size);
        expect_refused("state of another image", other, size, save(other, before));
    }
    /* The unchanged state loads. */
    CHECK_EQ_U64(cogmaster__state_load(target, good, size), COGMASTER_STATUS_OK);
    cogmaster_destroy(other);
    cogmaster_destroy(src);
    cogmaster_destroy(target);
}

/* Header corruption, a wrong length and a bad checksum are each refused and
   change nothing. */
static void test_rejects_header(void)
{
    cogmaster *src = make(COGMASTER_MACHINE_SMS2);
    run_frames(src, 60u);
    cogmaster *target = make(COGMASTER_MACHINE_SMS2);
    run_frames(target, 5u);
    size_t size = save(src, good);
    size_t target_size = save(target, before);
    size_t n = record(src);
    const struct cogmaster__state_field *magic = field(n, "magic");
    const struct cogmaster__state_field *total = field(n, "total_size");
    const struct cogmaster__state_field *header = field(n, "header_size");
    const struct cogmaster__state_field *crc = field(n, "body_crc32");
    const struct cogmaster__state_field *reserved = field(n, "reserved");
    const struct cogmaster__state_field *ticks = field(n, "ticks");
    if (size == 0u || magic == NULL || total == NULL || header == NULL || crc == NULL ||
        reserved == NULL || ticks == NULL) {
        return;
    }
    CHECK_EQ_U64(magic->offset, 0);
    CHECK_EQ_U64(reserved->offset, 52);

    for (uint32_t i = 0; i < 8u; i++) {
        memcpy(bad, good, size);
        bad[magic->offset + i] ^= 0x80u;
        expect_refused("magic", target, size, target_size);
    }
    const uint64_t off_by_one[2] = {size + 1u, size - 1u};
    for (size_t j = 0; j < 2u; j++) {
        memcpy(bad, good, size);
        put_le(bad + total->offset, off_by_one[j], total->width);
        expect_refused("total size field", target, size, target_size);
        memcpy(bad, good, size);
        expect_refused("buffer length", target, (size_t)off_by_one[j], target_size);
    }
    const uint64_t header_sizes[2] = {63u, 65u};
    for (size_t j = 0; j < 2u; j++) {
        memcpy(bad, good, size);
        put_le(bad + header->offset, header_sizes[j], header->width);
        expect_refused("header size", target, size, target_size);
    }
    memcpy(bad, good, size);
    bad[crc->offset] ^= 1u;
    expect_refused("body checksum field", target, size, target_size);
    memcpy(bad, good, size);
    bad[size - 1u] ^= 1u;
    expect_refused("body byte", target, size, target_size);
    for (uint32_t i = 0; i < 12u; i++) {
        memcpy(bad, good, size);
        bad[reserved->offset + i] = 1u;
        expect_refused("reserved byte", target, size, target_size);
    }
    /* Ticks one away from frame x 179,208, both inside their own bounds. */
    memcpy(bad, good, size);
    put_le(bad + ticks->offset, get_le(good + ticks->offset, 8u) + 1u, 8u);
    cogmaster__state_seal(bad, size);
    expect_refused("ticks off the frame count", target, size, target_size);
    /* A state of the other machine. */
    cogmaster *gg = make(COGMASTER_MACHINE_GG);
    run_frames(gg, 60u);
    if (save(gg, bad) == size) {
        expect_refused("other machine", target, size, target_size);
    }
    cogmaster_destroy(gg);

    reject_image_states();

    CHECK_EQ_U64(cogmaster__state_load(target, NULL, size), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster__state_load(NULL, good, size), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster__state_save(NULL, bad, size), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster__state_save(target, NULL, size), COGMASTER_STATUS_ERR_ARGUMENT);

    /* Before a load there is no state to save or to load into. */
    cogmaster_config cfg = make_config();
    cogmaster *empty = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &empty), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_size(empty), 0);
    CHECK_EQ_U64(cogmaster__state_record(empty, fields, MAX_FIELDS), 0);
    CHECK_EQ_U64(cogmaster__state_save(empty, bad, BUF), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK_EQ_U64(cogmaster__state_load(empty, good, size), COGMASTER_STATUS_ERR_NO_CONTENT);
    cogmaster_destroy(empty);
    cogmaster_destroy(src);
    cogmaster_destroy(target);
}

/* After 60 frames the header's frame field reads 60 and its ticks field
   60 x 179,208. */
static void test_frame_counter(void)
{
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = make(machines[k]);
        run_frames(m, 60u);
        size_t size = save(m, good);
        size_t n = record(m);
        const struct cogmaster__state_field *frame = field(n, "frame");
        const struct cogmaster__state_field *ticks = field(n, "ticks");
        if (size != 0u && frame != NULL && ticks != NULL) {
            CHECK_EQ_U64(frame->width, 8);
            CHECK_EQ_U64(ticks->width, 8);
            CHECK_EQ_U64(get_le(good + frame->offset, 8u), 60);
            CHECK_EQ_U64(get_le(good + ticks->offset, 8u), 60u * TICKS_PER_FRAME);
            CHECK_EQ_U64(get_le(good + ticks->offset, 8u), 10752480u);
        }
        cogmaster_destroy(m);
    }
}

static cogmaster_fault fault_of(const cogmaster *m)
{
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(f.size, sizeof f);
    return f;
}

/* A fault with every field set, distinct per seed. */
static struct cogmaster__fault sample_fault(uint32_t seed)
{
    struct cogmaster__fault f;
    memset(&f, 0, sizeof f);
    f.frame = 7u + seed;
    f.line = 100u + seed;
    f.dot = 200u + seed;
    f.pc = (uint16_t)(0x1234u + seed);
    f.addr = (uint16_t)(0xBEEFu + seed);
    f.opcode = (uint8_t)(0xEDu + seed);
    f.width = (uint8_t)(1u + seed);
    f.device = (uint8_t)(3u + seed);
    f.kind = (uint8_t)(5u + seed);
    return f;
}

static void expect_fault(const cogmaster *m, const struct cogmaster__fault *want)
{
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 1);
    CHECK_EQ_U64(f.frame, want->frame);
    CHECK_EQ_U64(f.line, want->line);
    CHECK_EQ_U64(f.dot, want->dot);
    CHECK_EQ_HEX(f.pc, want->pc);
    CHECK_EQ_HEX(f.addr, want->addr);
    CHECK_EQ_HEX(f.opcode, want->opcode);
    CHECK_EQ_U64(f.width, want->width);
    CHECK_EQ_U64(f.device, want->device);
    CHECK_EQ_U64(f.kind, want->kind);
}

/* The size-tag rule for the fault struct on m: below its size the struct is
   left alone; at any larger size only its own bytes are written. */
static void check_fault_tags(const cogmaster *m)
{
    const uint32_t small[] = {0u, (uint32_t)sizeof(cogmaster_fault) - 1u};
    for (size_t i = 0; i < 2u; i++) {
        memset(tag_buf, POISON, TAG_BUF);
        memcpy(tag_buf, &small[i], sizeof small[i]);
        memcpy(tag_before, tag_buf, TAG_BUF);
        CHECK_EQ_U64(cogmaster_get_fault(m, (cogmaster_fault *)(void *)tag_buf),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE);
        CHECK(memcmp(tag_before, tag_buf, TAG_BUF) == 0);
    }
    const uint32_t big[] = {(uint32_t)sizeof(cogmaster_fault) + 8u, TAG_BUF};
    for (size_t i = 0; i < 2u; i++) {
        memset(tag_buf, POISON, TAG_BUF);
        memcpy(tag_buf, &big[i], sizeof big[i]);
        CHECK_EQ_U64(cogmaster_get_fault(m, (cogmaster_fault *)(void *)tag_buf),
                     COGMASTER_STATUS_OK);
        cogmaster_fault f;
        memcpy(&f, tag_buf, sizeof f);
        CHECK_EQ_U64(f.size, big[i]);
        CHECK_EQ_U64(f.present, 0);
        uint32_t touched = 0;
        for (size_t k = sizeof f; k < TAG_BUF; k++) {
            if (tag_buf[k] != POISON) {
                touched++;
            }
        }
        CHECK_EQ_U64(touched, 0);
    }
}

/* No fault after create or after load, and the size-tag rule holds. */
static void test_fault_empty(void)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(fault_of(m).present, 0);
    cogmaster_destroy(m);

    m = make(COGMASTER_MACHINE_GG);
    run_frames(m, 3u);
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 0);
    CHECK_EQ_U64(f.frame, 0);
    CHECK_EQ_U64(f.pc, 0);
    check_fault_tags(m);
    cogmaster_fault out;
    memset(&out, 0, sizeof out);
    out.size = (uint32_t)sizeof out;
    CHECK_EQ_U64(cogmaster_get_fault(NULL, &out), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_fault(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_destroy(m);
}

/* Only the first fault since load or reset is kept. */
static void test_fault_first_wins(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    struct cogmaster__fault first = sample_fault(0u);
    struct cogmaster__fault second = sample_fault(1u);
    cogmaster__fault_record(m, &first);
    cogmaster__fault_record(m, &second);
    expect_fault(m, &first);
    cogmaster_destroy(m);
}

/* Both reset kinds clear the fault and return frame and ticks to zero; the
   card keeps running afterwards. */
static void test_fault_reset_clears(void)
{
    const uint32_t kinds[] = {COGMASTER_RESET_SOFT, COGMASTER_RESET_POWER};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS2);
        run_frames(m, 10u);
        struct cogmaster__fault f = sample_fault(0u);
        cogmaster__fault_record(m, &f);
        CHECK_EQ_U64(cogmaster_reset(m, kinds[k]), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(fault_of(m).present, 0);
        size_t size = save(m, good);
        size_t n = record(m);
        const struct cogmaster__state_field *frame = field(n, "frame");
        const struct cogmaster__state_field *ticks = field(n, "ticks");
        if (size != 0u && frame != NULL && ticks != NULL) {
            CHECK_EQ_U64(get_le(good + frame->offset, 8u), 0);
            CHECK_EQ_U64(get_le(good + ticks->offset, 8u), 0);
        }
        /* A fault after the reset is recorded again. */
        struct cogmaster__fault g = sample_fault(2u);
        cogmaster__fault_record(m, &g);
        expect_fault(m, &g);
        run_frames(m, 1u);
        if (save(m, good) != 0u && frame != NULL) {
            CHECK_EQ_U64(get_le(good + frame->offset, 8u), 1);
        }
        cogmaster_destroy(m);
    }
}

/* A recorded fault survives save and load into another instance, and a
   state without one clears the target's. */
static void test_fault_state_round_trip(void)
{
    cogmaster *a = make(COGMASTER_MACHINE_GG);
    run_frames(a, 4u);
    struct cogmaster__fault f = sample_fault(0u);
    cogmaster__fault_record(a, &f);
    cogmaster *b = make(COGMASTER_MACHINE_GG);
    size_t size = save(a, good);
    if (size != 0u) {
        CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
        expect_fault(b, &f);
        CHECK_EQ_U64(save(b, after), size);
        CHECK(memcmp(good, after, size) == 0);
    }
    cogmaster *clean = make(COGMASTER_MACHINE_GG);
    size = save(clean, good);
    if (size != 0u) {
        CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(fault_of(b).present, 0);
        /* A fault field set while no fault is present is refused, though the
           value is inside the field's own bounds. */
        size_t n = record(clean);
        const struct cogmaster__state_field *line = field(n, "fault_line");
        if (line != NULL) {
            memcpy(bad, good, size);
            put_le(bad + line->offset, 5u, line->width);
            cogmaster__state_seal(bad, size);
            CHECK_EQ_U64(cogmaster__state_load(b, bad, size), COGMASTER_STATUS_ERR_ARGUMENT);
        }
    }
    cogmaster_destroy(a);
    cogmaster_destroy(b);
    cogmaster_destroy(clean);
}

/* The loaded instance's saved bytes and field equal the snapshot taken by
   take_snapshot. */
static size_t snapshot_size;

static void take_snapshot(const cogmaster *m)
{
    snapshot_size = save(m, before);
    memcpy(pixels, m->field, sizeof pixels);
}

static void expect_unchanged(const char *what, cogmaster_status got, cogmaster_status want,
                             const cogmaster *m)
{
    if (got != want) {
        fprintf(stderr, "%s: status %lu, expected %lu\n", what, (unsigned long)got,
                (unsigned long)want);
        check_failures++;
    }
    if (save(m, after) != snapshot_size || memcmp(before, after, snapshot_size) != 0 ||
        memcmp(pixels, m->field, sizeof pixels) != 0) {
        fprintf(stderr, "%s: the instance changed\n", what);
        check_failures++;
    }
}

/* Before a load the whole instance is compared. */
static void expect_untouched(const char *what, cogmaster_status got, cogmaster_status want,
                             const cogmaster *m)
{
    if (got != want) {
        fprintf(stderr, "%s: status %lu, expected %lu\n", what, (unsigned long)got,
                (unsigned long)want);
        check_failures++;
    }
    if (memcmp(snapshot, m, sizeof snapshot) != 0) {
        fprintf(stderr, "%s: the instance changed\n", what);
        check_failures++;
    }
}

static void *null_alloc(void *user, size_t size)
{
    (void)user;
    (void)size;
    return NULL;
}

/* Every error a public call returns leaves the instance as it was. */
static void test_error_changes_nothing(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    run_frames(m, 7u);
    struct cogmaster__fault f = sample_fault(0u);
    cogmaster__fault_record(m, &f);
    take_snapshot(m);

    /* create: every refusal leaves *out and the other instance alone. */
    cogmaster_config cfg = make_config();
    cogmaster *out = m;
    expect_unchanged("create without config", cogmaster_create(NULL, &out),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("create without out", cogmaster_create(&cfg, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    cfg.size = 0u;
    expect_unchanged("create with size 0", cogmaster_create(&cfg, &out),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cfg = make_config();
    cfg.allocator.alloc = NULL;
    expect_unchanged("create without alloc", cogmaster_create(&cfg, &out),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    cfg.allocator.alloc = null_alloc;
    expect_unchanged("create out of memory", cogmaster_create(&cfg, &out),
                     COGMASTER_STATUS_ERR_NO_MEMORY, m);
    CHECK(out == m);

    /* load */
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = COGMASTER_MACHINE_GG;
    expect_unchanged("load without instance", cogmaster_load(NULL, &media),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("load without media", cogmaster_load(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT,
                     m);
    media.size = 0u;
    expect_unchanged("load with size 0", cogmaster_load(m, &media),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    media.size = (uint32_t)sizeof media;
    static const uint8_t image[1] = {0};
    media.image = image;
    media.image_size = 1u;
    expect_unchanged("load a 1-byte image", cogmaster_load(m, &media),
                     COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE, m);
    media.image = NULL;
    expect_unchanged("load a size without an image", cogmaster_load(m, &media),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    media.image = NULL;
    media.image_size = 0u;
    media.machine = 3u;
    expect_unchanged("load another machine", cogmaster_load(m, &media),
                     COGMASTER_STATUS_ERR_PROFILE_REJECTED, m);

    /* reset */
    expect_unchanged("reset without instance", cogmaster_reset(NULL, COGMASTER_RESET_SOFT),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("reset kind 2", cogmaster_reset(m, 2u), COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("reset kind max", cogmaster_reset(m, 0xFFFFFFFFu),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);

    /* run_frame */
    cogmaster_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    expect_unchanged("run_frame without instance", cogmaster_run_frame(NULL, &result),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    result.size = 0u;
    expect_unchanged("run_frame with size 0", cogmaster_run_frame(m, &result),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);

    /* get_frame */
    cogmaster_frame io;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pitch_bytes = 512u;
    static uint16_t sink[256u * 240u];
    io.pixels = sink;
    expect_unchanged("get_frame without instance", cogmaster_get_frame(NULL, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_frame without request", cogmaster_get_frame(m, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.size = 0u;
    expect_unchanged("get_frame with size 0", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    io.size = (uint32_t)sizeof io;
    io.flags = 2u;
    expect_unchanged("get_frame unknown flag", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.flags = 0u;
    io.pixels = NULL;
    expect_unchanged("get_frame without pixels", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.pixels = sink;
    io.pitch_bytes = 65537u;
    expect_unchanged("get_frame pitch too wide", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.pitch_bytes = 511u;
    expect_unchanged("get_frame pitch short", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL, m);

    /* get_info, get_fault, get_versions */
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    expect_unchanged("get_info without instance", cogmaster_get_info(NULL, &info),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_info without out", cogmaster_get_info(m, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_info with size 0", cogmaster_get_info(m, &info),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cogmaster_fault fault;
    memset(&fault, 0, sizeof fault);
    expect_unchanged("get_fault without instance", cogmaster_get_fault(NULL, &fault),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_fault without out", cogmaster_get_fault(m, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_fault with size 0", cogmaster_get_fault(m, &fault),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cogmaster_versions versions;
    memset(&versions, 0, sizeof versions);
    expect_unchanged("get_versions without out", cogmaster_get_versions(NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_versions with size 0", cogmaster_get_versions(&versions),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cogmaster_destroy(m);

    /* Before a load: the calls that need content refuse it and change no
       byte of the instance. */
    cfg = make_config();
    cogmaster *e = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &e), COGMASTER_STATUS_OK);
    memcpy(snapshot, e, sizeof snapshot);
    result.size = (uint32_t)sizeof result;
    expect_untouched("run_frame before load", cogmaster_run_frame(e, &result),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    io.pitch_bytes = 512u;
    expect_untouched("get_frame before load", cogmaster_get_frame(e, &io),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    info.size = (uint32_t)sizeof info;
    expect_untouched("get_info before load", cogmaster_get_info(e, &info),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    expect_untouched("reset before load", cogmaster_reset(e, COGMASTER_RESET_POWER),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    media.machine = 3u;
    expect_untouched("load another machine before load", cogmaster_load(e, &media),
                     COGMASTER_STATUS_ERR_PROFILE_REJECTED, e);
    cogmaster_destroy(e);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"api.error_changes_nothing", test_error_changes_nothing},
    {"fault.empty_after_create", test_fault_empty},
    {"fault.first_wins", test_fault_first_wins},
    {"fault.reset_clears", test_fault_reset_clears},
    {"fault.state_round_trip", test_fault_state_round_trip},
    {"state.frame_counter", test_frame_counter},
    {"state.mapper", test_state_mapper},
    {"state.record_sweep", test_record_sweep},
    {"state.rejects_header", test_rejects_header},
    {"state.round_trip", test_round_trip},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-state <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-state: unknown case %s\n", argv[1]);
    return 2;
}
