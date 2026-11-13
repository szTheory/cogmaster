/* --diagnose: run a cartridge for a fixed number of frames and print one line of facts about how
   it booted. Every printed value is a number, an enum word or a word from a static table, so the
   line leaks no digest, byte, image size, name or path. It does print the first-event call numbers
   and the fault pc; the fault's own frame is bucketed. docs/runner.md describes the line. */
#ifndef COGMASTER_RUNNER_DIAG_H
#define COGMASTER_RUNNER_DIAG_H

#include <stddef.h>
#include <stdint.h>

#include "cogmaster.h"
#include "expect.h"

#define COGMASTER__DIAG_FRAMES 1800u
#define COGMASTER__DIAG_HELD 8u /* distinct crop hashes kept; the counts cap here */

typedef struct cogmaster__diag {
    cogmaster__expect e; /* the state --expect judges boots@F from; no case is loaded */
    uint32_t first_display, first_nonblank, first_change; /* 1-based run call, 0 for none */
    uint32_t held;                                        /* hashes in seen, at most 8 */
    char seen[COGMASTER__DIAG_HELD][65];
    uint32_t distinct600, verdict600;
    uint8_t ram[8192]; /* $C000-$DFFF at call 1740 */
    uint32_t live;     /* the work RAM differs between calls 1740 and 1800 */
} cogmaster__diag;

void cogmaster__diag_init(cogmaster__diag *d);

/* Folds in run call `call` (1-based). Returns an exit status. */
int cogmaster__diag_frame(cogmaster__diag *d, const cogmaster *m, uint32_t call, uint32_t display,
                          uint16_t *pixels);

/* Prints the line when ran is the last frame; otherwise prints one fixed message on standard
   error and returns the failure status. image is the file's bytes. Returns an exit status. */
int cogmaster__diag_print(const cogmaster__diag *d, const cogmaster *m, const uint8_t *image,
                          size_t image_size, uint32_t ran);

/* The words print_fault and the diagnose line share: "unknown" for a value outside the tables. */
const char *cogmaster__fault_kind_name(uint32_t kind);
const char *cogmaster__fault_device_name(uint32_t device);
/* The word for a COGMASTER_BOARD_SOURCE_* value: none, db, header, heuristic or default. */
const char *cogmaster__board_source_name(uint32_t source);

#endif /* COGMASTER_RUNNER_DIAG_H */
