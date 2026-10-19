/* The internal state visitor over the test-card instance:
   cogmaster-test-state <case>. The tests call the visitor directly; the
   library has no public state API yet. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define BUF 4096u     /* bytes in each state buffer; the card's state is far smaller */
#define MAX_FIELDS 256u
#define TICKS_PER_FRAME 179208u

static uint8_t good[BUF];
static uint8_t bad[BUF];
static uint8_t before[BUF];
static uint8_t after[BUF];
static struct cogmaster__state_field fields[MAX_FIELDS];

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

/* A new instance with the test card of machine loaded. */
static cogmaster *make(uint32_t machine)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = machine;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

static void run_frames(cogmaster *m, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    }
}

/* Saves m into buf and returns the state size, or 0 after a failed check. */
static size_t save(const cogmaster *m, uint8_t *buf)
{
    size_t size = cogmaster__state_size(m);
    CHECK(size > 64u && size <= BUF);
    if (size <= 64u || size > BUF) {
        return 0u;
    }
    CHECK_EQ_U64(cogmaster__state_save(m, buf, size), COGMASTER_STATUS_OK);
    return size;
}

static uint64_t get_le(const uint8_t *p, uint32_t width)
{
    uint64_t v = 0u;
    for (uint32_t i = 0; i < width; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

/* The fields of m's state; fails the test when there are none. */
static size_t record(const cogmaster *m)
{
    size_t n = cogmaster__state_record(m, fields, MAX_FIELDS);
    CHECK(n > 0u && n <= MAX_FIELDS);
    return n <= MAX_FIELDS ? n : 0u;
}

/* The first field named name, or NULL after a failed check. */
static const struct cogmaster__state_field *field(size_t n, const char *name)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            return &fields[i];
        }
    }
    fprintf(stderr, "no state field named %s\n", name);
    check_failures++;
    return NULL;
}

/* Loads the size bytes of bad into target, which must refuse them and still
   save the bytes it saved before. */
static void expect_refused(const char *what, cogmaster *target, size_t size, size_t target_size)
{
    cogmaster_status st = cogmaster__state_load(target, bad, size);
    if (st == COGMASTER_STATUS_OK) {
        fprintf(stderr, "%s: accepted\n", what);
        check_failures++;
    }
    if (save(target, after) != target_size || memcmp(before, after, target_size) != 0) {
        fprintf(stderr, "%s: the refused load changed the instance\n", what);
        check_failures++;
    }
}

/* save, load into a fresh instance of the same machine, save again: the two
   states are equal byte for byte. */
static void test_round_trip(void)
{
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    const uint32_t frames[] = {0u, 1u, 60u};
    for (size_t k = 0; k < 2u; k++) {
        for (size_t f = 0; f < 3u; f++) {
            cogmaster *a = make(machines[k]);
            run_frames(a, frames[f]);
            size_t size = save(a, good);
            cogmaster *b = make(machines[k]);
            run_frames(b, 3u);
            if (size != 0u) {
                CHECK_EQ_U64(cogmaster__state_size(b), size);
                CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
                CHECK_EQ_U64(save(b, after), size);
                CHECK(memcmp(good, after, size) == 0);
                CHECK_EQ_U64(cogmaster__state_save(a, bad, size - 1u),
                             COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL);
            }
            cogmaster_destroy(a);
            cogmaster_destroy(b);
        }
    }
}

/* Every field set one step outside its bounds is refused, and the refusal
   changes nothing. A field the visitor adds later is swept with no new test
   code. */
static void test_record_sweep(void)
{
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *src = make(machines[k]);
        run_frames(src, 60u);
        cogmaster *target = make(machines[k]);
        run_frames(target, 5u);
        size_t size = save(src, good);
        size_t target_size = save(target, before);
        size_t n = record(src);
        CHECK(n >= 14u);
        size_t swept = 0;
        for (size_t i = 0; i < n && size != 0u; i++) {
            const struct cogmaster__state_field *fd = &fields[i];
            CHECK(fd->offset + fd->width <= size);
            uint64_t type_max = fd->width == 8u ? UINT64_MAX : (UINT64_C(1) << (8u * fd->width)) - 1u;
            const uint64_t outside[2] = {fd->max + 1u, fd->min - 1u};
            const int sweep[2] = {fd->max < type_max, fd->min > 0u};
            for (size_t j = 0; j < 2u; j++) {
                if (!sweep[j]) {
                    continue;
                }
                memcpy(bad, good, size);
                put_le(bad + fd->offset, outside[j], fd->width);
                cogmaster__state_seal(bad, size);
                expect_refused(fd->name, target, size, target_size);
                swept++;
            }
        }
        printf("state fields: %lu recorded, %lu out-of-range values refused\n", (unsigned long)n,
               (unsigned long)swept);
        CHECK(swept >= 14u);
        /* The control: the unchanged state, resealed, loads. */
        if (size != 0u) {
            memcpy(bad, good, size);
            cogmaster__state_seal(bad, size);
            CHECK(memcmp(bad, good, size) == 0);
            CHECK_EQ_U64(cogmaster__state_load(target, bad, size), COGMASTER_STATUS_OK);
        }
        cogmaster_destroy(src);
        cogmaster_destroy(target);
    }
}

