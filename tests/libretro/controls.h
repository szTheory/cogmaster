/* Normal guest execution exposes controller pins and reset residue as
   backdrop colours, without a test-only adapter entry point. */
static uint32_t control_buttons[2];
static unsigned control_reads[2][16];
static uint32_t control_profile;
static unsigned control_rotate;

static int16_t RETRO_CALLCONV control_input(unsigned port, unsigned device, unsigned index,
                                            unsigned id)
{
    CHECK_EQ_U64(device, RETRO_DEVICE_JOYPAD);
    CHECK_EQ_U64(index, 0u);
    CHECK(port < 2u && id < 16u);
    if (port >= 2u || id >= 16u)
        return 0;
    control_reads[port][id]++;
    return (control_buttons[port] & (1u << id)) != 0u ? 1 : 0;
}

/* The guest program of the controls cases, written into a 32 KiB image. */
static void control_image(uint8_t *image, uint32_t profile, uint32_t source, unsigned rotate)
{
    memset(image, 0, 32768u);
    /* DI; SP=$DFF0; ++boots; Mode 4 blank; JP $80. */
    static const uint8_t boot[] = {0xF3, 0x31, 0xF0, 0xDF, 0x21, 0x02, 0xC2, 0x34, 0x3E, 4,
                                   0xD3, 0xBF, 0x3E, 0x80, 0xD3, 0xBF, 0xC3, 0x80, 0};
    static const uint8_t nmi[] = {0x21, 0x03, 0xC2, 0x34, 0xED, 0x45};
    /* Read port/RAM; rotate to expose high pins; write backdrop; repeat. */
    uint8_t loop[] = {0xDB, 0xDC, 0,    0,    0,    0xE6, 0x3F, 0x47, 0x3E, 16,   0xD3, 0xBF, 0x3E,
                      0xC0, 0xD3, 0xBF, 0x78, 0xD3, 0xBE, 0,    0,    0,    0xC3, 0x80, 0};
    if (source < 256u)
        loop[1] = (uint8_t)source;
    else {
        loop[0] = 0x3A;
        loop[1] = (uint8_t)source;
        loop[2] = (uint8_t)(source >> 8);
    }
    loop[3] = rotate ? 0x0Fu : 0u;
    loop[4] = rotate ? 0x0Fu : 0u;
    if (profile == 2u) {
        loop[9] = 32u;
        loop[19] = 0xAFu;
        loop[20] = 0xD3u;
        loop[21] = 0xBEu;
    }
    memcpy(image, boot, sizeof boot);
    memcpy(image + 0x66u, nmi, sizeof nmi);
    memcpy(image + 0x80u, loop, sizeof loop);
    memcpy(image + 0x7FF0u, "TMR SEGA", 8u);
    image[0x7FFFu] = profile >= 2u ? 0x6Cu : 0x4Cu;
}

static void control_load(uint32_t profile, uint32_t source, unsigned rotate)
{
    static uint8_t image[32768];
    control_image(image, profile, source, rotate);
    option_machine = profile == 0u ? "sms1" : profile == 1u ? "sms2" : "gg";
    option_mode = profile == 3u ? "sms" : "auto";
    option_view = "native";
    option_region = "ntsc-u";
    option_aspect = "6:5";
    struct retro_game_info game = {0};
    game.data = image;
    game.size = sizeof image;
    control_profile = profile;
    control_rotate = rotate;
    want_w = profile == 2u ? 160u : 256u;
    want_h = profile == 2u ? 144u : 192u;
    CHECK(p_load_game(&game));
    memset(control_buttons, 0, sizeof control_buttons);
    p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
    p_set_controller_port_device(1u, RETRO_DEVICE_JOYPAD);
    p_set_input_poll(on_poll);
    p_set_input_state(control_input);
}

static void control_frame(uint8_t raw)
{
    if (control_rotate)
        raw = (uint8_t)((raw >> 2) | (raw << 6));
    uint16_t colour = raw & 63u;
    if (control_profile != 2u)
        colour = (uint16_t)(((colour & 3u) * 5u) | (((colour >> 2) & 3u) * 5u << 4) |
                            ((colour >> 4) * 5u << 8));
    unsigned before = poll_calls;
    (void)run_one();
    CHECK_EQ_U64(poll_calls - before, 1u);
    uint16_t got;
    CHECK(to_rgb444((want_h / 2u) * want_w + want_w / 2u, &got));
    CHECK_EQ_HEX(got, colour);
}

