/* SHA-256 (FIPS 180-4), owned code for the runner's frame hashes. */
#ifndef COGMASTER_RUNNER_SHA256_H
#define COGMASTER_RUNNER_SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct cogmaster__sha256 {
    uint32_t h[8];     /* intermediate hash value */
    uint8_t block[64]; /* bytes of the current, unfinished block */
    uint32_t used;     /* bytes in block */
    uint64_t total;    /* message length in bytes */
} cogmaster__sha256;

void cogmaster__sha256_init(cogmaster__sha256 *s);
void cogmaster__sha256_update(cogmaster__sha256 *s, const void *data, size_t len);
void cogmaster__sha256_final(cogmaster__sha256 *s, uint8_t digest[32]);

/* Writes the digest as 64 lowercase hex digits and a terminating NUL. */
void cogmaster__sha256_hex(const uint8_t digest[32], char hex[65]);

#endif /* COGMASTER_RUNNER_SHA256_H */
