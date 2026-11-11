/* Writes the synthetic cartridge images the runner tests read and the fuzz
   targets start from. Nothing here is a game: each image exists to reach
   one branch of the loader, the probe or the first-fault record.

     cogmaster-test-seeds OUTDIR           every image below, into OUTDIR
     cogmaster-test-seeds --oversize FILE  one file of 64 MiB + 1 byte

   The oversize file goes elsewhere: the fuzz replay reads every file in the
   seeds directory, and the runner must refuse this one before reading it.

   Sizes (a DI; HALT program at $0000):
     size-1023.sms  size-1024.sms  size-16k.sms
     size-16k-512.sms (a 512-byte copier header, then size-16k.sms)
     size-48k.sms  size-1m.sms  size-1m-512.sms  size-1m-513.sms
   ROM headers at $7FF0, one per region code, 32 KiB, good checksum:
     header-japan-32k.sms  header-export-32k.sms  header-gg-japan-32k.sms
     header-gg-export-32k.sms  header-gg-intl-32k.sms
   ROM headers per size code, and bad checksums:
     header-export-64k.sms  header-export-128k.sms  header-export-256k.sms
     header-export-32k-bad.sms  header-export-128k-bad.sms
   ROM headers at the other places, and the Game Gear cheat device:
     header-3ff0-16k.sms  header-1ff0-8k.sms  gg-8k.gg
   Boards:
     codemasters-64k.sms (a header at $7FE0)
     unsupported-board.sms (64 KiB storing to another board's $A000)
     janggun-board.sms (64 KiB storing to a board's $6000, which the core names and refuses)
   Programs at $0000, 32 KiB with no header unless noted:
     di-halt.sms  di-halt.gg  jr-self.sms  in-a-00.sms  ld-8000.sms
     ed-00.sms  be-hammer.sms  fffc-writes.sms (64 KiB, Sega board)
   An EEPROM clock stream for fuzz_eeprom.c: eeprom-clocks.bin */
/* Boot ROM images for fuzz_bios.c, each 8192 bytes unless noted:
     bios-owned.bin (DI, jump to itself, then a pseudo-random body)
     bios-ff.bin  bios-zero.bin  bios-never-disables.bin (loops, no write to $3E)
     bios-both.bin (writes $3E enabling the BIOS and the cartridge, then loops)
     bios-oversize.bin (262,145 bytes of a pseudo-random pattern) */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define KIB 1024u
#define MIB (1024u * KIB)

/* The largest image: 1 MiB, a copier header and one byte more. */
static uint8_t image[MIB + 513u];

static const char *outdir;
static int failures;

static void write_image(const char *name, size_t n)
{
    char path[4096];
    int len = snprintf(path, sizeof path, "%s/%s", outdir, name);
    FILE *f = len > 0 && (size_t)len < sizeof path ? fopen(path, "wb") : NULL;
    int ok = f != NULL && fwrite(image, 1u, n, f) == n;
    if (f != NULL && fclose(f) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "cogmaster-test-seeds: cannot write %s\n", path);
        failures++;
    }
}

/* Clears n bytes and puts a program at $0000 of the body at off. */
static void program(size_t off, size_t n, const uint8_t *code, size_t code_n)
{
    memset(image, 0, n);
    memcpy(image + off, code, code_n);
}

static const uint8_t di_halt[] = {0xF3u, 0x76u};
/* Mode 4 display enabled, zero palette/pixels, then permanent HALT. */
static const uint8_t static_display[] = {0xF3u, 0x3Eu, 0x04u, 0xD3u, 0xBFu, 0x3Eu,
                                         0x80u, 0xD3u, 0xBFu, 0x3Eu, 0x40u, 0xD3u,
                                         0xBFu, 0x3Eu, 0x81u, 0xD3u, 0xBFu, 0x76u};

/* The bytes the checksum covers, by size code (SMS-CARTRIDGES section 1.2). */
static uint32_t checksum_range(uint32_t code)
{
    static const uint32_t range[16] = {
        0x40000u, 0x80000u, 0x100000u, 0u,      0u,      0u,      0u,       0u,
        0u,       0u,       0x1FF0u,   0x3FF0u, 0x7FF0u, 0xCFF0u, 0x10000u, 0x20000u,
    };
    return range[code & 15u];
}

