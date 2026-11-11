/* The cartridge image: the size rules, the copier-header strip, the ROM
   header, the board guess and the padding (ARCHITECTURE section 6,
   SMS-CARTRIDGES sections 1 and 2). */
#include <string.h>

#include "internal.h"
#include "cartdb.h"

#define CART_PAGE 16384u
#define CART_COPIER_HEADER 512u
/* The smallest and largest images accepted: 1 KiB, and 1 MiB with a copier
   header in front. The body after the strip is at most 1 MiB, the most the
   largest Sega-mapper board addresses (SMS-CARTRIDGES section 1). */
#define CART_MIN 1024u
#define CART_BODY_MAX 1048576u
#define CART_MAX (CART_BODY_MAX + CART_COPIER_HEADER)
/* A body of at most 48 KiB decodes linearly with no mapper; the BIOS
   assumes the Sega mapper above it (SMS-CARTRIDGES section 2.1). */
#define CART_NO_MAPPER_MAX 49152u
/* The Game Gear X-Terminator cheat device is an 8 KiB image (section 1.4). */
#define CART_GG_DEVICE 8192u
/* The 8 KiB bank of the boards that page by bank. */
#define CART_BANK 8192u
/* Cartridge RAM: two 16 KiB banks on the Sega board, 8 KiB on the
   Codemasters board (section 2). */
#define CART_RAM_SEGA 32768u
#define CART_RAM_CODEMASTERS 8192u
#define CART_RAM_EEPROM 128u

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
static void read_header(const uint8_t *b, size_t body, cogmaster_cart_info *p)
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

/* Whether a header read by read_header selects a board: the magic found, a
   region code of 3 to 7, a matching checksum and a checksum range that lies
   inside the body (SMS-CARTRIDGES section 1.2). A bad checksum argues for
   nothing. */
static int header_valid(const cogmaster_cart_info *p, size_t body)
{
    return p->header_at != 0u && p->region_code >= 3u && p->region_code <= 7u && p->checksum_ok &&
           checksum_range[p->rom_size_code] <= body;
}

/* Names the unsupported slot in out: its id and "name: reason". */
static void name_unsupported(const struct cogmaster__cart_slot *slot, cogmaster_cart_info *out)
{
    size_t k = 0u;
    if (slot == NULL || slot->reason[0] == 0) {
        return;
    }
    out->unsupported_id = slot->id;
    for (size_t i = 0u;
         i < sizeof slot->name && slot->name[i] != 0 && k < sizeof out->unsupported_reason - 1u;
         i++)
        out->unsupported_reason[k++] = slot->name[i];
    if (k < sizeof out->unsupported_reason - 2u) {
        out->unsupported_reason[k++] = ':';
        out->unsupported_reason[k++] = ' ';
    }
    for (size_t i = 0u;
         i < sizeof slot->reason && slot->reason[i] != 0 && k < sizeof out->unsupported_reason - 1u;
         i++)
        out->unsupported_reason[k++] = slot->reason[i];
    out->unsupported_reason[k] = 0;
}

/* The board a family of store sites points at; Janggun is no board. */
static const uint8_t family_board[COGMASTER__SCAN_FAMILIES] = {
    COGMASTER__BOARD_SEGA, COGMASTER__BOARD_KOREAN, COGMASTER__BOARD_ZEMINA,
    COGMASTER__BOARD_4PAK, COGMASTER__BOARD_NONE,
};

/* Whether the image size fits the family's board: Korean and 4 PAK page
   16 KiB, Zemina and Janggun page 8 KiB, and Zemina has more than 64 KiB to
   page (MAME src/devices/bus/sms_cart/rom.cpp at the pinned commit). */
static int geometry_fits(uint32_t family, size_t body)
{
    switch (family) {
    case COGMASTER__SCAN_KOREAN:
    case COGMASTER__SCAN_4PAK:
        return (body & (CART_PAGE - 1u)) == 0u;
    case COGMASTER__SCAN_ZEMINA:
        return (body & (CART_BANK - 1u)) == 0u && body > 65536u;
    default:
        return (body & (CART_BANK - 1u)) == 0u;
    }
}

/* The scan decides what no database row, header or size settled. F is the
   family with the most store sites (Korean, Zemina, 4 PAK, Janggun in that
   order break a tie), x its count, y the runner-up's and s the Sega count. F
   wins with at least 3 sites, at least 2s + 2 over the Sega count, at least
   2y + 2 over the runner-up, a fitting geometry and no Game Gear header. The
   Sega board wins by the same margin over F. Anything between refuses naming
   F: a doubtful image never plays. Three sites is a conservative start
   until measured on real dumps. */
