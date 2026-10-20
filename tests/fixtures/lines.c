/* Guest-rendered fields checked through the public API, without VDP helpers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "sha256.h"
#include "../check.h"

static void *alloc(void *u, size_t n)
{
    (void)u;
    return malloc(n);
}
static void release(void *u, void *p, size_t n)
{
    (void)u;
    (void)n;
    free(p);
}
/* Literal diagonal pixels and independently laid out feature coordinates.
   IRQs on 31/63/... write after the following line's 600 latch, so the
   new bands start at 33/65/...; the first 16 rows lock horizontal scroll. */
static uint16_t pixel(uint32_t x, uint32_t y, uint32_t height)
{
    if (x < 8u)
        return 0xF00u;
    uint32_t band = y < 33u ? 0u : (y - 1u) / 32u;
    uint32_t sx = y < 16u ? x : (x + 256u - 8u * (band + 1u)) % 256u;
    uint32_t sy = sx >= 192u ? y : (y + 8u) % (height == 192u ? 224u : 256u);
    uint32_t col = sx / 8u, row = sy / 8u;
    uint32_t tx = sx % 8u, ty = sy % 8u;
    if (row == 1u && col == 2u)
        tx = 7u - tx;
    if (row == 1u && col == 3u)
        ty = 7u - ty;
    uint32_t opaque = tx == ty;
    uint16_t bg = opaque ? (row == 1u && col == 4u ? 0xF00u : 0x00Fu) : 0u;
    uint16_t sprite = 0u;
    if (y < 32u && x >= 40u && x < 64u)
        sprite = y < 16u ? 0x0F0u : 0xFFFu;
    if (y >= 96u && y < 104u && x >= 72u && x < 80u)
        sprite = 0x0F0u;
    return sprite != 0u && !(opaque && row == 1u && col == 5u) ? sprite : bg;
}
int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    static uint8_t image[32768];
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL)
        return 1;
    CHECK_EQ_U64(fread(image, 1u, sizeof image, file), sizeof image);
    CHECK(fclose(file) == 0);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = alloc;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = COGMASTER_MACHINE_SMS2;
    media.tv = strcmp(argv[3], "pal") == 0 ? COGMASTER_TV_PAL : COGMASTER_TV_NTSC;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (uint32_t i = 0u; i < 30u; i++)
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    cogmaster_info info = {0};
    info.size = sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    uint32_t height = (uint32_t)strtoul(argv[2], NULL, 10);
    CHECK_EQ_U64(info.field_h, height);
    static uint16_t pixels[240][256];
    cogmaster_frame frame = {0};
    frame.size = sizeof frame;
    frame.pixels = &pixels[0][0];
    frame.pitch_bytes = 512u;
    CHECK_EQ_U64(cogmaster_get_frame(m, &frame), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(frame.h, height);
    cogmaster__sha256 hash;
    cogmaster__sha256_init(&hash);
    for (uint32_t y = 0u; y < height; y++) {
        for (uint32_t x = 0u; x < 256u; x++) {
            uint16_t want = pixel(x, y, height);
            if (pixels[y][x] != want) {
                fprintf(stderr, "pixel %u,%u: %03x expected %03x\n", x, y, pixels[y][x], want);
                cogmaster_destroy(m);
                return 1;
            }
            uint8_t bytes[2] = {(uint8_t)want, (uint8_t)(want >> 8)};
            cogmaster__sha256_update(&hash, bytes, sizeof bytes);
        }
    }
    uint8_t digest[32];
    cogmaster__sha256_final(&hash, digest);
    for (uint32_t i = 0u; i < 32u; i++)
        printf("%02x", digest[i]);
    puts("");
    cogmaster_destroy(m);
    CHECK_DONE();
}
