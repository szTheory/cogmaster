/* Fuzz the loader: probe the bytes, then load them on the profile the probe
   guesses, as cogmaster-run --machine auto does. Any input must return a
   status; the probe and the load must agree on whether the image is
   accepted. */
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
    cogmaster_status probed = cogmaster_probe(data, size, &info);

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
    cogmaster_status loaded = cogmaster_load(m, &media);
    cogmaster_destroy(m);

    /* A probe that accepts what the load refuses, or the reverse, would
       send a host's library view and its load down different paths. */
    if ((probed == COGMASTER_STATUS_OK) != (loaded == COGMASTER_STATUS_OK)) {
        abort();
    }
    return 0;
}
