/* Guest-rendered fields checked through the public API, without VDP helpers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "sha256.h"
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
int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    static uint8_t image[32768];
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL)
        return 1;
    CHECK_EQ_U64(fread(image, 1u, sizeof image, file), sizeof image);
    CHECK(fclose(file) == 0);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = alloc;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = COGMASTER_MACHINE_SMS2;
    media.tv = strcmp(argv[3], "pal") == 0 ? COGMASTER_TV_PAL : COGMASTER_TV_NTSC;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    for (uint32_t i = 0u; i < 30u; i++)
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    cogmaster_info info = {0};
    info.size = sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    uint32_t height = (uint32_t)strtoul(argv[2], NULL, 10);
    CHECK_EQ_U64(info.field_h, height);
    cogmaster_destroy(m);
    CHECK_DONE();
}
