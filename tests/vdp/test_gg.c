/* Independent native palette, LCD coordinates and controller decode checks. */
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "state.h"
#include "../check.h"

static void *alloc(void *u, size_t n) { (void)u; return malloc(n); }
static void release(void *u, void *p, size_t n) { (void)u; (void)n; free(p); }
static cogmaster *make(void)
{
    cogmaster_config c = {0};
    c.size = sizeof c; c.allocator.alloc = alloc; c.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&c, &m), 0u);
    cogmaster_media media = {0};
    media.size = sizeof media; media.machine = COGMASTER_MACHINE_GG;
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
int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "palette") != 0) return 2;
    palette();
    CHECK_DONE();
}
