/* The runner's command line. Any option not in the synopsis, a second file
   or a combination the synopsis does not allow is a usage error, and nothing
   runs. */
#include "args.h"

#include <stdio.h>
#include <string.h>

#include "bench.h"
#include "cogmaster.h"
#include "exit_status.h"

static const char synopsis[] =
    "usage: cogmaster-run [--machine NAME] [--region NAME] --frames N [OPTION]... [FILE]\n"
    "       cogmaster-run [--machine NAME] [--region NAME] --bench N [OPTION]... [FILE]\n"
    "       cogmaster-run --probe FILE\n"
    "       cogmaster-run --help | --version\n"
    "  FILE, --rom FILE     the cartridge image to run; without one, the test card\n"
    "  --machine NAME       auto (default), sms or gg\n"
    "  --region NAME        auto (default), ntsc-u, ntsc-j or pal\n"
    "  --frames N           run N frames (N >= 1)\n"
    "  --bench N            run N frames and report the time per frame\n"
    "  --hash               print the SHA-256 of the last frame's crop\n"
    "  --input FILE         absolute pad and console record per frame\n"
    "  --port1 pad|none     first controller device (default pad)\n"
    "  --port2 pad|none     second controller device (default pad)\n"
    "  --png N:FILE         write frame N's crop to FILE as a PNG (1 <= N <= --frames)\n"
    "  --trace first-fault  stop at the first fault and print it, or 'fault none'\n"
    "  --mailbox            stop when the RAM mailbox reports done; print its records\n"
    "  --probe              print the file's header fields and digests; run nothing\n"
    "  --help               print this text and the exit statuses\n"
    "  --version            print the version\n";

static int usage(const char *why)
{
    fprintf(stderr, "cogmaster-run: %s\n%s", why, synopsis);
    return COGMASTER__EXIT_USAGE;
}

static void help(void)
{
    fputs(synopsis, stdout);
    fputs("exit status:\n", stdout);
    for (size_t i = 0; i < sizeof cogmaster__exit_statuses / sizeof cogmaster__exit_statuses[0];
         i++) {
        printf("  %d  %-7s  %s\n", cogmaster__exit_statuses[i].code,
               cogmaster__exit_statuses[i].name, cogmaster__exit_statuses[i].meaning);
    }
}

