/* The one entry each fuzz target defines. libFuzzer calls it on the nightly
   lane; replay.c calls it for every seed and regression on the pull-request
   lanes. */
#ifndef COGMASTER_FUZZ_H
#define COGMASTER_FUZZ_H

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

#endif
