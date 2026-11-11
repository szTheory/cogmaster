/* --hash-audio: the runner's view of the sound. After each frame it drains the
   core's level-change log and its samples, merges the log into one canonical
   stream of transitions and hashes both. docs/runner.md describes the output
   for users. */
#ifndef COGMASTER_RUNNER_AUDIO_H
#define COGMASTER_RUNNER_AUDIO_H

#include <stdint.h>

#include "cogmaster.h"
#include "sha256.h"

/* The records one frame can log: the core's bound is about 21,200 on NTSC and
   25,400 on PAL. */
#define COGMASTER_RUN_TRACE_RECORDS 32768u

/* The sample frames the runner offers the core each frame. A frame holds 801
   or 802 (965 or 966 on PAL), so the default leaves room for the largest. The
   tests build a runner with a smaller value to show that a short frame fails. */
#ifndef COGMASTER_RUN_AUDIO_CAP
#define COGMASTER_RUN_AUDIO_CAP 966u
#endif

/* The sound sources the core logs: tone and noise 0 to 3, the Game Gear mask,
   the FM output and the FM control byte. */
#define COGMASTER__AUDIO_SOURCES 7u

struct cogmaster__audio_entry;

typedef struct cogmaster__audio {
    cogmaster_audio_record *records; /* the drained log of one frame */
    struct cogmaster__audio_entry *sorted;
    int16_t *pcm;
    cogmaster__sha256 transitions_hash;
    cogmaster__sha256 pcm_hash;
    uint64_t last_tick[COGMASTER__AUDIO_SOURCES];  /* newest tick seen per source */
    uint32_t last_level[COGMASTER__AUDIO_SOURCES]; /* newest merged level per source */
    uint64_t transitions;
    uint64_t samples;
    uint32_t peak;
    uint32_t clipped;
    uint32_t channels;
    uint64_t rate_num; /* samples per frame as a fraction: 3,504,512 / 4,375, or on PAL */
    uint64_t rate_den; /* 685,094,400 / 709,379 */
} cogmaster__audio;

/* Allocates the buffers and starts both hashes. pal is 1 for a PAL instance.
   Returns the exit status; a zeroed a is always safe to close. */
int cogmaster__audio_open(cogmaster__audio *a, int pal);

/* Drains and hashes the frame just run. Returns the exit status: 1, after a
   line on standard error, when the log overflowed, a source's ticks went
   backwards, the core dropped samples or the frame did not hold exactly
   floor(f x num / den) - floor((f - 1) x num / den) samples, f being its number. */
int cogmaster__audio_frame(cogmaster__audio *a, cogmaster *m, uint64_t frame);

/* Writes the SHA-256 of the transitions so far as 64 lowercase hex digits and a NUL. It finalises a
   copy of the running hash, so the run keeps accumulating and may ask again at a later frame. */
void cogmaster__audio_transitions_hex(const cogmaster__audio *a, char hex[65]);

/* Prints the two result lines for the frames that ran. */
void cogmaster__audio_print(cogmaster__audio *a, uint32_t frames);

void cogmaster__audio_close(cogmaster__audio *a);

#endif /* COGMASTER_RUNNER_AUDIO_H */
