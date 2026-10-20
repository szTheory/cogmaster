/* MD5 (RFC 1321) for the runner's --probe digests: the RetroAchievements
   hash of a Master System or Game Gear file is the MD5 of the whole file. */
#ifndef COGMASTER_RUNNER_MD5_H
#define COGMASTER_RUNNER_MD5_H

#include <stddef.h>
#include <stdint.h>

typedef struct cogmaster__md5 {
    uint32_t h[4];     /* the state A, B, C, D */
    uint8_t block[64]; /* bytes of the current, unfinished block */
    uint32_t used;     /* bytes in block */
    uint64_t total;    /* message length in bytes */
} cogmaster__md5;

void cogmaster__md5_init(cogmaster__md5 *s);
void cogmaster__md5_update(cogmaster__md5 *s, const void *data, size_t len);
void cogmaster__md5_final(cogmaster__md5 *s, uint8_t digest[16]);

#endif /* COGMASTER_RUNNER_MD5_H */
