/* The 93C46 as a device with no board: framed commands clocked bit by bit through
   the one-byte window the cartridge gives a game (b0 data in, b1 clock, b2 chip
   select), with the expected image worked out by hand from the data sheet; and
   the Sega EEPROM board that wires it to the cartridge window, driven through
   the bus. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "internal.h"
#include "state.h"
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

/* A change is reported once, when chip select falls after a completed command
   that left a byte different; never per clock edge, never for the same data. */
static void case_changes(void)
{
    struct dev d;
    reset(&d, 0xFF);
    ewen(&d);
    cs_up(&d);
    for (unsigned i = 0; i < 3000u; i++) {
        put(&d, (uint8_t)(4u | ((i >> 1) & 1u))); /* data in moves with the clock low */
        pulse(&d, 0u);
    }
    send(&d, 1u, 17u);
    for (unsigned i = 0; i < 16u; i++) {
        pulse(&d, (0x2CA5u >> (15u - i)) & 1u);
        CHECK_EQ_U64(d.changes, 0u); /* no edge of the command reports one */
    }
    for (unsigned i = 0; i < 2000u; i++)
        pulse(&d, i & 1u);
    CHECK_EQ_U64(d.changes, 0u);
    cs_down(&d);
    CHECK_EQ_U64(d.changes, 1u);
    CHECK_EQ_HEX(word_of(&d, 17u), 0x2CA5u);

    write_word(&d, 17u, 0x2CA5u); /* the value already stored */
    CHECK_EQ_U64(d.changes, 1u);
    write_word(&d, 17u, 0x2CA4u); /* one bit different */
    CHECK_EQ_U64(d.changes, 2u);

    wral(&d, 0x1357u); /* 64 words change, one report */
    CHECK_EQ_U64(d.changes, 3u);
    wral(&d, 0x1357u); /* an image already holding that word */
    CHECK_EQ_U64(d.changes, 3u);
    erase_word(&d, 3u);
    CHECK_EQ_U64(d.changes, 4u);
    erase_word(&d, 3u);
    CHECK_EQ_U64(d.changes, 4u);
    eral(&d);
    CHECK_EQ_U64(d.changes, 5u);
    eral(&d); /* an image that is all ones */
    CHECK_EQ_U64(d.changes, 5u);
    CHECK(all_words(&d, 0xFFFFu));
}

/* A model written from the data sheet's instruction table, applied a command at
   a time to its own array of words, against the device clocked bit by bit.
   Frames are always framed by chip select; the generator draws commands,
   commands abandoned at a random bit, idle clocks before the start bit and extra
   clocks after the command. */
struct model {
    uint16_t w[WORDS];
    int enabled;
};
enum { EWEN, EWDS, WRITE, ERASE, WRAL, ERAL, READ };

