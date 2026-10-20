/* Writes the synthetic cartridge images the runner tests read and the fuzz
   targets start from. Nothing here is a game: each image exists to reach
   one branch of the loader, the probe or the first-fault record.

     cogmaster-test-seeds OUTDIR           every image below, into OUTDIR
     cogmaster-test-seeds --oversize FILE  one file of 64 MiB + 1 byte

   The oversize file goes elsewhere: the fuzz replay reads every file in the
   seeds directory, and the runner must refuse this one before reading it.

   Sizes (a DI; HALT program at $0000):
     size-1023.sms  size-1024.sms  size-16k-512.sms (512-byte copier header)
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
   Programs at $0000, 32 KiB with no header unless noted:
     di-halt.sms  di-halt.gg  jr-self.sms  in-a-00.sms  ld-8000.sms
     ed-00.sms  be-hammer.sms  fffc-writes.sms (64 KiB, Sega board) */
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
        {"size-1023.sms", 1023u}, {"size-1024.sms", 1024u},        {"size-48k.sms", 48u * KIB},
        {"size-1m.sms", MIB},     {"size-1m-513.sms", MIB + 513u},
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
        {"di-halt.gg", di_halt, sizeof di_halt, 32u * KIB},
        {"jr-self.sms", jr_self, sizeof jr_self, 32u * KIB},
        {"in-a-00.sms", in_a_00, sizeof in_a_00, 32u * KIB},
        {"ld-8000.sms", ld_8000, sizeof ld_8000, 32u * KIB},
        {"ed-00.sms", ed_00, sizeof ed_00, 32u * KIB},
        {"be-hammer.sms", be_hammer, sizeof be_hammer, 32u * KIB},
        {"fffc-writes.sms", fffc, sizeof fffc, 64u * KIB},
    };
    for (size_t i = 0; i < sizeof list / sizeof list[0]; i++) {
        program(0u, list[i].n, list[i].code, list[i].code_n);
        write_image(list[i].name, list[i].n);
    }
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
    return failures != 0 ? 1 : 0;
}
