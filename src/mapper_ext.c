/* The cartridge boards outside the three arms of mapper.c: one switch arm per
   board, reached from the default arm of each mapper.c switch, so the Sega,
   Codemasters and ROM-only paths never see them. The dispatch is a switch and
   not a table of function pointers because a pointer table built at load time
   in a position-independent library lands in a relocated read-only data
   section, which the symbol check on the built library rejects.

   These boards page 8 KiB banks. The state holds the bank shown in each of the
   six 8 KiB windows of $0000-$BFFF, always below twice the page count, so one
   read serves every board. A bank number that runs past the image wraps onto
   it; past a power-of-two image the high bits are simply not wired, and past
   any other image there is no ROM to wrap onto, so it is a mapper_range
   fault, the rule mapper.c applies to its own banks. */
#include "internal.h"

#define EXT_SLOTS 6u
#define EXT_BANK_BITS 13u
#define EXT_BANK_MASK 0x1FFFu

/* The 8 KiB bank number i reduced onto the image; 0 when there is no image. */
static uint8_t ext_bank(const cogmaster *m, uint32_t i)
{
    uint32_t n = 2u * m->cart.pages;
    return (uint8_t)(n != 0u ? i % n : 0u);
}

/* Records mapper_range when the highest 8 KiB bank a register write selects is
   past the end of an image whose page count is not a power of two. */
static void ext_check(cogmaster *m, uint16_t addr, uint32_t highest)
{
    uint32_t pages = m->cart.pages;
    if (highest >= 2u * pages && (pages & (pages - 1u)) != 0u) {
        cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_MAPPER, COGMASTER__FAULT_MAPPER_RANGE);
    }
}

uint8_t cogmaster__mapext_read(const cogmaster *m, uint16_t addr)
{
    switch (m->cart.board) {
    case COGMASTER__BOARD_KOREAN:
    case COGMASTER__BOARD_KOREAN_NB:
    case COGMASTER__BOARD_HICOM:
        return m->cart.rom[((uint32_t)m->mapper.bank8[addr >> EXT_BANK_BITS] << EXT_BANK_BITS) |
                           (addr & EXT_BANK_MASK)];
    default:
        return 0xFFu;
    }
}

void cogmaster__mapext_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    struct cogmaster__mapper *p = &m->mapper;
    switch (m->cart.board) {
    case COGMASTER__BOARD_KOREAN:
        /* The one register is a write to exactly $A000: it picks the 16 KiB
           bank of $8000-$BFFF, value modulo the page count (MAME
           src/devices/bus/sega8/rom.cpp, sega8_korean_device, pin
           1d7a454ed79e). Every other write below $C000 is a write to ROM. */
        if (addr == 0xA000u) {
            ext_check(m, addr, 2u * (uint32_t)v + 1u);
            p->bank8[4] = ext_bank(m, 2u * (uint32_t)v);
            p->bank8[5] = ext_bank(m, 2u * (uint32_t)v + 1u);
        } else if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    case COGMASTER__BOARD_HICOM:
        /* A write to $FFFF picks the 32 KiB bank of $0000-$7FFF; $8000-$BFFF
           keep showing the first 16 KiB (MAME rom.cpp, sega8_hicom_device,
           same pin). MAME's index can run past an odd page count; here every
           bank is reduced onto the image. */
        if (addr == 0xFFFFu) {
            ext_check(m, addr, 4u * (uint32_t)v + 3u);
            for (uint32_t i = 0; i < 4u; i++) {
                p->bank8[i] = ext_bank(m, 4u * (uint32_t)v + i);
            }
        } else if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    default:
        /* The no-bank Korean board has no register (MAME rom.cpp,
           sega8_korean_nb_device, same pin). Writes to ROM change nothing.
           $FFFC-$FFFF are work RAM only. */
        if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    }
}

void cogmaster__mapext_reset(cogmaster *m)
{
    struct cogmaster__mapper *p = &m->mapper;
    switch (m->cart.board) {
    case COGMASTER__BOARD_KOREAN:
    case COGMASTER__BOARD_KOREAN_NB:
        /* Banks 0 to 5 in order: the image from $0000, 16 KiB bank 2 at $8000
           (sega8_korean_device and sega8_korean_nb_device, MAME rom.cpp). A
           32 KiB image mirrors its first page there. */
        for (uint32_t i = 0; i < EXT_SLOTS; i++) {
            p->bank8[i] = ext_bank(m, i);
        }
        break;
    case COGMASTER__BOARD_HICOM:
        /* 32 KiB bank 0 at $0000, the first 16 KiB again at $8000. */
        for (uint32_t i = 0; i < EXT_SLOTS; i++) {
            p->bank8[i] = ext_bank(m, i < 4u ? i : i - 4u);
        }
        break;
    default:
        /* The caller cleared every bank. */
        break;
    }
}
