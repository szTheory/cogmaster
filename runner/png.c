#include "png.h"

#include "crc32.h"

uint32_t cogmaster__adler32(uint32_t adler, const uint8_t *p, size_t n)
{
    (void)p;
    (void)n;
    return adler;
}

size_t cogmaster__png_size(uint32_t width, uint32_t height)
{
    (void)width;
    (void)height;
    return 0;
}

size_t cogmaster__png_write(uint8_t *out, size_t cap, const uint8_t *rgb, uint32_t width,
                            uint32_t height)
{
    (void)out;
    (void)cap;
    (void)rgb;
    (void)width;
    (void)height;
    return 0;
}
