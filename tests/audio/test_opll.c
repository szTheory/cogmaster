/* The YM2413 chip module on its own: cogmaster-test-fm <case> [file]. The chip is driven
   through its three entry points and read back through its public structure. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opll.h"
#include "opll_stream.h"
#include "sha256.h"
#include "../check.h"

#define LOCKSTEP_SAMPLES 2000000u

/* The port's output over the seeded stream, hashed as the original's int16 samples are:
   two bytes per sample, low byte first. */
static void port_stream_digest(uint32_t n, char hex[65])
{
    static struct cogmaster__opll chip;
    struct opll_stream st;
    cogmaster__sha256 h;
    uint8_t digest[32];
    uint32_t reg;
    uint32_t val;

    cogmaster__opll_reset(&chip);
    opll_stream_init(&st, OPLL_STREAM_SEED);
    cogmaster__sha256_init(&h);
    for (uint32_t i = 0; i < n; i++) {
        if (opll_stream_step(&st, &reg, &val)) {
            cogmaster__opll_write(&chip, reg, val);
        }
        const uint16_t s = (uint16_t)cogmaster__opll_sample(&chip);
        const uint8_t b[2] = {(uint8_t)(s & 0xffu), (uint8_t)(s >> 8)};
        cogmaster__sha256_update(&h, b, 2);
    }
    cogmaster__sha256_final(&h, digest);
    cogmaster__sha256_hex(digest, hex);
}

/* The port over the seeded register stream equals the digest the unmodified original
   produced once; the file names the original's source by its SHA-256. */
static void lockstep(const char *path)
{
    char line[512];
    char frozen[65];
    char hex[65];
    unsigned samples = 0;
    unsigned long long seed = 0;
    FILE *f = fopen(path, "r");

    if (f == NULL || fgets(line, sizeof line, f) == NULL) {
        fprintf(stderr, "lockstep: cannot read %s\n", path);
        check_failures++;
        if (f != NULL) {
            fclose(f);
        }
        return;
    }
    fclose(f);
    CHECK(sscanf(line, "samples %u seed 0x%llx sha256 %64s", &samples, &seed, frozen) == 3);
    CHECK_EQ_U64(samples, LOCKSTEP_SAMPLES);
    CHECK_EQ_HEX(seed, OPLL_STREAM_SEED);
    port_stream_digest(LOCKSTEP_SAMPLES, hex);
    if (strcmp(hex, frozen) != 0) {
        fprintf(stderr, "lockstep: port digest %s, frozen digest %s\n", hex, frozen);
        check_failures++;
    }
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "lockstep") == 0) {
        lockstep(argv[2]);
        CHECK_DONE();
    }
    fprintf(stderr, "usage: cogmaster-test-fm lockstep <digest file>\n");
    return 2;
}