static int controls_test(void)
{
    p_set_environment(env);
    p_init();
    set_callbacks();
    const unsigned ids[6] = {RETRO_DEVICE_ID_JOYPAD_UP,   RETRO_DEVICE_ID_JOYPAD_DOWN,
                             RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT,
                             RETRO_DEVICE_ID_JOYPAD_B,    RETRO_DEVICE_ID_JOYPAD_A};
    for (uint32_t profile = 0u; profile < 4u; profile++) {
        for (uint32_t port = 0u; port < 2u; port++) {
            control_load(profile, port == 0u ? 0xDCu : 0xDDu, port == 0u ? 2u : 0u);
            control_frame(255u);
            for (uint32_t pad = 0u; pad < 2u; pad++) {
                for (uint32_t bit = 0u; bit < 6u; bit++) {
                    control_buttons[pad] = 1u << ids[bit];
                    uint32_t p1 = pad == 0u ? 1u << bit : 0u;
                    uint32_t p2 = pad == 1u && profile < 2u ? 1u << bit : 0u;
                    uint8_t raw = (uint8_t)~(port == 0u ? p1 | ((p2 & 3u) << 6) : p2 >> 2);
                    control_frame(raw);
                    control_buttons[pad] = 0u;
                }
            }
            /* Both opposing direction pairs are submitted together. */
            control_buttons[0] = control_buttons[1] = 0u;
            for (uint32_t bit = 0u; bit < 6u; bit++)
                control_buttons[0] |= 1u << ids[bit];
            control_frame(port == 0u ? 0xC0u : 255u);
            p_set_controller_port_device(0u, RETRO_DEVICE_NONE);
            control_frame(255u);
            p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
            control_frame(port == 0u ? 0xC0u : 255u);
            p_set_input_state(NULL);
            control_frame(255u);
            p_set_input_state(control_input);
            p_set_input_poll(NULL);
            /* Absent poll is also a disconnected input source. */
            (void)run_one();
            uint16_t got;
            CHECK(to_rgb444(want_w * (want_h / 2u), &got));
            CHECK_EQ_HEX(got, profile == 2u ? 63u : 0xFFFu);
        }
        /* Physical Reset is a level, never a restart, and port 2 has no
           console-control ownership. */
        control_load(profile, 0xDDu, 0u);
        control_buttons[1] =
            (1u << RETRO_DEVICE_ID_JOYPAD_SELECT) | (1u << RETRO_DEVICE_ID_JOYPAD_START);
        memset(control_reads, 0, sizeof control_reads);
        control_frame(255u);
        CHECK_EQ_U64(control_reads[1][RETRO_DEVICE_ID_JOYPAD_START], 0u);
        CHECK_EQ_U64(control_reads[1][RETRO_DEVICE_ID_JOYPAD_SELECT], 0u);
        control_buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_SELECT;
        control_frame(profile == 0u ? 0xEFu : 255u);
        /* Restart re-executes boot: SOFT retains SMS boots, POWER clears GG. */
        control_load(profile, 0xC202u, 0u);
        control_frame(1u);
        control_buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_SELECT;
        control_frame(1u);
        p_reset();
        control_frame(profile < 2u ? 2u : 1u);
        /* Native Start stays a level; SMS/GG SMS-mode Start is one NMI. */
        control_load(profile, 0xC203u, 0u);
        control_buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_START;
        control_frame((uint8_t)(profile == 2u ? 0u : 1u));
        control_frame((uint8_t)(profile == 2u ? 0u : 1u));
        p_set_input_state(NULL);
        control_frame((uint8_t)(profile == 2u ? 0u : 1u));
        p_set_input_state(control_input);
        control_frame((uint8_t)(profile == 2u ? 0u : 1u));
        control_buttons[0] = 0u;
        control_frame((uint8_t)(profile == 2u ? 0u : 1u));
        control_buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_START;
        control_frame(profile == 2u ? 0u : 2u);
        p_set_controller_port_device(0u, RETRO_DEVICE_NONE);
        control_frame(profile == 2u ? 0u : 2u);
        p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
        control_frame(profile == 2u ? 0u : 2u);
        control_frame(profile == 2u ? 0u : 2u);
        if (profile == 2u) {
            control_load(profile, 0u, 2u);
            control_frame(0xC0u);
            control_buttons[0] = 1u << RETRO_DEVICE_ID_JOYPAD_START;
            control_frame(0x40u);
            control_frame(0x40u);
            p_set_controller_port_device(0u, RETRO_DEVICE_NONE);
            control_frame(0xC0u);
        }
    }
    p_unload_game();
    p_deinit();
    return check_failures != 0 ? 1 : 0;
}

/* ---- Sports Pad, Paddle and the 3-D views through the adapter ----
   The picture the adapter presents is compared with the one the public library draws when it is
   given the same scripted input, the devices the design's table names and the sums written out
   again below, so a wrong device, a wrong sum or a held picture shows as a different picture. */

#define DEV_FRAME_DOTS (256u * 192u)
#define DEV_BIT(v, id) (((v) >> (id)) & 1u)

static struct {
    int16_t mouse[2][2], stick[2][2];
    uint32_t pad[2];   /* a bit per RetroPad id */
    uint32_t click[2]; /* b0 left, b1 right */
} dev_in;
static unsigned dev_polled[2]; /* b0 the pad, b1 the mouse, b2 the stick, b3 anything else */
typedef void (*dev_script)(unsigned step);

/* What a frontend serves: base device ids only, and any other device value is noted. */
static int16_t RETRO_CALLCONV dev_input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port < 2u && device == RETRO_DEVICE_JOYPAD && index == 0u && id < 16u) {
        dev_polled[port] |= id == RETRO_DEVICE_ID_JOYPAD_UP ? 1u : 0u;
        return (int16_t)DEV_BIT(dev_in.pad[port], id);
    }
    if (port < 2u && device == RETRO_DEVICE_MOUSE && index == 0u && id < 4u) {
        dev_polled[port] |= 2u;
        return id < 2u ? dev_in.mouse[port][id]
                       : (int16_t)DEV_BIT(dev_in.click[port], id - RETRO_DEVICE_ID_MOUSE_LEFT);
    }
    if (port < 2u && device == RETRO_DEVICE_ANALOG && index == RETRO_DEVICE_INDEX_ANALOG_LEFT &&
        id < 2u) {
        dev_polled[port] |= 4u;
        return dev_in.stick[port][id];
    }
    dev_polled[port & 1u] |= 8u;
    return 0;
}

