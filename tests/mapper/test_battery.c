#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "../check.h"

static uint8_t image[65536], saved[400000], after[400000];
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
static cogmaster *machine(void)
{
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL)
        exit(1);
    return m;
}
/* Internal unit setup exercises the actual allocator and physical mapper;
   the owned image below separately proves the public classification path. */
static void classified(cogmaster *m, uint32_t board, uint32_t knowledge, uint32_t n,
                       uint32_t geometry)
{
    cogmaster_media media = {0};
    media.size = sizeof media;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    cogmaster_cart_info p = {0};
    p.board = board;
    p.persistence = knowledge;
    p.persistent_size = n;
    p.persistence_geometry = geometry;
    struct cogmaster__cart c = {0};
    CHECK_EQ_U64(cogmaster__cart_copy(m, image, sizeof image, &p, &c), COGMASTER_STATUS_OK);
    cogmaster__cart_install(m, &c);
    cogmaster__mapper_reset(m);
}

static cogmaster_battery battery(cogmaster *m)
{
    cogmaster_battery b = {0};
    b.size = sizeof b;
    CHECK_EQ_U64(cogmaster_get_battery(m, &b), COGMASTER_STATUS_OK);
    return b;
}
static void spans(cogmaster *m)
{
    static const uint32_t sizes[] = {8192u, 16384u, 32768u};
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        uint32_t n = sizes[i];
        classified(m, COGMASTER_BOARD_SEGA, COGMASTER_PERSISTENCE_BATTERY, n,
                   COGMASTER_GEOMETRY_SUPPORTED);
        cogmaster_battery b = battery(m);
        CHECK_EQ_U64(b.knowledge, COGMASTER_BATTERY_KNOWN);
        CHECK_EQ_U64(b.length, n);
        CHECK(b.data != NULL);
        if (b.data == NULL)
            continue;
        for (uint32_t j = 0; j < n; j++)
            CHECK_EQ_HEX(b.data[j], 0xffu);
        cogmaster__bus_write(m, 0xfffcu, 8u);
        cogmaster__bus_write(m, 0x8003u, 0x11u);
        cogmaster__bus_write(m, 0xa003u, 0x22u);
        cogmaster__bus_write(m, 0xfffcu, 12u);
        cogmaster__bus_write(m, 0x8003u, 0x33u);
        CHECK_EQ_HEX(b.data[3], n == 32768u ? 0x11u : 0x33u);
        CHECK_EQ_HEX(b.data[n == 8192u ? 3u : 8195u], n == 8192u ? 0x33u : 0x22u);
        if (n == 32768u)
            CHECK_EQ_HEX(b.data[16387], 0x33u);
        for (uint32_t reset = 0; reset < 2u; reset++) {
            CHECK_EQ_U64(cogmaster_reset(m, reset), COGMASTER_STATUS_OK);
            cogmaster_battery again = battery(m);
            CHECK(again.data == b.data);
            CHECK_EQ_U64(again.length, n);
            CHECK_EQ_U64(again.generation, 3u);
            CHECK_EQ_HEX(again.data[3], n == 32768u ? 0x11u : 0x33u);
        }
    }
}
static void generations(cogmaster *m)
{
    classified(m, COGMASTER_BOARD_SEGA, COGMASTER_PERSISTENCE_BATTERY, 32768u,
               COGMASTER_GEOMETRY_SUPPORTED);
    cogmaster__bus_write(m, 0xfffcu, 8u);
    cogmaster__bus_write(m, 0x8000u, 0xffu);
    cogmaster__bus_write(m, 0x8000u, 0xffu);
    CHECK_EQ_U64(battery(m).generation, 2u);
    (void)cogmaster__bus_read(m, 0x8000u);
    cogmaster__bus_write(m, 0xfffdu, 1u);
    cogmaster__bus_write(m, 0xc001u, 1u);
    cogmaster__io_out(m, 0x3eu, 0xebu);
    cogmaster__bus_write(m, 0x8000u, 3u);
    CHECK_EQ_U64(battery(m).generation, 2u);
    size_t n = cogmaster__state_size(m);
    CHECK(n < sizeof saved);
    CHECK_EQ_U64(cogmaster__state_save(m, saved, sizeof saved), COGMASTER_STATUS_OK);
    cogmaster__io_out(m, 0x3eu, 0xabu);
    cogmaster__bus_write(m, 0x8000u, 4u);
    CHECK_EQ_U64(battery(m).generation, 3u);
    CHECK_EQ_U64(cogmaster__state_load(m, saved, n), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(battery(m).generation, 2u);
    cogmaster_battery b = battery(m);
    b.size = sizeof b - 1u;
    CHECK_EQ_U64(cogmaster_get_battery(m, &b), COGMASTER_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(cogmaster__state_save(m, after, sizeof after), COGMASTER_STATUS_OK);
    CHECK(memcmp(saved, after, n) == 0);
}
static void excluded(cogmaster *m)
{
    static const uint32_t knowledge[] = {COGMASTER_PERSISTENCE_UNKNOWN, COGMASTER_PERSISTENCE_NONE,
                                         COGMASTER_PERSISTENCE_BATTERY};
    static const uint32_t expected[] = {COGMASTER_BATTERY_UNKNOWN, COGMASTER_BATTERY_NONE,
                                        COGMASTER_BATTERY_UNSUPPORTED};
    for (uint32_t i = 0; i < 3u; i++) {
        classified(m, COGMASTER_BOARD_SEGA, knowledge[i], i == 2u ? 24576u : 0u,
                   i == 2u ? COGMASTER_GEOMETRY_UNSUPPORTED : COGMASTER_GEOMETRY_NONE);
        cogmaster_battery b = battery(m);
        CHECK_EQ_U64(b.knowledge, expected[i]);
        CHECK(b.data == NULL);
        CHECK_EQ_U64(b.length, 0u);
        cogmaster__bus_write(m, 0xfffcu, 8u);
        cogmaster__bus_write(m, 0x8000u, 1u);
        CHECK_EQ_U64(battery(m).generation, 0u);
    }
    classified(m, COGMASTER_BOARD_CODEMASTERS, COGMASTER_PERSISTENCE_BATTERY, 8192u,
               COGMASTER_GEOMETRY_SUPPORTED);
    cogmaster__bus_write(m, 0x4000u, 0x80u);
    cogmaster__bus_write(m, 0xa001u, 0xffu);
    cogmaster__bus_write(m, 0xbfffu, 0x55u);
    CHECK_EQ_U64(battery(m).generation, 2u);
    CHECK_EQ_HEX(battery(m).data[8191], 0x55u);
    classified(m, COGMASTER_BOARD_CODEMASTERS, COGMASTER_PERSISTENCE_NONE, 0u,
               COGMASTER_GEOMETRY_NONE);
    cogmaster__bus_write(m, 0x4000u, 0x80u);
    cogmaster__bus_write(m, 0xa001u, 1u);
    CHECK_EQ_U64(battery(m).generation, 0u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xa001u), 1u);
}
static void fixture(cogmaster *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    if (f == NULL)
        return;
    CHECK_EQ_U64(fread(image, 1, sizeof image, f), sizeof image);
    CHECK_EQ_U64(fclose(f), 0u);
    cogmaster_media media = {0};
    media.size = sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    cogmaster_battery b = battery(m);
    CHECK_EQ_U64(b.length, 32768u);
    CHECK_EQ_U64(b.knowledge, COGMASTER_BATTERY_KNOWN);
    for (uint32_t i = 0; i < 8u; i++)
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK(battery(m).data == b.data);
    CHECK_EQ_HEX(b.data[0], 0xa5u);
    CHECK_EQ_HEX(b.data[1], 0x5au);
    CHECK_EQ_U64(battery(m).generation, 3u);
}

int main(int argc, char **argv)
{
    cogmaster *m = machine();
    if (argc == 3)
        fixture(m, argv[2]);
    else if (argc == 2 && strcmp(argv[1], "spans") == 0)
        spans(m);
    else if (argc == 2 && strcmp(argv[1], "excluded") == 0)
        excluded(m);
    else
        generations(m);

    cogmaster_destroy(m);
    CHECK_DONE();
}
