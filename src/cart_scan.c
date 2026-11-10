/* The board scan: counts LD (nn),A store sites in the first 32 KiB of a
   cartridge body, grouped by the register family the target address belongs
   to. The sweep decodes instruction lengths so an operand byte that happens to
   be $32 is not a store. Locals only, no allocation, at most 32,768 steps
   (SMS-CARTRIDGES section 1.4). */
#include "internal.h"

/* The opcode scan covers the first 32 KiB (section 1.4). */
#define SCAN_SPAN 32768u

/* Bytes in an unprefixed Z80 instruction by opcode: immediate loads and ALU
   operations take 2, 16-bit loads, absolute jumps, calls and loads take 3;
   0 marks the four prefixes, decoded in the sweep (Zilog Z80 CPU User Manual,
   instruction set tables). */
static const uint8_t op_len[256] = {
    1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
    2, 3, 3, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1, 2, 3, 3, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 0, 3, 3, 2, 1, 1, 1, 3, 2, 3, 1, 2, 1, 1, 1, 3, 2, 3, 0, 2, 1,
    1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 0, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 0, 2, 1,
};

/* After DD or FD the opcodes that name (HL) take a displacement byte, so they
   grow by one: INC and DEC (HL), LD (HL),n, LD r,(HL), LD (HL),r (not HALT)
   and the eight ALU operations on (HL). */
static int uses_hl_memory(uint8_t op)
{
    if (op == 0x34u || op == 0x35u || op == 0x36u) {
        return 1;
    }
    if (op >= 0x40u && op <= 0x7Fu) {
        return op != 0x76u && ((op & 7u) == 6u || (op & 0xF8u) == 0x70u);
    }
    return op >= 0x80u && op <= 0xBFu && (op & 7u) == 6u;
}

/* The register family a store target belongs to, or -1 for none. Sega: the
   mapper registers $FFFC-$FFFF. Korean: $A000. Zemina and Nemesis: $0001-$0003
   ($0000 is also the Codemasters register, so it counts for neither). 4 PAK:
   $3FFE, $7FFF, $BFFF. Janggun: $6000 ($4000 and $8000 are Codemasters
   registers, $A000 is Korean's). MAME src/devices/bus/sms_cart/rom.cpp at the
   pinned commit, SMS-CARTRIDGES section 1.4. */
static int family_of(uint32_t t)
{
    if (t >= 0xFFFCu) {
        return COGMASTER__SCAN_SEGA;
    }
    if (t == 0xA000u) {
        return COGMASTER__SCAN_KOREAN;
    }
    if (t >= 0x0001u && t <= 0x0003u) {
        return COGMASTER__SCAN_ZEMINA;
    }
    if (t == 0x3FFEu || t == 0x7FFFu || t == 0xBFFFu) {
        return COGMASTER__SCAN_4PAK;
    }
    return t == 0x6000u ? COGMASTER__SCAN_JANGGUN : -1;
}

/* The length of the instruction at b[i], with at least one more byte ahead
   when it has a prefix (the sweep guards i + 1 < end). A prefix followed by
   another prefix counts as one byte: the first changes nothing. */
static uint32_t length_at(const uint8_t *b, size_t i, size_t end)
{
    uint8_t op = b[i];
    uint32_t len = op_len[op];
    if (len != 0u) {
        return len;
    }
    if (i + 1u >= end) {
        return 1u;
    }
    uint8_t next = b[i + 1u];
    if (op == 0xCBu) {
        return 2u;
    }
    if (op == 0xEDu) {
        /* LD (nn),rr and LD rr,(nn) are four bytes; the rest two. */
        return (next & 0xC7u) == 0x43u ? 4u : 2u;
    }
    /* DD and FD. */
    if (op_len[next] == 0u) {
        return next == 0xCBu ? 4u : 1u;
    }
    return 1u + op_len[next] + (uses_hl_memory(next) ? 1u : 0u);
}

void cogmaster__cart_scan(const uint8_t *b, size_t body, struct cogmaster__scan *out)
{
    size_t end = body < SCAN_SPAN ? body : SCAN_SPAN;
    size_t i = 0u;
    for (uint32_t k = 0; k < COGMASTER__SCAN_FAMILIES; k++) {
        out->count[k] = 0u;
    }
    while (i < end) {
        if (b[i] == 0x32u && i + 3u <= end) {
            int f = family_of((uint32_t)b[i + 1u] | (uint32_t)b[i + 2u] << 8);
            if (f >= 0) {
                out->count[f]++;
            }
        }
        i += length_at(b, i, end);
    }
}
