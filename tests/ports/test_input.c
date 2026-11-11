/* cogmaster_set_input against callers of every size: cogmaster-test-input
   <case>. A caller built against an older header passes a smaller struct
   with its size tag; the core must read exactly that many whole fields and
   not one byte more. The poison cases fill the bytes just past the tag with
   0x7F, which is out of range as an axis, so a read past the tag shows as a
   stored value or a refusal. The exact-size cases hand over a heap block of
   precisely the caller's size; the address-sanitizer lane flags any read or
   write past it. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define TAG_MAX 4096u
#define FIRST_SIZE 32u
#define POISON 0x7Fu
#define PAD_BYTES 16u

static uint8_t image[IMAGE_SIZE];

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

/* The struct as API versions 1 to 11 had it: declared here, not taken from
   the header that has since grown. */
typedef struct first_input {
    uint32_t size;
    uint32_t p1, p2;
    uint32_t pause, reset, start;
    uint32_t port1, port2;
} first_input;

static cogmaster *make(int loaded)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    if (loaded) {
        memset(image, 0x76, sizeof image);
        cogmaster_media media;
        memset(&media, 0, sizeof media);
        media.size = (uint32_t)sizeof media;
        media.image = image;
        media.image_size = sizeof image;
        media.machine = COGMASTER_MACHINE_SMS2;
        media.region = COGMASTER_REGION_EXPORT;
        media.tv = COGMASTER_TV_NTSC;
        CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    }
    return m;
}

/* A full-size object, tag as given, every byte from `from` on poisoned. */
static cogmaster_input poisoned(uint32_t tag, size_t from)
{
    cogmaster_input in;
    memset(&in, 0, sizeof in);
    memset((uint8_t *)&in + from, POISON, sizeof in - from);
    in.size = tag;
    return in;
}

static int pending_axes_zero(const cogmaster *m)
{
    return m->pending_input.x1 == 0 && m->pending_input.y1 == 0 && m->pending_input.x2 == 0 &&
           m->pending_input.y2 == 0;
}

static void test_poison(void)
{
    cogmaster *m = make(1);
    /* Tag 32 with the 16 bytes past it poisoned: only the first eight fields
       are read, so the call succeeds and the axes stay 0. */
    cogmaster_input in = poisoned(FIRST_SIZE, FIRST_SIZE);
    in.p1 = 5u;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_OK);
    CHECK(pending_axes_zero(m));
    CHECK_EQ_U64(m->pending_input.p1, 5u);
    /* The same bytes with the full tag are read as axes, and 0x7F7F7F7F is
       out of range: the poison is visible. */
    in.size = (uint32_t)sizeof in;
    CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_destroy(m);
}

