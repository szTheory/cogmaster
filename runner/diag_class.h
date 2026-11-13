/* The words the --diagnose line derives from a fault: the frame bucket, the order against the
   first picture, the program-counter slot and the address class. Pure functions of their
   arguments, so tests/runner/test_runner.c calls them with chosen values. */
#ifndef COGMASTER_RUNNER_DIAG_CLASS_H
#define COGMASTER_RUNNER_DIAG_CLASS_H

#include <stdint.h>

#include "cogmaster.h"

/* The core keeps one first fault, with its 0-based frame. A fault at frame 599 happened within
   the first 600 run calls, so a frame-600 verdict sees it; one at 600 or later it does not. */
static inline const char *cogmaster__diag_bucket(uint32_t frame)
{
    return frame == 0u    ? "0"
           : frame < 10u  ? "1-9"
           : frame < 100u ? "10-99"
           : frame < 600u ? "100-599"
                          : "600+";
}

/* A fault at 0-based frame f was recorded during run call f + 1, so it is before the first
   non-blank picture (a 1-based call) when f + 1 <= first_nonblank, written without the wrap that
   f + 1 has at 4294967295. first_nonblank 0 means no picture ever showed. */
static inline const char *cogmaster__diag_order(uint32_t present, uint32_t frame,
                                                uint32_t first_nonblank)
{
    return !present                 ? "none"
           : first_nonblank == 0u   ? "nopicture"
           : frame < first_nonblank ? "before"
                                    : "after";
}

/* Where the program counter sat in the Z80 address map (.planning/preparation/SMS-SYSTEM.md
   section 3): 0 is the first 256 bytes, where the reset and restart vectors live, 1 the rest of
   slot 0, 2 slot 1, 3 slot 2 and 4 the work RAM and its mirror. */
static inline uint32_t cogmaster__diag_pcslot(uint32_t pc)
{
    return pc < 0x0100u ? 0u : pc < 0x4000u ? 1u : pc < 0x8000u ? 2u : pc < 0xC000u ? 3u : 4u;
}

/* The kind of address a fault names, from the faulting device first. The cartridge answers
   $0000-$7FFF from slots 0 and 1 (rom) and $8000-$BFFF from slot 2 (slot2); the mapper class
   covers every mapper register, the Codemasters ones at $0000, $4000 and $8000 included; the
   controller and an unnamed device are both port accesses. */
static inline const char *cogmaster__diag_where(uint32_t device, uint32_t addr)
{
    switch (device) {
    case COGMASTER_FAULT_DEVICE_CART:
        return addr < 0x8000u ? "rom" : "slot2";
    case COGMASTER_FAULT_DEVICE_RAM:
        return "ram";
    case COGMASTER_FAULT_DEVICE_MAPPER:
        return "mapper";
    case COGMASTER_FAULT_DEVICE_CTRL:
    case COGMASTER_FAULT_DEVICE_NONE:
        return "port";
    case COGMASTER_FAULT_DEVICE_CPU:
        return "cpu";
    case COGMASTER_FAULT_DEVICE_VDP:
        return "vdp";
    default:
        return "unknown";
    }
}

#endif /* COGMASTER_RUNNER_DIAG_CLASS_H */
