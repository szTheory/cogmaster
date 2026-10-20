/* The core library through its public header: cogmaster-test-core <case>.
   Expected test-card values follow the card's rules as docs/ describes them;
   the pinned hashes are checked through the runner. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define FIELD_W 256u
#define FIELD_MAX_H 240u
#define POISON 0xA5u  /* fill the core must leave alone */
#define TAG_BUF 8192u /* bytes in the size-tag scratch buffer */
#define TAG_MAX 4096u /* the largest input size tag the core accepts */
#define P ((uint32_t)sizeof(void *))

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
static uint16_t wide[336u * FIELD_MAX_H]; /* rows of 336 pixels */
static uint64_t tag_buf[TAG_BUF / 8u];    /* aligned for every public struct */
static cogmaster *shared;                 /* the instance the size-tag calls use */

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
    cogmaster_frame io = frame_io(buf, FIELD_W * 2u, COGMASTER_FRAME_FULL_FIELD);
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
        {100, 136, GREEN}, {1, 40, WHITE},    {2, 40, BLACK},      {15, 41, BLACK},
        {144, 40, BLUE},   {158, 40, BLUE},   {16, 8, 0x0000u},    {17, 9, 0x0000u},
        {18, 8, 0x0001u},  {16, 10, 0x0010u}, {143, 135, 0x0FFFu},
    };
    m = full_field(COGMASTER_MACHINE_GG, pixels);
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
    m = full_field(COGMASTER_MACHINE_GG, pixels);
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
    cogmaster *m = full_field(COGMASTER_MACHINE_GG, pixels);
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

    m = make(COGMASTER_MACHINE_GG);
    info = info_of(m);
    CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_GG);
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
    CHECK_EQ_U64(info.region, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(info.tv, COGMASTER_TV_NTSC);
    cogmaster_destroy(m);

    /* A PAL Master System: 214,092 ticks of 53,203,424 / 5 Hz a frame. */
    cogmaster_config cfg = make_config();
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = make_media(COGMASTER_MACHINE_SMS2);
    media.tv = COGMASTER_TV_PAL;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    info = info_of(m);
    CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(info.region, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(info.tv, COGMASTER_TV_PAL);
    CHECK_EQ_U64(info.fps_num, 13300856);
    CHECK_EQ_U64(info.fps_den, 267615);
    CHECK_EQ_U64((uint64_t)info.fps_num * 5u * 214092u, (uint64_t)info.fps_den * 53203424u);
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
    cogmaster *m = make(COGMASTER_MACHINE_GG);
    run_frames(m, 1);
    static const uint32_t bad[] = {4u, 0xFFFFFFFFu};
    for (size_t i = 0; i < 2u; i++) {
        cogmaster_media media = make_media(bad[i]);
        CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_PROFILE_REJECTED);
        cogmaster_info info = info_of(m);
        CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_GG);
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

/* The input rule for one struct: below its first released size or above
   4096 is ERR_STRUCT_SIZE without reading past the tag; a larger struct is
   OK only when every byte past ours is zero. */
static void check_input(const char *what, const void *proto, uint32_t first, uint32_t ours,
                        cogmaster_status (*call)(void *))
{
    const uint32_t small[] = {0u, first - 1u};
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

/* The output rule for one struct: below its first released size is
   ERR_STRUCT_SIZE with the struct untouched; any larger size is OK with only
   ours bytes written. */
static void check_output(const char *what, uint32_t first, uint32_t ours,
                         cogmaster_status (*call)(void *))
{
    static uint8_t before[TAG_BUF];
    uint8_t *b = (uint8_t *)tag_buf;
    const uint32_t small[] = {0u, first - 1u};
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

static cogmaster_status call_get_fault(void *s)
{
    return cogmaster_get_fault(shared, (cogmaster_fault *)s);
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

    check_input("config", &cfg, 4u * P, (uint32_t)sizeof cfg, call_create);
    check_input("media", &media, 4u * P, (uint32_t)sizeof media, call_load);
    run_frames(shared, 1);
    check_input("frame", &io, 2u * P + 32u, (uint32_t)sizeof io, call_get_frame);
    check_output("info", 48u, (uint32_t)sizeof info, call_get_info);
    check_output("versions", 32u, (uint32_t)sizeof versions, call_get_versions);
    check_output("run_result", 12u, (uint32_t)sizeof result, call_run_frame);
    check_output("fault", 32u, (uint32_t)sizeof(cogmaster_fault), call_get_fault);
    cogmaster_destroy(shared);
    shared = NULL;
}

/* A counting allocator: every call, the bytes live, and whether each free
   names the size its block was allocated with. */
typedef struct counts {
    uint32_t allocs, frees, size_mismatches;
    uint64_t live_bytes;
    int fail; /* nonzero: alloc returns NULL */
} counts;

static void *count_alloc(void *user, size_t size)
{
    counts *c = (counts *)user;
    c->allocs++;
    if (c->fail) {
        return NULL;
    }
    /* A header before the block records its size, so free can check it. */
    uint64_t *block = (uint64_t *)malloc(size + 16u);
    if (block == NULL) {
        return NULL;
    }
    block[0] = (uint64_t)size;
    c->live_bytes += size;
    return block + 2;
}

static void count_free(void *user, void *ptr, size_t size)
{
    counts *c = (counts *)user;
    c->frees++;
    uint64_t *block = (uint64_t *)ptr - 2;
    if (block[0] != (uint64_t)size) {
        c->size_mismatches++;
    }
    c->live_bytes -= block[0];
    free(block);
}

/* The statuses a caller meets before content, with a short pitch and with a
   cartridge image of a size the loader refuses. */
static void test_statuses(void)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    cogmaster_frame io = frame_io(pixels, FIELD_W * 2u, 0u);
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(cogmaster_run_frame(m, &result), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_ERR_NO_CONTENT);

    cogmaster_media media = make_media(COGMASTER_MACHINE_GG);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    run_frames(m, 1);

    /* One byte short of a row: the buffer is left as it was. */
    memset(pixels, POISON, sizeof pixels);
    io = frame_io(pixels, GG_W * 2u - 1u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL);
    uint32_t touched = 0;
    for (size_t i = 0; i < sizeof pixels; i++) {
        if (((const uint8_t *)pixels)[i] != POISON) {
            touched++;
        }
    }
    CHECK_EQ_U64(touched, 0);
    io = frame_io(pixels, FIELD_W * 2u - 1u, COGMASTER_FRAME_FULL_FIELD);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL);
    /* A pitch above 65536 bytes, or no buffer, is an argument error. */
    io = frame_io(pixels, 65537u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_ERR_ARGUMENT);
    io = frame_io(NULL, GG_W * 2u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_ERR_ARGUMENT);

    /* An image of the wrong size, or a size with no image, is refused and
       the loaded card keeps running. */
    static const uint8_t image[1] = {0};
    cogmaster_media cart = make_media(COGMASTER_MACHINE_SMS2);
    cart.image = image;
    CHECK_EQ_U64(cogmaster_load(m, &cart), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    cart.image_size = 1u;
    CHECK_EQ_U64(cogmaster_load(m, &cart), COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    cart.image = NULL;
    CHECK_EQ_U64(cogmaster_load(m, &cart), COGMASTER_STATUS_ERR_ARGUMENT);
    /* An 8 KiB image on a Game Gear is the X-Terminator, on no supported
       board. */
    static const uint8_t device[8192] = {0};
    cart = make_media(COGMASTER_MACHINE_GG);
    cart.image = device;
    cart.image_size = sizeof device;
    CHECK_EQ_U64(cogmaster_load(m, &cart), COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
    CHECK_EQ_U64(cogmaster_run_frame(m, &result), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(result.stop, COGMASTER_STOP_FRAME);
    CHECK_EQ_U64(result.overshoot_ticks, 0);
    CHECK_EQ_U64(info_of(m).machine, COGMASTER_MACHINE_GG);
    io = frame_io(pixels, GG_W * 2u, 0u);
    CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(io.w, GG_W);
    cogmaster_destroy(m);
}

/* Allocation goes through the caller's hooks only, each free names the size
   allocated, and a failed allocation leaves *out alone. */
static void test_allocator(void)
{
    counts c;
    memset(&c, 0, sizeof c);
    cogmaster_config cfg = make_config();
    cfg.allocator.alloc = count_alloc;
    cfg.allocator.free = count_free;
    cfg.allocator.user = &c;
    cogmaster *marker = (cogmaster *)&c;
    cogmaster *m = marker;

    cfg.allocator.alloc = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_ERR_ARGUMENT);
    cfg.allocator.alloc = count_alloc;
    cfg.allocator.free = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_ERR_ARGUMENT);
    cfg.allocator.free = count_free;
    CHECK_EQ_U64(c.allocs, 0);
    CHECK(m == marker);

    c.fail = 1;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_ERR_NO_MEMORY);
    CHECK_EQ_U64(c.allocs, 1);
    CHECK(m == marker);

    c.fail = 0;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    CHECK(m != marker && m != NULL);
    cogmaster_media media = make_media(COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    uint32_t after_load = c.allocs;
    run_frames(m, 60);
    CHECK_EQ_U64(c.allocs, after_load);
    CHECK(c.live_bytes > 0u);
    cogmaster_destroy(m);
    CHECK_EQ_U64(c.frees, c.allocs - 1u);
    CHECK_EQ_U64(c.size_mismatches, 0);
    CHECK_EQ_U64(c.live_bytes, 0);

    cogmaster_destroy(NULL);
}

/* After load, 600 frames with a frame copy each, then a state save and a
   state load into buffers the caller owns, make no allocator call at all:
   the frame loop never allocates. */
static void test_no_alloc_after_load(void)
{
    counts c;
    memset(&c, 0, sizeof c);
    cogmaster_config cfg = make_config();
    cfg.allocator.alloc = count_alloc;
    cfg.allocator.free = count_free;
    cfg.allocator.user = &c;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media = make_media(COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    size_t size = cogmaster__state_size(m);
    CHECK(size > 0u);
    uint8_t *state = (uint8_t *)malloc(size);
    CHECK(state != NULL);
    if (m == NULL || state == NULL) {
        free(state);
        cogmaster_destroy(m);
        return;
    }
    uint32_t allocs = c.allocs;
    uint32_t frees = c.frees;
    for (uint32_t i = 0; i < 600u; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
        cogmaster_frame io = frame_io(pixels, FIELD_W * 2u, 0u);
        CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
    }
    CHECK_EQ_U64(cogmaster__state_save(m, state, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(m, state, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(c.allocs - allocs, 0);
    CHECK_EQ_U64(c.frees - frees, 0);
    free(state);
    cogmaster_destroy(m);
    CHECK_EQ_U64(c.frees, c.allocs);
}

/* Each struct at the size it was first released with, derived here from its
   documented layout: inputs and outputs are accepted at exactly that size,
   one byte less is refused and an output struct is left untouched. */
static void test_first_released_sizes(void)
{
    shared = make(COGMASTER_MACHINE_SMS2);
    run_frames(shared, 1);
    cogmaster_config cfg = make_config();
    cogmaster_media media = make_media(COGMASTER_MACHINE_SMS2);
    cogmaster_frame io = frame_io(pixels, FIELD_W * 2u, 0u);
    const struct {
        const char *what;
        const void *proto;
        uint32_t first;
        cogmaster_status (*call)(void *);
    } inputs[] = {
        {"config", &cfg, 4u * P, call_create},
        {"media", &media, 4u * P, call_load},
        {"frame", &io, 2u * P + 32u, call_get_frame},
    };
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
        uint32_t first = inputs[i].first;
        expect_status(inputs[i].what, first, inputs[i].call(tagged(inputs[i].proto, first, first)),
                      COGMASTER_STATUS_OK);
        expect_status(inputs[i].what, first - 1u,
                      inputs[i].call(tagged(inputs[i].proto, first, first - 1u)),
                      COGMASTER_STATUS_ERR_STRUCT_SIZE);
        run_frames(shared, 1);
    }

    const struct {
        const char *what;
        uint32_t first;
        cogmaster_status (*call)(void *);
    } outputs[] = {
        {"info", 48u, call_get_info},
        {"versions", 32u, call_get_versions},
        {"run_result", 12u, call_run_frame},
        {"fault", 32u, call_get_fault},
    };
    static uint8_t before[TAG_BUF];
    uint8_t *b = (uint8_t *)tag_buf;
    for (size_t i = 0; i < sizeof outputs / sizeof outputs[0]; i++) {
        uint32_t first = outputs[i].first;
        memset(b, POISON, TAG_BUF);
        memcpy(b, &first, sizeof first);
        expect_status(outputs[i].what, first, outputs[i].call(b), COGMASTER_STATUS_OK);
        uint32_t small = first - 1u;
        memset(b, POISON, TAG_BUF);
        memcpy(b, &small, sizeof small);
        memcpy(before, b, TAG_BUF);
        expect_status(outputs[i].what, small, outputs[i].call(b), COGMASTER_STATUS_ERR_STRUCT_SIZE);
        CHECK(memcmp(before, b, TAG_BUF) == 0);
    }
    cogmaster_destroy(shared);
    shared = NULL;
}

/* A pitch wider than a row: each crop row lands at its offset and the
   padding after it keeps the caller's bytes. */
static void test_pitch_wide(void)
{
    const uint32_t pitch = 336u * 2u;
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = make(machines[k]);
        run_frames(m, 1);
        cogmaster_frame tight = frame_io(pixels2, FIELD_W * 2u, 0u);
        CHECK_EQ_U64(cogmaster_get_frame(m, &tight), COGMASTER_STATUS_OK);
        memset(wide, POISON, sizeof wide);
        cogmaster_frame io = frame_io(wide, pitch, 0u);
        CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
        CHECK(io.w == tight.w && io.h == tight.h);
        const uint8_t *bytes = (const uint8_t *)wide;
        const uint8_t *want = (const uint8_t *)pixels2;
        uint32_t rows_wrong = 0;
        uint32_t pad_written = 0;
        for (uint32_t y = 0; y < io.h; y++) {
            const uint8_t *row = bytes + (size_t)y * pitch;
            if (memcmp(row, want + (size_t)y * tight.pitch_bytes, (size_t)io.w * 2u) != 0) {
                rows_wrong++;
            }
            for (uint32_t i = io.w * 2u; i < pitch; i++) {
                if (row[i] != POISON) {
                    pad_written++;
                }
            }
        }
        CHECK_EQ_U64(rows_wrong, 0);
        CHECK_EQ_U64(pad_written, 0);
        cogmaster_destroy(m);
    }
}

/* Cartridge images. Sizes are bytes; a page is 16 KiB. */
#define PAGE 16384u
#define IMAGE_MIN 1024u
#define IMAGE_MAX (1048576u + 512u)
static uint8_t big[IMAGE_MAX + 64u];

/* Fills big with bytes that differ from page to page and from $FF. */
static void fill_big(void)
{
    for (size_t i = 0; i < sizeof big; i++) {
        big[i] = (uint8_t)((i * 7u + (i >> 14)) % 255u);
    }
}

static cogmaster_media image_media(const void *image, size_t n, uint32_t machine)
{
    cogmaster_media media = make_media(machine);
    media.image = image;
    media.image_size = n;
    return media;
}

/* Loads n bytes of big into m and checks the buffer: the body after any
   copier header, then $FF up to the next page. */
static void expect_loaded(cogmaster *m, size_t n, size_t header, uint32_t padded)
{
    cogmaster_media media = image_media(big, n, COGMASTER_MACHINE_SMS2);
    expect_status("load", (uint32_t)n, cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->cart.size, padded);
    if (m->cart.rom == NULL || m->cart.size != padded) {
        return;
    }
    size_t body = n - header;
    CHECK(memcmp(m->cart.rom, big + header, body) == 0);
    uint32_t not_ff = 0;
    for (size_t i = body; i < padded; i++) {
        if (m->cart.rom[i] != 0xFFu) {
            not_ff++;
        }
    }
    CHECK_EQ_U64(not_ff, 0);
}

/* The size rules: 1 KiB to 1 MiB + 512 bytes, a 512-byte copier header
   stripped when the size is 512 past a whole page, $FF padding to a page. */
static void test_load_sizes(void)
{
    fill_big();
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        return;
    }
    static const size_t refused[] = {0u, 1u, IMAGE_MIN - 1u, IMAGE_MAX + 1u, IMAGE_MAX + 64u};
    for (size_t i = 0; i < sizeof refused / sizeof refused[0]; i++) {
        cogmaster_media media = image_media(big, refused[i], COGMASTER_MACHINE_SMS2);
        expect_status("load", (uint32_t)refused[i], cogmaster_load(m, &media),
                      COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE);
    }
    /* No image pointer with a size is an argument error, not a size. */
    cogmaster_media media = image_media(NULL, IMAGE_MIN, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_ERR_ARGUMENT);

    expect_loaded(m, IMAGE_MIN, 0u, PAGE);
    expect_loaded(m, PAGE + 512u, 512u, PAGE);
    expect_loaded(m, 3u * PAGE, 0u, 3u * PAGE);
    expect_loaded(m, 20u * 1024u, 0u, 2u * PAGE);
    expect_loaded(m, 1048576u, 0u, 1048576u);
    expect_loaded(m, IMAGE_MAX, 512u, 1048576u);
    /* A 20 KiB image reads $FF from 20,480 to 32,767 through the bus too. */
    expect_loaded(m, 20u * 1024u, 0u, 2u * PAGE);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 20479u), big[20479]);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 20480u), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 32767u), 0xFF);
    /* Loading the test card again frees the image. */
    media = make_media(COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK(m->cart.rom == NULL);
    cogmaster_destroy(m);
}

