/* Loads the built libretro module at run time and calls it in the order a
 * frontend such as RetroArch does.
 *
 *   cogmaster-test-libretro-host <module> xrgb8888|rgb565 <sha256> W H AW AH
 *
 * <sha256> is the pinned card hash the runner's test also uses; W x H is the
 * expected crop and AW/AH the expected display aspect. The host turns the
 * 60th delivered frame back into RGB444, rows top to bottom, each dot low
 * byte first, and hashes it as cogmaster-run --hash does, so equal digests
 * mean the delivered frame equals the runner's byte for byte. Pixels are
 * read as native-endian integers, so a swapped channel order, 555 for 565 or
 * a swapped byte order fails here. In rgb565 mode the host registers no
 * batch audio callback, so the single-sample path runs. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../check.h"
#include "cogmaster.h"
#include "libretro.h"
#include "palette.h"
#include "sha256.h"
#include "../fixtures/play.h"

#define MAX_W 256u
#define MAX_H 240u
#define FRAMES 60u
/* 4,375 frames at 1,640,625/27,379 Hz last exactly 3,504,512 samples at
   48,000 Hz (.planning/preparation/ARCHITECTURE.md section 5). */
#define AUDIO_FRAMES 4375u
#define AUDIO_SAMPLES 3504512u
#define POISON 0xA5u

