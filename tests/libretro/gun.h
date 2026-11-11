/* The Light Phaser through the adapter, on its own fake frontend so that the shared one in
   controls.h keeps serving a control pad only. The guest is assembled here: a white square on
   black, and a small patch in the top left corner whose colour is what the guest saw in the last
   frame, a bit each for the sensor, the trigger and a Pause, so the picture alone tells it. Nothing
   reads a ROM file. */
#include "aim.h"

#define GUN_W 256u
#define GUN_H 192u
#define GUN_DOTS (GUN_W * GUN_H)
/* The lit square, in dots from the top left of the field, and the patch's probe dot. */
#define GUN_SQ_X 96u
#define GUN_SQ_Y 40u
#define GUN_SQ_SIZE 16u
#define GUN_PROBE (4u * GUN_W + 4u)

static struct {
    int16_t sx[2], sy[2];
    int off[2], trig[2], start[2], select[2];
    uint32_t pad[2];
    int16_t mouse[2][2], stick[2][2];
    uint32_t click[2];
} gun_in;
static unsigned gun_asked[2][2][32]; /* per port: [0] control pad ids, [1] light gun ids */

/* What a frontend serves for a port that carries a light gun or a control pad. */
static int16_t RETRO_CALLCONV gun_input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port >= 2u || id >= 32u) {
        CHECK(0);
        return 0;
    }
    if (device == RETRO_DEVICE_LIGHTGUN && index == 0u) {
        gun_asked[port][1][id]++;
        switch (id) {
        case RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X:
            return gun_in.sx[port];
        case RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y:
            return gun_in.sy[port];
        case RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN:
            return (int16_t)gun_in.off[port];
        case RETRO_DEVICE_ID_LIGHTGUN_TRIGGER:
            return (int16_t)gun_in.trig[port];
        case RETRO_DEVICE_ID_LIGHTGUN_START:
            return (int16_t)gun_in.start[port];
        case RETRO_DEVICE_ID_LIGHTGUN_SELECT:
            return (int16_t)gun_in.select[port];
        default:
            return 0;
        }
    }
    if (device == RETRO_DEVICE_JOYPAD && index == 0u && id < 16u) {
        gun_asked[port][0][id]++;
        return (int16_t)((gun_in.pad[port] >> id) & 1u);
    }
    if (device == RETRO_DEVICE_MOUSE && index == 0u && id < 4u) {
        return id < 2u ? gun_in.mouse[port][id]
                       : (int16_t)((gun_in.click[port] >> (id - RETRO_DEVICE_ID_MOUSE_LEFT)) & 1u);
    }
    if (device == RETRO_DEVICE_ANALOG && index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id < 2u) {
        return gun_in.stick[port][id];
    }
    return 0;
}

/* ---- the guest ---- */

static uint8_t *gun_mem;
static size_t gun_at;

static void gun_put(const uint8_t *bytes, size_t n)
{
    memcpy(gun_mem + gun_at, bytes, n);
    gun_at += n;
}

#define GUN_PUT(...)                                                                               \
    do {                                                                                           \
        const uint8_t bytes_[] = {__VA_ARGS__};                                                    \
        gun_put(bytes_, sizeof bytes_);                                                            \
    } while (0)

/* Sets a VDP register: LD A,v; OUT ($BF),A; LD A,r|$80; OUT ($BF),A. */
#define GUN_REG(r, v) GUN_PUT(0x3E, v, 0xD3, 0xBF, 0x3E, (r) | 0x80, 0xD3, 0xBF)
/* Points the VDP at an address for writing: VRAM, or colour RAM with code $C0. */
#define GUN_ADDR(lo, hi) GUN_PUT(0x3E, lo, 0xD3, 0xBF, 0x3E, hi, 0xD3, 0xBF)
/* Two name table entries of one tile each, with plain attributes. */
#define GUN_PAIR(tile)                                                                             \
    GUN_PUT(0x3E, tile, 0xD3, 0xBE, 0xAF, 0xD3, 0xBE, 0x3E, tile, 0xD3, 0xBE, 0xAF, 0xD3, 0xBE)