static void test_exact_malloc(void)
{
    cogmaster *m = make(1);
    CHECK_EQ_U64(sizeof(first_input), FIRST_SIZE);
    first_input *in = (first_input *)malloc(FIRST_SIZE);
    if (in == NULL) {
        exit(1);
    }
    memset(in, 0, FIRST_SIZE);
    in->size = FIRST_SIZE;
    in->p1 = 9u;
    in->port2 = COGMASTER_DEVICE_NONE;
    CHECK_EQ_U64(cogmaster_set_input(m, (const cogmaster_input *)in), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(m->pending_input.p1, 9u);
    CHECK_EQ_U64(m->pending_input.port2, COGMASTER_DEVICE_NONE);
    CHECK(pending_axes_zero(m));
    /* An undersized tag on the exact block is refused before any read past
       the tag word. */
    in->size = FIRST_SIZE - 1u;
    CHECK_EQ_U64(cogmaster_set_input(m, (const cogmaster_input *)in),
                 COGMASTER_STATUS_ERR_STRUCT_SIZE);
    free(in);
    cogmaster_destroy(m);
}

static void test_tags(void)
{
    cogmaster *m = make(1);
    cogmaster_input in = poisoned(0u, sizeof in);
    static const uint32_t refused[] = {31u, TAG_MAX + 1u, 0u, 0xFFFFFFFFu};
    for (size_t i = 0; i < sizeof refused / sizeof refused[0]; i++) {
        in.size = refused[i];
        CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_ERR_STRUCT_SIZE);
    }
    /* The largest tag with a zero tail is accepted; a non-zero byte among
       the fields this build does not know is refused. */
    uint8_t *big = (uint8_t *)calloc(TAG_MAX, 1u);
    if (big == NULL) {
        exit(1);
    }
    uint32_t tag = TAG_MAX;
    memcpy(big, &tag, sizeof tag);
    CHECK_EQ_U64(cogmaster_set_input(m, (const cogmaster_input *)(const void *)big),
                 COGMASTER_STATUS_OK);
    big[TAG_MAX - 1u] = 1u;
    CHECK_EQ_U64(cogmaster_set_input(m, (const cogmaster_input *)(const void *)big),
                 COGMASTER_STATUS_ERR_STRUCT_SIZE);
    big[TAG_MAX - 1u] = 0u;
    big[sizeof(cogmaster_input)] = 1u;
    CHECK_EQ_U64(cogmaster_set_input(m, (const cogmaster_input *)(const void *)big),
                 COGMASTER_STATUS_ERR_STRUCT_SIZE);
    free(big);
    /* Tags between the first size and ours copy whole fields only. Axis y1
       out of range is read from tag 40 on, and y2 from tag 48. */
    for (uint32_t t = FIRST_SIZE; t < sizeof in; t++) {
        /* One axis out of range, the rest zero. */
        cogmaster_input obj = poisoned(0u, sizeof obj);
        obj.size = t;
        obj.y1 = 40000;
        CHECK_EQ_U64(cogmaster_set_input(m, &obj),
                     t < 40u ? COGMASTER_STATUS_OK : COGMASTER_STATUS_ERR_ARGUMENT);
        obj.y1 = 0;
        obj.x2 = 40000;
        CHECK_EQ_U64(cogmaster_set_input(m, &obj),
                     t < 44u ? COGMASTER_STATUS_OK : COGMASTER_STATUS_ERR_ARGUMENT);
        obj.x2 = 0;
        obj.y2 = 40000;
        CHECK_EQ_U64(cogmaster_set_input(m, &obj),
                     t < 48u ? COGMASTER_STATUS_OK : COGMASTER_STATUS_ERR_ARGUMENT);
        obj.y2 = 0;
        obj.x1 = 40000;
        CHECK_EQ_U64(cogmaster_set_input(m, &obj),
                     t < 36u ? COGMASTER_STATUS_OK : COGMASTER_STATUS_ERR_ARGUMENT);
    }
    cogmaster_destroy(m);
}

/* The saved state and the pending input, before and after a call that must
   change nothing. */
