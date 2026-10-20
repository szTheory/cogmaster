/* The libretro adapter: the 25 entry points of libretro.h, written as a thin
 * client of cogmaster.h. "Lnnn" is a line of the vendored libretro.h
 * (RetroArch v1.22.2, see PROVENANCE.md).
 *
 * This release loads no content: retro_load_game(NULL) loads the built-in
 * test card and any content is refused. Audio is silence of the exact
 * per-frame length, so a frontend that paces on audio runs at the core's
 * rate. The av_info division is the only floating point in the shipped
 * code; the core itself is integer only. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "libretro.h"
#include "palette.h"

/* Fixed until the sound phase gives the core a configured output rate. */
#define LR_SAMPLE_RATE 48000u
/* Stereo frames per video frame are at most ceil(48000 / 59.92) = 802. */
#define LR_AUDIO_CAPACITY 802u

#ifdef COGMASTER_LIBRETRO_RGB565
typedef uint16_t lr_pixel;
#define LR_PIXEL_FORMAT RETRO_PIXEL_FORMAT_RGB565
#define LR_CONVERT cogmaster__rgb444_to_rgb565
#else
typedef uint32_t lr_pixel;
#define LR_PIXEL_FORMAT RETRO_PIXEL_FORMAT_XRGB8888
#define LR_CONVERT cogmaster__rgb444_to_rgb888
#endif

/* The machine a no-content load shows: the Master System card, or the Game
   Gear card in a test-only build. */
#ifdef COGMASTER_LIBRETRO_TEST_GG
#define LR_NO_CONTENT_MACHINE COGMASTER_MACHINE_GG
#else
#define LR_NO_CONTENT_MACHINE COGMASTER_MACHINE_SMS2
#endif

/* All adapter state. A frontend may keep the module loaded between sessions,
   so retro_deinit returns every one of these to its initial value (L7607),
   and retro_load_game resets the per-game ones. */
static retro_environment_t env_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static cogmaster *inst;
static int loaded;
static cogmaster_info info;
static uint16_t *native;
static lr_pixel *pixels;
static uint64_t audio_acc;
static int16_t silence[LR_AUDIO_CAPACITY * 2u];

/* The allocator the adapter hands the core: the C library's malloc and free. */
static void *lr_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void lr_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* Frees the frame buffers and forgets the loaded game. */
static void lr_unload(void)
{
    free(native);
    native = NULL;
    free(pixels);
    pixels = NULL;
    loaded = 0;
    memset(&info, 0, sizeof info);
    audio_acc = 0u;
}

/* L7511: called before retro_init. Declaring no-game support (L1051) makes
   the frontend call retro_load_game(NULL). */
