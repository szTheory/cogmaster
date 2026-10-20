/* Independent native palette, LCD coordinates and controller decode checks. */
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
    cogmaster_config c = {0};
    c.size = sizeof c;
    c.allocator.alloc = alloc;
    c.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&c, &m), 0u);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.machine = COGMASTER_MACHINE_GG;
    CHECK_EQ_U64(cogmaster_load(m, &media), 0u);
    return m;
}
static void cram(cogmaster *m, uint16_t addr, uint8_t value)
{
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, (uint8_t)addr);
    cogmaster__vdp_write(m, COGMASTER__VDP_CONTROL, (uint8_t)(0xC0u | (addr >> 8)));
    cogmaster__vdp_write(m, COGMASTER__VDP_DATA, value);
}
static void palette(void)
{
    cogmaster *m = make();
    cram(m, 32u, 0xABu);
    CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), 0u);
    cram(m, 33u, 0xF5u);
    CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), 0x5ABu);
    cram(m, 0u, 0x67u);
    cram(m, 33u, 8u);
    CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), 0x867u);
    cram(m, 0x3FFEu, 0x89u);
    cram(m, 0x3FFFu, 0xFAu);
    CHECK_EQ_HEX(m->vdp.addr, 0u);
    cram(m, 33u, 2u);
    CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), 0x289u);
    cogmaster_destroy(m);
}
static void colours(void)
{
    cogmaster *m = make();
    for (uint32_t c = 0u; c < 4096u; c++) {
        cram(m, 32u, (uint8_t)c);
        cram(m, 33u, (uint8_t)(c >> 8));
        CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), c);
    }
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.machine = COGMASTER_MACHINE_GG;
    media.video_mode = COGMASTER_VIDEO_SMS;
    CHECK_EQ_U64(cogmaster_load(m, &media), 0u);
    for (uint32_t c = 0u; c < 64u; c++) {
        cram(m, 48u, (uint8_t)c);
        uint32_t expected = (c % 4u) * 5u + ((c / 4u) % 4u) * 80u + (c / 16u) * 1280u;
        CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), expected);
    }
    cogmaster_destroy(m);
}
static void intervals(void)
{
    cogmaster *m = make();
    cram(m, 32u, 0xABu);
    cram(m, 33u, 5u);
    cogmaster__vdp_render_until(m, 0u, 8u);
    cram(m, 32u, 0x67u);
    cogmaster__vdp_render_until(m, 0u, 16u);
    cram(m, 33u, 8u);
    cogmaster__vdp_render_until(m, 0u, 24u);
    for (uint32_t x = 0u; x < 24u; x++)
        CHECK_EQ_HEX(m->field[0][x], x < 16u ? 0x5ABu : 0x867u);
    cogmaster_destroy(m);
}
static uint16_t pixels[256u * 240u], crop[160u * 144u];
static cogmaster_frame image(cogmaster *m, int full)
{
    cogmaster_frame f = {0};
    f.size = sizeof f;
    f.pixels = full ? pixels : crop;
    f.pitch_bytes = full ? 512u : 320u;
    f.flags = full ? COGMASTER_FRAME_FULL_FIELD : 0u;
    CHECK_EQ_U64(cogmaster_get_frame(m, &f), 0u);
    return f;
}
static void publish(cogmaster *m, uint32_t h, uint32_t seed)
{
    m->vdp.height = (uint8_t)h;
    for (uint32_t y = 0u; y < h; y++)
        for (uint32_t x = 0u; x < 256u; x++)
            m->field[y][x] = (uint16_t)((y * 7u + x + seed) & 4095u);
    m->render_line = h;
    m->render_x = 0u;
    cogmaster__vdp_finish_field(m);
}
static void coordinates(void)
{
    cogmaster *m = make();
    for (uint32_t h = 192u; h <= 224u; h += 32u) {
        publish(m, h, 0u);
        cogmaster_frame f = image(m, 0);
        CHECK_EQ_U64(f.x, 48u);
        CHECK_EQ_U64(f.y, h == 192u ? 24u : 40u);
        CHECK_EQ_U64(f.w, 160u);
        CHECK_EQ_U64(f.h, 144u);
        (void)image(m, 1);
        for (uint32_t y = 0u; y < 144u; y++)
            for (uint32_t x = 0u; x < 160u; x++) {
                CHECK_EQ_HEX(crop[y * 160u + x], ((y + f.y) * 7u + x + 48u) & 4095u);
                CHECK_EQ_HEX(crop[y * 160u + x], pixels[(y + f.y) * 256u + x + 48u]);
            }
    }
    cogmaster_destroy(m);
}
static void freeze(void)
{
    cogmaster *m = make();
    publish(m, 224u, 0u);
    (void)image(m, 0);
    uint16_t held = crop[0];
    publish(m, 240u, 100u);
    cogmaster_frame f = image(m, 0);
    CHECK_EQ_U64(f.field_h, 224u);
    CHECK_EQ_U64(f.y, 40u);
    CHECK_EQ_HEX(crop[0], held);
    f = image(m, 1);
    CHECK_EQ_U64(f.h, 240u);
    CHECK_EQ_HEX(pixels[0], 100u);
    publish(m, 192u, 200u);
    (void)image(m, 0);
    CHECK_EQ_HEX(crop[0], held);
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), 0u);
    publish(m, 192u, 300u);
    (void)image(m, 0);
    CHECK_EQ_HEX(crop[0], held);
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_POWER), 0u);
    publish(m, 192u, 400u);
    f = image(m, 0);
    CHECK_EQ_U64(f.field_h, 192u);
    CHECK_EQ_U64(f.y, 24u);
    CHECK_EQ_HEX(crop[0], 24u * 7u + 48u + 400u);
    cogmaster_destroy(m);
}
static void profiles(void)
{
    cogmaster *m = make();
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.machine = COGMASTER_MACHINE_SMS2;
    media.video_mode = COGMASTER_VIDEO_GG;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_PROFILE_REJECTED);
    CHECK_EQ_U64(m->machine, COGMASTER_MACHINE_GG);
    media.machine = COGMASTER_MACHINE_GG;
    media.video_mode = 3u;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_PROFILE_REJECTED);
    media.video_mode = COGMASTER_VIDEO_SMS;
    CHECK_EQ_U64(cogmaster_load(m, &media), 0u);
    cogmaster_info i = {0};
    i.size = sizeof i;
    CHECK_EQ_U64(cogmaster_get_info(m, &i), 0u);
    CHECK_EQ_U64(i.video_mode, COGMASTER_VIDEO_SMS);
    CHECK_EQ_U64(i.crop_w, 256u);
    CHECK_EQ_U64(i.par_num, 8u);
    CHECK_EQ_U64(i.par_den, 7u);
    media.size = sizeof(void *) == 8u ? 40u : 24u;
    media.video_mode = 99u;
    CHECK_EQ_U64(cogmaster_load(m, &media), 0u);
    CHECK_EQ_U64(m->video_mode, COGMASTER_VIDEO_GG);
    media.size = 4u * sizeof(void *);
    CHECK_EQ_U64(cogmaster_load(m, &media), 0u);
    CHECK_EQ_U64(m->video_mode, COGMASTER_VIDEO_GG);
    cogmaster_destroy(m);
}
static void ports(void)
{
    cogmaster *m = make();
    for (uint32_t native = 0u; native < 2u; native++) {
        m->video_mode = native ? COGMASTER_VIDEO_GG : COGMASTER_VIDEO_SMS;
        m->input.p1 = 0x15u;
        m->input.p2 = 63u;
        m->input.start = 1u;
        for (uint32_t p = 0u; p < 256u; p++) {
            m->input_read = 0u;
            uint8_t v = cogmaster__io_in(m, (uint16_t)p);
            int dc = p == 0xC0u || p == 0xDCu;
            int dd = p == 0xC1u || p == 0xDDu;
            CHECK_EQ_U64(m->input_read, dc || dd || (native && p == 0u));
            if (dc)
                CHECK_EQ_HEX(v, 0xEAu);
            else if (dd)
                CHECK_EQ_HEX(v, 255u);
            else if (native && p == 0u)
                CHECK_EQ_HEX(v, 0x40u);
            else if (p > 6u || !p)
                CHECK_EQ_HEX(v, 255u);
        }
        cogmaster_input in = {0};
        in.size = sizeof in;
        in.start = 1u;
        memset(&m->input, 0, sizeof m->input);
        CHECK_EQ_U64(cogmaster_set_input(m, &in), 0u);
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), 0u);
        CHECK_EQ_U64(m->vdp.pause, !native);
        m->z80.nmi_pending = 0u;
        cogmaster__vdp_event(m, 260u, 611u);
        CHECK_EQ_U64(m->z80.nmi_pending, !native);
        m->vdp.pause = 0u;
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), 0u);
        CHECK_EQ_U64(m->vdp.pause, 0u);
    }
    cogmaster_destroy(m);
}
static void state(void)
{
    cogmaster *m = make();
    publish(m, 224u, 0u);
    publish(m, 240u, 99u);
    cram(m, 0u, 0x45u);
    size_t n = cogmaster__state_size(m);
    uint8_t *b = malloc(n);
    CHECK(b != NULL);
    if (b) {
        CHECK_EQ_U64(cogmaster__state_save(m, b, n), 0u);
        cram(m, 0u, 0u);
        m->lcd.pixels[0][0] = 0u;
        m->lcd.frozen = 0u;
        CHECK_EQ_U64(cogmaster__state_load(m, b, n), 0u);
        CHECK_EQ_U64(m->lcd.frozen, 1u);
        CHECK_EQ_HEX(m->lcd.pixels[0][0], 40u * 7u + 48u);
        cram(m, 33u, 6u);
        CHECK_EQ_HEX(cogmaster__vdp_backdrop(m), 0x645u);
        m->video_mode = COGMASTER_VIDEO_SMS;
        CHECK_EQ_U64(cogmaster__state_load(m, b, n), COGMASTER_STATUS_ERR_ARGUMENT);
        free(b);
    }
    cogmaster_destroy(m);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "palette") == 0)
        palette();
    else if (strcmp(argv[1], "colours") == 0)
        colours();
    else if (strcmp(argv[1], "intervals") == 0)
        intervals();
    else if (strcmp(argv[1], "coordinates") == 0)
        coordinates();
    else if (strcmp(argv[1], "freeze") == 0)
        freeze();
    else if (strcmp(argv[1], "profiles") == 0)
        profiles();
    else if (strcmp(argv[1], "ports") == 0)
        ports();
    else if (strcmp(argv[1], "state") == 0)
        state();
    else
        return 2;
    CHECK_DONE();
}
