/* Independent coordinate oracle: hand-expanded diamond/bar bitplanes,
   then the SMS's 2-bit channels expanded to RGB444. No renderer calls. */
#ifndef COGMASTER_TEST_PLAY_H
#define COGMASTER_TEST_PLAY_H
#include <stdint.h>
static uint16_t play_pixel(uint32_t x, uint32_t y, uint32_t scroll)
{
    static const uint8_t diamond[8][8] = {{1, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 1, 0},
                                          {0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0},
                                          {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0},
                                          {0, 1, 0, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 0, 0, 0, 1}};
    uint32_t col = (x + 256u - scroll) & 7u;
    uint32_t index =
        diamond[y & 7u][col] + (col & 1u) * 2u + ((col / 2u) & 1u) * 4u + (col / 4u) * 8u;
    index = index == 15u && scroll != 0u ? scroll & 63u : index;
    index = x < (scroll & 7u) ? 16u : index;
    return (uint16_t)(((index & 3u) * 5u) | (((index >> 2) & 3u) * 5u << 4) |
                      ((index >> 4) * 5u << 8));
}
#endif