/* Header corruption, a wrong length and a bad checksum are each refused and
   change nothing. */
static void test_rejects_header(void)
{
    cogmaster *src = make(COGMASTER_MACHINE_SMS2);
    run_frames(src, 60u);
    cogmaster *target = make(COGMASTER_MACHINE_SMS2);
    run_frames(target, 5u);
    size_t size = save(src, good);
    size_t target_size = save(target, before);
    size_t n = record(src);
    const struct cogmaster__state_field *magic = field(n, "magic");
    const struct cogmaster__state_field *total = field(n, "total_size");
    const struct cogmaster__state_field *header = field(n, "header_size");
    const struct cogmaster__state_field *crc = field(n, "body_crc32");
    const struct cogmaster__state_field *reserved = field(n, "reserved");
    const struct cogmaster__state_field *ticks = field(n, "ticks");
    if (size == 0u || magic == NULL || total == NULL || header == NULL || crc == NULL ||
        reserved == NULL || ticks == NULL) {
        return;
    }
    CHECK_EQ_U64(magic->offset, 0);
    CHECK_EQ_U64(reserved->offset, 52);

    for (uint32_t i = 0; i < 8u; i++) {
        memcpy(bad, good, size);
        bad[magic->offset + i] ^= 0x80u;
        expect_refused("magic", target, size, target_size);
    }
    const uint64_t off_by_one[2] = {size + 1u, size - 1u};
    for (size_t j = 0; j < 2u; j++) {
        memcpy(bad, good, size);
        put_le(bad + total->offset, off_by_one[j], total->width);
        expect_refused("total size field", target, size, target_size);
        memcpy(bad, good, size);
        expect_refused("buffer length", target, (size_t)off_by_one[j], target_size);
    }
    const uint64_t header_sizes[2] = {63u, 65u};
    for (size_t j = 0; j < 2u; j++) {
        memcpy(bad, good, size);
        put_le(bad + header->offset, header_sizes[j], header->width);
        expect_refused("header size", target, size, target_size);
    }
    memcpy(bad, good, size);
    bad[crc->offset] ^= 1u;
    expect_refused("body checksum field", target, size, target_size);
    memcpy(bad, good, size);
    bad[size - 1u] ^= 1u;
    expect_refused("body byte", target, size, target_size);
    for (uint32_t i = 0; i < 12u; i++) {
        memcpy(bad, good, size);
        bad[reserved->offset + i] = 1u;
        expect_refused("reserved byte", target, size, target_size);
    }
    /* Ticks one away from frame x 179,208, both inside their own bounds. */
    memcpy(bad, good, size);
    put_le(bad + ticks->offset, get_le(good + ticks->offset, 8u) + 1u, 8u);
    cogmaster__state_seal(bad, size);
    expect_refused("ticks off the frame count", target, size, target_size);
    /* A state of the other machine. */
    cogmaster *gg = make(COGMASTER_MACHINE_GG);
    run_frames(gg, 60u);
    if (save(gg, bad) == size) {
        expect_refused("other machine", target, size, target_size);
    }
    cogmaster_destroy(gg);

    CHECK_EQ_U64(cogmaster__state_load(target, NULL, size), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster__state_load(NULL, good, size), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster__state_save(NULL, bad, size), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster__state_save(target, NULL, size), COGMASTER_STATUS_ERR_ARGUMENT);

    /* Before a load there is no state to save or to load into. */
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *empty = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &empty), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_size(empty), 0);
    CHECK_EQ_U64(cogmaster__state_record(empty, fields, MAX_FIELDS), 0);
    CHECK_EQ_U64(cogmaster__state_save(empty, bad, BUF), COGMASTER_STATUS_ERR_NO_CONTENT);
    CHECK_EQ_U64(cogmaster__state_load(empty, good, size), COGMASTER_STATUS_ERR_NO_CONTENT);
    cogmaster_destroy(empty);
    cogmaster_destroy(src);
    cogmaster_destroy(target);
}

/* After 60 frames the header's frame field reads 60 and its ticks field
   60 x 179,208. */
static void test_frame_counter(void)
{
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS2, COGMASTER_MACHINE_GG};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = make(machines[k]);
        run_frames(m, 60u);
        size_t size = save(m, good);
        size_t n = record(m);
        const struct cogmaster__state_field *frame = field(n, "frame");
        const struct cogmaster__state_field *ticks = field(n, "ticks");
        if (size != 0u && frame != NULL && ticks != NULL) {
            CHECK_EQ_U64(frame->width, 8);
            CHECK_EQ_U64(ticks->width, 8);
            CHECK_EQ_U64(get_le(good + frame->offset, 8u), 60);
            CHECK_EQ_U64(get_le(good + ticks->offset, 8u), 60u * TICKS_PER_FRAME);
            CHECK_EQ_U64(get_le(good + ticks->offset, 8u), 10752480u);
        }
        cogmaster_destroy(m);
    }
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"state.frame_counter", test_frame_counter},
    {"state.record_sweep", test_record_sweep},
    {"state.rejects_header", test_rejects_header},
    {"state.round_trip", test_round_trip},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-state <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-state: unknown case %s\n", argv[1]);
    return 2;
}