static uint64_t splitmix(uint64_t *s)
{
    uint64_t z = (*s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
static uint32_t pick(uint64_t *s, uint32_t n)
{
    return (uint32_t)(splitmix(s) % n);
}

/* The data sheet's rule for one completed command; 1 when a word changes. */
static uint32_t model_run(struct model *m, uint32_t kind, uint32_t a, uint16_t v)
{
    uint32_t changed = 0u, i;
    if (kind == EWEN || kind == EWDS)
        m->enabled = kind == EWEN;
    if (!m->enabled)
        return 0u;
    for (i = 0; i < WORDS; i++) {
        uint16_t now = m->w[i];
        if ((kind == WRITE && i == a) || (kind == WRAL))
            m->w[i] = v;
        if ((kind == ERASE && i == a) || kind == ERAL)
            m->w[i] = 0xFFFFu;
        changed |= now != m->w[i];
    }
    return changed;
}

static int frame(struct dev *d, struct model *m, uint64_t *s)
{
    static const uint32_t kinds[] = {EWEN, EWDS, WRITE, WRITE, ERASE, WRAL, ERAL, READ, READ};
    uint32_t kind = kinds[pick(s, 9u)], a = pick(s, WORDS), before = d->changes, expect = 0u;
    uint32_t arg = a, op = 0u, total = 9u, cut = 99u, i;
    uint16_t v = (uint16_t)splitmix(s);
    uint32_t r = pick(s, 8u);
    if (r < 2u)
        v = m->w[a]; /* the word already stored */
    else if (r == 2u)
        v = 0xFFFFu;
    if (kind == EWEN || kind == EWDS || kind == WRAL || kind == ERAL) {
        static const uint32_t ext[] = {3u, 0u, 1u, 2u};
        arg = (ext[kind == EWEN ? 0 : kind == EWDS ? 1 : kind == WRAL ? 2 : 3] << 4) | pick(s, 16u);
    }
    op = kind == WRITE ? 1u : kind == ERASE ? 3u : kind == READ ? 2u : 0u;
    if (kind == WRITE || kind == WRAL)
        total += 16u;
    if (pick(s, 5u) == 0u)
        cut = pick(s, total); /* abandoned before the command is complete */

    cs_up(d);
    for (i = pick(s, 6u); i > 0u; i--) {
        put(d, (uint8_t)(4u | pick(s, 2u))); /* data in moves, clock low */
        pulse(d, 0u);                        /* idle clock, no start bit */
    }
    for (i = 0; i < total && i < cut; i++) {
        if (i == 0u)
            pulse(d, 1u);
        else if (i < 3u)
            pulse(d, (op >> (2u - i)) & 1u);
        else if (i < 9u)
            pulse(d, (arg >> (8u - i)) & 1u);
        else
            pulse(d, (v >> (24u - i)) & 1u);
    }
    if (cut == 99u) {
        if (kind == READ) {
            uint32_t n = pick(s, 40u) + 1u, k;
            CHECK_EQ_U64(out(d), 0u); /* the dummy bit */
            for (k = 0; k < n; k++) {
                pulse(d, 0u);
                if (out(d) != ((m->w[(a + k / 16u) & 63u] >> (15u - k % 16u)) & 1u))
                    return 0;
            }
        } else {
            for (i = pick(s, 20u); i > 0u; i--)
                pulse(d, pick(s, 2u));
            expect = model_run(m, kind, a, v);
        }
    }
    cs_down(d);
    if (d->changes - before != expect)
        return 0;
    for (i = 0; i < WORDS; i++)
        if (word_of(d, i) != m->w[i])
            return 0;
    return 1;
}

static void case_differential(void)
{
    static const uint64_t seeds[] = {1u, 2u, 0xC0FFEEu, 0xDEADBEEFu};
    for (unsigned n = 0; n < sizeof seeds / sizeof seeds[0]; n++) {
        struct dev d;
        struct model m;
        uint64_t s = seeds[n];
        reset(&d, 0x00);
        memset(&m, 0, sizeof m);
        for (uint32_t w = 0; w < WORDS; w++) {
            uint16_t v = (uint16_t)splitmix(&s);
            d.img[2u * w] = (uint8_t)(v & 0xFFu);
            d.img[2u * w + 1u] = (uint8_t)(v >> 8);
            m.w[w] = v;
        }
        for (unsigned f = 0; f < 500u; f++) {
            if (!frame(&d, &m, &s)) {
                fprintf(stderr, "%s:%d: device and model differ, seed %u frame %u\n", __FILE__,
                        __LINE__, n, f);
                CHECK(0);
                return;
            }
        }
    }
}

/* The board: a 16 KiB-paged image of the given size on board 10, with the
   persistence fields a database row would carry, installed in a running instance
   and driven only through the bus. */
#define PAGE 16384u
#define WIN 0x8000u
#define OPEN 0x08u

struct rig {
    cogmaster *m;
    uint8_t *img;
};

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}
static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}
static uint8_t rom_byte(uint32_t i)
{
    return (uint8_t)((i * 2654435761u) >> 24);
}

