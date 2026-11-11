/* Host stereo helper, outside the core's determinism boundary: nothing here
   is hashed or saved. The libretro adapter uses it to show two eyes in one
   picture. */
#ifndef COGMASTER_HOST_EYES_H
#define COGMASTER_HOST_EYES_H

#include <stddef.h>
#include <stdint.h>

/* Red-cyan composition of two RGB444 pictures (0000BBBBGGGGRRRR). Each output
   dot takes its red channel (bits 0-3) from left and its green and blue
   channels (bits 4-11) from right; the top four bits are zero. out must not
   overlap either input and the inputs are not written. A NULL pointer with a
   non-zero count is a caller error and does nothing. */
void cogmaster__eyes_anaglyph(const uint16_t *left, const uint16_t *right, uint16_t *out,
                              size_t count);

#endif /* COGMASTER_HOST_EYES_H */
