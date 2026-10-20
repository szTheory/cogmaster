#ifndef COGMASTER_CARTDB_H
#define COGMASTER_CARTDB_H
#include <stddef.h>
#include <stdint.h>
struct cogmaster__cart_fact {
    char sha1[41], title[256];
    uint32_t bytes, board, machine, region, video, persistence, declared, geometry;
    uint32_t public_identity;
};
const struct cogmaster__cart_fact *cogmaster__cartdb_find(const char *sha1);
const struct cogmaster__cart_fact *cogmaster__cartdb_image(const void *p, size_t n);
#endif
