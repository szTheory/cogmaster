/* The runner's cartridge file and its --probe report. The header fields
   come from cogmaster_probe; the digests are the runner's own, over the
   whole file and again over the body after a 512-byte copier header, using the shared SHA-1
   implementation. md5 over the whole file, copier header included, is the RetroAchievements hash
   for both machines (rcheevos rc_hash.c, generic whole-file case; HOSTS-AND-INTEGRATION section 2).
 */
#include "probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "crc32.h"
#include "exit_status.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"

/* A copier wrote 512 bytes of its own in front when the size is 512 past a
   whole number of 16 KiB pages (SMS-CARTRIDGES section 1). */
#define COPIER_HEADER 512u

static int lower(int c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

int cogmaster__has_extension(const char *path, const char *ext)
{
    size_t n = strlen(path), e = strlen(ext);
    if (n <= e) {
        return 0;
    }
    for (size_t i = 0; i < e; i++) {
        if (lower((unsigned char)path[n - e + i]) != ext[i]) {
            return 0;
        }
    }
    return 1;
}

int cogmaster__read_file(const char *path, uint8_t **out, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "cogmaster-run: cannot read %s\n", path);
        return COGMASTER__EXIT_FAILURE;
    }
    /* A byte at offset 64 MiB means the file is larger: refused before any
       of it is read, and with no size arithmetic that could overflow. */
    if (fseek(f, (long)COGMASTER__FILE_MAX, SEEK_SET) == 0 && fgetc(f) != EOF) {
        fclose(f);
        fprintf(stderr, "cogmaster-run: unsupported: %s is larger than 64 MiB (%lu bytes)\n", path,
                (unsigned long)COGMASTER__FILE_MAX);
        return COGMASTER__EXIT_REFUSED;
    }
    long end = -1;
    if (fseek(f, 0, SEEK_END) == 0) {
        end = ftell(f);
    }
    uint8_t *buf = NULL;
    int ok = end >= 0 && fseek(f, 0, SEEK_SET) == 0;
    if (ok) {
        buf = (uint8_t *)malloc(end > 0 ? (size_t)end : 1u);
        ok = buf != NULL && fread(buf, 1u, (size_t)end, f) == (size_t)end;
    }
    fclose(f);
    if (!ok) {
        fprintf(stderr, "cogmaster-run: cannot read %s\n", path);
        free(buf);
        return COGMASTER__EXIT_FAILURE;
    }
    *out = buf;
    *n = (size_t)end;
    return COGMASTER__EXIT_OK;
}

uint32_t cogmaster__auto_machine(const char *path, const uint8_t *image, size_t n)
{
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    (void)cogmaster_probe(image, n, &info);
    if (info.machine_guess != COGMASTER_MACHINE_AUTO) {
        return info.machine_guess;
    }
    return cogmaster__has_extension(path, ".gg") ? COGMASTER_MACHINE_GG : COGMASTER_MACHINE_SMS2;
}

static void hex(char *out, const uint8_t *digest, size_t n)
{
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2u * i] = digits[digest[i] >> 4];
        out[2u * i + 1u] = digits[digest[i] & 0x0fu];
    }
    out[2u * n] = '\0';
}

/* Prints sha1, sha256, crc32 and md5 of n bytes, each key followed by
   suffix. */
static void print_digests(const uint8_t *p, size_t n, const char *suffix)
{
    uint8_t d[32];
    char text[65];
    cogmaster__sha1 s1;
    cogmaster__sha1_init(&s1);
    cogmaster__sha1_update(&s1, p, n);
    cogmaster__sha1_final(&s1, d);
    hex(text, d, 20u);
    printf("sha1%s=%s\n", suffix, text);
    cogmaster__sha256 s256;
    cogmaster__sha256_init(&s256);
    cogmaster__sha256_update(&s256, p, n);
    cogmaster__sha256_final(&s256, d);
    hex(text, d, 32u);
    printf("sha256%s=%s\n", suffix, text);
    printf("crc32%s=%08lx\n", suffix, (unsigned long)cogmaster__runner_crc32(0u, p, n));
    cogmaster__md5 m;
    cogmaster__md5_init(&m);
    cogmaster__md5_update(&m, p, n);
    cogmaster__md5_final(&m, d);
    hex(text, d, 16u);
    printf("md5%s=%s\n", suffix, text);
}

/* Prints key=value, or key=none when the image has no ROM header. */
static void print_field(const char *key, uint32_t value, int header)
{
    if (header) {
        printf("%s=%lu\n", key, (unsigned long)value);
    } else {
        printf("%s=none\n", key);
    }
}

int cogmaster__probe_print(const char *path, const uint8_t *image, size_t n)
{
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    cogmaster_status st = cogmaster_probe(image, n, &info);
    int header = info.header_at != 0u;
    int copier = n % 16384u == COPIER_HEADER;
    static const char *const mappers[] = {"unsupported", "none", "sega", "codemasters"};
    uint32_t machine = cogmaster__auto_machine(path, image, n);

    printf("size=%lu\n", (unsigned long)n);
    printf("copier_header=%d\n", copier);
    if (header) {
        printf("header_at=$%04lX\n", (unsigned long)info.header_at);
    } else {
        printf("header_at=none\n");
    }
    printf("header_magic=%d\n", header);
    print_field("checksum", info.checksum, header);
    print_field("checksum_ok", info.checksum_ok, header);
    print_field("product_code", info.product_code, header);
    print_field("version", info.version, header);
    print_field("region_code", info.region_code, header);
    print_field("rom_size_code", info.rom_size_code, header);
    printf("machine_guess=%s\n", machine == COGMASTER_MACHINE_GG ? "gg" : "sms");
    printf("region_guess=%s\n", info.region_guess == COGMASTER_REGION_JAPAN ? "ntsc-j" : "ntsc-u");
    printf("mapper_guess=%s\n", mappers[info.board <= 3u ? info.board : 0u]);
    print_digests(image, n, "");
    if (copier) {
        print_digests(image + COPIER_HEADER, n - COPIER_HEADER, "_headerless");
    } else {
        printf("sha1_headerless=none\nsha256_headerless=none\n"
               "crc32_headerless=none\nmd5_headerless=none\n");
    }
    printf("identified=%lu\npublic_identity=%lu\npersistence=%lu\npersistent_size=%lu\n"
           "persistence_geometry=%lu\nvideo_mode=%lu\nidentity_sha1=%s\ntitle=%s\n",
           (unsigned long)info.identified, (unsigned long)info.public_identity,
           (unsigned long)info.persistence, (unsigned long)info.persistent_size,
           (unsigned long)info.persistence_geometry, (unsigned long)info.video_mode, info.sha1,
           info.title);
    if (st == COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD ||
        st == COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE) {
        return COGMASTER__EXIT_REFUSED;
    }
    return st == COGMASTER_STATUS_OK ? COGMASTER__EXIT_OK : COGMASTER__EXIT_FAILURE;
}
