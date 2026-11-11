/* Fuzz the boot ROM: the input is the BIOS image, whatever its length, loaded
   with a fixed cartridge that does nothing (DI; HALT) on a Master System 1 or
   2. A wrong size must be refused; an 8192-byte image runs two frames, each of
   which must return within its bounded emulated time whatever the image does. */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "fuzz.h"
#include "internal.h"

#define CART_SIZE 16384u

static uint32_t allocations;

static void *fuzz_alloc(void *user, size_t size)
{
    (void)user;
    allocations++;
    return malloc(size);
}

static void fuzz_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static uint8_t cart[CART_SIZE];
    cart[0] = 0xF3u;
    cart[1] = 0x76u;

    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = fuzz_alloc;
    cfg.allocator.free = fuzz_free;
    cogmaster *m = NULL;
    if (cogmaster_create(&cfg, &m) != COGMASTER_STATUS_OK) {
        abort();
    }

    /* An empty input still passes a non-NULL pointer, so the pair rule is reached with size 0. */
    static const uint8_t none = 0u;
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = cart;
    media.image_size = sizeof cart;
    media.machine =
        size > 0u && (data[size - 1u] & 1u) == 0u ? COGMASTER_MACHINE_SMS1 : COGMASTER_MACHINE_SMS2;
    media.bios = size > 0u ? data : &none;
    media.bios_size = size;
    cogmaster_status loaded = cogmaster_load(m, &media);
    if (loaded == COGMASTER_STATUS_OK) {
        if (size != COGMASTER__BIOS_SIZE) {
            abort();
        }
        uint32_t loaded_allocations = allocations;
        for (int i = 0; i < 2; i++) {
            cogmaster_run_result r;
            memset(&r, 0, sizeof r);
            r.size = (uint32_t)sizeof r;
            if (cogmaster_run_frame(m, &r) != COGMASTER_STATUS_OK) {
                abort();
            }
            uint64_t deadline = (uint64_t)(i + 1) * 179208u;
            if (allocations != loaded_allocations || r.overshoot_ticks > 69u ||
                m->ticks < deadline || m->ticks > deadline + 69u || m->render_line > 240u ||
                m->render_x >= 256u || m->vdp.sprite_count > 8u || m->vdp.sprite_scanned > 64u) {
                abort();
            }
        }
    } else if (loaded != (size == 0u ? COGMASTER_STATUS_ERR_ARGUMENT
                                     : COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE) ||
               m->loaded != 0u) {
        abort();
    }
    cogmaster_destroy(m);
    return 0;
}