/* a == b as doubles, printed with every digit. */
#define CHECK_EQ_DBL(a, b)                                                                         \
    do {                                                                                           \
        double check_a_ = (double)(a);                                                             \
        double check_b_ = (double)(b);                                                             \
        if (check_a_ != check_b_) {                                                                \
            fprintf(stderr, "%s:%d: %s == %s failed: %.17g != %.17g\n", __FILE__, __LINE__, #a,    \
                    #b, check_a_, check_b_);                                                       \
            check_failures++;                                                                      \
        }                                                                                          \
    } while (0)

/* ---- module loading ---- */

#ifdef _WIN32
static HMODULE module;
static int module_open(const char *path)
{
    module = LoadLibraryA(path);
    return module != NULL;
}
static void *module_sym(const char *name)
{
    FARPROC f = GetProcAddress(module, name);
    void *p = NULL;
    memcpy(&p, &f, sizeof p);
    return p;
}
static void module_close(void)
{
    FreeLibrary(module);
}
#else
static void *module;
static int module_open(const char *path)
{
    module = dlopen(path, RTLD_NOW);
    if (module == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
    }
    return module != NULL;
}
static void *module_sym(const char *name)
{
    return dlsym(module, name);
}
static void module_close(void)
{
    (void)dlclose(module);
}
#endif

/* The 25 entry points, typed as libretro.h declares them (L7511-7840). */
static void(RETRO_CALLCONV *p_set_environment)(retro_environment_t);
static void(RETRO_CALLCONV *p_set_video_refresh)(retro_video_refresh_t);
static void(RETRO_CALLCONV *p_set_audio_sample)(retro_audio_sample_t);
static void(RETRO_CALLCONV *p_set_audio_sample_batch)(retro_audio_sample_batch_t);
static void(RETRO_CALLCONV *p_set_input_poll)(retro_input_poll_t);
static void(RETRO_CALLCONV *p_set_input_state)(retro_input_state_t);
static void(RETRO_CALLCONV *p_init)(void);
static void(RETRO_CALLCONV *p_deinit)(void);
static unsigned(RETRO_CALLCONV *p_api_version)(void);
static void(RETRO_CALLCONV *p_get_system_info)(struct retro_system_info *);
static void(RETRO_CALLCONV *p_get_system_av_info)(struct retro_system_av_info *);
static void(RETRO_CALLCONV *p_set_controller_port_device)(unsigned, unsigned);
static void(RETRO_CALLCONV *p_reset)(void);
static void(RETRO_CALLCONV *p_run)(void);
static size_t(RETRO_CALLCONV *p_serialize_size)(void);
static bool(RETRO_CALLCONV *p_serialize)(void *, size_t);
static bool(RETRO_CALLCONV *p_unserialize)(const void *, size_t);
static void(RETRO_CALLCONV *p_cheat_reset)(void);
static void(RETRO_CALLCONV *p_cheat_set)(unsigned, bool, const char *);
static bool(RETRO_CALLCONV *p_load_game)(const struct retro_game_info *);
static bool(RETRO_CALLCONV *p_load_game_special)(unsigned, const struct retro_game_info *, size_t);
static void(RETRO_CALLCONV *p_unload_game)(void);
static unsigned(RETRO_CALLCONV *p_get_region)(void);
static void *(RETRO_CALLCONV *p_get_memory_data)(unsigned);
static size_t(RETRO_CALLCONV *p_get_memory_size)(unsigned);

static const char *missing;

/* ISO C has no conversion from void * to a function pointer, so the address
   is copied byte for byte. The first missing name is kept. */
#define RESOLVE(var, name)                                                                         \
    do {                                                                                           \
        void *p_ = module_sym(name);                                                               \
        if (p_ == NULL) {                                                                          \
            if (missing == NULL) {                                                                 \
                missing = name;                                                                    \
            }                                                                                      \
        } else {                                                                                   \
            memcpy(&(var), &p_, sizeof(var));                                                      \
        }                                                                                          \
    } while (0)

static void resolve_all(void)
{
    RESOLVE(p_set_environment, "retro_set_environment");
    RESOLVE(p_set_video_refresh, "retro_set_video_refresh");
    RESOLVE(p_set_audio_sample, "retro_set_audio_sample");
    RESOLVE(p_set_audio_sample_batch, "retro_set_audio_sample_batch");
    RESOLVE(p_set_input_poll, "retro_set_input_poll");
    RESOLVE(p_set_input_state, "retro_set_input_state");
    RESOLVE(p_init, "retro_init");
    RESOLVE(p_deinit, "retro_deinit");
    RESOLVE(p_api_version, "retro_api_version");
    RESOLVE(p_get_system_info, "retro_get_system_info");
    RESOLVE(p_get_system_av_info, "retro_get_system_av_info");
    RESOLVE(p_set_controller_port_device, "retro_set_controller_port_device");
    RESOLVE(p_reset, "retro_reset");
    RESOLVE(p_run, "retro_run");
    RESOLVE(p_serialize_size, "retro_serialize_size");
    RESOLVE(p_serialize, "retro_serialize");
    RESOLVE(p_unserialize, "retro_unserialize");
    RESOLVE(p_cheat_reset, "retro_cheat_reset");
    RESOLVE(p_cheat_set, "retro_cheat_set");
    RESOLVE(p_load_game, "retro_load_game");
    RESOLVE(p_load_game_special, "retro_load_game_special");
    RESOLVE(p_unload_game, "retro_unload_game");
    RESOLVE(p_get_region, "retro_get_region");
    RESOLVE(p_get_memory_data, "retro_get_memory_data");
    RESOLVE(p_get_memory_size, "retro_get_memory_size");
}

/* ---- frontend callbacks: record what the core does ---- */

static int rgb565; /* the mode: 0 xrgb8888, 1 rgb565 */
static unsigned want_w, want_h;
static int no_game_calls, no_game_value;
static int pixel_format_calls;
static enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned video_calls, video_bad;
static unsigned char frame[MAX_W * MAX_H * 4u];
static size_t sample_frames;
static unsigned sample_nonzero;
static unsigned batch_calls;
static size_t batch_frames;
static unsigned batch_nonzero;
static unsigned content_frame, poll_calls;
static int content_mode;
static unsigned descriptors_calls, controllers_calls;
static unsigned options_calls;

static bool RETRO_CALLCONV env(unsigned cmd, void *data)
{
    if (cmd == RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION) {
        *(unsigned *)data = 2u;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) {
        const struct retro_core_options_v2 *options = data;
        CHECK(options->definitions != NULL);
        options_calls++;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_CONTROLLER_INFO) {
        const struct retro_controller_info *ports = data;
        controllers_calls++;
        CHECK_EQ_U64(ports[0].num_types, 2u);
        CHECK_EQ_U64(ports[1].num_types, 2u);
        CHECK_EQ_U64(ports[0].types[0].id, RETRO_DEVICE_JOYPAD);
        CHECK_EQ_U64(ports[0].types[1].id, RETRO_DEVICE_NONE);
        CHECK_EQ_U64(ports[2].num_types, 0u);
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS) {
        const struct retro_input_descriptor *desc = data;
        unsigned n = 0u;
        descriptors_calls++;
        while (n < 32u && desc[n].description != NULL) {
            CHECK(desc[n].port < 2u);
            CHECK_EQ_U64(desc[n].device, RETRO_DEVICE_JOYPAD);
            n++;
        }
        CHECK_EQ_U64(n, 14u);
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME) {
        no_game_calls++;
        no_game_value = *(const bool *)data ? 1 : 0;
        return true;
    }
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT) {
        pixel_format_calls++;
        pixel_format = *(const enum retro_pixel_format *)data;
        return true;
    }
    return false;
}

