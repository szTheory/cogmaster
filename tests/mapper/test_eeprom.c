/* The 93C46 as a device with no board: framed commands clocked bit by bit through
   the one-byte window the cartridge gives a game (b0 data in, b1 clock, b2 chip
   select), with the expected image worked out by hand from the data sheet. */
#include <stdint.h>
#include <string.h>
#include "internal.h"
#include "../check.h"

#define WORDS 64u

struct dev {
    struct cogmaster__eeprom e;
    uint8_t img[128];
    uint32_t changes;
};

static void put(struct dev *d, uint8_t v)
{
    d->changes += cogmaster__eeprom_write(&d->e, d->img, v);
}
static unsigned out(const struct dev *d)
{
    return cogmaster__eeprom_read(&d->e) & 1u;
}
static void cs_up(struct dev *d)
{
    put(d, 4);
}
static void cs_down(struct dev *d)
{
    put(d, 0);
}
/* One clock pulse with chip select high and data in di. */
static void pulse(struct dev *d, unsigned di)
{
    put(d, (uint8_t)(4u | di));
    put(d, (uint8_t)(4u | 2u | di));
    put(d, (uint8_t)(4u | di));
}
static void bits(struct dev *d, uint32_t v, unsigned n)
{
    while (n-- > 0u)
        pulse(d, (v >> n) & 1u);
}
/* Start bit, opcode, 6 bits of address or sub-code. */
static void send(struct dev *d, uint32_t op, uint32_t arg)
{
    bits(d, 1u, 1u);
    bits(d, op, 2u);
    bits(d, arg, 6u);
}
static void reset(struct dev *d, uint8_t fill)
{
    cogmaster__eeprom_reset(&d->e);
    memset(d->img, fill, sizeof d->img);
    d->changes = 0u;
}
static void ewen(struct dev *d)
{
    cs_up(d);
    send(d, 0u, 0x30u);
    cs_down(d);
}
static void ewds(struct dev *d)
{
    cs_up(d);
    send(d, 0u, 0x00u);
    cs_down(d);
}
static void write_word(struct dev *d, uint32_t w, uint16_t v)
{
    cs_up(d);
    send(d, 1u, w);
    bits(d, v, 16u);
    cs_down(d);
}
static void erase_word(struct dev *d, uint32_t w)
{
    cs_up(d);
    send(d, 3u, w);
    cs_down(d);
}
static void wral(struct dev *d, uint16_t v)
{
    cs_up(d);
    send(d, 0u, 0x10u);
    bits(d, v, 16u);
    cs_down(d);
}
static void eral(struct dev *d)
{
    cs_up(d);
    send(d, 0u, 0x20u);
    cs_down(d);
}
/* READ from word w: the dummy bit must be 0, then n words follow. */
static void read_words(struct dev *d, uint32_t w, unsigned n, uint16_t *words)
{
    cs_up(d);
    send(d, 2u, w);
    CHECK_EQ_U64(out(d), 0u);
    for (unsigned i = 0; i < n; i++) {
        uint16_t v = 0;
        for (unsigned b = 0; b < 16u; b++) {
            pulse(d, 0u);
            v = (uint16_t)((v << 1) | (uint16_t)out(d));
        }
        words[i] = v;
    }
    cs_down(d);
}
static uint16_t word_of(const struct dev *d, uint32_t w)
{
    return (uint16_t)(d->img[2u * w] | (d->img[2u * w + 1u] << 8));
}
static int all_words(const struct dev *d, uint16_t v)
{
    for (uint32_t w = 0; w < WORDS; w++)
        if (word_of(d, w) != v)
            return 0;
    return 1;
}
static void idle_state(const struct dev *d)
{
    CHECK_EQ_U64(d->e.phase, 0u);
    CHECK_EQ_U64(d->e.count, 0u);
}

/* Powers up write-disabled; EWEN enables, EWDS locks again; each write-type
   command lands in the words the data sheet says. */
static void case_commands(void)
{
    struct dev d;
    uint16_t back[2];
    reset(&d, 0x5A);
    /* Power-up: WRITE, ERASE, WRAL and ERAL are all refused. */
    write_word(&d, 5u, 0x1234u);
    erase_word(&d, 6u);
    wral(&d, 0x0000u);
    eral(&d);
    CHECK_EQ_U64(d.changes, 0u);
    CHECK(all_words(&d, 0x5A5Au));
    read_words(&d, 5u, 2u, back); /* READ ignores the write-enable latch */
    CHECK_EQ_HEX(back[0], 0x5A5Au);
    CHECK_EQ_HEX(back[1], 0x5A5Au);

    ewen(&d);
    write_word(&d, 5u, 0x1234u);
    CHECK_EQ_HEX(d.img[10], 0x34);
    CHECK_EQ_HEX(d.img[11], 0x12);
    CHECK_EQ_U64(d.changes, 1u);
    CHECK_EQ_HEX(d.img[8], 0x5A);
    CHECK_EQ_HEX(d.img[12], 0x5A);
    read_words(&d, 5u, 1u, back);
    CHECK_EQ_HEX(back[0], 0x1234u);

    ewds(&d);
    write_word(&d, 5u, 0xBEEFu);
    erase_word(&d, 5u);
    CHECK_EQ_HEX(word_of(&d, 5u), 0x1234u);
    CHECK_EQ_U64(d.changes, 1u);

    ewen(&d);
    erase_word(&d, 5u);
    CHECK_EQ_HEX(word_of(&d, 5u), 0xFFFFu);
    CHECK_EQ_HEX(word_of(&d, 4u), 0x5A5Au);
    CHECK_EQ_U64(d.changes, 2u);

    /* WRAL overwrites every word with the data, not the data AND the old word. */
    memset(d.img, 0x00, sizeof d.img);
    wral(&d, 0x00A5u);
    CHECK(all_words(&d, 0x00A5u));
    CHECK_EQ_U64(d.changes, 3u);
    eral(&d);
    CHECK(all_words(&d, 0xFFFFu));
    for (unsigned i = 0; i < sizeof d.img; i++)
        CHECK_EQ_HEX(d.img[i], 0xFF);
    CHECK_EQ_U64(d.changes, 4u);
    idle_state(&d);
}

