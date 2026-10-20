/* The runner's command line: the options, their parse, --help and
   --version. docs/runner.md describes them for users. */
#ifndef COGMASTER_RUNNER_ARGS_H
#define COGMASTER_RUNNER_ARGS_H

#include <stdint.h>

/* One --png request. */
typedef struct cogmaster__png_request {
    uint32_t frame;
    const char *path;
} cogmaster__png_request;

typedef struct cogmaster__run_options {
    uint32_t frames;
    int bench;
    int hash;
    const char *input;
    uint32_t port1, port2;
    uint32_t machine; /* COGMASTER_MACHINE_*; AUTO is resolved from the file */
    uint32_t video_mode;
    uint32_t region, tv;          /* COGMASTER_REGION_* and COGMASTER_TV_* */
    const char *file;             /* the cartridge image, or NULL for the test card */
    int probe;                    /* --probe: print the file's report, run nothing */
    int trace;                    /* --trace first-fault */
    int mailbox;                  /* --mailbox: stop at the mailbox's done status and print it */
    cogmaster__png_request *pngs; /* room for one per argument */
    uint32_t png_count;
    int printed; /* --help or --version ran; nothing else to do */
} cogmaster__run_options;

/* Fills o from the command line. Returns 0 when the options are valid, else
   the usage status after a usage message on standard error. --help and
   --version print here and set o->printed. o->pngs must have room for argc
   entries. */
int cogmaster__parse_options(int argc, char **argv, cogmaster__run_options *o);

#endif /* COGMASTER_RUNNER_ARGS_H */
