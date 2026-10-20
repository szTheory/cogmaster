#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "../check.h"

static uint8_t image[65536], saved[400000], after[400000];
static uint8_t raw[32769];
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
    /* A persistent extent must never replace physical address decoding. */
    classified(m, COGMASTER_BOARD_SEGA, COGMASTER_PERSISTENCE_BATTERY, 32768u,
               COGMASTER_GEOMETRY_SUPPORTED);
    m->cart.persistent_size = 8192u;
    cogmaster_battery b = battery(m);
    CHECK_EQ_U64(b.length, 8192u);
    cogmaster__bus_write(m, 0xfffcu, 8u);
    cogmaster__bus_write(m, 0xa000u, 0x77u);
    cogmaster__bus_write(m, 0xfffcu, 12u);
    cogmaster__bus_write(m, 0x8000u, 0x88u);
    CHECK_EQ_HEX(m->cart.ram[8192], 0x77u);
    CHECK_EQ_HEX(m->cart.ram[16384], 0x88u);
    CHECK_EQ_HEX(b.data[0], 0xffu);
    CHECK_EQ_U64(battery(m).generation, 0u);
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

static cogmaster_status import_save(cogmaster *m, const void *data, size_t n)
{
    return cogmaster_import_battery(m, data, n);
}
static void rejected(cogmaster *m, const void *data, size_t n, cogmaster_status expected)
{
    size_t state_n = cogmaster__state_size(m);
    CHECK(state_n <= sizeof saved);
    if (state_n > sizeof saved)
        return;
    CHECK_EQ_U64(cogmaster__state_save(m, saved, sizeof saved), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(import_save(m, data, n), expected);
    CHECK_EQ_U64(cogmaster__state_save(m, after, sizeof after), COGMASTER_STATUS_OK);
    CHECK(memcmp(saved, after, state_n) == 0);
}
static void imports(cogmaster *m)
{
    static const uint32_t sizes[] = {8192u, 16384u, 32768u};
    memset(raw, 0x3c, sizeof raw);
    for (uint32_t i = 0; i < 3u; i++) {
        uint32_t n = sizes[i];
        classified(m, COGMASTER_BOARD_SEGA, COGMASTER_PERSISTENCE_BATTERY, n,
                   COGMASTER_GEOMETRY_SUPPORTED);
        cogmaster__bus_write(m, 0xfffcu, 8u);
        cogmaster__bus_write(m, 0x8000u, 0x12u);
        cogmaster_battery b = battery(m);
        size_t lengths[] = {0u, 1u, n - 1u, n};
        for (uint32_t j = 0; j < 4u; j++) {
            size_t length = lengths[j];
            memset(b.data, 0, n);
            cogmaster_status st = import_save(m, length == 0u ? NULL : raw, length);
            CHECK_EQ_U64(st, COGMASTER_STATUS_OK);
            if (st != COGMASTER_STATUS_OK)
                continue;
            for (uint32_t k = 0; k < n; k++)
                CHECK_EQ_HEX(b.data[k], k < length ? 0x3cu : 0xffu);
            CHECK_EQ_U64(battery(m).generation, 1u);
            size_t state_n = cogmaster__state_size(m);
            CHECK_EQ_U64(cogmaster__state_save(m, saved, sizeof saved), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(import_save(m, length == 0u ? NULL : raw, length), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(cogmaster__state_save(m, after, sizeof after), COGMASTER_STATUS_OK);
            CHECK(memcmp(saved, after, state_n) == 0);
        }
        rejected(m, raw, n + 1u, COGMASTER_STATUS_ERR_SAVE_SIZE);
        rejected(m, raw, SIZE_MAX, COGMASTER_STATUS_ERR_SAVE_SIZE);
        rejected(m, NULL, 1u, COGMASTER_STATUS_ERR_ARGUMENT);
        /* A frontend may import from the already exposed span, including an overlap. */
        CHECK_EQ_U64(import_save(m, b.data + 1u, n - 1u), COGMASTER_STATUS_OK);
        CHECK_EQ_HEX(b.data[n - 1u], 0xffu);
    }
    classified(m, COGMASTER_BOARD_SEGA, COGMASTER_PERSISTENCE_BATTERY, 32768u,
               COGMASTER_GEOMETRY_SUPPORTED);
    m->cart.persistent_size = 8192u;
    memset(m->cart.ram, 0x77, m->cart.ram_size);
    CHECK_EQ_U64(import_save(m, raw, 1u), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(m->cart.ram[0], 0x3cu);
    CHECK_EQ_HEX(m->cart.ram[8191], 0xffu);
    CHECK_EQ_HEX(m->cart.ram[8192], 0x77u);
    CHECK_EQ_HEX(m->cart.ram[32767], 0x77u);
    CHECK_EQ_U64(battery(m).generation, 0u);
    for (uint32_t knowledge = 0; knowledge < 3u; knowledge++) {
        classified(m, COGMASTER_BOARD_SEGA, knowledge, knowledge == 2u ? 24576u : 0u,
                   knowledge == 2u ? COGMASTER_GEOMETRY_UNSUPPORTED : COGMASTER_GEOMETRY_NONE);
        rejected(m, raw, 0u,
                 knowledge == 1u ? COGMASTER_STATUS_ERR_NO_CONTENT
                                 : COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
        rejected(m, raw, SIZE_MAX,
                 knowledge == 1u ? COGMASTER_STATUS_ERR_NO_CONTENT
                                 : COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
    }
}
static void continuation(cogmaster *m, const char *path)
{
    fixture(m, path);
    memset(raw, 0xff, sizeof raw);
    raw[0] = 0xa5u;
    raw[1] = 0x5au;
    const uint32_t profiles[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t i = 0; i < sizeof profiles / sizeof profiles[0]; i++) {
        cogmaster_media media = {0};
        media.size = sizeof media;
        media.machine = profiles[i];
        media.video_mode =
            profiles[i] == COGMASTER_MACHINE_GG ? COGMASTER_VIDEO_GG : COGMASTER_VIDEO_SMS;
        media.image = image;
        media.image_size = sizeof image;
        CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
        cogmaster_battery b = battery(m);
        CHECK_EQ_U64(b.generation, 0u);
        /* Frontend-style direct restoration is also generation-neutral. */
        memcpy(b.data, raw, b.length);
        CHECK_EQ_U64(battery(m).generation, 0u);
        for (uint32_t reset = 0; reset < 2u; reset++) {
            uint32_t previous = reset == COGMASTER_RESET_SOFT ? 7u : 19u;
            raw[2] = (uint8_t)previous;
            uint64_t generation = battery(m).generation;
            CHECK_EQ_U64(import_save(m, raw, 32768u), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(import_save(m, raw, 32768u), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(battery(m).generation, generation);
            CHECK_EQ_U64(cogmaster_reset(m, reset), COGMASTER_STATUS_OK);
            cogmaster_battery again = battery(m);
            CHECK(again.data == b.data);
            CHECK_EQ_U64(again.length, b.length);
            CHECK_EQ_U64(again.generation, generation);
            CHECK(memcmp(again.data, raw, b.length) == 0);
            for (uint32_t frame = 0; frame < 8u; frame++)
                CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xc112u), previous);
            CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xc113u), previous + 1u);
            CHECK(battery(m).data == b.data);
            CHECK_EQ_U64(battery(m).generation, generation + 1u);
        }
    }
}

int main(int argc, char **argv)
{
    cogmaster *m = machine();
    CHECK_EQ_U64(cogmaster_import_battery(m, NULL, 0u), COGMASTER_STATUS_ERR_NO_CONTENT);
    if (argc == 3 && strcmp(argv[1], "continuation") == 0)
        continuation(m, argv[2]);
    else if (argc == 3)
        fixture(m, argv[2]);
    else if (argc == 2 && strcmp(argv[1], "import") == 0)
        imports(m);
    else if (argc == 2 && strcmp(argv[1], "spans") == 0)
        spans(m);
    else if (argc == 2 && strcmp(argv[1], "excluded") == 0)
        excluded(m);
    else
        generations(m);

    cogmaster_destroy(m);
    CHECK_DONE();
}
