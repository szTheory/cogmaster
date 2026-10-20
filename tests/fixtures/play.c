/* The owned image through the public API, judged at every coordinate. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "sha256.h"
#include "../check.h"
#include "play.h"

static void *alloc(void *u, size_t n) { (void)u; return malloc(n); }
static void release(void *u, void *p, size_t n) { (void)u; (void)n; free(p); }
int main(int argc, char **argv)
{
    if (argc != 2) { return 2; }
    uint8_t image[32768];
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) { return 1; }
    size_t n = fread(image, 1u, sizeof image, f);
    int extra = fgetc(f);
    CHECK(fclose(f) == 0);
    CHECK(n == sizeof image && extra == EOF);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg; cfg.allocator.alloc = alloc; cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media; media.image = image; media.image_size = n;
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (uint32_t i = 0; i < 4u; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
    uint16_t pixels[192][256];
    cogmaster_frame frame = {0};
    frame.size = sizeof frame; frame.pixels = &pixels[0][0]; frame.pitch_bytes = 512u;
    CHECK_EQ_U64(cogmaster_get_frame(m, &frame), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(frame.w, 256u); CHECK_EQ_U64(frame.h, 192u);
    cogmaster__sha256 hash;
    cogmaster__sha256_init(&hash);
    for (uint32_t y = 0; y < 192u; y++) {
        for (uint32_t x = 0; x < 256u; x++) {
            uint16_t want = play_pixel(x, y, 0u);
            if (pixels[y][x] != want) {
                CHECK_EQ_HEX(pixels[y][x], want);
                cogmaster_destroy(m); return 1;
            }
            uint8_t bytes[2] = {(uint8_t)want, (uint8_t)(want >> 8)};
            cogmaster__sha256_update(&hash, bytes, sizeof bytes);
        }
    }
    uint8_t digest[32];
    cogmaster__sha256_final(&hash, digest);
    for (uint32_t i = 0; i < 32u; i++) { printf("%02x", digest[i]); }
    puts("");
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC112u), 1u);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC113u), 15u);
    cogmaster_destroy(m);
    CHECK_DONE();
}