void retro_set_environment(retro_environment_t cb)
{
    bool no_game = true;
    env_cb = cb;
    if (cb != NULL) {
        (void)cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
    }
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

/* L7448-7465: the frontend may register either audio callback; retro_run
   prefers the batch callback when both are registered. */
void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

/* L7582: the one instance lives from here to retro_deinit. A failed create
   leaves inst NULL and retro_load_game then refuses. */
void retro_init(void)
{
    cogmaster_config cfg;
    if (inst != NULL) {
        return;
    }
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = lr_alloc;
    cfg.allocator.free = lr_free;
    if (cogmaster_create(&cfg, &inst) != COGMASTER_STATUS_OK) {
        inst = NULL;
    }
}

/* L7607: reset every static; the module may be initialised again. */
void retro_deinit(void)
{
    lr_unload();
    cogmaster_destroy(inst);
    inst = NULL;
    env_cb = NULL;
    video_cb = NULL;
    audio_cb = NULL;
    audio_batch_cb = NULL;
    input_poll_cb = NULL;
    input_state_cb = NULL;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

/* L7627: static strings. No content loads yet, so no extension is listed. */
void retro_get_system_info(struct retro_system_info *sys)
{
    if (sys == NULL) {
        return;
    }
    memset(sys, 0, sizeof *sys);
    sys->library_name = "cogmaster";
    sys->library_version = COGMASTER_LIBRETRO_VERSION;
    sys->valid_extensions = "";
    sys->need_fullpath = false;
    sys->block_extract = false;
}

/* L7642: geometry and timing, all from cogmaster_get_info of the loaded
   machine. The base size is the crop, the maximum the largest field. The
   display aspect is the crop's width times the pixel aspect ratio over its
   height: 256 x 8/7 over 192 = 32/21 for the Master System. */
void retro_get_system_av_info(struct retro_system_av_info *av)
{
    if (av == NULL) {
        return;
    }
    memset(av, 0, sizeof *av);
    if (!loaded) {
        return;
    }
    av->geometry.base_width = info.crop_w;
    av->geometry.base_height = info.crop_h;
    av->geometry.max_width = info.max_w;
    av->geometry.max_height = info.max_h;
    av->geometry.aspect_ratio = (float)((double)info.crop_w * (double)info.par_num /
                                        ((double)info.crop_h * (double)info.par_den));
    av->timing.fps = (double)info.fps_num / (double)info.fps_den;
    av->timing.sample_rate = (double)LR_SAMPLE_RATE;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;
}

/* L7680: a power cycle; the test card has no other reset. */
void retro_reset(void)
{
    if (loaded) {
        (void)cogmaster_reset(inst, COGMASTER_RESET_POWER);
    }
}

/* Sends this frame's silence. Each frame is 48000 x fps_den / fps_num stereo
   frames, carried as a remainder so the total never drifts: over 4,375 frames
   at the NTSC rate exactly 3,504,512, each frame 801 or 802. */
static void lr_audio(void)
{
    uint32_t n, i;
    audio_acc += (uint64_t)LR_SAMPLE_RATE * info.fps_den;
    n = (uint32_t)(audio_acc / info.fps_num);
    audio_acc %= info.fps_num;
    if (n > LR_AUDIO_CAPACITY) {
        n = LR_AUDIO_CAPACITY;
    }
    if (audio_batch_cb != NULL) {
        (void)audio_batch_cb(silence, n);
    } else if (audio_cb != NULL) {
        for (i = 0; i < n; i++) {
            audio_cb(silence[2u * i], silence[2u * i + 1u]);
        }
    }
}

/* L7694: one frame. Input is polled once and not yet read; the crop is
   converted into the frame buffer and sent with one video call. A failed
   frame sends no video or audio. */
void retro_run(void)
{
    cogmaster_frame io;
    uint32_t x, y;

    if (input_poll_cb != NULL) {
        input_poll_cb();
    }
    if (!loaded || cogmaster_run_frame(inst, NULL) != COGMASTER_STATUS_OK) {
        return;
    }
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pixels = native;
    io.pitch_bytes = info.max_w * 2u;
    if (cogmaster_get_frame(inst, &io) != COGMASTER_STATUS_OK || io.w > info.max_w ||
        io.h > info.max_h) {
        return;
    }
    for (y = 0; y < io.h; y++) {
        const uint16_t *src = native + (size_t)y * info.max_w;
        lr_pixel *dst = pixels + (size_t)y * io.w;
        for (x = 0; x < io.w; x++) {
            dst[x] = LR_CONVERT(src[x]);
        }
    }
    if (video_cb != NULL) {
        video_cb(pixels, io.w, io.h, (size_t)io.w * sizeof pixels[0]);
    }
    lr_audio();
}

/* L7707: no save states in this release; the size is 0 and nothing loads. */
size_t retro_serialize_size(void)
{
    return 0u;
}

bool retro_serialize(void *data, size_t len)
{
    (void)data;
    (void)len;
    return false;
}

bool retro_unserialize(const void *data, size_t len)
{
    (void)data;
    (void)len;
    return false;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index;
    (void)enabled;
    (void)code;
}

/* L7761. A NULL game loads the test card; content is refused in this
   release. XRGB8888, or RGB565 in the test build, is requested here (L865);
   the default 0RGB1555 is deprecated. The frame buffers are allocated once,
   here, at the largest field. */
bool retro_load_game(const struct retro_game_info *game)
{
    enum retro_pixel_format format = LR_PIXEL_FORMAT;
    cogmaster_media media;
    size_t dots;

    lr_unload();
    if (game != NULL || inst == NULL) {
        return false;
    }
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) {
        return false;
    }
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = LR_NO_CONTENT_MACHINE;
    info.size = (uint32_t)sizeof info;
    if (cogmaster_load(inst, &media) != COGMASTER_STATUS_OK ||
        cogmaster_get_info(inst, &info) != COGMASTER_STATUS_OK || info.fps_num == 0u) {
        lr_unload();
        return false;
    }
    dots = (size_t)info.max_w * info.max_h;
    native = (uint16_t *)malloc(dots * sizeof *native);
    pixels = (lr_pixel *)malloc(dots * sizeof *pixels);
    if (native == NULL || pixels == NULL) {
        lr_unload();
        return false;
    }
    loaded = 1;
    return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *game,
                             size_t num_info)
{
    (void)game_type;
    (void)game;
    (void)num_info;
    return false;
}

/* L7798: the frame buffers go with the game; the instance stays. */
void retro_unload_game(void)
{
    lr_unload();
}

/* L7812: the test card runs at the NTSC rate. */
unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

/* L7826: no memory regions are exposed in this release. */
void *retro_get_memory_data(unsigned id)
{
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0u;
}