/* splitmix64 (Steele, Lea and Flood, 2014): a seeded sequence the same on
   every platform. */
static uint64_t splitmix64(uint64_t *s)
{
    uint64_t z = (*s += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static uint8_t instance_before[sizeof(struct cogmaster)];
/* An image up to 1 MiB + 511 bytes with no copier header pads to 65 pages. */
static uint8_t rom_before[1048576u + PAGE];

/* 200 seeded loads with sizes near both bounds and any machine, region and
   tv value: each
   refused load returns the status the rules give, allocates nothing and
   leaves every byte of the instance and of its image as it was. */
static void test_load_rejected_unchanged(void)
{
    fill_big();
    counts c;
    memset(&c, 0, sizeof c);
    cogmaster_config cfg = make_config();
    cfg.allocator.alloc = count_alloc;
    cfg.allocator.free = count_free;
    cfg.allocator.user = &c;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        return;
    }
    cogmaster_media first = image_media(big, IMAGE_MIN, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &first), COGMASTER_STATUS_OK);
    run_frames(m, 2);

    uint64_t seed = UINT64_C(0x5EED0000C0C0A5A5);
    const size_t bases[3] = {0u, IMAGE_MIN, IMAGE_MAX};
    uint32_t refusals = 0, changed = 0;
    for (uint32_t i = 0; i < 200u; i++) {
        uint64_t r = splitmix64(&seed);
        size_t base = bases[r % 3u];
        size_t delta = (size_t)((r >> 8) % 33u);
        size_t n = base + delta >= 16u ? base + delta - 16u : delta;
        uint32_t machine = (r >> 24) % 4u == 0u ? (uint32_t)(r >> 32) : (uint32_t)((r >> 40) % 4u);
        uint64_t r2 = splitmix64(&seed);
        uint32_t region = r2 % 8u == 0u ? (uint32_t)(r2 >> 32) : (uint32_t)((r2 >> 8) % 4u);
        uint32_t tv = (r2 >> 16) % 8u == 0u ? (uint32_t)(r2 >> 24) : (uint32_t)((r2 >> 20) % 4u);
        cogmaster_status want = COGMASTER_STATUS_OK;
        if (n < IMAGE_MIN || n > IMAGE_MAX) {
            want = COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE;
        } else if (machine > COGMASTER_MACHINE_SMS1 || region > COGMASTER_REGION_JAPAN ||
                   tv > COGMASTER_TV_PAL) {
            want = COGMASTER_STATUS_ERR_PROFILE_REJECTED;
        } else if (tv == COGMASTER_TV_PAL &&
                   (machine == COGMASTER_MACHINE_GG || region == COGMASTER_REGION_JAPAN)) {
            want = COGMASTER_STATUS_ERR_PROFILE_REJECTED;
        }
        memcpy(instance_before, m, sizeof instance_before);
        uint32_t rom_size = m->cart.size;
        if (m->cart.rom != NULL) {
            memcpy(rom_before, m->cart.rom, rom_size);
        }
        uint32_t allocs = c.allocs;
        cogmaster_media media = image_media(big, n, machine);
        media.region = region;
        media.tv = tv;
        cogmaster_status got = cogmaster_load(m, &media);
        expect_status("seeded load", (uint32_t)n, got, want);
        if (got != COGMASTER_STATUS_OK) {
            refusals++;
            if (memcmp(instance_before, m, sizeof instance_before) != 0 || c.allocs != allocs ||
                (m->cart.rom != NULL && memcmp(rom_before, m->cart.rom, rom_size) != 0)) {
                fprintf(stderr,
                        "seeded load %lu: size %lu machine %lu region %lu tv %lu changed the "
                        "instance\n",
                        (unsigned long)i, (unsigned long)n, (unsigned long)machine,
                        (unsigned long)region, (unsigned long)tv);
                changed++;
            }
        } else {
            run_frames(m, 1);
        }
    }
    printf("seeded loads: 200 run, %lu refused, %lu changed the instance\n",
           (unsigned long)refusals, (unsigned long)changed);
    CHECK(refusals >= 50u);
    CHECK_EQ_U64(changed, 0);
    cogmaster_destroy(m);
    CHECK_EQ_U64(c.frees, c.allocs);
    CHECK_EQ_U64(c.live_bytes, 0);
}