/* mouse plus stick past a 20 percent dead zone times k over 32768 plus two counts for the D-pad,
   held to the library's range. */
static int32_t dev_sum(int32_t mouse, int32_t stick, int32_t dpad, int32_t k)
{
    int32_t sum = mouse + (stick >= 6554 || stick <= -6554 ? stick * k / 32768 : 0) + 2 * dpad;
    return sum > 32767 ? 32767 : (sum < -32767 ? -32767 : sum);
}

/* The library input the frontend's state makes for a port that carries device. */
static void dev_expect(unsigned port, uint32_t device, cogmaster_input *in)
{
    uint32_t pad = dev_in.pad[port], click = dev_in.click[port], bits = 0u;
    int32_t x = 0, y = 0;
    if (device == COGMASTER_DEVICE_PAD) {
        bits = DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_UP) |
               DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_DOWN) << 1 |
               DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_LEFT) << 2 |
               DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_RIGHT) << 3 |
               DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_B) << 4 |
               DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_A) << 5;
    } else if (device != COGMASTER_DEVICE_NONE) {
        int32_t k = device == COGMASTER_DEVICE_SPORTS_PAD ? 6 : 4;
        bits = (DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_B) | (click & 1u)) << 4 |
               (DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_A) | (click >> 1)) << 5;
        x = dev_sum(dev_in.mouse[port][0], dev_in.stick[port][0],
                    (int32_t)DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_RIGHT) -
                        (int32_t)DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_LEFT),
                    k);
        y = dev_sum(dev_in.mouse[port][1], dev_in.stick[port][1],
                    (int32_t)DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_DOWN) -
                        (int32_t)DEV_BIT(pad, RETRO_DEVICE_ID_JOYPAD_UP),
                    k);
    }
    if (port == 0u) {
        in->p1 = bits;
        in->x1 = x;
        in->y1 = y;
    } else {
        in->p2 = bits;
        in->x2 = x;
        in->y2 = y;
    }
}

typedef struct {
    const uint8_t *image;
    const char *p1, *p2, *view; /* the adapter's options, NULL for unset */
    uint32_t d1, d2;            /* the library's devices */
    unsigned frames, reset_at;  /* a reset after that many frames, 0 for none */
    dev_script script;
    int frontend_none;    /* the frontend takes port 1 away */
    uint32_t *eyes;       /* per frame, library only */
    uint16_t *dots, *pic; /* the first dot of every frame, the last whole picture */
} dev_run;

static uint16_t dev_native[MAX_W * MAX_H];

