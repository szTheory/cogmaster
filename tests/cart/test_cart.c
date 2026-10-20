/* cogmaster_probe over synthetic images built here, with no instance:
   cogmaster-test-cart <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "../check.h"

#define KIB 1024u
#define MIB (1024u * 1024u)
#define IMAGE_MAX (MIB + 512u)

static uint8_t image[IMAGE_MAX + 64u];

/* The ROM header magic as the BIOS compares it, written as bytes. */
static const uint8_t header_magic[8] = {0x54u, 0x4Du, 0x52u, 0x20u, 0x53u, 0x45u, 0x47u, 0x41u};

/* The Codemasters header at $7FE0 as literal bytes: 8 pages, 15 June 1994
   12:30, checksum $1234 and its complement $EDCC, $7FEF zero. */
static const uint8_t codemasters_header[16] = {0x08u, 0x15u, 0x06u, 0x94u, 0x12u, 0x30u,
                                               0x34u, 0x12u, 0xCCu, 0xEDu, 0x00u, 0x00u,
                                               0x00u, 0x00u, 0x00u, 0x00u};

static cogmaster_cart_info new_info(void)
{
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    return info;
}

static cogmaster_status probe(size_t n, cogmaster_cart_info *info)
{
    *info = new_info();
    return cogmaster_probe(image, n, info);
}

/* n bytes of a pattern with no $32 byte, so the opcode scan sees no
   stores. */
static void fill(size_t n)
{
    for (size_t i = 0; i < n; i++) {
        image[i] = (uint8_t)(i % 37u);
    }
}

/* A ROM header at at (relative to base): product code bytes, version,
   region code and size code. */
static void put_header(size_t base, uint32_t at, uint8_t region, uint8_t size_code)
{
    uint8_t *h = image + base + at;
    memcpy(h, header_magic, sizeof header_magic);
    h[8] = 0x00u;
    h[9] = 0x00u;
    h[12] = 0x26u;
    h[13] = 0x70u;
    h[14] = 0xA3u;
    h[15] = (uint8_t)(region << 4 | size_code);
}

/* The sum the export BIOS compares for a header at at covering range bytes
   from base, skipping the header, stored at $xFFA. */
static uint32_t seal(size_t base, uint32_t at, uint32_t range)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < range; i++) {
        if (i < at || i >= at + 16u) {
            sum += image[base + i];
        }
    }
    sum &= 0xFFFFu;
    image[base + at + 10u] = (uint8_t)sum;
    image[base + at + 11u] = (uint8_t)(sum >> 8);
    return sum;
}

/* count LD (nn),A stores to target, from offset 0x100 on. */
static void stores(uint32_t target, uint32_t count, size_t from)
{
    for (uint32_t i = 0; i < count; i++) {
        uint8_t *p = image + from + 3u * i;
        p[0] = 0x32u;
        p[1] = (uint8_t)target;
        p[2] = (uint8_t)(target >> 8);
    }
}

static void expect_board(const char *what, size_t n, cogmaster_status want_status,
                         uint32_t want_board, uint32_t want_at, cogmaster_cart_info *info)
{
    cogmaster_status st = probe(n, info);
    if (st != want_status || info->board != want_board || info->header_at != want_at ||
        info->image_size != n) {
        fprintf(stderr, "%s: status %lu board %lu header_at 0x%lx image_size %lu\n", what,
                (unsigned long)st, (unsigned long)info->board, (unsigned long)info->header_at,
                (unsigned long)info->image_size);
        check_failures++;
    }
}