static uint8_t board_img[64u * 1024u];

/* Loads n bytes of board_img as machine, which must be refused with
   ERR_UNSUPPORTED_BOARD and leave the instance, its image and the
   allocator untouched. */
static void expect_board_refused(const char *what, cogmaster *m, const counts *c, size_t n,
                                 uint32_t machine)
{
    memcpy(instance_before, m, sizeof instance_before);
    uint32_t rom_size = m->cart.size;
    if (m->cart.rom != NULL) {
        memcpy(rom_before, m->cart.rom, rom_size);
    }
    uint32_t allocs = c->allocs;
    cogmaster_media media = image_media(board_img, n, machine);
    expect_status(what, (uint32_t)n, cogmaster_load(m, &media),
                  COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
    if (memcmp(instance_before, m, sizeof instance_before) != 0 || c->allocs != allocs ||
        (m->cart.rom != NULL && memcmp(rom_before, m->cart.rom, rom_size) != 0)) {
        fprintf(stderr, "%s: the refused load changed the instance\n", what);
        check_failures++;
    }
}

/* An image on no supported board is refused by load with the status the
   probe gives, and the instance stays byte for byte as it was: a 64 KiB
   image storing to $A000, an 8 KiB image whose header names the Game Gear,
   and an 8 KiB image loaded as a Game Gear. As a Master System the last
   loads with no mapper. */
static void test_load_board(void)
{
    fill_big();
    counts c;
    memset(&c, 0, sizeof c);
    cogmaster_config cfg = make_config();
    cfg.allocator.alloc = count_alloc;
    cfg.allocator.free = count_free;
    cfg.allocator.user = &c;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        return;
    }
    cogmaster_media first = image_media(big, 64u * 1024u, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &first), COGMASTER_STATUS_OK);
    run_frames(m, 2);

    /* Four LD ($A000),A stores, the Korean board's register. */
    memset(board_img, 0, sizeof board_img);
    for (uint32_t i = 0; i < 4u; i++) {
        board_img[0x100u + 3u * i] = 0x32u;
        board_img[0x101u + 3u * i] = 0x00u;
        board_img[0x102u + 3u * i] = 0xA0u;
    }
    cogmaster_cart_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(cogmaster_probe(board_img, sizeof board_img, &info),
                 COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
    CHECK_EQ_U64(info.board, 0);
    expect_board_refused("$A000 stores", m, &c, sizeof board_img, COGMASTER_MACHINE_SMS2);
    expect_board_refused("$A000 stores as auto", m, &c, sizeof board_img, COGMASTER_MACHINE_AUTO);

    /* The ROM header magic as bytes at $1FF0, region code 5 (Game Gear
       Japan), size code $A. */
    static const uint8_t header[16] = {0x54u, 0x4Du, 0x52u, 0x20u, 0x53u, 0x45u, 0x47u, 0x41u,
                                       0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x5Au};
    memset(board_img, 0, sizeof board_img);
    memcpy(board_img + 0x1FF0u, header, sizeof header);
    expect_board_refused("8 KiB Game Gear header", m, &c, 8192u, COGMASTER_MACHINE_SMS2);
    memset(board_img + 0x1FF0u, 0, sizeof header);
    expect_board_refused("8 KiB as a Game Gear", m, &c, 8192u, COGMASTER_MACHINE_GG);
    cogmaster_media sms = image_media(board_img, 8192u, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &sms), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->cart.board, COGMASTER_BOARD_ROM_ONLY);
    cogmaster_destroy(m);
    CHECK_EQ_U64(c.frees, c.allocs);
    CHECK_EQ_U64(c.live_bytes, 0);
}

