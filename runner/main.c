/* cogmaster-run: the headless runner.
 *
 *   cogmaster-run [--machine M] [--region R] --frames N [OPTION]... [FILE]
 *   cogmaster-run [--machine M] [--region R] --bench N [OPTION]... [FILE]
 *   cogmaster-run --probe FILE
 *   cogmaster-run --help | --version
 *
 * Runs N frames (N >= 1) of one instance. FILE (or --rom FILE) is the
 * cartridge image; without one the built-in test card shows. args.c parses
 * the command line; probe.c reads the file and prints --probe. With --hash
 * it prints, after the last frame:
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
 * --bench N also times each frame with the host's monotonic clock and prints
 * the report described in bench.h. SIGINT or SIGTERM ends the run after the
 * frame in progress: what was asked is printed for the frames that ran, and
 * the status is 0 unless an output or save flush fails.
 *
 * --hash-audio drains the core's sound each frame (audio.h) and, after the
 * last frame, prints one line for the merged level changes and one for the
 * samples.
 *
 * --trace first-fault reads the core's first-fault record after each frame,
 * stops at the first fault and prints it as one line, or "fault none".
 *
 * --eyes counts the 3-D glasses eye the core reports for each frame and,
 * after the last frame, prints one line of key=value counts (docs/runner.md).
 * It changes no hashed output.
 *
 * --mailbox peeks the RAM mailbox after each frame, stops at the first
 * frame whose status reads $00, and prints the mailbox (mailbox.h).
 * docs/runner.md describes all of this for users.
 *
 * --boot bios --bios FILE starts from that 8192-byte Master System BIOS
 * instead of the fast hand-off; bios_load reads and labels it, and prints
 * neither its path nor a byte.
 *
 * Exit statuses are listed in exit_status.h and printed by --help.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "audio.h"
#include "bench.h"
#include "bios.h"
#include "clock.h"
#include "cogmaster.h"
#include "exit_status.h"
#include "expect.h"
#include "input.h"
#include "mailbox.h"
#include "palette.h"
#include "path.h"
#include "png.h"
#include "probe.h"
#include "rss.h"
#include "save.h"
#include "sha256.h"
#include "stop.h"

/* The tests build a second runner with COGMASTER_RUN_RAISE_AT=N, which
   raises SIGTERM after frame N is admitted, to show the stop is clean. The
   installed runner has no such hook. */
#ifdef COGMASTER_RUN_RAISE_AT
#include <signal.h>
#ifndef COGMASTER_RUN_RAISE_SIGNAL
#define COGMASTER_RUN_RAISE_SIGNAL SIGTERM
#endif
#endif

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
    FILE *f = n != 0u ? cogmaster__path_open(path, "wb") : NULL;
    int ok = f != NULL && fwrite(file, 1, n, f) == n;
    if (f != NULL && fclose(f) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "cogmaster-run: cannot write %s\n", path);
        if (f != NULL) {
            (void)cogmaster__path_remove(path);
        }
        status = COGMASTER__EXIT_FAILURE;
    }
    free(rgb);
    free(file);
    return status;
}

/* Reads the first-fault record into fault. Returns the exit status. */
static int read_fault(const cogmaster *m, cogmaster_fault *fault)
{
    cogmaster_status st = cogmaster_get_fault(m, fault);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_get_fault failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
    return COGMASTER__EXIT_OK;
}

/* Prints the first fault as one line of key=value fields, or "fault none".
   The kind and device names are the ones docs/embedding.md gives each
   COGMASTER_FAULT_* constant. */