/* Sets up Mode 4 with a 16 by 16 white square at the SQ position, a black field and a 16 by 16
   patch of the third palette entry in the top left corner; then, once per frame, at the start of
   the blank, writes the third palette entry as 9 x (sensor bit + 2 x trigger bit + 4 x Pause bit)
   and clears the bits. The sensor and the trigger are read from $DD bit 6 and $DC bit 4, which read
   low while the gun sees light or the trigger is held. Pause is the NMI. */
static void gun_image(uint8_t *image)
{
    size_t loop;
    memset(image, 0, 32768u);
    gun_mem = image;
    gun_at = 0u;
    GUN_PUT(0xC3, 0x80, 0x00); /* jp $0080 */
    gun_at = 0x66u;
    GUN_PUT(0xF5, 0x3E, 0x01, 0x32, 0x00, 0xC0, 0xF1, 0xED, 0x45); /* push af; ld a,1; ld ($C000),a;
                                                                       pop af; retn */
    gun_at = 0x80u;
    GUN_PUT(0xF3, 0x31, 0xF0, 0xDF); /* di; ld sp,$DFF0 */
    GUN_PUT(0x3E, 0xFF, 0xD3, 0x3F); /* both TH lines inputs */
    GUN_REG(0, 0x04);
    GUN_REG(1, 0x40);
    GUN_REG(2, 0xFF);
    GUN_REG(5, 0xFF);
    GUN_REG(6, 0xFF);
    GUN_ADDR(0x00, 0xC0); /* colour RAM from 0: black, white, black */
    GUN_PUT(0xAF, 0xD3, 0xBE, 0x3E, 0x3F, 0xD3, 0xBE, 0xAF, 0xD3, 0xBE);
    GUN_ADDR(0x20, 0x40); /* tile 1, colour 1 throughout, then tile 2, colour 2 */
    GUN_PUT(0x06, 0x08, 0x3E, 0xFF, 0xD3, 0xBE, 0xAF, 0xD3, 0xBE, 0xD3, 0xBE, 0xD3, 0xBE, 0x10,
            0xF3);
    GUN_PUT(0x06, 0x08, 0xAF, 0xD3, 0xBE, 0x3E, 0xFF, 0xD3, 0xBE, 0xAF, 0xD3, 0xBE, 0xD3, 0xBE,
            0x10, 0xF2);
    GUN_ADDR(0x58, 0x79); /* name table row 5, column 12: tile row of the square */
    GUN_PAIR(0x01);
    GUN_ADDR(0x98, 0x79);
    GUN_PAIR(0x01);
    GUN_ADDR(0x00, 0x78); /* the patch */
    GUN_PAIR(0x02);
    GUN_ADDR(0x40, 0x78);
    GUN_PAIR(0x02);
    GUN_ADDR(0x00, 0x7F); /* no sprites */
    GUN_PUT(0x3E, 0xD0, 0xD3, 0xBE);
    GUN_PUT(0x16, 0x00, 0x1E, 0x00, 0xAF, 0x32, 0x00,
            0xC0); /* ld d,0; ld e,0; clear the Pause bit */
    loop = gun_at;
    /* in a,($DD); and $40; jr nz,+2; ld d,1; in a,($DC); and $10; jr nz,+2; ld e,1 */
    GUN_PUT(0xDB, 0xDD, 0xE6, 0x40, 0x20, 0x02, 0x16, 0x01, 0xDB, 0xDC, 0xE6, 0x10, 0x20, 0x02,
            0x1E, 0x01);
    GUN_PUT(0xDB, 0xBF, 0x07, 0x30); /* in a,($BF); rlca; jr nc,loop */
    GUN_PUT((uint8_t)(loop - (gun_at + 1u)));
    /* ld a,($C000); add a,a twice; add a,e twice; add a,d; ld b,a; add a,a three times; add a,b */
    GUN_PUT(0x3A, 0x00, 0xC0, 0x87, 0x87, 0x83, 0x83, 0x82, 0x47, 0x87, 0x87, 0x87, 0x80);
    GUN_PUT(0x4F); /* ld c,a */
    GUN_ADDR(0x02, 0xC0);
    GUN_PUT(0x79, 0xD3, 0xBE);                               /* ld a,c; out ($BE),a */
    GUN_PUT(0x16, 0x00, 0x1E, 0x00, 0xAF, 0x32, 0x00, 0xC0); /* clear the bits */
    GUN_PUT(0x18);                                           /* jr loop */
    GUN_PUT((uint8_t)(loop - (gun_at + 1u)));
    memcpy(image + 0x7FF0u, "TMR SEGA", 8u);
    image[0x7FFFu] = 0x4Cu;
}