/* Every profile pair: AUTO is an export console on an NTSC set; each legal
   pair loads, reports itself and runs frames of its TV's length; PAL on a
   Game Gear or a Japanese console, which were never made (SMS-SYSTEM C6),
   and a value outside its set are refused with the instance unchanged. */
static void test_load_profiles(void)
{
    static const struct {
        uint32_t machine, region, tv;
        cogmaster_status status;
        uint32_t want_region, want_tv;
    } rows[] = {
        {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC, COGMASTER_STATUS_OK,
         COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC},
        {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL, COGMASTER_STATUS_OK,
         COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL},
        {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_JAPAN, COGMASTER_TV_PAL,
         COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u, 0u},
        {COGMASTER_MACHINE_AUTO, COGMASTER_REGION_AUTO, COGMASTER_TV_AUTO, COGMASTER_STATUS_OK,
         COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL, COGMASTER_STATUS_OK,
         COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_AUTO, COGMASTER_TV_PAL,
         COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u, 0u},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL,
         COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u, 0u},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_PAL,
         COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u, 0u},
        {COGMASTER_MACHINE_AUTO, COGMASTER_REGION_JAPAN, COGMASTER_TV_PAL,
         COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u, 0u},
        {COGMASTER_MACHINE_SMS2, 3u, COGMASTER_TV_NTSC, COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u,
         0u},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, 3u, COGMASTER_STATUS_ERR_PROFILE_REJECTED,
         0u, 0u},
        {COGMASTER_MACHINE_SMS2, 0xFFFFFFFFu, COGMASTER_TV_AUTO,
         COGMASTER_STATUS_ERR_PROFILE_REJECTED, 0u, 0u},
        {COGMASTER_MACHINE_AUTO, COGMASTER_REGION_AUTO, COGMASTER_TV_PAL, COGMASTER_STATUS_OK,
         COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_AUTO, COGMASTER_STATUS_OK,
         COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC, COGMASTER_STATUS_OK,
         COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_AUTO, COGMASTER_TV_AUTO, COGMASTER_STATUS_OK,
         COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC},
    };
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        run_frames(m, 2);
        cogmaster_media media = make_media(rows[i].machine);
        media.region = rows[i].region;
        media.tv = rows[i].tv;
        memcpy(instance_before, m, sizeof instance_before);
        cogmaster_status got = cogmaster_load(m, &media);
        expect_status("profile row", (uint32_t)i, got, rows[i].status);
        if (rows[i].status != COGMASTER_STATUS_OK) {
            if (memcmp(instance_before, m, sizeof instance_before) != 0) {
                fprintf(stderr, "profile row %lu: the refused load changed the instance\n",
                        (unsigned long)i);
                check_failures++;
            }
            continue;
        }
        cogmaster_info info = info_of(m);
        CHECK_EQ_U64(info.region, rows[i].want_region);
        CHECK_EQ_U64(info.tv, rows[i].want_tv);
        int pal = rows[i].want_tv == COGMASTER_TV_PAL;
        CHECK_EQ_U64(info.fps_num, pal ? 13300856u : 1640625u);
        CHECK_EQ_U64(info.fps_den, pal ? 267615u : 27379u);
        run_frames(m, 1);
        CHECK_EQ_U64(m->ticks, pal ? 214092u : 179208u);
    }

    /* A size tag at the first released size reads region and tv as AUTO,
       even where the old struct's tail padding holds them. */
    cogmaster_media media = make_media(COGMASTER_MACHINE_SMS2);
    media.region = COGMASTER_REGION_JAPAN;
    media.tv = COGMASTER_TV_PAL;
    uint8_t *b = tagged(&media, (uint32_t)sizeof media, 4u * P);
    CHECK_EQ_U64(cogmaster_load(m, (const cogmaster_media *)b), COGMASTER_STATUS_OK);
    cogmaster_info info = info_of(m);
    CHECK_EQ_U64(info.region, COGMASTER_REGION_EXPORT);
    CHECK_EQ_U64(info.tv, COGMASTER_TV_NTSC);
    cogmaster_destroy(m);
}

