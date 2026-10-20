/* API version 2: every name version 2 adds, used as a caller would. Once
   released this file never changes; v1.c covers the names of version 1. */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "../check.h"

static void *v2_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v2_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint8_t image[1024];

int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 2);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE, 7);

    cogmaster_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(cogmaster_get_versions(&v), COGMASTER_STATUS_OK);
    CHECK(v.api >= 2);

    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v2_alloc;
    cfg.allocator.free = v2_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        CHECK_DONE();
    }

    /* An image below 1 KiB is refused by size; at 1 KiB it loads and runs. */
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image - 1u;
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    media.image_size = sizeof image;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);

    cogmaster_destroy(m);
    CHECK_DONE();
}
