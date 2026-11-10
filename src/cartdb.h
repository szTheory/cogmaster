#ifndef COGMASTER_CARTDB_H
#define COGMASTER_CARTDB_H
#include <stddef.h>
#include <stdint.h>
#include "cogmaster.h"
/* One whole-image identity fact: 36 bytes, no padding, no pointer, no title. */
struct cogmaster__cart_fact {
    uint8_t sha1[20];
    uint32_t bytes;
    uint16_t ram_bytes;
    uint8_t board, slot, machine, region, video, persistence, geometry, ctrl1, ctrl2, flags;
};
/* One cartridge slot: a board constant, or board 0 with the reason it is unsupported. */
struct cogmaster__cart_slot {
    uint8_t id, board;
    char name[16];
    char reason[96];
};
const struct cogmaster__cart_fact *cogmaster__cartdb_find(const uint8_t sha1[20]);
const struct cogmaster__cart_fact *cogmaster__cartdb_image(const void *p, size_t n);
const struct cogmaster__cart_fact *cogmaster__cartdb_row(size_t i);
const struct cogmaster__cart_slot *cogmaster__cartdb_slot(uint32_t id);
const struct cogmaster__cart_slot *cogmaster__cartdb_slot_named(const char *name);
cogmaster_status cogmaster__cart_fact_to_info(const struct cogmaster__cart_fact *fact,
                                              cogmaster_cart_info *out);
#endif
