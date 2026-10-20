/* The cartridge boards at register level: synthetic images built here are
   loaded through the public API, then read and written through the bus in
   src/internal.h: cogmaster-test-mapper <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define PAGE 16384u
#define IMAGE_MAX (1024u * 1024u)

static uint8_t image[IMAGE_MAX];

/* The ROM header magic as the BIOS compares it, written as bytes. */
static const uint8_t header_magic[8] = {0x54u, 0x4Du, 0x52u, 0x20u, 0x53u, 0x45u, 0x47u, 0x41u};

/* Where each bank carries its stamp: three offsets, so slot 0's fixed first
   1 KiB, the rest of slot 0 and the middle of a slot are each seen. */
static const uint16_t stamp_at[3] = {0x0000u, 0x0400u, 0x2000u};

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

/* The stamp bank b carries: never $32, so the opcode scan sees no store. */
static uint8_t stamp(uint32_t b)
{
    return (uint8_t)(0x40u | b);
}

/* n bytes of $FF with each 16 KiB bank stamped at stamp_at, and a ROM
   header at $7FF0 with region code 4 and size code 1 (512 KiB). */
static void sega_image(size_t n)
{
    memset(image, 0xFF, n);
    for (uint32_t b = 0; (size_t)b * PAGE < n; b++) {
        for (uint32_t i = 0; i < 3u; i++) {
            size_t at = (size_t)b * PAGE + stamp_at[i];
            if (at < n) {
                image[at] = stamp(b);
            }
        }
    }
    memcpy(image + 0x7FF0u, header_magic, sizeof header_magic);
    image[0x7FFFu] = 0x41u;
}

/* A new Master System instance with the first n bytes of image loaded. */
static cogmaster *load(size_t n)
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
    media.image_size = n;
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    if (m->cart.rom == NULL) {
        exit(1);
    }
    return m;
}

/* The bank slot s shows, read from its middle stamp. */
static uint32_t slot_bank(cogmaster *m, uint32_t s)
{
    return cogmaster__bus_read(m, (uint16_t)(s * PAGE + 0x2000u)) & 0x3Fu;
}

/* A 512 KiB image is a Sega board paged 0, 1, 2. Each of banks 0 to 31
   reads its own stamp through slot 1 ($FFFE) and slot 2 ($FFFF). After
   $FFFD = 5, slot 0 below $0400 still reads bank 0 and from $0400 bank 5. */
static void test_sega_banks(void)
{
    sega_image(512u * 1024u);
    cogmaster *m = load(512u * 1024u);
    CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_SEGA);
    CHECK_EQ_U64(slot_bank(m, 0u), 0);
    CHECK_EQ_U64(slot_bank(m, 1u), 1);
    CHECK_EQ_U64(slot_bank(m, 2u), 2);
    uint32_t wrong = 0;
    for (uint32_t b = 0; b < 32u; b++) {
        cogmaster__bus_write(m, 0xFFFEu, (uint8_t)b);
        cogmaster__bus_write(m, 0xFFFFu, (uint8_t)b);
        wrong += slot_bank(m, 1u) != b;
        wrong += slot_bank(m, 2u) != b;
        wrong += cogmaster__bus_read(m, 0x4400u) != stamp(b);
        wrong += cogmaster__bus_read(m, 0x8000u) != stamp(b);
    }
    CHECK_EQ_U64(wrong, 0);
    cogmaster__bus_write(m, 0xFFFDu, 5u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0000u), stamp(0u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x03FFu), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0400u), stamp(5u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x2000u), stamp(5u));
    cogmaster_destroy(m);
}

/* A bank number past the end wraps modulo the page count: on 512 KiB, 35
   selects bank 3 and $FF bank 31; on 384 KiB (24 pages), 30 selects 6, 24
   selects 0 and $FF selects 15. */
static void test_sega_wrap(void)
{
    sega_image(512u * 1024u);
    cogmaster *m = load(512u * 1024u);
    cogmaster__bus_write(m, 0xFFFEu, 35u);
    CHECK_EQ_U64(slot_bank(m, 1u), 3);
    cogmaster__bus_write(m, 0xFFFEu, 0xFFu);
    CHECK_EQ_U64(slot_bank(m, 1u), 31);
    cogmaster_destroy(m);

    sega_image(384u * 1024u);
    m = load(384u * 1024u);
    CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_SEGA);
    CHECK_EQ_U64(m->cart.pages, 24);
    cogmaster__bus_write(m, 0xFFFFu, 30u);
    CHECK_EQ_U64(slot_bank(m, 2u), 6);
    cogmaster__bus_write(m, 0xFFFFu, 24u);
    CHECK_EQ_U64(slot_bank(m, 2u), 0);
    cogmaster__bus_write(m, 0xFFFDu, 0xFFu);
    CHECK_EQ_U64(slot_bank(m, 0u), 15);
    cogmaster_destroy(m);
}

