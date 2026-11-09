/* API v9: the audio drain. Every name the version adds is used here. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"
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
int main(void)
{
    const size_t p = sizeof(void *);
    CHECK(COGMASTER_API_VERSION >= 9);
    CHECK(COGMASTER_BEHAVIOUR_REVISION >= 11);
    CHECK(COGMASTER_STATE_FORMAT_VERSION >= 11);
    CHECK_EQ_U64(offsetof(cogmaster_audio, size), 0u);
    CHECK_EQ_U64(offsetof(cogmaster_audio, samples), p);
    CHECK_EQ_U64(offsetof(cogmaster_audio, capacity), 2u * p);
    CHECK_EQ_U64(offsetof(cogmaster_audio, written), 2u * p + 4u);
    CHECK_EQ_U64(offsetof(cogmaster_audio, channels), 2u * p + 8u);
    CHECK_EQ_U64(offsetof(cogmaster_audio, dropped), 2u * p + 12u);
    CHECK_EQ_U64(sizeof(cogmaster_audio), 2u * p + 16u);

    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = allocate;
    cfg.allocator.free = release;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = {0};
    media.size = sizeof media;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);

    /* A test card frame is 801 mono samples, all silent. */
    int16_t pcm[966u * 2u];
    memset(pcm, 0x5a, sizeof pcm);
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = sizeof a;
    a.samples = pcm;
    a.capacity = 966u;
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 0u);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(a.written, 801u);
    CHECK_EQ_U64(a.channels, 1u);
    CHECK_EQ_U64(a.dropped, 0u);
    for (uint32_t i = 0; i < a.written; i++)
        CHECK_EQ_U64(pcm[i], 0u);
    cogmaster_destroy(m);
    CHECK_DONE();
}