/* Writes a 16-byte ROM header at at in a body of n bytes: the magic as hex
   bytes, product code 1234 version 1, the region and size codes and the
   checksum the export BIOS computes, plus one when bad (SMS-CARTRIDGES
   section 1.1). */
static void header(size_t n, uint32_t at, uint32_t region, uint32_t code, int bad)
{
    static const uint8_t magic[8] = {0x54u, 0x4Du, 0x52u, 0x20u, 0x53u, 0x45u, 0x47u, 0x41u};
    uint8_t *h = image + at;
    memcpy(h, magic, sizeof magic);
    h[8] = 0u;
    h[9] = 0u;
    h[12] = 0x34u;
    h[13] = 0x12u;
    h[14] = 0x01u;
    h[15] = (uint8_t)(region << 4 | code);
    size_t end = checksum_range(code) < n ? checksum_range(code) : n;
    uint32_t sum = 0u;
    for (size_t i = 0; i < end; i++) {
        if (i < at || i >= at + 16u) {
            sum += image[i];
        }
    }
    sum = (sum + (bad ? 1u : 0u)) & 0xFFFFu;
    h[10] = (uint8_t)sum;
    h[11] = (uint8_t)(sum >> 8);
}

static void header_image(const char *name, size_t n, uint32_t at, uint32_t region, uint32_t code,
                         int bad)
{
    program(0u, n, di_halt, sizeof di_halt);
    /* Some bytes that are not zero, so the checksum is not trivially 0. */
    for (size_t i = 2u; i < 256u; i++) {
        image[i] = (uint8_t)i;
    }
    header(n, at, region, code, bad);
    write_image(name, n);
}

static void sizes(void)
{
    static const struct {
        const char *name;
        size_t n;
    } list[] = {
        {"size-1023.sms", 1023u},    {"size-1024.sms", 1024u}, {"size-16k.sms", 16u * KIB},
        {"size-48k.sms", 48u * KIB}, {"size-1m.sms", MIB},     {"size-1m-513.sms", MIB + 513u},
    };
    for (size_t i = 0; i < sizeof list / sizeof list[0]; i++) {
        program(0u, list[i].n, di_halt, sizeof di_halt);
        write_image(list[i].name, list[i].n);
    }
    /* A copier header: 512 bytes of its own before the body. */
    program(512u, 16u * KIB + 512u, di_halt, sizeof di_halt);
    memset(image, 0xAA, 512u);
    write_image("size-16k-512.sms", 16u * KIB + 512u);
    program(512u, MIB + 512u, di_halt, sizeof di_halt);
    memset(image, 0xAA, 512u);
    write_image("size-1m-512.sms", MIB + 512u);
}

static void headers(void)
{
    /* Region codes 3 and 4 are the Master System (Japan, export), 5 to 7
       the Game Gear (Japan, export, international); size code $C is 32 KiB,
       $E 64 KiB, $F 128 KiB, $0 256 KiB, $B 16 KiB and $A 8 KiB. */
    header_image("header-japan-32k.sms", 32u * KIB, 0x7FF0u, 3u, 0xCu, 0);
    header_image("header-export-32k.sms", 32u * KIB, 0x7FF0u, 4u, 0xCu, 0);
    header_image("header-gg-japan-32k.sms", 32u * KIB, 0x7FF0u, 5u, 0xCu, 0);
    header_image("header-gg-export-32k.sms", 32u * KIB, 0x7FF0u, 6u, 0xCu, 0);
    header_image("header-gg-intl-32k.sms", 32u * KIB, 0x7FF0u, 7u, 0xCu, 0);
    header_image("header-export-64k.sms", 64u * KIB, 0x7FF0u, 4u, 0xEu, 0);
    header_image("header-export-128k.sms", 128u * KIB, 0x7FF0u, 4u, 0xFu, 0);
    header_image("header-export-256k.sms", 256u * KIB, 0x7FF0u, 4u, 0x0u, 0);
    header_image("header-export-32k-bad.sms", 32u * KIB, 0x7FF0u, 4u, 0xCu, 1);
    header_image("header-export-128k-bad.sms", 128u * KIB, 0x7FF0u, 4u, 0xFu, 1);
    header_image("header-3ff0-16k.sms", 16u * KIB, 0x3FF0u, 4u, 0xBu, 0);
    header_image("header-1ff0-8k.sms", 8u * KIB, 0x1FF0u, 4u, 0xAu, 0);
    /* An 8 KiB Game Gear image is the X-Terminator cheat device
       (SMS-CARTRIDGES section 1.4). */
    header_image("gg-8k.gg", 8u * KIB, 0x1FF0u, 6u, 0xAu, 0);
}

