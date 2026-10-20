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
int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "heights") != 0)
        return 2;
    heights();
    CHECK_DONE();
}