/* One image per branch of the board guess, in its order. */
static void test_branches(void)
{
    cogmaster_cart_info info;

    /* A copier header: 64 KiB + 512 bytes, the ROM header 512 bytes in. */
    fill(64u * KIB + 512u);
    put_header(512u, 0x7FF0u, 4u, 0xEu);
    expect_board("copier header", 64u * KIB + 512u, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA,
                 0x7FF0u, &info);
    CHECK_EQ_U64(info.body_offset, 512);

    /* A header at $7FF0 with a good checksum over 32 KiB (size code $C):
       every field decoded; no mapper at 32 KiB. */
    fill(32u * KIB);
    put_header(0u, 0x7FF0u, 4u, 0xCu);
    uint32_t sum = seal(0u, 0x7FF0u, 0x7FF0u);
    expect_board("header $7FF0", 32u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY, 0x7FF0u,
                 &info);
    CHECK_EQ_U64(info.body_offset, 0);
    CHECK_EQ_HEX(info.checksum, sum);
    CHECK_EQ_U64(info.checksum_ok, 1);
    CHECK_EQ_U64(info.product_code, 107026);
    CHECK_EQ_U64(info.version, 3);
    CHECK_EQ_U64(info.region_code, 4);
    CHECK_EQ_HEX(info.rom_size_code, 0xC);
    CHECK_EQ_U64(info.machine_guess, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(info.region_guess, COGMASTER_REGION_EXPORT);
    /* A bad checksum is recorded and refuses nothing. */
    image[0x100u] ^= 0x01u;
    expect_board("bad checksum", 32u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY, 0x7FF0u,
                 &info);
    CHECK_EQ_U64(info.checksum_ok, 0);
    CHECK_EQ_HEX(info.checksum, sum);

    /* $7FF0 wins over $3FF0; without it, $3FF0 is read: a Game Gear export
       header. */
    put_header(0u, 0x3FF0u, 6u, 0xBu);
    expect_board("both headers", 32u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY, 0x7FF0u,
                 &info);
    memset(image + 0x7FF0u, 0, 16u);
    expect_board("header $3FF0", 32u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY, 0x3FF0u,
                 &info);
    CHECK_EQ_U64(info.machine_guess, COGMASTER_MACHINE_GG);
    CHECK_EQ_U64(info.region_guess, COGMASTER_REGION_EXPORT);

    /* A header at $1FF0 in 16 KiB: a Japanese Master System image. */
    fill(16u * KIB);
    put_header(0u, 0x1FF0u, 3u, 0xAu);
    seal(0u, 0x1FF0u, 0x1FF0u);
    expect_board("header $1FF0", 16u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY, 0x1FF0u,
                 &info);
    CHECK_EQ_U64(info.checksum_ok, 1);
    CHECK_EQ_U64(info.machine_guess, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(info.region_guess, COGMASTER_REGION_JAPAN);

    /* A Codemasters header at $7FE0 in 128 KiB. */
    fill(128u * KIB);
    memcpy(image + 0x7FE0u, codemasters_header, sizeof codemasters_header);
    expect_board("codemasters", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_CODEMASTERS, 0u,
                 &info);
    CHECK_EQ_U64(info.machine_guess, 0);
    /* $7FEF not zero, or a checksum without its complement, is no header. */
    image[0x7FEFu] = 1u;
    expect_board("codemasters $7FEF", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA, 0u,
                 &info);
    image[0x7FEFu] = 0u;
    image[0x7FE8u] ^= 1u;
    expect_board("codemasters complement", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA,
                 0u, &info);

    /* 48 KiB with no header: no mapper. */
    fill(48u * KIB);
    expect_board("48 KiB", 48u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY, 0u, &info);
    CHECK_EQ_U64(info.machine_guess, 0);
    CHECK_EQ_U64(info.region_guess, 0);

    /* Above 48 KiB, stores to $A000 (the Korean board's register) outnumber
       stores to the Sega registers: refused, every other field filled. */
    fill(64u * KIB);
    put_header(0u, 0x7FF0u, 4u, 0xEu);
    stores(0xA000u, 4u, 0x100u);
    stores(0xFFFFu, 2u, 0x200u);
    expect_board("$A000 stores", 64u * KIB, COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD, 0u, 0x7FF0u,
                 &info);
    CHECK_EQ_U64(info.region_code, 4);
    CHECK_EQ_U64(info.machine_guess, COGMASTER_MACHINE_SMS2);
    /* Stores to the Sega registers win: the Sega board. */
    stores(0xFFFFu, 6u, 0x200u);
    expect_board("$FFFF stores", 64u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA, 0x7FF0u,
                 &info);
    /* Stores past the first 32 KiB are not scanned. */
    fill(64u * KIB);
    stores(0xA000u, 4u, 0x8000u);
    expect_board("late stores", 64u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA, 0u, &info);

    /* An 8 KiB image whose header names the Game Gear: refused. The same
       size naming the Master System loads with no mapper. */
    fill(8u * KIB);
    put_header(0u, 0x1FF0u, 5u, 0xAu);
    expect_board("8 KiB Game Gear", 8u * KIB, COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD, 0u, 0x1FF0u,
                 &info);
    CHECK_EQ_U64(info.machine_guess, COGMASTER_MACHINE_GG);
    put_header(0u, 0x1FF0u, 4u, 0xAu);
    expect_board("8 KiB Master System", 8u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ROM_ONLY,
                 0x1FF0u, &info);
}

/* splitmix64 (Steele, Lea and Flood, 2014): a seeded sequence the same on
   every platform. */
static uint64_t splitmix64(uint64_t *s)
{
    uint64_t z = (*s += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* 500 seeded images, each copied into a heap buffer of exactly its size so
   a read past the end meets the allocation's edge: probing twice gives the
   same bytes out, and the buffer is unchanged. Sizes cluster on the size,
   copier-header and 48 KiB boundaries; some images carry headers and
   stores. */
static void test_pure(void)
{
    static const size_t bases[] = {1023u,      1024u,          8u * KIB,  16u * KIB + 512u,
                                   48u * KIB,  48u * KIB + 1u, 64u * KIB, MIB,
                                   MIB + 512u, MIB + 513u};
    uint64_t seed = UINT64_C(0xC0C0A5A55EED0009);
    uint8_t *copy = (uint8_t *)malloc(IMAGE_MAX + 64u);
    if (copy == NULL) {
        exit(1);
    }
    uint32_t differ = 0, changed = 0, statuses[16] = {0};
    for (uint32_t i = 0; i < 500u; i++) {
        uint64_t r = splitmix64(&seed);
        size_t base = bases[r % (sizeof bases / sizeof bases[0])];
        size_t n = base + (size_t)((r >> 8) % 3u) - 1u;
        if (n > IMAGE_MAX + 64u) {
            n = IMAGE_MAX + 64u;
        }
        uint8_t *buf = (uint8_t *)malloc(n);
        if (buf == NULL) {
            exit(1);
        }
        for (size_t j = 0; j < n; j++) {
            buf[j] = (uint8_t)splitmix64(&seed);
        }
        size_t off = (n & 0x3FFFu) == 512u ? 512u : 0u;
        uint32_t kind = (uint32_t)(r >> 16) % 4u;
        if (kind == 1u && n >= off + 0x8000u) {
            memcpy(buf + off + 0x7FF0u, header_magic, sizeof header_magic);
        } else if (kind == 2u && n >= off + 0x8000u) {
            memcpy(buf + off + 0x7FE0u, codemasters_header, sizeof codemasters_header);
        }
        memcpy(copy, buf, n);
        cogmaster_cart_info a = new_info(), b = new_info();
        cogmaster_status sa = cogmaster_probe(buf, n, &a);
        cogmaster_status sb = cogmaster_probe(buf, n, &b);
        differ += sa != sb || memcmp(&a, &b, sizeof a) != 0;
        changed += memcmp(copy, buf, n) != 0;
        statuses[sa & 15u]++;
        free(buf);
    }
    free(copy);
    printf("probes: 500 run; ok %lu, size %lu, board %lu\n", (unsigned long)statuses[0],
           (unsigned long)statuses[COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE],
           (unsigned long)statuses[COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD]);
    CHECK_EQ_U64(differ, 0);
    CHECK_EQ_U64(changed, 0);
    CHECK(statuses[0] >= 100u);
    CHECK(statuses[COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE] >= 20u);

    /* No image: refused without a read. No output or a short size tag. */
    cogmaster_cart_info info = new_info();
    cogmaster_status st = cogmaster_probe(NULL, 0u, &info);
    CHECK(st == COGMASTER_STATUS_ERR_ARGUMENT || st == COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    CHECK_EQ_U64(cogmaster_probe(NULL, 4096u, &info), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_probe(image, 4096u, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    info.size = 55u;
    CHECK_EQ_U64(cogmaster_probe(image, 4096u, &info), COGMASTER_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(info.board, 0);
    /* A larger size tag keeps the bytes past ours. */
    union {
        cogmaster_cart_info info;
        uint8_t bytes[sizeof(cogmaster_cart_info) + 8u];
    } big;
    memset(&big, 0xA5, sizeof big);
    big.info.size = (uint32_t)sizeof big;
    fill(4096u);
    CHECK_EQ_U64(cogmaster_probe(image, 4096u, &big.info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(big.info.size, sizeof big);
    CHECK_EQ_U64(big.info.board, COGMASTER_BOARD_ROM_ONLY);
    CHECK_EQ_HEX(big.bytes[sizeof(cogmaster_cart_info)], 0xA5);
    CHECK_EQ_HEX(big.bytes[sizeof big - 1u], 0xA5);
}

/* The size bounds: 1,023 bytes and 1 MiB + 513 are refused with the size
   filled in; 1,024 and 1 MiB + 512 (a copier header) are probed. */
static void test_sizes(void)
{
    cogmaster_cart_info info;
    fill(IMAGE_MAX + 1u);
    CHECK_EQ_U64(probe(1023u, &info), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    CHECK_EQ_U64(info.image_size, 1023);
    CHECK_EQ_U64(info.board, 0);
    CHECK_EQ_U64(probe(MIB + 513u, &info), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    CHECK_EQ_U64(info.image_size, MIB + 513u);
    CHECK_EQ_U64(info.body_offset, 0);
    CHECK_EQ_U64(probe(1024u, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.image_size, 1024);
    CHECK_EQ_U64(info.board, COGMASTER_BOARD_ROM_ONLY);
    CHECK_EQ_U64(probe(MIB + 512u, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.body_offset, 512);
    CHECK_EQ_U64(info.board, COGMASTER_BOARD_SEGA);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"probe.branches", test_branches},
    {"probe.pure", test_pure},
    {"probe.sizes", test_sizes},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-cart <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-cart: unknown case %s\n", argv[1]);
    return 2;
}
