/* Selected timing model, independently expected edges (video.md). */
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
static cogmaster *make_profile(uint32_t machine, uint32_t tv)
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
static cogmaster *make(void)
{
    return make_profile(COGMASTER_MACHINE_SMS2, COGMASTER_TV_NTSC);
}
static void reg(cogmaster *m, uint8_t r, uint8_t v)
{
    cogmaster__bus_out(m, 0xBFu, v);
    cogmaster__bus_out(m, 0xBFu, (uint8_t)(0x80u | r));
}
static void beam(cogmaster *m, uint32_t line, uint32_t tick)
{
    m->bus.line = line;
    m->bus.t = tick;
    m->bus.at = m->ticks;
}
static void pass(cogmaster *m, uint32_t ticks)
{
    m->ticks += ticks;
    cogmaster__bus_sync(m);
}
static uint8_t status(cogmaster *m)
{
    return cogmaster__bus_in(m, 0xBFu);
}
static void irq_edges(void)
{
    /* Each status access is stamped independently at edge-1/edge/edge+1. */
    for (uint32_t offset = 0u; offset < 3u; offset++) {
        cogmaster *m = make();
        reg(m, 0u, 0x14u);
        reg(m, 1u, 0x20u);
        reg(m, 10u, 0u);
        beam(m, 261u, 607u);
        pass(m, 1u); /* blank reload */
        beam(m, 20u, 606u);
        m->ticks += offset + 1u;
        CHECK_EQ_HEX(status(m), 0x1Fu);
        CHECK_EQ_U64(m->z80.int_line, 0u);
        pass(m, 2u - offset);
        CHECK_EQ_U64(m->z80.int_line, offset == 0u ? 1u : 0u);
        (void)status(m);
        beam(m, 192u, 605u);
        m->ticks += offset + 1u;
        CHECK_EQ_HEX(status(m), offset == 0u ? 0x1Fu : 0x9Fu);
        CHECK_EQ_U64(m->z80.int_line, 0u);
        /* A frame clear at 607 does not consume the distinct 608 line event. */
        if (offset == 1u) {
            pass(m, 1u);
            CHECK_EQ_U64(m->z80.int_line, 1u);
        }
        cogmaster_destroy(m);
    }
}
static void irq_reload(void)
{
    static const uint8_t reloads[] = {0u, 1u, 255u};
    for (uint32_t r = 0u; r < 3u; r++) {
        cogmaster *m = make();
        reg(m, 0u, 0x14u);
        reg(m, 10u, reloads[r]);
        beam(m, 261u, 607u);
        pass(m, 1u);
        for (uint32_t line = 0u; line <= 193u; line++) {
            beam(m, line, 607u);
            pass(m, 1u);
            /* Literal schedules: every line, odd lines, never. */
            uint32_t expected = line <= 192u && (r == 0u || (r == 1u && line % 2u == 1u));
            CHECK_EQ_U64(m->z80.int_line, expected);
            (void)status(m);
        }
        cogmaster_destroy(m);
    }
    cogmaster *m = make();
    reg(m, 0u, 0x14u);
    reg(m, 10u, 1u);
    beam(m, 261u, 607u);
    pass(m, 1u);
    beam(m, 0u, 607u);
    pass(m, 1u);
    reg(m, 10u, 255u); /* must not replace the running zero */
    beam(m, 1u, 607u);
    pass(m, 1u);
    CHECK_EQ_U64(m->z80.int_line, 1u);
    (void)status(m);
    beam(m, 2u, 607u);
    pass(m, 1u);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    cogmaster_destroy(m);
}
static void irq_pending(void)
{
    cogmaster *m = make();
    reg(m, 10u, 0u);
    beam(m, 261u, 607u);
    pass(m, 1u);
    beam(m, 0u, 607u);
    pass(m, 1u);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    reg(m, 0u, 0x14u);
    CHECK_EQ_U64(m->z80.int_line, 1u);
    reg(m, 0u, 4u);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    beam(m, 192u, 606u);
    pass(m, 1u);
    reg(m, 1u, 0x20u);
    CHECK_EQ_U64(m->z80.int_line, 1u);
    reg(m, 0u, 0x14u);
    reg(m, 1u, 0u);
    CHECK_EQ_U64(m->z80.int_line, 1u);
    CHECK_EQ_HEX(status(m), 0x9Fu);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    reg(m, 1u, 0x20u);
    CHECK_EQ_U64(m->z80.int_line, 0u);
    cogmaster_destroy(m);
}
static void irq_cpu(void)
{
    cogmaster *m = make();
    /* Owned EI/HALT loop and IM1 handler: two RAM counts, frame vs line. */
    static const uint8_t loop[] = {0xF3u, 0x31u, 0xF0u, 0xDFu, 0xEDu,
                                   0x56u, 0xFBu, 0x76u, 0x18u, 0xFDu};
    static const uint8_t handler[] = {0xF5u, 0xDBu, 0xBFu, 0x21u, 0x00u, 0xC1u, 0xCBu, 0x7Fu,
                                      0x20u, 0x01u, 0x23u, 0x34u, 0xF1u, 0xFBu, 0xEDu, 0x4Du};
    memcpy(m->cart.rom, loop, sizeof loop);
    memcpy(m->cart.rom + 0x38u, handler, sizeof handler);
    reg(m, 0u, 0x14u);
    reg(m, 1u, 0x20u);
    reg(m, 10u, 31u);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK(m->ticks >= 179208u && m->ticks < 179208u + 69u);
    for (uint32_t i = 0u; i < 40u; i++) {
        cogmaster__z80_step(m);
        cogmaster__bus_sync(m);
    }
    CHECK_EQ_U64(m->ram[0x100], 1u);
    CHECK_EQ_U64(m->ram[0x101], 6u);
    cogmaster_destroy(m);
}
static void irq_state(void)
{
    cogmaster *m = make();
    reg(m, 0u, 0x14u);
    reg(m, 10u, 0u);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->z80.int_line, 1u);
    size_t n = cogmaster__state_size(m);
    uint8_t *bytes = malloc(n);
    size_t count = cogmaster__state_record(m, NULL, 0u);
    struct cogmaster__state_field *fields = malloc(count * sizeof *fields);
    CHECK(bytes != NULL && fields != NULL);
    if (bytes != NULL && fields != NULL) {
        CHECK_EQ_U64(cogmaster__state_save(m, bytes, n), COGMASTER_STATUS_OK);
        (void)status(m);
        CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(m->z80.int_line, 1u);
        CHECK_EQ_U64(m->vdp.line_pending, 1u);
        CHECK_EQ_U64(m->vdp.line_counter, 0u);
        CHECK_EQ_U64(cogmaster__state_record(m, fields, count), count);
        for (size_t i = 0u; i < count; i++) {
            if (strcmp(fields[i].name, "vdp_line_pending") == 0) {
                bytes[fields[i].offset] = 0u;
                cogmaster__state_seal(bytes, n);
                CHECK_EQ_U64(cogmaster__state_load(m, bytes, n), COGMASTER_STATUS_ERR_ARGUMENT);
                CHECK_EQ_U64(m->vdp.line_pending, 1u);
                CHECK_EQ_U64(m->z80.int_line, 1u);
            }
        }
    }
    free(fields);
    free(bytes);
    cogmaster_destroy(m);
}
static void vtable(uint32_t tv, uint32_t height, const uint8_t ranges[][2], size_t count)
{
    uint8_t table[313];
    uint32_t n = 0u;
    for (size_t i = 0u; i < count; i++) {
        for (uint32_t v = ranges[i][0]; v <= ranges[i][1]; v++)
            table[n++] = (uint8_t)v;
    }
    CHECK_EQ_U64(n, tv == COGMASTER_TV_PAL ? 313u : height == 240u ? 263u : 262u);
    cogmaster *m = make_profile(COGMASTER_MACHINE_SMS2, tv);
    beam(m, 0u, 603u);
    reg(m, 0u, 6u);
    reg(m, 1u, height == 224u ? 0x10u : height == 240u ? 8u : 0u);
    for (uint32_t line = 0u; line < n; line++) {
        beam(m, line % (tv == COGMASTER_TV_PAL ? 313u : 262u), 603u);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Eu), table[line]);
        pass(m, 1u);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Eu), table[(line + 1u) % n]);
        pass(m, 1u);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Eu), table[(line + 1u) % n]);
    }
    cogmaster_destroy(m);
}
static void hcounter(void)
{
    cogmaster *m = make();
    /* Literal active-origin intervals: all 171 count values, four ticks each. */
    static const struct {
        uint32_t first, last;
        uint8_t low, high;
    } intervals[] = {
        {0u, 559u, 0x08u, 0x93u}, {560u, 651u, 0xE9u, 0xFFu}, {652u, 683u, 0x00u, 0x07u}};
    for (size_t i = 0u; i < 3u; i++) {
        uint32_t t = intervals[i].first;
        for (uint32_t value = intervals[i].low; value <= intervals[i].high; value++) {
            for (uint32_t phase = 0u; phase < 4u; phase++, t++) {
                beam(m, 20u, t);
                CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), value);
            }
        }
        CHECK_EQ_U64(t, intervals[i].last + 1u);
    }
    static const uint32_t ticks[] = {0u, 96u, 559u, 560u, 606u, 607u, 608u, 651u, 652u, 683u};
    static const uint8_t values[] = {8u, 0x20u, 0x93u, 0xE9u, 0xF4u, 0xF4u, 0xF5u, 0xFFu, 0u, 7u};
    for (size_t i = 0u; i < sizeof values; i++) {
        beam(m, 10u, ticks[i]);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), values[i]);
    }
    cogmaster_destroy(m);
}
static void th_capture(void)
{
    cogmaster *m = make();
    beam(m, 20u, 0u);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), 8u);
    cogmaster__bus_out(m, 0x3Fu, 0xDDu); /* A TH falls. */
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu) & 0xC0u, 0x80u);
    beam(m, 20u, 96u);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), 0x20u);
    cogmaster__bus_out(m, 0x3Fu, 0xFDu); /* A TH rises: capture $20. */
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu) & 0xC0u, 0xC0u);
    beam(m, 20u, 560u);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), 0x20u);
    cogmaster__bus_out(m, 0x3Fu, 0xFDu); /* unchanged level cannot recapture */
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), 0x20u);
    cogmaster__bus_out(m, 0x3Fu, 0x77u); /* B falls */
    cogmaster__bus_out(m, 0x3Fu, 0xFFu); /* input pull-up rises */
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), 0xE9u);
    beam(m, 21u, 0u);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x7Fu), 0xE9u);
    cogmaster_destroy(m);
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "irq_edges") == 0)
        irq_edges();
    else if (strcmp(argv[1], "irq_reload") == 0)
        irq_reload();
    else if (strcmp(argv[1], "irq_pending") == 0)
        irq_pending();
    else if (strcmp(argv[1], "irq_cpu") == 0)
        irq_cpu();
    else if (strcmp(argv[1], "irq_state") == 0)
        irq_state();
    else if (strcmp(argv[1], "hcounter") == 0)
        hcounter();
    else if (strcmp(argv[1], "th_capture") == 0)
        th_capture();
    else if (strcmp(argv[1], "v_ntsc_192") == 0) {
        static const uint8_t ranges[][2] = {{0u, 0xDAu}, {0xD5u, 0xFFu}};
        vtable(COGMASTER_TV_NTSC, 192u, ranges, 2u);
    } else if (strcmp(argv[1], "v_ntsc_224") == 0) {
        static const uint8_t ranges[][2] = {{0u, 0xEAu}, {0xE5u, 0xFFu}};
        vtable(COGMASTER_TV_NTSC, 224u, ranges, 2u);
    } else if (strcmp(argv[1], "v_ntsc_240") == 0) {
        static const uint8_t ranges[][2] = {{0u, 0xFFu}, {0u, 6u}};
        vtable(COGMASTER_TV_NTSC, 240u, ranges, 2u);
    } else if (strcmp(argv[1], "v_pal_192") == 0) {
        static const uint8_t ranges[][2] = {{0u, 0xF2u}, {0xBAu, 0xFFu}};
        vtable(COGMASTER_TV_PAL, 192u, ranges, 2u);
    } else if (strcmp(argv[1], "v_pal_224") == 0) {
        static const uint8_t ranges[][2] = {{0u, 0xFFu}, {0u, 2u}, {0xCAu, 0xFFu}};
        vtable(COGMASTER_TV_PAL, 224u, ranges, 3u);
    } else if (strcmp(argv[1], "v_pal_240") == 0) {
        static const uint8_t ranges[][2] = {{0u, 0xFFu}, {0u, 10u}, {0xD2u, 0xFFu}};
        vtable(COGMASTER_TV_PAL, 240u, ranges, 3u);
    } else
        return 2;
    CHECK_DONE();
}