static void boards(void)
{
    /* The Codemasters header at $7FE0: 4 pages, a BCD date in 1994, a
       checksum word and its complement, and $7FEF zero (SMS-CARTRIDGES
       section 1.2). */
    program(0u, 64u * KIB, di_halt, sizeof di_halt);
    static const uint8_t cm[16] = {0x04u, 0x15u, 0x03u, 0x94u, 0x12u, 0x30u, 0x34u, 0x12u,
                                   0xCCu, 0xEDu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
    memcpy(image + 0x7FE0u, cm, sizeof cm);
    write_image("codemasters-64k.sms", 64u * KIB);
    /* Two LD ($A000),A stores and none to the Sega registers: another
       board's mapper (section 1.4). */
    static const uint8_t other[] = {0x3Eu, 0x01u, 0x32u, 0x00u, 0xA0u, 0x3Cu,
                                    0x32u, 0x00u, 0xA0u, 0xF3u, 0x76u};
    program(0u, 64u * KIB, other, sizeof other);
    write_image("unsupported-board.sms", 64u * KIB);
    /* Four LD ($6000),A stores and none to the Sega registers: the store sites of Janggun's
       $6000 register make the scan name Janggun, an unsupported board (SMS-CARTRIDGES section
       1.4; MAME src/devices/bus/sms_cart/rom.cpp, sega8_janggun_device). */
    static const uint8_t janggun[] = {0x3Eu, 0x01u, 0x32u, 0x00u, 0x60u, 0x3Eu, 0x02u, 0x32u,
                                      0x00u, 0x60u, 0x3Eu, 0x03u, 0x32u, 0x00u, 0x60u, 0x3Eu,
                                      0x04u, 0x32u, 0x00u, 0x60u, 0xF3u, 0x76u};
    program(0u, 64u * KIB, janggun, sizeof janggun);
    write_image("janggun-board.sms", 64u * KIB);
}

static void programs(void)
{
    static const uint8_t jr_self[] = {0x18u, 0xFEu};
    /* LD A,$00 first, so IN A,($00) puts $0000 on the address bus whatever
       A held at boot. */
    static const uint8_t in_a_00[] = {0x3Eu, 0x00u, 0xDBu, 0x00u, 0xF3u, 0x76u};
    static const uint8_t ld_8000[] = {0x3Eu, 0x00u, 0x32u, 0x00u, 0x80u, 0xF3u, 0x76u};
    static const uint8_t ed_00[] = {0xEDu, 0x00u, 0xF3u, 0x76u};
    /* LD A,0; loop: OUT ($BE),A; INC A; JR loop. */
    static const uint8_t be_hammer[] = {0x3Eu, 0x00u, 0xD3u, 0xBEu, 0x3Cu, 0x18u, 0xFBu};
    /* Enable display; alternate VRAM/CRAM/register control bytes while
       writing data continuously. No interrupt or HALT can end the loop. */
    static const uint8_t render_hammer[] = {
        0xF3u, 0x3Eu, 4u,    0xD3u, 0xBFu, 0x3Eu, 0x80u, 0xD3u, 0xBFu, 0x3Eu, 0x40u, 0xD3u, 0xBFu,
        0x3Eu, 0x81u, 0xD3u, 0xBFu, 0xD3u, 0xBFu, 0xD3u, 0xBEu, 0x3Cu, 0xC3u, 17u,   0u};
    /* Each Sega register in turn: $FFFC = 0, $FFFD = 1, $FFFE = 2, $FFFF = 3. */
    static const uint8_t fffc[] = {0x3Eu, 0x00u, 0x32u, 0xFCu, 0xFFu, 0x3Cu, 0x32u,
                                   0xFDu, 0xFFu, 0x3Cu, 0x32u, 0xFEu, 0xFFu, 0x3Cu,
                                   0x32u, 0xFFu, 0xFFu, 0xF3u, 0x76u};
    static const struct {
        const char *name;
        const uint8_t *code;
        size_t code_n, n;
    } list[] = {
        {"di-halt.sms", di_halt, sizeof di_halt, 32u * KIB},
        {"static-display.sms", static_display, sizeof static_display, 32u * KIB},
        {"di-halt.gg", di_halt, sizeof di_halt, 32u * KIB},
        {"jr-self.sms", jr_self, sizeof jr_self, 32u * KIB},
        {"in-a-00.sms", in_a_00, sizeof in_a_00, 32u * KIB},
        {"ld-8000.sms", ld_8000, sizeof ld_8000, 32u * KIB},
        {"ed-00.sms", ed_00, sizeof ed_00, 32u * KIB},
        {"be-hammer.sms", be_hammer, sizeof be_hammer, 32u * KIB},
        {"render-hammer.sms", render_hammer, sizeof render_hammer, 32u * KIB},
        {"fffc-writes.sms", fffc, sizeof fffc, 64u * KIB},
    };
    for (size_t i = 0; i < sizeof list / sizeof list[0]; i++) {
        program(0u, list[i].n, list[i].code, list[i].code_n);
        write_image(list[i].name, list[i].n);
    }
}

/* The stream fuzz_eeprom.c reads as (register, value) pairs: an even first byte
   writes the chip window. EWEN, WRITE word 5, READ word 5 and a few clocks past
   it, ERAL abandoned by chip select, then the reset bit, so a replay reaches
   every phase of the chip. Command bits are start, opcode and six more. */
static size_t stream_n;

static void stream_put(uint8_t v)
{
    image[stream_n++] = 0u;
    image[stream_n++] = v;
}

static void stream_bits(uint32_t v, unsigned n, unsigned clocks_after)
{
    while (n-- > 0u) {
        uint8_t di = (uint8_t)((v >> n) & 1u);
        stream_put((uint8_t)(4u | di));
        stream_put((uint8_t)(4u | 2u | di));
        stream_put((uint8_t)(4u | di));
    }
    while (clocks_after-- > 0u) {
        stream_put(6u);
        stream_put(4u);
    }
}

static void eeprom_stream(void)
{
    stream_n = 0u;
    stream_put(4u); /* EWEN: 1 00 11xxxx */
    stream_bits(0x130u, 9u, 0u);
    stream_put(0u);
    stream_put(4u); /* WRITE word 5: 1 01 000101, then 16 data bits */
    stream_bits(0x145u, 9u, 0u);
    stream_bits(0xA55Au, 16u, 0u);
    stream_put(0u);
    stream_put(4u); /* READ word 5, then 20 more clocks */
    stream_bits(0x185u, 9u, 20u);
    stream_put(0u);
    stream_put(4u); /* ERAL, abandoned after its start bit and two more */
    stream_bits(0x4u, 3u, 0u);
    stream_put(0u);
    stream_put(4u); /* a start bit, then the reset bit mid-command */
    stream_bits(0x1u, 1u, 0u);
    image[stream_n++] = 1u; /* an odd first byte writes $FFFC */
    image[stream_n++] = 0x88u;
    write_image("eeprom-clocks.bin", stream_n);
}

/* The stream fuzz_pad.c reads: a profile byte, ten bytes that name saved fields to change, then
   operations. The operation numbers and the selector tables are that file's: devices 0 pad, 1 none,
   2 Paddle, 3 Sports Pad, 4 Light Phaser, 6 one past the last; an axis or tick count with its top
   bit or selector 5 set is a raw word that follows; tick selector 2 is one short of the Sports
   Pad's idle limit and 3 is the limit itself. The program walks a Sports Pad read on port 1 and a
   Paddle on port 2, abandons a read just inside the limit and restarts one at it, writes the
   glasses mirror, resets both ways and sends a hostile axis and an unknown device. The gun streams
   below paint the backdrop, aim a phaser at it from one port or both, wait a picture out and poll.
 */
enum {
    P_SET,
    P_TH,
    P_TH2,
    P_CTRL,
    P_READ,
    P_EYE,
    P_TICKS,
    P_RESET,
    P_FRAME,
    P_TICKS2,
    P_PAINT,
    P_MODE
};

static void pad_put(uint8_t v)
{
    image[stream_n++] = v;
}

static void pad_word(uint32_t v)
{
    for (uint32_t i = 0u; i < 4u; i++) {
        pad_put((uint8_t)(v >> (8u * i)));
    }
}

static void pad_axis(int32_t v)
{
    pad_put(0x80u);
    pad_word((uint32_t)v);
}

static void pad_set(uint8_t d1, uint8_t d2, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    pad_put(P_SET);
    pad_put(d1);
    pad_put(d2);
    pad_put(0u); /* p1 */
    pad_put(0u); /* p2 */
    pad_put(0u); /* pause, reset, start */
    pad_axis(x1);
    pad_axis(y1);
    pad_axis(x2);
    pad_axis(y2);
}

static void pad_th(uint8_t v)
{
    pad_put(P_TH);
    pad_put(v);
}

static void pad_read(uint8_t which)
{
    pad_put(P_READ);
    pad_put(which);
}

static void pad_wait(uint8_t sel)
{
    pad_put(P_TICKS);
    pad_put(sel);
}

static void pad_wait_raw(uint32_t ticks)
{
    pad_put(P_TICKS);
    pad_put(5u);
    pad_word(ticks);
}

static void pad_eye(uint8_t where, uint8_t v)
{
    pad_put(P_EYE);
    pad_put(where);
    pad_put(v);
}

/* A fall then a rise of TH on port 1, each read, a pad's nibbles as a program takes them. */
static void pad_nibbles(uint32_t first, uint32_t next)
{
    pad_wait_raw(first);
    pad_th(0x05u);
    pad_read(0u);
    pad_wait_raw(first);
    pad_th(0x25u);
    pad_read(0u);
    pad_wait_raw(next);
    pad_th(0x05u);
    pad_read(0u);
    pad_wait_raw(next);
    pad_th(0x25u);
    pad_read(0u);
}

static void pad_stream(uint8_t profile, const char *name)
{
    stream_n = 0u;
    pad_put(profile);
    for (uint32_t i = 0u; i < 10u; i++) {
        pad_put(0u); /* the saved fields to change at the end: the first, complemented */
    }
    pad_set(3u, 2u, 53, -45, 70, 0);
    pad_put(P_FRAME);
    pad_nibbles(858u, 429u);
    pad_wait_raw(300u); /* the Paddle's clock on port 2 */
    pad_read(1u);
    pad_wait_raw(300u);
    pad_read(1u);
    pad_set(3u, 2u, 127, 127, -20, 0);
    pad_put(P_FRAME);
    pad_th(0x05u); /* a read abandoned one tick inside the idle limit keeps its place */
    pad_wait_raw(858u);
    pad_th(0x25u);
    pad_wait(2u);
    pad_th(0x05u);
    pad_read(0u);
    pad_wait(1u);
    pad_th(0x25u);
    pad_th(0x05u); /* one abandoned at the limit starts over */
    pad_wait_raw(858u);
    pad_th(0x25u);
    pad_wait(3u);
    pad_th(0x05u);
    pad_read(0u);
    pad_wait(4u);
    pad_eye(0u, 0u);
    pad_eye(3u, 1u);
    pad_put(P_CTRL);
    pad_put(0xFFu);
    pad_eye(1u, 0u);
    pad_set(3u, 3u, 5, -5, 6, -6);
    pad_put(P_FRAME);
    pad_put(P_RESET);
    pad_put(0u);
    pad_nibbles(858u, 429u);
    pad_put(P_RESET);
    pad_put(1u);
    pad_set(0u, 6u, 0, 0, 0, 0); /* an unknown device is refused */
    pad_set(3u, 0u, 0, 0, 0, 0);
    pad_put(P_SET); /* an axis past the range is refused */
    pad_put(3u);
    pad_put(0u);
    pad_put(0u);
    pad_put(0u);
    pad_put(0u);
    pad_axis(INT32_MIN);
    pad_put(0u);
    pad_put(0u);
    pad_put(0u);
    pad_put(P_FRAME);
    write_image(name, stream_n);
}

static void pad_header(uint8_t profile)
{
    stream_n = 0u;
    pad_put(profile);
    for (uint32_t i = 0u; i < 10u; i++) {
        pad_put(0u);
    }
}

/* n waits of 2000 ticks, the most one operation moves time on, about three lines each: 55 of them
   from the end of a frame are the rest of the blanking and the rows down to a disc at row 100. */
static void pad_wait_rows(uint32_t n)
{
    for (uint32_t i = 0u; i < n; i++) {
        pad_wait_raw(2000u);
    }
}

/* A gun on each port aimed at one band of the painted picture, the first port's far to the
   right of the second's, so that its edges are the later ones in every row, in force after a frame,
   then a poll of $DD and of the H counter a few rows into the scan, a toggle of TH and the rest
   of the picture. */
static void pad_gun_band(void)
{
    pad_header(0u);
    pad_set(4u, 4u, 200, 100, 40, 100);
    pad_put(P_PAINT);
    pad_th(0xFFu);
    pad_put(P_FRAME);
    pad_wait_rows(55u);
    pad_read(1u);
    pad_wait_raw(14u);
    pad_read(1u);
    pad_th(0xF5u);
    pad_th(0xFFu);
    pad_wait_rows(8u); /* the disc goes by with nothing reading, so only the twin looks */
    pad_put(P_FRAME);
    pad_set(4u, 0u, 200, 100, 0, 0); /* the second port goes back to a pad: its block clears */
    pad_wait_rows(55u);
    pad_read(1u);
    pad_wait_rows(8u);
    pad_put(P_FRAME);
    write_image("pad-gun-band.bin", stream_n);
}

/* A gun parked, then aimed at each edge of the field and just past it: a pair that is no field
   dot is stored as no aim, and the picture is painted so each one that is a dot is scanned. */
static void pad_gun_edges(void)
{
    static const int32_t aims[][2] = {{256, 240}, {1, 1},   {257, 1},   {1, 241},
                                      {0, 5},     {256, 0}, {255, 239}, {-1, 5}};
    pad_header(1u);
    pad_set(4u, 1u, 0, 0, 0, 0);
    pad_put(P_FRAME);
    pad_read(1u);
    pad_put(P_PAINT);
    pad_th(0xFFu);
    for (size_t i = 0u; i < sizeof aims / sizeof aims[0]; i++) {
        pad_set(4u, 4u, aims[i][0], aims[i][1], aims[i][1], aims[i][0]);
        pad_wait_rows(2u);
        pad_read(1u);
        pad_put(P_FRAME);
    }
    write_image("pad-gun-edges.bin", stream_n);
}

/* A gun at a painted band, a switch to a mode the core does not draw with the display on in the
   middle of its picture, polls inside it and after it, and the paint again. */
static void pad_gun_mode(void)
{
    pad_header(2u);
    pad_set(4u, 3u, 120, 96, 7, 7);
    pad_put(P_PAINT);
    pad_th(0xFFu);
    pad_put(P_FRAME);
    pad_wait_rows(30u);
    pad_put(P_MODE);
    pad_wait_rows(2u);
    pad_read(1u);
    pad_wait_raw(40u);
    pad_read(1u);
    pad_put(P_FRAME);
    pad_read(1u);
    pad_put(P_PAINT);
    pad_wait_rows(4u);
    pad_put(P_FRAME);
    write_image("pad-gun-mode.bin", stream_n);
}

/* Small steps of time with a poll between them over a lit disc on both ports, a reset in the
   middle of a scan, the paint again and a device change under a scan. */
static void pad_gun_split(void)
{
    pad_header(0u);
    pad_set(4u, 4u, 40, 30, 44, 34);
    pad_put(P_PAINT);
    pad_th(0xFFu);
    pad_put(P_FRAME);
    pad_wait_rows(32u);
    for (uint32_t i = 0u; i < 12u; i++) {
        pad_wait_raw(400u);
        pad_read(1u);
        pad_th((uint8_t)(i % 2u == 0u ? 0xF5u : 0xFFu));
    }
    pad_put(P_RESET);
    pad_put(0u);
    pad_put(P_PAINT);
    pad_set(0u, 4u, 0, 0, 200, 150);
    pad_th(0xFFu);
    pad_put(P_FRAME);
    pad_wait_rows(34u);
    pad_read(1u);
    pad_set(4u, 2u, 40, 30, 3, 3);
    pad_put(P_FRAME);
    write_image("pad-gun-split.bin", stream_n);
}

/* One byte at offset 64 MiB: the file system can leave the rest as a hole. */
static int oversize(const char *path)
{
    FILE *f = fopen(path, "wb");
    int ok = f != NULL && fseek(f, 64L * (long)MIB, SEEK_SET) == 0 && fputc(0, f) != EOF;
    if (f != NULL && fclose(f) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "cogmaster-test-seeds: cannot write %s\n", path);
        return 1;
    }
    return 0;
}

