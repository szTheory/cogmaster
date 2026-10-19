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
