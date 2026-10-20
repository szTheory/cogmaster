/* The cartridge image: the size rules, the copier-header strip, the ROM
   header, the board guess and the padding (ARCHITECTURE section 6,
   SMS-CARTRIDGES sections 1 and 2). */
#include <string.h>

#include "internal.h"

#define CART_PAGE 16384u
#define CART_COPIER_HEADER 512u
/* The smallest and largest images accepted: 1 KiB, and 1 MiB with a copier
   header in front. */
#define CART_MIN 1024u
#define CART_MAX (1048576u + CART_COPIER_HEADER)
/* A body of at most 48 KiB decodes linearly with no mapper; the BIOS
   assumes the Sega mapper above it (SMS-CARTRIDGES section 2.1). */
#define CART_NO_MAPPER_MAX 49152u
/* The Game Gear X-Terminator cheat device is an 8 KiB image (section 1.4). */
#define CART_GG_DEVICE 8192u
/* The opcode scan covers the first 32 KiB (section 1.4). */
#define CART_SCAN 32768u
/* Cartridge RAM: two 16 KiB banks on the Sega board, 8 KiB on the
   Codemasters board (section 2). */
#define CART_RAM_SEGA 32768u
#define CART_RAM_CODEMASTERS 8192u

/* The ROM header's 16 bytes, its magic and the places it may sit
   (SMS-CARTRIDGES section 1.1). The magic is the ASCII text the BIOS
   compares. */
#define HEADER_SIZE 16u
static const uint8_t header_magic[8] = {0x54u, 0x4Du, 0x52u, 0x20u, 0x53u, 0x45u, 0x47u, 0x41u};
static const uint16_t header_places[3] = {0x7FF0u, 0x3FF0u, 0x1FF0u};

/* The bytes the checksum covers, by size code; 0 for a code with no range
   (SMS-CARTRIDGES section 1.2). */
static const uint32_t checksum_range[16] = {
    0x40000u, 0x80000u, 0x100000u, 0u,      0u,      0u,      0u,       0u,
    0u,       0u,       0x1FF0u,   0x3FF0u, 0x7FF0u, 0xCFF0u, 0x10000u, 0x20000u,
};

/* A body of n bytes after any copier header: a copier wrote 512 bytes of
   its own in front when n is 512 past a whole number of 16 KiB pages. */
static size_t body_offset(size_t n)
{
    return (n & (CART_PAGE - 1u)) == CART_COPIER_HEADER ? CART_COPIER_HEADER : 0u;
}