/* The public library alone, driven as the table says. */
static void dev_reference(const dev_run *r)
{
    cogmaster_config cfg = {0};
    cogmaster *lib = NULL;
    cogmaster_cart_info cart = {0};
    cogmaster_media media = {0};
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = presentation_alloc;
    cfg.allocator.free = presentation_free;
    CHECK_EQ_U64(cogmaster_create(&cfg, &lib), COGMASTER_STATUS_OK);
    cart.size = (uint32_t)sizeof cart;
    CHECK_EQ_U64(cogmaster_probe(r->image, 32768u, &cart), COGMASTER_STATUS_OK);
    media.size = (uint32_t)sizeof media;
    media.machine = cart.machine_guess;
    media.region = cart.region_guess;
    media.image = r->image;
    media.image_size = 32768u;
    CHECK_EQ_U64(cogmaster_load(lib, &media), COGMASTER_STATUS_OK);
    for (unsigned f = 1u; f <= r->frames; f++) {
        cogmaster_input in = {0};
        cogmaster_run_result res = {0};
        cogmaster_frame io = {0};
        if (f == r->reset_at + 1u && r->reset_at != 0u) {
            CHECK_EQ_U64(cogmaster_reset(lib, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
        }
        r->script(f);
        in.size = (uint32_t)sizeof in;
        in.port1 = r->d1;
        in.port2 = r->d2;
        dev_expect(0u, r->d1, &in);
        dev_expect(1u, r->d2, &in);
        CHECK_EQ_U64(cogmaster_set_input(lib, &in), COGMASTER_STATUS_OK);
        res.size = (uint32_t)sizeof res;
        CHECK_EQ_U64(cogmaster_run_frame(lib, &res), COGMASTER_STATUS_OK);
        io.size = (uint32_t)sizeof io;
        io.pixels = dev_native;
        io.pitch_bytes = MAX_W * 2u;
        CHECK_EQ_U64(cogmaster_get_frame(lib, &io), COGMASTER_STATUS_OK);
        CHECK(io.w == 256u && io.h == 192u);
        if (r->eyes != NULL) {
            r->eyes[f - 1u] = res.glasses;
        }
        if (r->dots != NULL) {
            r->dots[f - 1u] = dev_native[0];
        }
        if (f == r->frames && r->pic != NULL) {
            memcpy(r->pic, dev_native, DEV_FRAME_DOTS * sizeof r->pic[0]);
        }
    }
    cogmaster_destroy(lib);
}

/* The adapter as a frontend runs it. */
static void dev_adapter(const dev_run *r)
{
    struct retro_game_info game = {0};
    unsigned options0 = options_calls, descriptors0 = descriptors_calls;
    option_machine = option_mode = option_region = option_aspect = option_view = option_fm = NULL;
    option_port1 = r->p1;
    option_port2 = r->p2;
    option_3d = r->view;
    want_w = 256u;
    want_h = 192u;
    dev_polled[0] = dev_polled[1] = 0u;
    log_lines = 0u;
    p_set_environment(env);
    p_init();
    set_callbacks();
    p_set_input_state(dev_input);
    p_set_controller_port_device(0u, r->frontend_none ? RETRO_DEVICE_NONE : RETRO_DEVICE_JOYPAD);
    p_set_controller_port_device(1u, RETRO_DEVICE_JOYPAD);
    game.data = r->image;
    game.size = 32768u;
    CHECK(p_load_game(&game));
    CHECK_EQ_U64(options_calls - options0, 1u);
    CHECK_EQ_U64(descriptors_calls - descriptors0, 1u);
    for (unsigned f = 1u; f <= r->frames; f++) {
        uint16_t dot = 0u, mid = 0u, last = 0u;
        if (f == r->reset_at + 1u && r->reset_at != 0u) {
            p_reset();
        }
        r->script(f);
        (void)run_one();
        CHECK(to_rgb444(0u, &dot));
        if (r->dots != NULL) {
            /* The picture is one colour, so three dots tell. */
            CHECK(to_rgb444(96u * 256u + 128u, &mid) && to_rgb444(DEV_FRAME_DOTS - 1u, &last));
            CHECK(mid == dot && last == dot);
            r->dots[f - 1u] = dot;
        }
        if (f == r->frames && r->pic != NULL) {
            for (uint32_t i = 0u; i < DEV_FRAME_DOTS; i++) {
                CHECK(to_rgb444(i, &r->pic[i]));
            }
        }
    }
    CHECK_EQ_U64(video_bad, 0u);
    CHECK_EQ_U64(dev_polled[0] & 8u, 0u);
    CHECK_EQ_U64(dev_polled[1] & 8u, 0u);
    p_unload_game();
    p_deinit();
}

static uint32_t dev_fold(const uint16_t *pic)
{
    uint32_t h = 2166136261u;
    for (uint32_t i = 0u; i < DEV_FRAME_DOTS; i++) {
        h = (h ^ pic[i]) * 16777619u;
    }
    return h;
}

static uint16_t dev_pic_a[DEV_FRAME_DOTS], dev_pic_r[DEV_FRAME_DOTS];

/* The adapter's last picture must be the library's. */
static void dev_same(const char *what, const dev_run *r)
{
    dev_run a = *r, b = *r;
    a.pic = dev_pic_a;
    b.pic = dev_pic_r;
    dev_adapter(&a);
    dev_reference(&b);
    if (memcmp(dev_pic_a, dev_pic_r, sizeof dev_pic_a) != 0) {
        fprintf(stderr, "%s: the adapter's picture is not the library's\n", what);
        check_failures++;
    }
}

/* The adapter's last picture alone. */
static uint32_t dev_shown(const dev_run *r)
{
    dev_run a = *r;
    a.pic = dev_pic_a;
    dev_adapter(&a);
    return dev_fold(dev_pic_a);
}

static void dev_clear(unsigned step)
{
    (void)step;
    memset(&dev_in, 0, sizeof dev_in);
}

/* Every source at once, a different mix on each port. */
static void dev_mix(unsigned step)
{
    dev_clear(step);
    dev_in.mouse[0][0] = 11;
    dev_in.mouse[0][1] = -5;
    dev_in.mouse[1][0] = -7;
    dev_in.mouse[1][1] = 4;
    dev_in.stick[0][0] = 20000;
    dev_in.stick[0][1] = -20000;
    dev_in.stick[1][0] = -9000;
    dev_in.stick[1][1] = 7000;
    dev_in.pad[0] = 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT | 1u << RETRO_DEVICE_ID_JOYPAD_UP |
                    1u << RETRO_DEVICE_ID_JOYPAD_B;
    dev_in.pad[1] = 1u << RETRO_DEVICE_ID_JOYPAD_LEFT | 1u << RETRO_DEVICE_ID_JOYPAD_A;
    dev_in.click[0] = 2u;
    dev_in.click[1] = 1u;
}

/* One source on port 1, in one direction, for the first dev_until frames. */
enum { DEV_NONE, DEV_MOUSE, DEV_STICK, DEV_DPAD, DEV_INSIDE, DEV_EDGE, DEV_HUGE };
static int dev_kind, dev_sign;
static unsigned dev_until;

static void dev_motion(unsigned step)
{
    int16_t s = (int16_t)dev_sign;
    dev_clear(step);
    if (step > dev_until) {
        return;
    }
    if (dev_kind == DEV_MOUSE) {
        dev_in.mouse[0][0] = (int16_t)(11 * s);
    } else if (dev_kind == DEV_STICK) {
        dev_in.stick[0][0] = (int16_t)(20000 * s);
    } else if (dev_kind == DEV_DPAD) {
        dev_in.pad[0] = 1u << (s > 0 ? RETRO_DEVICE_ID_JOYPAD_RIGHT : RETRO_DEVICE_ID_JOYPAD_LEFT);
    } else if (dev_kind == DEV_INSIDE) {
        dev_in.stick[0][0] = (int16_t)(6553 * s);
    } else if (dev_kind == DEV_EDGE) {
        dev_in.stick[0][0] = (int16_t)(6554 * s);
    } else if (dev_kind == DEV_HUGE) {
        /* Every term at its limit: the sum is past the range on both axes. */
        dev_in.mouse[0][0] = (int16_t)(s > 0 ? 32767 : -32768);
        dev_in.mouse[0][1] = (int16_t)(s > 0 ? -32768 : 32767);
        dev_in.stick[0][0] = (int16_t)(s > 0 ? 32767 : -32768);
        dev_in.stick[0][1] = (int16_t)(s > 0 ? -32768 : 32767);
        dev_in.pad[0] = 1u << (s > 0 ? RETRO_DEVICE_ID_JOYPAD_RIGHT : RETRO_DEVICE_ID_JOYPAD_LEFT) |
                        1u << (s > 0 ? RETRO_DEVICE_ID_JOYPAD_UP : RETRO_DEVICE_ID_JOYPAD_DOWN);
    }
}

/* One button of port 1 at a time (dev_kind): B, A, mouse left, mouse right; with the mouse
   running as far as it can. */
static void dev_buttons(unsigned step)
{
    dev_clear(step);
    dev_in.mouse[0][0] = -32768;
    dev_in.pad[0] = dev_kind == 0 ? 1u << RETRO_DEVICE_ID_JOYPAD_B
                                  : (dev_kind == 1 ? 1u << RETRO_DEVICE_ID_JOYPAD_A : 0u);
    dev_in.click[0] = dev_kind == 2 ? 1u : (dev_kind == 3 ? 2u : 0u);
}

enum {
    DEV_SPORTS_IMAGE,
    DEV_PADDLE_IMAGE,
    DEV_CONTROL_IMAGE,
    DEV_GLASSES_IMAGE,
    DEV_ALT_IMAGE,
    DEV_ONCE_IMAGE,
    DEV_IMAGES
};
static uint8_t dev_image[DEV_IMAGES][32768];

/* A guest that sets the shutter eye and the backdrop together every frame, the left eye with a
   red backdrop and the right with a cyan one; once is the same picture with the shutter written
   a single time. */
static void dev_stereo_image(uint8_t *image, int alternate)
{
    static const uint8_t init[] = {0xF3, 0x31, 0xF0, 0xDF, 0x3E, 4,    0xD3, 0xBF, 0x3E,
                                   0x80, 0xD3, 0xBF, 0xAF, 0xD3, 0xBF, 0x3E, 0x81, 0xD3,
                                   0xBF, 0x16, 0x00, 0x3E, 0x00, 0x32, 0xF8, 0xFF};
    static const uint8_t wait[] = {0xDB, 0xBF, 0xB7, 0xF2, 0x1B, 0x00, 0x7A, 0xEE, 0x01, 0x57};
    static const uint8_t shutter[] = {0x32, 0xF8, 0xFF};
    static const uint8_t paint[] = {0x3E, 0x3C, 0xCB, 0x42, 0x28, 0x02, 0x3E, 0x03,
                                    0x4F, 0x3E, 0x10, 0xD3, 0xBF, 0x3E, 0xC0, 0xD3,
                                    0xBF, 0x79, 0xD3, 0xBE, 0xC3, 0x1B, 0x00};
    size_t at = 0u, loop;
    memset(image, 0, 32768u);
    /* Without the first write the init stops before LD A,0 and LD (nn),A. */
    memcpy(image, init, sizeof init - (alternate ? 5u : 0u));
    at = sizeof init - (alternate ? 5u : 0u);
    loop = at;
    memcpy(image + at, wait, sizeof wait);
    at += sizeof wait;
    if (alternate) {
        memcpy(image + at, shutter, sizeof shutter);
        at += sizeof shutter;
    }
    memcpy(image + at, paint, sizeof paint);
    /* The wait loop jumps back to its own first byte, and the paint loop to it too. */
    image[loop + 4u] = (uint8_t)loop;
    image[at + sizeof paint - 2u] = (uint8_t)loop;
    memcpy(image + 0x7FF0u, "TMR SEGA", 8u);
    image[0x7FFFu] = 0x4Cu;
}

static const char *const dev_options[5] = {"auto", "pad", "paddle", "sports", "none"};

/* The device an option gives on content whose table default is table: the option wins, auto takes
   the default, and a Paddle needs the Japanese console. */
static uint32_t dev_device(const char *option, uint32_t table, int japan)
{
    if (strcmp(option, "pad") == 0)
        return COGMASTER_DEVICE_PAD;
    if (strcmp(option, "none") == 0)
        return COGMASTER_DEVICE_NONE;
    if (strcmp(option, "sports") == 0)
        return COGMASTER_DEVICE_SPORTS_PAD;
    if (strcmp(option, "paddle") == 0)
        return japan ? COGMASTER_DEVICE_PADDLE : COGMASTER_DEVICE_PAD;
    return table;
}

/* The reads a port makes: the pad, then the mouse and the stick as well for a moving device. */
static unsigned dev_reads(uint32_t device)
{
    return device == COGMASTER_DEVICE_PAD ? 1u : (device == COGMASTER_DEVICE_NONE ? 0u : 7u);
}

/* Which device each option gives, on content made for each of them, seen through the reads the
   adapter makes, the log and the picture the guest ends with. */
static void dev_options_test(void)
{
    /* The default devices of each fixture's table row, whether its region is Japanese, and
       whether its picture tells every device from every other. */
    static const struct {
        int image;
        uint32_t table1, table2;
        int japan, strict;
        unsigned frames;
    } rows[3] = {
        {DEV_SPORTS_IMAGE, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0, 1, 20u},
        {DEV_PADDLE_IMAGE, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_PAD, 1, 0, 40u},
        {DEV_CONTROL_IMAGE, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, 0, 1, 20u}};
    for (unsigned row = 0u; row < 3u; row++) {
        dev_run r = {0};
        uint32_t seen[4] = {0};
        r.image = dev_image[rows[row].image];
        r.frames = rows[row].frames;
        /* A Paddle saturates under the mixed input, so its fixture gets a short push. */
        r.script = rows[row].japan ? dev_motion : dev_mix;
        dev_kind = DEV_MOUSE;
        dev_sign = 1;
        dev_until = 8u;
        for (unsigned i = 0u; i < 5u; i++) {
            const char *o1 = dev_options[i], *o2 = dev_options[(i + 2u) % 5u];
            uint32_t e1 = dev_device(o1, rows[row].table1, rows[row].japan);
            uint32_t e2 = dev_device(o2, rows[row].table2, rows[row].japan);
            unsigned degraded = (strcmp(o1, "paddle") == 0 && !rows[row].japan ? 1u : 0u) +
                                (strcmp(o2, "paddle") == 0 && !rows[row].japan ? 1u : 0u);
            r.p1 = o1;
            r.p2 = o2;
            r.d1 = e1;
            r.d2 = e2;
            dev_same(o1, &r);
            CHECK_EQ_U64(log_lines, degraded);
            CHECK_EQ_U64(dev_polled[0], dev_reads(e1));
            CHECK_EQ_U64(dev_polled[1], dev_reads(e2));
        }
        /* Control: the picture tells the devices apart, so the sweep above can fail. */
        r.p1 = r.p2 = NULL;
        r.d2 = rows[row].table2;
        for (uint32_t d = 0u; d < 4u; d++) {
            if (d == COGMASTER_DEVICE_PADDLE && !rows[row].japan)
                continue;
            r.d1 = d;
            r.pic = dev_pic_r;
            dev_reference(&r);
            seen[d] = dev_fold(dev_pic_r);
            for (uint32_t e = 0u; e < d; e++) {
                int paddle = d == COGMASTER_DEVICE_PADDLE || e == COGMASTER_DEVICE_PADDLE;
                if (seen[e] == seen[d] && (rows[row].strict || paddle) &&
                    (e != COGMASTER_DEVICE_PADDLE || rows[row].japan)) {
                    fprintf(stderr, "image %u: devices %u and %u draw the same picture\n", row,
                            (unsigned)e, (unsigned)d);
                    check_failures++;
                }
            }
        }
        /* A frontend that takes port 1 away wins over the table. */
        r.pic = NULL;
        r.d1 = COGMASTER_DEVICE_NONE;
        r.frontend_none = 1;
        dev_same("frontend none", &r);
        CHECK_EQ_U64(dev_polled[0], 0u);
    }
}

/* The sums of every source, in both signs, and the buttons, against the closed form; the dead zone
   edge in the picture; and no device id beyond the base ones (dev_adapter notes any). */
static void dev_sources_test(void)
{
    dev_run r = {0};
    uint32_t zero;
    r.script = dev_motion;
    for (unsigned image = 0u; image < 2u; image++) {
        r.image = dev_image[image];
        r.d2 = image == 0u ? COGMASTER_DEVICE_SPORTS_PAD : COGMASTER_DEVICE_PAD;
        r.d1 = image == 0u ? COGMASTER_DEVICE_SPORTS_PAD : COGMASTER_DEVICE_PADDLE;
        r.frames = image == 0u ? 20u : 40u;
        dev_until = image == 0u ? 20u : 8u;
        dev_kind = DEV_NONE;
        dev_sign = 1;
        zero = dev_shown(&r);
        for (int kind = DEV_MOUSE; kind <= DEV_EDGE; kind++) {
            for (int sign = -1; sign <= 1; sign += 2) {
                dev_kind = kind;
                dev_sign = sign;
                dev_same("one source", &r);
                CHECK_EQ_U64(dev_polled[0], 7u);
                /* The edge: 6553 is inside the dead zone and moves nothing; 6554 moves a
                   Sports Pad by one count (6554 * 6 / 32768) and a Paddle by none (x4). */
                if (kind == DEV_INSIDE || (kind == DEV_EDGE && image == 1u)) {
                    CHECK(dev_fold(dev_pic_a) == zero);
                } else {
                    CHECK(dev_fold(dev_pic_a) != zero);
                }
            }
        }
    }
    /* Every source on both ports at once, and the buttons, read back through the pins. */
    r.image = dev_image[DEV_CONTROL_IMAGE];
    r.frames = 6u;
    r.d1 = r.d2 = COGMASTER_DEVICE_SPORTS_PAD;
    r.p1 = r.p2 = "sports";
    r.script = dev_mix;
    dev_same("both ports", &r);
    r.d2 = COGMASTER_DEVICE_PAD;
    r.p2 = NULL;
    r.script = dev_buttons;
    dev_kind = 4;
    zero = dev_shown(&r);
    for (dev_kind = 0; dev_kind < 4; dev_kind++) {
        dev_same("button", &r);
        CHECK(dev_fold(dev_pic_a) != zero);
    }
}

/* A sum past the range, and a mouse delta of -32768, are held to the range: the call is not
   refused, so the picture is the library's and the buttons are fresh. */
static void dev_clamp_test(void)
{
    dev_run r = {0};
    r.d1 = r.d2 = COGMASTER_DEVICE_SPORTS_PAD;
    r.script = dev_motion;
    dev_kind = DEV_HUGE;
    dev_until = 20u;
    r.image = dev_image[DEV_SPORTS_IMAGE];
    r.frames = 20u;
    for (dev_sign = -1; dev_sign <= 1; dev_sign += 2) {
        dev_same("huge sums", &r);
        CHECK_EQ_U64(log_lines, 0u);
    }
    r.d1 = COGMASTER_DEVICE_PADDLE;
    r.d2 = COGMASTER_DEVICE_PAD;
    r.image = dev_image[DEV_PADDLE_IMAGE];
    r.frames = 40u;
    dev_until = 8u;
    for (dev_sign = -1; dev_sign <= 1; dev_sign += 2) {
        dev_same("huge sums on a paddle", &r);
        CHECK_EQ_U64(log_lines, 0u);
    }
    r.d1 = COGMASTER_DEVICE_SPORTS_PAD;
    r.d2 = COGMASTER_DEVICE_PAD;
    r.p1 = "sports";
    r.image = dev_image[DEV_CONTROL_IMAGE];
    r.frames = 6u;
    r.script = dev_buttons;
    for (dev_kind = 0; dev_kind < 4; dev_kind++) {
        dev_same("buttons beside -32768", &r);
        CHECK_EQ_U64(log_lines, 0u);
    }
}

/* The guest sees the motion: each source in each sign ends on a picture that is not the one the
   still pad gives, the signs end apart, and the same input ends on the same picture. */
static void dev_motion_test(void)
{
    dev_run r = {0};
    r.script = dev_motion;
    for (unsigned image = 0u; image < 2u; image++) {
        uint32_t still, plus, minus;
        r.image = dev_image[image];
        r.frames = image == 0u ? 20u : 40u;
        dev_until = image == 0u ? 20u : 8u;
        dev_kind = DEV_NONE;
        dev_sign = 1;
        still = dev_shown(&r);
        for (int kind = DEV_MOUSE; kind <= DEV_DPAD; kind++) {
            dev_kind = kind;
            dev_sign = 1;
            plus = dev_shown(&r);
            CHECK(dev_shown(&r) == plus);
            dev_sign = -1;
            minus = dev_shown(&r);
            CHECK(plus != still && minus != still && plus != minus);
        }
    }
}

/* Whether a flip of the shutter eye lies at frame f, counting from the first frame of the run
   since the last reset: an eye that is not the last non-zero eye before it. */
static int dev_flip(const uint32_t *eyes, unsigned f, unsigned epoch)
{
    if (f < epoch || eyes[f] == 0u)
        return 0;
    for (unsigned j = f; j-- > epoch;) {
        if (eyes[j] != 0u)
            return eyes[j] != eyes[f];
    }
    return 0;
}

/* The picture the view should present at frame n, from the library's own eyes and pictures: the
   chosen eye's latest picture, or the mix, while a flip lies in the last four frames; the raw
   frame otherwise. */
static uint16_t dev_model(const char *view, const uint32_t *eyes, const uint16_t *dots, unsigned n,
                          unsigned reset_at)
{
    unsigned epoch = reset_at != 0u && n >= reset_at ? reset_at : 0u;
    uint16_t left = 0u, right = 0u;
    int active = 0;
    for (unsigned f = n >= epoch + 3u ? n - 3u : epoch; f <= n; f++)
        active |= dev_flip(eyes, f, epoch);
    for (unsigned j = epoch; j <= n; j++) {
        left = eyes[j] == COGMASTER_GLASSES_LEFT ? dots[j] : left;
        right = eyes[j] == COGMASTER_GLASSES_RIGHT ? dots[j] : right;
    }
    if (!active || view == NULL || strcmp(view, "left") == 0)
        return active ? left : dots[n];
    if (strcmp(view, "right") == 0)
        return right;
    if (strcmp(view, "anaglyph") == 0)
        return (uint16_t)((left & 0x00Fu) | (right & 0x0FF0u));
    return dots[n];
}

#define DEV_STEREO_FRAMES 36u
static uint32_t dev_stereo_fold;

/* Runs the adapter in a view next to the library; every frame must be the model's. Returns how
   many frames the view changed from the raw one, and leaves what it presented in got. */
static unsigned dev_stereo_run(int image, const char *view, unsigned reset_at, uint16_t *got)
{
    static uint32_t eyes[DEV_STEREO_FRAMES];
    static uint16_t raw[DEV_STEREO_FRAMES];
    dev_run r = {0};
    unsigned bad = 0u, composed = 0u;
    r.image = dev_image[image];
    r.view = view;
    r.frames = DEV_STEREO_FRAMES;
    r.reset_at = reset_at;
    r.script = dev_clear;
    r.eyes = eyes;
    r.dots = raw;
    dev_reference(&r);
    dev_stereo_fold = 2166136261u;
    for (unsigned n = 0u; n < DEV_STEREO_FRAMES; n++)
        dev_stereo_fold = (dev_stereo_fold ^ raw[n]) * 16777619u;
    r.eyes = NULL;
    r.dots = got;
    dev_adapter(&r);
    for (unsigned n = 0u; n < DEV_STEREO_FRAMES; n++) {
        if (got[n] != dev_model(view, eyes, raw, n, reset_at)) {
            if (bad++ == 0u)
                fprintf(stderr, "view %s: frame %u shows %03X, expected %03X\n",
                        view != NULL ? view : "default", n + 1u, (unsigned)got[n],
                        (unsigned)dev_model(view, eyes, raw, n, reset_at));
        }
        composed += got[n] != raw[n];
    }
    CHECK_EQ_U64(bad, 0u);
    return composed;
}

static const char *const dev_views[5] = {"left", "right", "alternate", "anaglyph", "off"};

/* A guest that alternates the eye is shown through the chosen view; the library's frames do not
   change with the view; a reset starts the rule again; the fixture draws what the library draws. */
static void dev_alternate_test(void)
{
    static uint16_t got[5][DEV_STEREO_FRAMES], again[DEV_STEREO_FRAMES];
    unsigned composed[5];
    uint32_t fold = 0u;
    for (unsigned v = 0u; v < 5u; v++) {
        composed[v] = dev_stereo_run(DEV_ALT_IMAGE, dev_views[v], 0u, got[v]);
        CHECK(v == 0u || dev_stereo_fold == fold);
        fold = dev_stereo_fold;
    }
    CHECK(composed[0] > 0u && composed[1] > 0u && composed[3] > 0u);
    CHECK_EQ_U64(composed[2], 0u);
    CHECK_EQ_U64(composed[4], 0u);
    CHECK(memcmp(got[0], got[3], sizeof got[0]) != 0);
    CHECK(memcmp(got[0], got[2], sizeof got[0]) != 0);
    CHECK_EQ_U64(dev_stereo_run(DEV_ALT_IMAGE, NULL, 0u, again), composed[0]);
    CHECK(memcmp(got[0], again, sizeof again) == 0);
    for (unsigned at = 20u; at <= 21u; at++) {
        for (unsigned v = 0u; v < 2u; v++)
            CHECK(dev_stereo_run(DEV_ALT_IMAGE, dev_views[v], at, again) > 0u);
    }
    for (unsigned v = 0u; v < 5u; v++) {
        dev_run r = {0};
        r.image = dev_image[DEV_GLASSES_IMAGE];
        r.view = dev_views[v];
        r.frames = 30u;
        r.script = dev_clear;
        dev_same(dev_views[v], &r);
    }
}

/* A guest that sets one eye and keeps it is never held on a picture that does not arrive: every
   view shows the raw frames, while the alternating guest is composed. */
static void dev_one_eye_test(void)
{
    static uint16_t got[DEV_STEREO_FRAMES];
    for (unsigned v = 0u; v < 5u; v++)
        CHECK_EQ_U64(dev_stereo_run(DEV_ONCE_IMAGE, dev_views[v], 0u, got), 0u);
    CHECK_EQ_U64(dev_stereo_run(DEV_ONCE_IMAGE, NULL, 0u, got), 0u);
    CHECK(dev_stereo_run(DEV_ALT_IMAGE, "left", 0u, got) > 0u);
}

static size_t dev_read(const char *dir, const char *name, uint8_t *image)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    size_t n = 0u;
    if (f == NULL) {
        fprintf(stderr, "cannot open %s\n", path);
        check_failures++;
        return 0u;
    }
    n = fread(image, 1u, 32768u, f);
    CHECK_EQ_U64(n, 32768u);
    (void)fclose(f);
    return n;
}

static int adapter_test(const char *name, const char *dir)
{
    dev_read(dir, "sports_pad.sms", dev_image[DEV_SPORTS_IMAGE]);
    dev_read(dir, "paddle.sms", dev_image[DEV_PADDLE_IMAGE]);
    dev_read(dir, "glasses.sms", dev_image[DEV_GLASSES_IMAGE]);
    control_image(dev_image[DEV_CONTROL_IMAGE], 1u, 0xDCu, 0u);
    dev_stereo_image(dev_image[DEV_ALT_IMAGE], 1);
    dev_stereo_image(dev_image[DEV_ONCE_IMAGE], 0);
    if (strcmp(name, "options") == 0)
        dev_options_test();
    else if (strcmp(name, "sources") == 0)
        dev_sources_test();
    else if (strcmp(name, "clamp") == 0)
        dev_clamp_test();
    else if (strcmp(name, "motion") == 0)
        dev_motion_test();
    else if (strcmp(name, "alternate") == 0)
        dev_alternate_test();
    else if (strcmp(name, "one_eye") == 0)
        dev_one_eye_test();
    else
        return 2;
    return check_failures != 0 ? 1 : 0;
}
