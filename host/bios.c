/* Host BIOS helper. Faults prevented: a dump read without a byte bound (a
   FIFO, /dev/zero or a 4 GB file costing more bytes than a BIOS is), a
   recognition that becomes trust (it changes the printed label only and
   selects no machine, region or FM), and a path or a byte printed by a
   failure message (this file prints nothing). */
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "bios.h"
#include "../runner/path.h"
#include "../src/sha1.h"

/* File names, sizes and digests of the known dumps. */
static const struct cogmaster__bios_row bios_table[] = {
#include "bios_table.inc"
};

const struct cogmaster__bios_row *cogmaster__bios_rows(size_t *count)
{
    if (count != NULL) {
        *count = sizeof bios_table / sizeof bios_table[0];
    }
    return bios_table;
}

const char *cogmaster__bios_lookup(const struct cogmaster__bios_row *rows, size_t count,
                                   const uint8_t sha1[20], uint32_t size)
{
    if (rows == NULL || sha1 == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        if (rows[i].size == size && memcmp(rows[i].sha1, sha1, 20) == 0) {
            return rows[i].name;
        }
    }
    return NULL;
}

const char *cogmaster__bios_identify(const struct cogmaster__bios_row *rows, size_t count,
                                     const uint8_t *data, size_t size, uint8_t sha1_out[20])
{
    cogmaster__sha1 s;
    uint64_t wide = size;
    cogmaster__sha1_init(&s);
    if (data != NULL) {
        cogmaster__sha1_update(&s, data, size);
    }
    cogmaster__sha1_final(&s, sha1_out);
    if (wide > UINT32_MAX) {
        return NULL;
    }
    return cogmaster__bios_lookup(rows, count, sha1_out, (uint32_t)wide);
}

int cogmaster__bios_read(const char *path, uint8_t *buf, size_t *n)
{
    FILE *f;
    size_t got;
    int failed;
    if (n == NULL) {
        return COGMASTER_BIOS_READ_ERROR;
    }
    *n = 0;
    if (path == NULL || buf == NULL) {
        return COGMASTER_BIOS_READ_ERROR;
    }
    errno = 0;
    f = cogmaster__path_open(path, "rb");
    if (f == NULL) {
        return errno == ENOENT || errno == ENOTDIR ? COGMASTER_BIOS_READ_ABSENT
                                                   : COGMASTER_BIOS_READ_ERROR;
    }
    /* One bounded read, never a loop to the end of the file. A directory
       opens on POSIX and fails here, which counts as an error. */
    got = fread(buf, 1, COGMASTER_BIOS_READ_CAP, f);
    failed = ferror(f);
    (void)fclose(f);
    if (failed != 0) {
        return COGMASTER_BIOS_READ_ERROR;
    }
    *n = got;
    return COGMASTER_BIOS_READ_OK;
}