static uint8_t pattern(uint32_t i, uint32_t seed)
{
    return (uint8_t)(i * 13u + seed);
}

/* Writes pattern seed over the 16 KiB of slot 2 and returns how many
   bytes read back differently. */
static uint32_t fill_slot2(cogmaster *m, uint32_t seed)
{
    for (uint32_t i = 0; i < PAGE; i++) {
        cogmaster__bus_write(m, (uint16_t)(0x8000u + i), pattern(i, seed));
    }
    uint32_t wrong = 0;
    for (uint32_t i = 0; i < PAGE; i++) {
        wrong += cogmaster__bus_read(m, (uint16_t)(0x8000u + i)) != pattern(i, seed);
    }
    return wrong;
}

static uint32_t slot2_differs(cogmaster *m, uint32_t seed)
{
    uint32_t wrong = 0;
    for (uint32_t i = 0; i < PAGE; i++) {
        wrong += cogmaster__bus_read(m, (uint16_t)(0x8000u + i)) != pattern(i, seed);
    }
    return wrong;
}

/* $FFFC bit 3 maps cartridge RAM over slot 2: a pattern written reads back,
   clearing the bit shows the ROM bank again, setting it shows the pattern
   again. Bit 2 selects a second 16 KiB bank that keeps its own bytes. */
static void test_sega_ram(void)
{
    sega_image(512u * 1024u);
    cogmaster *m = load(512u * 1024u);
    CHECK_EQ_U64(m->cart.ram_size, 32768);
    cogmaster__bus_write(m, 0xFFFFu, 9u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), stamp(9u));
    cogmaster__bus_write(m, 0xFFFCu, 0x08u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), 0x00);
    CHECK_EQ_U64(fill_slot2(m, 1u), 0);
    cogmaster__bus_write(m, 0xFFFCu, 0x00u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), stamp(9u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x8000u), stamp(9u));
    cogmaster__bus_write(m, 0xFFFCu, 0x08u);
    CHECK_EQ_U64(slot2_differs(m, 1u), 0);
    /* The second bank starts zeroed and keeps its own pattern. */
    cogmaster__bus_write(m, 0xFFFCu, 0x0Cu);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x8000u), 0x00);
    CHECK_EQ_U64(fill_slot2(m, 2u), 0);
    cogmaster__bus_write(m, 0xFFFCu, 0x08u);
    CHECK_EQ_U64(slot2_differs(m, 1u), 0);
    cogmaster__bus_write(m, 0xFFFCu, 0x0Cu);
    CHECK_EQ_U64(slot2_differs(m, 2u), 0);
    /* Slot 1 still reads ROM, and a soft reset keeps the RAM. */
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x6000u), stamp(1u));
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->mapper.ctrl, 0);
    cogmaster__bus_write(m, 0xFFFCu, 0x0Cu);
    CHECK_EQ_U64(slot2_differs(m, 2u), 0);
    cogmaster_destroy(m);
}

/* The registers are write-only; each write also lands in work RAM, so
   $DFFC-$DFFF and $FFFC-$FFFF read the values back. */
static void test_sega_mirror(void)
{
    sega_image(512u * 1024u);
    cogmaster *m = load(512u * 1024u);
    static const uint8_t values[4] = {0x08u, 3u, 4u, 7u};
    for (uint32_t i = 0; i < 4u; i++) {
        cogmaster__bus_write(m, (uint16_t)(0xFFFCu + i), values[i]);
    }
    for (uint32_t i = 0; i < 4u; i++) {
        CHECK_EQ_HEX(cogmaster__bus_read(m, (uint16_t)(0xDFFCu + i)), values[i]);
        CHECK_EQ_HEX(cogmaster__bus_read(m, (uint16_t)(0xFFFCu + i)), values[i]);
    }
    CHECK_EQ_U64(slot_bank(m, 0u), 3);
    CHECK_EQ_U64(slot_bank(m, 1u), 4);
    /* A write to the mirror at $DFFF is only RAM: it pages nothing. */
    cogmaster__bus_write(m, 0xFFFCu, 0x00u);
    cogmaster__bus_write(m, 0xDFFFu, 12u);
    CHECK_EQ_U64(slot_bank(m, 2u), 7);
    cogmaster_destroy(m);
}

/* $FFFC bits 1-0 add $18, $10 or $08 to every later bank write; banks
   written before keep their value. */
