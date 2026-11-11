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
    "  --machine NAME       auto (default), sms, sms1 or gg\n"
    "  --video-mode NAME    auto (default), sms or gg\n"
    "  --region NAME        auto (default), ntsc-u, ntsc-j or pal\n"
    "  --frames N           run N frames (N >= 1)\n"
    "  --bench N            run N frames and report the time per frame\n"
    "  --hash               print the SHA-256 of the last frame's crop\n"
    "  --hash-audio         print the SHA-256 of the sound's level changes and of its samples\n"
    "  --expect FILE        judge bounded accepted frame/RAM/mailbox/boot predicates\n"
    "  --input FILE         absolute pad and console record per frame\n"
    "  --save-dir DIR       read and persist battery data in an existing directory\n"
    "  --save-interval N    also write the save after every N frames (0 is off; needs --save-dir)\n"
    "  --port1 NAME         first controller: auto (default), pad, none, paddle or sports\n"
    "  --port2 NAME         second controller: auto (default), pad, none, paddle or sports\n"
    "  --png N:FILE         write frame N's crop to FILE as a PNG (1 <= N <= --frames)\n"
    "  --trace first-fault  stop at the first fault and print it, or 'fault none'\n"
    "  --eyes               after the last frame, print how often each 3-D glasses eye showed\n"
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

/* Parses text as a decimal number from 0 to 4294967295. Returns 0 on any
   other text. */
static int parse_number(const char *text, uint32_t *out)
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
    *out = (uint32_t)v;
    return 1;
}

/* The same from 1 to 4294967295. */
static int parse_count(const char *text, uint32_t *out)
{
    return parse_number(text, out) && *out != 0u;
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

static const struct {
    const char *name;
    uint32_t device;
} devices[] = {
    {"pad", COGMASTER_DEVICE_PAD},
    {"none", COGMASTER_DEVICE_NONE},
    {"paddle", COGMASTER_DEVICE_PADDLE},
    {"sports", COGMASTER_DEVICE_SPORTS_PAD},
};

const char *cogmaster__device_name(uint32_t device)
{
    for (size_t i = 0; i < sizeof devices / sizeof devices[0]; i++) {
        if (devices[i].device == device) {
            return devices[i].name;
        }
    }
    return "pad";
}

/* --port1 and --port2 NAME: auto leaves the choice to the library's default for the
   loaded content. Returns 0 for an unknown name. */
static int parse_device(const char *value, uint32_t *device, int *given)
{
    *given = 1;
    for (size_t i = 0; i < sizeof devices / sizeof devices[0]; i++) {
        if (is(value, devices[i].name)) {
            *device = devices[i].device;
            return 1;
        }
    }
    *given = 0;
    return is(value, "auto");
}

/* --machine NAME. Returns 0 for an unknown name. */
static int parse_machine(const char *value, uint32_t *machine)
{
    if (is(value, "auto")) {
        *machine = COGMASTER_MACHINE_AUTO;
    } else if (is(value, "sms")) {
        *machine = COGMASTER_MACHINE_SMS2;
    } else if (is(value, "sms1")) {
        *machine = COGMASTER_MACHINE_SMS1;
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
        if (o->frames != 0u || o->hash || o->hash_audio || o->png_count != 0u || o->trace ||
            o->mailbox || o->eyes || o->input || o->save_dir || o->expect ||
            o->save_interval != 0u) {
            return usage("--probe runs nothing, so it takes no run option");
        }
        return COGMASTER__EXIT_OK;
    }
    if (o->frames == 0u) {
        return usage("--frames N or --bench N is required");
    }
    if (o->save_dir != NULL && o->file == NULL)
        return usage("--save-dir requires content");
    if (o->save_interval != 0u && o->save_dir == NULL)
        return usage("--save-interval requires --save-dir");
    if (o->hash_audio && o->bench)
        return usage("--hash-audio needs --frames, not --bench");
    if (o->expect && (o->trace || o->mailbox || o->bench))
        return usage("--expect requires the complete --frames run");
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
        } else if (strcmp(arg, "--save-dir") == 0) {
            if (value == NULL || *value == '\0' || o->save_dir != NULL)
                return usage("--save-dir needs one directory");
            o->save_dir = value;
            i++;
        } else if (strcmp(arg, "--save-interval") == 0) {
            if (!parse_number(value, &o->save_interval))
                return usage("--save-interval needs a whole number of frames, 0 or more");
            i++;
        } else if (strcmp(arg, "--expect") == 0) {
            if (value == NULL || *value == '\0' || o->expect != NULL)
                return usage("--expect needs one file");
            o->expect = value;
            i++;
        } else if (strcmp(arg, "--input") == 0) {
            if (value == NULL || *value == '\0' || o->input != NULL) {
                return usage("--input needs one file");
            }
            o->input = value;
            i++;
        } else if (strcmp(arg, "--port1") == 0 || strcmp(arg, "--port2") == 0) {
            int first = strcmp(arg, "--port1") == 0;
            if (!parse_device(value, first ? &o->port1 : &o->port2,
                              first ? &o->port1_given : &o->port2_given)) {
                return usage("--port1 and --port2 need auto, pad, none, paddle or sports");
            }
            i++;
        } else if (strcmp(arg, "--hash") == 0) {
            o->hash = 1;
        } else if (strcmp(arg, "--hash-audio") == 0) {
            o->hash_audio = 1;
        } else if (strcmp(arg, "--png") == 0) {
            if (!parse_png(value, &o->pngs[o->png_count])) {
                return usage("--png needs N:FILE with N >= 1");
            }
            o->png_count++;
            i++;
        } else if (strcmp(arg, "--machine") == 0) {
            if (!parse_machine(value, &o->machine)) {
                return usage("--machine needs auto, sms, sms1 or gg");
            }
            i++;
        } else if (strcmp(arg, "--video-mode") == 0) {
            if (!is(value, "auto") && !is(value, "sms") && !is(value, "gg"))
                return usage("--video-mode needs auto, sms or gg");
            o->video_mode = is(value, "gg")    ? COGMASTER_VIDEO_GG
                            : is(value, "sms") ? COGMASTER_VIDEO_SMS
                                               : COGMASTER_VIDEO_AUTO;
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
        } else if (strcmp(arg, "--eyes") == 0) {
            o->eyes = 1;
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
