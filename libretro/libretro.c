/* The libretro adapter: the 25 entry points of libretro.h, written as a thin
 * client of cogmaster.h. "Lnnn" is a line of the vendored libretro.h
 * (RetroArch v1.22.2, see PROVENANCE.md).
 *
 * retro_load_game(NULL) loads the built-in test card; game.data loads a
 * cartridge through the public library API. Audio is whatever the library
 * delivers each frame, so a frontend that paces on audio runs at the core's
 * rate. The av_info division is the only floating point in the shipped
 * code; the core itself is integer only. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "eyes.h"
#include "libretro.h"
#include "palette.h"

/* The library renders at 48 kHz only. */
#define LR_SAMPLE_RATE 48000u
/* The most sample frames one video frame holds: 966 on PAL. */
#define LR_AUDIO_CAPACITY 966u

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
static int16_t audio_pcm[LR_AUDIO_CAPACITY * 2u];
static unsigned controllers[2] = {RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD};
static int full_field, square_pixels;
static struct retro_game_geometry geometry;
/* What each port carries (COGMASTER_DEVICE_*), chosen at load. */
static uint32_t port_device[2];
static int status_logged;
/* The 3-D view: a pure host rule over the pictures; nothing here is hashed or saved. `stereo` is
   4 for the frames after the shutter eye changed, `last_eye` the eye before, and the retained
   pictures are one per eye plus the red-cyan mix, all in the native RGB444 layout. */
enum { LR_VIEW_LEFT, LR_VIEW_RIGHT, LR_VIEW_ALTERNATE, LR_VIEW_ANAGLYPH, LR_VIEW_OFF };
#define LR_STEREO_FRAMES 4u
static int eye_view;
static unsigned stereo, last_eye;
static uint16_t *eye_mem, *eye_pic[2], *eye_mix;

#include "options.h"

static void lr_presentation(void)
{
    full_field = lr_option("cogmaster_gg_view", "full");
    square_pixels = lr_option("cogmaster_gg_aspect", "square");
    eye_view = lr_option("cogmaster_3d_view", "right")       ? LR_VIEW_RIGHT
               : lr_option("cogmaster_3d_view", "alternate") ? LR_VIEW_ALTERNATE
               : lr_option("cogmaster_3d_view", "anaglyph")  ? LR_VIEW_ANAGLYPH
               : lr_option("cogmaster_3d_view", "off")       ? LR_VIEW_OFF
                                                             : LR_VIEW_LEFT;
}

/* One line to the frontend's log, when it offers one. */
static void lr_log(const char *message)
{
    struct retro_log_callback cb = {0};
    if (env_cb != NULL && env_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &cb) && cb.log != NULL) {
        cb.log(RETRO_LOG_WARN, "cogmaster: %s\n", message);
    }
}

static struct retro_game_geometry lr_geometry(uint32_t w, uint32_t h)
{
    struct retro_game_geometry g;
    uint32_t pn = info.par_num, pd = info.par_den;
    if (info.video_mode == COGMASTER_VIDEO_GG && square_pixels) {
        pn = pd = 1u;
    }
    g.base_width = w;
    g.base_height = h;
    g.max_width = info.max_w;
    g.max_height = info.max_h;
    g.aspect_ratio = (float)((double)w * pn / ((double)h * pd));
    return g;
}

static const struct retro_controller_description lr_devices[] = {
    {"Control Pad", RETRO_DEVICE_JOYPAD}, {"None", RETRO_DEVICE_NONE}};
