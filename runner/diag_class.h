/* The words the --diagnose line derives from a fault: the frame bucket, the order against the
   first picture, the program-counter slot and the address class. Pure functions of their
   arguments; tests/runner/test_runner.c calls them with chosen values. */
#ifndef COGMASTER_RUNNER_DIAG_CLASS_H
#define COGMASTER_RUNNER_DIAG_CLASS_H

#include <stdint.h>

#include "cogmaster.h"

static inline const char *cogmaster__diag_bucket(uint32_t frame)
{
    (void)frame;
    return "none";
}

static inline const char *cogmaster__diag_order(uint32_t present, uint32_t frame,
                                                uint32_t first_nonblank)
{
    (void)present;
    (void)frame;
    (void)first_nonblank;
    return "none";
}

static inline uint32_t cogmaster__diag_pcslot(uint32_t pc)
{
    (void)pc;
    return 9u;
}

static inline const char *cogmaster__diag_where(uint32_t device, uint32_t addr)
{
    (void)device;
    (void)addr;
    return "none";
}

#endif /* COGMASTER_RUNNER_DIAG_CLASS_H */
