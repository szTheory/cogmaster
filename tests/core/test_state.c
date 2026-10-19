/* The internal state visitor, the first-fault record and reset over the
   test-card instance: cogmaster-test-state <case>. The tests call the
   visitor and the fault recorder directly; the library has no public state
   API yet, and nothing in it records a fault yet. */
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
#define POISON 0xA5u
#define TAG_BUF 8192u

static uint8_t good[BUF];
static uint8_t bad[BUF];
static uint8_t before[BUF];
static uint8_t after[BUF];
static struct cogmaster__state_field fields[MAX_FIELDS];
static uint8_t tag_buf[TAG_BUF];
static uint8_t tag_before[TAG_BUF];
static uint16_t pixels[256u * 240u];
static uint8_t snapshot[sizeof(struct cogmaster)];

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

/* A new instance with the test card of machine loaded. */
static cogmaster *make(uint32_t machine)
{
    cogmaster_config cfg = make_config();
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
    cogmaster_config cfg = make_config();
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

static cogmaster_fault fault_of(const cogmaster *m)
{
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(f.size, sizeof f);
    return f;
}

/* A fault with every field set, distinct per seed. */
static struct cogmaster__fault sample_fault(uint32_t seed)
{
    struct cogmaster__fault f;
    memset(&f, 0, sizeof f);
    f.frame = 7u + seed;
    f.line = 100u + seed;
    f.dot = 200u + seed;
    f.pc = (uint16_t)(0x1234u + seed);
    f.addr = (uint16_t)(0xBEEFu + seed);
    f.opcode = (uint8_t)(0xEDu + seed);
    f.width = (uint8_t)(1u + seed);
    f.device = (uint8_t)(3u + seed);
    f.kind = (uint8_t)(5u + seed);
    return f;
}

static void expect_fault(const cogmaster *m, const struct cogmaster__fault *want)
{
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 1);
    CHECK_EQ_U64(f.frame, want->frame);
    CHECK_EQ_U64(f.line, want->line);
    CHECK_EQ_U64(f.dot, want->dot);
    CHECK_EQ_HEX(f.pc, want->pc);
    CHECK_EQ_HEX(f.addr, want->addr);
    CHECK_EQ_HEX(f.opcode, want->opcode);
    CHECK_EQ_U64(f.width, want->width);
    CHECK_EQ_U64(f.device, want->device);
    CHECK_EQ_U64(f.kind, want->kind);
}

/* The size-tag rule for the fault struct on m: below its size the struct is
   left alone; at any larger size only its own bytes are written. */
static void check_fault_tags(const cogmaster *m)
{
    const uint32_t small[] = {0u, (uint32_t)sizeof(cogmaster_fault) - 1u};
    for (size_t i = 0; i < 2u; i++) {
        memset(tag_buf, POISON, TAG_BUF);
        memcpy(tag_buf, &small[i], sizeof small[i]);
        memcpy(tag_before, tag_buf, TAG_BUF);
        CHECK_EQ_U64(cogmaster_get_fault(m, (cogmaster_fault *)(void *)tag_buf),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE);
        CHECK(memcmp(tag_before, tag_buf, TAG_BUF) == 0);
    }
    const uint32_t big[] = {(uint32_t)sizeof(cogmaster_fault) + 8u, TAG_BUF};
    for (size_t i = 0; i < 2u; i++) {
        memset(tag_buf, POISON, TAG_BUF);
        memcpy(tag_buf, &big[i], sizeof big[i]);
        CHECK_EQ_U64(cogmaster_get_fault(m, (cogmaster_fault *)(void *)tag_buf),
                     COGMASTER_STATUS_OK);
        cogmaster_fault f;
        memcpy(&f, tag_buf, sizeof f);
        CHECK_EQ_U64(f.size, big[i]);
        CHECK_EQ_U64(f.present, 0);
        uint32_t touched = 0;
        for (size_t k = sizeof f; k < TAG_BUF; k++) {
            if (tag_buf[k] != POISON) {
                touched++;
            }
        }
        CHECK_EQ_U64(touched, 0);
    }
}