static void print_fault(const cogmaster_fault *f)
{
    static const char *const kinds[] = {
        [0] = "none",
        [COGMASTER_FAULT_KIND_UNMAPPED_READ] = "unmapped_read",
        [COGMASTER_FAULT_KIND_UNMAPPED_WRITE] = "unmapped_write",
        [COGMASTER_FAULT_KIND_ROM_WRITE] = "rom_write",
        [COGMASTER_FAULT_KIND_MAPPER_RANGE] = "mapper_range",
        [COGMASTER_FAULT_KIND_ED_UNDEFINED] = "ed_undefined",
        [COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE] = "unsupported_video_mode",
    };
    static const char *const devices[] = {
        [COGMASTER_FAULT_DEVICE_NONE] = "none",     [COGMASTER_FAULT_DEVICE_CPU] = "cpu",
        [COGMASTER_FAULT_DEVICE_CART] = "cart",     [COGMASTER_FAULT_DEVICE_RAM] = "ram",
        [COGMASTER_FAULT_DEVICE_MAPPER] = "mapper", [COGMASTER_FAULT_DEVICE_CTRL] = "ctrl",
        [COGMASTER_FAULT_DEVICE_VDP] = "vdp",
    };
    if (!f->present) {
        printf("fault none\n");
        return;
    }
    const char *kind = f->kind < sizeof kinds / sizeof kinds[0] ? kinds[f->kind] : "unknown";
    const char *dev =
        f->device < sizeof devices / sizeof devices[0] ? devices[f->device] : "unknown";
    printf("fault frame=%llu line=%lu dot=%lu pc=$%04X op=$%02X addr=$%04X width=%u dev=%s "
           "kind=%s\n",
           (unsigned long long)f->frame, (unsigned long)f->line, (unsigned long)f->dot,
           (unsigned)f->pc, (unsigned)f->opcode, (unsigned)f->addr, (unsigned)f->width, dev, kind);
}

/* The exit status for cogmaster_load's status, after a message: a refusal
   for what the core does not support, a failure for anything else. */
