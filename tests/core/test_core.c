/* The core library through its public header: cogmaster-test-core <case>.
   Expected test-card values follow the card's rules as docs/ describes them;
   the pinned hashes are checked through the runner. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define FIELD_W 256u
#define FIELD_MAX_H 240u
#define POISON 0xA5u  /* fill the core must leave alone */
#define TAG_BUF 8192u /* bytes in the size-tag scratch buffer */
#define TAG_MAX 4096u /* the largest input size tag the core accepts */

/* The values the public header gives the Game Gear machine, the full-field
   frame flag and the rejected-profile status. */
#define TEST_MACHINE_GG 2u
#define TEST_FRAME_FULL_FIELD 1u
#define TEST_STATUS_ERR_PROFILE_REJECTED 6u

/* The card's colours, RGB444 0000BBBBGGGGRRRR. */
#define WHITE 0x0FFFu
#define BLACK 0x0000u
#define RED 0x000Fu
#define GREEN 0x00F0u
#define BLUE 0x0F00u
#define MAGENTA 0x0F0Fu

/* The Game Gear window inside the 256x192 field. */
#define GG_X 48u
#define GG_Y 24u
#define GG_W 160u
#define GG_H 144u

static uint16_t pixels[FIELD_W * FIELD_MAX_H];
static uint16_t pixels2[FIELD_W * FIELD_MAX_H];
static uint64_t tag_buf[TAG_BUF / 8u]; /* aligned for every public struct */
static cogmaster *shared;              /* the instance the size-tag calls use */

/* The plain malloc allocator, as every host defines it. */
static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static cogmaster_config make_config(void)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    return cfg;
}

static cogmaster_media make_media(uint32_t machine)
{
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = machine;
    return media;
}

/* A new instance with the test card of machine loaded. */
static cogmaster *make(uint32_t machine)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = make_media(machine);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

/* A frame request for buf at pitch bytes per row, after a memset. */
static cogmaster_frame frame_io(uint16_t *buf, uint32_t pitch, uint32_t flags)
{
    cogmaster_frame io;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pixels = buf;
    io.pitch_bytes = pitch;
    io.flags = flags;
    return io;
}

static void run_frames(cogmaster *m, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
}

static void expect_status(const char *what, uint32_t size, cogmaster_status got,
                          cogmaster_status want)
{
    if (got != want) {
        fprintf(stderr, "%s with size %lu: status %lu, expected %lu\n", what, (unsigned long)size,
                (unsigned long)got, (unsigned long)want);
        check_failures++;
    }
}

/* Frame 1 and frame 60 of the SMS card are equal, and every crop pixel is
   the card's pixel function at that field position. */
static void test_static(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    run_frames(m, 1);
    cogmaster_frame io = frame_io(pixels, FIELD_W * 2u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(io.x, 0);
    CHECK_EQ_U64(io.y, 0);
    CHECK_EQ_U64(io.w, 256);
    CHECK_EQ_U64(io.h, 192);
    CHECK_EQ_U64(io.field_h, 192);
    CHECK_EQ_U64(io.backdrop_rgb444, 0);
    run_frames(m, 59);
    cogmaster_frame io2 = frame_io(pixels2, FIELD_W * 2u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io2), COGMASTER_STATUS_OK);
    CHECK(memcmp(pixels, pixels2, (size_t)io.w * io.h * 2u) == 0);
    uint32_t wrong = 0;
    for (uint32_t y = 0; y < io.h; y++) {
        for (uint32_t x = 0; x < io.w; x++) {
            uint16_t want = cogmaster__testcard_pixel(io.x + x, io.y + y, COGMASTER_MACHINE_SMS2);
            if (pixels2[y * io.w + x] != want) {
                wrong++;
            }
        }
    }
    CHECK_EQ_U64(wrong, 0);
    cogmaster_destroy(m);
}

/* The colour at field dot (x, y) of a full-field copy. */
static uint16_t at(const uint16_t *field, uint32_t x, uint32_t y)
{
    return field[y * FIELD_W + x];
}

static void expect_pixel(const char *what, uint32_t x, uint32_t y, uint16_t got, uint16_t want)
{
    if (got != want) {
        fprintf(stderr, "%s (%lu, %lu): 0x%04x, expected 0x%04x\n", what, (unsigned long)x,
                (unsigned long)y, (unsigned)got, (unsigned)want);
        check_failures++;
    }
}

