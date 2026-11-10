#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../fixtures/play.h"
static unsigned compare_hex(const uint8_t *p)
{
    return (unsigned)p[0] << 16 | (unsigned)p[1] << 8 | (unsigned)p[2];
}
static uint32_t compare_le32(const uint8_t *b, size_t off)
{
    return (uint32_t)b[off] | (uint32_t)b[off + 1] << 8 | (uint32_t)b[off + 2] << 16 |
           (uint32_t)b[off + 3] << 24;
}
/* Reads a 24-bit uncompressed BMP, bottom-up or top-down, as top-to-bottom RGB rows. */
static uint8_t *compare_bmp(const uint8_t *b, size_t n, uint32_t *w, uint32_t *h)
{
    uint32_t offset = compare_le32(b, 10), height = compare_le32(b, 22);
    int top_down = (height & 0x80000000u) != 0u;
    uint32_t rows = top_down ? 0u - height : height;
    *w = compare_le32(b, 18);
    *h = rows;
    uint64_t stride = ((uint64_t)*w * 24u + 31u) / 32u * 4u;
    if (b[0] != 'B' || b[1] != 'M' || compare_le32(b, 14) != 40u ||
        compare_le32(b, 26) != 0x180001u || compare_le32(b, 30) != 0u || offset < 54u ||
        offset > n || *w == 0u || *w > n || rows == 0u || rows > n ||
        (uint64_t)offset + stride * rows > (uint64_t)n)
        return NULL;
    uint8_t *out = malloc((size_t)*w * rows * 3u);
    for (uint32_t y = 0; out != NULL && y < rows; y++) {
        const uint8_t *src = b + offset + (size_t)stride * (top_down ? y : rows - 1u - y);
        for (uint32_t i = 0; i < *w * 3u; i++)
            out[(size_t)y * *w * 3u + i] = src[i / 3u * 3u + 2u - i % 3u];
    }
    return out;
}
static uint8_t *compare_read(const char *path, uint32_t *w, uint32_t *h)
{
    FILE *f = fopen(path, "rb");
    long end = f != NULL && fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
    uint8_t *buf = end >= 70 && fseek(f, 0, SEEK_SET) == 0 ? malloc((size_t)end) : NULL,
            *out = NULL;
    if (buf != NULL && fread(buf, 1, (size_t)end, f) == (size_t)end)
        out = compare_bmp(buf, (size_t)end, w, h);
    free(buf);
    if (f != NULL)
        fclose(f);
    return out;
}
int main(int argc, char **argv)
{
    if (argc != 3 && !(argc == 4 && strcmp(argv[3], "--flip") == 0))
        return 2;
    uint32_t w[2], h[2];
    uint8_t *rgb[2] = {NULL, NULL};
    int rc = 0;
    const char *scroll = getenv("EXPECT_SCROLL");
    int gg = getenv("EXPECT_GG") != NULL;
    for (int i = 0; i < 2 && rc == 0; i++)
        if ((rgb[i] = compare_read(argv[i + 1], &w[i], &h[i])) == NULL) {
            fprintf(stderr, "compare: cannot read %s as a BMP\n", argv[i + 1]);
            rc = 2;
        }
    if (rc == 0 && (w[0] != w[1] || h[0] != h[1])) {
        puts("size differs");
        rc = 1;
    }
    if (rc == 0 && argc == 4 && w[1] > 7u && h[1] > 3u)
        rgb[1][((size_t)3u * w[1] + 7u) * 3u] ^= 255u;
    for (uint32_t y = 0; rc == 0 && y < h[0]; y++) {
        for (uint32_t x = 0; rc == 0 && x < w[0]; x++) {
            const uint8_t *a = rgb[0] + ((size_t)y * w[0] + x) * 3u;
            const uint8_t *b = rgb[1] + ((size_t)y * w[0] + x) * 3u;
            if (scroll != NULL) {
                uint16_t p = play_pixel(x + (gg ? 48u : 0u), y + (gg ? 24u : 0u),
                                        (uint32_t)strtoul(scroll, NULL, 10));
                unsigned want =
                    ((unsigned)(p & 15u) << 16 | (unsigned)(p & 240u) << 4 | (unsigned)(p >> 8)) *
                    17u;
                if (w[0] != (gg ? 160u : 256u) || h[0] != (gg ? 144u : 192u) ||
                    compare_hex(a) != want) {
                    printf("oracle pixel difference at %u,%u: expected %06X, actual %06X\n",
                           (unsigned)x, (unsigned)y, want, compare_hex(a));
                    rc = 1;
                }
            }
            if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
                printf("first difference at %u,%u: expected %06X, actual %06X\n", (unsigned)x,
                       (unsigned)y, compare_hex(a), compare_hex(b));
                rc = 1;
            }
        }
    }
    if (rc == 0)
        printf("%ux%u, every pixel equal\n", (unsigned)w[0], (unsigned)h[0]);
    free(rgb[0]);
    free(rgb[1]);
    return rc;
}
