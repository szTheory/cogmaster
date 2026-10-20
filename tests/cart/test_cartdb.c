#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"

static uint8_t image[32768];
static void *allocate(void *u, size_t n) { (void)u; return malloc(n); }
static void release(void *u, void *p, size_t n) { (void)u; (void)n; free(p); }
int main(void)
{
    static const uint8_t magic[8] = {0x54,0x4d,0x52,0x20,0x53,0x45,0x47,0x41};
    memcpy(image + 0x7ff0, magic, 8);
    image[0x7fff] = 0x6c;
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    cogmaster_info info = {0};
    info.size = sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_GG);
    cogmaster_destroy(m);
    CHECK_DONE();
}
