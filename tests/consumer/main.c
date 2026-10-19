/* A program that knows only the installed headers: it supplies its own
   allocator, creates an instance, loads the test card, runs one frame, copies
   it and destroys the instance. Exits 0 and prints one fixed line on success,
   1 otherwise. */
#include <cogmaster.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The core has no default allocator; the caller passes one. */
static void *consumer_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void consumer_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint16_t pixels[256 * 240];

int main(void)
{
    cogmaster_versions v;
    cogmaster_config cfg;
    cogmaster_media media;
    cogmaster_frame io;
    cogmaster *m = NULL;
    cogmaster_status st;
    int ok;

    /* Zero every struct, then set its size, as the header requires. */
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    if (cogmaster_get_versions(&v) != COGMASTER_STATUS_OK) {
        printf("cogmaster_get_versions failed\n");
        return 1;
    }

    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = consumer_alloc;
    cfg.allocator.free = consumer_free;
    st = cogmaster_create(&cfg, &m);
    if (st != COGMASTER_STATUS_OK) {
        printf("cogmaster_create: status %u\n", (unsigned)st);
        return 1;
    }

    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pixels = pixels;
    io.pitch_bytes = 256 * 2;
    st = cogmaster_load(m, &media);
    if (st == COGMASTER_STATUS_OK) {
        st = cogmaster_run_frame(m, NULL);
    }
    if (st == COGMASTER_STATUS_OK) {
        st = cogmaster_get_frame(m, &io);
    }
    /* The Master System test card is the whole 256 x 192 field. */
    ok = st == COGMASTER_STATUS_OK && io.w == 256u && io.h == 192u;
    cogmaster_destroy(m);
    if (!ok) {
        printf("frame failed: status %u, %ux%u\n", (unsigned)st, (unsigned)io.w, (unsigned)io.h);
        return 1;
    }
    printf("consumer: api %u, one frame %ux%u\n", (unsigned)v.api, (unsigned)io.w, (unsigned)io.h);
    return 0;
}