static void rig_open(struct rig *r, uint32_t kib)
{
    static uint8_t boot[32u * 1024u];
    memset(boot, 0xFF, sizeof boot);
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = boot;
    media.image_size = sizeof boot;
    media.machine = COGMASTER_MACHINE_GG;
    r->m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &r->m), COGMASTER_STATUS_OK);
    if (r->m == NULL || cogmaster_load(r->m, &media) != COGMASTER_STATUS_OK)
        exit(1);
    uint32_t size = kib * 1024u;
    r->img = (uint8_t *)malloc(size);
    if (r->img == NULL)
        exit(1);
    for (uint32_t i = 0; i < size; i++)
        r->img[i] = rom_byte(i);
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.image_size = size;
    info.board = COGMASTER__BOARD_SEGA_EEPROM;
    info.persistence = COGMASTER_PERSISTENCE_BATTERY;
    info.persistence_geometry = COGMASTER_GEOMETRY_SUPPORTED;
    info.persistent_size = 128u;
    struct cogmaster__cart cart;
    CHECK_EQ_U64(cogmaster__cart_copy(r->m, r->img, size, &info, &cart), COGMASTER_STATUS_OK);
    cogmaster__cart_install(r->m, &cart);
    CHECK_EQ_U64(cogmaster_reset(r->m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
}

static void rig_close(struct rig *r)
{
    cogmaster_destroy(r->m);
    free(r->img);
}

static void wr(const struct rig *r, uint16_t addr, uint8_t v)
{
    cogmaster__bus_write(r->m, addr, v);
}
static uint8_t rd(const struct rig *r, uint16_t addr)
{
    return cogmaster__bus_read(r->m, addr);
}
static void bput(const struct rig *r, uint8_t v)
{
    wr(r, WIN, v);
}
static void bpulse(const struct rig *r, unsigned di)
{
    bput(r, (uint8_t)(OPEN | 4u | di));
    bput(r, (uint8_t)(OPEN | 4u | 2u | di));
    bput(r, (uint8_t)(OPEN | 4u | di));
}
static void bbits(const struct rig *r, uint32_t v, unsigned n)
{
    while (n-- > 0u)
        bpulse(r, (v >> n) & 1u);
}
static void bsend(const struct rig *r, uint32_t op, uint32_t arg)
{
    bbits(r, 1u, 1u);
    bbits(r, op, 2u);
    bbits(r, arg, 6u);
}
static void bews(const struct rig *r, uint32_t sub)
{
    bput(r, OPEN | 4u);
    bsend(r, 0u, sub);
    bput(r, OPEN);
}
static void bwrite(const struct rig *r, uint32_t w, uint16_t v)
{
    bput(r, OPEN | 4u);
    bsend(r, 1u, w);
    bbits(r, v, 16u);
    bput(r, OPEN);
}
static uint16_t bread(const struct rig *r, uint32_t w)
{
    uint16_t v = 0;
    bput(r, OPEN | 4u);
    bsend(r, 2u, w);
    CHECK_EQ_U64(rd(r, WIN) & 1u, 0u);
    for (unsigned b = 0; b < 16u; b++) {
        bpulse(r, 0u);
        v = (uint16_t)(((uint32_t)v << 1) | (uint32_t)(rd(r, WIN) & 1u));
    }
    bput(r, OPEN);
    return v;
}
static int fault_present(const struct rig *r)
{
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(r->m, &f), COGMASTER_STATUS_OK);
    return (int)f.present;
}

/* $FFFC bit 3 opens a window at exactly $8000; every other address stays ROM
   and the first 1 KiB stays fixed. */
static void board_window(void)
{
    struct rig r;
    rig_open(&r, 64u);
    CHECK_EQ_HEX(rd(&r, WIN), r.img[2u * PAGE]); /* closed: ROM */
    wr(&r, 0xFFFCu, OPEN);
    CHECK_EQ_HEX(rd(&r, WIN), 0x03); /* nothing written: bit 1 and Data Out */
    bput(&r, 0xA8u);
    CHECK_EQ_HEX(rd(&r, WIN), 0xABu); /* last written AND FC, OR 02, OR Data Out */
    CHECK_EQ_HEX(rd(&r, WIN + 1u), r.img[2u * PAGE + 1u]);
    CHECK_EQ_HEX(rd(&r, WIN - 1u), r.img[PAGE + (PAGE - 1u)]);
    CHECK_EQ_HEX(rd(&r, 0xBFFFu), r.img[3u * PAGE - 1u]);
    wr(&r, 0xFFFCu, 0x00u);
    CHECK_EQ_HEX(rd(&r, WIN), r.img[2u * PAGE]);
    /* The first 1 KiB is page 0 whatever slot 0 shows; the rest of slot 0 pages. */
    wr(&r, 0xFFFDu, 3u);
    CHECK_EQ_HEX(rd(&r, 0x0000u), r.img[0]);
    CHECK_EQ_HEX(rd(&r, 0x03FFu), r.img[0x3FFu]);
    CHECK_EQ_HEX(rd(&r, 0x0400u), r.img[3u * PAGE + 0x400u]);
    CHECK_EQ_U64(fault_present(&r), 0u);
    rig_close(&r);
}

