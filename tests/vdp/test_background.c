/* Independent Mode 4 coordinates; test sources never call the pixel decoder. */
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
static cogmaster *make(void)
{
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = alloc;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    static const uint8_t image[1024] = {0xF3, 0x76};
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    m->vdp.reg[0] = 4u;
    m->vdp.reg[1] = 0x40u;
    m->vdp.reg[7] = 3u;
    m->vdp.cram[19] = 0x30u;
    m->vdp.cram[1] = 3u;
    m->vdp.cram[2] = 12u;
    return m;
}
static void name(cogmaster *m, uint32_t col, uint32_t row, uint32_t attr)
{
    uint32_t at = 0x3800u + row * 64u + col * 2u;
    m->vdp.vram[at] = (uint8_t)attr;
    m->vdp.vram[at + 1u] = (uint8_t)(attr >> 8);
}
static void solid(cogmaster *m, uint32_t tile, uint32_t colour)
{
    for (uint32_t y = 0u; y < 8u; y++) {
        for (uint32_t p = 0u; p < 4u; p++) {
            m->vdp.vram[tile * 32u + y * 4u + p] =
                (uint8_t)((colour & (1u << p)) != 0u ? 255u : 0u);
        }
    }
}
static void run(cogmaster *m)
{
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
}
static void reg(cogmaster *m, uint8_t r, uint8_t value)
{
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, value);
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, (uint8_t)(0x80u | r));
}
static void tiles(void)
{
    cogmaster *m = make();
    /* Four planar bytes encode 0..7, then 8..15; blue is independently
       present so a host-endian hash cannot hide its second byte. */
    static const uint16_t colours[16] = {0x000, 0x005, 0x00A, 0x00F, 0x050, 0x055, 0x05A, 0x05F,
                                         0x0A0, 0x0A5, 0x0AA, 0x0AF, 0x0F0, 0x0F5, 0x0FA, 0x0FF};
    for (uint32_t i = 0u; i < 16u; i++) {
        m->vdp.cram[i] = (uint8_t)i;
        m->vdp.cram[16u + i] = (uint8_t)(i | 0x30u);
    }
    m->vdp.vram[0] = 0x55u;
    m->vdp.vram[1] = 0x33u;
    m->vdp.vram[2] = 0x0Fu;
    m->vdp.vram[3] = 0u;
    m->vdp.vram[4] = 0x55u;
    m->vdp.vram[5] = 0x33u;
    m->vdp.vram[6] = 0x0Fu;
    m->vdp.vram[7] = 0xFFu;
    solid(m, 256u, 1u);
    name(m, 1u, 0u, 0x100u);
    name(m, 2u, 0u, 0x200u);
    name(m, 3u, 0u, 0x400u);
    name(m, 4u, 0u, 0x800u);
    run(m);
    for (uint32_t x = 0u; x < 8u; x++) {
        CHECK_EQ_HEX(m->field[0][x], colours[x]);
        CHECK_EQ_HEX(m->field[1][x], colours[x + 8u]);
        CHECK_EQ_HEX(m->field[0][16u + x], colours[7u - x]);
        CHECK_EQ_HEX(m->field[7][24u + x], colours[x]);
        CHECK_EQ_HEX(m->field[0][32u + x], colours[x] | 0xF00u);
    }
    CHECK_EQ_HEX(m->field[0][8], 0x005u);
    CHECK_EQ_HEX(m->field[0][24], 0u);
    cogmaster_destroy(m);
}
static void scroll(void)
{
    cogmaster *m = make();
    solid(m, 1u, 1u);
    solid(m, 2u, 2u);
    name(m, 31u, 0u, 1u);
    name(m, 0u, 27u, 2u);
    m->vdp.reg[8] = 9u;
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0xF00u);
    CHECK_EQ_HEX(m->field[0][1], 0x00Fu);
    CHECK_EQ_HEX(m->field[0][8], 0x00Fu);
    CHECK_EQ_HEX(m->field[0][9], 0u);
    m->vdp.reg[8] = 0u;
    m->vdp.reg[9] = 223u;
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0x0F0u);
    CHECK_EQ_HEX(m->field[1][0], 0u);
    m->vdp.reg[9] = 255u;
    name(m, 0u, 3u, 1u);
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0x00Fu);
    cogmaster_destroy(m);
}
static void locks(void)
{
    cogmaster *m = make();
    solid(m, 1u, 1u);
    solid(m, 2u, 2u);
    name(m, 0u, 0u, 1u);
    name(m, 31u, 0u, 2u);
    name(m, 0u, 2u, 1u);
    name(m, 31u, 2u, 2u);
    m->vdp.reg[0] = 0x44u;
    m->vdp.reg[8] = 8u;
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0x00Fu);
    CHECK_EQ_HEX(m->field[15][0], 0u);
    CHECK_EQ_HEX(m->field[16][0], 0x0F0u);
    m->vdp.reg[0] = 0x84u;
    m->vdp.reg[8] = 0u;
    m->vdp.reg[9] = 8u;
    name(m, 23u, 0u, 1u);
    name(m, 24u, 0u, 1u);
    name(m, 23u, 1u, 2u);
    name(m, 24u, 1u, 2u);
    run(m);
    CHECK_EQ_HEX(m->field[0][191], 0x0F0u);
    CHECK_EQ_HEX(m->field[0][192], 0x00Fu);
    m->vdp.reg[0] = 0x24u;
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0xF00u);
    CHECK_EQ_HEX(m->field[0][7], 0xF00u);
    CHECK_EQ_HEX(m->field[0][8], 0u);
    cogmaster_destroy(m);
}
static void intervals(void)
{
    cogmaster *m = make();
    solid(m, 0u, 1u);
    run(m);
    /* Writes at dot 4 and dot 8 retain precisely the earlier intervals. */
    m->bus.line = 0u;
    m->bus.t = 8u;
    m->vdp.code = 3u;
    m->vdp.addr = 1u;
    cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 12u);
    m->bus.t = 16u;
    m->vdp.code = 1u;
    m->vdp.addr = 1u;
    cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 0xFFu);
    cogmaster__vdp_render_to(m, 1u);
    CHECK_EQ_HEX(m->field[0][3], 0x00Fu);
    CHECK_EQ_HEX(m->field[0][4], 0x0F0u);
    CHECK_EQ_HEX(m->field[0][7], 0x0F0u);
    CHECK_EQ_HEX(m->field[0][8], 0u);
    /* Display-off and backdrop changes also preserve emitted dots. */
    m->bus.line = 1u;
    m->bus.t = 600u;
    reg(m, 1u, 0u);
    cogmaster__vdp_event(m, 1u, 600u);
    m->bus.line = 2u;
    m->bus.t = 8u;
    reg(m, 7u, 1u);
    cogmaster__vdp_render_to(m, 3u);
    CHECK_EQ_HEX(m->field[2][3], 0xF00u);
    CHECK_EQ_HEX(m->field[2][4], 0u);
    cogmaster_destroy(m);
}
static void latches(void)
{
    cogmaster *m = make();
    solid(m, 1u, 1u);
    solid(m, 2u, 2u);
    name(m, 0u, 0u, 1u);
    name(m, 31u, 0u, 2u);
    name(m, 0u, 1u, 2u);
    run(m);
    m->bus.line = 0u;
    m->bus.t = 599u;
    reg(m, 8u, 8u);
    reg(m, 9u, 8u);
    cogmaster__vdp_event(m, 0u, 600u);
    m->bus.line = 1u;
    m->bus.t = 601u;
    reg(m, 8u, 0u);
    cogmaster__vdp_render_to(m, 3u);
    CHECK_EQ_HEX(m->field[0][0], 0x00Fu);
    CHECK_EQ_HEX(m->field[1][0], 0x0F0u);
    CHECK_EQ_HEX(m->field[2][0], 0x0F0u);
    cogmaster__vdp_finish_field(m);
    cogmaster__vdp_event(m, 261u, 600u);
    cogmaster__vdp_render_to(m, 1u);
    CHECK_EQ_HEX(m->field[0][0], 0x0F0u);
    cogmaster_destroy(m);
}
static void empty(void)
{
    cogmaster *m = make();
    /* No SAT entries or names; enabled background colour zero is CRAM[0]. */
    m->vdp.cram[0] = 0x15u;
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0x555u);
    CHECK_EQ_HEX(m->field[191][255], 0x555u);
    m->vdp.reg[1] = 0u;
    run(m);
    CHECK_EQ_HEX(m->field[0][0], 0xF00u);
    CHECK_EQ_HEX(m->field[191][255], 0xF00u);
    cogmaster_destroy(m);
}
int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "tiles") == 0)
        tiles();
    else if (strcmp(argv[1], "scroll") == 0)
        scroll();
    else if (strcmp(argv[1], "locks") == 0)
        locks();
    else if (strcmp(argv[1], "intervals") == 0)
        intervals();
    else if (strcmp(argv[1], "latches") == 0)
        latches();
    else if (strcmp(argv[1], "empty") == 0)
        empty();
    else
        return 2;
    CHECK_DONE();
}
