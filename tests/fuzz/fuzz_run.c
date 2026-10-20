/* Fuzz a short run: load the bytes with fast boot on the profile the probe
   guesses and run two frames. Each frame must return within its bounded
   emulated time, whatever the program does. */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "fuzz.h"

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
    if (cogmaster_load(m, &media) == COGMASTER_STATUS_OK) {
        for (int i = 0; i < 2; i++) {
            cogmaster_run_result r;
            memset(&r, 0, sizeof r);
            r.size = (uint32_t)sizeof r;
            if (cogmaster_run_frame(m, &r) != COGMASTER_STATUS_OK) {
                abort();
            }
        }
    }
    cogmaster_destroy(m);
    return 0;
}
