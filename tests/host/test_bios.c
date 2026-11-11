/* The BIOS helper. The case is named on the command line, followed by a
   scratch directory, an owned 8192-byte file, its SHA-1 as 40 hex digits
   (computed by CMake, not by the core) and a second owned 8192-byte file with
   other bytes. No firmware is involved: every digest here is a published test
   vector, a row's own digest or one CMake computed over bytes this test owns,
   and the recognition tables other than the built-in one are injected. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bios.h"
#include "../check.h"

#define GUARD 16u
#define BUF_LEN (COGMASTER_BIOS_READ_CAP + GUARD)
#define PATH_LEN 1024u
#define SENTINEL 0xA5u

static uint8_t buf[BUF_LEN];

/* The SHA-1 known-answer vector for the three bytes abc (RFC 3174, section 7.3). */
static const uint8_t abc_sha1[20] = {0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
                                     0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d};

static void fill_guard(void)
{
    memset(buf, (int)SENTINEL, sizeof buf);
}

/* Bytes of buf from index first that no longer hold the sentinel. */
static uint32_t guard_hits(size_t first)
{
    uint32_t bad = 0;
    for (size_t i = first; i < sizeof buf; i++) {
        bad += (uint32_t)(buf[i] != SENTINEL);
    }
    return bad;
}

static uint8_t counter_byte(size_t i)
{
    return (uint8_t)((i * 7u + 1u) & 0xffu);
}

static void join(char *out, const char *dir, const char *name)
{
    int n = snprintf(out, PATH_LEN, "%s/%s", dir, name);
    CHECK(n > 0 && (size_t)n < PATH_LEN);
}

