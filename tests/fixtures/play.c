/* The owned image through the public API, judged at every coordinate. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "sha256.h"
#include "../check.h"
#include "play.h"

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
int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) {
        return 2;
    }
    int scripted = argc == 3;
    int both = scripted && strcmp(argv[2], "both") == 0;
    int pad2 = scripted && strcmp(argv[2], "pad2") == 0;
    static uint8_t image[32768];
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        return 1;
    }
    size_t n = fread(image, 1u, sizeof image, f);
    int extra = fgetc(f);
    CHECK(fclose(f) == 0);
    CHECK(n == sizeof image && extra == EOF);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = alloc;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = n;
    int gg = (image[32767] >> 4) == 6u;
    media.machine = gg ? COGMASTER_MACHINE_GG : COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (uint32_t i = 0; i < 14u; i++) {
        cogmaster_input in = {0};
        in.size = sizeof in;
        if (scripted && (i == 4u || i == 5u || i == 7u)) {
            if (gg)
                in.start = 1u;
            else if (pad2)
                in.p2 = 16u;
            else
                in.p1 = 16u;
        }
        if (both && (i == 5u || i == 6u || i == 9u))
            in.p2 = 16u;
        CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
        if (i >= 3u) {
            uint32_t count = scripted ? (i >= (both ? 9u : 7u) ? 2u : (i >= 4u ? 1u : 0u)) : 0u;
            CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC200u), count);
        }
    }
    static uint16_t pixels[192][256];
    cogmaster_frame frame = {0};
    frame.size = sizeof frame;
    frame.pixels = &pixels[0][0];
    frame.pitch_bytes = 512u;
    CHECK_EQ_U64(cogmaster_get_frame(m, &frame), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(frame.w, gg ? 160u : 256u);
    CHECK_EQ_U64(frame.h, gg ? 144u : 192u);
    CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC105u), 0u);
    CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC106u), gg ? 16u : 8u);
    static const uint8_t counts[8] = {0u, 1u, 1u, 1u, 2u, 2u, 2u, 2u};
    for (uint32_t i = 0u; i < 8u; i++) {
        uint32_t value = scripted ? counts[i] : 0u;
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC110u + (gg ? 8u : 4u) * i), i + 1u);
        static const uint8_t counts2[8] = {0u, 0u, 1u, 1u, 1u, 1u, 2u, 2u};
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC112u + (gg ? 8u : 4u) * i),
                     gg || pad2 ? value
                     : both     ? counts2[i]
                                : 0u);
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC113u + (gg ? 8u : 4u) * i),
                     !gg && pad2 ? 0u : value);
        if (gg) {
            CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC114u + 8u * i), i + 17u);
            CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC116u + 8u * i),
                         scripted && (i == 1u || i == 2u || i == 4u) ? 0x40u : 0xC0u);
        }
    }
    cogmaster__sha256 hash;
    cogmaster__sha256_init(&hash);
    for (uint32_t y = 0; y < frame.h; y++) {
        for (uint32_t x = 0; x < frame.w; x++) {
            uint16_t want = play_pixel(x + frame.x, y + frame.y, scripted ? 2u : 0u);
            if (pixels[y][x] != want) {
                CHECK_EQ_HEX(pixels[y][x], want);
                cogmaster_destroy(m);
                return 1;
            }
            uint8_t bytes[2] = {(uint8_t)want, (uint8_t)(want >> 8)};
            cogmaster__sha256_update(&hash, bytes, sizeof bytes);
        }
    }
    uint8_t digest[32];
    cogmaster__sha256_final(&hash, digest);
    for (uint32_t i = 0; i < 32u; i++) {
        printf("%02x", digest[i]);
    }
    puts("");
    cogmaster_destroy(m);
    CHECK_DONE();
}
