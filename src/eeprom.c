/* The 93C46 serial EEPROM of a Game Gear cartridge, as a pure state machine over
   the byte a game writes to the cartridge window. It follows the Microchip
   93AA46/93LC46/93C46 1K Microwire serial EEPROM data sheet in its x16
   organisation: 64 words of 16 bits. Sections cited below are that data sheet's
   instruction set table, READ, EWEN and EWDS, WRITE, ERASE, WRAL, ERAL, the Data
   Out pin and the programming cycle.

   A command is a start bit (DI high on a rising clock edge with chip select
   high), 2 opcode bits and 6 address bits, then 16 data bits for WRITE and WRAL.
   Clocks before the start bit are ignored. Bits are taken on the rising clock
   edge, most significant first. Chip select falling abandons a command that is
   not complete and, for a complete WRITE, ERASE, WRAL or ERAL, is where the
   self-timed programming cycle runs. The chip is ready at once here: Data Out is
   high except while a READ shifts data, so a game that polls for ready sees it
   ready on its first look.

   The image is the 128 bytes of cartridge RAM, one word at two bytes, low byte
   first; an erased word is all ones. */
#include "internal.h"

#define EE_WORDS 64u
#define EE_START_BITS 8u
#define EE_DATA_BITS 16u

#define EE_DO_LOW 0x01u
#define EE_WRITE_ON 0x02u
#define EE_CS 0x04u
#define EE_CLK 0x08u
#define EE_PENDING 0x10u

enum { EE_IDLE, EE_COMMAND, EE_DATA, EE_READ, EE_DONE };

/* Opcode bits of the instruction set table: the top two bits of cmd, and for
   opcode 00 the two bits below them. */
#define EE_OP_EXT 0u
#define EE_OP_WRITE 1u
#define EE_OP_READ 2u
#define EE_OP_ERASE 3u
#define EE_EXT_EWDS 0u
#define EE_EXT_WRAL 1u
#define EE_EXT_ERAL 2u
#define EE_EXT_EWEN 3u

/* The two functions that own the little-endian word order of the image. */
static uint16_t ee_word(const uint8_t *image, uint32_t w)
{
    return (uint16_t)(image[2u * w] | (image[2u * w + 1u] << 8));
}

/* Stores v in word w; 1 when a byte changed. */
static uint32_t ee_store(uint8_t *image, uint32_t w, uint16_t v)
{
    uint8_t lo = (uint8_t)(v & 0xFFu), hi = (uint8_t)(v >> 8);
    uint32_t changed = (image[2u * w] != lo || image[2u * w + 1u] != hi) ? 1u : 0u;
    image[2u * w] = lo;
    image[2u * w + 1u] = hi;
    return changed;
}

void cogmaster__eeprom_reset(struct cogmaster__eeprom *e)
{
    e->last = 0u;
    e->phase = EE_IDLE;
    e->count = 0u;
    e->cmd = 0u;
    e->data = 0u;
    e->flags = 0u;
}

/* The command is complete. The data sheet ignores WRITE, ERASE, WRAL and ERAL
   while writes are disabled (power-up and after EWDS); EWEN and EWDS act at once. */
static void ee_decode(struct cogmaster__eeprom *e)
{
    uint32_t op = (uint32_t)e->cmd >> 6, ext = ((uint32_t)e->cmd >> 4) & 3u;
    e->count = 0u;
    e->phase = EE_DONE;
    if (op == EE_OP_READ) {
        e->phase = EE_READ; /* READ: a dummy 0 shows first, then D15 to D0 */
        e->flags |= EE_DO_LOW;
    } else if (op == EE_OP_WRITE || (op == EE_OP_EXT && ext == EE_EXT_WRAL)) {
        e->phase = EE_DATA;
    } else if (op == EE_OP_EXT && ext == EE_EXT_EWEN) {
        e->flags |= EE_WRITE_ON;
    } else if (op == EE_OP_EXT && ext == EE_EXT_EWDS) {
        e->flags = (uint8_t)(e->flags & ~EE_WRITE_ON);
    } else if (e->flags & EE_WRITE_ON) {
        e->flags |= EE_PENDING; /* ERASE or ERAL: runs when chip select falls */
    }
}