/* Chip select low abandons a command; clocks before the start bit are ignored;
   the write-enable latch outlives chip select; reset locks it again. */
static void case_lock(void)
{
    struct dev d;
    uint16_t back[1];
    reset(&d, 0x00);
    ewen(&d);

    /* Abandoned after 10 of the 16 data bits. */
    cs_up(&d);
    send(&d, 1u, 3u);
    bits(&d, 0xFFFFu, 10u);
    CHECK_EQ_U64(d.e.phase, 2u);
    cs_down(&d);
    idle_state(&d);
    CHECK(all_words(&d, 0x0000u));
    CHECK_EQ_U64(d.changes, 0u);
    /* The start bit of the next command is not the data of the abandoned one. */
    cs_up(&d);
    for (unsigned i = 0; i < 40u; i++)
        pulse(&d, 0u); /* before the start bit: ignored */
    CHECK_EQ_U64(d.e.phase, 0u);
    send(&d, 1u, 9u);
    bits(&d, 0xC3A5u, 16u);
    for (unsigned i = 0; i < 20u; i++)
        pulse(&d, i & 1u);       /* after a complete command: ignored */
    CHECK_EQ_U64(d.changes, 0u); /* nothing runs until chip select falls */
    cs_down(&d);
    CHECK_EQ_U64(d.changes, 1u);
    CHECK_EQ_HEX(word_of(&d, 9u), 0xC3A5u);
    CHECK_EQ_HEX(word_of(&d, 8u), 0x0000u);
    CHECK_EQ_HEX(word_of(&d, 10u), 0x0000u);
    idle_state(&d);

    /* A command short of its last data bit when chip select falls writes nothing. */
    cs_up(&d);
    send(&d, 1u, 9u);
    bits(&d, 0x1111u, 15u);
    cs_down(&d);
    CHECK_EQ_HEX(word_of(&d, 9u), 0xC3A5u);

    /* The latch survived the chip select drops above; reset clears it. */
    write_word(&d, 1u, 0x0102u);
    CHECK_EQ_HEX(word_of(&d, 1u), 0x0102u);
    cogmaster__eeprom_reset(&d.e);
    write_word(&d, 1u, 0x0304u);
    CHECK_EQ_HEX(word_of(&d, 1u), 0x0102u);
    read_words(&d, 1u, 1u, back);
    CHECK_EQ_HEX(back[0], 0x0102u);
}

/* READ: dummy 0, then D15 to D0, word after word, 63 wrapping to 0; Data Out is
   high when idle; the window byte is the last write with bit 1 set and Data Out. */
static void case_read(void)
{
    struct dev d;
    uint16_t back[3];
    reset(&d, 0x00);
    for (uint32_t w = 0; w < WORDS; w++) {
        d.img[2u * w] = (uint8_t)(w * 3u + 1u);
        d.img[2u * w + 1u] = (uint8_t)(0xA0u + w);
    }
    CHECK_EQ_U64(out(&d), 1u);
    read_words(&d, 63u, 3u, back);
    CHECK_EQ_HEX(back[0], word_of(&d, 63u));
    CHECK_EQ_HEX(back[1], word_of(&d, 0u));
    CHECK_EQ_HEX(back[2], word_of(&d, 1u));
    CHECK_EQ_U64(out(&d), 1u); /* chip select low releases Data Out */
    idle_state(&d);

    /* Chip select dropped in the middle of a word. */
    cs_up(&d);
    send(&d, 2u, 7u);
    bits(&d, 0u, 5u);
    CHECK(d.e.count <= 15u);
    cs_down(&d);
    CHECK_EQ_U64(out(&d), 1u);
    idle_state(&d);

    /* The window byte. */
    cogmaster__eeprom_reset(&d.e);
    CHECK_EQ_HEX(cogmaster__eeprom_read(&d.e), 0x03);
    put(&d, 0xFD); /* data in high, clock low, chip select high: ignored by the chip */
    CHECK_EQ_HEX(cogmaster__eeprom_read(&d.e), 0xFF);
    put(&d, 0x00);
    CHECK_EQ_HEX(cogmaster__eeprom_read(&d.e), 0x03);
    put(&d, 0xA8);
    CHECK_EQ_HEX(cogmaster__eeprom_read(&d.e), 0xAB);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "commands") == 0)
        case_commands();
    else if (strcmp(argv[1], "lock") == 0)
        case_lock();
    else if (strcmp(argv[1], "read") == 0)
        case_read();
    else
        return 2;
    CHECK_DONE();
}