static cogmaster_status scan_board(const uint8_t *b, size_t body, cogmaster_cart_info *out)
{
    struct cogmaster__scan sc;
    uint32_t f = COGMASTER__SCAN_KOREAN, x = 0u, y = 0u;
    cogmaster__cart_scan(b, body, &sc);
    for (uint32_t k = COGMASTER__SCAN_KOREAN; k < COGMASTER__SCAN_FAMILIES; k++) {
        if (sc.count[k] > x) {
            y = x;
            x = sc.count[k];
            f = k;
        } else if (sc.count[k] > y) {
            y = sc.count[k];
        }
    }
    uint32_t s = sc.count[COGMASTER__SCAN_SEGA];
    out->scan_sega = s;
    out->scan_candidate = x;
    out->scan_runner_up = y;
    if (x == 0u) {
        out->board = COGMASTER__BOARD_SEGA;
        out->board_source = COGMASTER_BOARD_SOURCE_DEFAULT;
        return COGMASTER_STATUS_OK;
    }
    out->board_source = COGMASTER_BOARD_SOURCE_HEURISTIC;
    out->board_candidate = family_board[f];
    if (x >= 3u && x >= 2u * s + 2u && x >= 2u * y + 2u && geometry_fits(f, body) &&
        !(out->region_code >= 5u && out->region_code <= 7u)) {
        if (f == COGMASTER__SCAN_JANGGUN) {
            name_unsupported(cogmaster__cartdb_slot_named("janggun"), out);
        } else {
            out->board = family_board[f];
            /* Nemesis is a 128 KiB Zemina-family image with $00 $00 $00 at 0 and
               DI; IM 1 (F3 ED 56) at 120 KiB, the start of its last 8 KiB bank
               (SMS-CARTRIDGES-AND-PERIPHERALS.md section 2, scan step 5; the
               board itself is sega8_nemesis_device, MAME rom.cpp). One byte of
               that signature would also match an ordinary Zemina image. */
            const uint8_t *last = b + (body - CART_BANK);
            if (f == COGMASTER__SCAN_ZEMINA && body == 131072u && last[0] == 0xF3u &&
                last[1] == 0xEDu && last[2] == 0x56u && b[0] == 0u && b[1] == 0u && b[2] == 0u) {
                out->board = COGMASTER__BOARD_NEMESIS;
                out->board_candidate = COGMASTER__BOARD_NEMESIS;
            }
            return COGMASTER_STATUS_OK;
        }
    } else if (s >= 2u * x + 2u) {
        out->board = COGMASTER__BOARD_SEGA;
        return COGMASTER_STATUS_OK;
    }
    return COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD;
}

/* The board of an image the database does not know, in order: a Codemasters
   header, the 8 KiB Game Gear cheat device (refused), a valid ROM header,
   a body of at most 48 KiB, then the scan. board_source says which step
   decided. */
static cogmaster_status guess_board(const uint8_t *b, size_t body, cogmaster_cart_info *out)
{
    int small = body <= CART_NO_MAPPER_MAX;
    if (codemasters_header(b, body)) {
        out->board = COGMASTER__BOARD_CODEMASTERS;
        out->board_source = COGMASTER_BOARD_SOURCE_HEADER;
    } else if (body == CART_GG_DEVICE && out->machine_guess == COGMASTER_MACHINE_GG) {
        out->board_source = COGMASTER_BOARD_SOURCE_DEFAULT;
        return COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD;
    } else if (header_valid(out, body)) {
        out->board = small ? COGMASTER__BOARD_ROM_ONLY : COGMASTER__BOARD_SEGA;
        out->board_source = COGMASTER_BOARD_SOURCE_HEADER;
    } else if (small) {
        out->board = COGMASTER__BOARD_ROM_ONLY;
        out->board_source = COGMASTER_BOARD_SOURCE_DEFAULT;
    } else {
        return scan_board(b, body, out);
    }
    return COGMASTER_STATUS_OK;
}

/* The table's controller codes: 1 is a light phaser, 2 a paddle and 3 a sports pad, which are the
   device numbers of the same names. Code 4 (graphic board) and any other read as a plain pad until
   a later release models them. A light phaser row names no region or machine of its own, so it
   leaves region_guess and machine_guess alone. */
#define CART_CTRL_LIGHT_PHASER 1u
#define CART_CTRL_PADDLE 2u
#define CART_CTRL_SPORTS_PAD 3u

static uint32_t ctrl_device(uint8_t code)
{
    if (code == CART_CTRL_LIGHT_PHASER)
        return COGMASTER_DEVICE_LIGHT_PHASER;
    if (code == CART_CTRL_PADDLE)
        return COGMASTER_DEVICE_PADDLE;
    if (code == CART_CTRL_SPORTS_PAD)
        return COGMASTER_DEVICE_SPORTS_PAD;
    return COGMASTER_DEVICE_PAD;
}

/* Copies one database row into the probe result. A row of board 0 is unsupported by
   design and names its slot and reason. Titles live in tests/compat/catalog.tsv, not
   in the table. A persistence value above BATTERY is not public yet and reads as unknown.
   A Japanese paddle cartridge is only ever used on a Japanese console (SMS Power, Paddle Control),
   so a paddle row with no region of its own reads as Japan; a row that names a region keeps it. */
