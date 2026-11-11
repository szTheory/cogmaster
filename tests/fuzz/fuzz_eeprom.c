/* Fuzz the 93C46 bit-banger on its cartridge board. The bytes are read in
   pairs: the low bit of the first picks a write to $8000 (the chip window) or
   to $FFFC (the window and reset register), the second is the value. After
   every write the chip's fields must sit inside the bounds the data sheet's
   command lengths give (Microchip 93AA46/93LC46/93C46 1K Microwire serial
   EEPROM, instruction set table: 8 command bits after the start bit, 16 data
   bits, a READ that streams on), whatever the clocking. At the end the state is
   saved and loaded into a second instance of the same cartridge, and the two
   saves must be byte-equal. */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "fuzz.h"
#include "internal.h"
#include "state.h"

#define IMAGE_SIZE (64u * 1024u)
#define MAX_PAIRS 4096u
#define WINDOW 0x8000u
#define CTRL 0xFFFCu
#define WINDOW_OPEN 0x08u

/* Chip phases as eeprom.c numbers them, and the flag bits of the chip. */
enum { IDLE, COMMAND, DATA_IN, READ, COMPLETE };
#define FLAG_DATA_LOW 0x01u
#define FLAG_CS 0x04u
#define FLAG_PENDING 0x10u
#define FLAG_ALL 0x1Fu

static uint8_t boot[32u * 1024u];
static uint8_t image[IMAGE_SIZE];

static void *fuzz_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void fuzz_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* A Game Gear instance carrying a 64 KiB image on the EEPROM board with its
   128-byte save, powered up. */
static cogmaster *open_board(void)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = fuzz_alloc;
    cfg.allocator.free = fuzz_free;
    cogmaster *m = NULL;
    if (cogmaster_create(&cfg, &m) != COGMASTER_STATUS_OK) {
        abort();
    }
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = boot;
    media.image_size = sizeof boot;
    media.machine = COGMASTER_MACHINE_GG;
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.image_size = IMAGE_SIZE;
    info.board = COGMASTER__BOARD_SEGA_EEPROM;
    info.persistence = COGMASTER_PERSISTENCE_BATTERY;
    info.persistence_geometry = COGMASTER_GEOMETRY_SUPPORTED;
    info.persistent_size = 128u;
    struct cogmaster__cart cart;
    if (cogmaster_load(m, &media) != COGMASTER_STATUS_OK ||
        cogmaster__cart_copy(m, image, IMAGE_SIZE, &info, &cart) != COGMASTER_STATUS_OK) {
        abort();
    }
    cogmaster__cart_install(m, &cart);
    if (cogmaster_reset(m, COGMASTER_RESET_POWER) != COGMASTER_STATUS_OK) {
        abort();
    }
    return m;
}

/* The chip's fields against their bounds. A command takes 8 clocks after the
   start bit, so 7 are counted while it is taken in; WRITE and WRAL data take
   16, so 15; a READ shows at most 16 bits of a word before the next word. Idle
   and complete count nothing. Chip select low is idle, the output pin is only
   driven low while reading, and only a complete command waits to program. */
static void check_chip(const struct cogmaster__eeprom *e)
{
    uint32_t limit = e->phase == COMMAND ? 7u : e->phase == IDLE || e->phase == COMPLETE ? 0u : 15u;
    if (e->phase > COMPLETE || e->count > limit || (e->flags & ~FLAG_ALL) != 0u ||
        ((e->flags & FLAG_CS) == 0u && (e->phase != IDLE || e->count != 0u)) ||
        ((e->flags & FLAG_DATA_LOW) != 0u && e->phase != READ) ||
        ((e->flags & FLAG_PENDING) != 0u && e->phase != COMPLETE)) {
        abort();
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int ready;
    if (!ready) {
        memset(boot, 0xFF, sizeof boot);
        for (uint32_t i = 0; i < IMAGE_SIZE; i++) {
            image[i] = (uint8_t)((i * 2654435761u) >> 24);
        }
        ready = 1;
    }
    cogmaster *a = open_board();
    /* The window is open from the start, so $8000 traffic reaches the chip. */
    cogmaster__bus_write(a, CTRL, WINDOW_OPEN);
    size_t pairs = size / 2u < MAX_PAIRS ? size / 2u : MAX_PAIRS;
    for (size_t i = 0; i < pairs; i++) {
        cogmaster__bus_write(a, (data[2u * i] & 1u) ? CTRL : WINDOW, data[2u * i + 1u]);
        (void)cogmaster__bus_read(a, WINDOW);
        check_chip(&a->eeprom);
    }

    size_t n = cogmaster__state_size(a);
    uint8_t *save_a = (uint8_t *)malloc(n);
    uint8_t *save_b = (uint8_t *)malloc(n);
    cogmaster *b = open_board();
    if (n == 0u || save_a == NULL || save_b == NULL ||
        cogmaster__state_save(a, save_a, n) != COGMASTER_STATUS_OK ||
        cogmaster__state_load(b, save_a, n) != COGMASTER_STATUS_OK ||
        cogmaster__state_size(b) != n ||
        cogmaster__state_save(b, save_b, n) != COGMASTER_STATUS_OK ||
        memcmp(save_a, save_b, n) != 0 || memcmp(a->cart.ram, b->cart.ram, 128u) != 0) {
        abort();
    }
    free(save_a);
    free(save_b);
    cogmaster_destroy(a);
    cogmaster_destroy(b);
    return 0;
}
