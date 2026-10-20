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
    else
        return 2;
    CHECK_DONE();
}
