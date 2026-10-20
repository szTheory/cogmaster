/* Hand-derived sprite coordinates, observed through the live frame API. */
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "state.h"
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
static cogmaster *make(uint32_t machine)
{
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = alloc;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    static const uint8_t image[1024] = {0xF3u, 0x76u};
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    m->vdp.reg[0] = 4u;
    m->vdp.reg[1] = 0x40u;
    m->vdp.reg[3] = 255u;
    m->vdp.reg[4] = 7u;
    m->vdp.reg[5] = 0x7Fu;
    m->vdp.reg[6] = 3u;
    m->vdp.vram[0x3F00] = 0xD0u;
    m->vdp.cram[1] = 0x3Fu;
    m->vdp.cram[17] = 3u;
    m->vdp.cram[18] = 12u;
    m->vdp.cram[19] = 48u;
    return m;
}
static void sprite(cogmaster *m, uint32_t i, uint8_t y, uint8_t x, uint8_t tile)
{
    m->vdp.vram[0x3F00u + i] = y;
    m->vdp.vram[0x3F80u + 2u * i] = x;
    m->vdp.vram[0x3F81u + 2u * i] = tile;
    if (i < 63u)
        m->vdp.vram[0x3F01u + i] = 0xD0u;
}
static void solid(cogmaster *m, uint32_t tile, uint32_t colour)
{
    for (uint32_t row = 0u; row < 8u; row++) {
        for (uint32_t p = 0u; p < 4u; p++) {
            m->vdp.vram[tile * 32u + row * 4u + p] =
                (uint8_t)((colour & (1u << p)) != 0u ? 255u : 0u);
        }
    }
}
static void run(cogmaster *m)
{
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
}
static const uint16_t *frame(cogmaster *m)
{
    static uint16_t pixels[192][256];
    cogmaster_frame f = {0};
    f.size = sizeof f;
    f.pixels = &pixels[0][0];
    f.pitch_bytes = 512u;
    CHECK_EQ_U64(cogmaster_get_frame(m, &f), COGMASTER_STATUS_OK);
    return &pixels[0][0];
}
static void shapes(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    /* Independent bitplane rows give red, green, blue, transparent. */
    m->vdp.vram[64] = 0x80u;
    m->vdp.vram[65] = 0x40u;
    m->vdp.vram[68] = 0x20u;
    m->vdp.vram[69] = 0x20u;
    sprite(m, 0u, 9u, 20u, 2u);
    run(m);
    const uint16_t *p = frame(m);
    CHECK_EQ_HEX(p[10u * 256u + 20u], 0x00Fu);
    CHECK_EQ_HEX(p[10u * 256u + 21u], 0x0F0u);
    CHECK_EQ_HEX(p[10u * 256u + 22u], 0u);
    CHECK_EQ_HEX(p[11u * 256u + 22u], 0xF00u);
    CHECK_EQ_HEX(p[9u * 256u + 20u], 0u);
    solid(m, 2u, 1u);
    solid(m, 3u, 2u);
    sprite(m, 0u, 9u, 20u, 3u);
    m->vdp.reg[1] |= 2u;
    run(m);
    p = frame(m);
    CHECK_EQ_HEX(p[10u * 256u + 20u], 0x00Fu);
    CHECK_EQ_HEX(p[17u * 256u + 27u], 0x00Fu);
    CHECK_EQ_HEX(p[18u * 256u + 20u], 0x0F0u);
    CHECK_EQ_HEX(p[25u * 256u + 27u], 0x0F0u);
    CHECK_EQ_HEX(p[26u * 256u + 20u], 0u);
    cogmaster_destroy(m);
}
static void wrap_clip(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    solid(m, 2u, 1u);
    sprite(m, 0u, 0xFCu, 2u, 2u);
    sprite(m, 1u, 190u, 255u, 2u);
    /* D0 ends evaluation even with later populated entries. */
    sprite(m, 3u, 0xFFu, 40u, 2u);
    run(m);
    const uint16_t *p = frame(m);
    CHECK_EQ_HEX(p[2], 0x00Fu);
    CHECK_EQ_HEX(p[4u * 256u + 2u], 0x00Fu);
    CHECK_EQ_HEX(p[5u * 256u + 2u], 0u);
    CHECK_EQ_HEX(p[40], 0u);
    CHECK_EQ_HEX(p[191u * 256u + 255u], 0x00Fu);
    CHECK_EQ_HEX(p[191u * 256u], 0u);
    m->vdp.reg[0] |= 8u;
    run(m);
    p = frame(m);
    CHECK_EQ_HEX(p[0], 0x00Fu);
    CHECK_EQ_HEX(p[1], 0x00Fu);
    CHECK_EQ_HEX(p[2], 0u);
    CHECK_EQ_HEX(p[191u * 256u + 247u], 0x00Fu);
    CHECK_EQ_HEX(p[191u * 256u + 254u], 0x00Fu);
    CHECK_EQ_HEX(p[191u * 256u + 255u], 0u);
    cogmaster_destroy(m);
}
static void zoom(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    m->vdp.reg[1] |= 3u;
    m->vdp.vram[64] = 0x80u;
    m->vdp.vram[96 + 29] = 1u;
    sprite(m, 0u, 9u, 20u, 3u);
    run(m);
    const uint16_t *p = frame(m);
    CHECK_EQ_HEX(p[10u * 256u + 20u], 0x00Fu);
    CHECK_EQ_HEX(p[11u * 256u + 21u], 0x00Fu);
    CHECK_EQ_HEX(p[12u * 256u + 20u], 0u);
    CHECK_EQ_HEX(p[40u * 256u + 34u], 0x0F0u);
    CHECK_EQ_HEX(p[41u * 256u + 35u], 0x0F0u);
    CHECK_EQ_HEX(p[42u * 256u + 35u], 0u);
    cogmaster_destroy(m);
}
static void sms1_zoom(void)
{
    for (uint32_t count = 2u; count <= 8u; count += 2u) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS1);
        m->vdp.reg[1] |= 1u;
        solid(m, 2u, 1u);
        for (uint32_t i = 0u; i < count; i++)
            sprite(m, i, 19u, (uint8_t)(i * 24u), 2u);
        run(m);
        const uint16_t *p = frame(m);
        /* Measured table: VV, VVVV, FFVVVV, FFFFVVVV. */
        for (uint32_t i = 0u; i < count; i++) {
            uint32_t x = i * 24u;
            CHECK_EQ_HEX(p[20u * 256u + x + 7u], 0x00Fu);
            CHECK_EQ_HEX(p[20u * 256u + x + 8u], i + 4u < count ? 0x00Fu : 0u);
            CHECK_EQ_HEX(p[35u * 256u + x], 0x00Fu);
            CHECK_EQ_HEX(p[36u * 256u + x], 0u);
        }
        cogmaster_destroy(m);
    }
}
static void priority(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    solid(m, 2u, 1u);
    solid(m, 3u, 2u);
    sprite(m, 0u, 0xFFu, 0u, 2u);
    sprite(m, 1u, 0xFFu, 0u, 3u);
    run(m);
    CHECK_EQ_HEX(frame(m)[0], 0x00Fu);
    /* Transparent early sprite reveals the later sprite. */
    m->vdp.vram[64] = 0x7Fu;
    run(m);
    CHECK_EQ_HEX(frame(m)[0], 0x0F0u);
    CHECK_EQ_HEX(frame(m)[1], 0x00Fu);
    m->vdp.vram[0x3801] = 0x10u;
    run(m);
    CHECK_EQ_HEX(frame(m)[1], 0x00Fu);
    solid(m, 0u, 1u);
    run(m);
    CHECK_EQ_HEX(frame(m)[1], 0xFFFu);
    m->vdp.reg[0] |= 0x20u;
    run(m);
    CHECK_EQ_HEX(frame(m)[1], 0u);
    cogmaster_destroy(m);
}
static void gg_slots(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_GG);
    solid(m, 2u, 1u);
    for (uint32_t i = 0u; i < 8u; i++)
        sprite(m, i, 24u, 0u, 0u);
    sprite(m, 8u, 24u, 60u, 2u);
    run(m);
    CHECK_EQ_HEX(frame(m)[25u * 256u + 60u], 0u);
    sprite(m, 7u, 24u, 60u, 2u);
    run(m);
    CHECK_EQ_HEX(frame(m)[25u * 256u + 60u], 0x00Fu);
    cogmaster_destroy(m);
}
static void intervals(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    solid(m, 2u, 1u);
    sprite(m, 0u, 0xFFu, 0u, 2u);
    run(m);
    /* Catch up before a pattern mutation, then hammer an already emitted dot. */
    m->bus.line = 0u;
    m->bus.t = 8u;
    m->vdp.code = 1u;
    m->vdp.addr = 64u;
    cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 0u);
    for (uint32_t i = 0u; i < 1000u; i++) {
        m->vdp.addr = 0x3F80u;
        cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 100u);
    }
    cogmaster__vdp_render_to(m, 1u);
    CHECK_EQ_HEX(m->field[0][3], 0x00Fu);
    CHECK_EQ_HEX(m->field[0][4], 0u);
    CHECK_EQ_HEX(m->field[0][100], 0u);
    cogmaster_destroy(m);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "shapes") == 0)
        shapes();
    else if (strcmp(argv[1], "wrap_clip") == 0)
        wrap_clip();
    else if (strcmp(argv[1], "zoom") == 0)
        zoom();
    else if (strcmp(argv[1], "sms1_zoom") == 0)
        sms1_zoom();
    else if (strcmp(argv[1], "priority") == 0)
        priority();
    else if (strcmp(argv[1], "gg_slots") == 0)
        gg_slots();
    else if (strcmp(argv[1], "intervals") == 0)
        intervals();
    else
        return 2;
    CHECK_DONE();
}
