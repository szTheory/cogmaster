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

/* The class of the video mode an unsupported-mode fault names: the low nibble of the fault
   address is the mode select M4 M3 M2 M1 (.planning/preparation/SMS-VDP.md section 2.5). 0 to 7
   have the mode 4 bit clear, so they are the TMS9918 modes: graphics1 (0), text (1), graphics2
   (2), multicolor (4) and the mixed combinations (3, 5, 6, 7). 8, 10 and 12 are mode 4 on every
   chip; 9 and 13 are the invalid text mode everywhere; 11, 14 and 15 are mode 4 on SMS2 and Game
   Gear but the invalid text mode on SMS1. The core accepts the mode 4 nibbles, so "mode4" should
   never print; it is here so a future change shows up as a word, not as a surprise. */
static inline const char *cogmaster__diag_vdpmode(uint32_t kind, uint32_t addr)
{
    if (kind != COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE)
        return "none";
    switch (addr & 15u) {
    case 0u:
        return "graphics1";
    case 1u:
        return "text";
    case 2u:
        return "graphics2";
    case 4u:
        return "multicolor";
    case 3u:
    case 5u:
    case 6u:
    case 7u:
        return "mixed";
    case 8u:
    case 10u:
    case 12u:
        return "mode4";
    case 9u:
    case 13u:
        return "invalid";
    default:
        return "sms2only";
    }
}

/* Which mapper control address a mapper-range fault names: $FFFC is the Sega RAM-select and bank
   shift register, $FFFD to $FFFF the three bank registers, and any other address is a range the
   mapper does not decode (.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md section 2.1).
   Other kinds name no mapper address. */
static inline const char *cogmaster__diag_faddr(uint32_t kind, uint32_t addr)
{
    if (kind != COGMASTER_FAULT_KIND_MAPPER_RANGE)
        return "none";
    addr &= 0xFFFFu;
    return addr == 0xFFFCu ? "fffc" : addr >= 0xFFFDu ? "fffd-f" : "other";
}

/* Whether the image body is a power-of-two count of 16 KiB pages, the page count src/cart.c
   rounds a body up to. A body of 0 bytes has no pages and answers n. */
static inline const char *cogmaster__diag_pow2(uint32_t bytes)
{
    uint32_t pages = (uint32_t)(((uint64_t)bytes + 16383u) / 16384u);
    return pages != 0u && (pages & (pages - 1u)) == 0u ? "y" : "n";
}

/* The class of the address a rom-write fault names. The other mappers answer writes in the
   cartridge area at their own registers (.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md
   sections 2.1 and 2.2): $0000 is shared by the Codemasters and Zemina boards, $0001 to $0003 are
   more Zemina registers, $4000 and $8000 the other Codemasters ones, $A000 the Korean one and
   $3FFE, $7FFF and $BFFF the 4 Pak All Action ones. The register classes come before the plain
   address ranges, which split the rest at $0100, the end of slot 0's vectors, $8000 and $C000. */
static inline const char *cogmaster__diag_wclass(uint32_t kind, uint32_t addr)
{
    if (kind != COGMASTER_FAULT_KIND_ROM_WRITE)
        return "none";
    addr &= 0xFFFFu;
    if (addr == 0x0000u)
        return "reg-0";
    if (addr <= 0x0003u)
        return "reg-zm";
    if (addr == 0x4000u || addr == 0x8000u)
        return "reg-cm";
    if (addr == 0xA000u)
        return "reg-korean";
    if (addr == 0x3FFEu || addr == 0x7FFFu || addr == 0xBFFFu)
        return "reg-4pak";
    return addr < 0x0100u ? "lt100" : addr < 0x8000u ? "rom" : addr < 0xC000u ? "slot2" : "high";
}

/* The class of the port an unmapped access names: the port is the low byte of the address bus
   (.planning/preparation/SMS-SYSTEM.md section 4). $00 to $06 are below the memory control port,
   $3E and $3F the memory and I/O control ports, $40 to $BF the counters, PSG and VDP. */
static inline const char *cogmaster__diag_pclass(uint32_t kind, uint32_t device, uint32_t addr)
{
    if ((kind != COGMASTER_FAULT_KIND_UNMAPPED_READ && kind != COGMASTER_FAULT_KIND_UNMAPPED_WRITE) ||
        (device != COGMASTER_FAULT_DEVICE_CTRL && device != COGMASTER_FAULT_DEVICE_NONE))
        return "none";
    uint32_t port = addr & 0xFFu;
    return port <= 0x06u ? "00-06" : port >= 0xC0u ? "high" : port >= 0x40u ? "40-bf"
           : port >= 0x3Eu ? "3e-3f" : "lt40";
}

/* The image body size as a class: the smallest listed limit that holds it. */
static inline const char *cogmaster__diag_size(uint32_t bytes)
{
    return bytes <= 0x4000u    ? "le16k"
           : bytes <= 0x8000u  ? "le32k"
           : bytes <= 0xC000u  ? "le48k"
           : bytes <= 0x10000u ? "le64k"
           : bytes <= 0x20000u ? "le128k"
           : bytes <= 0x40000u ? "le256k"
           : bytes <= 0x80000u ? "le512k"
                               : "gt512k";
}

/* Whether the probe found a header and its checksum agreed (.planning/preparation/
   SMS-CARTRIDGES-AND-PERIPHERALS.md section 1): header_at is 0 when there is none. */
static inline const char *cogmaster__diag_header(uint32_t header_at, uint32_t checksum_ok)
{
    return header_at == 0u ? "none" : checksum_ok ? "valid" : "bad";
}

#endif /* COGMASTER_RUNNER_DIAG_CLASS_H */
