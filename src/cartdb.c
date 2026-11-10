#include <stddef.h>
#include <string.h>
#include "cartdb.h"
#include "sha1.h"

#include "cartdb.inc"

/* The row is whole bytes with no padding, so the table is the same size and layout on
   every compiler and word size (little-endian, big-endian and wasm32 alike). */
_Static_assert(sizeof(struct cogmaster__cart_fact) == 36u, "a table row is 36 bytes");
_Static_assert(offsetof(struct cogmaster__cart_fact, sha1) == 0u, "sha1 sits first");
_Static_assert(offsetof(struct cogmaster__cart_fact, bytes) == 20u, "bytes follows the digest");
_Static_assert(offsetof(struct cogmaster__cart_fact, ram_bytes) == 24u, "ram_bytes at 24");
_Static_assert(offsetof(struct cogmaster__cart_fact, board) == 26u, "board at 26");
_Static_assert(offsetof(struct cogmaster__cart_fact, flags) == 35u, "flags is the last byte");
/* Fault prevented: a table that grows past 100,000 bytes on any lane that compiles this file. */
_Static_assert(sizeof cartdb_facts < 100000u, "the hash table stays under 100 KB");

#define SLOT_COUNT (sizeof cartdb_slots / sizeof cartdb_slots[0])
#define FACT_COUNT (sizeof cartdb_facts / sizeof cartdb_facts[0])

/* Binary search over the 20-byte digest; the rows are sorted by digest, so the byte
   order of memcmp is the sort order. */
const struct cogmaster__cart_fact *cogmaster__cartdb_find(const uint8_t sha1[20])
{
    size_t lo = 0, hi = FACT_COUNT;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        int c = memcmp(sha1, cartdb_facts[mid].sha1, 20u);
        if (!c)
            return &cartdb_facts[mid];
        if (c < 0)
            hi = mid;
        else
            lo = mid + 1u;
    }
    return NULL;
}

const struct cogmaster__cart_fact *cogmaster__cartdb_image(const void *p, size_t n)
{
    cogmaster__sha1 state;
    uint8_t digest[20];
    cogmaster__sha1_init(&state);
    cogmaster__sha1_update(&state, p, n);
    cogmaster__sha1_final(&state, digest);
    const struct cogmaster__cart_fact *f = cogmaster__cartdb_find(digest);
    return f && f->bytes == n ? f : NULL;
}

const struct cogmaster__cart_fact *cogmaster__cartdb_row(size_t i)
{
    return i < FACT_COUNT ? &cartdb_facts[i] : NULL;
}

const struct cogmaster__cart_slot *cogmaster__cartdb_slot(uint32_t id)
{
    for (size_t i = 0; i < SLOT_COUNT; i++)
        if (cartdb_slots[i].id == id)
            return &cartdb_slots[i];
    return NULL;
}

const struct cogmaster__cart_slot *cogmaster__cartdb_slot_named(const char *name)
{
    for (size_t i = 0; i < SLOT_COUNT; i++) {
        size_t k = 0;
        while (k < sizeof cartdb_slots[i].name && cartdb_slots[i].name[k] == name[k] && name[k] != 0)
            k++;
        if (k < sizeof cartdb_slots[i].name && cartdb_slots[i].name[k] == name[k])
            return &cartdb_slots[i];
    }
    return NULL;
}