static void refuse(cogmaster *m, const cogmaster_input *in, uint32_t expect)
{
    size_t size = cogmaster__state_size(m);
    uint8_t *before = (uint8_t *)malloc(size);
    uint8_t *after = (uint8_t *)malloc(size);
    if (before == NULL || after == NULL) {
        exit(1);
    }
    cogmaster_input pending = m->pending_input;
    CHECK_EQ_U64(cogmaster__state_save(m, before, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_set_input(m, in), expect);
    CHECK_EQ_U64(cogmaster__state_save(m, after, size), COGMASTER_STATUS_OK);
    CHECK(memcmp(before, after, size) == 0);
    CHECK(memcmp(&pending, &m->pending_input, sizeof pending) == 0);
    free(before);
    free(after);
}

static void test_refusals(void)
{
    cogmaster *m = make(1);
    /* Something non-default is pending, so a refusal that wrote would show. */
    cogmaster_input good = poisoned(0u, sizeof good);
    good.size = (uint32_t)sizeof good;
    good.p1 = 21u;
    good.p2 = 42u;
    CHECK_EQ_U64(cogmaster_set_input(m, &good), COGMASTER_STATUS_OK);

    cogmaster_input in = good;
    in.port1 = 99u;
    refuse(m, &in, COGMASTER_STATUS_ERR_ARGUMENT);
    in = good;
    in.port2 = 99u;
    refuse(m, &in, COGMASTER_STATUS_ERR_ARGUMENT);
    in = good;
    in.p1 = 64u;
    refuse(m, &in, COGMASTER_STATUS_ERR_ARGUMENT);
    in = good;
    in.pause = 2u;
    refuse(m, &in, COGMASTER_STATUS_ERR_ARGUMENT);
    in = good;
    in.x2 = 32768;
    refuse(m, &in, COGMASTER_STATUS_ERR_ARGUMENT);
    in = good;
    in.size = 31u;
    refuse(m, &in, COGMASTER_STATUS_ERR_STRUCT_SIZE);
    in = good;
    in.size = TAG_MAX + 1u;
    refuse(m, &in, COGMASTER_STATUS_ERR_STRUCT_SIZE);
    refuse(m, NULL, COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_set_input(NULL, &good), COGMASTER_STATUS_ERR_ARGUMENT);
    cogmaster_destroy(m);

    /* Nothing loaded: the call is refused and the instance is untouched. */
    cogmaster *empty = make(0);
    CHECK_EQ_U64(cogmaster_set_input(empty, &good), COGMASTER_STATUS_ERR_NO_CONTENT);
    cogmaster_destroy(empty);
}

/* Axis 0 to 3 is x1, y1, x2, y2; the other three are zeroed. */
static void set_axis(cogmaster_input *in, uint32_t axis, int32_t value)
{
    int32_t axes[4] = {0, 0, 0, 0};
    axes[axis] = value;
    memcpy((uint8_t *)in + offsetof(cogmaster_input, x1), axes, sizeof axes);
}

static void test_range(void)
{
    cogmaster *m = make(1);
    static const int32_t bad[] = {INT32_MIN, INT32_MIN + 1, INT32_MAX, 32768, -32768};
    static const int32_t good[] = {32767, -32767, 0, 1, -1};
    for (uint32_t axis = 0u; axis < 4u; axis++) {
        for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
            cogmaster_input in = poisoned(0u, sizeof in);
            in.size = (uint32_t)sizeof in;
            set_axis(&in, axis, bad[i]);
            refuse(m, &in, COGMASTER_STATUS_ERR_ARGUMENT);
        }
        for (size_t i = 0; i < sizeof good / sizeof good[0]; i++) {
            cogmaster_input in = poisoned(0u, sizeof in);
            in.size = (uint32_t)sizeof in;
            set_axis(&in, axis, good[i]);
            CHECK_EQ_U64(cogmaster_set_input(m, &in), COGMASTER_STATUS_OK);
            CHECK(pending_axes_zero(m));
        }
    }
    cogmaster_destroy(m);
}

/* A block of exactly `size` bytes with a poisoned pad after it, so a write
   past the requested size shows on every lane and not only under the
   address sanitizer. The exact block is checked by that lane. */
static void check_out(cogmaster *m, int run, uint32_t size)
{
    uint8_t *exact = (uint8_t *)malloc(size);
    uint8_t *padded = (uint8_t *)malloc((size_t)size + PAD_BYTES);
    if (exact == NULL || padded == NULL) {
        exit(1);
    }
    memset(exact, 0, size);
    memcpy(exact, &size, sizeof size);
    memset(padded, 0, size);
    memset(padded + size, POISON, PAD_BYTES);
    memcpy(padded, &size, sizeof size);
    if (run) {
        CHECK_EQ_U64(cogmaster_run_frame(m, (cogmaster_run_result *)(void *)exact),
                     COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_run_frame(m, (cogmaster_run_result *)(void *)padded),
                     COGMASTER_STATUS_OK);
    } else {
        CHECK_EQ_U64(cogmaster_get_info(m, (cogmaster_info *)(void *)exact), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_get_info(m, (cogmaster_info *)(void *)padded), COGMASTER_STATUS_OK);
    }
    for (uint32_t i = 0u; i < PAD_BYTES; i++) {
        CHECK_EQ_U64(padded[size + i], POISON);
    }
    free(exact);
    free(padded);
}

static void test_outputs(void)
{
    cogmaster *m = make(1);
    static const uint32_t run_sizes[] = {12u, 16u, 20u, 24u};
    static const uint32_t info_sizes[] = {60u, 64u, 68u, 76u};
    for (size_t i = 0; i < sizeof run_sizes / sizeof run_sizes[0]; i++) {
        check_out(m, 1, run_sizes[i]);
    }
    for (size_t i = 0; i < sizeof info_sizes / sizeof info_sizes[0]; i++) {
        check_out(m, 0, info_sizes[i]);
    }
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"poison", test_poison}, {"exact_malloc", test_exact_malloc},
    {"tags", test_tags},     {"refusals", test_refusals},
    {"range", test_range},   {"outputs", test_outputs},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-input <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-input: unknown case %s\n", argv[1]);
    return 2;
}
