/* API v8: appended, bounded per-frame display observation. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"
static void *allocate(void *u, size_t n)
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
    CHECK(COGMASTER_API_VERSION >= 8);
    CHECK_EQ_U64(offsetof(cogmaster_run_result, display_enabled), 16u);
    CHECK_EQ_U64(sizeof(cogmaster_run_result), 20u);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (uint32_t size = 12u; size <= 20u; size += 4u) {
        cogmaster_run_result r;
        memset(&r, 0x5a, sizeof r);
        r.size = size;
        CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(r.display_enabled, size < 20u ? 0x5a5a5a5au : 0u);
        if (size == 12u)
            CHECK_EQ_U64(r.lagged, 0x5a5a5a5au);
    }
    cogmaster_destroy(m);
    CHECK_DONE();
}
