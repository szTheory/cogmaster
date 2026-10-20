#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    uint8_t b[262144];
    if (argc != 3)
        return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f)
        return 2;
    size_t n = fread(b, 1, sizeof b, f);
    int bad = fclose(f) != 0;
    if (bad || n < 1600 || n == sizeof b || memcmp(b, "2VSB\2\0\0\0", 8))
        return 1;
    size_t start = n - 1556;
    for (size_t frame = 0; frame < 13; frame++) {
        size_t off = start + frame * 120;
        if (b[off] || b[off + 1] != 14 || b[off + 2] || b[off + 115] != 102)
            return 1;
        for (size_t i = off + 3; i < off + 115; i += 8) {
            if (b[i + 6] || b[i + 7])
                return 1;
            if (!b[i] && b[i + 1] == 1 && !b[i + 2] && !b[i + 4] && !b[i + 5]) {
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