/* The library's version, which is the release's version. */
static void version(void)
{
    cogmaster_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    (void)cogmaster_get_versions(&v);
    printf("cogmaster-run %lu.%lu.%lu\n", (unsigned long)v.library_major,
           (unsigned long)v.library_minor, (unsigned long)v.library_patch);
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

/* Parses "N:FILE" with N from 1 to 4294967295 and FILE not empty. Returns 0
   on any other text. */
static int parse_png(const char *text, cogmaster__png_request *out)
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

static int is(const char *value, const char *word)
{
    return value != NULL && strcmp(value, word) == 0;
}

/* --machine NAME. Returns 0 for an unknown name. */
static int parse_machine(const char *value, uint32_t *machine)
{
    if (is(value, "auto")) {
        *machine = COGMASTER_MACHINE_AUTO;
    } else if (is(value, "sms")) {
        *machine = COGMASTER_MACHINE_SMS2;
    } else if (is(value, "gg")) {
        *machine = COGMASTER_MACHINE_GG;
    } else {
        return 0;
    }
    return 1;
}

/* --region NAME sets the console's region and the TV standard together.
   Returns 0 for an unknown name. */
static int parse_region(const char *value, uint32_t *region, uint32_t *tv)
{
    if (is(value, "auto")) {
        *region = COGMASTER_REGION_AUTO;
        *tv = COGMASTER_TV_AUTO;
    } else if (is(value, "ntsc-u")) {
        *region = COGMASTER_REGION_EXPORT;
        *tv = COGMASTER_TV_NTSC;
    } else if (is(value, "ntsc-j")) {
        *region = COGMASTER_REGION_JAPAN;
        *tv = COGMASTER_TV_NTSC;
    } else if (is(value, "pal")) {
        *region = COGMASTER_REGION_EXPORT;
        *tv = COGMASTER_TV_PAL;
    } else {
        return 0;
    }
    return 1;
}

/* Sets the file once, from a positional argument or --rom. */
static int set_file(cogmaster__run_options *o, const char *path)
{
    if (o->file != NULL) {
        return usage("give one file");
    }
    if (path == NULL || *path == '\0') {
        return usage("--rom needs a file name");
    }
    o->file = path;
    return COGMASTER__EXIT_OK;
}

/* Checks the combination once every argument is read. */
static int check_combination(const cogmaster__run_options *o)
{
    if (o->probe) {
        if (o->file == NULL) {
            return usage("--probe needs a file");
        }
        if (o->frames != 0u || o->hash || o->png_count != 0u || o->trace || o->mailbox ||
            o->input) {
            return usage("--probe runs nothing, so it takes no run option");
        }
        return COGMASTER__EXIT_OK;
    }
    if (o->frames == 0u) {
        return usage("--frames N or --bench N is required");
    }
    for (uint32_t k = 0; k < o->png_count; k++) {
        if (o->pngs[k].frame > o->frames) {
            return usage("--png N must not exceed --frames");
        }
    }
    return COGMASTER__EXIT_OK;
}

int cogmaster__parse_options(int argc, char **argv, cogmaster__run_options *o)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        help();
        o->printed = 1;
        return COGMASTER__EXIT_OK;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        version();
        o->printed = 1;
        return COGMASTER__EXIT_OK;
    }
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        const char *value = i + 1 < argc ? argv[i + 1] : NULL;
        int status = COGMASTER__EXIT_OK;
        if (strcmp(arg, "--frames") == 0 || strcmp(arg, "--bench") == 0) {
            if (o->frames != 0u) {
                return usage("give --frames or --bench once");
            }
            o->bench = strcmp(arg, "--bench") == 0;
            if (!parse_count(value, &o->frames)) {
                return usage("--frames and --bench need a whole number of 1 or more");
            }
            if (o->bench && cogmaster__bench_bytes(o->frames) == 0u) {
                return usage("--bench is at most 1073741823");
            }
            i++;
        } else if (strcmp(arg, "--input") == 0) {
            if (value == NULL || *value == '\0' || o->input != NULL) {
                return usage("--input needs one file");
            }
            o->input = value;
            i++;
        } else if (strcmp(arg, "--port1") == 0 || strcmp(arg, "--port2") == 0) {
            if (!is(value, "pad") && !is(value, "none")) {
                return usage("--port1 and --port2 need pad or none");
            }
            uint32_t device = is(value, "none") ? COGMASTER_DEVICE_NONE : COGMASTER_DEVICE_PAD;
            if (strcmp(arg, "--port1") == 0) {
                o->port1 = device;
            } else {
                o->port2 = device;
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
            if (!parse_machine(value, &o->machine)) {
                return usage("--machine needs auto, sms or gg");
            }
            i++;
        } else if (strcmp(arg, "--region") == 0) {
            if (!parse_region(value, &o->region, &o->tv)) {
                return usage("--region needs auto, ntsc-u, ntsc-j or pal");
            }
            i++;
        } else if (strcmp(arg, "--trace") == 0) {
            if (!is(value, "first-fault")) {
                return usage("--trace needs first-fault");
            }
            o->trace = 1;
            i++;
        } else if (strcmp(arg, "--mailbox") == 0) {
            o->mailbox = 1;
        } else if (strcmp(arg, "--probe") == 0) {
            o->probe = 1;
        } else if (strcmp(arg, "--rom") == 0) {
            status = set_file(o, value);
            i++;
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "--version") == 0) {
            return usage("--help and --version take no other arguments");
        } else if (arg[0] == '-') {
            return usage("unknown option");
        } else {
            status = set_file(o, arg);
        }
        if (status != COGMASTER__EXIT_OK) {
            return status;
        }
    }
    return check_combination(o);
}