/* ---- the frontend run ---- */

static const uint16_t gun_colours[4] = {0x0FFF, 0x000F, 0x00F0, 0x00FF};
static const char *const gun_names[4] = {"white", "red", "green", "yellow"};

/* The RGB444 dot of a 6-bit colour RAM value, as controls.h reads it. */
static uint16_t gun_rgb(unsigned c)
{
    return (uint16_t)(((c & 3u) * 5u) | (((c >> 2) & 3u) * 5u << 4) | ((c >> 4) * 5u << 8));
}

static void gun_load(const char *p1, const char *p2, const char *cross, const char *colour,
                     unsigned dev0, unsigned dev1)
{
    static uint8_t image[32768];
    struct retro_game_info game = {0};
    gun_image(image);
    option_machine = "sms2";
    option_mode = "auto";
    option_view = "native";
    option_region = "ntsc-u";
    option_aspect = "6:5";
    option_fm = option_3d = NULL;
    option_port1 = p1;
    option_port2 = p2;
    option_crosshair = cross;
    option_colour = colour;
    want_w = GUN_W;
    want_h = GUN_H;
    memset(&gun_in, 0, sizeof gun_in);
    memset(gun_asked, 0, sizeof gun_asked);
    log_lines = 0u;
    p_set_environment(env);
    p_init();
    set_callbacks();
    p_set_input_state(gun_input);
    p_set_controller_port_device(0u, dev0);
    p_set_controller_port_device(1u, dev1);
    game.data = image;
    game.size = sizeof image;
    CHECK(p_load_game(&game));
}

static void gun_unload(void)
{
    CHECK_EQ_U64(video_bad, 0u);
    p_unload_game();
    p_deinit();
}

static void gun_frames(unsigned n)
{
    for (unsigned i = 0u; i < n; i++) {
        (void)run_one();
    }
}

/* What the patch says the guest saw last frame: 1 sensor, 2 trigger, 4 Pause; -1 for a colour no
   such sum draws. */
static int gun_patch(void)
{
    uint16_t got;
    if (!to_rgb444(GUN_PROBE, &got)) {
        return -1;
    }
    for (unsigned v = 0u; v < 8u; v++) {
        if (got == gun_rgb(v * 9u)) {
            return (int)v;
        }
    }
    return -1;
}

/* The pointer value that maps to a dot of a field axis of size: the middle of the dot's interval,
   checked against the definition (u = s + 32768, dot = u * size >> 16). */
static int16_t gun_screen(uint32_t dot, uint32_t size)
{
    uint32_t u = (dot * 65536u + 32768u) / size;
    CHECK(((u * size) >> 16) == dot);
    CHECK(u >= 1u && u <= 65535u);
    return (int16_t)((int32_t)u - 32768);
}

static void gun_aim_at(unsigned port, uint32_t x, uint32_t y)
{
    struct cogmaster__aim_rect r = {0u, 0u, GUN_W, GUN_H};
    uint32_t ax = 0u, ay = 0u;
    gun_in.sx[port] = gun_screen(x, GUN_W);
    gun_in.sy[port] = gun_screen(y, GUN_H);
    gun_in.off[port] = 0;
    cogmaster__aim_from_screen(&r, gun_in.sx[port], gun_in.sy[port], 0, &ax, &ay);
    CHECK_EQ_U64(ax, x + 1u);
    CHECK_EQ_U64(ay, y + 1u);
}

/* After frames frames of the current input, what the patch says. */
static int gun_see(unsigned frames)
{
    gun_frames(frames);
    return gun_patch();
}

/* ---- gun.aim ---- */