/* The window write drives data in, clock and chip select from bits 0, 1 and 2:
   a WRITE after EWEN stores the word little-endian and moves the generation
   once, a repeat of the same data does not move it. */
static void board_pins(void)
{
    struct rig r;
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bews(&r, 0x30u);
    bwrite(&r, 0u, 0x1234u);
    CHECK_EQ_HEX(r.m->cart.ram[0], 0x34);
    CHECK_EQ_HEX(r.m->cart.ram[1], 0x12);
    CHECK_EQ_HEX(r.m->cart.ram[2], 0xFF);
    CHECK_EQ_U64(r.m->cart.generation, 1u);
    bwrite(&r, 0u, 0x1234u);
    CHECK_EQ_U64(r.m->cart.generation, 1u);
    CHECK_EQ_HEX(bread(&r, 0u), 0x1234u);
    bwrite(&r, 63u, 0xBEEFu);
    CHECK_EQ_U64(r.m->cart.generation, 2u);
    CHECK_EQ_HEX(r.m->cart.ram[126], 0xEF);
    CHECK_EQ_HEX(r.m->cart.ram[127], 0xBE);
    /* Clock edges mid-command move nothing. */
    bput(&r, OPEN | 4u);
    bsend(&r, 1u, 5u);
    bbits(&r, 0x5555u, 16u);
    CHECK_EQ_U64(r.m->cart.generation, 2u);
    bput(&r, OPEN);
    CHECK_EQ_U64(r.m->cart.generation, 3u);
    CHECK_EQ_U64(fault_present(&r), 0u);
    rig_close(&r);
}

/* A $FFFC write with bit 7 set drops chip select, clears the shift state and
   locks writes again. */
static void board_reset_bit(void)
{
    struct rig r;
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bews(&r, 0x30u);
    bwrite(&r, 2u, 0x0102u);
    CHECK_EQ_U64(r.m->cart.generation, 1u);
    bput(&r, OPEN | 4u);
    bsend(&r, 1u, 3u);
    bbits(&r, 0xFFFFu, 5u);
    CHECK(r.m->eeprom.phase != 0u);
    wr(&r, 0xFFFCu, 0x80u | OPEN);
    CHECK_EQ_U64(r.m->eeprom.phase, 0u);
    CHECK_EQ_U64(r.m->eeprom.count, 0u);
    CHECK_EQ_U64(r.m->eeprom.flags & 4u, 0u); /* chip select is low */
    CHECK_EQ_HEX(r.m->mapper.ctrl, 0x88);
    bwrite(&r, 3u, 0x00FFu); /* locked again: no EWEN since the reset */
    CHECK_EQ_HEX(r.m->cart.ram[6], 0xFF);
    CHECK_EQ_HEX(r.m->cart.ram[7], 0xFF);
    CHECK_EQ_U64(r.m->cart.generation, 1u);
    bews(&r, 0x30u);
    bwrite(&r, 3u, 0x00FFu);
    CHECK_EQ_HEX(r.m->cart.ram[6], 0xFF);
    CHECK_EQ_HEX(r.m->cart.ram[7], 0x00);
    CHECK_EQ_U64(r.m->cart.generation, 2u);
    CHECK_EQ_HEX(bread(&r, 2u), 0x0102u); /* the image survived the reset */
    rig_close(&r);
}

/* $FFFD-$FFFF page 16 KiB banks modulo the page count, mapper_range only past
   an image that is not a power of two; documented writes and writes to ROM
   record no fault. */
