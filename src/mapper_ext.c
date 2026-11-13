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
   fault, the rule mapper.c applies to its own banks. The Sega EEPROM board is
   the exception: it pages 16 KiB banks as the Sega board does. */
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

/* Points the 16 KiB slot s at page val modulo the page count, as two 8 KiB
   banks. */
static void ext_pak_slot(cogmaster *m, uint16_t addr, uint32_t s, uint32_t val)
{
    ext_check(m, addr, 2u * val + 1u);
    m->mapper.bank8[2u * s] = ext_bank(m, 2u * val);
    m->mapper.bank8[2u * s + 1u] = ext_bank(m, 2u * val + 1u);
}

/* The 16 KiB page v reduced onto the image, as the Sega board does; records
   mapper_range past the end of an image whose page count is not a power of two. */
static uint8_t ext_page(cogmaster *m, uint16_t addr, uint32_t v)
{
    ext_check(m, addr, 2u * v + 1u);
    return (uint8_t)(m->cart.pages != 0u ? v % m->cart.pages : 0u);
}

/* The window of the Sega EEPROM board: $8000 while $FFFC bit 3 is set. */
#define EEPROM_WINDOW 0x8000u
#define EEPROM_OPEN 0x08u
#define EEPROM_RESET 0x80u

uint8_t cogmaster__mapext_read(const cogmaster *m, uint16_t addr)
{
    switch (m->cart.board) {
    case COGMASTER__BOARD_KOREAN:
    case COGMASTER__BOARD_KOREAN_NB:
    case COGMASTER__BOARD_HICOM:
    case COGMASTER__BOARD_ZEMINA:
    case COGMASTER__BOARD_NEMESIS:
    case COGMASTER__BOARD_4PAK:
        return m->cart.rom[((uint32_t)m->mapper.bank8[addr >> EXT_BANK_BITS] << EXT_BANK_BITS) |
                           (addr & EXT_BANK_MASK)];
    case COGMASTER__BOARD_SEGA_EEPROM:
        /* The window shows the chip's last written bits with bit 1 set and
           Data Out in bit 0; the first 1 KiB is always the first 1 KiB of the
           image (MAME rom.cpp, sega8_eeprom_device::read_cart, pin
           1d7a454ed79e). Everything else is 16 KiB paging. */
        if (addr == EEPROM_WINDOW && (m->mapper.ctrl & EEPROM_OPEN) != 0u) {
            return cogmaster__eeprom_read(&m->eeprom);
        }
        if (addr < 0x0400u) {
            return m->cart.rom[addr];
        }
        return m->cart.rom[((uint32_t)m->mapper.bank[addr >> 14] << 14) | (addr & 0x3FFFu)];
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
    case COGMASTER__BOARD_ZEMINA:
    case COGMASTER__BOARD_NEMESIS:
        /* Writes to exactly $0000-$0003 pick the 8 KiB banks of $8000, $A000,
           $4000 and $6000, value modulo twice the page count; $0000-$3FFF
           otherwise stay fixed. Nemesis is the same board (MAME rom.cpp,
           sega8_zemina_device::write_cart, same pin). Every other write below
           $C000 is a write to ROM. */
        if (addr < 4u) {
            static const uint8_t window[4] = {4u, 5u, 2u, 3u};
            ext_check(m, addr, v);
            p->bank8[window[addr]] = ext_bank(m, v);
        } else if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    case COGMASTER__BOARD_4PAK:
        /* Three registers hold 16 KiB page numbers, each modulo the page
           count: $3FFE for $0000-$3FFF, $7FFF for $4000-$7FFF and $BFFF for
           $8000-$BFFF, where the high bits of the $3FFE value are added to
           the $BFFF value (MAME rom.cpp, sega8_4pak_device::write_cart, same
           pin). raw[0] and raw[1] keep the $3FFE and $BFFF values.
           $FFFD-$FFFF have two readings. MAME: the 4 PAK class overrides
           only read_cart and write_cart, so the Sega registers it inherits
           (sega8_rom_device::write_mapper) still set slots 0, 1 and 2 to the
           value modulo the page count, and $FFFC drives only cartridge RAM,
           which this board lacks. The other reading: the board decodes only
           the three addresses above and the Sega registers do nothing. The
           first is implemented; it is what the one reference that runs the
           image does. */
        if (addr == 0x3FFEu) {
            p->raw[0] = v;
            ext_pak_slot(m, addr, 0u, v);
            ext_pak_slot(m, addr, 2u, (uint32_t)(p->raw[0] & 0x30u) + p->raw[1]);
        } else if (addr == 0x7FFFu) {
            ext_pak_slot(m, addr, 1u, v);
        } else if (addr == 0xBFFFu) {
            p->raw[1] = v;
            ext_pak_slot(m, addr, 2u, (uint32_t)(p->raw[0] & 0x30u) + p->raw[1]);
        } else if (addr >= 0xFFFDu) {
            ext_pak_slot(m, addr, (uint32_t)addr - 0xFFFDu, v);
        } else if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    case COGMASTER__BOARD_SEGA_EEPROM:
        /* $FFFC bit 3 opens the window and bit 7 resets the chip, dropping
           chip select; $FFFD-$FFFF page 16 KiB banks, value modulo the page
           count (MAME rom.cpp, sega8_eeprom_device::write_mapper, pin
           1d7a454ed79e). A write to the open window drives data in, clock and
           chip select from bits 0, 1 and 2 (write_cart). The cartridge RAM
           holds the chip's image: a completed command that changed a byte
           moves the battery generation once. Any other write below $C000 is a
           write to ROM. */
        if (addr == 0xFFFCu) {
            p->ctrl = (uint8_t)(v & (EEPROM_OPEN | EEPROM_RESET));
            if ((v & EEPROM_RESET) != 0u) {
                cogmaster__eeprom_reset(&m->eeprom);
            }
        } else if (addr >= 0xFFFDu) {
            p->bank[addr - 0xFFFDu] = ext_page(m, addr, v);
        } else if (addr == EEPROM_WINDOW && (p->ctrl & EEPROM_OPEN) != 0u) {
            if (cogmaster__eeprom_write(&m->eeprom, m->cart.ram, v) != 0u &&
                cogmaster__battery_length(m) != 0u) {
                m->cart.generation++;
            }
        } else if (addr < 0xC000u) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_CART, COGMASTER__FAULT_ROM_WRITE);
        }
        break;
    default:
        /* The no-bank Korean board has no register (MAME rom.cpp,
           sega8_korean_nb_device, same pin). Writes to ROM change nothing.
           $FFFC-$FFFF are work RAM only. */
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
    case COGMASTER__BOARD_ZEMINA:
    case COGMASTER__BOARD_NEMESIS:
        /* Banks 0 to 5 in order (sega8_zemina_device::late_bank_setup, MAME
           rom.cpp). Nemesis starts with the last 8 KiB bank at $0000
           (sega8_nemesis_device::late_bank_setup). */
        for (uint32_t i = 0; i < EXT_SLOTS; i++) {
            p->bank8[i] = ext_bank(m, i);
        }
        if (m->cart.board == COGMASTER__BOARD_NEMESIS) {
            p->bank8[0] = ext_bank(m, 2u * m->cart.pages - 1u);
        }
        break;
    case COGMASTER__BOARD_4PAK:
        /* All three registers power up at 0 (sega8_4pak_device::device_reset,
           MAME rom.cpp), so each 16 KiB slot shows page 0: banks 0 and 1. */
        for (uint32_t i = 0; i < EXT_SLOTS; i++) {
            p->bank8[i] = ext_bank(m, i & 1u);
        }
        break;
    case COGMASTER__BOARD_SEGA_EEPROM:
        /* Banks 0, 1 and 2 as the Sega board powers up, and a chip with writes
           disabled and no command under way (MAME rom.cpp,
           sega8_eeprom_device::device_reset and late_bank_setup). */
        for (uint32_t i = 0; i < 3u; i++) {
            p->bank[i] = (uint8_t)(m->cart.pages != 0u ? i % m->cart.pages : 0u);
        }
        cogmaster__eeprom_reset(&m->eeprom);
        break;
    default:
        break;
    }
}
