/* The cartridge image: the size rules, the copier-header strip and the
   padding (ARCHITECTURE section 6). */
#include <string.h>

#include "internal.h"

#define CART_PAGE 16384u
#define CART_COPIER_HEADER 512u
/* The smallest and largest images accepted: 1 KiB, and 1 MiB with a copier
   header in front. */
#define CART_MIN 1024u
#define CART_MAX (1048576u + CART_COPIER_HEADER)

/* A body of n bytes after any copier header: a copier wrote 512 bytes of
   its own in front when n is 512 past a whole number of 16 KiB pages. */
static size_t body_offset(size_t n)
{
    return (n & (CART_PAGE - 1u)) == CART_COPIER_HEADER ? CART_COPIER_HEADER : 0u;
}

cogmaster_status cogmaster__cart_check(const void *image, size_t n)
{
    if (image == NULL || n < CART_MIN || n > CART_MAX) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster__cart_copy(cogmaster *m, const uint8_t *image, size_t n,
                                      struct cogmaster__cart *out)
{
    size_t off = body_offset(n);
    size_t body = n - off;
    uint32_t size = (uint32_t)((body + CART_PAGE - 1u) & ~(size_t)(CART_PAGE - 1u));
    uint8_t *rom = (uint8_t *)m->allocator.alloc(m->allocator.user, size);
    if (rom == NULL) {
        return COGMASTER_STATUS_ERR_NO_MEMORY;
    }
    memcpy(rom, image + off, body);
    memset(rom + body, 0xFF, size - body);
    out->rom = rom;
    out->size = size;
    out->crc32 = cogmaster__crc32(0u, image + off, body);
    return COGMASTER_STATUS_OK;
}

void cogmaster__cart_install(cogmaster *m, const struct cogmaster__cart *c)
{
    if (m->cart.rom != NULL) {
        m->allocator.free(m->allocator.user, m->cart.rom, m->cart.size);
    }
    m->cart = *c;
}