static void board_banks(void)
{
    struct rig r;
    rig_open(&r, 64u);
    wr(&r, 0xFFFDu, 1u);
    wr(&r, 0xFFFEu, 6u); /* 6 mod 4 on a power-of-two image: wraps, no fault */
    wr(&r, 0xFFFFu, 3u);
    CHECK_EQ_HEX(rd(&r, 0x0400u), r.img[PAGE + 0x400u]);
    CHECK_EQ_HEX(rd(&r, 0x4000u), r.img[2u * PAGE]);
    CHECK_EQ_HEX(rd(&r, 0x8001u), r.img[3u * PAGE + 1u]);
    wr(&r, 0xFFFCu, OPEN);
    bput(&r, 0x00u);
    CHECK_EQ_U64(fault_present(&r), 0u);
    rig_close(&r);

    rig_open(&r, 48u);
    wr(&r, 0xFFFFu, 5u);
    CHECK_EQ_HEX(rd(&r, 0x8001u), r.img[2u * PAGE + 1u]);
    CHECK_EQ_U64(fault_present(&r), 1u);
    rig_close(&r);

    rig_open(&r, 64u);
    wr(&r, 0x1000u, 0u);
    CHECK_EQ_U64(fault_present(&r), 0u);
    rig_close(&r);

    rig_open(&r, 64u);
    bput(&r, 0x00u); /* the window is closed: a write to ROM */
    CHECK_EQ_U64(fault_present(&r), 0u);
    rig_close(&r);
}

/* The image is cartridge RAM, all ones at start, exported through the battery
   API at its 128 bytes; an import of a known word is what the guest reads. */