/* Runs one frame of machine and copies the whole field into buf. */
static cogmaster *full_field(uint32_t machine, uint16_t *buf)
{
    cogmaster *m = make(machine);
    run_frames(m, 1);
    cogmaster_frame io = frame_io(buf, FIELD_W * 2u, TEST_FRAME_FULL_FIELD);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(io.x, 0);
    CHECK_EQ_U64(io.y, 0);
    CHECK_EQ_U64(io.w, 256);
    CHECK_EQ_U64(io.h, 192);
    return m;
}

/* Each card rule at and next to its boundary. */
static void test_boundaries(void)
{
    static const struct {
        uint32_t x, y;
        uint16_t want;
    } sms[] = {
        {0, 0, WHITE},     {7, 7, WHITE},   {8, 0, WHITE},    {8, 1, RED},      {255, 100, WHITE},
        {100, 191, WHITE}, {100, 0, WHITE}, {100, 1, RED},    {100, 2, RED},    {100, 31, RED},
        {100, 161, GREEN}, {1, 40, WHITE},  {2, 40, BLACK},   {63, 40, WHITE},  {193, 100, BLUE},
        {254, 100, BLUE},  {64, 40, BLACK}, {100, 32, BLACK}, {100, 48, BLACK}, {192, 100, BLACK},
        {100, 160, BLACK},
    };
    cogmaster *m = full_field(COGMASTER_MACHINE_SMS2, pixels);
    for (size_t i = 0; i < sizeof sms / sizeof sms[0]; i++) {
        expect_pixel("sms", sms[i].x, sms[i].y, at(pixels, sms[i].x, sms[i].y), sms[i].want);
    }
    for (uint32_t x = 0; x < 256u; x++) {
        expect_pixel("sms top edge", x, 0, at(pixels, x, 0), WHITE);
        expect_pixel("sms bottom edge", x, 191, at(pixels, x, 191), WHITE);
    }
    for (uint32_t y = 0; y < 192u; y++) {
        expect_pixel("sms left edge", 0, y, at(pixels, 0, y), WHITE);
        expect_pixel("sms right edge", 255, y, at(pixels, 255, y), WHITE);
    }
    cogmaster_destroy(m);

    /* Game Gear, in window coordinates u = x - 48, v = y - 24. */
    static const struct {
        uint32_t u, v;
        uint16_t want;
    } gg[] = {
        {0, 0, WHITE},     {7, 7, WHITE},     {8, 1, RED},         {100, 7, RED},
        {100, 136, GREEN}, {1, 40, WHITE},    {2, 40, BLACK},      {15, 40, BLACK},
        {144, 40, BLUE},   {158, 40, BLUE},   {16, 8, 0x0000u},    {17, 9, 0x0000u},
        {18, 8, 0x0001u},  {16, 10, 0x0010u}, {143, 135, 0x0FFFu},
    };
    m = full_field(TEST_MACHINE_GG, pixels);
    for (size_t i = 0; i < sizeof gg / sizeof gg[0]; i++) {
        expect_pixel("gg", gg[i].u, gg[i].v, at(pixels, GG_X + gg[i].u, GG_Y + gg[i].v),
                     gg[i].want);
    }
    for (uint32_t u = 0; u < GG_W; u++) {
        expect_pixel("gg top edge", u, 0, at(pixels, GG_X + u, GG_Y), WHITE);
        expect_pixel("gg bottom edge", u, GG_H - 1u, at(pixels, GG_X + u, GG_Y + GG_H - 1u), WHITE);
        expect_pixel("gg above", u, 0, at(pixels, GG_X + u, GG_Y - 1u), MAGENTA);
        expect_pixel("gg below", u, GG_H - 1u, at(pixels, GG_X + u, GG_Y + GG_H), MAGENTA);
    }
    for (uint32_t v = 0; v < GG_H; v++) {
        expect_pixel("gg left edge", 0, v, at(pixels, GG_X, GG_Y + v), WHITE);
        expect_pixel("gg right edge", GG_W - 1u, v, at(pixels, GG_X + GG_W - 1u, GG_Y + v), WHITE);
        expect_pixel("gg left of", 0, v, at(pixels, GG_X - 1u, GG_Y + v), MAGENTA);
        expect_pixel("gg right of", GG_W - 1u, v, at(pixels, GG_X + GG_W, GG_Y + v), MAGENTA);
    }
    expect_pixel("gg field", 47, 24, at(pixels, 47, 24), MAGENTA);
    expect_pixel("gg field", 208, 24, at(pixels, 208, 24), MAGENTA);
    expect_pixel("gg field", 48, 23, at(pixels, 48, 23), MAGENTA);
    expect_pixel("gg field", 48, 168, at(pixels, 48, 168), MAGENTA);
    cogmaster_destroy(m);
}

