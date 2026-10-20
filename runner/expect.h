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
int cogmaster__expect_frame(cogmaster__expect *e, const cogmaster *m, uint32_t frame,
                            uint32_t display, uint16_t *pixels);
int cogmaster__expect_finish(const cogmaster__expect *e);
#endif
