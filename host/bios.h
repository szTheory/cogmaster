/* Host BIOS helper, outside the core's determinism boundary: nothing here is
   hashed or saved. The runner and the libretro adapter use it to read a
   user's dump with a byte bound and to name it for a log line. The behaviour
   is described in docs/controllers.md. */
#ifndef COGMASTER_HOST_BIOS_H
#define COGMASTER_HOST_BIOS_H

#include <stddef.h>
#include <stdint.h>

/* One known dump: its file name in the public record, its size in bytes and
   the SHA-1 of the whole file. */
struct cogmaster__bios_row {
    const char *name;
    uint32_t size;
    uint8_t sha1[20];
};

#define COGMASTER_BIOS_SIZE 8192u
#define COGMASTER_BIOS_READ_CAP 8193u

enum { COGMASTER_BIOS_READ_OK = 0, COGMASTER_BIOS_READ_ABSENT = 1, COGMASTER_BIOS_READ_ERROR = 2 };

/* The built-in table of known dumps; *count gets its row count. */
const struct cogmaster__bios_row *cogmaster__bios_rows(size_t *count);

/* The name of the row whose digest and size both equal the arguments, else
   NULL. Pure: it reads only its arguments. */
const char *cogmaster__bios_lookup(const struct cogmaster__bios_row *rows, size_t count,
                                   const uint8_t sha1[20], uint32_t size);

/* Always fills sha1_out with the SHA-1 of data, then returns the name of the
   row of the given table that matches the digest and size, else NULL. The
   name is a label only. */
const char *cogmaster__bios_identify(const struct cogmaster__bios_row *rows, size_t count,
                                     const uint8_t *data, size_t size, uint8_t sha1_out[20]);

/* Reads at most COGMASTER_BIOS_READ_CAP bytes of path into buf, which has
   room for that many, and stores the count in *n. Returns READ_ABSENT when
   the path does not exist, READ_ERROR for any other failure, else READ_OK.
   Prints nothing. */
int cogmaster__bios_read(const char *path, uint8_t *buf, size_t *n);

#endif /* COGMASTER_HOST_BIOS_H */
