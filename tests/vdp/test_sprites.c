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
    f.flags = COGMASTER_FRAME_FULL_FIELD;
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
    memset(m->vdp.cram, 0, sizeof m->vdp.cram);
    m->vdp.cram[34] = 15u;
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
static void bounds_state(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    solid(m, 2u, 1u);
    for (uint32_t i = 0u; i < 64u; i++)
        sprite(m, i, i < 56u ? 100u : 0xFFu, (uint8_t)(i - 56u), 2u);
    run(m);
    /* Last eight entries match line zero: the full 64-entry scan is bounded. */
    m->ticks += (262u - m->bus.line) * 684u - m->bus.t + 8u;
    cogmaster__bus_sync(m);
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, 0u);
    CHECK_EQ_U64(m->vdp.sprite_count, 8u);
    CHECK_EQ_U64(m->vdp.sprite_scanned, 64u);
    CHECK_EQ_U64(m->render_x, 4u);
    size_t n = cogmaster__state_size(m);
    uint8_t *bytes = malloc(n);
    CHECK(bytes != NULL);
    if (bytes != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(m, bytes, n), COGMASTER_STATUS_OK);
        /* Cached X/tile/row survive both a later SAT write and restoration. */
        m->vdp.vram[0x3FF0] = 200u;
        cogmaster__vdp_render_to(m, 1u);
        CHECK_EQ_HEX(m->field[0][8], 0x00Fu);
        CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(m->render_x, 4u);
        cogmaster__vdp_render_to(m, 1u);
        CHECK_EQ_HEX(m->field[0][8], 0x00Fu);
        CHECK_EQ_HEX(m->field[0][15], 0u);
        free(bytes);
    }
    /* A thousand writes at a fixed stamp cannot rescan the table. */
    for (uint32_t i = 0u; i < 1000u; i++)
        cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 0u);
    CHECK_EQ_U64(m->vdp.sprite_scanned, 64u);
    CHECK_EQ_U64(m->render_line, 1u);
    CHECK_EQ_U64(m->render_x, 0u);
    cogmaster_destroy(m);
}
static uint8_t status_at(cogmaster *m, uint32_t line, uint32_t tick)
{
    m->bus.line = line;
    m->bus.t = tick;
    m->bus.at = m->ticks;
    return cogmaster__bus_in(m, 0xBFu);
}
static void collision(void)
{
    for (uint32_t distance = 7u; distance <= 9u; distance++) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS2);
        solid(m, 2u, 1u);
        sprite(m, 0u, 0xFFu, 20u, 2u);
        sprite(m, 1u, 0xFFu, (uint8_t)(20u + distance), 2u);
        run(m);
        (void)status_at(m, 192u, 608u);
        /* Pixel 27 is the only shared dot for distance 7. */
        CHECK_EQ_HEX(status_at(m, 0u, 53u), 0x1Fu);
        CHECK_EQ_HEX(status_at(m, 0u, 54u), distance == 7u ? 0x3Fu : 0x1Fu);
        CHECK_EQ_HEX(status_at(m, 0u, 55u), 0x1Fu);
        CHECK_EQ_HEX(status_at(m, 0u, 56u), 0x1Fu);
        cogmaster_destroy(m);
    }
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    solid(m, 2u, 1u);
    solid(m, 3u, 2u);
    sprite(m, 0u, 0xFFu, 20u, 2u);
    sprite(m, 1u, 0xFFu, 20u, 3u);
    m->vdp.reg[1] |= 0x20u;
    run(m);
    CHECK_EQ_U64(m->z80.int_line, 1u);
    m->vdp.latch = 1u;
    CHECK_EQ_HEX(status_at(m, 192u, 608u), 0xBFu);
    CHECK_EQ_U64(m->vdp.latch, 0u);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    CHECK_EQ_HEX(status_at(m, 0u, 40u), 0x3Fu);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    CHECK_EQ_HEX(status_at(m, 0u, 41u), 0x1Fu);
    CHECK_EQ_HEX(status_at(m, 0u, 42u), 0x3Fu);
    CHECK_EQ_HEX(m->field[0][20], 0x00Fu);
    /* Priority conceals both sprites, but does not remove their collision. */
    m->vdp.vram[0x3805] = 0x10u;
    solid(m, 0u, 1u);
    CHECK_EQ_HEX(status_at(m, 0u, 44u), 0x3Fu);
    CHECK_EQ_HEX(m->field[0][22], 0xFFFu);
    cogmaster_destroy(m);
}
static void overflow(void)
{
    for (uint32_t count = 8u; count <= 9u; count++) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS2);
        /* Transparent and right-clipped still match Y and consume slots. */
        for (uint32_t i = 0u; i < count; i++)
            sprite(m, i, 0xFFu, 255u, 0u);
        run(m);
        (void)status_at(m, 192u, 608u);
        CHECK_EQ_HEX(m->vdp.status, 0u);
        CHECK_EQ_HEX(status_at(m, 0u, 0u), count == 9u ? 0x5Fu : 0x1Fu);
        CHECK_EQ_HEX(status_at(m, 0u, 1u), 0x1Fu);
        CHECK_EQ_HEX(status_at(m, 0u, 2u), 0x1Fu);
        CHECK_EQ_U64(m->vdp.sprite_count, 8u);
        CHECK_EQ_U64(m->vdp.sprite_scanned, 9u);
        CHECK_EQ_HEX(status_at(m, 1u, 0u), count == 9u ? 0x5Fu : 0x1Fu);
        cogmaster_destroy(m);
    }
}
static void inactive(void)
{
    for (uint32_t variant = 0u; variant < 5u; variant++) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS2);
        solid(m, 2u, 1u);
        for (uint32_t i = 0u; i < 9u; i++)
            sprite(m, i, 0xFFu, 0u, 2u);
        if (variant == 0u)
            m->vdp.reg[1] &= 0xBFu;
        if (variant == 1u)
            m->vdp.reg[0] |= 0x20u;
        if (variant == 2u) {
            m->vdp.reg[0] |= 8u;
            /* X=0 shifted fully left; no horizontal wrap/collision. */
        }
        if (variant == 3u)
            solid(m, 2u, 0u);
        run(m);
        (void)status_at(m, 192u, 608u);
        if (variant == 4u) {
            /* No active-dot work in either blanking line or HBlank. */
            CHECK_EQ_HEX(status_at(m, 200u, 20u), 0x1Fu);
            cogmaster__vdp_render_to(m, 1u);
            (void)status_at(m, 0u, 510u);
            CHECK_EQ_HEX(status_at(m, 0u, 512u), 0x1Fu);
        } else {
            CHECK_EQ_HEX(status_at(m, 0u, 16u), variant == 0u ? 0x1Fu : 0x5Fu);
            CHECK_EQ_HEX(status_at(m, 0u, 20u), 0x1Fu);
        }
        cogmaster_destroy(m);
    }
}
static void sms1_collision(void)
{
    for (uint32_t machine = 1u; machine <= 3u; machine += 2u) {
        cogmaster *m = make(machine);
        solid(m, 2u, 1u);
        m->vdp.reg[1] |= 1u;
        sprite(m, 0u, 0xFFu, 20u, 2u);
        sprite(m, 1u, 0xFFu, 35u, 2u);
        run(m);
        (void)status_at(m, 192u, 608u);
        CHECK_EQ_HEX(status_at(m, 0u, 70u), machine == 3u ? 0x1Fu : 0x3Fu);
        cogmaster_destroy(m);
    }
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
    else if (strcmp(argv[1], "bounds_state") == 0)
        bounds_state();
    else if (strcmp(argv[1], "collision") == 0)
        collision();
    else if (strcmp(argv[1], "overflow") == 0)
        overflow();
    else if (strcmp(argv[1], "inactive") == 0)
        inactive();
    else if (strcmp(argv[1], "sms1_collision") == 0)
        sms1_collision();
    else
        return 2;
    CHECK_DONE();
}
