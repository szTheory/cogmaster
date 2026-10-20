#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"
#include "cartdb.h"

static uint8_t image[32768];
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
int main(int argc, char **argv)
{
    if (argc == 2) {
        static uint8_t fixture[65536 + 512];
        FILE *file = fopen(argv[1], "rb");
        CHECK(file != NULL);
        if (file == NULL)
            CHECK_DONE();
        size_t n = fread(fixture, 1, sizeof fixture, file);
        fclose(file);
        cogmaster_cart_info p = {0};
        p.size = sizeof p;
        CHECK_EQ_U64(cogmaster_probe(fixture, n, &p), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(p.identified, 1u);
        CHECK_EQ_U64(p.public_identity, 0u);
        CHECK_EQ_U64(p.persistence, COGMASTER_PERSISTENCE_BATTERY);
        CHECK_EQ_U64(p.persistent_size, 32768u);
        CHECK_EQ_U64(p.persistence_geometry, COGMASTER_GEOMETRY_SUPPORTED);
        CHECK_DONE();
    }
    const struct cogmaster__cart_fact *f =
        cogmaster__cartdb_find("0dad260b5357b2f61d1452bb73d8321ca53d7262");
    CHECK(f != NULL);
    if (f) {
        CHECK_EQ_U64(f->persistence, COGMASTER_PERSISTENCE_BATTERY);
        CHECK_EQ_U64(f->declared, 24576u);
        CHECK_EQ_U64(f->geometry, COGMASTER_GEOMETRY_UNSUPPORTED);
    }
    CHECK(cogmaster__cartdb_find("0000000000000000000000000000000000000000") == NULL);
    f = cogmaster__cartdb_find("c3305d745792a931ea92da6e6457c43d7cc45cf2");
    CHECK(f != NULL);
    if (f) {
        CHECK_EQ_U64(f->machine, COGMASTER_MACHINE_GG);
        CHECK_EQ_U64(f->video, COGMASTER_VIDEO_SMS);
        CHECK_EQ_U64(f->persistence, COGMASTER_PERSISTENCE_NONE);
        CHECK_EQ_U64(f->public_identity, 1u);
    }
    static const uint8_t magic[8] = {0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41};
    memcpy(image + 0x7ff0, magic, 8);
    image[0x7fff] = 0x6c;
    cogmaster_cart_info probe = {0};
    probe.size = sizeof probe;
    CHECK_EQ_U64(cogmaster_probe(image, sizeof image, &probe), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(probe.persistence, COGMASTER_PERSISTENCE_UNKNOWN);
    probe.size = 52u;
    probe.identified = 0xabcdefu;
    CHECK_EQ_U64(cogmaster_probe(image, sizeof image, &probe), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(probe.identified, 0xabcdefu);
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
