/* Completed images are independent of the live mode and next raster. */
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "state.h"
#include "../check.h"

static uint32_t allocations;
static void *alloc(void *u, size_t n)
{
    (void)u;
    allocations++;
    return malloc(n);
}
static void release(void *u, void *p, size_t n)
{
    (void)u;
    (void)n;
    free(p);
}
static cogmaster *make(uint32_t machine, uint32_t tv)
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
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}
static void reg(cogmaster *m, uint8_t r, uint8_t value)
{
    cogmaster__bus_out(m, 0xBFu, value);
    cogmaster__bus_out(m, 0xBFu, (uint8_t)(0x80u | r));
}
static void mode(cogmaster *m, uint32_t height)
{
    reg(m, 0u, 6u);
    reg(m, 1u, (uint8_t)(0x40u | (height == 224u ? 0x10u : height == 240u ? 8u : 0u)));
}
static cogmaster_info info(cogmaster *m)
{
    cogmaster_info i = {0};
    i.size = sizeof i;
    CHECK_EQ_U64(cogmaster_get_info(m, &i), COGMASTER_STATUS_OK);
    return i;
}
static void run(cogmaster *m)
{
    uint32_t n = allocations;
    uint64_t end = m->bus.deadline;
    cogmaster_run_result r = {0};
    r.size = sizeof r;
    CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(allocations, n);
    CHECK(m->ticks >= end && m->ticks < end + 69u);
    CHECK_EQ_U64(m->bus.deadline, end + (m->tv == COGMASTER_TV_PAL ? 214092u : 179208u));
}
static void heights(void)
{
    for (uint32_t tv = COGMASTER_TV_NTSC; tv <= COGMASTER_TV_PAL; tv++) {
        for (uint32_t h = 192u; h <= 240u; h += h == 192u ? 32u : 16u) {
            cogmaster *m = make(COGMASTER_MACHINE_SMS2, tv);
            CHECK_EQ_U64(info(m).field_h, 192u);
            mode(m, h);
            run(m);
            run(m);
            CHECK_EQ_U64(info(m).field_h, h);
            CHECK_EQ_U64(info(m).crop_h, h);
            CHECK_EQ_U64(info(m).par_num, tv == COGMASTER_TV_PAL ? 2950000u : 8u);
            CHECK_EQ_U64(info(m).par_den, tv == COGMASTER_TV_PAL ? 2128137u : 7u);
            cogmaster_destroy(m);
        }
    }
}
static void scene(cogmaster *m, uint32_t h)
{
    /* Solid tile 1 red, tile 2 green; alternating map rows independently
       expose the final row and the 224/256-pixel virtual-map wrap. */
    mode(m, h);
    reg(m, 5u, 0x7Eu);
    memset(m->vdp.vram + 0x3F00u, 0xE0, 64u);
    m->vdp.cram[1] = 3u;
    m->vdp.cram[2] = 12u;
    for (uint32_t y = 0u; y < 8u; y++) {
        m->vdp.vram[32u + y * 4u] = 255u;
        m->vdp.vram[64u + y * 4u + 1u] = 255u;
    }
    uint32_t base = h == 192u ? 0x3800u : 0x3700u;
    for (uint32_t row = 0u; row < (h == 192u ? 28u : 32u); row++) {
        for (uint32_t x = 0u; x < 32u; x++) {
            m->vdp.vram[base + row * 64u + x * 2u] = (uint8_t)(1u + (row & 1u));
        }
    }
}
static uint16_t pixels[240u * 260u + 16u];
static cogmaster_frame image(cogmaster *m, uint32_t flags, uint32_t pitch)
{
    cogmaster_frame f = {0};
    f.size = sizeof f;
    f.pixels = pixels + 8u;
    f.flags = flags;
    f.pitch_bytes = pitch;
    CHECK_EQ_U64(cogmaster_get_frame(m, &f), COGMASTER_STATUS_OK);
    return f;
}
static void coordinates(void)
{
    for (uint32_t h = 192u; h <= 240u; h += h == 192u ? 32u : 16u) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC);
        scene(m, h);
        reg(m, 9u, 8u);
        run(m);
        run(m);
        for (uint32_t i = 0u; i < sizeof pixels / sizeof pixels[0]; i++)
            pixels[i] = 0xA55Au;
        cogmaster_frame f = image(m, COGMASTER_FRAME_FULL_FIELD, 520u);
        CHECK_EQ_U64(f.h, h);
        for (uint32_t y = 0u; y < h; y++) {
            uint32_t sy = (y + 8u) % (h == 192u ? 224u : 256u);
            CHECK_EQ_HEX(pixels[8u + y * 260u], (sy / 8u & 1u) != 0u ? 0x0F0u : 0x00Fu);
            CHECK_EQ_HEX(pixels[8u + y * 260u + 255u], pixels[8u + y * 260u]);
            CHECK_EQ_HEX(pixels[8u + y * 260u + 256u], 0xA55Au);
        }
        CHECK_EQ_HEX(pixels[7], 0xA55Au);
        CHECK_EQ_HEX(pixels[8u + h * 260u], 0xA55Au);
        reg(m, 9u, 255u);
        run(m);
        run(m);
        (void)image(m, 0u, 520u);
        CHECK_EQ_HEX(pixels[8u + 260u], 0x00Fu); /* 256 wraps to row 0 or row 4. */
        cogmaster_destroy(m);
    }
}
static void transitions(void)
{
    static const uint32_t heights[] = {192u, 224u, 240u};
    for (uint32_t tv = COGMASTER_TV_NTSC; tv <= COGMASTER_TV_PAL; tv++) {
        for (uint32_t a = 0u; a < 3u; a++) {
            for (uint32_t b = 0u; b < 3u; b++) {
                for (uint32_t edge = 599u; edge <= 601u; edge++) {
                    cogmaster *m = make(COGMASTER_MACHINE_SMS2, tv);
                    scene(m, heights[a]);
                    run(m);
                    run(m);
                    uint32_t before = info(m).field_h;
                    /* A following raster's live palette and backdrop cannot
                       change any pixel or metadata of the completed image. */
                    m->vdp.cram[1] = 0x30u;
                    m->vdp.cram[17] = 0x0Cu;
                    reg(m, 7u, 1u);
                    cogmaster_frame old = image(m, 0u, 520u);
                    CHECK_EQ_U64(old.h, before);
                    CHECK_EQ_HEX(old.backdrop_rgb444, 0u);
                    CHECK_EQ_HEX(pixels[8u], 0x00Fu);
                    CHECK_EQ_HEX(pixels[8u + (before - 1u) * 260u], 0x0F0u);
                    uint32_t last = tv == COGMASTER_TV_PAL ? 312u : 261u;
                    m->bus.line = last;
                    m->bus.t = 599u;
                    m->bus.at = m->ticks;
                    m->ticks += edge - 599u;
                    mode(m, heights[b]);
                    if (edge == 599u) {
                        m->ticks++;
                        cogmaster__bus_sync(m);
                    }
                    CHECK_EQ_U64(info(m).field_h, before);
                    /* At/after-edge writes miss this latch. Publish the selected
                       outgoing field; never label it with pending registers. */
                    cogmaster__vdp_finish_field(m);
                    CHECK_EQ_U64(info(m).field_h, edge == 599u ? heights[b] : heights[a]);
                    cogmaster__vdp_event(m, last, 600u);
                    cogmaster__vdp_finish_field(m);
                    CHECK_EQ_U64(info(m).field_h, heights[b]);
                    cogmaster_destroy(m);
                }
            }
        }
    }
}
static void buffers(void)
{
    for (uint32_t machine = COGMASTER_MACHINE_SMS2; machine <= COGMASTER_MACHINE_GG; machine++) {
        cogmaster *m = make(machine, COGMASTER_TV_NTSC);
        scene(m, 224u);
        run(m);
        run(m);
        for (uint32_t full = 0u; full < 2u; full++) {
            for (uint32_t i = 0u; i < sizeof pixels / sizeof pixels[0]; i++)
                pixels[i] = 0xA55Au;
            cogmaster_frame f = image(m, full, 520u);
            CHECK_EQ_U64(f.field_h, 224u);
            CHECK_EQ_U64(f.y, !full && machine == COGMASTER_MACHINE_GG ? 40u : 0u);
            CHECK_EQ_U64(f.h, !full && machine == COGMASTER_MACHINE_GG ? 144u : 224u);
            CHECK_EQ_HEX(pixels[7u], 0xA55Au);
            CHECK_EQ_HEX(pixels[8u + f.h * 260u], 0xA55Au);
            f.pitch_bytes = f.w * 2u - 1u;
            memset(pixels, 0xA5, sizeof pixels);
            CHECK_EQ_U64(cogmaster_get_frame(m, &f), COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL);
            f.pitch_bytes = 65537u;
            CHECK_EQ_U64(cogmaster_get_frame(m, &f), COGMASTER_STATUS_ERR_ARGUMENT);
            for (uint32_t i = 0u; i < sizeof pixels / sizeof pixels[0]; i++)
                CHECK_EQ_HEX(pixels[i], 0xA5A5u);
        }
        cogmaster_destroy(m);
    }
}
static void state(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC);
    scene(m, 224u);
    run(m);
    run(m);
    size_t n = cogmaster__state_size(m);
    uint8_t *bytes = malloc(n);
    size_t count = cogmaster__state_record(m, NULL, 0u);
    struct cogmaster__state_field *fields = malloc(count * sizeof *fields);
    CHECK(bytes != NULL && fields != NULL);
    if (bytes != NULL && fields != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(m, bytes, n), COGMASTER_STATUS_OK);
        mode(m, 240u);
        run(m);
        run(m);
        CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(info(m).field_h, 224u);
        (void)image(m, 0u, 520u);
        CHECK_EQ_HEX(pixels[8u + 223u * 260u], 0x0F0u);
        cogmaster__state_record(m, fields, count);
        for (size_t i = 0u; i < count; i++) {
            if (strcmp(fields[i].name, "completed_height") == 0 ||
                strcmp(fields[i].name, "vdp_height") == 0) {
                uint8_t saved = bytes[fields[i].offset];
                bytes[fields[i].offset] = 223u;
                cogmaster__state_seal(bytes, n);
                CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_ERR_ARGUMENT);
                CHECK_EQ_U64(info(m).field_h, 224u);
                bytes[fields[i].offset] = saved;
            }
        }
    }
    free(fields);
    free(bytes);
    cogmaster_destroy(m);
}
static void hostile(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC);
    /* LD A,6; OUT(BF),A; LD A,80; OUT(BF),A; INC A; JP 2.
       Repeated guest writes cannot extend a fixed frame deadline. */
    static const uint8_t code[] = {0x3Eu, 6u,    0xD3u, 0xBFu, 0x3Eu, 0x80u,
                                   0xD3u, 0xBFu, 0x3Cu, 0xC3u, 2u,    0u};
    memcpy(m->cart.rom, code, sizeof code);
    for (uint32_t i = 0u; i < 6u; i++)
        run(m);
    cogmaster_destroy(m);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "heights") == 0)
        heights();
    else if (strcmp(argv[1], "coordinates") == 0)
        coordinates();
    else if (strcmp(argv[1], "transitions") == 0)
        transitions();
    else if (strcmp(argv[1], "buffers") == 0)
        buffers();
    else if (strcmp(argv[1], "state") == 0)
        state();
    else if (strcmp(argv[1], "hostile") == 0)
        hostile();
    else
        return 2;
    CHECK_DONE();
}