static uint32_t le16(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t bcd(uint32_t b)
{
    return (b >> 4) * 10u + (b & 0x0Fu);
}

/* The 16-bit sum of the body's bytes up to the size code's range, clamped to
   the body, skipping the header itself, as the export BIOS computes it
   (SMS-CARTRIDGES section 1.2). */
static uint32_t header_sum(const uint8_t *b, size_t body, uint32_t at, uint32_t range)
{
    size_t end = range < body ? range : body;
    uint32_t sum = 0u;
    for (size_t i = 0; i < end; i++) {
        if (i < at || i >= at + HEADER_SIZE) {
            sum += b[i];
        }
    }
    return sum & 0xFFFFu;
}

/* The first ROM header at $7FF0, $3FF0 or $1FF0 that lies inside the body,
   decoded into p (SMS-CARTRIDGES section 1.1). */
static void read_header(const uint8_t *b, size_t body, struct cogmaster__probe *p)
{
    for (uint32_t i = 0; i < 3u; i++) {
        uint32_t at = header_places[i];
        if (at + HEADER_SIZE > body || memcmp(b + at, header_magic, sizeof header_magic) != 0) {
            continue;
        }
        const uint8_t *h = b + at;
        p->header_at = at;
        p->checksum = le16(h + 10);
        p->product_code = bcd(h[12]) + 100u * bcd(h[13]) + 10000u * (uint32_t)(h[14] >> 4);
        p->version = h[14] & 0x0Fu;
        p->region_code = (uint32_t)h[15] >> 4;
        p->rom_size_code = h[15] & 0x0Fu;
        uint32_t range = checksum_range[p->rom_size_code];
        p->checksum_ok = range != 0u && header_sum(b, body, at, range) == p->checksum;
        /* Region codes 3 and 4 are the Master System, 5 to 7 the Game Gear
           (Japan, export, international). */
        switch (p->region_code) {
        case 3u:
            p->machine_guess = COGMASTER_MACHINE_SMS2;
            p->region_guess = COGMASTER_REGION_JAPAN;
            break;
        case 4u:
            p->machine_guess = COGMASTER_MACHINE_SMS2;
            p->region_guess = COGMASTER_REGION_EXPORT;
            break;
        case 5u:
            p->machine_guess = COGMASTER_MACHINE_GG;
            p->region_guess = COGMASTER_REGION_JAPAN;
            break;
        case 6u:
        case 7u:
            p->machine_guess = COGMASTER_MACHINE_GG;
            p->region_guess = COGMASTER_REGION_EXPORT;
            break;
        default:
            break;
        }
        return;
    }
}

/* The Codemasters header at $7FE0: a page count of 1 to 64 whose low
   nibble is a decimal digit, a BCD year of 93 to 95, a checksum word whose
   complement follows it, and $7FEF zero (SMS-CARTRIDGES section 1.2). */
static int codemasters_header(const uint8_t *b, size_t body)
{
    if (body < 0x8000u) {
        return 0;
    }
    const uint8_t *h = b + 0x7FE0u;
    uint32_t pages = h[0];
    return pages >= 1u && pages <= 64u && (pages & 0x0Fu) <= 9u && h[3] >= 0x93u && h[3] <= 0x95u &&
           ((le16(h + 6) + le16(h + 8)) & 0xFFFFu) == 0u && h[15] == 0u;
}

/* The opcode scan: LD (nn),A ($32 nn) stores over the first 32 KiB. Stores
   to the Sega registers count for the Sega board; stores to another board's
   registers ($0002-$0004 MSX and Zemina, $3FFE 4 PAK, $6000 Janggun, $A000
   Korean) count against it; the Codemasters $8000 family counts for
   neither (SMS-CARTRIDGES section 1.4). */
static int other_board(const uint8_t *b, size_t body)
{
    size_t end = body < CART_SCAN ? body : CART_SCAN;
    uint32_t sega = 0u, other = 0u;
    for (size_t i = 0; i + 2u < end; i++) {
        if (b[i] != 0x32u) {
            continue;
        }
        uint32_t target = le16(b + i + 1u);
        if (target >= 0xFFFCu) {
            sega++;
        } else if ((target >= 0x0002u && target <= 0x0004u) || target == 0x3FFEu ||
                   target == 0x6000u || target == 0xA000u) {
            other++;
        }
    }
    return other > sega;
}

cogmaster_status cogmaster__cart_probe(const void *image, size_t n, struct cogmaster__probe *out)
{
    memset(out, 0, sizeof *out);
    if (image == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    out->image_size = n > UINT32_MAX ? UINT32_MAX : (uint32_t)n;
    if (n < CART_MIN || n > CART_MAX) {
        return COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE;
    }
    size_t off = body_offset(n);
    const uint8_t *b = (const uint8_t *)image + off;
    size_t body = n - off;
    out->body_offset = (uint32_t)off;
    read_header(b, body, out);
    if (codemasters_header(b, body)) {
        out->board = COGMASTER__BOARD_CODEMASTERS;
    } else if (body == CART_GG_DEVICE && out->machine_guess == COGMASTER_MACHINE_GG) {
        out->board = COGMASTER__BOARD_NONE;
    } else if (body <= CART_NO_MAPPER_MAX) {
        out->board = COGMASTER__BOARD_ROM_ONLY;
    } else if (other_board(b, body)) {
        out->board = COGMASTER__BOARD_NONE;
    } else {
        out->board = COGMASTER__BOARD_SEGA;
    }
    return out->board == COGMASTER__BOARD_NONE ? COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD
                                               : COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster__cart_copy(cogmaster *m, const uint8_t *image, size_t n,
                                      const struct cogmaster__probe *p, struct cogmaster__cart *out)
{
    size_t off = p->body_offset;
    size_t body = n - off;
    uint32_t size = (uint32_t)((body + CART_PAGE - 1u) & ~(size_t)(CART_PAGE - 1u));
    uint32_t ram_size = p->board == COGMASTER__BOARD_SEGA          ? CART_RAM_SEGA
                        : p->board == COGMASTER__BOARD_CODEMASTERS ? CART_RAM_CODEMASTERS
                                                                   : 0u;
    uint8_t *rom = (uint8_t *)m->allocator.alloc(m->allocator.user, (size_t)size + ram_size);
    if (rom == NULL) {
        return COGMASTER_STATUS_ERR_NO_MEMORY;
    }
    memcpy(rom, image + off, body);
    memset(rom + body, 0xFF, size - body);
    memset(rom + size, 0, ram_size);
    out->rom = rom;
    out->ram = ram_size != 0u ? rom + size : NULL;
    out->size = size;
    out->pages = size / CART_PAGE;
    out->ram_size = ram_size;
    out->crc32 = cogmaster__crc32(0u, image + off, body);
    out->board = p->board;
    return COGMASTER_STATUS_OK;
}

void cogmaster__cart_install(cogmaster *m, const struct cogmaster__cart *c)
{
    if (m->cart.rom != NULL) {
        m->allocator.free(m->allocator.user, m->cart.rom, (size_t)m->cart.size + m->cart.ram_size);
    }
    m->cart = *c;
}