static void board_span(void)
{
    struct rig r;
    cogmaster_battery b;
    uint8_t save[128];
    rig_open(&r, 64u);
    memset(&b, 0, sizeof b);
    b.size = (uint32_t)sizeof b;
    CHECK_EQ_U64(cogmaster_get_battery(r.m, &b), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(b.length, 128u);
    CHECK_EQ_U64(b.knowledge, COGMASTER_BATTERY_KNOWN);
    CHECK(b.data == r.m->cart.ram);
    CHECK_EQ_U64(b.generation, 0u);
    for (unsigned i = 0; i < 128u; i++)
        CHECK_EQ_HEX(((const uint8_t *)b.data)[i], 0xFF);

    memset(save, 0xFF, sizeof save);
    save[0] = 0x34;
    save[1] = 0x12;
    CHECK_EQ_U64(cogmaster_import_battery(r.m, save, sizeof save), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(r.m->cart.generation, 0u);
    wr(&r, 0xFFFCu, OPEN);
    CHECK_EQ_HEX(bread(&r, 0u), 0x1234u);
    CHECK_EQ_HEX(bread(&r, 1u), 0xFFFFu);
    rig_close(&r);
}

/* A state saved at any point of a command resumes it exactly. */
static uint8_t *save_of(const struct rig *r, size_t *n)
{
    *n = cogmaster__state_size(r->m);
    uint8_t *buf = (uint8_t *)malloc(*n);
    if (*n == 0u || buf == NULL)
        exit(1);
    CHECK_EQ_U64(cogmaster__state_save(r->m, buf, *n), COGMASTER_STATUS_OK);
    return buf;
}

/* The same image in every rig, with a known word pattern in the chip. */
static void rig_pattern(struct rig *r, uint32_t kib)
{
    uint8_t save[128];
    rig_open(r, kib);
    for (unsigned i = 0; i < sizeof save; i++)
        save[i] = (uint8_t)(i * 7u + 3u);
    CHECK_EQ_U64(cogmaster_import_battery(r->m, save, sizeof save), COGMASTER_STATUS_OK);
    wr(r, 0xFFFCu, OPEN);
}

/* Clock number i of a WRITE of word 5 (25 clocks: start, opcode 01, six
   address bits, sixteen data bits of 0xA55A) or of a READ of word 5 (nine
   command clocks, then clocks that only shift data out). */
static unsigned seq_bit(int is_read, unsigned i)
{
    const uint32_t cmd = (1u << 8) | ((is_read ? 2u : 1u) << 6) | 5u;
    if (i < 9u)
        return (cmd >> (8u - i)) & 1u;
    return is_read ? 0u : (0xA55Au >> (24u - i)) & 1u;
}

/* Clocks from..to-1; the Data Out level after each goes to out[]. */
static void drive(const struct rig *r, int is_read, unsigned from, unsigned to, uint8_t *out)
{
    for (unsigned i = from; i < to; i++) {
        bpulse(r, seq_bit(is_read, i));
        out[i] = (uint8_t)(rd(r, WIN) & 1u);
    }
}

static void state_mid_command(void)
{
    for (int is_read = 0; is_read < 2; is_read++) {
        unsigned total = is_read ? 26u : 25u;
        struct rig ref;
        uint8_t ref_out[32], a_out[32], b_out[32];
        memset(ref_out, 0, sizeof ref_out);
        rig_pattern(&ref, 64u);
        bews(&ref, 0x30u);
        bput(&ref, OPEN | 4u);
        drive(&ref, is_read, 0u, total, ref_out);
        bput(&ref, OPEN);
        for (unsigned k = 0; k <= total; k++) {
            struct rig a, b;
            size_t n, nb;
            rig_pattern(&a, 64u);
            rig_pattern(&b, 64u);
            bews(&a, 0x30u);
            bput(&a, OPEN | 4u);
            memset(a_out, 0, sizeof a_out);
            memset(b_out, 0, sizeof b_out);
            drive(&a, is_read, 0u, k, a_out);
            uint8_t *buf = save_of(&a, &n);
            CHECK_EQ_U64(cogmaster__state_load(b.m, buf, n), COGMASTER_STATUS_OK);
            drive(&a, is_read, k, total, a_out);
            drive(&b, is_read, k, total, b_out);
            bput(&a, OPEN);
            bput(&b, OPEN);
            uint8_t *fa = save_of(&a, &n);
            uint8_t *fb = save_of(&b, &nb);
            CHECK_EQ_U64(n, nb);
            CHECK(memcmp(fa, fb, n) == 0);
            CHECK(memcmp(&a.m->eeprom, &b.m->eeprom, sizeof a.m->eeprom) == 0);
            CHECK(memcmp(a_out + k, b_out + k, total - k) == 0);
            CHECK(memcmp(b_out + k, ref_out + k, total - k) == 0);
            CHECK_EQ_U64(a.m->cart.generation, b.m->cart.generation);
            CHECK_EQ_U64(b.m->cart.generation, ref.m->cart.generation);
            CHECK(memcmp(b.m->cart.ram, ref.m->cart.ram, 128u) == 0);
            free(fa);
            free(fb);
            free(buf);
            rig_close(&a);
            rig_close(&b);
        }
        CHECK_EQ_U64(ref.m->cart.generation, is_read ? 0u : 1u);
        CHECK_EQ_HEX(ref.m->cart.ram[10], is_read ? (uint8_t)(10u * 7u + 3u) : 0x5A);
        rig_close(&ref);
    }
}

/* A state saved with the window open loads into a fresh instance that reads the
   window and the banks as the first did. */
static void state_window_open(void)
{
    struct rig a, b;
    size_t n;
    rig_open(&a, 64u);
    rig_open(&b, 64u);
    wr(&a, 0xFFFCu, 0x88u);
    wr(&a, 0xFFFDu, 1u);
    wr(&a, 0xFFFEu, 3u);
    wr(&a, 0xFFFFu, 2u);
    bput(&a, 0xA8u);
    uint8_t *buf = save_of(&a, &n);
    CHECK_EQ_U64(cogmaster__state_load(b.m, buf, n), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(b.m->mapper.ctrl, 0x88);
    CHECK_EQ_HEX(rd(&b, WIN), 0xABu);
    CHECK_EQ_HEX(rd(&b, 0x0400u), a.img[PAGE + 0x400u]);
    CHECK_EQ_HEX(rd(&b, 0x4001u), a.img[3u * PAGE + 1u]);
    CHECK_EQ_HEX(rd(&b, 0x8001u), a.img[2u * PAGE + 1u]);
    free(buf);
    rig_close(&a);
    rig_close(&b);
}

/* The saved bytes of the named field, replaced by v (width 1), sealed again. */
static int field_offset(const struct rig *r, const char *name, uint32_t *off)
{
    size_t fields = cogmaster__state_record(r->m, NULL, 0u);
    struct cogmaster__state_field *rec =
        (struct cogmaster__state_field *)malloc(fields * sizeof *rec);
    if (rec == NULL)
        exit(1);
    CHECK_EQ_U64(cogmaster__state_record(r->m, rec, fields), fields);
    int found = 0;
    for (size_t i = 0; i < fields && !found; i++) {
        if (strcmp(rec[i].name, name) == 0) {
            *off = rec[i].offset;
            found = 1;
        }
    }
    free(rec);
    return found;
}

/* One hostile byte: the state is refused and the instance is unchanged. */
static void refuse(struct rig *r, const char *name, uint8_t v)
{
    size_t n, n2;
    uint32_t off = 0;
    uint8_t *good = save_of(r, &n);
    uint8_t *bad = (uint8_t *)malloc(n);
    if (bad == NULL || !field_offset(r, name, &off))
        exit(1);
    memcpy(bad, good, n);
    bad[off] = v;
    cogmaster__state_seal(bad, n);
    CHECK_EQ_U64(cogmaster__state_load(r->m, bad, n), COGMASTER_STATUS_ERR_ARGUMENT);
    uint8_t *after = save_of(r, &n2);
    CHECK_EQ_U64(n, n2);
    CHECK(memcmp(good, after, n) == 0);
    free(good);
    free(bad);
    free(after);
}

static void state_bounds(void)
{
    struct rig r;
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bews(&r, 0x30u);
    /* Chip select low and idle: a phase, a count or reserved flag bits that
       chip select low cannot leave are refused. */
    refuse(&r, "eeprom_phase", 1u);
    refuse(&r, "eeprom_count", 3u);
    refuse(&r, "eeprom_flags", 0x20u);
    refuse(&r, "eeprom_phase", 5u);
    /* The control byte keeps bits 3 and 7 only. */
    refuse(&r, "mapper_ctrl", 0x04u);
    refuse(&r, "mapper_ctrl", 0x10u);
    refuse(&r, "mapper_ctrl", 0x89u);
    /* Chip select high: a count past the width of the phase, or past any phase, is refused. */
    bput(&r, OPEN | 4u);
    bsend(&r, 1u, 5u);
    bbits(&r, 0u, 3u); /* data in, 3 of 16 */
    refuse(&r, "eeprom_count", 16u);
    refuse(&r, "eeprom_phase", 7u);
    refuse(&r, "eeprom_flags", 0x20u);
    /* The fields hold together: Data Out low and a waiting program only where
       the chip can have them, and a command that is not the phase's. */
    refuse(&r, "eeprom_flags", 0x07u);
    refuse(&r, "eeprom_flags", 0x16u);
    refuse(&r, "eeprom_cmd", 0x85u);
    rig_close(&r);
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bput(&r, OPEN | 4u);
    bsend(&r, 2u, 5u); /* a read of word 5 */
    refuse(&r, "eeprom_cmd", 0x45u);
    rig_close(&r);
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bput(&r, OPEN | 4u);
    bsend(&r, 0u, 0u); /* EWDS: complete, writes disabled */
    refuse(&r, "eeprom_flags", 0x14u);
    rig_close(&r);
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bput(&r, OPEN | 4u);
    bsend(&r, 0u, 0x30u); /* EWEN: complete, writes enabled, nothing to program */
    refuse(&r, "eeprom_flags", 0x16u);
    rig_close(&r);
    rig_open(&r, 64u);
    wr(&r, 0xFFFCu, OPEN);
    bput(&r, OPEN | 4u);
    bbits(&r, 1u, 1u);
    bbits(&r, 0u, 4u); /* command phase, 4 of 8 */
    refuse(&r, "eeprom_count", 8u);
    rig_close(&r);
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
    else if (strcmp(argv[1], "changes") == 0)
        case_changes();
    else if (strcmp(argv[1], "differential") == 0)
        case_differential();
    else if (strcmp(argv[1], "board.window") == 0)
        board_window();
    else if (strcmp(argv[1], "board.pins") == 0)
        board_pins();
    else if (strcmp(argv[1], "board.reset_bit") == 0)
        board_reset_bit();
    else if (strcmp(argv[1], "board.banks") == 0)
        board_banks();
    else if (strcmp(argv[1], "board.span") == 0)
        board_span();
    else if (strcmp(argv[1], "state.window_open") == 0)
        state_window_open();
    else if (strcmp(argv[1], "state.mid_command") == 0)
        state_mid_command();
    else if (strcmp(argv[1], "state.bounds") == 0)
        state_bounds();
    else
        return 2;
    CHECK_DONE();
}
