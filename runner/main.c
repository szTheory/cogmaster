/* cogmaster-run: the headless runner.
 *
 *   cogmaster-run --frames N [--hash] [--png N:FILE]... [--machine auto|sms|gg]
 *
 * Runs N frames (N >= 1) of one instance with no content, which shows the
 * built-in test card. With --hash it prints, after the last frame:
 *
 *   frame <N> sha256 <64 lowercase hex digits>
 *
 * The hash is SHA-256 over the frame's crop, rows top to bottom, each RGB444
 * pixel as two bytes, low byte first, so it does not depend on the host's
 * byte order.
 *
 * --png N:FILE writes frame N's crop (1 <= N <= --frames) to FILE as an RGB
 * PNG, each RGB444 channel times 0x11. It may be given more than once. The
 * whole file is built in memory after frame N and written in one call.
 *
 * Exit statuses are listed in exit_status.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "exit_status.h"
#include "palette.h"
#include "png.h"
#include "sha256.h"

/* The allocator the runner hands the core: the C library's malloc and free. */
static void *run_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void run_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static const char synopsis[] =
    "usage: cogmaster-run --frames N [--hash] [--png N:FILE]... [--machine auto|sms|gg]\n"
    "  --frames N         run N frames (N >= 1)\n"
    "  --hash             print the SHA-256 of the last frame's crop\n"
    "  --png N:FILE       write frame N's crop to FILE as a PNG (1 <= N <= --frames)\n"
    "  --machine NAME     auto (default), sms or gg\n";

static int usage(const char *why)
{
    fprintf(stderr, "cogmaster-run: %s\n%s", why, synopsis);
    return COGMASTER__EXIT_USAGE;
}

/* Parses text as a decimal number from 1 to 4294967295. Returns 0 on any
   other text. */
static int parse_count(const char *text, uint32_t *out)
{
    uint64_t v = 0;
    if (text == NULL || *text == '\0') {
        return 0;
    }
    for (const char *p = text; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
        v = v * 10u + (uint64_t)(*p - '0');
        if (v > 0xffffffffu) {
            return 0;
        }
    }
    if (v == 0u) {
        return 0;
    }
    *out = (uint32_t)v;
    return 1;
}

/* One --png request. */
typedef struct png_request {
    uint32_t frame;
    const char *path;
} png_request;

typedef struct options {
    uint32_t frames;
    int hash;
    uint32_t machine;
    png_request *pngs; /* room for one per argument */
    uint32_t png_count;
} options;

/* Parses "N:FILE" with N from 1 to 4294967295 and FILE not empty. Returns 0
   on any other text. */
static int parse_png(const char *text, png_request *out)
{
    const char *colon = text != NULL ? strchr(text, ':') : NULL;
    char number[11];
    size_t len = colon != NULL ? (size_t)(colon - text) : 0u;
    if (colon == NULL || len == 0u || len >= sizeof number || colon[1] == '\0') {
        return 0;
    }
    memcpy(number, text, len);
    number[len] = '\0';
    if (!parse_count(number, &out->frame)) {
        return 0;
    }
    out->path = colon + 1;
    return 1;
}

