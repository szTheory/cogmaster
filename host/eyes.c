/* Red-cyan anaglyph composition. A core dot is 0000BBBBGGGGRRRR
   (.planning/preparation/ARCHITECTURE.md section 5), so red is the low four
   bits and green and blue are the next eight. Red glass passes the left
   eye's red; cyan glass passes the right eye's green and blue. */
#include "eyes.h"

void cogmaster__eyes_anaglyph(const uint16_t *left, const uint16_t *right, uint16_t *out,
                              size_t count)
{
    if (count == 0u || left == NULL || right == NULL || out == NULL) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        out[i] = (uint16_t)((left[i] & 0x000Fu) | (right[i] & 0x0FF0u));
    }
}