cogmaster_status cogmaster__cart_fact_to_info(const struct cogmaster__cart_fact *fact,
                                              cogmaster_cart_info *out)
{
    static const char digits[] = "0123456789abcdef";
    out->identified = 1u;
    out->public_identity = fact->flags & 1u;
    out->board = fact->board;
    out->board_source = COGMASTER_BOARD_SOURCE_DB;
    out->machine_guess = fact->machine;
    uint32_t paddle = fact->ctrl1 == CART_CTRL_PADDLE || fact->ctrl2 == CART_CTRL_PADDLE;
    if (fact->region != 0u)
        out->region_guess = fact->region;
    else if (paddle)
        out->region_guess = COGMASTER_REGION_JAPAN;
    out->default_port1 = ctrl_device(fact->ctrl1);
    out->default_port2 = ctrl_device(fact->ctrl2);
    out->video_mode = fact->video;
    out->persistence = fact->persistence > COGMASTER_PERSISTENCE_BATTERY
                           ? (uint32_t)COGMASTER_PERSISTENCE_UNKNOWN
                           : fact->persistence;
    out->persistent_size = fact->ram_bytes;
    out->persistence_geometry = fact->geometry;
    for (size_t i = 0u; i < 20u; i++) {
        out->sha1[2u * i] = digits[fact->sha1[i] >> 4];
        out->sha1[2u * i + 1u] = digits[fact->sha1[i] & 15u];
    }
    out->sha1[40] = 0;
    out->title[0] = 0;
    if (fact->board != 0u)
        return COGMASTER_STATUS_OK;
    name_unsupported(cogmaster__cartdb_slot(fact->slot), out);
    return COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD;
}

cogmaster_status cogmaster__cart_probe(const void *image, size_t n, cogmaster_cart_info *out)
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
    size_t body = n - off;
    if (body > CART_BODY_MAX) {
        return COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE;
    }
    const uint8_t *b = (const uint8_t *)image + off;
    out->body_offset = (uint32_t)off;
    read_header(b, body, out);
    const struct cogmaster__cart_fact *fact = cogmaster__cartdb_image(b, body);
    if (fact != NULL) {
        return cogmaster__cart_fact_to_info(fact, out);
    }
    return guess_board(b, body, out);
}

cogmaster_status cogmaster__cart_copy(cogmaster *m, const uint8_t *image, size_t n,
                                      const cogmaster_cart_info *p, struct cogmaster__cart *out)
{
    size_t off = p->body_offset;
    size_t body = n - off;
    uint32_t size = (uint32_t)((body + CART_PAGE - 1u) & ~(size_t)(CART_PAGE - 1u));
    uint32_t ram_size = p->board == COGMASTER__BOARD_SEGA          ? CART_RAM_SEGA
                        : p->board == COGMASTER__BOARD_CODEMASTERS ? CART_RAM_CODEMASTERS
                        : p->board == COGMASTER__BOARD_SEGA_EEPROM ? CART_RAM_EEPROM
                                                                   : 0u;
    if (p->persistence_geometry == COGMASTER_GEOMETRY_SUPPORTED &&
        p->board == COGMASTER__BOARD_SEGA)
        ram_size = p->persistent_size;
    uint8_t *rom = (uint8_t *)m->allocator.alloc(m->allocator.user, (size_t)size + ram_size);
    if (rom == NULL) {
        return COGMASTER_STATUS_ERR_NO_MEMORY;
    }
    memcpy(rom, image + off, body);
    memset(rom + body, 0xFF, size - body);
    /* The EEPROM board's 128 bytes start erased, all ones, whatever the persistence. */
    memset(rom + size,
           p->persistence == COGMASTER_PERSISTENCE_BATTERY ||
                   p->board == COGMASTER__BOARD_SEGA_EEPROM
               ? 0xFF
               : 0,
           ram_size);
    out->rom = rom;
    out->ram = ram_size != 0u ? rom + size : NULL;
    out->size = size;
    out->pages = size / CART_PAGE;
    out->ram_size = ram_size;
    out->crc32 = cogmaster__crc32(0u, image + off, body);
    out->board = p->board;
    out->persistence = p->persistence;
    out->persistent_size = p->persistent_size;
    out->persistence_geometry = p->persistence_geometry;
    out->generation = 0u;
    return COGMASTER_STATUS_OK;
}

void cogmaster__cart_install(cogmaster *m, const struct cogmaster__cart *c)
{
    if (m->cart.rom != NULL) {
        m->allocator.free(m->allocator.user, m->cart.rom, (size_t)m->cart.size + m->cart.ram_size);
    }
    m->cart = *c;
}

cogmaster_status cogmaster_probe(const void *image, size_t n, cogmaster_cart_info *out)
{
    if (out == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(out, COGMASTER__FIRST_SIZE_CART_INFO);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (image == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_cart_info info;
    st = cogmaster__cart_probe(image, n, &info);
    info.size = out->size;
    cogmaster__write_out(out, &info, (uint32_t)sizeof info);
    return st;
}