/* After load the CPU and RAM are as the BIOS leaves them, with no BIOS code
   in RAM: SP $DFF0, $C000 = $AB (Master System only), PC 0. */
static void test_boot_handoff(void)
{
    fill_big();
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        return;
    }
    cogmaster_media media = image_media(big, IMAGE_MIN, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(m->z80.sp, 0xDFF0);
    CHECK_EQ_HEX(m->z80.pc, 0x0000);
    CHECK_EQ_HEX(m->z80.reg[COGMASTER__Z80_A], 0xFF);
    CHECK_EQ_HEX(m->z80.reg[COGMASTER__Z80_F], 0xFF);
    CHECK_EQ_U64(m->z80.im, 0);
    CHECK_EQ_U64(m->z80.iff1, 0);
    CHECK_EQ_U64(m->z80.iff2, 0);
    CHECK_EQ_U64(m->z80.halted, 0);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xC000u), 0xAB);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xE000u), 0xAB);
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xAB);
    CHECK_EQ_HEX(m->io.io_ctrl, 0xFF);
    uint32_t written = 0;
    for (size_t i = 1; i < sizeof m->ram; i++) {
        if (m->ram[i] != 0u) {
            written++;
        }
    }
    CHECK_EQ_U64(written, 0);
    media = image_media(big, IMAGE_MIN, COGMASTER_MACHINE_GG);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(m->z80.sp, 0xDFF0);
    CHECK_EQ_HEX(m->ram[0], 0x00);
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xA8);
    CHECK_EQ_HEX(m->io.io_ctrl, 0xFF);
    cogmaster_destroy(m);
}

