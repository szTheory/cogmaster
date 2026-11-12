/* Bounded accepted expectations; hashes are values, never case identity. */
#ifndef COGMASTER_RUNNER_EXPECT_H
#define COGMASTER_RUNNER_EXPECT_H
#include "cogmaster.h"
#define COGMASTER__EXPECT_MAX 64u
#define COGMASTER__EXPECT_FRAMES 1000000u
typedef struct cogmaster__expect_case {
    uint32_t kind, frame, address, value, seen, pass;
    char hash[65];
} cogmaster__expect_case;
typedef struct cogmaster__expect {
    cogmaster__expect_case cases[COGMASTER__EXPECT_MAX];
    uint32_t count, display, distinct, fault;
    char first[65];
} cogmaster__expect;
int cogmaster__expect_load(cogmaster__expect *e, const char *path, uint32_t frames);
/* Whether any audio@F case exists, and whether one is at frame F: the runner drains the sound once
   per frame and hands the running transition hash only to a frame that has a case. */
int cogmaster__expect_has_audio(const cogmaster__expect *e);
int cogmaster__expect_audio_at(const cogmaster__expect *e, uint32_t frame);
/* audio_hex is NULL except at a frame with an audio case; then it and the transition count so far
   are the observed values. */
int cogmaster__expect_frame(cogmaster__expect *e, const cogmaster *m, uint32_t frame,
                            uint32_t display, uint16_t *pixels, const char *audio_hex,
                            uint64_t transitions);
/* Reads the frame the last run call produced: sets its crop hash in hex and *nonblank to 1 when the
   crop holds two or more distinct values, and folds the display and fault into e. No case is
   judged. */
int cogmaster__expect_observe(cogmaster__expect *e, const cogmaster *m, uint32_t display,
                              uint16_t *pixels, char hex[65], uint32_t *nonblank);
/* The boots predicate over what observe has folded in so far: display on, two distinct pictures,
   no fault. */
int cogmaster__expect_boots(const cogmaster__expect *e);
int cogmaster__expect_finish(const cogmaster__expect *e);
#endif
