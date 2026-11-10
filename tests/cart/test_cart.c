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

/* n bytes of single-byte Z80 opcodes (LD r,r' at $40 to $64) with no $32
   byte, so the instruction-aware scan stays aligned and sees no stores; every
   store written by stores() lies on an instruction boundary. */
static void fill(size_t n)
{
    for (size_t i = 0; i < n; i++) {
        image[i] = (uint8_t)(0x40u + i % 37u);
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

    /* Above 48 KiB with a header whose checksum is bad (it is not sealed),
       four stores to $A000 (the Korean board's register) against two to the
       Sega registers are neither three times the Sega count nor beyond it:
       the doubt zone refuses, every other field filled. */
    fill(64u * KIB);
    put_header(0u, 0x7FF0u, 4u, 0xEu);
    stores(0xA000u, 4u, 0x100u);
    stores(0xFFFFu, 2u, 0x200u);
    expect_board("$A000 stores", 64u * KIB, COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD, 0u, 0x7FF0u,
                 &info);
    CHECK_EQ_U64(info.region_code, 4);
    CHECK_EQ_U64(info.machine_guess, COGMASTER_MACHINE_SMS2);
    /* The Sega registers win only at twice the other count plus two (here 10
       against 4); 6 against 4 is a refusal now. */
    stores(0xFFFFu, 6u, 0x200u);
    expect_board("$FFFF stores few", 64u * KIB, COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD, 0u, 0x7FF0u,
                 &info);
    stores(0xFFFFu, 10u, 0x200u);
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

/* The expected result of one probe: status, board, source, candidate. */
static void expect_scan(const char *what, size_t n, cogmaster_status status, uint32_t board,
                        uint32_t source, uint32_t candidate)
{
    cogmaster_cart_info info;
    cogmaster_status st = probe(n, &info);
    if (st != status || info.board != board || info.board_source != source ||
        info.board_candidate != candidate) {
        fprintf(stderr, "%s: status %lu board %lu source %lu candidate %lu\n", what,
                (unsigned long)st, (unsigned long)info.board, (unsigned long)info.board_source,
                (unsigned long)info.board_candidate);
        check_failures++;
    }
}

/* The three scan counts of the last probe. */
static void expect_counts(const char *what, size_t n, uint32_t sega, uint32_t cand, uint32_t runner)
{
    cogmaster_cart_info info;
    (void)probe(n, &info);
    if (info.scan_sega != sega || info.scan_candidate != cand || info.scan_runner_up != runner) {
        fprintf(stderr, "%s: counts %lu %lu %lu\n", what, (unsigned long)info.scan_sega,
                (unsigned long)info.scan_candidate, (unsigned long)info.scan_runner_up);
        check_failures++;
    }
}

/* Zemina stores: targets $0001 to $0003 in turn, count of them. */
static void zemina_stores(uint32_t count, size_t from)
{
    for (uint32_t i = 0; i < count; i++) {
        stores(1u + i % 3u, 1u, from + 3u * i);
    }
}

/* 4 PAK stores: targets $3FFE, $7FFF, $BFFF in turn. */
static void pak_stores(uint32_t count, size_t from)
{
    static const uint32_t t[3] = {0x3FFEu, 0x7FFFu, 0xBFFFu};
    for (uint32_t i = 0; i < count; i++) {
        stores(t[i % 3u], 1u, from + 3u * i);
    }
}

/* The scan rule: the database, then the Codemasters header, then a valid
   header, then at most 48 KiB, then store sites at instruction boundaries
   with a threshold, margins and a fitting geometry; doubt refuses with the
   candidate named. */
static void test_scan(void)
{
    cogmaster_cart_info info;
    const cogmaster_status refused = COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD;

    /* A sealed valid header outranks ten Korean-looking stores. */
    fill(128u * KIB);
    stores(0xA000u, 10u, 0x100u);
    put_header(0u, 0x7FF0u, 4u, 0xFu);
    seal(0u, 0x7FF0u, 0x20000u);
    expect_scan("decoy", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA,
                COGMASTER_BOARD_SOURCE_HEADER, 0u);
    /* A header whose range reaches past the body is no valid header. */
    fill(64u * KIB);
    stores(0xA000u, 10u, 0x100u);
    put_header(0u, 0x7FF0u, 4u, 0xFu);
    seal(0u, 0x7FF0u, 0x10000u);
    expect_scan("range past body", 64u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_KOREAN,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_KOREAN);
    /* A bad checksum, two Sega and four Korean stores: doubt. */
    fill(128u * KIB);
    stores(0xA000u, 4u, 0x100u);
    stores(0xFFFFu, 2u, 0x200u);
    put_header(0u, 0x7FF0u, 4u, 0xFu);
    expect_scan("bad checksum", 128u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_KOREAN);
    expect_counts("bad checksum counts", 128u * KIB, 2u, 4u, 0u);
    /* Below the threshold. */
    fill(128u * KIB);
    stores(0xA000u, 2u, 0x100u);
    expect_scan("below threshold", 128u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_KOREAN);
    /* Ties: five and five against another family, five against the Sega. */
    fill(128u * KIB);
    stores(0xA000u, 5u, 0x100u);
    zemina_stores(5u, 0x200u);
    expect_scan("tie", 128u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_KOREAN);
    expect_counts("tie counts", 128u * KIB, 0u, 5u, 5u);
    fill(128u * KIB);
    stores(0xA000u, 5u, 0x100u);
    stores(0xFFFFu, 5u, 0x200u);
    expect_scan("tie with Sega", 128u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_KOREAN);
    /* A $32 operand of a load is no store: 3E 32 00 A0 repeated. */
    for (size_t i = 0; i + 4u <= 64u * KIB; i += 4u) {
        image[i] = 0x3Eu;
        image[i + 1u] = 0x32u;
        image[i + 2u] = 0x00u;
        image[i + 3u] = 0xA0u;
    }
    expect_scan("operand decoy", 64u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA,
                COGMASTER_BOARD_SOURCE_DEFAULT, 0u);
    expect_counts("operand decoy counts", 64u * KIB, 0u, 0u, 0u);

    /* The families, headerless. */
    fill(128u * KIB);
    stores(0xA000u, 10u, 0x100u);
    expect_scan("korean", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_KOREAN,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_KOREAN);
    expect_counts("korean counts", 128u * KIB, 0u, 10u, 0u);
    fill(128u * KIB);
    zemina_stores(10u, 0x100u);
    expect_scan("zemina", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ZEMINA,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_ZEMINA);
    /* The Nemesis signature: DI at the first byte of the last 8 KiB bank and
       not at byte 0. */
    image[120u * KIB] = 0xF3u;
    expect_scan("nemesis", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_NEMESIS,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_NEMESIS);
    image[0] = 0xF3u;
    expect_scan("nemesis byte 0", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_ZEMINA,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_ZEMINA);
    fill(128u * KIB);
    pak_stores(10u, 0x100u);
    expect_scan("4 PAK", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_4PAK,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_4PAK);
    /* Janggun is named, not played. */
    fill(128u * KIB);
    stores(0x6000u, 10u, 0x100u);
    expect_scan("janggun", 128u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC, 0u);
    (void)probe(128u * KIB, &info);
    CHECK(info.unsupported_id != 0u);
    CHECK(strstr(info.unsupported_reason, "reverse bit") != NULL);
    /* A Game Gear region header, bad checksum, never selects a Korean board. */
    fill(128u * KIB);
    stores(0xA000u, 10u, 0x100u);
    put_header(0u, 0x7FF0u, 6u, 0xFu);
    expect_scan("game gear", 128u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_KOREAN);
    /* Geometry: 136 KiB is not whole 16 KiB pages, 64 KiB is not over 64 KiB. */
    fill(136u * KIB);
    stores(0xA000u, 10u, 0x100u);
    expect_scan("korean geometry", 136u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_KOREAN);
    fill(64u * KIB);
    zemina_stores(10u, 0x100u);
    expect_scan("zemina geometry", 64u * KIB, refused, 0u, COGMASTER_BOARD_SOURCE_HEURISTIC,
                COGMASTER_BOARD_ZEMINA);
    /* Ten Sega and four Korean stores: the Sega board by the margin. */
    fill(128u * KIB);
    stores(0xFFFFu, 10u, 0x100u);
    stores(0xA000u, 4u, 0x200u);
    expect_scan("sega margin", 128u * KIB, COGMASTER_STATUS_OK, COGMASTER_BOARD_SEGA,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_KOREAN);

    /* A copier header in front of a Korean image. */
    fill(128u * KIB + 512u);
    stores(0xA000u, 10u, 512u + 0x100u);
    expect_scan("copier korean", 128u * KIB + 512u, COGMASTER_STATUS_OK, COGMASTER_BOARD_KOREAN,
                COGMASTER_BOARD_SOURCE_HEURISTIC, COGMASTER_BOARD_KOREAN);
    (void)probe(128u * KIB + 512u, &info);
    CHECK_EQ_U64(info.body_offset, 512);
    /* A body of exactly 1 MiB with a copier header is probed. */
    fill(MIB + 512u);
    (void)probe(MIB + 512u, &info);
    CHECK_EQ_U64(info.board, COGMASTER_BOARD_SEGA);
    CHECK_EQ_U64(info.board_source, COGMASTER_BOARD_SOURCE_DEFAULT);
    /* Prefix bytes only: the sweep takes one byte per prefix and returns. */
    memset(image, 0xDD, MIB);
    CHECK_EQ_U64(probe(MIB, &info), COGMASTER_STATUS_OK);
    memset(image, 0xED, MIB);
    CHECK_EQ_U64(probe(MIB, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.scan_candidate, 0);
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
    info.size = 51u;
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
   filled in, and so is a body over 1 MiB with no copier header (1 MiB + 1
   to 1 MiB + 511); 1,024, 1 MiB and 1 MiB + 512 (a copier header) are
   probed. */
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
    CHECK_EQ_U64(probe(MIB + 1u, &info), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    CHECK_EQ_U64(info.image_size, MIB + 1u);
    CHECK_EQ_U64(info.body_offset, 0);
    CHECK_EQ_U64(probe(MIB + 511u, &info), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    CHECK_EQ_U64(info.image_size, MIB + 511u);
    CHECK_EQ_U64(probe(1024u, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.image_size, 1024);
    CHECK_EQ_U64(info.board, COGMASTER_BOARD_ROM_ONLY);
    CHECK_EQ_U64(probe(MIB, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.body_offset, 0);
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
    {"probe.scan", test_scan},
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