/* No fault after create or after load, and the size-tag rule holds. */
static void test_fault_empty(void)
{
    cogmaster_config cfg = make_config();
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(fault_of(m).present, 0);
    cogmaster_destroy(m);

    m = make(COGMASTER_MACHINE_GG);
    run_frames(m, 3u);
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 0);
    CHECK_EQ_U64(f.frame, 0);
    CHECK_EQ_U64(f.pc, 0);
    check_fault_tags(m);
    cogmaster_fault out;
    memset(&out, 0, sizeof out);
    out.size = (uint32_t)sizeof out;
    CHECK_EQ_U64(cogmaster_get_fault(NULL, &out), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_get_fault(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_destroy(m);
}

/* Only the first fault since load or reset is kept. */
static void test_fault_first_wins(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    struct cogmaster__fault first = sample_fault(0u);
    struct cogmaster__fault second = sample_fault(1u);
    cogmaster__fault_record(m, &first);
    cogmaster__fault_record(m, &second);
    expect_fault(m, &first);
    cogmaster_destroy(m);
}

/* Both reset kinds clear the fault and return frame and ticks to zero; the
   card keeps running afterwards. */
static void test_fault_reset_clears(void)
{
    const uint32_t kinds[] = {COGMASTER_RESET_SOFT, COGMASTER_RESET_POWER};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = make(COGMASTER_MACHINE_SMS2);
        run_frames(m, 10u);
        struct cogmaster__fault f = sample_fault(0u);
        cogmaster__fault_record(m, &f);
        CHECK_EQ_U64(cogmaster_reset(m, kinds[k]), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(fault_of(m).present, 0);
        size_t size = save(m, good);
        size_t n = record(m);
        const struct cogmaster__state_field *frame = field(n, "frame");
        const struct cogmaster__state_field *ticks = field(n, "ticks");
        if (size != 0u && frame != NULL && ticks != NULL) {
            CHECK_EQ_U64(get_le(good + frame->offset, 8u), 0);
            CHECK_EQ_U64(get_le(good + ticks->offset, 8u), 0);
        }
        /* A fault after the reset is recorded again. */
        struct cogmaster__fault g = sample_fault(2u);
        cogmaster__fault_record(m, &g);
        expect_fault(m, &g);
        run_frames(m, 1u);
        if (save(m, good) != 0u && frame != NULL) {
            CHECK_EQ_U64(get_le(good + frame->offset, 8u), 1);
        }
        cogmaster_destroy(m);
    }
}

/* A recorded fault survives save and load into another instance, and a
   state without one clears the target's. */
static void test_fault_state_round_trip(void)
{
    cogmaster *a = make(COGMASTER_MACHINE_GG);
    run_frames(a, 4u);
    struct cogmaster__fault f = sample_fault(0u);
    cogmaster__fault_record(a, &f);
    cogmaster *b = make(COGMASTER_MACHINE_GG);
    size_t size = save(a, good);
    if (size != 0u) {
        CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
        expect_fault(b, &f);
        CHECK_EQ_U64(save(b, after), size);
        CHECK(memcmp(good, after, size) == 0);
    }
    cogmaster *clean = make(COGMASTER_MACHINE_GG);
    size = save(clean, good);
    if (size != 0u) {
        CHECK_EQ_U64(cogmaster__state_load(b, good, size), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(fault_of(b).present, 0);
        /* A fault field set while no fault is present is refused, though the
           value is inside the field's own bounds. */
        size_t n = record(clean);
        const struct cogmaster__state_field *line = field(n, "fault_line");
        if (line != NULL) {
            memcpy(bad, good, size);
            put_le(bad + line->offset, 5u, line->width);
            cogmaster__state_seal(bad, size);
            CHECK_EQ_U64(cogmaster__state_load(b, bad, size), COGMASTER_STATUS_ERR_ARGUMENT);
        }
    }
    cogmaster_destroy(a);
    cogmaster_destroy(b);
    cogmaster_destroy(clean);
}

/* The loaded instance's saved bytes and field equal the snapshot taken by
   take_snapshot. */
static size_t snapshot_size;

static void take_snapshot(const cogmaster *m)
{
    snapshot_size = save(m, before);
    memcpy(pixels, m->field, sizeof pixels);
}

static void expect_unchanged(const char *what, cogmaster_status got, cogmaster_status want,
                             const cogmaster *m)
{
    if (got != want) {
        fprintf(stderr, "%s: status %lu, expected %lu\n", what, (unsigned long)got,
                (unsigned long)want);
        check_failures++;
    }
    if (save(m, after) != snapshot_size || memcmp(before, after, snapshot_size) != 0 ||
        memcmp(pixels, m->field, sizeof pixels) != 0) {
        fprintf(stderr, "%s: the instance changed\n", what);
        check_failures++;
    }
}

/* Before a load the whole instance is compared. */
static void expect_untouched(const char *what, cogmaster_status got, cogmaster_status want,
                             const cogmaster *m)
{
    if (got != want) {
        fprintf(stderr, "%s: status %lu, expected %lu\n", what, (unsigned long)got,
                (unsigned long)want);
        check_failures++;
    }
    if (memcmp(snapshot, m, sizeof snapshot) != 0) {
        fprintf(stderr, "%s: the instance changed\n", what);
        check_failures++;
    }
}

static void *null_alloc(void *user, size_t size)
{
    (void)user;
    (void)size;
    return NULL;
}

/* Every error a public call returns leaves the instance as it was. */
static void test_error_changes_nothing(void)
{
    cogmaster *m = make(COGMASTER_MACHINE_SMS2);
    run_frames(m, 7u);
    struct cogmaster__fault f = sample_fault(0u);
    cogmaster__fault_record(m, &f);
    take_snapshot(m);

    /* create: every refusal leaves *out and the other instance alone. */
    cogmaster_config cfg = make_config();
    cogmaster *out = m;
    expect_unchanged("create without config", cogmaster_create(NULL, &out),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("create without out", cogmaster_create(&cfg, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    cfg.size = 0u;
    expect_unchanged("create with size 0", cogmaster_create(&cfg, &out),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cfg = make_config();
    cfg.allocator.alloc = NULL;
    expect_unchanged("create without alloc", cogmaster_create(&cfg, &out),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    cfg.allocator.alloc = null_alloc;
    expect_unchanged("create out of memory", cogmaster_create(&cfg, &out),
                     COGMASTER_STATUS_ERR_NO_MEMORY, m);
    CHECK(out == m);

    /* load */
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.machine = COGMASTER_MACHINE_GG;
    expect_unchanged("load without instance", cogmaster_load(NULL, &media),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("load without media", cogmaster_load(m, NULL), COGMASTER_STATUS_ERR_ARGUMENT,
                     m);
    media.size = 0u;
    expect_unchanged("load with size 0", cogmaster_load(m, &media),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    media.size = (uint32_t)sizeof media;
    static const uint8_t image[1] = {0};
    media.image = image;
    media.image_size = 1u;
    expect_unchanged("load an image", cogmaster_load(m, &media), COGMASTER_STATUS_ERR_ARGUMENT, m);
    media.image = NULL;
    media.image_size = 0u;
    media.machine = 3u;
    expect_unchanged("load another machine", cogmaster_load(m, &media),
                     COGMASTER_STATUS_ERR_PROFILE_REJECTED, m);

    /* reset */
    expect_unchanged("reset without instance", cogmaster_reset(NULL, COGMASTER_RESET_SOFT),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("reset kind 2", cogmaster_reset(m, 2u), COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("reset kind max", cogmaster_reset(m, 0xFFFFFFFFu),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);

    /* run_frame */
    cogmaster_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    expect_unchanged("run_frame without instance", cogmaster_run_frame(NULL, &result),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    result.size = 0u;
    expect_unchanged("run_frame with size 0", cogmaster_run_frame(m, &result),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);

    /* get_frame */
    cogmaster_frame io;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pitch_bytes = 512u;
    static uint16_t sink[256u * 240u];
    io.pixels = sink;
    expect_unchanged("get_frame without instance", cogmaster_get_frame(NULL, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_frame without request", cogmaster_get_frame(m, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.size = 0u;
    expect_unchanged("get_frame with size 0", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    io.size = (uint32_t)sizeof io;
    io.flags = 2u;
    expect_unchanged("get_frame unknown flag", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.flags = 0u;
    io.pixels = NULL;
    expect_unchanged("get_frame without pixels", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.pixels = sink;
    io.pitch_bytes = 65537u;
    expect_unchanged("get_frame pitch too wide", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    io.pitch_bytes = 511u;
    expect_unchanged("get_frame pitch short", cogmaster_get_frame(m, &io),
                     COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL, m);

    /* get_info, get_fault, get_versions */
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    expect_unchanged("get_info without instance", cogmaster_get_info(NULL, &info),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_info without out", cogmaster_get_info(m, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_info with size 0", cogmaster_get_info(m, &info),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cogmaster_fault fault;
    memset(&fault, 0, sizeof fault);
    expect_unchanged("get_fault without instance", cogmaster_get_fault(NULL, &fault),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_fault without out", cogmaster_get_fault(m, NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_fault with size 0", cogmaster_get_fault(m, &fault),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cogmaster_versions versions;
    memset(&versions, 0, sizeof versions);
    expect_unchanged("get_versions without out", cogmaster_get_versions(NULL),
                     COGMASTER_STATUS_ERR_ARGUMENT, m);
    expect_unchanged("get_versions with size 0", cogmaster_get_versions(&versions),
                     COGMASTER_STATUS_ERR_STRUCT_SIZE, m);
    cogmaster_destroy(m);

    /* Before a load: the calls that need content refuse it and change no
       byte of the instance. */
    cfg = make_config();
    cogmaster *e = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &e), COGMASTER_STATUS_OK);
    memcpy(snapshot, e, sizeof snapshot);
    result.size = (uint32_t)sizeof result;
    expect_untouched("run_frame before load", cogmaster_run_frame(e, &result),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    io.pitch_bytes = 512u;
    expect_untouched("get_frame before load", cogmaster_get_frame(e, &io),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    info.size = (uint32_t)sizeof info;
    expect_untouched("get_info before load", cogmaster_get_info(e, &info),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    expect_untouched("reset before load", cogmaster_reset(e, COGMASTER_RESET_POWER),
                     COGMASTER_STATUS_ERR_NO_CONTENT, e);
    media.machine = 3u;
    expect_untouched("load another machine before load", cogmaster_load(e, &media),
                     COGMASTER_STATUS_ERR_PROFILE_REJECTED, e);
    cogmaster_destroy(e);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"api.error_changes_nothing", test_error_changes_nothing},
    {"fault.empty_after_create", test_fault_empty},
    {"fault.first_wins", test_fault_first_wins},
    {"fault.reset_clears", test_fault_reset_clears},
    {"fault.state_round_trip", test_fault_state_round_trip},
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