/* The addresses the peek program reads with LD A,(nn): slot 0 inside and
   past its fixed first 1 KiB, slots 1 and 2, work RAM and its mirror, and
   the mapper register's mirror at $FFFF. */
static const uint16_t peek_at[] = {0x0000u, 0x03FFu, 0x0400u, 0x4123u, 0x7FFFu,
                                   0x8456u, 0xBFFFu, 0xC000u, 0xE000u, 0xFFFFu};
#define PEEK_COPY 0xC800u /* where the program stores what it read */

/* A 128 KiB Sega image: each byte is a function of its page and offset, and
   at $0000 a program that selects bank 3 in slot 2, copies each peek_at
   byte to PEEK_COPY, writes $5A to VRAM $0000, sets a VRAM read (which fills
   the read-ahead buffer) and spins with interrupts off. */
static size_t peek_image(void)
{
    size_t n = 8u * PAGE;
    for (size_t i = 0; i < n; i++) {
        big[i] = (uint8_t)((i >> 14) * 0x11u + (i & 0xFFu) * 7u);
    }
    uint8_t *p = big;
    static const uint8_t head[] = {0xF3u, 0x3Eu, 0x03u, 0x32u, 0xFFu, 0xFFu};
    memcpy(p, head, sizeof head);
    p += sizeof head;
    for (size_t k = 0; k < sizeof peek_at / sizeof peek_at[0]; k++) {
        uint16_t to = (uint16_t)(PEEK_COPY + k);
        const uint8_t copy[] = {0x3Au, (uint8_t)(peek_at[k] & 0xFFu), (uint8_t)(peek_at[k] >> 8),
                                0x32u, (uint8_t)(to & 0xFFu),         (uint8_t)(to >> 8)};
        memcpy(p, copy, sizeof copy);
        p += sizeof copy;
    }
    static const uint8_t tail[] = {0x3Eu, 0x00u, 0xD3u, 0xBFu, 0x3Eu, 0x40u, 0xD3u, 0xBFu,
                                   0x3Eu, 0x5Au, 0xD3u, 0xBEu, 0x3Eu, 0x00u, 0xD3u, 0xBFu,
                                   0x3Eu, 0x00u, 0xD3u, 0xBFu, 0x18u, 0xFEu};
    memcpy(p, tail, sizeof tail);
    return n;
}