/* Returns 0 when the options are valid, else 2 after a usage message. */
static int parse_options(int argc, char **argv, options *o)
{
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        const char *value = i + 1 < argc ? argv[i + 1] : NULL;
        if (strcmp(arg, "--frames") == 0) {
            if (o->frames != 0u) {
                return usage("give --frames once");
            }
            if (!parse_count(value, &o->frames)) {
                return usage("--frames needs a whole number of 1 or more");
            }
            i++;
        } else if (strcmp(arg, "--hash") == 0) {
            o->hash = 1;
        } else if (strcmp(arg, "--png") == 0) {
            if (!parse_png(value, &o->pngs[o->png_count])) {
                return usage("--png needs N:FILE with N >= 1");
            }
            o->png_count++;
            i++;
        } else if (strcmp(arg, "--machine") == 0) {
            if (value != NULL && strcmp(value, "auto") == 0) {
                o->machine = COGMASTER_MACHINE_AUTO;
            } else if (value != NULL && strcmp(value, "sms") == 0) {
                o->machine = COGMASTER_MACHINE_SMS2;
            } else if (value != NULL && strcmp(value, "gg") == 0) {
                o->machine = COGMASTER_MACHINE_GG;
            } else {
                return usage("--machine needs auto, sms or gg");
            }
            i++;
        } else {
            return usage(arg[0] == '-' ? "unknown option" : "unexpected argument");
        }
    }
    if (o->frames == 0u) {
        return usage("--frames N is required");
    }
    for (uint32_t k = 0; k < o->png_count; k++) {
        if (o->pngs[k].frame > o->frames) {
            return usage("--png N must not exceed --frames");
        }
    }
    return COGMASTER__EXIT_OK;
}

/* Copies the crop of the frame just run into pixels, which has room for the
   largest field, and fills io. Returns the exit status. */
static int fetch_crop(const cogmaster *m, uint16_t *pixels, cogmaster_frame *io)
{
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    cogmaster_status st = cogmaster_get_info(m, &info);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_get_info failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
    memset(io, 0, sizeof *io);
    io->size = (uint32_t)sizeof *io;
    io->pixels = pixels;
    io->pitch_bytes = info.crop_w * 2u;
    st = cogmaster_get_frame(m, io);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_get_frame failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
    if (io->w > 256u) {
        fprintf(stderr, "cogmaster-run: a %lu-dot row is wider than the field\n",
                (unsigned long)io->w);
        return COGMASTER__EXIT_FAILURE;
    }
    return COGMASTER__EXIT_OK;
}

/* Prints the hash line for the frame just run. Returns the exit status. */
static int print_hash(const cogmaster *m, uint32_t frames, uint16_t *pixels)
{
    cogmaster_frame io;
    int status = fetch_crop(m, pixels, &io);
    if (status != COGMASTER__EXIT_OK) {
        return status;
    }
    /* Each row is serialised low byte first before hashing; the uint16_t
       array itself is never hashed, so the result is the same on a
       big-endian host. */
    uint8_t row[2u * 256u];
    cogmaster__sha256 s;
    uint8_t digest[32];
    char hex[65];
    cogmaster__sha256_init(&s);
    for (uint32_t y = 0; y < io.h; y++) {
        const uint16_t *src = pixels + (size_t)y * (io.pitch_bytes / 2u);
        for (uint32_t x = 0; x < io.w; x++) {
            row[2u * x] = (uint8_t)(src[x] & 0xFFu);
            row[2u * x + 1u] = (uint8_t)(src[x] >> 8);
        }
        cogmaster__sha256_update(&s, row, (size_t)io.w * 2u);
    }
    cogmaster__sha256_final(&s, digest);
    cogmaster__sha256_hex(digest, hex);
    printf("frame %lu sha256 %s\n", (unsigned long)frames, hex);
    return COGMASTER__EXIT_OK;
}

/* Writes the crop of the frame just run to path as a PNG. The file is built
   in memory and written with one fwrite, after the frame is complete; a
   file that cannot be written in full is removed. Returns the exit
   status. */
