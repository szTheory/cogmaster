/* Replay raw save bytes and hostile lengths against classified physical RAM.
   Internal setup isolates the parser; battery.continuation separately exercises
   production image identity and the owned guest. No commercial image is needed. */
#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "fuzz.h"

static uint8_t image[65536], before[400000], after[400000];

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
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    if (cogmaster_create(&cfg, &m) != COGMASTER_STATUS_OK)
        abort();
    cogmaster_media media = {0};
    media.size = sizeof media;
    if (cogmaster_load(m, &media) != COGMASTER_STATUS_OK)
        abort();
    cogmaster_cart_info p = {0};
    p.board = COGMASTER_BOARD_SEGA;
    /* Four classifications, driven independently of the raw length. */
    uint32_t selection = size != 0u ? data[0] % 4u : 0u;
    p.persistence = selection == 0u   ? COGMASTER_PERSISTENCE_BATTERY
                    : selection == 1u ? COGMASTER_PERSISTENCE_NONE
                    : selection == 2u ? COGMASTER_PERSISTENCE_UNKNOWN
                                      : COGMASTER_PERSISTENCE_BATTERY;
    p.persistent_size = selection == 3u ? 24576u : 32768u;
    p.persistence_geometry =
        selection == 3u ? COGMASTER_GEOMETRY_UNSUPPORTED : COGMASTER_GEOMETRY_SUPPORTED;
    struct cogmaster__cart cart = {0};
    if (cogmaster__cart_copy(m, image, sizeof image, &p, &cart) != COGMASTER_STATUS_OK)
        abort();
    cogmaster__cart_install(m, &cart);
    size_t state_n = cogmaster__state_size(m);
    if (state_n > sizeof before ||
        cogmaster__state_save(m, before, sizeof before) != COGMASTER_STATUS_OK)
        abort();
    cogmaster_status st = cogmaster_import_battery(m, data, size);
    cogmaster_status expected = selection == 1u   ? COGMASTER_STATUS_ERR_NO_CONTENT
                                : selection != 0u ? COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD
                                : size > 32768u   ? COGMASTER_STATUS_ERR_SAVE_SIZE
                                                  : COGMASTER_STATUS_OK;
    if (st != expected || m->cart.generation != 0u)
        abort();
    if (st != COGMASTER_STATUS_OK) {
        if (cogmaster__state_save(m, after, sizeof after) != COGMASTER_STATUS_OK ||
            memcmp(before, after, state_n) != 0)
            abort();
    } else {
        if (size != 0u && memcmp(m->cart.ram, data, size) != 0)
            abort();
        for (size_t i = size; i < 32768u; i++)
            if (m->cart.ram[i] != 0xffu)
                abort();
        if (cogmaster__state_save(m, before, sizeof before) != COGMASTER_STATUS_OK ||
            cogmaster_import_battery(m, data, size) != COGMASTER_STATUS_OK ||
            cogmaster__state_save(m, after, sizeof after) != COGMASTER_STATUS_OK ||
            memcmp(before, after, state_n) != 0)
            abort();
    }
    /* SIZE_MAX must reject before touching even a one-byte source. */
    if (cogmaster__state_save(m, before, sizeof before) != COGMASTER_STATUS_OK ||
        cogmaster_import_battery(m, image, SIZE_MAX) == COGMASTER_STATUS_OK ||
        cogmaster__state_save(m, after, sizeof after) != COGMASTER_STATUS_OK ||
        memcmp(before, after, state_n) != 0)
        abort();
    cogmaster_destroy(m);
    return 0;
}
