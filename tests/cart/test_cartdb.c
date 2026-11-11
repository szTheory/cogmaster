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
static const struct cogmaster__cart_fact *find_hex(const char *hex)
{
    uint8_t digest[20];
    for (size_t i = 0; i < 20u; i++) {
        unsigned int v = 0;
        for (size_t j = 0; j < 2u; j++) {
            char c = hex[2u * i + j];
            v = v * 16u + (unsigned int)(c <= '9' ? c - '0' : c - 'a' + 10);
        }
        digest[i] = (uint8_t)v;
    }
    return cogmaster__cartdb_find(digest);
}
/* Whether a row's size fits its board's geometry, so a row on the wrong board
   fails here and not as a bad bank at run time. Korean and 4 PAK page 16 KiB;
   Zemina pages 8 KiB and has more than 64 KiB to page; Nemesis is 128 KiB;
   Hi-Com pages 32 KiB; Korean no-bank is 8 KiB banks up to 48 KiB; no mapper
   is at most 48 KiB. The Sega and Codemasters boards need no 8 KiB multiple:
   17 Game Gear rows on the Sega board measure 514313 to 524282 bytes on the
   pinned data. Every row is at most 1 MiB. */
static int row_fits(const struct cogmaster__cart_fact *row)
{
    uint32_t n = row->bytes;
    if (n > 1048576u)
        return 0;
    switch (row->board) {
    case COGMASTER_BOARD_ROM_ONLY:
        return n <= 49152u;
    case COGMASTER_BOARD_KOREAN:
    case COGMASTER_BOARD_4PAK:
        return n % 16384u == 0u;
    case COGMASTER_BOARD_KOREAN_NB:
        return n % 8192u == 0u && n <= 49152u;
    case COGMASTER_BOARD_HICOM:
        return n % 32768u == 0u;
    case COGMASTER_BOARD_ZEMINA:
        return n % 8192u == 0u && n > 65536u;
    case COGMASTER_BOARD_NEMESIS:
        return n == 131072u;
    default:
        return 1;
    }
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
        CHECK_EQ_U64(p.title[0], 0u);
        CHECK_EQ_U64(p.board_source, COGMASTER_BOARD_SOURCE_DB);
        CHECK_EQ_U64(p.persistence, COGMASTER_PERSISTENCE_BATTERY);
        CHECK_EQ_U64(p.persistent_size, 32768u);
        CHECK_EQ_U64(p.persistence_geometry, COGMASTER_GEOMETRY_SUPPORTED);
        cogmaster_config cfg = {0};
        cfg.size = sizeof cfg;
        cfg.allocator.alloc = allocate;
        cfg.allocator.free = release;
        cogmaster *m = NULL;
        CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
        cogmaster_media media = {0};
        media.size = sizeof media;
        media.image = fixture;
        media.image_size = n;
        CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
        cogmaster_run_result result = {0};
        result.size = sizeof result;
        for (unsigned int frame = 0; frame < 8u; frame++)
            CHECK_EQ_U64(cogmaster_run_frame(m, &result), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xc105), 0u);
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xc112), 0u);
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xc113), 1u);
        CHECK_EQ_U64(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0x8000), 0xa5u);
        cogmaster_destroy(m);
        fixture[32] ^= 1u;
        CHECK_EQ_U64(cogmaster_probe(fixture, n, &p), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(p.identified, 0u);
        CHECK_EQ_U64(p.persistence, COGMASTER_PERSISTENCE_UNKNOWN);
        fixture[32] ^= 1u;
        memmove(fixture + 512, fixture, n);
        memset(fixture, 0, 512);
        CHECK_EQ_U64(cogmaster_probe(fixture, n + 512u, &p), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(p.identified, 1u);
        CHECK_EQ_U64(p.body_offset, 512u);
        CHECK_DONE();
    }
    const struct cogmaster__cart_fact *f = find_hex("0dad260b5357b2f61d1452bb73d8321ca53d7262");
    CHECK(f != NULL);
    if (f) {
        CHECK_EQ_U64(f->persistence, COGMASTER_PERSISTENCE_BATTERY);
        CHECK_EQ_U64(f->ram_bytes, 24576u);
        CHECK_EQ_U64(f->geometry, COGMASTER_GEOMETRY_UNSUPPORTED);
    }
    /* The owned EEPROM fixture, by the hash of the built image. */
    f = find_hex("0170c40f75d8ed3f3bbc9547fa2d9e89192fc3f0");
    CHECK(f != NULL);
    if (f) {
        CHECK_EQ_U64(f->board, COGMASTER_BOARD_SEGA_EEPROM);
        CHECK_EQ_U64(f->persistence, COGMASTER_PERSISTENCE_BATTERY);
        CHECK_EQ_U64(f->ram_bytes, 128u);
        CHECK_EQ_U64(f->geometry, COGMASTER_GEOMETRY_SUPPORTED);
        CHECK_EQ_U64(f->machine, COGMASTER_MACHINE_GG);
    }
    CHECK(find_hex("0000000000000000000000000000000000000000") == NULL);
    f = find_hex("c3305d745792a931ea92da6e6457c43d7cc45cf2");
    CHECK(f != NULL);
    if (f) {
        CHECK_EQ_U64(f->machine, COGMASTER_MACHINE_GG);
        CHECK_EQ_U64(f->video, COGMASTER_VIDEO_SMS);
        CHECK_EQ_U64(f->persistence, COGMASTER_PERSISTENCE_NONE);
        CHECK_EQ_U64(f->flags & 1u, 1u);
    }
    /* The named unsupported rows probe through the real table: board 0, the slot id and a reason.
     */
    static const char *const named[3] = {"seojin", "korean_188in1", "janggun"};
    static const char *const reason[3] = {"$FFF0", "menu entries 6 and 13", "reverse bit"};
    for (size_t k = 0; k < 3u; k++) {
        const struct cogmaster__cart_slot *slot = cogmaster__cartdb_slot_named(named[k]);
        CHECK(slot != NULL);
        const struct cogmaster__cart_fact *row = NULL;
        for (size_t i = 0; slot != NULL && (row = cogmaster__cartdb_row(i)) != NULL; i++)
            if (row->slot == slot->id)
                break;
        CHECK(row != NULL);
        if (slot == NULL || row == NULL)
            continue;
        cogmaster_cart_info named_info = {0};
        named_info.size = sizeof named_info;
        CHECK_EQ_U64(cogmaster__cart_fact_to_info(row, &named_info),
                     COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
        CHECK_EQ_U64(named_info.board, 0u);
        CHECK_EQ_U64(named_info.unsupported_id, slot->id);
        CHECK(strncmp(named_info.unsupported_reason, named[k], strlen(named[k])) == 0);
        CHECK(strstr(named_info.unsupported_reason, reason[k]) != NULL);
        CHECK_EQ_U64(named_info.title[0], 0u);
        CHECK_EQ_U64(named_info.board_source, COGMASTER_BOARD_SOURCE_DB);
    }
    /* The first row of each Korean-family slot probes OK on its board, from the table. */
    static const char *const korean_slot[6] = {"korean", "korean_nb", "hicom",
                                               "zemina", "nemesis",   "4pak"};
    static const uint8_t korean_board[6] = {COGMASTER_BOARD_KOREAN,  COGMASTER_BOARD_KOREAN_NB,
                                            COGMASTER_BOARD_HICOM,   COGMASTER_BOARD_ZEMINA,
                                            COGMASTER_BOARD_NEMESIS, COGMASTER_BOARD_4PAK};
    for (size_t k = 0; k < 6u; k++) {
        const struct cogmaster__cart_slot *slot = cogmaster__cartdb_slot_named(korean_slot[k]);
        CHECK(slot != NULL);
        const struct cogmaster__cart_fact *row = NULL;
        for (size_t i = 0; slot != NULL && (row = cogmaster__cartdb_row(i)) != NULL; i++)
            if (row->slot == slot->id)
                break;
        CHECK(row != NULL);
        if (slot == NULL || row == NULL)
            continue;
        cogmaster_cart_info korean_info = {0};
        korean_info.size = sizeof korean_info;
        CHECK_EQ_U64(cogmaster__cart_fact_to_info(row, &korean_info), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(korean_info.board, korean_board[k]);
        CHECK_EQ_U64(korean_info.board_source, COGMASTER_BOARD_SOURCE_DB);
    }
    /* The first row of the eeprom slot probes as the EEPROM board with its 128-byte save. */
    {
        const struct cogmaster__cart_slot *slot = cogmaster__cartdb_slot_named("eeprom");
        const struct cogmaster__cart_fact *row = NULL;
        CHECK(slot != NULL);
        for (size_t i = 0; slot != NULL && (row = cogmaster__cartdb_row(i)) != NULL; i++)
            if (row->slot == slot->id)
                break;
        CHECK(row != NULL);
        if (row != NULL) {
            cogmaster_cart_info eeprom_info = {0};
            eeprom_info.size = sizeof eeprom_info;
            CHECK_EQ_U64(cogmaster__cart_fact_to_info(row, &eeprom_info), COGMASTER_STATUS_OK);
            CHECK_EQ_U64(eeprom_info.board, COGMASTER_BOARD_SEGA_EEPROM);
            CHECK_EQ_U64(eeprom_info.persistence, COGMASTER_PERSISTENCE_BATTERY);
            CHECK_EQ_U64(eeprom_info.persistent_size, 128u);
            CHECK_EQ_U64(eeprom_info.persistence_geometry, COGMASTER_GEOMETRY_SUPPORTED);
        }
    }
    /* A slot is a board or a reason, never both. */
    for (uint32_t id = 1; id <= 20u; id++) {
        const struct cogmaster__cart_slot *slot = cogmaster__cartdb_slot(id);
        CHECK(slot != NULL);
        if (slot != NULL)
            CHECK_EQ_U64(slot->board == 0u, slot->reason[0] != 0);
    }
    CHECK(cogmaster__cartdb_slot(0) == NULL);
    CHECK(cogmaster__cartdb_slot_named("zz_new") == NULL);
    /* Every supported board owns at least one row, so a slot-to-board slip fails here. */
    static const uint8_t supported[10] = {COGMASTER_BOARD_ROM_ONLY,    COGMASTER_BOARD_SEGA,
                                          COGMASTER_BOARD_CODEMASTERS, COGMASTER_BOARD_KOREAN,
                                          COGMASTER_BOARD_KOREAN_NB,   COGMASTER_BOARD_HICOM,
                                          COGMASTER_BOARD_ZEMINA,      COGMASTER_BOARD_NEMESIS,
                                          COGMASTER_BOARD_4PAK,        COGMASTER_BOARD_SEGA_EEPROM};
    uint32_t per_board[10] = {0};
    uint32_t per_persistence[4] = {0, 0, 0, 0};
    uint32_t per_ctrl1[5] = {0}, per_ctrl2[5] = {0};
    for (size_t i = 0;; i++) {
        const struct cogmaster__cart_fact *row = cogmaster__cartdb_row(i);
        if (row == NULL)
            break;
        for (size_t b = 0; b < 10u; b++)
            per_board[b] += row->board == supported[b];
        if (!row_fits(row)) {
            fprintf(stderr, "row %lu: slot %u board %u size %lu does not fit\n", (unsigned long)i,
                    (unsigned)row->slot, (unsigned)row->board, (unsigned long)row->bytes);
            check_failures++;
        }
        CHECK(row->persistence < 4u && row->ctrl1 < 5u && row->ctrl2 < 5u);
        if (row->persistence < 4u && row->ctrl1 < 5u && row->ctrl2 < 5u) {
            per_persistence[row->persistence]++;
            per_ctrl1[row->ctrl1]++;
            per_ctrl2[row->ctrl2]++;
        }
    }
    for (size_t b = 0; b < 10u; b++)
        CHECK(per_board[b] > 0u);
    /* Row counts from the pinned lists: EEPROM, battery, and the controller defaults. */
    CHECK_EQ_U64(per_persistence[2], 71u);
    CHECK_EQ_U64(per_persistence[3], 0u);
    CHECK_EQ_U64(per_ctrl1[0], 1637u);
    CHECK_EQ_U64(per_ctrl1[1], 13u);
    CHECK_EQ_U64(per_ctrl1[2], 5u);
    CHECK_EQ_U64(per_ctrl1[3], 4u);
    CHECK_EQ_U64(per_ctrl1[4], 1u);
    CHECK_EQ_U64(per_ctrl2[0], 1654u);
    CHECK_EQ_U64(per_ctrl2[1], 1u);
    CHECK_EQ_U64(per_ctrl2[2], 1u);
    CHECK_EQ_U64(per_ctrl2[3], 4u);
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
