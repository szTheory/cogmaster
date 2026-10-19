/* The runner's PNG encoder, checked by a parse that shares no code with it:
   chunk CRCs with a bitwise CRC-32, stored blocks reassembled by hand and
   Adler-32 by its definition. Cases:

     png             a 2 x 2 fixture and the checksums' published values
     png_file PATH   PATH, written by cogmaster-run --png, is the SMS card:
                     256 x 192 RGB, every pixel the card's RGB444 dot with
                     each channel times 0x11 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../../runner/crc32.h"
#include "../../runner/png.h"
#include "../check.h"

/* CRC-32 one bit at a time (PNG specification, Annex D), sharing nothing
   with the encoder's table. */
static uint32_t bitwise_crc(const uint8_t *p, size_t n)
{
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) {
            c = (c & 1u) != 0u ? 0xedb88320u ^ (c >> 1) : c >> 1;
        }
    }
    return ~c;
}

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* Parses the PNG in d (n bytes) as a width x height RGB image and copies its
   decoded image data, one filter byte and 3 * width bytes per row, into raw.
   Returns 0 when every structural check passed. */
static int decode(const uint8_t *d, size_t n, uint32_t width, uint32_t height, uint8_t *raw)
{
    static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    int before = check_failures;
    uint32_t raw_size = height * (1u + 3u * width);
    CHECK(n >= 8u && memcmp(d, signature, 8) == 0);
    if (n < 8u) {
        return -1;
    }

    /* Exactly IHDR, IDAT, IEND, each with a valid CRC. */
    static const char *const want[3] = {"IHDR", "IDAT", "IEND"};
    const uint8_t *chunk_data[3] = {NULL, NULL, NULL};
    uint32_t chunk_len[3] = {0, 0, 0};
    size_t at = 8;
    for (int c = 0; c < 3; c++) {
        CHECK(at + 12u <= n);
        if (at + 12u > n) {
            return -1;
        }
        uint32_t len = be32(d + at);
        CHECK(len <= n - at - 12u);
        if (len > n - at - 12u) {
            return -1;
        }
        CHECK(memcmp(d + at + 4, want[c], 4) == 0);
        CHECK_EQ_HEX(be32(d + at + 8 + len), bitwise_crc(d + at + 4, 4u + (size_t)len));
        chunk_data[c] = d + at + 8;
        chunk_len[c] = len;
        at += 12u + len;
    }
    CHECK_EQ_U64(at, n);

    /* IHDR: width, height, depth 8, colour type 2, compression, filter and
       interlace 0 (PNG section 11.2.1). */
    uint8_t ihdr[13] = {0, 0, 0, 0, 0, 0, 0, 0, 8, 2, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        ihdr[i] = (uint8_t)(width >> (24 - 8 * i));
        ihdr[4 + i] = (uint8_t)(height >> (24 - 8 * i));
    }
    CHECK_EQ_U64(chunk_len[0], 13u);
    CHECK(chunk_len[0] == 13u && memcmp(chunk_data[0], ihdr, 13) == 0);
    CHECK_EQ_U64(chunk_len[2], 0u);

    /* IDAT: zlib header 78 01, stored blocks, Adler-32. */
    const uint8_t *z = chunk_data[1];
    uint32_t zn = chunk_len[1];
    CHECK(zn >= 6u && z[0] == 0x78 && z[1] == 0x01);
    uint32_t pos = 2, got = 0, blocks = 0;
    int final = 0;
    while (!final && pos + 5u <= zn) {
        CHECK((z[pos] & 0xfeu) == 0u); /* BTYPE 00, no other bits */
        final = z[pos] & 1;
        uint32_t len = z[pos + 1] | ((uint32_t)z[pos + 2] << 8);
        uint32_t nlen = z[pos + 3] | ((uint32_t)z[pos + 4] << 8);
        CHECK_EQ_HEX(nlen, ~len & 0xffffu);
        pos += 5u;
        if (pos + len > zn || got + len > raw_size) {
            CHECK(0);
            return -1;
        }
        memcpy(raw + got, z + pos, len);
        pos += len;
        got += len;
        blocks++;
    }
    CHECK(final);
    CHECK_EQ_U64(got, raw_size);
    CHECK_EQ_U64(blocks, (raw_size + 65534u) / 65535u);
    CHECK_EQ_U64(pos + 4u, zn);
    if (pos + 4u != zn) {
        return -1;
    }
    uint32_t s1 = 1, s2 = 0;
    for (uint32_t i = 0; i < got; i++) {
        s1 = (s1 + raw[i]) % 65521u;
        s2 = (s2 + s1) % 65521u;
    }
    CHECK_EQ_HEX(be32(z + zn - 4u), (s2 << 16) | s1);

    /* Filter type 0 (None) on every row. */
    uint32_t bad = 0;
    for (uint32_t y = 0; y < height; y++) {
        bad += (uint32_t)(raw[y * (1u + 3u * width)] != 0u);
    }
    CHECK_EQ_U64(bad, 0u);
    return check_failures == before ? 0 : -1;
}