/* Keeps the rows of the last frame, packed, in the module's own byte order.
   A frame of the wrong size or pitch is counted and not kept. */
static void RETRO_CALLCONV video(const void *data, unsigned width, unsigned height, size_t pitch)
{
    size_t row = (size_t)want_w * (rgb565 ? 2u : 4u);
    unsigned y;
    video_calls++;
    if (data == NULL || width != want_w || height != want_h || pitch != row) {
        fprintf(stderr, "video: %ux%u pitch %lu, expected %ux%u pitch %lu%s\n", width, height,
                (unsigned long)pitch, want_w, want_h, (unsigned long)row,
                data == NULL ? " (NULL data)" : "");
        video_bad++;
        return;
    }
    for (y = 0; y < height; y++) {
        memcpy(&frame[y * row], (const unsigned char *)data + y * pitch, row);
    }
}

static void RETRO_CALLCONV sample(int16_t left, int16_t right)
{
    sample_frames++;
    if (left != 0 || right != 0) {
        sample_nonzero++;
    }
}

static size_t RETRO_CALLCONV batch(const int16_t *data, size_t frames)
{
    size_t i;
    batch_calls++;
    batch_frames += frames;
    for (i = 0; i < frames * 2u; i++) {
        if (data[i] != 0) {
            batch_nonzero++;
        }
    }
    return frames;
}

static void RETRO_CALLCONV on_poll(void)
{
    poll_calls++;
}

static int16_t RETRO_CALLCONV input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (content_mode && port == 0u && device == RETRO_DEVICE_JOYPAD && index == 0u &&
        id == RETRO_DEVICE_ID_JOYPAD_B) {
        CHECK_EQ_U64(poll_calls, content_frame);
        return content_frame == 5u || content_frame == 6u || content_frame == 8u ? 1 : 0;
    }
    return 0;
}

/* got must equal want; prints both. */
static void check_string(const char *what, const char *got, const char *want)
{
    if (got == NULL || strcmp(got, want) != 0) {
        fprintf(stderr, "%s: got \"%s\", expected \"%s\"\n", what, got != NULL ? got : "(null)",
                want);
        check_failures++;
    }
}

/* ---- the delivered frame as the runner's crop ---- */

/* Rebuilds the RGB444 dot at index i of the captured frame. The conversions
   repeat each channel's bits, so the top bits are the original channel; the
   dot is converted forward again and must give the delivered pixel back,
   which shows the conversion lost nothing. Returns 0 on a pixel no RGB444
   dot produces. */
static int to_rgb444(uint32_t i, uint16_t *out)
{
    if (rgb565) {
        uint16_t px;
        memcpy(&px, &frame[2u * i], sizeof px);
        uint32_t r = (uint32_t)(px >> 11) >> 1;
        uint32_t g = (uint32_t)((px >> 5) & 63u) >> 2;
        uint32_t b = (uint32_t)(px & 31u) >> 1;
        *out = (uint16_t)(b << 8 | g << 4 | r);
        return cogmaster__rgb444_to_rgb565(*out) == px;
    }
    uint32_t px;
    memcpy(&px, &frame[4u * i], sizeof px);
    uint32_t r = ((px >> 16) & 0xFFu) >> 4;
    uint32_t g = ((px >> 8) & 0xFFu) >> 4;
    uint32_t b = (px & 0xFFu) >> 4;
    *out = (uint16_t)(b << 8 | g << 4 | r);
    return cogmaster__rgb444_to_rgb888(*out) == (px & 0xFFFFFFu);
}

