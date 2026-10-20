/* The exit statuses of cogmaster-run, defined once. --help prints this table
   and docs/runner.md repeats it; runner.exit.table keeps the two equal. A
   status enters the table when the runner first returns it. */
#ifndef COGMASTER_RUNNER_EXIT_STATUS_H
#define COGMASTER_RUNNER_EXIT_STATUS_H

enum {
    COGMASTER__EXIT_OK = 0,
    COGMASTER__EXIT_FAILURE = 1,
    COGMASTER__EXIT_USAGE = 2,
    COGMASTER__EXIT_MISMATCH = 3,
    COGMASTER__EXIT_SAVE_SIZE = 4,
    COGMASTER__EXIT_REFUSED = 77
};

typedef struct cogmaster__exit_status {
    int code;
    const char *name;
    const char *meaning;
} cogmaster__exit_status;

static const cogmaster__exit_status cogmaster__exit_statuses[] = {
    {COGMASTER__EXIT_OK, "ok",
     "every frame ran or the probe printed, or a signal stopped the run after a whole frame"},
    {COGMASTER__EXIT_FAILURE, "failure",
     "a file could not be read or written, memory ran out or the core failed"},
    {COGMASTER__EXIT_USAGE, "usage", "the command line or input script is invalid, so nothing ran"},
    {COGMASTER__EXIT_MISMATCH, "mismatch",
     "an accepted expectation did not match the observed predicate"},
    {COGMASTER__EXIT_SAVE_SIZE, "save", "the battery file is oversized, so nothing ran"},
    {COGMASTER__EXIT_REFUSED, "refused",
     "the cartridge, its size or the machine and region is not supported, so nothing ran"},
};

#endif /* COGMASTER_RUNNER_EXIT_STATUS_H */