/* The SMS chart holds exactly the 64 SMS colours, each channel widened as
   v | v << 2; the Game Gear chart holds all 4,096 RGB444 values, each as one
   2x2 cell. */
static void test_all_colours(void)
{
    static uint32_t seen[4096];
    memset(seen, 0, sizeof seen);
    cogmaster *m = full_field(COGMASTER_MACHINE_SMS2, pixels);
    for (uint32_t y = 32; y <= 160u; y++) {
        for (uint32_t x = 64; x <= 192u; x++) {
            if ((x - 64u) % 16u != 0u && (y - 32u) % 16u != 0u) {
                seen[at(pixels, x, y) & 0x0FFFu]++;
            }
        }
    }
    uint32_t sms_colours = 0;
    uint32_t sms_wanted = 0;
    for (uint32_t c = 0; c < 4096u; c++) {
        uint32_t r = c & 15u;
        uint32_t g = (c >> 4) & 15u;
        uint32_t b = (c >> 8) & 15u;
        int wanted = r % 5u == 0u && g % 5u == 0u && b % 5u == 0u;
        if (seen[c] != 0u) {
            sms_colours++;
            if (wanted) {
                sms_wanted++;
            }
        }
    }
    CHECK_EQ_U64(sms_colours, 64);
    CHECK_EQ_U64(sms_wanted, 64);
    cogmaster_destroy(m);

    memset(seen, 0, sizeof seen);
    m = full_field(TEST_MACHINE_GG, pixels);
    uint32_t split_cells = 0;
    for (uint32_t cy = 0; cy < 64u; cy++) {
        for (uint32_t cx = 0; cx < 64u; cx++) {
            uint32_t x = GG_X + 16u + 2u * cx;
            uint32_t y = GG_Y + 8u + 2u * cy;
            uint16_t c = at(pixels, x, y);
            if (at(pixels, x + 1u, y) != c || at(pixels, x, y + 1u) != c ||
                at(pixels, x + 1u, y + 1u) != c) {
                split_cells++;
            }
            seen[c & 0x0FFFu]++;
        }
    }
    uint32_t distinct = 0;
    for (uint32_t c = 0; c < 4096u; c++) {
        if (seen[c] == 1u) {
            distinct++;
        }
    }
    CHECK_EQ_U64(split_cells, 0);
    CHECK_EQ_U64(distinct, 4096);
    cogmaster_destroy(m);
}

/* With the full-field flag the Game Gear frame is the whole 256x192 field,
   magenta outside the window; the default crop is exactly the window. */
