#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exit_status.h"

/* A level no source can hold: every level is 16 bits once serialised. */
#define NO_LEVEL 0x10000u

/* One logged change, with its place in the log so the sort is total. */
struct cogmaster__audio_entry {
    uint64_t tick;
    uint32_t source;
    uint32_t level;
    uint32_t index;
};

int cogmaster__audio_open(cogmaster__audio *a, int pal)
{
    memset(a, 0, sizeof *a);
    a->records = (cogmaster_audio_record *)malloc(COGMASTER_RUN_TRACE_RECORDS * sizeof *a->records);
    a->sorted =
        (struct cogmaster__audio_entry *)malloc(COGMASTER_RUN_TRACE_RECORDS * sizeof *a->sorted);
    a->pcm = (int16_t *)malloc((size_t)COGMASTER_RUN_AUDIO_CAP * 2u * sizeof *a->pcm);
    if (a->records == NULL || a->sorted == NULL || a->pcm == NULL) {
        fprintf(stderr, "cogmaster-run: out of memory\n");
        return COGMASTER__EXIT_FAILURE;
    }
    cogmaster__sha256_init(&a->transitions_hash);
    cogmaster__sha256_init(&a->pcm_hash);
    for (uint32_t s = 0; s < COGMASTER__AUDIO_SOURCES; s++) {
        a->last_level[s] = NO_LEVEL;
    }
    a->frame_samples = pal ? 965u : 801u;
    return COGMASTER__EXIT_OK;
}

void cogmaster__audio_close(cogmaster__audio *a)
{
    free(a->records);
    free(a->sorted);
    free(a->pcm);
    memset(a, 0, sizeof *a);
}

/* Orders by tick, then source, then place in the log. Only < and > compare,
   so no subtraction can wrap. */
static int entry_order(const void *left, const void *right)
{
    const struct cogmaster__audio_entry *x = (const struct cogmaster__audio_entry *)left;
    const struct cogmaster__audio_entry *y = (const struct cogmaster__audio_entry *)right;
    if (x->tick < y->tick) {
        return -1;
    }
    if (x->tick > y->tick) {
        return 1;
    }
    if (x->source < y->source) {
        return -1;
    }
    if (x->source > y->source) {
        return 1;
    }
    if (x->index < y->index) {
        return -1;
    }
    return x->index > y->index ? 1 : 0;
}

/* Hashes one merged change as 11 bytes: the tick (8, low byte first), the
   source (1) and the level (2, low byte first). Nothing here depends on the
   host's byte order or on a struct's layout. */
static void hash_transition(cogmaster__audio *a, const struct cogmaster__audio_entry *e,
                            uint32_t level)
{
    uint8_t bytes[11];
    for (uint32_t k = 0; k < 8u; k++) {
        bytes[k] = (uint8_t)(e->tick >> (8u * k));
    }
    bytes[8] = (uint8_t)e->source;
    bytes[9] = (uint8_t)(level & 0xFFu);
    bytes[10] = (uint8_t)(level >> 8);
    cogmaster__sha256_update(&a->transitions_hash, bytes, sizeof bytes);
    a->transitions++;
}

/* Drains the frame's log, checks each source's ticks never go backwards in the
   order the core logged them, sorts, keeps the last level per tick and
   source, drops a level its source already holds and hashes what is left. */
static int merge_trace(cogmaster__audio *a, cogmaster *m, uint64_t frame)
{
    cogmaster_audio_trace tr;
    memset(&tr, 0, sizeof tr);
    tr.size = (uint32_t)sizeof tr;
    tr.records = a->records;
    tr.capacity = COGMASTER_RUN_TRACE_RECORDS;
    cogmaster_status st = cogmaster_get_audio_trace(m, &tr);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_get_audio_trace failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
    if (tr.dropped > 0u) {
        fprintf(stderr, "cogmaster-run: frame %llu lost %lu audio trace records\n",
                (unsigned long long)frame, (unsigned long)tr.dropped);
        return COGMASTER__EXIT_FAILURE;
    }
    for (uint32_t i = 0; i < tr.written; i++) {
        const cogmaster_audio_record *r = &a->records[i];
        if (r->source >= COGMASTER__AUDIO_SOURCES || r->tick < a->last_tick[r->source]) {
            fprintf(
                stderr, "cogmaster-run: frame %llu logged source %lu at tick %llu out of order\n",
                (unsigned long long)frame, (unsigned long)r->source, (unsigned long long)r->tick);
            return COGMASTER__EXIT_FAILURE;
        }
        a->last_tick[r->source] = r->tick;
        a->sorted[i].tick = r->tick;
        a->sorted[i].source = r->source;
        a->sorted[i].level = r->level;
        a->sorted[i].index = i;
    }
    qsort(a->sorted, tr.written, sizeof *a->sorted, entry_order);
    uint32_t i = 0;
    while (i < tr.written) {
        uint32_t last = i;
        while (last + 1u < tr.written && a->sorted[last + 1u].tick == a->sorted[i].tick &&
               a->sorted[last + 1u].source == a->sorted[i].source) {
            last++;
        }
        const struct cogmaster__audio_entry *e = &a->sorted[last];
        uint32_t level = e->level & 0xFFFFu;
        if (level != a->last_level[e->source]) {
            a->last_level[e->source] = level;
            hash_transition(a, e, level);
        }
        i = last + 1u;
    }
    return COGMASTER__EXIT_OK;
}