static void gun_aim_test(void)
{
    const uint32_t cx = GUN_SQ_X + GUN_SQ_SIZE / 2u, cy = GUN_SQ_Y + GUN_SQ_SIZE / 2u;
    static const struct {
        uint32_t x, y;
        int sees;
    } spots[] = {{GUN_SQ_X + 8u, GUN_SQ_Y + 15u, 1}, {GUN_SQ_X + 8u, GUN_SQ_Y, 1},
                 {GUN_SQ_X, GUN_SQ_Y + 8u, 1},       {GUN_SQ_X + 15u, GUN_SQ_Y + 8u, 1},
                 {GUN_SQ_X + 8u, GUN_SQ_Y + 24u, 0}, {GUN_SQ_X + 8u, GUN_SQ_Y - 9u, 0},
                 {GUN_SQ_X - 9u, GUN_SQ_Y + 8u, 0},  {GUN_SQ_X - 16u, GUN_SQ_Y + 8u, 0},
                 {GUN_SQ_X + 25u, GUN_SQ_Y + 8u, 0}};
    gun_load("lphaser", "pad", "off", NULL, RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_JOYPAD);
    /* A parked gun sees nothing and pulls nothing. */
    gun_in.off[0] = 1;
    gun_in.sx[0] = gun_in.sy[0] = -32768;
    CHECK_EQ_U64(gun_see(4u), 0);
    /* Aimed at the square it sees light, with no trigger; the trigger shows on its own. */
    gun_aim_at(0u, cx, cy);
    CHECK_EQ_U64(gun_see(4u), 1);
    gun_in.trig[0] = 1;
    CHECK_EQ_U64(gun_see(4u), 3);
    gun_in.trig[0] = 0;
    CHECK_EQ_U64(gun_see(4u), 1);
    for (unsigned i = 0u; i < sizeof spots / sizeof spots[0]; i++) {
        gun_aim_at(0u, spots[i].x, spots[i].y);
        gun_in.trig[0] = (int)(i & 1u);
        CHECK_EQ_U64(gun_see(4u), spots[i].sees + 2 * (int)(i & 1u));
    }
    /* Sixteen dots off the square, the trigger alone. */
    gun_aim_at(0u, GUN_SQ_X - 16u, cy);
    gun_in.trig[0] = 1;
    CHECK_EQ_U64(gun_see(4u), 2);
    gun_in.trig[0] = 0;
    /* An off-screen flag with the pointer on the square, and either coordinate at -32768, is no
       aim: never clamped onto an edge dot, never a sighting. */
    gun_aim_at(0u, cx, cy);
    gun_in.off[0] = 1;
    CHECK_EQ_U64(gun_see(4u), 0);
    gun_in.off[0] = 0;
    gun_in.sx[0] = -32768;
    CHECK_EQ_U64(gun_see(4u), 0);
    gun_aim_at(0u, cx, cy);
    gun_in.sy[0] = -32768;
    CHECK_EQ_U64(gun_see(4u), 0);
    gun_aim_at(0u, cx, cy);
    CHECK_EQ_U64(gun_see(4u), 1);
    /* The reload id is not mapped, and a control pad was never asked about the gun's port. */
    CHECK_EQ_U64(gun_asked[0][1][RETRO_DEVICE_ID_LIGHTGUN_RELOAD], 0u);
    CHECK_EQ_U64(gun_asked[0][0][RETRO_DEVICE_ID_JOYPAD_B], 0u);
    gun_unload();
    /* A port whose option is not the phaser ignores the frontend's gun: the pad gets nothing. */
    gun_load("pad", "pad", "off", NULL, RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_JOYPAD);
    gun_aim_at(0u, cx, cy);
    gun_in.trig[0] = 1;
    CHECK_EQ_U64(gun_see(4u), 0);
    gun_unload();
}

/* ---- gun.cursor ---- */

static int32_t gun_cx, gun_cy; /* the model's cursor, 0-based dots */

static int32_t gun_clamp(int32_t v, int32_t hi)
{
    return v < 0 ? 0 : (v > hi ? hi : v);
}

/* The dots of the cross the model gives for a centre, 17 inside the field: marks[] is 1 there. */
static unsigned gun_cross(int32_t cx, int32_t cy, uint8_t *marks)
{
    unsigned n = 0u;
    memset(marks, 0, GUN_DOTS);
    for (int32_t k = -4; k <= 4; k++) {
        for (int arm = 0; arm < 2; arm++) {
            int32_t x = arm == 0 ? cx + k : cx;
            int32_t y = arm == 0 ? cy : cy + k;
            if (x >= 0 && x < (int32_t)GUN_W && y >= 0 && y < (int32_t)GUN_H &&
                marks[(uint32_t)y * GUN_W + (uint32_t)x] == 0u) {
                marks[(uint32_t)y * GUN_W + (uint32_t)x] = 1u;
                n++;
            }
        }
    }
    return n;
}