/* Hashes the captured frame as cogmaster-run --hash hashes the crop. */
static void hash_frame(char hex[65])
{
    cogmaster__sha256 s;
    uint8_t digest[32];
    uint8_t dot[2];
    uint32_t i, lossy = 0;
    cogmaster__sha256_init(&s);
    for (i = 0; i < want_w * want_h; i++) {
        uint16_t c = 0;
        if (!to_rgb444(i, &c)) {
            if (lossy == 0) {
                fprintf(stderr, "first pixel no RGB444 dot gives: (%u,%u)\n", i % want_w,
                        i / want_w);
            }
            lossy++;
        }
        dot[0] = (uint8_t)(c & 0xFFu);
        dot[1] = (uint8_t)(c >> 8);
        cogmaster__sha256_update(&s, dot, sizeof dot);
    }
    CHECK_EQ_U64(lossy, 0);
    cogmaster__sha256_final(&s, digest);
    cogmaster__sha256_hex(digest, hex);
}

/* Compares the captured frame with an earlier one; prints the first
   differing pixel. */
static void check_same(const unsigned char *first, unsigned run)
{
    size_t bpp = rgb565 ? 2u : 4u;
    size_t i, n = (size_t)want_w * want_h * bpp;
    for (i = 0; i < n; i++) {
        if (frame[i] != first[i]) {
            size_t px = i / bpp;
            fprintf(stderr, "frame %u differs from frame 1 at (%lu,%lu)\n", run,
                    (unsigned long)(px % want_w), (unsigned long)(px / want_w));
            check_failures++;
            return;
        }
    }
}

/* Runs one frame into a poisoned copy; returns the stereo frames of audio
   it delivered. */
static size_t run_one(void)
{
    size_t before = batch_frames + sample_frames;
    memset(frame, POISON, sizeof frame);
    p_run();
    return batch_frames + sample_frames - before;
}

/* Sets the frontend callbacks, as a frontend does after retro_init. */
static void set_callbacks(void)
{
    p_set_video_refresh(video);
    p_set_audio_sample(sample);
    p_set_audio_sample_batch(rgb565 ? NULL : batch);
    p_set_input_poll(on_poll);
    p_set_input_state(input);
}

static unsigned parse(const char *s)
{
    return (unsigned)strtoul(s, NULL, 10);
}

/* The same production module, owned ROM, script and independent pixels as
   the runner. All scripted levels belong to this frontend. */
static int content_test(const char *path, const char *digest)
{
    uint8_t image[32768];
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return 1;
    }
    size_t n = fread(image, 1u, sizeof image, f);
    CHECK_EQ_U64(n, sizeof image);
    CHECK_EQ_U64(fclose(f), 0u);
    want_w = 256u;
    want_h = 192u;
    content_mode = 1;
    p_set_environment(env);
    p_init();
    set_callbacks();
    struct retro_game_info game = {0};
    game.data = image;
    game.size = n;
    bool loaded = p_load_game(&game);
    CHECK(loaded);
    if (!loaded) {
        p_deinit();
        return 1;
    }
    CHECK_EQ_U64(descriptors_calls, 1u);
    CHECK_EQ_U64(controllers_calls, 1u);
    for (content_frame = 1u; content_frame <= 14u; content_frame++) {
        unsigned before = video_calls;
        (void)run_one();
        CHECK_EQ_U64(video_calls, before + 1u);
        CHECK_EQ_U64(poll_calls, content_frame);
        if (content_frame >= 4u) {
            uint32_t scroll = content_frame >= 8u ? 2u : (content_frame >= 5u ? 1u : 0u);
            for (uint32_t y = 0u; y < 192u; y++) {
                for (uint32_t x = 0u; x < 256u; x++) {
                    uint16_t got;
                    if (!to_rgb444(y * 256u + x, &got) || got != play_pixel(x, y, scroll)) {
                        CHECK_EQ_HEX(got, play_pixel(x, y, scroll));
                        p_deinit();
                        return 1;
                    }
                }
            }
        }
    }
    char hex[65];
    hash_frame(hex);
    check_string("content runner digest", hex, digest);
    CHECK_EQ_U64(video_bad, 0u);
    /* A disconnected device and absent state callback both submit released
       levels, even while the frontend's scripted button would be held. */
    for (unsigned mode = 0u; mode < 3u; mode++) {
        CHECK(p_load_game(&game));
        poll_calls = 0u;
        p_set_input_poll(on_poll);
        p_set_input_state(input);
        p_set_controller_port_device(0u, mode == 0u ? RETRO_DEVICE_NONE : RETRO_DEVICE_JOYPAD);
        if (mode == 1u) {
            p_set_input_state(NULL);
        }
        if (mode == 2u) {
            p_set_input_poll(NULL);
        }
        for (content_frame = 1u; content_frame <= 14u; content_frame++) {
            (void)run_one();
        }
        for (uint32_t y = 0u; y < 192u; y++) {
            for (uint32_t x = 0u; x < 256u; x++) {
                uint16_t got;
                CHECK(to_rgb444(y * 256u + x, &got));
                if (got != play_pixel(x, y, 0u)) {
                    CHECK_EQ_HEX(got, play_pixel(x, y, 0u));
                    p_deinit();
                    return 1;
                }
            }
        }
    }
    p_unload_game();
    p_deinit();
    return check_failures != 0 ? 1 : 0;
}

