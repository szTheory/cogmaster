#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../fixtures/play.h"
int bmp_read(const char *path, uint32_t *w, uint32_t *h, uint8_t **rgb);
static unsigned compare_hex(const uint8_t *p)
{
    return (unsigned)p[0] << 16 | (unsigned)p[1] << 8 | (unsigned)p[2];
}
int main(int argc, char **argv)
{
    if (argc != 3 && !(argc == 4 && strcmp(argv[3], "--flip") == 0)) {
        fprintf(stderr, "usage: compare EXPECTED.bmp ACTUAL.bmp\n");
        return 2;
    }
    uint32_t w[2], h[2];
    uint8_t *rgb[2] = {NULL, NULL};
    int rc = 0;
    const char *scroll = getenv("EXPECT_SCROLL");
    for (int i = 0; i < 2 && rc == 0; i++) {
        if (bmp_read(argv[i + 1], &w[i], &h[i], &rgb[i]) != 0) {
            fprintf(stderr, "compare: cannot read %s as a BMP\n", argv[i + 1]);
            rc = 2;
        }
    }
    if (rc == 0 && (w[0] != w[1] || h[0] != h[1])) {
        printf("size differs: expected %ux%u, actual %ux%u\n", (unsigned)w[0], (unsigned)h[0],
               (unsigned)w[1], (unsigned)h[1]);
        rc = 1;
    }
    if (rc == 0 && argc == 4 && w[1] > 7u && h[1] > 3u)
        rgb[1][((size_t)3u * w[1] + 7u) * 3u] ^= 255u;
    for (uint32_t y = 0; rc == 0 && y < h[0]; y++) {
        for (uint32_t x = 0; rc == 0 && x < w[0]; x++) {
            const uint8_t *a = rgb[0] + ((size_t)y * w[0] + x) * 3u;
            const uint8_t *b = rgb[1] + ((size_t)y * w[0] + x) * 3u;
            if (scroll != NULL) {
                uint16_t p = play_pixel(x, y, (uint32_t)strtoul(scroll, NULL, 10));
                unsigned want =
                    ((unsigned)(p & 15u) << 16 | (unsigned)(p & 240u) << 4 | (unsigned)(p >> 8)) *
                    17u;
                if (w[0] != 256u || h[0] != 192u || compare_hex(a) != want) {
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