/* Saves m's state into a new buffer of *n bytes; NULL on failure. */
static uint8_t *peek_save(const cogmaster *m, size_t *n)
{
    *n = cogmaster__state_size(m);
    uint8_t *buf = (uint8_t *)malloc(*n != 0u ? *n : 1u);
    if (buf == NULL || cogmaster__state_save(m, buf, *n) != COGMASTER_STATUS_OK) {
        free(buf);
        return NULL;
    }
    return buf;
}

/* Peeks all 64 KiB and checks the saved state is byte for byte the same. */
static void expect_peek_pure(const char *what, cogmaster *m)
{
    size_t na = 0;
    size_t nb = 0;
    uint8_t *a = peek_save(m, &na);
    struct cogmaster__counters before = m->counters;
    for (uint32_t addr = 0; addr <= 0xFFFFu; addr++) {
        (void)cogmaster_peek(m, COGMASTER_SPACE_BUS, addr);
    }
    uint8_t *b = peek_save(m, &nb);
    CHECK(a != NULL && b != NULL);
    if (a != NULL && b != NULL && (na != nb || memcmp(a, b, na) != 0)) {
        fprintf(stderr, "%s: peeking changed the saved state\n", what);
        check_failures++;
    }
    CHECK(memcmp(&before, &m->counters, sizeof before) == 0);
    free(a);
    free(b);
}