/* An owned file of count counter bytes. */
static void write_counter(const char *path, size_t count)
{
    FILE *f = fopen(path, "wb");
    size_t ok = 0;
    CHECK(f != NULL);
    if (f == NULL) {
        return;
    }
    ok = 1;
    for (size_t i = 0; i < count; i++) {
        ok &= (size_t)(fputc(counter_byte(i), f) != EOF);
    }
    ok &= (size_t)(fclose(f) == 0);
    CHECK_EQ_U64(ok, 1u);
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static int parse_sha1(const char *hex, uint8_t out[20])
{
    if (strlen(hex) != 40u) {
        return 0;
    }
    for (size_t i = 0; i < 20u; i++) {
        int hi = hex_digit(hex[2u * i]);
        int lo = hex_digit(hex[2u * i + 1u]);
        if (hi < 0 || lo < 0) {
            return 0;
        }
        out[i] = (uint8_t)(hi * 16 + lo);
    }
    return 1;
}

/* A row's own digest and size identify to itself, a bit flipped in the digest
   or one more byte of size identify nothing. */
static void table(void)
{
    size_t count = 0;
    const struct cogmaster__bios_row *rows = cogmaster__bios_rows(&count);
    CHECK_EQ_U64(count, 13u);
    CHECK(rows != NULL);
    for (size_t i = 0; rows != NULL && i < count; i++) {
        uint8_t flipped[20];
        const char *got = cogmaster__bios_lookup(rows, count, rows[i].sha1, rows[i].size);
        CHECK(got != NULL && strcmp(got, rows[i].name) == 0);
        memcpy(flipped, rows[i].sha1, 20);
        flipped[i % 20u] ^= 0x01u;
        CHECK(cogmaster__bios_lookup(rows, count, flipped, rows[i].size) == NULL);
        CHECK(cogmaster__bios_lookup(rows, count, rows[i].sha1, rows[i].size + 1u) == NULL);
        CHECK(cogmaster__bios_lookup(rows, 0u, rows[i].sha1, rows[i].size) == NULL);
    }
}

/* The real path, bytes to digest to name, against a one-row injected table. */
static void vector(void)
{
    struct cogmaster__bios_row row = {"vector", 3u, {0}};
    uint8_t out[20];
    const char *got;
    memcpy(row.sha1, abc_sha1, 20);

    memset(out, 0, sizeof out);
    got = cogmaster__bios_identify(&row, 1u, (const uint8_t *)"abc", 3u, out);
    CHECK(got != NULL && strcmp(got, "vector") == 0);
    CHECK(memcmp(out, abc_sha1, 20) == 0);

    got = cogmaster__bios_identify(&row, 1u, (const uint8_t *)"abd", 3u, out);
    CHECK(got == NULL);
    CHECK(memcmp(out, abc_sha1, 20) != 0);

    row.sha1[0] ^= 0x10u;
    got = cogmaster__bios_identify(&row, 1u, (const uint8_t *)"abc", 3u, out);
    CHECK(got == NULL);
    CHECK(memcmp(out, abc_sha1, 20) == 0);
    row.sha1[0] ^= 0x10u;

    row.size = 4u;
    got = cogmaster__bios_identify(&row, 1u, (const uint8_t *)"abc", 3u, out);
    CHECK(got == NULL);
}

/* An owned file read through the helper is identified against a row holding
   the digest CMake computed for it. */
static void owned(const char *path_a, const char *hex_a, const char *path_b)
{
    struct cogmaster__bios_row row = {"owned", COGMASTER_BIOS_SIZE, {0}};
    uint8_t out[20];
    uint8_t out_b[20];
    size_t n = 0;
    const char *got;
    int ok = parse_sha1(hex_a, row.sha1);
    CHECK(ok);
    if (!ok) {
        return;
    }

    fill_guard();
    CHECK_EQ_U64(cogmaster__bios_read(path_a, buf, &n), COGMASTER_BIOS_READ_OK);
    CHECK_EQ_U64(n, COGMASTER_BIOS_SIZE);
    CHECK_EQ_U64(guard_hits(n), 0u);
    /* The file holds 0123456789abcdef repeated and a final LF. */
    CHECK(n >= 16u && memcmp(buf, "0123456789abcdef", 16) == 0);

    memset(out, 0, sizeof out);
    got = cogmaster__bios_identify(&row, 1u, buf, n, out);
    CHECK(got != NULL && strcmp(got, "owned") == 0);
    CHECK(memcmp(out, row.sha1, 20) == 0);

    /* Controls: each must identify nothing. */
    row.sha1[19] ^= 0x01u;
    CHECK(cogmaster__bios_identify(&row, 1u, buf, n, out) == NULL);
    row.sha1[19] ^= 0x01u;
    row.size = COGMASTER_BIOS_SIZE + 1u;
    CHECK(cogmaster__bios_identify(&row, 1u, buf, n, out) == NULL);
    row.size = COGMASTER_BIOS_SIZE;
    CHECK(cogmaster__bios_identify(&row, 0u, buf, n, out) == NULL);

    fill_guard();
    CHECK_EQ_U64(cogmaster__bios_read(path_b, buf, &n), COGMASTER_BIOS_READ_OK);
    CHECK_EQ_U64(n, COGMASTER_BIOS_SIZE);
    CHECK(cogmaster__bios_identify(&row, 1u, buf, n, out_b) == NULL);
    CHECK(memcmp(out_b, row.sha1, 20) != 0);
}

/* One read of a file of count counter bytes: the status, the count, the bytes
   and the untouched sentinel after them. */
static void read_counter(const char *dir, const char *name, size_t count, size_t expect)
{
    char path[PATH_LEN];
    size_t n = 99u;
    uint32_t bad = 0;
    join(path, dir, name);
    write_counter(path, count);
    fill_guard();
    CHECK_EQ_U64(cogmaster__bios_read(path, buf, &n), COGMASTER_BIOS_READ_OK);
    CHECK_EQ_U64(n, expect);
    for (size_t i = 0; i < n && i < sizeof buf; i++) {
        bad += (uint32_t)(buf[i] != counter_byte(i));
    }
    CHECK_EQ_U64(bad, 0u);
    CHECK_EQ_U64(guard_hits(expect), 0u);
}

static void reads(const char *dir, const char *path_a)
{
    char path[PATH_LEN];
    size_t n = 99u;

    join(path, dir, "absent.bin");
    fill_guard();
    CHECK_EQ_U64(cogmaster__bios_read(path, buf, &n), COGMASTER_BIOS_READ_ABSENT);
    CHECK_EQ_U64(n, 0u);
    CHECK_EQ_U64(guard_hits(0u), 0u);

#ifndef _WIN32
    /* A path below a regular file is also absent; Windows reports it as not found. */
    n = 99u;
    join(path, path_a, "below");
    CHECK_EQ_U64(cogmaster__bios_read(path, buf, &n), COGMASTER_BIOS_READ_ABSENT);
    CHECK_EQ_U64(n, 0u);
#else
    (void)path_a;
#endif

    n = 99u;
    fill_guard();
    CHECK_EQ_U64(cogmaster__bios_read(dir, buf, &n), COGMASTER_BIOS_READ_ERROR);
    CHECK_EQ_U64(n, 0u);
    CHECK_EQ_U64(guard_hits(0u), 0u);

    n = 99u;
    CHECK_EQ_U64(cogmaster__bios_read(NULL, buf, &n), COGMASTER_BIOS_READ_ERROR);
    CHECK_EQ_U64(n, 0u);

    read_counter(dir, "big.bin", 16384u, COGMASTER_BIOS_READ_CAP);
    read_counter(dir, "cap.bin", COGMASTER_BIOS_READ_CAP, COGMASTER_BIOS_READ_CAP);
    read_counter(dir, "bios.bin", COGMASTER_BIOS_SIZE, COGMASTER_BIOS_SIZE);
    read_counter(dir, "empty.bin", 0u, 0u);

#if !defined(_WIN32) && !defined(__wasi__)
    /* Windows and WASI have no such device; elsewhere it never ends. */
    n = 0u;
    fill_guard();
    CHECK_EQ_U64(cogmaster__bios_read("/dev/zero", buf, &n), COGMASTER_BIOS_READ_OK);
    CHECK_EQ_U64(n, COGMASTER_BIOS_READ_CAP);
    CHECK_EQ_U64(guard_hits(COGMASTER_BIOS_READ_CAP), 0u);
#endif
}

int main(int argc, char **argv)
{
    if (argc == 6 && strcmp(argv[1], "bios") == 0) {
        table();
        vector();
        owned(argv[3], argv[4], argv[5]);
        reads(argv[2], argv[3]);
    } else {
        fprintf(stderr, "usage: cogmaster-test-bios-host bios DIR FILE SHA1 OTHER\n");
        return 2;
    }
    CHECK_DONE();
}