/* A 2 x 2 image round-trips through the encoder; sizes it cannot hold are
   refused. */
static void png(void)
{
    /* Published check values: CRC-32 of "123456789" (PNG specification,
       Annex D and the CRC catalogue), Adler-32 of "Wikipedia". */
    CHECK_EQ_HEX(cogmaster__runner_crc32(0, (const uint8_t *)"123456789", 9), 0xCBF43926u);
    CHECK_EQ_HEX(bitwise_crc((const uint8_t *)"123456789", 9), 0xCBF43926u);
    CHECK_EQ_HEX(cogmaster__adler32(1, (const uint8_t *)"Wikipedia", 9), 0x11E60398u);

    static const uint8_t rgb[12] = {0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
                                    0x00, 0x00, 0xff, 0x11, 0x22, 0x33};
    /* Signature 8, IHDR 25, IDAT 12 + 2 + 5 + 14 + 4, IEND 12. */
    CHECK_EQ_U64(cogmaster__png_size(2, 2), 82u);
    CHECK_EQ_U64(cogmaster__png_size(0, 2), 0u);
    CHECK_EQ_U64(cogmaster__png_size(2, 0), 0u);
    CHECK_EQ_U64(cogmaster__png_size(65536u, 65536u), 0u);

    uint8_t out[83];
    memset(out, 0xA5, sizeof out);
    CHECK_EQ_U64(cogmaster__png_write(out, 81, rgb, 2, 2), 0u);
    CHECK_EQ_U64(cogmaster__png_write(out, sizeof out, rgb, 2, 2), 82u);
    CHECK_EQ_HEX(out[82], 0xA5u);

    uint8_t raw[14];
    memset(raw, 0, sizeof raw);
    if (decode(out, 82, 2, 2, raw) == 0) {
        CHECK(memcmp(raw + 1, rgb, 6) == 0);
        CHECK(memcmp(raw + 8, rgb + 6, 6) == 0);
    }
}

/* Reads the file at path into a heap buffer; *n is its size. */
static uint8_t *read_file(const char *path, size_t limit, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    uint8_t *buf = (uint8_t *)malloc(limit + 1u);
    *n = buf == NULL ? 0u : fread(buf, 1, limit + 1u, f);
    fclose(f);
    return buf;
}

/* The runner's PNG of frame 60 is the SMS card through the times-0x11 map. */
static void png_file(const char *path)
{
    enum { W = 256, H = 192 };
    size_t limit = cogmaster__png_size(W, H);
    size_t n = 0;
    uint8_t *d = read_file(path, limit, &n);
    uint8_t *raw = (uint8_t *)malloc((size_t)H * (1u + 3u * W));
    CHECK(d != NULL && raw != NULL);
    if (d != NULL && raw != NULL && decode(d, n, W, H, raw) == 0) {
        uint32_t bad = 0;
        for (uint32_t y = 0; y < H; y++) {
            const uint8_t *row = raw + y * (1u + 3u * W) + 1u;
            for (uint32_t x = 0; x < W; x++) {
                uint32_t r = row[3u * x], g = row[3u * x + 1u], b = row[3u * x + 2u];
                bad += (uint32_t)(r % 0x11u != 0u || g % 0x11u != 0u || b % 0x11u != 0u);
                uint32_t p = (r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8);
                bad += (uint32_t)(p != cogmaster__testcard_pixel(x, y, COGMASTER_MACHINE_SMS2));
            }
        }
        CHECK_EQ_U64(bad, 0u);
    }
    free(d);
    free(raw);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "png") == 0) {
        png();
    } else if (argc == 3 && strcmp(argv[1], "png_file") == 0) {
        png_file(argv[2]);
    } else {
        fprintf(stderr, "usage: cogmaster-test-runner png | png_file PATH\n");
        return 2;
    }
    CHECK_DONE();
}
