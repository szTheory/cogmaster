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
static cogmaster *make_profile(uint32_t machine)
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
    media.machine = machine;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    m->vdp.reg[0] = 4u;
    m->vdp.reg[1] = 0x40u;
    m->vdp.reg[7] = 3u;
    m->vdp.cram[19] = 0x30u;
    m->vdp.cram[1] = 3u;
    m->vdp.cram[2] = 12u;
    m->vdp.reg[3] = 0xFFu;
    m->vdp.reg[4] = 7u;
    m->vdp.reg[5] = 0x7Fu;
    return m;
}
static cogmaster *make(void)
{
    return make_profile(COGMASTER_MACHINE_SMS2);
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
    /* Background-only cases terminate the SAT, separate from tile zero. */
    m->vdp.vram[0x3F00] = 0xD0u;
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
    name(m, 5u, 0u, 0xE00u);
    name(m, 6u, 0u, 0xE000u);
    run(m);
    for (uint32_t x = 0u; x < 8u; x++) {
        CHECK_EQ_HEX(m->field[0][x], colours[x]);
        CHECK_EQ_HEX(m->field[1][x], colours[x + 8u]);
        CHECK_EQ_HEX(m->field[0][16u + x], colours[7u - x]);
        CHECK_EQ_HEX(m->field[7][24u + x], colours[x]);
        CHECK_EQ_HEX(m->field[0][32u + x], colours[x] | 0xF00u);
        CHECK_EQ_HEX(m->field[7][40u + x], colours[7u - x] | 0xF00u);
        CHECK_EQ_HEX(m->field[0][48u + x], colours[x]);
    }
    CHECK_EQ_HEX(m->field[0][8], 0x005u);
    CHECK_EQ_HEX(m->field[0][24], 0u);
    solid(m, 0u, 1u);
    static const uint16_t levels[] = {0u, 5u, 10u, 15u};
    for (uint32_t c = 0u; c < 64u; c++) {
        m->vdp.cram[1] = (uint8_t)c;
        run(m);
        CHECK_EQ_HEX(m->field[0][0],
                     levels[c % 4u] + 16u * levels[(c / 4u) % 4u] + 256u * levels[c / 16u]);
    }
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
    m->vdp.reg[8] = 8u;
    run(m);
    CHECK_EQ_HEX(m->field[0][199], 0x0F0u);
    CHECK_EQ_HEX(m->field[0][200], 0x00Fu);
    m->vdp.reg[8] = 0u;
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
    cogmaster__vdp_event(m, 1u, 512u);
    cogmaster__vdp_event(m, 1u, 600u);
    m->bus.line = 1u;
    m->bus.t = 601u;
    reg(m, 8u, 0u);
    cogmaster__vdp_render_to(m, 3u);
    CHECK_EQ_HEX(m->field[0][0], 0x00Fu);
    CHECK_EQ_HEX(m->field[1][0], 0x0F0u);
    CHECK_EQ_HEX(m->field[2][0], 0x0F0u);
    cogmaster__vdp_event(m, 2u, 600u);
    cogmaster__vdp_render_to(m, 4u);
    CHECK_EQ_HEX(m->field[3][0], 0x00Fu);
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
static void priority(void)
{
    cogmaster *m = make();
    solid(m, 1u, 1u);
    name(m, 0u, 0u, 0x1000u);
    name(m, 1u, 0u, 0x1800u);
    name(m, 2u, 0u, 0x1001u);
    name(m, 3u, 0u, 0x0801u);
    run(m);
    CHECK_EQ_U64(cogmaster__vdp_background(m, 0u, 0u), 0u);
    CHECK_EQ_U64(cogmaster__vdp_background(m, 8u, 0u), 16u);
    CHECK_EQ_U64(cogmaster__vdp_background(m, 16u, 0u), 33u);
    CHECK_EQ_U64(cogmaster__vdp_background(m, 24u, 0u), 17u);
    cogmaster_destroy(m);
}
static void state_interval(void)
{
    cogmaster *m = make();
    solid(m, 0u, 1u);
    run(m);
    /* Place a real bus stamp in active line zero. */
    m->ticks += (262u - m->bus.line) * 684u - m->bus.t + 8u;
    cogmaster__bus_sync(m);
    m->vdp.code = 3u;
    m->vdp.addr = 1u;
    cogmaster__vdp_write(m, COGMASTER__VDP_DATA, 12u);
    size_t n = cogmaster__state_size(m);
    uint8_t *bytes = malloc(n);
    CHECK(bytes != NULL);
    if (bytes != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(m, bytes, n), COGMASTER_STATUS_OK);
        cogmaster__vdp_render_to(m, 1u);
        CHECK_EQ_HEX(m->field[0][3], 0x00Fu);
        CHECK_EQ_HEX(m->field[0][4], 0x0F0u);
        m->vdp.cram[1] = 0u;
        CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(m->render_line, 0u);
        CHECK_EQ_U64(m->render_x, 4u);
        cogmaster__vdp_render_to(m, 1u);
        CHECK_EQ_HEX(m->field[0][3], 0x00Fu);
        CHECK_EQ_HEX(m->field[0][4], 0x0F0u);
        free(bytes);
    }
    cogmaster_destroy(m);
}
static void sms1_masks(void)
{
    for (uint32_t machine = 1u; machine <= 3u; machine += 2u) {
        cogmaster *m = make_profile(machine);
        solid(m, 1u, 1u);
        solid(m, 2u, 2u);
        name(m, 0u, 0u, 1u);
        name(m, 0u, 16u, 2u);
        run(m);
        CHECK_EQ_HEX(m->field[128][0], machine == 3u ? 0x00Fu : 0x0F0u);
        m->vdp.reg[2] |= 1u;
        run(m);
        CHECK_EQ_HEX(m->field[128][0], 0x0F0u);
        /* One independently named pair per R3 address gate, both planes. */
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            for (uint32_t plane = 0u; plane < 2u; plane++) {
                memset(m->vdp.vram, 0, sizeof m->vdp.vram);
                m->vdp.vram[plane] = 0xFFu;
                name(m, 0u, 0u, 2u << bit);
                m->vdp.reg[3] = (uint8_t)(255u ^ (1u << bit));
                run(m);
                CHECK_EQ_HEX(m->field[0][0], machine == 3u ? (plane ? 0x0F0u : 0x00Fu) : 0u);
                m->vdp.reg[3] = 255u;
                run(m);
                CHECK_EQ_HEX(m->field[0][0], 0u);
            }
        }
        m->vdp.cram[4] = 0x30u;
        m->vdp.cram[8] = 0x3Fu;
        for (uint32_t bit = 0u; bit < 3u; bit++) {
            for (uint32_t plane = 2u; plane < 4u; plane++) {
                memset(m->vdp.vram, 0, sizeof m->vdp.vram);
                m->vdp.vram[plane] = 0xFFu;
                name(m, 0u, 0u, 64u << bit);
                m->vdp.reg[4] = (uint8_t)(7u ^ (1u << bit));
                run(m);
                CHECK_EQ_HEX(m->field[0][0], machine == 3u ? (plane == 2u ? 0xF00u : 0xFFFu) : 0u);
                m->vdp.reg[4] = 7u;
                run(m);
                CHECK_EQ_HEX(m->field[0][0], 0u);
            }
        }
        m->vdp.reg[5] = 0x7Eu;
        CHECK_EQ_HEX(cogmaster__vdp_sat_address(m, 0x86u), machine == 3u ? 0x3F06u : 0x3F86u);
        m->vdp.reg[5] = 0x7Fu;
        CHECK_EQ_HEX(cogmaster__vdp_sat_address(m, 0x86u), 0x3F86u);
        static const uint32_t expected[] = {0x200Cu, 0x280Cu, 0x300Cu, 0x380Cu};
        for (uint32_t bits = 0u; bits < 4u; bits++) {
            m->vdp.reg[6] = (uint8_t)(4u | bits);
            CHECK_EQ_HEX(cogmaster__vdp_sprite_address(m, 192u, 3u),
                         machine == 3u ? expected[bits] : 0x380Cu);
        }
        cogmaster_destroy(m);
    }
}
static void sms1_state(void)
{
    cogmaster *m = make_profile(COGMASTER_MACHINE_SMS1);
    run(m);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC000u), 0xABu);
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xABu);
    m->bus.data = 0x45u;
    size_t n = cogmaster__state_size(m);
    uint8_t *bytes = malloc(n);
    CHECK(bytes != NULL);
    if (bytes != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(m, bytes, n), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_OK);
        CHECK_EQ_HEX(m->bus.data, 0x45u);
        cogmaster *other = make();
        CHECK_EQ_U64(cogmaster__state_load(other, bytes, n), COGMASTER_STATUS_ERR_ARGUMENT);
        cogmaster_destroy(other);
        CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
        CHECK_EQ_HEX(m->bus.data, 0xFFu);
        free(bytes);
    }
    cogmaster_destroy(m);
}
static void unsupported(void)
{
    for (uint32_t machine = 1u; machine <= 3u; machine += 2u) {
        for (uint32_t mode = 0u; mode < 16u; mode++) {
            int valid = machine == 3u ? (mode == 8u || mode == 10u || mode == 12u)
                                      : (mode >= 8u && mode != 9u && mode != 13u);
            if (valid)
                continue;
            cogmaster *m = make_profile(machine);
            m->vdp.reg[0] = (uint8_t)(((mode & 8u) >> 1) | (mode & 2u));
            m->vdp.reg[1] = (uint8_t)(0x40u | ((mode & 4u) << 1) | ((mode & 1u) << 4));
            cogmaster_run_result r = {0};
            r.size = sizeof r;
            CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
            CHECK(r.overshoot_ticks < 69u);
            CHECK_EQ_HEX(m->field[0][0], 0xF00u);
            cogmaster_fault f = {0};
            f.size = sizeof f;
            CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(f.present, 1u);
            CHECK_EQ_U64(f.device, COGMASTER_FAULT_DEVICE_VDP);
            CHECK_EQ_U64(f.kind, COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE);
            cogmaster_destroy(m);
        }
    }
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
    else if (strcmp(argv[1], "priority") == 0)
        priority();
    else if (strcmp(argv[1], "state_interval") == 0)
        state_interval();
    else if (strcmp(argv[1], "sms1_masks") == 0)
        sms1_masks();
    else if (strcmp(argv[1], "sms1_state") == 0)
        sms1_state();
    else if (strcmp(argv[1], "unsupported") == 0)
        unsupported();
    else
        return 2;
    CHECK_DONE();
}
