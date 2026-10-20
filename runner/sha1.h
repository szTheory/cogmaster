/* SHA-1 (RFC 3174) for the runner's --probe digests. */
#ifndef COGMASTER_RUNNER_SHA1_H
#define COGMASTER_RUNNER_SHA1_H

#include <stddef.h>
#include <stdint.h>

typedef struct cogmaster__sha1 {
    uint32_t h[5];     /* intermediate hash value */
    uint8_t block[64]; /* bytes of the current, unfinished block */
    uint32_t used;     /* bytes in block */
    uint64_t total;    /* message length in bytes */
} cogmaster__sha1;

void cogmaster__sha1_init(cogmaster__sha1 *s);
void cogmaster__sha1_update(cogmaster__sha1 *s, const void *data, size_t len);
void cogmaster__sha1_final(cogmaster__sha1 *s, uint8_t digest[20]);

#endif /* COGMASTER_RUNNER_SHA1_H */