static int load_status(cogmaster_status st, const uint8_t *image, size_t image_size)
{
    switch (st) {
    case COGMASTER_STATUS_OK:
        return COGMASTER__EXIT_OK;
    case COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE:
        fprintf(stderr,
                "cogmaster-run: unsupported: an image of %lu bytes; the body after any 512-byte "
                "copier header must be 1024 bytes to 1 MiB\n",
                (unsigned long)image_size);
        return COGMASTER__EXIT_REFUSED;
    case COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD:
        if (image == NULL || !cogmaster__refusal_board(image, image_size)) {
            fprintf(stderr, "cogmaster-run: unsupported: the cartridge's board\n");
        }
        return COGMASTER__EXIT_REFUSED;
    case COGMASTER_STATUS_ERR_PROFILE_REJECTED:
        fprintf(stderr, "cogmaster-run: unsupported: no such console was made for this machine "
                        "and region\n");
        return COGMASTER__EXIT_REFUSED;
    default:
        fprintf(stderr, "cogmaster-run: cogmaster_load failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
}

/* The one call that sets the controllers before frame 1. A device the machine and region cannot
   take is a refusal that names the fix; the Paddle was made for the Japanese console only and the
   Game Gear has no controller ports (the same rule as src/ports.c). */
static cogmaster_status send_devices(cogmaster *m, uint32_t port1, uint32_t port2)
{
    cogmaster_input in = {0};
    in.size = sizeof in;
    in.port1 = port1;
    in.port2 = port2;
    return cogmaster_set_input(m, &in);
}

static int set_devices(cogmaster *m, uint32_t port1, uint32_t port2)
{
    cogmaster_status st = send_devices(m, port1, port2);
    if (st == COGMASTER_STATUS_OK) {
        return COGMASTER__EXIT_OK;
    }
    if (st != COGMASTER_STATUS_ERR_PROFILE_REJECTED) {
        fprintf(stderr, "cogmaster-run: cogmaster_set_input failed with status %lu\n",
                (unsigned long)st);
        return COGMASTER__EXIT_FAILURE;
    }
    /* A refused call changes nothing, so asking about port 1 alone names the culprit. */
    int first =
        send_devices(m, port1, COGMASTER_DEVICE_PAD) == COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    uint32_t bad = first ? port1 : port2;
    fprintf(
        stderr, "cogmaster-run: unsupported: %s on port %d is not available here; %s\n",
        cogmaster__device_name(bad), first ? 1 : 2,
        bad == COGMASTER_DEVICE_PADDLE
            ? "the Paddle needs --region ntsc-j on a Master System, and the Game Gear has no such "
              "controller"
            : "the Game Gear has no such controller");
    return COGMASTER__EXIT_REFUSED;
}

/* Runs the frames. The pixel buffer and the bench samples are on the heap:
   a field is too large for a small stack such as wasm32's, and the samples
   take 4 bytes per frame. Returns the exit status. */
static int run(const cogmaster__run_options *opt, const uint8_t *image, size_t image_size,
               const uint8_t *bios)
{
    cogmaster__expect expectations;
    if (opt->expect != NULL) {
        int rc = cogmaster__expect_load(&expectations, opt->expect, opt->frames);
        if (rc != COGMASTER__EXIT_OK)
            return rc;
    }
    uint16_t *pixels = (uint16_t *)malloc((size_t)256u * 240u * sizeof *pixels);
    uint32_t *samples = opt->bench ? (uint32_t *)malloc(cogmaster__bench_bytes(opt->frames)) : NULL;
    if (pixels == NULL || (opt->bench && samples == NULL)) {
        fprintf(stderr, "cogmaster-run: out of memory\n");
        free(pixels);
        free(samples);
        return COGMASTER__EXIT_FAILURE;
    }
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = run_alloc;
    cfg.allocator.free = run_free;
    if (opt->hash_audio) {
        cfg.trace_records = COGMASTER_RUN_TRACE_RECORDS;
    }
    cogmaster *m = NULL;
    cogmaster_status st = cogmaster_create(&cfg, &m);
    if (st != COGMASTER_STATUS_OK) {
        fprintf(stderr, "cogmaster-run: cogmaster_create failed with status %lu\n",
                (unsigned long)st);
        free(pixels);
        free(samples);
        return COGMASTER__EXIT_FAILURE;
    }

    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = image_size;
    media.machine = opt->machine;
    if (image != NULL && media.machine == COGMASTER_MACHINE_AUTO) {
        media.machine = cogmaster__auto_machine(opt->file, image, image_size);
    }
    media.region = opt->region;
    media.tv = opt->tv;
    media.video_mode = opt->video_mode;
    media.bios = bios;
    media.bios_size = bios != NULL ? (size_t)COGMASTER_BIOS_SIZE : 0u;
    int status = load_status(cogmaster_load(m, &media), image, image_size);
    /* auto takes the library's default for this content, already degraded to a pad when the
       machine and region cannot take it; the call is made once, so a later frame never changes it.
     */
    uint32_t port1 = opt->port1;
    uint32_t port2 = opt->port2;
    if (status == COGMASTER__EXIT_OK) {
        cogmaster_info dev = {0};
        dev.size = sizeof dev;
        if (cogmaster_get_info(m, &dev) != COGMASTER_STATUS_OK) {
            status = COGMASTER__EXIT_FAILURE;
        } else {
            port1 = opt->port1_given ? opt->port1 : dev.default_port1;
            port2 = opt->port2_given ? opt->port2 : dev.default_port2;
            status = set_devices(m, port1, port2);
        }
    }
    cogmaster__audio audio = {0};
    if (status == COGMASTER__EXIT_OK && opt->hash_audio) {
        cogmaster_info info = {0};
        info.size = sizeof info;
        status = cogmaster_get_info(m, &info) == COGMASTER_STATUS_OK
                     ? cogmaster__audio_open(&audio, info.tv == COGMASTER_TV_PAL)
                     : COGMASTER__EXIT_FAILURE;
    }
    cogmaster__save save = {0};
    int save_loaded = 0;
    if (status == COGMASTER__EXIT_OK && opt->save_dir != NULL) {
        status = cogmaster__save_load(&save, m, opt->save_dir, opt->file);
        save_loaded = status == COGMASTER__EXIT_OK;
    }
    FILE *input = NULL;
    if (status == COGMASTER__EXIT_OK && opt->input != NULL) {
        input = cogmaster__path_open(opt->input, "rb");
        int bad_port = 0;
        status = input != NULL
                     ? cogmaster__input_validate(input, opt->frames, port1, port2, &bad_port)
                     : COGMASTER__EXIT_FAILURE;
        if (bad_port != 0) {
            uint32_t bad_device = bad_port == 1 ? port1 : port2;
            fprintf(stderr, "cogmaster-run: input file error: port %d is %s, which takes %s\n",
                    bad_port, cogmaster__device_name(bad_device),
                    bad_device == COGMASTER_DEVICE_LIGHT_PHASER
                        ? "an aim of x 1 to 256 and y 1 to 240, or 0 0"
                        : "no motion");
        } else if (status != COGMASTER__EXIT_OK) {
            fprintf(stderr, "cogmaster-run: input file error (status %d)\n", status);
        }
    }
    if (status == COGMASTER__EXIT_OK && opt->expect != NULL) {
        cogmaster_info info = {0};
        info.size = sizeof info;
        cogmaster_cart_info cart = {0};
        cart.size = sizeof cart;
        cogmaster_versions versions = {0};
        versions.size = sizeof versions;
        if (cogmaster_get_info(m, &info) != COGMASTER_STATUS_OK ||
            cogmaster_get_versions(&versions) != COGMASTER_STATUS_OK ||
            (image != NULL && cogmaster_probe(image, image_size, &cart) != COGMASTER_STATUS_OK))
            status = COGMASTER__EXIT_FAILURE;
        cogmaster__sha256 hash;
        cogmaster__sha256_init(&hash);
        if (input != NULL) {
            int ch;
            while ((ch = fgetc(input)) != EOF) {
                uint8_t byte = (uint8_t)ch;
                cogmaster__sha256_update(&hash, &byte, 1u);
            }
            if (ferror(input) || fseek(input, 0, SEEK_SET) != 0)
                status = COGMASTER__EXIT_FAILURE;
        }
        uint8_t digest[32];
        char hex[65];
        cogmaster__sha256_final(&hash, digest);
        cogmaster__sha256_hex(digest, hex);
        printf("execution machine=%lu region=%lu tv=%lu video=%lu mapper=%lu port1=%s port2=%s "
               "script_sha256=%s cap=%lu behaviour=%lu\n",
               (unsigned long)info.machine, (unsigned long)info.region, (unsigned long)info.tv,
               (unsigned long)info.video_mode, (unsigned long)cart.board,
               cogmaster__device_name(port1), cogmaster__device_name(port2), hex,
               (unsigned long)opt->frames, (unsigned long)versions.behaviour);
    }
    cogmaster_fault fault;
    memset(&fault, 0, sizeof fault);
    fault.size = (uint32_t)sizeof fault;
    /* A 64-bit count, so the loop ends when --frames is 4294967295. Only
       cogmaster_run_frame is timed, not hashing or writing files. */
    uint32_t ran = 0;
    uint32_t lag_frames = 0;
    uint32_t eye_count[3] = {0u, 0u, 0u}; /* indexed by COGMASTER_GLASSES_* */
    uint32_t eye_flips = 0;
    uint32_t eye_last = COGMASTER_GLASSES_NONE;
    int mailbox_running = 0;
    for (uint64_t n = 1; n <= opt->frames && status == COGMASTER__EXIT_OK; n++) {
        cogmaster_input in = {0};
        in.size = sizeof in;
        if (input != NULL) {
            status = cogmaster__input_read(input, &in);
            if (status != COGMASTER__EXIT_OK) {
                break;
            }
        }
        in.port1 = port1;
        in.port2 = port2;
        if (cogmaster_set_input(m, &in) != COGMASTER_STATUS_OK) {
            status = COGMASTER__EXIT_FAILURE;
            break;
        }
        uint64_t start_ns = opt->bench ? cogmaster__clock_ns() : 0u;
        cogmaster_run_result result = {0};
        result.size = sizeof result;
#ifdef COGMASTER_RUN_RAISE_AT
        if (n == COGMASTER_RUN_RAISE_AT) {
            (void)raise(COGMASTER_RUN_RAISE_SIGNAL);
        }
#endif
        st = cogmaster_run_frame(m, &result);
        if (st == COGMASTER_STATUS_OK) {
            lag_frames += result.lagged;
            uint32_t eye = result.glasses <= COGMASTER_GLASSES_LEFT ? result.glasses : 0u;
            eye_count[eye]++;
            if (eye != COGMASTER_GLASSES_NONE) {
                eye_flips += eye_last != COGMASTER_GLASSES_NONE && eye_last != eye;
                eye_last = eye;
            }
        }
        if (opt->bench) {
            samples[ran] = cogmaster__bench_us(cogmaster__clock_ns() - start_ns);
        }
        ran = (uint32_t)n;
        if (st != COGMASTER_STATUS_OK) {
            fprintf(stderr, "cogmaster-run: cogmaster_run_frame failed with status %lu\n",
                    (unsigned long)st);
            status = COGMASTER__EXIT_FAILURE;
        }
        if (status == COGMASTER__EXIT_OK && opt->hash_audio) {
            status = cogmaster__audio_frame(&audio, m, n);
        }
        if (status == COGMASTER__EXIT_OK && opt->expect != NULL)
            status = cogmaster__expect_frame(&expectations, m, (uint32_t)n, result.display_enabled,
                                             pixels);
        for (uint32_t k = 0; k < opt->png_count && status == COGMASTER__EXIT_OK; k++) {
            if (opt->pngs[k].frame == n) {
                status = write_png(m, opt->pngs[k].path, pixels);
            }
        }
        /* --trace first-fault stops after the frame that recorded the
           first fault. */
        if (opt->trace && status == COGMASTER__EXIT_OK) {
            status = read_fault(m, &fault);
            if (fault.present) {
                break;
            }
        }
        /* --mailbox stops after the frame whose status reads done, once a
           frame has ended with it running. */
        if (opt->mailbox && status == COGMASTER__EXIT_OK &&
            cogmaster__mailbox_done(m, &mailbox_running)) {
            break;
        }
        /* --save-interval counts frames, never time. A failed periodic flush
           ends the run at once; the exit flush below stays as it is. */
        if (save_loaded && status == COGMASTER__EXIT_OK && opt->save_interval != 0u &&
            n % opt->save_interval == 0u && cogmaster__save_flush(&save, m) != COGMASTER__EXIT_OK) {
            status = COGMASTER__EXIT_FAILURE;
            break;
        }
        /* A stop request ends the run only between whole frames, after the
           frame's outputs are written. */
        if (cogmaster__stop_requested()) {
            break;
        }
    }
    /* A stop before a --png frame (first fault, mailbox done or a signal)
       leaves that file unwritten; say so, as the run itself succeeded. */
    for (uint32_t k = 0; k < opt->png_count && status == COGMASTER__EXIT_OK; k++) {
        if (opt->pngs[k].frame > ran) {
            fprintf(stderr,
                    "cogmaster-run: --png %lu:%s not written: the run stopped after frame %lu\n",
                    (unsigned long)opt->pngs[k].frame, opt->pngs[k].path, (unsigned long)ran);
        }
    }
    if (status == COGMASTER__EXIT_OK && opt->trace) {
        print_fault(&fault);
    }
    if (status == COGMASTER__EXIT_OK && opt->mailbox) {
        cogmaster__mailbox_print(m);
    }
    if (status == COGMASTER__EXIT_OK && opt->hash) {
        status = print_hash(m, ran, pixels);
    }
    if (status == COGMASTER__EXIT_OK && opt->hash_audio) {
        cogmaster__audio_print(&audio, ran);
    }
    if (status == COGMASTER__EXIT_OK && opt->bench) {
        char report[512];
        size_t len =
            cogmaster__bench_format(report, sizeof report, samples, ran, cogmaster__max_rss_kib());
        fwrite(report, 1u, len, stdout);
    }
    if (status == COGMASTER__EXIT_OK && opt->input != NULL) {
        printf("lag_frames=%lu\n", (unsigned long)lag_frames);
    }
    if (status == COGMASTER__EXIT_OK && opt->expect != NULL) {
        status = read_fault(m, &fault);
        if (status == COGMASTER__EXIT_OK)
            print_fault(&fault);
    }
    if (status == COGMASTER__EXIT_OK && opt->expect != NULL)
        status = cogmaster__expect_finish(&expectations);
    if (status == COGMASTER__EXIT_OK && opt->eyes) {
        printf("eyes frames=%lu none=%lu left=%lu right=%lu flips=%lu last=%s\n",
               (unsigned long)ran, (unsigned long)eye_count[COGMASTER_GLASSES_NONE],
               (unsigned long)eye_count[COGMASTER_GLASSES_LEFT],
               (unsigned long)eye_count[COGMASTER_GLASSES_RIGHT], (unsigned long)eye_flips,
               eye_last == COGMASTER_GLASSES_LEFT    ? "left"
               : eye_last == COGMASTER_GLASSES_RIGHT ? "right"
                                                     : "none");
    }
    if (input != NULL && fclose(input) != 0) {
        status = COGMASTER__EXIT_FAILURE;
    }
    /* Both signal handlers only request a stop. Persistence runs here after
       the admitted frame and its outputs, outside signal context. */
    if (save_loaded && cogmaster__save_flush(&save, m) != COGMASTER__EXIT_OK)
        status = COGMASTER__EXIT_FAILURE;
    cogmaster__save_dispose(&save);
    cogmaster__audio_close(&audio);
    cogmaster_destroy(m);
    free(pixels);
    free(samples);
    return status;
}

/* Writes the digest as lowercase hex digits and a NUL. */
static void sha1_text(const uint8_t d[20], char out[41])
{
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < 20u; i++) {
        out[2u * i] = digits[d[i] >> 4];
        out[2u * i + 1u] = digits[d[i] & 0x0fu];
    }
    out[40] = '\0';
}

/* Reads the BIOS for --boot bios into buf (room for the read cap) and prints its label line. Faults
   prevented: a missing firmware request that exits 1 like a crash, a Game Gear cartridge that
   starts a Master System BIOS, and a message that echoes the path or a byte of a dump. Every
   refusal text is fixed. Recognition is a label only: a dump of the right size that is not in the
   table still boots. Returns the exit status. */
static int bios_load(const cogmaster__run_options *opt, const uint8_t *image, size_t image_size,
                     uint8_t *buf)
{
    size_t n = 0;
    size_t rows = 0;
    uint8_t digest[20] = {0};
    char text[41];
    const char *name;
    int rc;
    if (opt->machine == COGMASTER_MACHINE_GG ||
        (opt->machine == COGMASTER_MACHINE_AUTO &&
         cogmaster__auto_machine(opt->file, image, image_size) == COGMASTER_MACHINE_GG)) {
        fprintf(stderr, "cogmaster-run: unsupported: a BIOS cannot start a Game Gear cartridge\n");
        return COGMASTER__EXIT_REFUSED;
    }
    rc = opt->bios != NULL ? cogmaster__bios_read(opt->bios, buf, &n) : COGMASTER_BIOS_READ_ABSENT;
    if (rc == COGMASTER_BIOS_READ_ABSENT) {
        fprintf(stderr,
                "cogmaster-run: unsupported: a BIOS is required and absent; give --bios FILE\n");
        return COGMASTER__EXIT_REFUSED;
    }
    if (rc != COGMASTER_BIOS_READ_OK) {
        fprintf(stderr, "cogmaster-run: cannot read the BIOS file\n");
        return COGMASTER__EXIT_FAILURE;
    }
    name = n < COGMASTER_BIOS_READ_CAP
               ? cogmaster__bios_identify(cogmaster__bios_rows(&rows), rows, buf, n, digest)
               : NULL;
    if (n != COGMASTER_BIOS_SIZE) {
        fprintf(stderr,
                "cogmaster-run: unsupported: a BIOS file of %s%lu bytes%s%s%s; this release "
                "runs 8192-byte Master System BIOS images\n",
                n < COGMASTER_BIOS_READ_CAP ? "" : "over ",
                (unsigned long)(n < COGMASTER_BIOS_READ_CAP ? n : COGMASTER_BIOS_SIZE),
                name != NULL ? " (" : "", name != NULL ? name : "", name != NULL ? ")" : "");
        return COGMASTER__EXIT_REFUSED;
    }
    if (name != NULL) {
        fprintf(stderr, "cogmaster-run: BIOS recognised: %s\n", name);
    } else {
        sha1_text(digest, text);
        fprintf(stderr, "cogmaster-run: BIOS not recognised (sha1 %s); running it as given\n",
                text);
    }
    return COGMASTER__EXIT_OK;
}

/* Reads the file, if any, then probes it or runs it. Returns the exit
   status. */
static int start(const cogmaster__run_options *opt)
{
    if (opt->file == NULL) {
        return run(opt, NULL, 0u, NULL);
    }
    /* SG-1000 and SC-3000 images are named .sg and .sc; their support is
       planned, so they are refused before they are read. */
    if (cogmaster__has_extension(opt->file, ".sg") || cogmaster__has_extension(opt->file, ".sc")) {
        fprintf(stderr, "cogmaster-run: unsupported: SG-1000 (planned)\n");
        return COGMASTER__EXIT_REFUSED;
    }
    uint8_t *image = NULL;
    size_t n = 0;
    int status = cogmaster__read_file(opt->file, &image, &n);
    if (status != COGMASTER__EXIT_OK) {
        return status;
    }
    if (opt->probe) {
        status = cogmaster__probe_print(opt->file, image, n);
    } else if (opt->boot_bios) {
        uint8_t bios[COGMASTER_BIOS_READ_CAP];
        status = bios_load(opt, image, n, bios);
        if (status == COGMASTER__EXIT_OK) {
            status = run(opt, image, n, bios);
        }
    } else {
        status = run(opt, image, n, NULL);
    }
    free(image);
    return status;
}

static int run_main(int argc, char **argv)
{
    cogmaster__run_options opt;
    memset(&opt, 0, sizeof opt);
    /* At most one --png per two arguments, so argc entries always suffice. */
    opt.pngs = (cogmaster__png_request *)malloc((size_t)(argc > 0 ? argc : 1) * sizeof *opt.pngs);
    if (opt.pngs == NULL) {
        fprintf(stderr, "cogmaster-run: out of memory\n");
        return COGMASTER__EXIT_FAILURE;
    }
    int status = cogmaster__parse_options(argc, argv, &opt);
    if (status == COGMASTER__EXIT_OK && !opt.printed) {
        cogmaster__stop_install();
        status = start(&opt);
    }
    free(opt.pngs);
    if (fflush(stdout) != 0) {
        fprintf(stderr, "cogmaster-run: cannot write output\n");
        return COGMASTER__EXIT_FAILURE;
    }
    return status;
}

#ifdef _WIN32
/* The CRT's narrow argv loses characters outside the active codepage. Keep
   the OS's wide arguments and use UTF-8 inside the host. */
int wmain(int argc, wchar_t **wide)
{
    char **argv = (char **)calloc((size_t)argc + 1u, sizeof *argv);
    if (argv == NULL)
        return COGMASTER__EXIT_FAILURE;
    int ok = 1;
    for (int i = 0; i < argc && ok; i++) {
        int n = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, NULL, 0, NULL, NULL);
        argv[i] = n > 0 ? (char *)malloc((size_t)n) : NULL;
        ok = argv[i] != NULL &&
             WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, argv[i], n, NULL, NULL) != 0;
    }
    int status = ok ? run_main(argc, argv) : COGMASTER__EXIT_FAILURE;
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
    return status;
}
#else
int main(int argc, char **argv)
{
    return run_main(argc, argv);
}
#endif