static int write_png(const cogmaster *m, const char *path, uint16_t *pixels)
{
    cogmaster_frame io;
    int status = fetch_crop(m, pixels, &io);
    if (status != COGMASTER__EXIT_OK) {
        return status;
    }
    size_t size = cogmaster__png_size(io.w, io.h);
    uint8_t *rgb = (uint8_t *)malloc((size_t)io.w * io.h * 3u + 1u);
    uint8_t *file = size != 0u ? (uint8_t *)malloc(size) : NULL;
    if (rgb == NULL || file == NULL) {
        fprintf(stderr, "cogmaster-run: out of memory\n");
        free(rgb);
        free(file);
        return COGMASTER__EXIT_FAILURE;
    }
    for (uint32_t y = 0; y < io.h; y++) {
        const uint16_t *src = pixels + (size_t)y * (io.pitch_bytes / 2u);
        uint8_t *dst = rgb + (size_t)y * io.w * 3u;
        for (uint32_t x = 0; x < io.w; x++) {
            uint32_t c = cogmaster__rgb444_to_rgb888(src[x]);
            dst[3u * x] = (uint8_t)(c >> 16);
            dst[3u * x + 1u] = (uint8_t)(c >> 8);
            dst[3u * x + 2u] = (uint8_t)c;
        }
    }
    size_t n = cogmaster__png_write(file, size, rgb, io.w, io.h);
    FILE *f = n != 0u ? fopen(path, "wb") : NULL;
    int ok = f != NULL && fwrite(file, 1, n, f) == n;
    if (f != NULL && fclose(f) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "cogmaster-run: cannot write %s\n", path);
        if (f != NULL) {
            (void)remove(path);
        }
        status = COGMASTER__EXIT_FAILURE;
    }
    free(rgb);
    free(file);
    return status;
}

/* Runs the frames. The pixel buffer is on the heap: a field is too large for
   a small stack such as wasm32's. Returns the exit status. */
static int run(const options *opt)
{
    uint16_t *pixels = (uint16_t *)malloc((size_t)256u * 240u * sizeof *pixels);
    if (pixels == NULL) {
        fprintf(stderr, "cogmaster-run: out of memory\n");
        return COGMASTER__EXIT_FAILURE;
    }
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = run_alloc;
    cfg.allocator.free = run_free;
    cogmaster *m = NULL;
    cogmaster_status st = cogmaster_create(&cfg, &m);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_create failed with status %lu\n",
                (unsigned long)st);
        free(pixels);
        return COGMASTER__EXIT_FAILURE;
    }

    int status = COGMASTER__EXIT_OK;
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = opt->machine;
    st = cogmaster_load(m, &media);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_load failed with status %lu\n",
                (unsigned long)st);
        status = COGMASTER__EXIT_FAILURE;
    }
    /* A 64-bit count, so the loop ends when --frames is 4294967295. */
    for (uint64_t n = 1; n <= opt->frames && status == COGMASTER__EXIT_OK; n++) {
        st = cogmaster_run_frame(m, NULL);
        if (st != COGMASTER_STATUS_OK) {
            fprintf(stderr, "cogmaster-run: cogmaster_run_frame failed with status %lu\n",
                    (unsigned long)st);
            status = COGMASTER__EXIT_FAILURE;
        }
        for (uint32_t k = 0; k < opt->png_count && status == COGMASTER__EXIT_OK; k++) {
            if (opt->pngs[k].frame == n) {
                status = write_png(m, opt->pngs[k].path, pixels);
            }
        }
    }
    if (status == COGMASTER__EXIT_OK && opt->hash) {
        status = print_hash(m, opt->frames, pixels);
    }
    cogmaster_destroy(m);
    free(pixels);
    return status;
}

int main(int argc, char **argv)
{
    options opt;
    memset(&opt, 0, sizeof opt);
    /* At most one --png per two arguments, so argc entries always suffice. */
    opt.pngs = (png_request *)malloc((size_t)(argc > 0 ? argc : 1) * sizeof *opt.pngs);
    if (opt.pngs == NULL) {
        fprintf(stderr, "cogmaster-run: out of memory\n");
        return COGMASTER__EXIT_FAILURE;
    }
    int status = parse_options(argc, argv, &opt);
    if (status == COGMASTER__EXIT_OK) {
        status = run(&opt);
    }
    free(opt.pngs);
    if (fflush(stdout) != 0) {
        fprintf(stderr, "cogmaster-run: cannot write output\n");
        return COGMASTER__EXIT_FAILURE;
    }
    return status;
}