static void test_sega_shift(void)
{
    sega_image(512u * 1024u);
    cogmaster *m = load(512u * 1024u);
    static const uint32_t added[4] = {0x00u, 0x18u, 0x10u, 0x08u};
    for (uint32_t code = 0; code < 4u; code++) {
        cogmaster__bus_write(m, 0xFFFCu, (uint8_t)code);
        CHECK_EQ_U64(slot_bank(m, 1u), 1);
        cogmaster__bus_write(m, 0xFFFFu, 2u);
        CHECK_EQ_U64(slot_bank(m, 2u), 2u + added[code]);
        cogmaster__bus_write(m, 0xFFFDu, 3u);
        CHECK_EQ_U64(slot_bank(m, 0u), 3u + added[code]);
        CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0000u), stamp(0u));
    }
    cogmaster_destroy(m);
}

/* A 128 KiB (8-page) Codemasters image: stamped banks and the header at
   $7FE0 as literal bytes: 8 pages, 15 June 1994 12:30, checksum $1234 and
   its complement $EDCC, $7FEF zero. */
static void codemasters_image(void)
{
    sega_image(128u * 1024u);
    memset(image + 0x7FF0u, 0xFF, 16u);
    static const uint8_t header[16] = {0x08u, 0x15u, 0x06u, 0x94u, 0x12u, 0x30u, 0x34u, 0x12u,
                                       0xCCu, 0xEDu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
    memcpy(image + 0x7FE0u, header, sizeof header);
}

/* Runs code copied to $C000 until it halts, at most 64 steps. */
static void run_from_ram(cogmaster *m, const uint8_t *code, size_t n)
{
    memcpy(m->ram, code, n);
    m->z80.pc = 0xC000u;
    for (uint32_t i = 0; i < 64u && !m->z80.halted; i++) {
        cogmaster__z80_step(m);
    }
    CHECK_EQ_U64(m->z80.halted, 1);
}

/* Reset banks are 0, 1, 0; writes to $4000 and $8000 page slots 1 and 2,
   and code running in RAM pages slot 0 through $0000, the first 1 KiB
   included. */
static void test_codemasters_banks(void)
{
    codemasters_image();
    cogmaster *m = load(128u * 1024u);
    CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_CODEMASTERS);
    CHECK_EQ_U64(m->cart.ram_size, 8192);
    CHECK_EQ_U64(slot_bank(m, 0u), 0);
    CHECK_EQ_U64(slot_bank(m, 1u), 1);
    CHECK_EQ_U64(slot_bank(m, 2u), 0);
    cogmaster__bus_write(m, 0x4000u, 5u);
    cogmaster__bus_write(m, 0x8000u, 7u);
    CHECK_EQ_U64(slot_bank(m, 1u), 5);
    CHECK_EQ_U64(slot_bank(m, 2u), 7);
    /* Only the first byte of a slot is a register. */
    cogmaster__bus_write(m, 0x4001u, 2u);
    cogmaster__bus_write(m, 0xFFFFu, 2u);
    CHECK_EQ_U64(slot_bank(m, 1u), 5);
    CHECK_EQ_U64(slot_bank(m, 2u), 7);
    /* DI; LD A,3; LD ($0000),A; HALT. */
    static const uint8_t code[] = {0xF3u, 0x3Eu, 0x03u, 0x32u, 0x00u, 0x00u, 0x76u};
    run_from_ram(m, code, sizeof code);
    CHECK_EQ_U64(slot_bank(m, 0u), 3);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0000u), stamp(3u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0400u), stamp(3u));
    /* A reset pages 0, 1, 0 again. */
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(slot_bank(m, 0u), 0);
    CHECK_EQ_U64(slot_bank(m, 1u), 1);
    CHECK_EQ_U64(slot_bank(m, 2u), 0);
    cogmaster_destroy(m);
}

/* Bit 7 of a $4000 write maps 8 KiB of cartridge RAM at $A000-$BFFF while
   $8000-$9FFF stays ROM; clearing it maps the ROM back. */
static void test_codemasters_ram(void)
{
    codemasters_image();
    cogmaster *m = load(128u * 1024u);
    cogmaster__bus_write(m, 0x8000u, 6u);
    cogmaster__bus_write(m, 0x4000u, 0x81u);
    CHECK_EQ_U64(slot_bank(m, 1u), 1);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x8000u), stamp(6u));
    uint32_t wrong = 0;
    for (uint32_t i = 0; i < 0x2000u; i++) {
        cogmaster__bus_write(m, (uint16_t)(0xA000u + i), pattern(i, 3u));
    }
    for (uint32_t i = 0; i < 0x2000u; i++) {
        wrong += cogmaster__bus_read(m, (uint16_t)(0xA000u + i)) != pattern(i, 3u);
    }
    CHECK_EQ_U64(wrong, 0);
    cogmaster__bus_write(m, 0x4000u, 0x01u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), stamp(6u));
    cogmaster__bus_write(m, 0xA000u, 0x99u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), stamp(6u));
    cogmaster__bus_write(m, 0x4000u, 0x81u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), pattern(0u, 3u));
    cogmaster_destroy(m);
}

