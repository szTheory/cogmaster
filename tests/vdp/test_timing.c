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
static cogmaster *make(void)
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
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
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
    else
        return 2;
    CHECK_DONE();
}
