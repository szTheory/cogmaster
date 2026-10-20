/* Fuzz a short run: load the bytes with fast boot on the profile the probe
   guesses and run two frames. Each frame must return within its bounded
   emulated time, whatever the program does. */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "fuzz.h"
#include "internal.h"

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
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    if (cogmaster_probe(data, size, &info) != COGMASTER_STATUS_OK) {
        return 0;
    }

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
    media.image = data;
    media.image_size = size;
    media.machine = info.machine_guess;
    media.region = info.region_guess;
    media.tv = info.machine_guess != COGMASTER_MACHINE_GG &&
                       info.region_guess != COGMASTER_REGION_JAPAN && size > 0u && (data[0] & 1u)
                   ? COGMASTER_TV_PAL
                   : COGMASTER_TV_NTSC;
    if (cogmaster_load(m, &media) == COGMASTER_STATUS_OK) {
        uint32_t loaded_allocations = allocations;
        for (int i = 0; i < 2; i++) {
            cogmaster_run_result r;
            memset(&r, 0, sizeof r);
            r.size = (uint32_t)sizeof r;
            if (cogmaster_run_frame(m, &r) != COGMASTER_STATUS_OK) {
                abort();
            }
            uint64_t deadline =
                (uint64_t)(i + 1) * (media.tv == COGMASTER_TV_PAL ? 214092u : 179208u);
            if (allocations != loaded_allocations || r.overshoot_ticks > 69u ||
                m->ticks < deadline || m->ticks > deadline + 69u || m->render_line > 240u ||
                m->render_x >= 256u || m->vdp.sprite_count > 8u || m->vdp.sprite_scanned > 64u) {
                abort();
            }
        }
    }
    cogmaster_destroy(m);
    return 0;
}