static uint8_t gun_marks[GUN_DOTS];
static uint16_t gun_pic[GUN_DOTS];

static void gun_grab(uint16_t *pic)
{
    for (uint32_t i = 0u; i < GUN_DOTS; i++) {
        CHECK(to_rgb444(i, &pic[i]));
    }
}

/* Runs one frame and checks the presented picture holds the cross of colour exactly where the
   model's cursor is, and nowhere else. */
static void gun_cursor_frame(uint16_t colour)
{
    int32_t dx, dy;
    uint32_t bad = 0u;
    dx = dev_sum_by(gun_in.mouse[0][0], gun_in.stick[0][0],
                    (int32_t)((gun_in.pad[0] >> RETRO_DEVICE_ID_JOYPAD_RIGHT) & 1u) -
                        (int32_t)((gun_in.pad[0] >> RETRO_DEVICE_ID_JOYPAD_LEFT) & 1u),
                    4, 1);
    dy = dev_sum_by(gun_in.mouse[0][1], gun_in.stick[0][1],
                    (int32_t)((gun_in.pad[0] >> RETRO_DEVICE_ID_JOYPAD_DOWN) & 1u) -
                        (int32_t)((gun_in.pad[0] >> RETRO_DEVICE_ID_JOYPAD_UP) & 1u),
                    4, 1);
    gun_cx = gun_clamp(gun_cx + dx, (int32_t)GUN_W - 1);
    gun_cy = gun_clamp(gun_cy + dy, (int32_t)GUN_H - 1);
    gun_frames(1u);
    gun_grab(gun_pic);
    (void)gun_cross(gun_cx, gun_cy, gun_marks);
    for (uint32_t i = 0u; i < GUN_DOTS; i++) {
        bad += (gun_pic[i] == colour) != (gun_marks[i] != 0u) ? 1u : 0u;
    }
    if (bad != 0u) {
        fprintf(stderr, "cursor model (%d,%d): %u dots differ from the expected cross\n",
                (int)gun_cx, (int)gun_cy, (unsigned)bad);
    }
    CHECK_EQ_U64(bad, 0u);
}

static void gun_pad_hold(unsigned id)
{
    gun_in.pad[0] = 1u << id;
}

