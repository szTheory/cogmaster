/* The runner's cartridge file: reading it with the size guard, the machine
   --machine auto picks, and the --probe report. */
#ifndef COGMASTER_RUNNER_PROBE_H
#define COGMASTER_RUNNER_PROBE_H

#include <stddef.h>
#include <stdint.h>

/* The largest file the runner reads: 64 MiB, the RetroAchievements hash's
   own cap. A larger file is refused before any of it is read. */
#define COGMASTER__FILE_MAX 67108864u

/* 1 when path ends in ext ("." and letters), compared without case. */
int cogmaster__has_extension(const char *path, const char *ext);

/* Reads the whole file at path into a malloc'd buffer (*out, *n; free it
   with free). Returns 0, the refusal status for a file over
   COGMASTER__FILE_MAX bytes, or the failure status, after a message. */
int cogmaster__read_file(const char *path, uint8_t **out, size_t *n);

/* The machine --machine auto runs the image on: the header's region code,
   then a .gg file name, then the Master System. */
uint32_t cogmaster__auto_machine(const char *path, const uint8_t *image, size_t n);

/* Prints the --probe report, one key=value per line in a fixed order.
   Returns 0, the refusal status for an image the core will not load, or
   the failure status. */
int cogmaster__probe_print(const char *path, const uint8_t *image, size_t n);

#endif /* COGMASTER_RUNNER_PROBE_H */