static void test_full_field(void)
{
    cogmaster *m = full_field(TEST_MACHINE_GG, pixels);
    uint32_t outside_wrong = 0;
    for (uint32_t y = 0; y < 192u; y++) {
        for (uint32_t x = 0; x < 256u; x++) {
            int inside = x >= GG_X && x < GG_X + GG_W && y >= GG_Y && y < GG_Y + GG_H;
            if (!inside && at(pixels, x, y) != MAGENTA) {
                outside_wrong++;
            }
        }
    }
    CHECK_EQ_U64(outside_wrong, 0);
    cogmaster_frame io = frame_io(pixels2, GG_W * 2u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(io.x, GG_X);
    CHECK_EQ_U64(io.y, GG_Y);
    CHECK_EQ_U64(io.w, GG_W);
    CHECK_EQ_U64(io.h, GG_H);
    CHECK_EQ_U64(io.field_h, 192);
    /* The crop equals the window, so the only magenta it holds is the one
       chart cell of that colour: four dots, none from the surround. */
    uint32_t differ = 0;
    uint32_t magenta = 0;
    for (uint32_t v = 0; v < GG_H; v++) {
        for (uint32_t u = 0; u < GG_W; u++) {
            uint16_t c = pixels2[v * GG_W + u];
            if (c != at(pixels, GG_X + u, GG_Y + v)) {
                differ++;
            }
            if (c == MAGENTA) {
                magenta++;
            }
        }
    }
    CHECK_EQ_U64(differ, 0);
    CHECK_EQ_U64(magenta, 4);
    /* A flag this release does not define is an argument error. */
    io = frame_io(pixels2, GG_W * 2u, 2u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_destroy(m);
}

static cogmaster_info info_of(const cogmaster *m)
{
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    return info;
}

/* Geometry, rate and pixel aspect for each machine, and the interface
   numbers. */
static void test_info(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_AUTO);
    cogmaster_info info = info_of(m);
    CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(info.crop_w, 256);
    CHECK_EQ_U64(info.crop_h, 192);
    CHECK_EQ_U64(info.field_w, 256);
    CHECK_EQ_U64(info.field_h, 192);
    CHECK_EQ_U64(info.max_w, 256);
    CHECK_EQ_U64(info.max_h, 240);
    CHECK_EQ_U64(info.fps_num, 1640625);
    CHECK_EQ_U64(info.fps_den, 27379);
    CHECK_EQ_U64(info.par_num, 8);
    CHECK_EQ_U64(info.par_den, 7);
    cogmaster_destroy(m);

    m = make(TEST_MACHINE_GG);
    info = info_of(m);
    CHECK_EQ_U64(info.machine, TEST_MACHINE_GG);
    CHECK_EQ_U64(info.crop_w, 160);
    CHECK_EQ_U64(info.crop_h, 144);
    CHECK_EQ_U64(info.field_w, 256);
    CHECK_EQ_U64(info.field_h, 192);
    CHECK_EQ_U64(info.max_w, 256);
    CHECK_EQ_U64(info.max_h, 240);
    CHECK_EQ_U64(info.fps_num, 1640625);
    CHECK_EQ_U64(info.fps_den, 27379);
    CHECK_EQ_U64(info.par_num, 6);
    CHECK_EQ_U64(info.par_den, 5);
    cogmaster_destroy(m);

    cogmaster_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(cogmaster_get_versions(&v), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(v.library_major, COGMASTER_VERSION_MAJOR);
    CHECK_EQ_U64(v.library_minor, COGMASTER_VERSION_MINOR);
    CHECK_EQ_U64(v.library_patch, COGMASTER_VERSION_PATCH);
    CHECK_EQ_U64(v.api, COGMASTER_API_VERSION);
    CHECK_EQ_U64(v.abi, COGMASTER_ABI_VERSION);
    CHECK_EQ_U64(v.behaviour, COGMASTER_BEHAVIOUR_REVISION);
    CHECK_EQ_U64(v.state, COGMASTER_STATE_FORMAT_VERSION);
}

/* A machine outside the profile set is refused before anything changes. */
static void test_profile_rejected(void)
{
    cogmaster *m = make(TEST_MACHINE_GG);
    run_frames(m, 1);
    static const uint32_t bad[] = {3u, 0xFFFFFFFFu};
    for (size_t i = 0; i < 2u; i++) {
        cogmaster_media media = make_media(bad[i]);
        CHECK_EQ_U64(cogmaster_load(m, &media), TEST_STATUS_ERR_PROFILE_REJECTED);
        cogmaster_info info = info_of(m);
        CHECK_EQ_U64(info.machine, TEST_MACHINE_GG);
        CHECK_EQ_U64(info.crop_w, 160);
    }
    cogmaster_frame io = frame_io(pixels, GG_W * 2u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(pixels[0], WHITE);
    cogmaster_destroy(m);
}

/* Writes proto (ours bytes) at the start of the zeroed scratch buffer with
   the size tag size. */
static uint8_t *tagged(const void *proto, uint32_t ours, uint32_t size)
{
    uint8_t *b = (uint8_t *)tag_buf;
    memset(b, 0, TAG_BUF);
    memcpy(b, proto, ours);
    memcpy(b, &size, sizeof size);
    return b;
}

/* The input rule for one struct: below its size or above 4096 is
   ERR_STRUCT_SIZE without reading past the tag; a larger struct is OK only
   when every byte past ours is zero. */
static void check_input(const char *what, const void *proto, uint32_t ours,
                        cogmaster_status (*call)(void *))
{
    const uint32_t small[] = {0u, ours - 1u};
    for (uint32_t i = 0; i < 2u; i++) {
        expect_status(what, small[i], call(tagged(proto, ours, small[i])),
                      COGMASTER_STATUS_ERR_STRUCT_SIZE);
    }
    /* The scratch buffer is all zero past the struct, so only the cap can
       refuse these two. */
    expect_status(what, TAG_MAX + 1u, call(tagged(proto, ours, TAG_MAX + 1u)),
                  COGMASTER_STATUS_ERR_STRUCT_SIZE);
    expect_status(what, 0xFFFFFFFFu, call(tagged(proto, ours, 0xFFFFFFFFu)),
                  COGMASTER_STATUS_ERR_STRUCT_SIZE);
    uint8_t *b = tagged(proto, ours, ours + 8u);
    b[ours + 3u] = 1u;
    expect_status(what, ours + 8u, call(b), COGMASTER_STATUS_ERR_STRUCT_SIZE);

    expect_status(what, ours, call(tagged(proto, ours, ours)), COGMASTER_STATUS_OK);
    expect_status(what, ours + 8u, call(tagged(proto, ours, ours + 8u)), COGMASTER_STATUS_OK);
    expect_status(what, TAG_MAX, call(tagged(proto, ours, TAG_MAX)), COGMASTER_STATUS_OK);
}

/* The output rule for one struct: below its size is ERR_STRUCT_SIZE with the
   struct untouched; any larger size is OK with only ours bytes written. */
static void check_output(const char *what, uint32_t ours, cogmaster_status (*call)(void *))
{
    static uint8_t before[TAG_BUF];
    uint8_t *b = (uint8_t *)tag_buf;
    const uint32_t small[] = {0u, ours - 1u};
    for (uint32_t i = 0; i < 2u; i++) {
        memset(b, POISON, TAG_BUF);
        memcpy(b, &small[i], sizeof small[i]);
        memcpy(before, b, TAG_BUF);
        expect_status(what, small[i], call(b), COGMASTER_STATUS_ERR_STRUCT_SIZE);
        if (memcmp(before, b, TAG_BUF) != 0) {
            fprintf(stderr, "%s with size %lu: struct changed\n", what, (unsigned long)small[i]);
            check_failures++;
        }
    }
    const uint32_t big[] = {ours, ours + 8u, TAG_BUF};
    for (uint32_t i = 0; i < 3u; i++) {
        memset(b, POISON, TAG_BUF);
        memcpy(b, &big[i], sizeof big[i]);
        expect_status(what, big[i], call(b), COGMASTER_STATUS_OK);
        uint32_t size;
        memcpy(&size, b, sizeof size);
        CHECK_EQ_U64(size, big[i]);
        uint32_t touched = 0;
        for (uint32_t k = ours; k < TAG_BUF; k++) {
            if (b[k] != POISON) {
                touched++;
            }
        }
        if (touched != 0u) {
            fprintf(stderr, "%s with size %lu: %lu bytes past the struct written\n", what,
                    (unsigned long)big[i], (unsigned long)touched);
            check_failures++;
        }
    }
}

static cogmaster_status call_create(void *s)
{
    cogmaster *m = NULL;
    cogmaster_status st = cogmaster_create((const cogmaster_config *)s, &m);
    cogmaster_destroy(m);
    return st;
}

static cogmaster_status call_load(void *s)
{
    return cogmaster_load(shared, (const cogmaster_media *)s);
}

static cogmaster_status call_get_frame(void *s)
{
    return cogmaster_get_frame(shared, (cogmaster_frame *)s);
}

static cogmaster_status call_get_info(void *s)
{
    return cogmaster_get_info(shared, (cogmaster_info *)s);
}

static cogmaster_status call_get_versions(void *s)
{
    return cogmaster_get_versions((cogmaster_versions *)s);
}

static cogmaster_status call_run_frame(void *s)
{
    return cogmaster_run_frame(shared, (cogmaster_run_result *)s);
}

/* NULL pointers and the size tags of every input and output struct. */
static void test_size_tags(void)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(NULL, &m), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_create(&cfg, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK(m == NULL);

    shared = make(COGMASTER_MACHINE_SMS2);
    run_frames(shared, 1);
    cogmaster_media media = make_media(COGMASTER_MACHINE_SMS2);
    cogmaster_frame io = frame_io(pixels, FIELD_W * 2u, 0u);
    cogmaster_info info;
    cogmaster_versions versions;
    cogmaster_run_result result;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    memset(&versions, 0, sizeof versions);
    versions.size = (uint32_t)sizeof versions;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(cogmaster_load(NULL, &media), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_load(shared, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_frame(NULL, &io), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_frame(shared, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_info(NULL, &info), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_info(shared, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_versions(NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_run_frame(NULL, &result), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_run_frame(shared, NULL), COGMASTER_STATUS_OK);

    check_input("config", &cfg, (uint32_t)sizeof cfg, call_create);
    check_input("media", &media, (uint32_t)sizeof media, call_load);
    run_frames(shared, 1);
    check_input("frame", &io, (uint32_t)sizeof io, call_get_frame);
    check_output("info", (uint32_t)sizeof info, call_get_info);
    check_output("versions", (uint32_t)sizeof versions, call_get_versions);
    check_output("run_result", (uint32_t)sizeof result, call_run_frame);
    cogmaster_destroy(shared);
    shared = NULL;
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"api.size_tags", test_size_tags},
    {"info", test_info},
    {"load.profile_rejected", test_profile_rejected},
    {"testcard.all_colours", test_all_colours},
    {"testcard.boundaries", test_boundaries},
    {"testcard.full_field", test_full_field},
    {"testcard.static", test_static},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-core <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-core: unknown case %s\n", argv[1]);
    return 2;
}
