/* Raw battery files owned by this host; no filesystem calls enter the core. */
#ifndef COGMASTER_RUNNER_SAVE_H
#define COGMASTER_RUNNER_SAVE_H
#include "cogmaster.h"
typedef struct cogmaster__save {
    char *path;
    uint64_t acknowledged;
} cogmaster__save;
/* Initialize a zeroed context, validate directory and import at most N+1 bytes. */
int cogmaster__save_load(cogmaster__save *save, cogmaster *m, const char *dir, const char *rom);
/* Replace only dirty data; acknowledge only after a successful replacement. */
int cogmaster__save_flush(cogmaster__save *save, cogmaster *m);
void cogmaster__save_dispose(cogmaster__save *save);
#endif
