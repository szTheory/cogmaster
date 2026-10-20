/* The host colour helpers. Each case is named on the command line. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "palette.h"
#include "../check.h"

/* Every RGB444 value: each RGB888 channel is the 4-bit value times 0x11, the
   top byte is 0, and c >> 4 per channel gives the value back. */
static void palette(void)
{
    uint32_t bad = 0;
    for (uint32_t p = 0; p < 4096u; p++) {
        uint32_t rgb = cogmaster__rgb444_to_rgb888((uint16_t)p);
        uint32_t r = (rgb >> 16) & 0xffu;
        uint32_t g = (rgb >> 8) & 0xffu;
        uint32_t b = rgb & 0xffu;
        bad += (uint32_t)((rgb >> 24) != 0u);
        bad += (uint32_t)(r != (p & 15u) * 0x11u);
        bad += (uint32_t)(g != ((p >> 4) & 15u) * 0x11u);
        bad += (uint32_t)(b != ((p >> 8) & 15u) * 0x11u);
        bad += (uint32_t)(((r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8)) != p);

        /* RGB565 keeps each channel's 4 bits at the top of its field. */
        uint32_t c = cogmaster__rgb444_to_rgb565((uint16_t)p);
        bad += (uint32_t)((c >> 12) != (p & 15u));
        bad += (uint32_t)(((c >> 7) & 15u) != ((p >> 4) & 15u));
        bad += (uint32_t)(((c >> 1) & 15u) != ((p >> 8) & 15u));
    }
    CHECK_EQ_U64(bad, 0u);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb888(0x0FFFu), 0x00FFFFFFu);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb888(0x0000u), 0x00000000u);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb888(0x0F00u), 0x000000FFu);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb565(0x0FFFu), 0xFFFFu);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb565(0x0000u), 0x0000u);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb565(0x000Fu), 0xF800u);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb565(0x00F0u), 0x07E0u);
    CHECK_EQ_HEX(cogmaster__rgb444_to_rgb565(0x0F00u), 0x001Fu);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "palette") == 0) {
        palette();
    } else {
        fprintf(stderr, "usage: cogmaster-test-host palette\n");
        return 2;
    }
    CHECK_DONE();
}