/* Boot ROM images for the BIOS fuzz entry. They are written after every other seed so no existing
   file changes. A pseudo-random body is owned pattern, never firmware. */
static void bios_body(size_t n, uint32_t seed)
{
    uint32_t s = seed;
    for (size_t i = 0; i < n; i++) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        image[i] = (uint8_t)(s >> 11);
    }
}

static void bios_images(void)
{
    /* DI; JR to itself. */
    static const uint8_t jam[] = {0xF3u, 0x18u, 0xFEu};
    /* DI; loop: INC A; JR loop. */
    static const uint8_t spin[] = {0xF3u, 0x3Cu, 0x18u, 0xFDu};
    /* DI; LD A,0; OUT ($3E),A (every slot on, the BIOS and the cartridge); loop: JR loop. */
    static const uint8_t both[] = {0xF3u, 0x3Eu, 0x00u, 0xD3u, 0x3Eu, 0x18u, 0xFEu};
    bios_body(8192u, 0x1B1057EDu);
    memcpy(image, jam, sizeof jam);
    write_image("bios-owned.bin", 8192u);
    memset(image, 0xFF, 8192u);
    write_image("bios-ff.bin", 8192u);
    memset(image, 0x00, 8192u);
    write_image("bios-zero.bin", 8192u);
    program(0u, 8192u, spin, sizeof spin);
    write_image("bios-never-disables.bin", 8192u);
    program(0u, 8192u, both, sizeof both);
    write_image("bios-both.bin", 8192u);
    bios_body(262145u, 0x2545F491u);
    write_image("bios-oversize.bin", 262145u);
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--oversize") == 0) {
        return oversize(argv[2]);
    }
    if (argc != 2 || argv[1][0] == '-') {
        fprintf(stderr, "usage: cogmaster-test-seeds OUTDIR | --oversize FILE\n");
        return 2;
    }
    outdir = argv[1];
    sizes();
    headers();
    boards();
    programs();
    eeprom_stream();
    pad_stream(2u, "pad-ops.bin");
    pad_stream(0u, "pad-ops-export.bin");
    pad_stream(3u, "pad-ops-gg.bin");
    pad_gun_band();
    pad_gun_edges();
    pad_gun_mode();
    pad_gun_split();
    memset(image, 0x3c, 32769u);
    write_image("save-empty.bin", 0u);
    write_image("save-one.bin", 1u);
    write_image("save-short.bin", 32767u);
    write_image("save-exact.bin", 32768u);
    write_image("save-long.bin", 32769u);
    for (uint32_t i = 1u; i < 4u; i++) {
        image[0] = (uint8_t)i;
        write_image(i == 1u   ? "save-none.bin"
                    : i == 2u ? "save-unknown.bin"
                              : "save-unsupported.bin",
                    32768u);
    }
    bios_images();
    return failures != 0 ? 1 : 0;
}