static void gun_cursor_test(void)
{
    const uint16_t red = gun_colours[1];
    gun_load("lphaser", "pad", "on", "red", RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD);
    /* Centred at first. */
    gun_cx = (int32_t)GUN_W / 2;
    gun_cy = (int32_t)GUN_H / 2;
    gun_cursor_frame(red);
    /* The D-pad moves it 2 a frame, each way. */
    for (unsigned i = 0u; i < 6u; i++) {
        gun_pad_hold(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        gun_cursor_frame(red);
    }
    for (unsigned i = 0u; i < 4u; i++) {
        gun_pad_hold(RETRO_DEVICE_ID_JOYPAD_UP);
        gun_cursor_frame(red);
    }
    gun_in.pad[0] = 0u;
    /* The stick: past the dead zone it moves 4 counts at full deflection, inside it nothing. The
       model must say 4 both ways, or it would follow the adapter's cut. */
    CHECK(dev_sum_by(0, 32767, 0, 4, 1) == 4 && dev_sum_by(0, -32768, 0, 4, 1) == -4);
    gun_in.stick[0][0] = 6553;
    gun_cursor_frame(red);
    gun_in.stick[0][0] = 32767;
    gun_in.stick[0][1] = -32768;
    {
        int32_t was_x = gun_cx, was_y = gun_cy;
        gun_cursor_frame(red);
        CHECK(gun_cx - was_x == 4 && was_y - gun_cy == 4);
    }
    for (unsigned i = 0u; i < 4u; i++) {
        gun_cursor_frame(red);
    }
    gun_in.stick[0][0] = gun_in.stick[0][1] = 0;
    /* The mouse delta, and all three sources at once. */
    gun_in.mouse[0][0] = -30;
    gun_in.mouse[0][1] = 17;
    gun_cursor_frame(red);
    gun_in.stick[0][0] = 20000;
    gun_in.stick[0][1] = 20000;
    gun_pad_hold(RETRO_DEVICE_ID_JOYPAD_LEFT);
    gun_cursor_frame(red);
    gun_in.stick[0][0] = gun_in.stick[0][1] = 0;
    gun_in.pad[0] = 0u;
    /* The sum is held at the picture's edge, in each corner, and the cross is clipped there. */
    static const int16_t corner[4][2] = {
        {32767, 32767}, {-32768, 32767}, {-32768, -32768}, {32767, -32768}};
    for (unsigned i = 0u; i < 4u; i++) {
        gun_in.mouse[0][0] = corner[i][0];
        gun_in.mouse[0][1] = corner[i][1];
        gun_cursor_frame(red);
        gun_cursor_frame(red);
    }
    memset(&gun_in, 0, sizeof gun_in);
    /* A reset puts the cursor back in the centre. */
    gun_cursor_frame(red);
    CHECK(gun_cx != (int32_t)GUN_W / 2 || gun_cy != (int32_t)GUN_H / 2);
    p_reset();
    gun_cx = (int32_t)GUN_W / 2;
    gun_cy = (int32_t)GUN_H / 2;
    gun_cursor_frame(red);
    /* The trigger is RetroPad B or A or the left mouse button, and reaches the guest's TL. */
    CHECK_EQ_U64(gun_see(3u), 0);
    gun_pad_hold(RETRO_DEVICE_ID_JOYPAD_B);
    CHECK_EQ_U64(gun_see(3u), 2);
    gun_pad_hold(RETRO_DEVICE_ID_JOYPAD_A);
    CHECK_EQ_U64(gun_see(3u), 2);
    gun_in.pad[0] = 0u;
    gun_in.click[0] = 1u;
    CHECK_EQ_U64(gun_see(3u), 2);
    gun_in.click[0] = 2u;
    CHECK_EQ_U64(gun_see(3u), 0);
    gun_in.click[0] = 0u;
    CHECK_EQ_U64(gun_see(3u), 0);
    /* No gun id was asked of a port that carries a control pad. */
    for (unsigned id = 0u; id < 32u; id++) {
        CHECK_EQ_U64(gun_asked[0][1][id], 0u);
    }
    gun_unload();

    /* A frontend gun on port 0 keeps Pause and Reset: Start and Select of the gun are read, and
       Pause reaches the guest's NMI exactly once for one press. */
    gun_load("lphaser", "pad", "off", NULL, RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_JOYPAD);
    gun_in.off[0] = 1;
    unsigned nmis = 0u;
    for (unsigned f = 0u; f < 12u; f++) {
        gun_in.start[0] = f == 4u || f == 5u;
        gun_frames(1u);
        nmis += gun_patch() >= 4 ? 1u : 0u;
    }
    CHECK_EQ_U64(nmis, 1u);
    CHECK(gun_asked[0][1][RETRO_DEVICE_ID_LIGHTGUN_START] > 0u);
    CHECK(gun_asked[0][1][RETRO_DEVICE_ID_LIGHTGUN_SELECT] > 0u);
    /* The control: no press, no Pause. */
    gun_in.start[0] = 0;
    nmis = 0u;
    for (unsigned f = 0u; f < 6u; f++) {
        gun_frames(1u);
        nmis += gun_patch() >= 4 ? 1u : 0u;
    }
    CHECK_EQ_U64(nmis, 0u);
    gun_unload();
}

/* ---- gun.crosshair ---- */

/* One stretch of frames at fixed aims: the dot each gun points at, or off screen. */
typedef struct {
    unsigned frames;
    uint32_t dot[2][2];
    int off[2];
} gun_phase;

#define GUN_PHASES 3u
static uint16_t gun_got[GUN_PHASES][GUN_DOTS], gun_ref[GUN_PHASES][GUN_DOTS];

/* The public library alone, driven as the adapter should: the aims are the dots plus one. */
static void gun_reference(uint32_t d1, uint32_t d2, const gun_phase *ph, unsigned n)
{
    cogmaster_config cfg = {0};
    cogmaster *lib = NULL;
    cogmaster_media media = {0};
    static uint8_t image[32768];
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = presentation_alloc;
    cfg.allocator.free = presentation_free;
    CHECK_EQ_U64(cogmaster_create(&cfg, &lib), COGMASTER_STATUS_OK);
    gun_image(image);
    media.size = (uint32_t)sizeof media;
    media.machine = COGMASTER_MACHINE_SMS2;
    media.region = COGMASTER_REGION_EXPORT;
    media.tv = COGMASTER_TV_NTSC;
    media.image = image;
    media.image_size = sizeof image;
    CHECK_EQ_U64(cogmaster_load(lib, &media), COGMASTER_STATUS_OK);
    for (unsigned p = 0u; p < n; p++) {
        for (unsigned f = 0u; f < ph[p].frames; f++) {
            cogmaster_input in = {0};
            cogmaster_run_result res = {0};
            cogmaster_frame io = {0};
            in.size = (uint32_t)sizeof in;
            in.port1 = d1;
            in.port2 = d2;
            in.x1 = ph[p].off[0] ? 0 : (int32_t)ph[p].dot[0][0] + 1;
            in.y1 = ph[p].off[0] ? 0 : (int32_t)ph[p].dot[0][1] + 1;
            in.x2 = ph[p].off[1] ? 0 : (int32_t)ph[p].dot[1][0] + 1;
            in.y2 = ph[p].off[1] ? 0 : (int32_t)ph[p].dot[1][1] + 1;
            CHECK_EQ_U64(cogmaster_set_input(lib, &in), COGMASTER_STATUS_OK);
            res.size = (uint32_t)sizeof res;
            CHECK_EQ_U64(cogmaster_run_frame(lib, &res), COGMASTER_STATUS_OK);
            io.size = (uint32_t)sizeof io;
            io.pixels = dev_native;
            io.pitch_bytes = MAX_W * 2u;
            CHECK_EQ_U64(cogmaster_get_frame(lib, &io), COGMASTER_STATUS_OK);
            CHECK(io.w == GUN_W && io.h == GUN_H);
        }
        for (uint32_t row = 0u; row < GUN_H; row++) {
            memcpy(&gun_ref[p][row * GUN_W], &dev_native[row * MAX_W],
                   GUN_W * sizeof dev_native[0]);
        }
    }
    cogmaster_destroy(lib);
}

/* The adapter through the same phases, the pointers of the fake frontend set from the dots. */
static void gun_presented(const gun_phase *ph, unsigned n)
{
    for (unsigned p = 0u; p < n; p++) {
        for (unsigned port = 0u; port < 2u; port++) {
            if (ph[p].off[port]) {
                gun_in.off[port] = 1;
                gun_in.sx[port] = gun_in.sy[port] = -32768;
            } else {
                gun_aim_at(port, ph[p].dot[port][0], ph[p].dot[port][1]);
            }
        }
        gun_frames(ph[p].frames);
        gun_grab(gun_got[p]);
    }
}

/* The picture got differs from ref in exactly the dots of the crosses at the given centres, each
   of colour there, and nowhere else. A centre of -1 is no cross. */
static void gun_expect_cross(const uint16_t *got, const uint16_t *ref, uint16_t colour,
                             int32_t centre[2][2])
{
    static uint8_t any[GUN_DOTS];
    unsigned crosses = 0u, same = 0u, bad = 0u;
    memset(any, 0, sizeof any);
    for (unsigned g = 0u; g < 2u; g++) {
        if (centre[g][0] >= 0) {
            crosses++;
            (void)gun_cross(centre[g][0], centre[g][1], gun_marks);
            for (uint32_t i = 0u; i < GUN_DOTS; i++) {
                any[i] = (uint8_t)(any[i] | gun_marks[i]);
            }
        }
    }
    for (uint32_t i = 0u; i < GUN_DOTS; i++) {
        if (any[i] != 0u) {
            bad += got[i] != colour ? 1u : 0u;
        } else {
            bad += got[i] != ref[i] ? 1u : 0u;
        }
        same += any[i] != 0u && ref[i] == colour ? 1u : 0u;
    }
    if (bad != 0u) {
        fprintf(stderr, "crosshair: %u dots are not the library's picture or the cross\n", bad);
    }
    CHECK_EQ_U64(bad, 0u);
    CHECK_EQ_U64(same, 0u);
    CHECK(crosses <= 2u);
}

static void gun_crosshair_test(void)
{
    static const gun_phase single[GUN_PHASES] = {{5u, {{160u, 100u}, {0u, 0u}}, {0, 1}},
                                                 {5u, {{60u, 150u}, {0u, 0u}}, {0, 1}},
                                                 {5u, {{60u, 150u}, {0u, 0u}}, {1, 1}}};
    static const gun_phase pair[GUN_PHASES] = {{5u, {{160u, 100u}, {60u, 150u}}, {0, 0}},
                                               {5u, {{200u, 20u}, {240u, 180u}}, {0, 0}},
                                               {5u, {{200u, 20u}, {240u, 180u}}, {1, 1}}};
    static int32_t none[2][2] = {{-1, -1}, {-1, -1}};
    /* The library's pictures for the three set-ups. */
    static uint16_t ref_single[GUN_PHASES][GUN_DOTS], ref_pair[GUN_PHASES][GUN_DOTS];
    gun_reference(COGMASTER_DEVICE_LIGHT_PHASER, COGMASTER_DEVICE_PAD, single, GUN_PHASES);
    memcpy(ref_single, gun_ref, sizeof ref_single);
    gun_reference(COGMASTER_DEVICE_LIGHT_PHASER, COGMASTER_DEVICE_LIGHT_PHASER, pair, GUN_PHASES);
    memcpy(ref_pair, gun_ref, sizeof ref_pair);

    /* Off: every presented picture is the library's, byte for byte. */
    gun_load("lphaser", "pad", "off", NULL, RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_JOYPAD);
    gun_presented(single, GUN_PHASES);
    for (unsigned p = 0u; p < GUN_PHASES; p++) {
        CHECK(memcmp(gun_got[p], ref_single[p], sizeof gun_got[p]) == 0);
    }
    gun_unload();

    /* On, in each colour: the cross is 17 dots at the aim in the native layout's colour; moving
       the aim leaves nothing of the old cross; an off-screen aim draws none. */
    for (unsigned c = 0u; c < 4u; c++) {
        int32_t at[2][2] = {{160, 100}, {-1, -1}};
        gun_load("lphaser", "pad", "on", gun_names[c], RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_JOYPAD);
        gun_presented(single, GUN_PHASES);
        gun_expect_cross(gun_got[0], ref_single[0], gun_colours[c], at);
        at[0][0] = 60;
        at[0][1] = 150;
        gun_expect_cross(gun_got[1], ref_single[1], gun_colours[c], at);
        gun_expect_cross(gun_got[2], ref_single[2], gun_colours[c], none);
        gun_unload();
    }

    /* Two guns: both are drawn. */
    {
        int32_t at[2][2] = {{160, 100}, {60, 150}};
        gun_load("lphaser", "lphaser", "on", "green", RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_LIGHTGUN);
        gun_presented(pair, GUN_PHASES);
        gun_expect_cross(gun_got[0], ref_pair[0], gun_colours[2], at);
        at[0][0] = 200;
        at[0][1] = 20;
        at[1][0] = 240;
        at[1][1] = 180;
        gun_expect_cross(gun_got[1], ref_pair[1], gun_colours[2], at);
        gun_expect_cross(gun_got[2], ref_pair[2], gun_colours[2], none);
        gun_unload();
    }

    /* No phaser on either port: the picture is the library's with the option on, for a pad. */
    {
        static uint16_t ref_pad[GUN_PHASES][GUN_DOTS];
        gun_reference(COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, single, GUN_PHASES);
        memcpy(ref_pad, gun_ref, sizeof ref_pad);
        gun_load("pad", "pad", "on", "red", RETRO_DEVICE_JOYPAD, RETRO_DEVICE_JOYPAD);
        gun_presented(single, GUN_PHASES);
        for (unsigned p = 0u; p < GUN_PHASES; p++) {
            CHECK(memcmp(gun_got[p], ref_pad[p], sizeof gun_got[p]) == 0);
        }
        gun_unload();
    }
}

static int gun_named(const char *name)
{
    return strcmp(name, "aim") == 0 || strcmp(name, "cursor") == 0 ||
           strcmp(name, "crosshair") == 0;
}

static int gun_test(const char *name)
{
    if (strcmp(name, "aim") == 0)
        gun_aim_test();
    else if (strcmp(name, "cursor") == 0)
        gun_cursor_test();
    else
        gun_crosshair_test();
    return check_failures != 0 ? 1 : 0;
}
