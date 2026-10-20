/* Synthetic BMP for the independent screenshot oracle's negative control. */
#include <stdint.h>
#include <stdio.h>
#include "../fixtures/play.h"
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    FILE *f = fopen(argv[1], "wb");
    if (!f)
        return 2;
    uint8_t header[54] = {'B', 'M', 54, 64, 2, 0, 0, 0,   0, 0, 54, 0, 0, 0, 40,
                          0,   0,   0,  0,  1, 0, 0, 192, 0, 0, 0,  1, 0, 24};
    int bad = fwrite(header, 1, sizeof header, f) != sizeof header;
    for (uint32_t y = 192; y-- > 0;) {
        for (uint32_t x = 0; x < 256; x++) {
            uint16_t p = play_pixel(x, y, 0);
            uint8_t rgb[3] = {(uint8_t)((p >> 8) * 17), (uint8_t)(((p >> 4) & 15) * 17),
                              (uint8_t)((p & 15) * 17)};
            bad |= fwrite(rgb, 1, 3, f) != 3;
        }
    }
    return fclose(f) != 0 || bad;
}