/* Mid-frame, with the frame flag pending and the read-ahead buffer filled,
   peeking every address changes no state, and each peek equals the byte the
   guest's LD A,(nn) read. With $3E taking the cartridge or work RAM off the
   bus, the peek reads $FF and records no fault and counts no open bus. */
static void test_peek_no_side_effects(void)
{
    size_t n = peek_image();
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        return;
    }
    cogmaster_media media = image_media(big, n, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->cart.board, COGMASTER_BOARD_SEGA);
    run_frames(m, 1u);
    for (uint32_t i = 0; i < 101u; i++) {
        cogmaster__z80_step(m);
    }
    CHECK((m->vdp.status & 0x80u) != 0u);
    CHECK_EQ_HEX(m->vdp.buffer, 0x5A);
    CHECK_EQ_U64(m->fault_present, 0);
    for (size_t k = 0; k < sizeof peek_at / sizeof peek_at[0]; k++) {
        uint32_t got = cogmaster_peek(m, COGMASTER_SPACE_BUS, peek_at[k]);
        if (got != m->ram[(PEEK_COPY + k) & (COGMASTER__RAM_SIZE - 1u)]) {
            fprintf(stderr, "peek $%04X is $%02lX, the guest read $%02X\n", (unsigned)peek_at[k],
                    (unsigned long)got, (unsigned)m->ram[(PEEK_COPY + k) & 0x1FFFu]);
            check_failures++;
        }
    }
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0x8000u), big[3u * PAGE]);
    expect_peek_pure("mid-frame", m);

    m->io.mem_ctrl = (uint8_t)(m->io.mem_ctrl | 0x50u);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0x0400u), 0xFF);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC000u), 0xFF);
    expect_peek_pure("cartridge and work RAM off", m);
    CHECK_EQ_U64(m->fault_present, 0);
    cogmaster_destroy(m);
}

/* A NULL instance, an instance with nothing loaded, a space other than the
   bus and an address above $FFFF each peek a value above 255. */
static void test_peek_args(void)
{
    CHECK(cogmaster_peek(NULL, COGMASTER_SPACE_BUS, 0u) > 255u);
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        return;
    }
    CHECK(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0u) > 255u);
    size_t n = peek_image();
    cogmaster_media media = image_media(big, n, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0u), 0xF3);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xFFFFu), m->ram[0x1FFFu]);
    CHECK(cogmaster_peek(m, COGMASTER_SPACE_BUS + 1u, 0u) > 255u);
    CHECK(cogmaster_peek(m, 0xFFFFFFFFu, 0u) > 255u);
    CHECK(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0x10000u) > 255u);
    CHECK(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xFFFFFFFFu) > 255u);
    /* The test card has no cartridge: its slots read $FF. */
    cogmaster_media card = make_media(COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(cogmaster_load(m, &card), COGMASTER_STATUS_OK);
    CHECK_EQ_HEX(cogmaster_peek(m, COGMASTER_SPACE_BUS, 0u), 0xFF);
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"api.first_released_sizes", test_first_released_sizes},
    {"api.size_tags", test_size_tags},
    {"api.statuses", test_statuses},
    {"boot.handoff", test_boot_handoff},
    {"create.allocator", test_allocator},
    {"frame.pitch_wide", test_pitch_wide},
    {"info", test_info},
    {"load.board", test_load_board},
    {"load.profile_rejected", test_profile_rejected},
    {"load.profiles", test_load_profiles},
    {"load.rejected_unchanged", test_load_rejected_unchanged},
    {"load.sizes", test_load_sizes},
    {"no_alloc_after_load", test_no_alloc_after_load},
    {"peek.args", test_peek_args},
    {"peek.no_side_effects", test_peek_no_side_effects},
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