int main(int argc, char **argv)
{
    static unsigned char first[MAX_W * MAX_H * 4u];
    struct retro_system_info sys;
    struct retro_system_av_info av;
    struct retro_game_info game;
    static const unsigned char one_byte[1] = {0};
    char version[32];
    char hex[65];
    cogmaster_versions v;
    unsigned run, aspect_w, aspect_h, bad_counts = 0;
    size_t total = 0;

    if (argc == 5 && strcmp(argv[2], "content") == 0) {
        if (!module_open(argv[1])) {
            return 1;
        }
        resolve_all();
        if (missing != NULL) {
            module_close();
            return 1;
        }
        int rc = content_test(argv[3], argv[4]);
        module_close();
        return rc;
    }
    if (argc != 8 || (strcmp(argv[2], "xrgb8888") != 0 && strcmp(argv[2], "rgb565") != 0) ||
        strlen(argv[3]) != 64u) {
        fprintf(stderr, "usage: cogmaster-test-libretro-host <module> xrgb8888|rgb565 <sha256> "
                        "W H AW AH\n");
        return 2;
    }
    rgb565 = strcmp(argv[2], "rgb565") == 0;
    want_w = parse(argv[4]);
    want_h = parse(argv[5]);
    aspect_w = parse(argv[6]);
    aspect_h = parse(argv[7]);
    if (want_w == 0u || want_w > MAX_W || want_h == 0u || want_h > MAX_H || aspect_w == 0u ||
        aspect_h == 0u) {
        fprintf(stderr, "cogmaster-test-libretro-host: bad geometry\n");
        return 2;
    }
    if (!module_open(argv[1])) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    /* Every entry point, before anything else is called. */
    resolve_all();
    if (missing != NULL) {
        fprintf(stderr, "missing entry point %s\n", missing);
        module_close();
        return 1;
    }

    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(cogmaster_get_versions(&v), COGMASTER_STATUS_OK);
    snprintf(version, sizeof version, "%u.%u.%u", (unsigned)v.library_major,
             (unsigned)v.library_minor, (unsigned)v.library_patch);

    CHECK_EQ_U64(p_api_version(), RETRO_API_VERSION);
    p_set_environment(env);
    CHECK_EQ_U64(no_game_calls, 1);
    CHECK_EQ_U64(options_calls, 1u);
    CHECK_EQ_U64(no_game_value, 1);
    p_init();

    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    check_string("library_name", sys.library_name, "cogmaster");
    check_string("library_version", sys.library_version, version);
    check_string("valid_extensions", sys.valid_extensions, "sms|gg|bin|rom");
    CHECK_EQ_U64(sys.need_fullpath, 0);
    set_callbacks();

    /* Content is refused; only the test card runs in this release. */
    memset(&game, 0, sizeof game);
    game.data = one_byte;
    game.size = sizeof one_byte;
    CHECK_EQ_U64(p_load_game(&game), 0);
    CHECK_EQ_U64(p_load_game_special(0, &game, 1), 0);
    CHECK_EQ_U64(pixel_format_calls, 0);

    /* No content: the pixel format is set before load returns (L861-865). */
    CHECK_EQ_U64(p_load_game(NULL), 1);
    CHECK_EQ_U64(pixel_format_calls, 1);
    CHECK_EQ_U64(pixel_format, rgb565 ? RETRO_PIXEL_FORMAT_RGB565 : RETRO_PIXEL_FORMAT_XRGB8888);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);

    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    CHECK_EQ_U64(av.geometry.base_width, want_w);
    CHECK_EQ_U64(av.geometry.base_height, want_h);
    CHECK_EQ_U64(av.geometry.max_width, MAX_W);
    CHECK_EQ_U64(av.geometry.max_height, MAX_H);
    CHECK_EQ_DBL(av.geometry.aspect_ratio, (float)aspect_w / (float)aspect_h);
    CHECK_EQ_DBL(av.timing.fps, 1640625.0 / 27379.0);
    CHECK_EQ_DBL(av.timing.sample_rate, 48000.0);

    /* No save states and no memory regions in this release. */
    CHECK_EQ_U64(p_serialize_size(), 0);
    CHECK_EQ_U64(p_serialize(first, sizeof first), 0);
    CHECK_EQ_U64(p_unserialize(first, sizeof first), 0);
    CHECK_EQ_U64(p_unserialize(NULL, 0u), 0);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 0);
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    p_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    p_cheat_reset();
    p_cheat_set(0, true, "");

    /* 60 frames, one video call each, every one equal to the first although
       the copy is poisoned before each; frame 60 hashes to the pin. Audio is
       counted from the first frame on. */
    for (run = 1; run <= AUDIO_FRAMES; run++) {
        size_t n = run_one();
        total += n;
        if (n != 801u && n != 802u) {
            if (bad_counts == 0) {
                fprintf(stderr, "frame %u delivered %lu stereo frames, expected 801 or 802\n", run,
                        (unsigned long)n);
            }
            bad_counts++;
        }
        if (run == 1u) {
            memcpy(first, frame, sizeof first);
        } else if (run <= FRAMES) {
            check_same(first, run);
        }
        if (run == FRAMES) {
            CHECK_EQ_U64(video_calls, FRAMES);
            hash_frame(hex);
            if (strcmp(hex, argv[3]) != 0) {
                fprintf(stderr, "frame 60 sha256 %s, expected %s: the digest differs\n", hex,
                        argv[3]);
                check_failures++;
            }
        }
    }
    CHECK_EQ_U64(video_calls, AUDIO_FRAMES);
    CHECK_EQ_U64(video_bad, 0);
    CHECK_EQ_U64(bad_counts, 0);
    CHECK_EQ_U64(total, AUDIO_SAMPLES);
    CHECK_EQ_U64(batch_calls, rgb565 ? 0u : AUDIO_FRAMES);
    CHECK_EQ_U64(rgb565 ? sample_frames : batch_frames, AUDIO_SAMPLES);
    CHECK_EQ_U64(batch_nonzero, 0);
    CHECK_EQ_U64(sample_nonzero, 0);

    /* A power cycle shows the same card. */
    p_reset();
    (void)run_one();
    check_same(first, AUDIO_FRAMES + 1u);

    /* Unload, deinit and start again: no state survives, so the first frame
       is the card and its audio starts from a zero remainder (801). */
    p_unload_game();
    p_deinit();
    no_game_calls = 0;
    pixel_format_calls = 0;
    p_set_environment(env);
    p_init();
    set_callbacks();
    CHECK_EQ_U64(no_game_calls, 1);
    CHECK_EQ_U64(p_load_game(NULL), 1);
    CHECK_EQ_U64(pixel_format_calls, 1);
    video_calls = 0;
    CHECK_EQ_U64(run_one(), 801u);
    CHECK_EQ_U64(video_calls, 1);
    check_same(first, 1u);

    p_unload_game();
    p_deinit();
    module_close();
    CHECK_DONE();
}
