#include <string.h>
#include "cartdb.h"
#include "sha1.h"

static const struct cogmaster__cart_fact facts[] = {
#include "cartdb.inc"
};

const struct cogmaster__cart_fact *cogmaster__cartdb_find(const char *sha1)
{
    size_t lo = 0, hi = sizeof facts / sizeof facts[0];
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        int c = memcmp(sha1, facts[mid].sha1, 40u);
        if (!c)
            return &facts[mid];
        if (c < 0)
            hi = mid;
        else
            lo = mid + 1u;
    }
    return NULL;
}

const struct cogmaster__cart_fact *cogmaster__cartdb_image(const void *p, size_t n)
{
    static const char digits[] = "0123456789abcdef";
    cogmaster__sha1 state;
    uint8_t digest[20];
    char hex[41];
    cogmaster__sha1_init(&state);
    cogmaster__sha1_update(&state, p, n);
    cogmaster__sha1_final(&state, digest);
    for (size_t i = 0; i < 20u; i++) {
        hex[2u * i] = digits[digest[i] >> 4];
        hex[2u * i + 1u] = digits[digest[i] & 15u];
    }
    hex[40] = 0;
    const struct cogmaster__cart_fact *f = cogmaster__cartdb_find(hex);
    return f && f->bytes == n ? f : NULL;
}
