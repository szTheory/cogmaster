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

static void control_load(uint32_t profile, uint32_t source, unsigned rotate)
{
    static uint8_t image[32768];
    memset(image, 0, sizeof image);
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
