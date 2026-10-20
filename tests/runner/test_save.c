/* Files here live only in the test's private build directory. */
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "../../runner/path.h"
#include "../../runner/save.h"
#include "../check.h"

static uint8_t image[65536], raw[32769], bytes[32769];
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
static cogmaster *machine(const char *rom)
{
    FILE *f = cogmaster__path_open(rom, "rb");
    if (f == NULL)
        exit(1);
    size_t n = fread(image, 1u, sizeof image, f);
    CHECK_EQ_U64(fclose(f), 0);
    CHECK_EQ_U64(n, sizeof image);
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL)
        exit(1);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = n;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}
static void write_file(const char *path, size_t n)
{
    FILE *f = cogmaster__path_open(path, "wb");
    CHECK(f != NULL);
    if (f == NULL)
        exit(1);
    CHECK_EQ_U64(fwrite(raw, 1u, n, f), n);
    CHECK_EQ_U64(fclose(f), 0);
}
static void check_file(const char *path, const uint8_t *expected, size_t n)
{
    FILE *f = cogmaster__path_open(path, "rb");
    CHECK(f != NULL);
    if (f == NULL)
        return;
    CHECK_EQ_U64(fread(bytes, 1u, sizeof bytes, f), n);
    CHECK(memcmp(bytes, expected, n) == 0);
    CHECK_EQ_U64(fclose(f), 0);
}
static cogmaster_battery battery(cogmaster *m)
{
    cogmaster_battery b = {0};
    b.size = sizeof b;
    CHECK_EQ_U64(cogmaster_get_battery(m, &b), COGMASTER_STATUS_OK);
    return b;
}
static void imports(cogmaster *m, const char *dir, const char *path)
{
    static const size_t sizes[] = {0, 3, 32768, 32769};
    for (size_t i = 0; i < sizeof raw; i++)
        raw[i] = (uint8_t)(i * 17u);
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        write_file(path, sizes[i]);
        cogmaster__save s = {0};
        int status = cogmaster__save_load(&s, m, dir, "folder/round.trip.sms");
        CHECK_EQ_U64(status, sizes[i] > 32768u ? 4 : 0);
        cogmaster_battery b = battery(m);
        if (status == 0) {
            for (size_t j = 0; j < b.length; j++)
                CHECK_EQ_HEX(b.data[j], j < sizes[i] ? raw[j] : 0xffu);
        } else {
            CHECK(memcmp(b.data, raw, b.length) == 0);
        }
        check_file(path, raw, sizes[i]);
        cogmaster__save_dispose(&s);
    }
}
static void replacement(cogmaster *m, const char *dir, const char *path)
{
    cogmaster__save s = {0};
    CHECK_EQ_U64(cogmaster__save_load(&s, m, dir, "round.trip.sms"), 0);
    cogmaster_battery b = battery(m);
    for (size_t i = 0; i < b.length; i++)
        CHECK_EQ_HEX(b.data[i], 0xffu);
    CHECK_EQ_U64(cogmaster__save_flush(&s, m), 0);
    FILE *f = cogmaster__path_open(path, "rb");
    CHECK(f == NULL);
    if (f != NULL)
        (void)fclose(f);
    cogmaster__bus_write(m, 0xfffcu, 8u);
    cogmaster__bus_write(m, 0x8000u, 0x42u);
    CHECK_EQ_U64(cogmaster__save_flush(&s, m), 0);
    b = battery(m);
    check_file(path, b.data, b.length);
    CHECK_EQ_U64(s.acknowledged, b.generation);
    cogmaster__bus_write(m, 0x8000u, 0x53u);
    CHECK_EQ_U64(cogmaster__save_flush(&s, m), 0);
    b = battery(m);
    check_file(path, b.data, b.length);
    /* A clean flush must not even replace an externally changed file. */
    write_file(path, 3u);
    CHECK_EQ_U64(cogmaster__save_flush(&s, m), 0);
    check_file(path, raw, 3u);
    cogmaster__save_dispose(&s);
}
static void excluded(cogmaster *m, const char *dir)
{
    /* Synthetic metadata uses the same core query as the real fixture.
       Commercial ROM bytes are neither needed nor opened. */
    cogmaster__save s = {0};
    m->cart.persistence = COGMASTER_PERSISTENCE_UNKNOWN;
    CHECK_EQ_U64(cogmaster__save_load(&s, m, dir, "unknown.sms"), 77);
    CHECK(s.path == NULL);
    m->cart.persistence = COGMASTER_PERSISTENCE_BATTERY;
    m->cart.persistence_geometry = COGMASTER_GEOMETRY_UNSUPPORTED;
    CHECK_EQ_U64(cogmaster__save_load(&s, m, dir, "unsupported.sms"), 77);
    CHECK(s.path == NULL);
    m->cart.persistence = COGMASTER_PERSISTENCE_NONE;
    CHECK_EQ_U64(cogmaster__save_load(&s, m, dir, "none.sms"), 0);
    CHECK_EQ_U64(cogmaster__save_flush(&s, m), 0);
    CHECK(s.path == NULL);
}
int main(int argc, char **argv)
{
    if (argc != 5)
        return 2;
    cogmaster *m = machine(argv[2]);
    if (strcmp(argv[1], "imports") == 0)
        imports(m, argv[3], argv[4]);
    else if (strcmp(argv[1], "replacement") == 0)
        replacement(m, argv[3], argv[4]);
    else if (strcmp(argv[1], "excluded") == 0)
        excluded(m, argv[3]);
    else
        CHECK(0);
    cogmaster_destroy(m);
    CHECK_DONE();
}
