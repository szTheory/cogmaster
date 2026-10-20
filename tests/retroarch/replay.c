#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    uint8_t b[262144];
    if (argc != 3 && !(argc == 4 && strcmp(argv[3], "gg") == 0))
        return 2;
    size_t count = argc == 4 ? 13u : 14u, stride = 8u + count * 8u;
    FILE *f = fopen(argv[1], "rb");
    if (!f)
        return 2;
    size_t n = fread(b, 1, sizeof b, f);
    int bad = fclose(f) != 0;
    if (bad || n < 13u * stride || n == sizeof b || memcmp(b, "2VSB\2\0\0\0", 8))
        return 1;
    size_t start = n - (13u * stride - 4u);
    for (size_t frame = 0; frame < 13; frame++) {
        size_t off = start + frame * stride;
        if (b[off] || b[off + 1] != count || b[off + 2] || b[off + 3u + count * 8u] != 102)
            return 1;
        for (size_t i = off + 3; i < off + 3u + count * 8u; i += 8) {
            if (b[i + 6] || b[i + 7])
                return 1;
            if (!b[i] && b[i + 1] == 1 && !b[i + 2] && b[i + 4] == (argc == 4 ? 3u : 0u) &&
                !b[i + 5]) {
                b[i + 6] = (uint8_t)(frame == 4 || frame == 5 || frame == 7);
            }
        }
    }
    f = fopen(argv[2], "wb");
    if (!f)
        return 2;
    bad = fwrite(b, 1, n, f) != n;
    return fclose(f) != 0 || bad;
}