/* bank = value mod 8 on the 128 KiB image. */
static void test_codemasters_wrap(void)
{
    codemasters_image();
    cogmaster *m = load(128u * 1024u);
    cogmaster__bus_write(m, 0x4000u, 9u);
    CHECK_EQ_U64(slot_bank(m, 1u), 1);
    cogmaster__bus_write(m, 0x8000u, 15u);
    CHECK_EQ_U64(slot_bank(m, 2u), 7);
    cogmaster__bus_write(m, 0x0000u, 0x7Fu);
    CHECK_EQ_U64(slot_bank(m, 0u), 7);
    cogmaster_destroy(m);
}

/* 32 KiB and 48 KiB images with no header read linearly up to $BFFF ($FF
   past the image); writes to ROM, the Sega registers included, page
   nothing; work RAM mirrors at $E000. */
static void test_none_layout(void)
{
    const size_t sizes[2] = {32u * 1024u, 48u * 1024u};
    for (size_t k = 0; k < 2u; k++) {
        sega_image(sizes[k]);
        memset(image + 0x7FF0u, 0xFF, 16u);
        cogmaster *m = load(sizes[k]);
        CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_ROM_ONLY);
        CHECK_EQ_U64(m->cart.ram_size, 0);
        uint32_t wrong = 0;
        for (uint32_t a = 0; a < 0xC000u; a++) {
            uint8_t want = a < sizes[k] ? image[a] : 0xFFu;
            wrong += cogmaster__bus_read(m, (uint16_t)a) != want;
        }
        CHECK_EQ_U64(wrong, 0);
        cogmaster__bus_write(m, 0xFFFFu, 5u);
        cogmaster__bus_write(m, 0xFFFEu, 0u);
        cogmaster__bus_write(m, 0xFFFCu, 0x08u);
        cogmaster__bus_write(m, 0x0400u, 0x12u);
        cogmaster__bus_write(m, 0x8000u, 0x12u);
        CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0400u), stamp(0u));
        CHECK_EQ_HEX(cogmaster__bus_read(m, 0x6000u), stamp(1u));
        CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), k == 1u ? stamp(2u) : 0xFFu);
        cogmaster__bus_write(m, 0xC123u, 0x5Au);
        CHECK_EQ_HEX(cogmaster__bus_read(m, 0xE123u), 0x5A);
        CHECK_EQ_HEX(cogmaster__bus_read(m, 0xDFFFu), 0x05);
        cogmaster_destroy(m);
    }
}

/* A 40 KiB image pads with $FF to 48 KiB. A 136 KiB Sega image pads to 9
   pages: the tail of bank 8 reads $FF, and banks 9 and 10 wrap to 0 and 1. */
static void test_pad_wrap(void)
{
    sega_image(40u * 1024u);
    memset(image + 0x7FF0u, 0xFF, 16u);
    image[40u * 1024u - 1u] = 0x5Au;
    cogmaster *m = load(40u * 1024u);
    CHECK_EQ_U64(m->cart.size, 48u * 1024u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x9FFFu), 0x5A);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xBFFFu), 0xFF);
    cogmaster_destroy(m);

    sega_image(136u * 1024u);
    image[136u * 1024u - 1u] = 0x5Au;
    m = load(136u * 1024u);
    CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_SEGA);
    CHECK_EQ_U64(m->cart.pages, 9);
    cogmaster__bus_write(m, 0xFFFFu, 8u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x8000u), stamp(8u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x9FFFu), 0x5A);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xA000u), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xBFFFu), 0xFF);
    cogmaster__bus_write(m, 0xFFFFu, 9u);
    CHECK_EQ_U64(slot_bank(m, 2u), 0);
    cogmaster__bus_write(m, 0xFFFFu, 10u);
    CHECK_EQ_U64(slot_bank(m, 2u), 1);
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"codemasters.banks", test_codemasters_banks},
    {"codemasters.ram", test_codemasters_ram},
    {"codemasters.wrap", test_codemasters_wrap},
    {"none.layout", test_none_layout},
    {"pad_wrap", test_pad_wrap},
    {"sega.banks", test_sega_banks},
    {"sega.mirror", test_sega_mirror},
    {"sega.ram", test_sega_ram},
    {"sega.shift", test_sega_shift},
    {"sega.wrap", test_sega_wrap},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-mapper <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-mapper: unknown case %s\n", argv[1]);
    return 2;
}
