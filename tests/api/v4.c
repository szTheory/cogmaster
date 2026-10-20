/* API v4 adds a machine and diagnostic values without changing any layout. */
#include <stdlib.h>
#include "cogmaster.h"
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
int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 4);
    CHECK_EQ_U64(COGMASTER_MACHINE_AUTO, 0u);
    CHECK_EQ_U64(COGMASTER_MACHINE_SMS2, 1u);
    CHECK_EQ_U64(COGMASTER_MACHINE_GG, 2u);
    CHECK_EQ_U64(COGMASTER_MACHINE_SMS1, 3u);
    CHECK_EQ_U64(COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE, 6u);
    CHECK_EQ_U64(COGMASTER_FAULT_DEVICE_VDP, 6u);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = alloc;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = (uint32_t)(4u * sizeof(void *));
    media.machine = COGMASTER_MACHINE_SMS1;
    media.region = 0xFFFFFFFFu;
    media.tv = 0xFFFFFFFFu;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    cogmaster_info info = {0};
    info.size = sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_SMS1);
    CHECK_EQ_U64(info.region, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(info.tv, COGMASTER_TV_NTSC);
    cogmaster_destroy(m);
    CHECK_DONE();
}
