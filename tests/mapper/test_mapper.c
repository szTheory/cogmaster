/* The cartridge boards at register level: synthetic images built here are
   loaded through the public API, then read and written through the bus in
   src/internal.h: cogmaster-test-mapper <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define PAGE 16384u
#define IMAGE_MAX (1024u * 1024u)

static uint8_t image[IMAGE_MAX];

/* The ROM header magic as the BIOS compares it, written as bytes. */
static const uint8_t header_magic[8] = {0x54u, 0x4Du, 0x52u, 0x20u, 0x53u, 0x45u, 0x47u, 0x41u};

/* Where each bank carries its stamp: three offsets, so slot 0's fixed first
   1 KiB, the rest of slot 0 and the middle of a slot are each seen. */
static const uint16_t stamp_at[3] = {0x0000u, 0x0400u, 0x2000u};

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* The stamp bank b carries: never $32, so the opcode scan sees no store. */
static uint8_t stamp(uint32_t b)
{
    return (uint8_t)(0x40u | b);
}

/* n bytes of $FF with each 16 KiB bank stamped at stamp_at, and a ROM
   header at $7FF0 with region code 4 and size code 1 (512 KiB). */
static void sega_image(size_t n)
{
    memset(image, 0xFF, n);
    for (uint32_t b = 0; (size_t)b * PAGE < n; b++) {
        for (uint32_t i = 0; i < 3u; i++) {
            size_t at = (size_t)b * PAGE + stamp_at[i];
            if (at < n) {
                image[at] = stamp(b);
            }
        }
    }
    memcpy(image + 0x7FF0u, header_magic, sizeof header_magic);
    image[0x7FFFu] = 0x41u;
}

/* A new Master System instance with the first n bytes of image loaded. */
static cogmaster *load(size_t n)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = n;
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    if (m->cart.rom == NULL) {
        exit(1);
    }
    return m;
}

/* The bank slot s shows, read from its middle stamp. */
static uint32_t slot_bank(cogmaster *m, uint32_t s)
{
    return cogmaster__bus_read(m, (uint16_t)(s * PAGE + 0x2000u)) & 0x3Fu;
}

/* A 512 KiB image is a Sega board paged 0, 1, 2. Each of banks 0 to 31
   reads its own stamp through slot 1 ($FFFE) and slot 2 ($FFFF). After
   $FFFD = 5, slot 0 below $0400 still reads bank 0 and from $0400 bank 5. */
static void test_sega_banks(void)
{
    sega_image(512u * 1024u);
    cogmaster *m = load(512u * 1024u);
    CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_SEGA);
    CHECK_EQ_U64(slot_bank(m, 0u), 0);
    CHECK_EQ_U64(slot_bank(m, 1u), 1);
    CHECK_EQ_U64(slot_bank(m, 2u), 2);
    uint32_t wrong = 0;
    for (uint32_t b = 0; b < 32u; b++) {
        cogmaster__bus_write(m, 0xFFFEu, (uint8_t)b);
        cogmaster__bus_write(m, 0xFFFFu, (uint8_t)b);
        wrong += slot_bank(m, 1u) != b;
        wrong += slot_bank(m, 2u) != b;
        wrong += cogmaster__bus_read(m, 0x4400u) != stamp(b);
        wrong += cogmaster__bus_read(m, 0x8000u) != stamp(b);
    }
    CHECK_EQ_U64(wrong, 0);
    cogmaster__bus_write(m, 0xFFFDu, 5u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0000u), stamp(0u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x03FFu), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x0400u), stamp(5u));
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0x2000u), stamp(5u));
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"sega.banks", test_sega_banks},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-mapper <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-mapper: unknown case %s\n", argv[1]);
    return 2;
}