static struct retro_controller_info lr_ports[] = {{lr_devices, 2u}, {lr_devices, 2u}, {NULL, 0u}};
static struct retro_input_descriptor lr_inputs[] = {
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_B, "Button 1"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_A, "Button 2"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_START, "Pause / Start"},
    {0u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_SELECT, "Console Reset"},
    {1u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
    {1u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
    {1u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
    {1u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
    {1u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_B, "Button 1"},
    {1u, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_A, "Button 2"},
    {0u, 0u, 0u, 0u, NULL}};

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
    free(eye_mem);
    eye_mem = eye_mix = eye_pic[0] = eye_pic[1] = NULL;
    stereo = last_eye = 0u;
    port_device[0] = port_device[1] = COGMASTER_DEVICE_PAD;
    status_logged = 0;
    loaded = 0;
    memset(&info, 0, sizeof info);
    full_field = square_pixels = 0;
    memset(&geometry, 0, sizeof geometry);
}

/* L7511: called before retro_init. Declaring no-game support (L1051) makes
   the frontend call retro_load_game(NULL). */
void retro_set_environment(retro_environment_t cb)
{
    bool no_game = true;
    env_cb = cb;
    if (cb != NULL) {
        (void)cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
        lr_register_options(cb);
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
    controllers[0] = RETRO_DEVICE_JOYPAD;
    controllers[1] = RETRO_DEVICE_JOYPAD;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

/* L7627: in-memory content enables frontend archive extraction and patches. */
void retro_get_system_info(struct retro_system_info *sys)
{
    if (sys == NULL) {
        return;
    }
    memset(sys, 0, sizeof *sys);
    sys->library_name = "cogmaster";
    sys->library_version = COGMASTER_LIBRETRO_VERSION;
    sys->valid_extensions = "sms|gg|bin|rom";
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
    av->geometry = geometry;
    av->timing.fps = (double)info.fps_num / (double)info.fps_den;
    av->timing.sample_rate = (double)LR_SAMPLE_RATE;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port < 2u) {
        controllers[port] = device == RETRO_DEVICE_JOYPAD ? device : RETRO_DEVICE_NONE;
    }
}

static uint32_t lr_button(unsigned port, unsigned id)
{
    return input_poll_cb != NULL && input_state_cb != NULL &&
                   controllers[port] == RETRO_DEVICE_JOYPAD &&
                   input_state_cb(port, RETRO_DEVICE_JOYPAD, 0u, id) != 0
               ? 1u
               : 0u;
}

/* How far a stick moves a Sports Pad or Paddle: k counts per frame at full deflection. These
   numbers are judged locally, as is the dead zone. RetroArch's
   delivery of a mouse to a port that is not set to a mouse is unmeasured, so the stick and the
   D-pad are the sure way to move the device. */
#define LR_STICK_SPORTS 6
#define LR_STICK_PADDLE 4
#define LR_STICK_DEAD 6554 /* 20 percent of the full 32768 */
#define LR_DPAD_COUNTS 2
#define LR_AXIS_MAX 32767

static uint32_t lr_mouse_button(unsigned port, unsigned id)
{
    return input_state_cb(port, RETRO_DEVICE_MOUSE, 0u, id) != 0 ? 1u : 0u;
}

/* One axis of a motion port, 0 for x and 1 for y: the mouse delta (already relative), the left
   stick past the dead zone and the D-pad, summed and held inside what the library accepts. pad
   holds the D-pad bits, up, down, left, right from bit 0. Only base device ids are asked. */
static int32_t lr_axis(unsigned port, unsigned axis, int32_t k, uint32_t pad)
{
    static const unsigned mouse_id[2] = {RETRO_DEVICE_ID_MOUSE_X, RETRO_DEVICE_ID_MOUSE_Y};
    static const unsigned stick_id[2] = {RETRO_DEVICE_ID_ANALOG_X, RETRO_DEVICE_ID_ANALOG_Y};
    int32_t mouse = input_state_cb(port, RETRO_DEVICE_MOUSE, 0u, mouse_id[axis]);
    int32_t stick =
        input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, stick_id[axis]);
    uint32_t more = axis == 0u ? pad >> 3 : pad >> 1;
    uint32_t less = axis == 0u ? pad >> 2 : pad;
    int32_t dpad = (int32_t)(more & 1u) - (int32_t)(less & 1u);
    int32_t sum = mouse +
                  (stick > -LR_STICK_DEAD && stick < LR_STICK_DEAD ? 0 : stick * k / 32768) +
                  dpad * LR_DPAD_COUNTS;
    return sum > LR_AXIS_MAX ? LR_AXIS_MAX : (sum < -LR_AXIS_MAX ? -LR_AXIS_MAX : sum);
}

static void lr_input(void)
{
    static const unsigned buttons[6] = {RETRO_DEVICE_ID_JOYPAD_UP,   RETRO_DEVICE_ID_JOYPAD_DOWN,
                                        RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT,
                                        RETRO_DEVICE_ID_JOYPAD_B,    RETRO_DEVICE_ID_JOYPAD_A};
    cogmaster_input in = {0};
    uint32_t pad[2] = {0u, 0u};
    int32_t axis[2][2] = {{0, 0}, {0, 0}};
    int connected = input_poll_cb != NULL && input_state_cb != NULL;
    in.size = sizeof in;
    for (unsigned port = 0u; port < 2u; port++) {
        uint32_t device = connected && controllers[port] != RETRO_DEVICE_NONE
                              ? port_device[port]
                              : COGMASTER_DEVICE_NONE;
        if (port == 0u) {
            in.port1 = device;
        } else {
            in.port2 = device;
        }
        if (device == COGMASTER_DEVICE_NONE) {
            continue;
        }
        for (unsigned bit = 0u; bit < 6u; bit++) {
            pad[port] |= lr_button(port, buttons[bit]) << bit;
        }
        if (device == COGMASTER_DEVICE_SPORTS_PAD || device == COGMASTER_DEVICE_PADDLE) {
            int32_t k = device == COGMASTER_DEVICE_SPORTS_PAD ? LR_STICK_SPORTS : LR_STICK_PADDLE;
            axis[port][0] = lr_axis(port, 0u, k, pad[port]);
            axis[port][1] = lr_axis(port, 1u, k, pad[port]);
            /* Buttons 1 and 2: RetroPad B and A, or the mouse buttons. */
            pad[port] = (pad[port] & 0x30u) |
                        lr_mouse_button(port, RETRO_DEVICE_ID_MOUSE_LEFT) << 4 |
                        lr_mouse_button(port, RETRO_DEVICE_ID_MOUSE_RIGHT) << 5;
        }
    }
    in.p1 = pad[0];
    in.p2 = pad[1];
    in.x1 = axis[0][0];
    in.y1 = axis[0][1];
    in.x2 = axis[1][0];
    in.y2 = axis[1][1];
    if (info.machine == COGMASTER_MACHINE_GG) {
        in.start = lr_button(0u, RETRO_DEVICE_ID_JOYPAD_START);
    } else {
        in.pause = lr_button(0u, RETRO_DEVICE_ID_JOYPAD_START);
        in.reset = lr_button(0u, RETRO_DEVICE_ID_JOYPAD_SELECT);
    }
    if (cogmaster_set_input(inst, &in) != COGMASTER_STATUS_OK && !status_logged) {
        status_logged = 1;
        lr_log("the input call was refused");
    }
}

/* The device a port carries: the option when it names one, else the library's table default,
   which it has already cut back to a pad where the machine cannot take it. libretro cannot refuse
   an option, so a choice this machine cannot take is a pad, with one line in the log. */
static uint32_t lr_choose(unsigned port, const char *key, uint32_t table)
{
    static const char *const names[4] = {"pad", "none", "paddle", "sports"};
    cogmaster_input probe = {0};
    uint32_t choice = table;
    for (uint32_t device = 0u; device < 4u; device++) {
        if (lr_option(key, names[device])) {
            choice = device;
        }
    }
    probe.size = sizeof probe;
    if (port == 0u) {
        probe.port1 = choice;
    } else {
        probe.port2 = choice;
    }
    if (cogmaster_set_input(inst, &probe) == COGMASTER_STATUS_ERR_PROFILE_REJECTED) {
        lr_log("this console cannot take that controller; using a control pad");
        choice = COGMASTER_DEVICE_PAD;
    }
    return choice;
}

/* L7680 prefers soft reset. Frontend Restart reapplies the hand-off on SMS,
   or power-cycles GG; Select is the separate physical Reset pin. */
void retro_reset(void)
{
    stereo = last_eye = 0u;
    if (loaded) {
        (void)cogmaster_reset(inst, info.machine == COGMASTER_MACHINE_GG ? COGMASTER_RESET_POWER
                                                                         : COGMASTER_RESET_SOFT);
    }
}

/* Takes this frame's samples from the library and sends them: one batch call
   when the frontend registered it, else one single-sample call each. A mono
   machine is widened to equal left and right in place, last frame first. The
   library paces the count (801 or 802 frames on NTSC, 965 or 966 on PAL) and
   the adapter keeps no pacing state; it never calls with zero frames. */
static void lr_audio(void)
{
    cogmaster_audio a;
    uint32_t i;

    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = audio_pcm;
    a.capacity = LR_AUDIO_CAPACITY;
    if (cogmaster_get_audio(inst, &a) != COGMASTER_STATUS_OK || a.written == 0u ||
        a.written > LR_AUDIO_CAPACITY) {
        return;
    }
    if (a.channels == 1u) {
        for (i = a.written; i-- > 0u;) {
            int16_t v = audio_pcm[i];
            audio_pcm[2u * i] = v;
            audio_pcm[2u * i + 1u] = v;
        }
    }
    if (audio_batch_cb != NULL) {
        (void)audio_batch_cb(audio_pcm, a.written);
    } else if (audio_cb != NULL) {
        for (i = 0; i < a.written; i++) {
            audio_cb(audio_pcm[2u * i], audio_pcm[2u * i + 1u]);
        }
    }
}

/* The shutter eye of this frame against the last eye seen: a title that alternates it, which is
   what a 3-D title does, sets stereo for the next frames; a title that wrote it once, or never,
   leaves stereo at 0 and its frames go out as they are. */
static void lr_stereo(uint32_t eye)
{
    if (stereo > 0u) {
        stereo--;
    }
    if (eye != COGMASTER_GLASSES_NONE) {
        if (last_eye != 0u && eye != last_eye) {
            stereo = LR_STEREO_FRAMES;
        }
        last_eye = eye;
    }
}

/* The picture to convert and send: the native frame, or while the eyes alternate the held
   picture of the chosen eye or the red-cyan mix. The pictures of both eyes are kept as the frames
   arrive, so the held one is there when the view needs it. */
static const uint16_t *lr_view(uint32_t eye, size_t dots)
{
    if (eye_mem == NULL || eye == COGMASTER_GLASSES_NONE) {
        return native;
    }
    memcpy(eye_pic[eye == COGMASTER_GLASSES_LEFT ? 0 : 1], native, dots * sizeof *native);
    if (stereo == 0u || eye_view == LR_VIEW_ALTERNATE || eye_view == LR_VIEW_OFF) {
        return native;
    }
    if (eye_view == LR_VIEW_ANAGLYPH) {
        cogmaster__eyes_anaglyph(eye_pic[0], eye_pic[1], eye_mix, dots);
        return eye_mix;
    }
    return eye_pic[eye_view == LR_VIEW_LEFT ? 0 : 1];
}

/* L7694: one frame. Input is polled once and not yet read; the frame's audio
   is drained before the video work, even with no callback registered, then the
   crop is converted into the frame buffer and sent with one video call. A
   frame the library refuses to run sends no video or audio. */
void retro_run(void)
{
    cogmaster_frame io;
    cogmaster_run_result result;
    const uint16_t *shown = native;
    uint32_t x, y;
    bool updated = false;

    if (env_cb != NULL && env_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        lr_presentation();
    }

    if (input_poll_cb != NULL) {
        input_poll_cb();
    }
    if (loaded) {
        lr_input();
    }
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    if (!loaded || cogmaster_run_frame(inst, &result) != COGMASTER_STATUS_OK) {
        return;
    }
    lr_stereo(result.glasses);
    lr_audio();
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pixels = native;
    io.pitch_bytes = info.max_w * 2u;
    if (info.video_mode == COGMASTER_VIDEO_GG && full_field) {
        io.flags = COGMASTER_FRAME_FULL_FIELD;
    }
    if (cogmaster_get_frame(inst, &io) != COGMASTER_STATUS_OK || io.w > info.max_w ||
        io.h > info.max_h) {
        return;
    }
    /* Completed metadata and pixels belong to the same field. A refusal
       affects frontend presentation, never bounded core delivery. */
    (void)cogmaster_get_info(inst, &info);
    struct retro_game_geometry next = lr_geometry(io.w, io.h);
    if (next.base_width != geometry.base_width || next.base_height != geometry.base_height ||
        next.aspect_ratio != geometry.aspect_ratio) {
        geometry = next;
        if (env_cb != NULL) {
            (void)env_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
        }
    }
    shown = lr_view(result.glasses, (size_t)info.max_w * info.max_h);
    for (y = 0; y < io.h; y++) {
        const uint16_t *src = shown + (size_t)y * info.max_w;
        lr_pixel *dst = pixels + (size_t)y * io.w;
        for (x = 0; x < io.w; x++) {
            dst[x] = LR_CONVERT(src[x]);
        }
    }
    if (video_cb != NULL) {
        video_cb(pixels, io.w, io.h, (size_t)io.w * sizeof pixels[0]);
    }
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

/* L7761. A NULL game loads the test card; content uses game.data and size.
   XRGB8888, or RGB565 in the test build, is requested here (L865);
   the default 0RGB1555 is deprecated. The frame buffers are allocated once,
   here, at the largest field. */
bool retro_load_game(const struct retro_game_info *game)
{
    enum retro_pixel_format format = LR_PIXEL_FORMAT;
    cogmaster_media media;
    size_t dots;

    lr_unload();
    if (inst == NULL || (game != NULL && (game->data == NULL || game->size == 0u))) {
        return false;
    }
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = game == NULL ? LR_NO_CONTENT_MACHINE : COGMASTER_MACHINE_AUTO;
    if (game != NULL) {
        cogmaster_cart_info cart = {0};
        cart.size = sizeof cart;
        if (cogmaster_probe(game->data, game->size, &cart) != COGMASTER_STATUS_OK) {
            return false;
        }
        media.machine = cart.machine_guess;
        media.region = cart.region_guess;
        if (lr_option("cogmaster_machine", "sms2"))
            media.machine = COGMASTER_MACHINE_SMS2;
        if (lr_option("cogmaster_machine", "sms1"))
            media.machine = COGMASTER_MACHINE_SMS1;
        if (lr_option("cogmaster_machine", "gg"))
            media.machine = COGMASTER_MACHINE_GG;
        if (lr_option("cogmaster_gg_mode", "sms"))
            media.video_mode = COGMASTER_VIDEO_SMS;
        if (lr_option("cogmaster_gg_mode", "gg"))
            media.video_mode = COGMASTER_VIDEO_GG;
        if (lr_option("cogmaster_region", "pal")) {
            media.tv = COGMASTER_TV_PAL;
            media.region = COGMASTER_REGION_EXPORT;
        }
        if (lr_option("cogmaster_region", "ntsc-u")) {
            media.tv = COGMASTER_TV_NTSC;
            media.region = COGMASTER_REGION_EXPORT;
        }
        if (lr_option("cogmaster_region", "ntsc-j")) {
            media.tv = COGMASTER_TV_NTSC;
            media.region = COGMASTER_REGION_JAPAN;
        }
        /* Automatic attaches the FM unit on Japanese content only; Off never does. On forces it
           on a Master System, and on a Game Gear, which has none, stays automatic so the content
           still loads. */
        if (lr_option("cogmaster_fm", "off"))
            media.fm = COGMASTER_FM_OFF;
        if (lr_option("cogmaster_fm", "on") && media.machine != COGMASTER_MACHINE_GG)
            media.fm = COGMASTER_FM_ON;
        media.image = game->data;
        media.image_size = game->size;
    }
    info.size = (uint32_t)sizeof info;
    if (cogmaster_load(inst, &media) != COGMASTER_STATUS_OK ||
        cogmaster_get_info(inst, &info) != COGMASTER_STATUS_OK || info.fps_num == 0u) {
        lr_unload();
        return false;
    }
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) {
        lr_unload();
        return false;
    }
    (void)env_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, lr_ports);
    (void)env_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, lr_inputs);
    dots = (size_t)info.max_w * info.max_h;
    native = (uint16_t *)malloc(dots * sizeof *native);
    pixels = (lr_pixel *)malloc(dots * sizeof *pixels);
    if (native == NULL || pixels == NULL) {
        lr_unload();
        return false;
    }
    /* A failed allocation costs the 3-D views only: every frame then goes out as it is. */
    eye_mem = (uint16_t *)malloc(3u * dots * sizeof *eye_mem);
    if (eye_mem == NULL) {
        lr_log("no memory for the 3-D views; frames go out as they are");
    } else {
        eye_pic[0] = eye_mem;
        eye_pic[1] = eye_mem + dots;
        eye_mix = eye_mem + 2u * dots;
    }
    port_device[0] = lr_choose(0u, "cogmaster_port1", info.default_port1);
    port_device[1] = lr_choose(1u, "cogmaster_port2", info.default_port2);
    loaded = 1;
    lr_presentation();
    geometry = lr_geometry(
        info.video_mode == COGMASTER_VIDEO_GG && full_field ? info.field_w : info.crop_w,
        info.video_mode == COGMASTER_VIDEO_GG && full_field ? info.field_h : info.crop_h);
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
    return loaded && info.tv == COGMASTER_TV_PAL ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

/* L7826: only the classified persistent span is SAVE_RAM. Its lifetime
   follows load/unload, not mapper enable; the frontend owns file I/O. */
static cogmaster_battery lr_battery(unsigned id)
{
    cogmaster_battery b = {0};
    b.size = sizeof b;
    if (loaded && id == RETRO_MEMORY_SAVE_RAM)
        (void)cogmaster_get_battery(inst, &b);
    return b;
}

void *retro_get_memory_data(unsigned id)
{
    return lr_battery(id).data;
}

size_t retro_get_memory_size(unsigned id)
{
    return lr_battery(id).length;
}