/* Drains the frame's samples and hashes them as 16-bit values, low byte
   first, left then right. */
static int hash_pcm(cogmaster__audio *a, cogmaster *m, uint64_t frame)
{
    cogmaster_audio io;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.samples = a->pcm;
    io.capacity = COGMASTER_RUN_AUDIO_CAP;
    cogmaster_status st = cogmaster_get_audio(m, &io);
    if (st != COGMASTER_STATUS_OK || (io.channels != 1u && io.channels != 2u)) {
        fprintf(stderr, "cogmaster-run: cogmaster_get_audio failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
    if (io.written != a->frame_samples && io.written != a->frame_samples + 1u) {
        fprintf(stderr, "cogmaster-run: frame %llu holds %lu samples, not %lu or %lu\n",
                (unsigned long long)frame, (unsigned long)io.written,
                (unsigned long)a->frame_samples, (unsigned long)(a->frame_samples + 1u));
        return COGMASTER__EXIT_FAILURE;
    }
    if (io.dropped > 0u) {
        fprintf(stderr, "cogmaster-run: the core dropped %lu samples by frame %llu\n",
                (unsigned long)io.dropped, (unsigned long long)frame);
        return COGMASTER__EXIT_FAILURE;
    }
    uint8_t bytes[2u * 2u * COGMASTER_RUN_AUDIO_CAP];
    uint32_t count = io.written * io.channels;
    for (uint32_t k = 0; k < count; k++) {
        int32_t v = a->pcm[k];
        uint32_t magnitude = (uint32_t)(v < 0 ? -v : v);
        if (magnitude > a->peak) {
            a->peak = magnitude;
        }
        if (v == 32767 || v == -32768) {
            a->clipped++;
        }
        uint16_t u = (uint16_t)a->pcm[k];
        bytes[2u * k] = (uint8_t)(u & 0xFFu);
        bytes[2u * k + 1u] = (uint8_t)(u >> 8);
    }
    cogmaster__sha256_update(&a->pcm_hash, bytes, (size_t)count * 2u);
    a->samples += io.written;
    a->channels = io.channels;
    return COGMASTER__EXIT_OK;
}

int cogmaster__audio_frame(cogmaster__audio *a, cogmaster *m, uint64_t frame)
{
    int status = merge_trace(a, m, frame);
    return status != COGMASTER__EXIT_OK ? status : hash_pcm(a, m, frame);
}

void cogmaster__audio_transitions_hex(const cogmaster__audio *a, char hex[65])
{
    cogmaster__sha256 copy = a->transitions_hash;
    uint8_t digest[32];
    cogmaster__sha256_final(&copy, digest);
    cogmaster__sha256_hex(digest, hex);
}

void cogmaster__audio_print(cogmaster__audio *a, uint32_t frames)
{
    uint8_t digest[32];
    char hex[65];
    cogmaster__audio_transitions_hex(a, hex);
    printf("audio-transitions %lu count %llu sha256 %s\n", (unsigned long)frames,
           (unsigned long long)a->transitions, hex);
    cogmaster__sha256_final(&a->pcm_hash, digest);
    cogmaster__sha256_hex(digest, hex);
    printf("audio-pcm %lu rate 48000 channels %lu samples %llu peak %lu clipped %lu sha256 %s\n",
           (unsigned long)frames, (unsigned long)a->channels, (unsigned long long)a->samples,
           (unsigned long)a->peak, (unsigned long)a->clipped, hex);
}
