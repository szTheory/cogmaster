/* CRC-32 (reflected polynomial 0xEDB88320) for the runner's PNG chunks. The
   core has its own CRC under another name; a runner definition of the core's
   name would replace it silently when the static library is linked. */
#ifndef COGMASTER_RUNNER_CRC32_H
#define COGMASTER_RUNNER_CRC32_H

#include <stddef.h>
#include <stdint.h>

/* Continues a CRC-32 over n bytes at p. Start with crc = 0; pass the result
   back in to extend it. */
uint32_t cogmaster__runner_crc32(uint32_t crc, const uint8_t *p, size_t n);

#endif /* COGMASTER_RUNNER_CRC32_H */
