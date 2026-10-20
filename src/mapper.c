/* The cartridge boards: no mapper, the Sega mapper and the Codemasters
   mapper, dispatched by a switch on the board (SMS-CARTRIDGES section 2).
   The image is padded to whole 16 KiB pages, so a bank number is reduced
   modulo the page count when it is written and every read stays inside the
   image (section 2.4). */
#include "internal.h"

#define SLOT_MASK 0x3FFFu
/* Slot 0's first 1 KiB stays on bank 0 on the Sega board, so the interrupt
   vectors never move (section 2.1). */
#define SEGA_FIXED 0x0400u
/* $FFFC: b3 maps cartridge RAM into slot 2, b2 picks its 16 KiB bank, and
   b1-0 shift every later bank write (section 2.1). */
#define SEGA_RAM_ON 0x08u
#define SEGA_RAM_BANK 0x04u
#define SEGA_SHIFT 0x03u
/* Codemasters: bit 7 of a $4000 write maps 8 KiB of cartridge RAM at
   $A000-$BFFF (section 2.2). */
#define CM_RAM_ON 0x80u
#define CM_RAM_BASE 0xA000u

/* The bank a written value selects: the value plus the Sega shift, modulo
   the page count. */
static uint8_t bank_of(const cogmaster *m, uint32_t v)
{
    return (uint8_t)(m->cart.pages != 0u ? v % m->cart.pages : 0u);
}

static uint8_t rom_at(const cogmaster *m, uint32_t bank, uint16_t addr)
{
    return m->cart.rom[bank << 14 | (addr & SLOT_MASK)];
}

void cogmaster__mapper_reset(cogmaster *m)
{
    struct cogmaster__mapper *p = &m->mapper;
    p->ctrl = 0u;
    switch (m->cart.board) {
    case COGMASTER__BOARD_SEGA:
        /* The 315-5235 powers up with $FFFD-$FFFF at 0, 1, 2, which the
           BIOS also leaves (section 2.1; SMS-SYSTEM C7). */
        p->bank[0] = bank_of(m, 0u);
        p->bank[1] = bank_of(m, 1u);
        p->bank[2] = bank_of(m, 2u);
        break;
    case COGMASTER__BOARD_CODEMASTERS:
        /* Power-on banks 0, 1, 0 (section 2.2). */
        p->bank[0] = bank_of(m, 0u);
        p->bank[1] = bank_of(m, 1u);
        p->bank[2] = bank_of(m, 0u);
        break;
    default:
        p->bank[0] = 0u;
        p->bank[1] = 0u;
        p->bank[2] = 0u;
        break;
    }
}

uint8_t cogmaster__mapper_read(const cogmaster *m, uint16_t addr)
{
    const struct cogmaster__mapper *p = &m->mapper;
    uint32_t slot = (uint32_t)addr >> 14;
    switch (m->cart.board) {
    case COGMASTER__BOARD_SEGA:
        if (addr < SEGA_FIXED) {
            return m->cart.rom[addr];
        }
        if (slot == 2u && (p->ctrl & SEGA_RAM_ON) != 0u) {
            uint32_t base = (p->ctrl & SEGA_RAM_BANK) != 0u ? 0x4000u : 0u;
            return m->cart.ram[(base | (addr & SLOT_MASK)) & (m->cart.ram_size - 1u)];
        }
        return rom_at(m, p->bank[slot], addr);
    case COGMASTER__BOARD_CODEMASTERS:
        if (addr >= CM_RAM_BASE && (p->ctrl & CM_RAM_ON) != 0u) {
            return m->cart.ram[addr & (m->cart.ram_size - 1u)];
        }
        return rom_at(m, p->bank[slot], addr);
    case COGMASTER__BOARD_ROM_ONLY:
        /* Linear: the image, then $FF up to $BFFF (section 2.1). */
        return addr < m->cart.size ? m->cart.rom[addr] : 0xFFu;
    default:
        return 0xFFu;
    }
}

void cogmaster__mapper_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    struct cogmaster__mapper *p = &m->mapper;
    switch (m->cart.board) {
    case COGMASTER__BOARD_SEGA:
        if (addr >= 0xFFFCu) {
            if (addr == 0xFFFCu) {
                p->ctrl = v;
            } else {
                /* Shift codes 1, 2 and 3 add $18, $10 and $08. */
                static const uint8_t shift[4] = {0x00u, 0x18u, 0x10u, 0x08u};
                p->bank[addr - 0xFFFDu] = bank_of(m, (uint32_t)v + shift[p->ctrl & SEGA_SHIFT]);
            }
        } else if (addr >= 0x8000u && (p->ctrl & SEGA_RAM_ON) != 0u) {
            uint32_t base = (p->ctrl & SEGA_RAM_BANK) != 0u ? 0x4000u : 0u;
            m->cart.ram[(base | (addr & SLOT_MASK)) & (m->cart.ram_size - 1u)] = v;
        }
        break;
    case COGMASTER__BOARD_CODEMASTERS:
        /* The registers sit at $0000, $4000 and $8000 exactly (section 2.2). */
        if ((addr & SLOT_MASK) == 0u && addr < 0xC000u) {
            uint32_t slot = (uint32_t)addr >> 14;
            p->bank[slot] = bank_of(m, v);
            if (slot == 1u) {
                p->ctrl = v & CM_RAM_ON;
            }
        } else if (addr >= CM_RAM_BASE && addr < 0xC000u && (p->ctrl & CM_RAM_ON) != 0u) {
            m->cart.ram[addr & (m->cart.ram_size - 1u)] = v;
        }
        break;
    default:
        /* Writes to ROM change nothing. */
        break;
    }
}