/* One rising clock edge with chip select high. */
static void ee_clock(struct cogmaster__eeprom *e, uint32_t di, const uint8_t *image)
{
    switch (e->phase) {
    case EE_IDLE:
        if (di) {
            e->phase = EE_COMMAND;
            e->count = 0u;
            e->cmd = 0u;
        }
        break;
    case EE_COMMAND:
        e->cmd = (uint8_t)(((uint32_t)e->cmd << 1) | di);
        if (++e->count == EE_START_BITS)
            ee_decode(e);
        break;
    case EE_DATA:
        e->data = (uint16_t)(((uint32_t)e->data << 1) | di);
        if (++e->count == EE_DATA_BITS) {
            e->count = 0u;
            e->phase = EE_DONE;
            if (e->flags & EE_WRITE_ON)
                e->flags |= EE_PENDING; /* WRITE or WRAL: runs when chip select falls */
        }
        break;
    case EE_READ: {
        /* READ goes on to the next word for as long as the clock runs, and past
           word 63 to word 0 (READ, sequential read). */
        uint32_t bit = ((uint32_t)ee_word(image, e->cmd & 0x3Fu) >> (15u - e->count)) & 1u;
        e->flags = (uint8_t)(bit ? e->flags & ~EE_DO_LOW : e->flags | EE_DO_LOW);
        if (++e->count == EE_DATA_BITS) {
            e->count = 0u;
            e->cmd = (uint8_t)((e->cmd & 0xC0u) | ((e->cmd + 1u) & 0x3Fu));
        }
        break;
    }
    default:
        break; /* clocks after a complete command change nothing */
    }
}

/* Chip select fell after a complete write-type command: the programming cycle.
   Returns 1 when a byte of the image changed. */
static uint32_t ee_program(const struct cogmaster__eeprom *e, uint8_t *image)
{
    uint32_t op = (uint32_t)e->cmd >> 6, w, changed = 0u;
    if (op == EE_OP_WRITE) /* WRITE and ERASE erase the word themselves */
        return ee_store(image, e->cmd & 0x3Fu, e->data);
    if (op == EE_OP_ERASE)
        return ee_store(image, e->cmd & 0x3Fu, 0xFFFFu);
    /* WRAL writes the data into every word, ERAL sets every word to ones. */
    for (w = 0u; w < EE_WORDS; w++)
        changed |= ee_store(image, w, (((e->cmd >> 4) & 3u) == EE_EXT_WRAL) ? e->data : 0xFFFFu);
    return changed;
}

uint32_t cogmaster__eeprom_write(struct cogmaster__eeprom *e, uint8_t *image, uint8_t v)
{
    uint32_t di = v & 1u, clk = (v >> 1) & 1u, cs = (v >> 2) & 1u, changed = 0u;
    e->last = v;
    /* Data in, then chip select, then clock: the order a game's single store of
       DI, CS and CLK together is taken in. */
    if (!cs && (e->flags & EE_CS)) {
        if (e->flags & EE_PENDING)
            changed = ee_program(e, image);
        e->phase = EE_IDLE;
        e->count = 0u;
        e->cmd = 0u;
        e->data = 0u;
        e->flags = (uint8_t)(e->flags & EE_WRITE_ON);
    } else if (cs) {
        e->flags |= EE_CS;
    }
    if (cs && clk && !(e->flags & EE_CLK))
        ee_clock(e, di, image);
    e->flags = (uint8_t)(clk ? e->flags | EE_CLK : e->flags & ~EE_CLK);
    return changed;
}

/* The byte a game reads back from the window: the bits it last wrote, with bit 1
   set and bit 0 showing Data Out. */
uint8_t cogmaster__eeprom_read(const struct cogmaster__eeprom *e)
{
    return (uint8_t)((e->last & 0xFCu) | 0x02u | ((e->flags & EE_DO_LOW) ? 0u : 1u));
}
