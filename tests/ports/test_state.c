/* The saved state of the controller ports against an independent bounds checker:
   cogmaster-test-ports-state <case>. A state is written, one byte of a device field is changed, the
   body checksum is made good again and the result is loaded; the loader must say what the checker
   written here says, which is derived from the bounds and cross-checks of the controller ports
   (every bound a run can reach and no rule a run cannot) and shares no code with src/state.c.
   A refusal must leave the instance byte-identical; an acceptance must hold the checker's
   invariants, save back to the patched bytes and run a frame. The field order and widths are read
   from the record the library reports; the encoding (little-endian, a signed field with its top
   bit flipped) is the format's, written out again below. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define BUF 393216u        /* a state: the emitted pictures, the RAM and the scalar fields */
#define MAX_FIELDS 240000u /* the same record capacity the core state tests use */
#define CTRL_BASE 0x05u    /* TR as inputs, TH as outputs; the levels are b5 and b7 */
#define FIRST_WAIT 858u    /* TH low before the first nibble: about 80 microseconds of ticks */
#define NEXT_WAIT 429u     /* and between the other nibbles: about 40 microseconds */
#define MAX_ITEMS 96u
#define SEEN_MAX 2048u

static uint8_t image[IMAGE_SIZE];
static uint8_t good[BUF];
static uint8_t bad[BUF];
static uint8_t again[BUF];
static uint8_t snapshot[sizeof(struct cogmaster)];
static struct cogmaster__state_field fields[MAX_FIELDS];
static uint32_t level[2]; /* the TH levels the helper has written, per port */

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

/* ---- instances ---- */

static void drive_th(cogmaster *m, uint64_t tick)
{
    m->ticks = tick;
    cogmaster__bus_sync(m);
    cogmaster__io_out(m, 0x3Fu, (uint8_t)(CTRL_BASE | (level[0] << 5) | (level[1] << 7)));
}

static void th(cogmaster *m, uint32_t port, uint32_t to, uint64_t tick)
{
    level[port] = to;
    drive_th(m, tick);
}

static cogmaster_status send(cogmaster *m, uint32_t d1, uint32_t d2, uint32_t p1, uint32_t p2,
                             int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    cogmaster_input in;
    memset(&in, 0, sizeof in);
    in.size = (uint32_t)sizeof in;
    in.port1 = d1;
    in.port2 = d2;
    in.p1 = p1;
    in.p2 = p2;
    in.x1 = x1;
    in.y1 = y1;
    in.x2 = x2;
    in.y2 = y2;
    return cogmaster_set_input(m, &in);
}

static void frame(cogmaster *m)
{
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
}

struct cfg {
    const char *name;
    uint32_t machine, region, tv;
    void (*prep)(cogmaster *m);
};

/* A new instance of the profile with the halt image loaded and both TH lines driven high. */
static cogmaster *build(const struct cfg *c)
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
    memset(image, 0x76u, sizeof image);
    cogmaster_media md;
    memset(&md, 0, sizeof md);
    md.size = (uint32_t)sizeof md;
    md.image = image;
    md.image_size = sizeof image;
    md.machine = c->machine;
    md.region = c->region;
    md.tv = c->tv;
    CHECK_EQ_U64(cogmaster_load(m, &md), COGMASTER_STATUS_OK);
    level[0] = level[1] = 1u;
    drive_th(m, 0u);
    return m;
}

/* Two Sports Pads, both at step 2 with a latched byte and motion left over, motion pending. */
static void prep_sports(cogmaster *m)
{
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0x13u, 0x25u,
                      300, -250, 40, 50),
                 COGMASTER_STATUS_OK);
    frame(m);
    uint64_t t = m->ticks + 20u;
    th(m, 0u, 0u, t);
    th(m, 0u, 1u, t + FIRST_WAIT);
    th(m, 1u, 0u, t + FIRST_WAIT + 10u);
    th(m, 1u, 1u, t + 2u * FIRST_WAIT + 10u);
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_SPORTS_PAD, COGMASTER_DEVICE_SPORTS_PAD, 0x13u, 0x25u, 7,
                      -8, 9, 10),
                 COGMASTER_STATUS_OK);
}

/* A Paddle off centre with motion pending, and a Sports Pad at step 1 on the other port. */
static void prep_paddle(cogmaster *m)
{
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_SPORTS_PAD, 0u, 0u, 70, 0, 25, -30),
        COGMASTER_STATUS_OK);
    frame(m);
    th(m, 1u, 0u, m->ticks + 20u);
    CHECK_EQ_U64(
        send(m, COGMASTER_DEVICE_PADDLE, COGMASTER_DEVICE_SPORTS_PAD, 0x11u, 0x22u, 5, 0, 6, 7),
        COGMASTER_STATUS_OK);
}

static void prep_gg(cogmaster *m)
{
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_NONE, 0x15u, 0x2Au, 0, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
}

/* The eye in force differs from the eye the last frame reported. */
static void prep_eye_a(cogmaster *m)
{
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, 0x3Fu, 0u, 0, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
    m->io.glasses = 1u;
    m->io.glasses_frame = 2u;
}

static void prep_eye_b(cogmaster *m)
{
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_NONE, 0u, 0u, 0, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
    m->io.glasses = 2u;
    m->io.glasses_frame = 1u;
}

static void prep_japan_pad(cogmaster *m)
{
    CHECK_EQ_U64(send(m, COGMASTER_DEVICE_PAD, COGMASTER_DEVICE_PAD, 0x0Fu, 0x30u, 0, 0, 0, 0),
                 COGMASTER_STATUS_OK);
    frame(m);
}

enum { SPORTS, PADDLE, GG, EYE_A, EYE_B, JAPAN_PAD, CFG_COUNT };

static const struct cfg cfgs[CFG_COUNT] = {
    {"sports", COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC, prep_sports},
    {"paddle", COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC, prep_paddle},
    {"game gear", COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC, prep_gg},
    {"eye a", COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC, prep_eye_a},
    {"eye b", COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL, prep_eye_b},
    {"japan pad", COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC,
     prep_japan_pad},
};

/* ---- the state as bytes ---- */

static size_t save(cogmaster *m, uint8_t *buf)
{
    size_t size = cogmaster__state_size(m);
    CHECK(size > 64u && size <= BUF);
    if (size <= 64u || size > BUF) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_save(m, buf, size), COGMASTER_STATUS_OK);
    return size;
}

static uint64_t get_le(const uint8_t *p, uint32_t width)
{
    uint64_t v = 0u;
    for (uint32_t i = 0u; i < width; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0u; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

/* A signed field is stored as the value with its top bit flipped, in the field's width. */
static int64_t unflip(uint64_t raw, uint32_t width)
{
    uint64_t top = UINT64_C(1) << (8u * width - 1u);
    uint64_t v = raw ^ top;
    return v >= top ? (int64_t)v - (int64_t)(top << 1) : (int64_t)v;
}

static uint64_t flip(int64_t v, uint32_t width)
{
    uint64_t top = UINT64_C(1) << (8u * width - 1u);
    return ((uint64_t)v ^ top) & ((top << 1) - 1u);
}

/* Where each field the checker reads sits in the state, from the record. */
struct item {
    const char *name;
    uint32_t k; /* which of the fields of this name, in the order of the record */
    uint32_t off, width;
    int is_signed, fixed; /* fixed: read but never mutated */
};

struct layout {
    struct item it[MAX_ITEMS];
    uint32_t n;
};

static const struct {
    const char *name;
    int is_signed, fixed;
} watched[] = {
    {"machine", 0, 1},    {"region", 0, 1},           {"ticks", 0, 1},       {"pending_x1", 1, 0},
    {"pending_y1", 1, 0}, {"pending_x2", 1, 0},       {"pending_y2", 1, 0},  {"active_x1", 1, 0},
    {"active_y1", 1, 0},  {"active_x2", 1, 0},        {"active_y2", 1, 0},   {"port_edge", 0, 0},
    {"port_acc", 1, 0},   {"port_val", 1, 0},         {"port_step", 0, 0},   {"port_pos", 0, 0},
    {"io_glasses", 0, 0}, {"io_glasses_frame", 0, 0}, {"input_port1", 0, 0}, {"input_port2", 0, 0},
};
#define WATCHED (sizeof watched / sizeof watched[0])

static void build_layout(struct layout *L, size_t n)
{
    uint32_t seen[WATCHED];
    memset(seen, 0, sizeof seen);
    memset(L, 0, sizeof *L);
    for (size_t i = 0u; i < n; i++) {
        for (size_t j = 0u; j < WATCHED; j++) {
            if (strcmp(fields[i].name, watched[j].name) != 0) {
                continue;
            }
            if (L->n >= MAX_ITEMS) {
                CHECK(L->n < MAX_ITEMS);
                return;
            }
            struct item *it = &L->it[L->n++];
            it->name = watched[j].name;
            it->k = seen[j]++;
            it->off = fields[i].offset;
            it->width = fields[i].width;
            it->is_signed = watched[j].is_signed;
            it->fixed = watched[j].fixed;
        }
    }
}

static const struct item *item_find(const struct layout *L, const char *name, uint32_t k)
{
    for (uint32_t i = 0u; i < L->n; i++) {
        if (L->it[i].k == k && strcmp(L->it[i].name, name) == 0) {
            return &L->it[i];
        }
    }
    fprintf(stderr, "no state field %s %u\n", name, (unsigned)k);
    check_failures++;
    return NULL;
}

static uint64_t rd(const struct layout *L, const uint8_t *b, const char *name, uint32_t k)
{
    const struct item *it = item_find(L, name, k);
    return it == NULL ? 0u : get_le(b + it->off, it->width);
}

static int64_t rds(const struct layout *L, const uint8_t *b, const char *name, uint32_t k)
{
    const struct item *it = item_find(L, name, k);
    return it == NULL ? 0 : unflip(get_le(b + it->off, it->width), it->width);
}

/* ---- the independent checker ---- */

/* Every value as the checker sees it, all 64 bits wide so a comparison of two is a memcmp. */
struct snap {
    uint64_t machine, region, ticks;
    int64_t pend[2][2], act[2][2];
    uint64_t dev[2][2]; /* [0] the device pending, [1] the device in force; per port */
    uint64_t edge[2];
    int64_t acc[2][2], val[2][2];
    uint64_t step[2], pos[2];
    uint64_t eye, eye_frame;
};

static void snap_from_buffer(struct snap *s, const struct layout *L, const uint8_t *b)
{
    static const char *const pend_names[2][2] = {{"pending_x1", "pending_y1"},
                                                 {"pending_x2", "pending_y2"}};
    static const char *const act_names[2][2] = {{"active_x1", "active_y1"},
                                                {"active_x2", "active_y2"}};
    static const char *const dev_names[2] = {"input_port1", "input_port2"};
    memset(s, 0, sizeof *s);
    s->machine = rd(L, b, "machine", 0u);
    s->region = rd(L, b, "region", 0u);
    s->ticks = rd(L, b, "ticks", 0u);
    for (uint32_t p = 0u; p < 2u; p++) {
        for (uint32_t a = 0u; a < 2u; a++) {
            s->pend[p][a] = rds(L, b, pend_names[p][a], 0u);
            s->act[p][a] = rds(L, b, act_names[p][a], 0u);
            s->acc[p][a] = rds(L, b, "port_acc", 2u * p + a);
            s->val[p][a] = rds(L, b, "port_val", 2u * p + a);
        }
        s->dev[p][0] = rd(L, b, dev_names[p], 0u);
        s->dev[p][1] = rd(L, b, dev_names[p], 1u);
        s->edge[p] = rd(L, b, "port_edge", p);
        s->step[p] = rd(L, b, "port_step", p);
        s->pos[p] = rd(L, b, "port_pos", p);
    }
    s->eye = rd(L, b, "io_glasses", 0u);
    s->eye_frame = rd(L, b, "io_glasses_frame", 0u);
}

/* The same values read straight from the instance, in the order the checker keeps them. */
static void snap_from_instance(struct snap *s, const cogmaster *m)
{
    memset(s, 0, sizeof *s);
    s->machine = m->machine;
    s->region = m->region;
    s->ticks = m->ticks;
    const cogmaster_input *pi = &m->pending_input;
    const cogmaster_input *ai = &m->input;
    s->pend[0][0] = pi->x1;
    s->pend[0][1] = pi->y1;
    s->pend[1][0] = pi->x2;
    s->pend[1][1] = pi->y2;
    s->act[0][0] = ai->x1;
    s->act[0][1] = ai->y1;
    s->act[1][0] = ai->x2;
    s->act[1][1] = ai->y2;
    s->dev[0][0] = pi->port1;
    s->dev[1][0] = pi->port2;
    s->dev[0][1] = ai->port1;
    s->dev[1][1] = ai->port2;
    for (uint32_t p = 0u; p < 2u; p++) {
        s->edge[p] = m->ports[p].edge;
        s->step[p] = m->ports[p].step;
        s->pos[p] = m->ports[p].pos;
        for (uint32_t a = 0u; a < 2u; a++) {
            s->acc[p][a] = m->ports[p].acc[a];
            s->val[p][a] = m->ports[p].val[a];
        }
    }
    s->eye = m->io.glasses;
    s->eye_frame = m->io.glasses_frame;
}

static int in_range(int64_t v, int64_t lo, int64_t hi)
{
    return v >= lo && v <= hi;
}

/* A Game Gear has no ports to attach anything to but a pad; a Paddle exists only on a Japanese
   console (SMS-CARTRIDGES-AND-PERIPHERALS section 3.4 and the two after it). */
static int available(const struct snap *s, uint64_t device)
{
    int gg = s->machine == COGMASTER_MACHINE_GG;
    if (device == COGMASTER_DEVICE_PAD || device == COGMASTER_DEVICE_NONE) {
        return 1;
    }
    if (device == COGMASTER_DEVICE_SPORTS_PAD) {
        return !gg;
    }
    if (device == COGMASTER_DEVICE_PADDLE) {
        return !gg && s->region == COGMASTER_REGION_JAPAN;
    }
    return 0;
}

static int model_ok(const struct snap *s)
{
    for (uint32_t p = 0u; p < 2u; p++) {
        int ok = in_range(s->pend[p][0], -32767, 32767) && in_range(s->pend[p][1], -32767, 32767);
        ok = ok && s->act[p][0] == 0 && s->act[p][1] == 0;
        ok = ok && s->edge[p] <= s->ticks && s->step[p] <= 4u && s->pos[p] <= 255u;
        for (uint32_t a = 0u; a < 2u; a++) {
            ok = ok && in_range(s->acc[p][a], -1023, 1023) && in_range(s->val[p][a], -127, 127);
        }
        uint64_t next = s->dev[p][0];
        uint64_t held = s->dev[p][1];
        ok = ok && available(s, next) && available(s, held);
        int block = s->edge[p] != 0u || s->acc[p][0] != 0 || s->acc[p][1] != 0 ||
                    s->val[p][0] != 0 || s->val[p][1] != 0 || s->step[p] != 0u;
        if (held == COGMASTER_DEVICE_PAD || held == COGMASTER_DEVICE_NONE) {
            ok = ok && !block && s->pos[p] == 0u;
        } else if (held == COGMASTER_DEVICE_SPORTS_PAD) {
            ok = ok && s->pos[p] == 0u;
        } else if (held == COGMASTER_DEVICE_PADDLE) {
            ok = ok && !block;
        }
        ok = ok && (s->step[p] != 0u || (s->val[p][0] == 0 && s->val[p][1] == 0));
        if (next == COGMASTER_DEVICE_PAD || next == COGMASTER_DEVICE_NONE) {
            ok = ok && s->pend[p][0] == 0 && s->pend[p][1] == 0;
        } else if (next == COGMASTER_DEVICE_PADDLE) {
            ok = ok && s->pend[p][1] == 0;
        }
        if (!ok) {
            return 0;
        }
    }
    if (s->eye > 2u || s->eye_frame > 2u || (s->eye_frame != 0u && s->eye == 0u)) {
        return 0;
    }
    return s->machine != COGMASTER_MACHINE_GG || (s->eye == 0u && s->eye_frame == 0u);
}

/* ---- loading a patched state ---- */

struct counts {
    unsigned accepted, refused;
};

/* Loads the size bytes of patched into target and requires what the checker says of the values in
   those bytes: a refusal leaves target byte-identical, an acceptance leaves values the checker
   holds, saves back to the same bytes and runs a frame. */
static void load_and_judge(const char *what, cogmaster *target, const struct layout *L, size_t size,
                           struct counts *c)
{
    struct snap want;
    snap_from_buffer(&want, L, bad);
    int expect_ok = model_ok(&want);
    if (!expect_ok) {
        memcpy(snapshot, target, sizeof snapshot);
    }
    cogmaster_status st = cogmaster__state_load(target, bad, size);
    if (st != COGMASTER_STATUS_OK) {
        if (st != COGMASTER_STATUS_ERR_ARGUMENT || expect_ok) {
            fprintf(stderr, "%s: refused (status %u) a state the checker holds\n", what,
                    (unsigned)st);
            check_failures++;
        } else if (memcmp(snapshot, target, sizeof snapshot) != 0) {
            fprintf(stderr, "%s: a refusal changed the instance\n", what);
            check_failures++;
        }
        c->refused++;
        return;
    }
    c->accepted++;
    if (!expect_ok) {
        fprintf(stderr, "%s: accepted a state the checker refuses\n", what);
        check_failures++;
        return;
    }
    struct snap got;
    snap_from_instance(&got, target);
    if (memcmp(&got, &want, sizeof got) != 0 || !model_ok(&got)) {
        fprintf(stderr, "%s: the loaded values are not the saved ones\n", what);
        check_failures++;
    }
    size_t resaved = save(target, again);
    if (resaved != size || memcmp(again, bad, size) != 0) {
        fprintf(stderr, "%s: saving the loaded state gives other bytes\n", what);
        check_failures++;
    }
    frame(target);
}

/* Builds the profile's two instances: one carrying the device state, one that only receives. */
struct pair {
    cogmaster *src, *target;
    struct layout L;
    size_t size;
};

static void pair_open(struct pair *pr, const struct cfg *c)
{
    pr->src = build(c);
    c->prep(pr->src);
    pr->target = build(c);
    pr->size = save(pr->src, good);
    size_t n = cogmaster__state_record(pr->src, fields, MAX_FIELDS);
    CHECK(n > 0u && n <= MAX_FIELDS);
    build_layout(&pr->L, n <= MAX_FIELDS ? n : 0u);
    struct snap s;
    snap_from_buffer(&s, &pr->L, good);
    CHECK(model_ok(&s));
    snap_from_instance(&s, pr->src);
    struct snap t;
    snap_from_buffer(&t, &pr->L, good);
    CHECK(memcmp(&s, &t, sizeof s) == 0);
}

static void pair_close(struct pair *pr)
{
    cogmaster_destroy(pr->src);
    cogmaster_destroy(pr->target);
}

/* ---- mutation ---- */

static void test_mutation(void)
{
    struct counts total = {0u, 0u};
    for (uint32_t ci = 0u; ci < CFG_COUNT; ci++) {
        struct pair pr;
        pair_open(&pr, &cfgs[ci]);
        struct counts c = {0u, 0u};
        for (uint32_t i = 0u; i < pr.L.n; i++) {
            const struct item *it = &pr.L.it[i];
            if (it->fixed) {
                continue;
            }
            for (uint32_t b = 0u; b < it->width; b++) {
                uint8_t was = good[it->off + b];
                const uint8_t values[6] = {0x00u, 0x01u, 0x7Fu, 0x80u, 0xFFu, (uint8_t)~was};
                for (uint32_t v = 0u; v < 6u; v++) {
                    memcpy(bad, good, pr.size);
                    bad[it->off + b] = values[v];
                    cogmaster__state_seal(bad, pr.size);
                    load_and_judge(it->name, pr.target, &pr.L, pr.size, &c);
                }
            }
        }
        CHECK(c.accepted > 0u && c.refused > 0u);
        printf("%s: %u accepted, %u refused\n", cfgs[ci].name, c.accepted, c.refused);
        total.accepted += c.accepted;
        total.refused += c.refused;
        pair_close(&pr);
    }
    printf("byte mutations: %u accepted, %u refused\n", total.accepted, total.refused);
    CHECK(total.accepted >= 100u && total.refused >= 1000u);
}

/* ---- outside ---- */

#define PAST_TICKS INT64_MAX /* the value: one tick past the state's own tick count */

struct patch {
    const char *name;
    uint32_t k;
    int64_t v;
};

#define MAX_PATCH 3u

struct outside {
    uint32_t cfg;
    const char *what;
    int refused; /* 0: a control the loader accepts */
    struct patch p[MAX_PATCH];
};

#define D_SPORTS COGMASTER_DEVICE_SPORTS_PAD
#define D_PADDLE COGMASTER_DEVICE_PADDLE

static const struct outside outside_table[] = {
    /* Each bound just outside its range, on both sides where the type has room. */
    {SPORTS, "pending x1 one above", 1, {{"pending_x1", 0u, 32768}}},
    {SPORTS, "pending x1 one below", 1, {{"pending_x1", 0u, -32768}}},
    {SPORTS, "pending y2 one above", 1, {{"pending_y2", 0u, 32768}}},
    {SPORTS, "pending x1 at the edge", 0, {{"pending_x1", 0u, 32767}}},
    {SPORTS, "pending x2 at the other edge", 0, {{"pending_x2", 0u, -32767}}},
    {SPORTS, "active x1 not zero", 1, {{"active_x1", 0u, 1}}},
    {SPORTS, "active y2 not zero", 1, {{"active_y2", 0u, -1}}},
    {SPORTS, "acc one above", 1, {{"port_acc", 0u, 1024}}},
    {SPORTS, "acc one below", 1, {{"port_acc", 3u, -1024}}},
    {SPORTS, "acc at the edge", 0, {{"port_acc", 1u, 1023}}},
    {SPORTS, "val one below", 1, {{"port_val", 0u, -128}}},
    {SPORTS, "val one above", 1, {{"port_val", 2u, 128}}},
    {SPORTS, "val at the edge", 0, {{"port_val", 0u, -127}}},
    {SPORTS, "step 5", 1, {{"port_step", 0u, 5}}},
    {SPORTS, "step 255", 1, {{"port_step", 1u, 255}}},
    {SPORTS, "step 4", 0, {{"port_step", 0u, 4}}},
    {SPORTS, "edge past the tick count", 1, {{"port_edge", 0u, PAST_TICKS}}},
    {SPORTS, "device 4 pending", 1, {{"input_port1", 0u, 4}}},
    {SPORTS, "device 4 in force", 1, {{"input_port2", 1u, 4}}},
    {EYE_A, "eye 3", 1, {{"io_glasses", 0u, 3}}},
    {EYE_A, "eye of the frame 3", 1, {{"io_glasses_frame", 0u, 3}}},
    {EYE_A, "eye of the frame 0", 0, {{"io_glasses_frame", 0u, 0}}},
    /* Each cross-check violated once. */
    {GG, "a pad with an acc", 1, {{"port_acc", 0u, 1}}},
    {GG, "a pad with a val and a step", 1, {{"port_val", 0u, 1}, {"port_step", 0u, 1}}},
    {GG, "no device with an edge", 1, {{"port_edge", 1u, 1}}},
    {GG, "no device with a pos", 1, {{"port_pos", 1u, 1}}},
    {GG, "a pad with a pending axis", 1, {{"pending_x1", 0u, 1}}},
    {GG, "no device with a pending axis", 1, {{"pending_y2", 0u, -1}}},
    {GG, "an eye on a Game Gear", 1, {{"io_glasses", 0u, 1}}},
    {GG, "both eyes on a Game Gear", 1, {{"io_glasses", 0u, 2}, {"io_glasses_frame", 0u, 1}}},
    {GG, "a Sports Pad pending on a Game Gear", 1, {{"input_port1", 0u, D_SPORTS}}},
    {GG, "a Paddle in force on a Game Gear", 1, {{"input_port2", 1u, D_PADDLE}}},
    {SPORTS, "a Sports Pad with a pos", 1, {{"port_pos", 0u, 1}}},
    {SPORTS, "step 0 with a val", 1, {{"port_step", 0u, 0}}},
    {SPORTS,
     "step 0 with no val",
     0,
     {{"port_step", 0u, 0}, {"port_val", 0u, 0}, {"port_val", 1u, 0}}},
    {SPORTS, "a pad pending with an axis", 1, {{"input_port1", 0u, 0}}},
    {SPORTS, "a pad in force with a block", 1, {{"input_port1", 1u, 0}}},
    {SPORTS, "no device in force with a block", 1, {{"input_port2", 1u, 1}}},
    {PADDLE, "a Paddle with a step", 1, {{"port_step", 0u, 1}}},
    {PADDLE, "a Paddle with an edge", 1, {{"port_edge", 0u, 1}}},
    {PADDLE, "a Paddle with an acc", 1, {{"port_acc", 1u, 1}}},
    {PADDLE, "a Paddle with a val", 1, {{"port_val", 0u, 1}}},
    {PADDLE, "a Paddle pending with a y", 1, {{"pending_y1", 0u, 1}}},
    {PADDLE, "a Sports Pad with a pos", 1, {{"port_pos", 1u, 5}}},
    {PADDLE, "a Paddle at its far end", 0, {{"port_pos", 0u, 255}}},
    {PADDLE, "a Paddle pending with an x", 0, {{"pending_x1", 0u, -32767}}},
    {EYE_B, "an eye of the frame with no eye", 1, {{"io_glasses", 0u, 0}}},
    {EYE_B, "the same eye twice", 0, {{"io_glasses_frame", 0u, 2}}},
    {EYE_B, "an eye with none of the frame", 0, {{"io_glasses_frame", 0u, 0}}},
};

static void patch_in(const struct pair *pr, const struct patch *p, uint8_t *buf)
{
    const struct item *it = item_find(&pr->L, p->name, p->k);
    if (it == NULL) {
        return;
    }
    int64_t v = p->v;
    if (v == PAST_TICKS) {
        v = (int64_t)get_le(buf + item_find(&pr->L, "ticks", 0u)->off, 8u) + 1;
    }
    put_le(buf + it->off, it->is_signed ? flip(v, it->width) : (uint64_t)v, it->width);
}

static void test_outside(void)
{
    struct pair pr[CFG_COUNT];
    memset(pr, 0, sizeof pr);
    for (size_t i = 0u; i < sizeof outside_table / sizeof outside_table[0]; i++) {
        const struct outside *o = &outside_table[i];
        if (pr[o->cfg].src == NULL) {
            pair_open(&pr[o->cfg], &cfgs[o->cfg]);
        }
        /* good holds the last opened profile's bytes, so take this profile's again. */
        size_t size = save(pr[o->cfg].src, good);
        memcpy(bad, good, size);
        for (uint32_t k = 0u; k < MAX_PATCH && o->p[k].name != NULL; k++) {
            patch_in(&pr[o->cfg], &o->p[k], bad);
        }
        cogmaster__state_seal(bad, size);
        struct snap s;
        snap_from_buffer(&s, &pr[o->cfg].L, bad);
        if (model_ok(&s) == o->refused) {
            fprintf(stderr, "%s: the checker says %s\n", o->what, o->refused ? "ok" : "refused");
            check_failures++;
        }
        struct counts c = {0u, 0u};
        load_and_judge(o->what, pr[o->cfg].target, &pr[o->cfg].L, size, &c);
        if ((c.refused == 1u) != (o->refused != 0)) {
            fprintf(stderr, "%s: the loader says %s\n", o->what, o->refused ? "ok" : "refused");
            check_failures++;
        }
    }
    for (uint32_t i = 0u; i < CFG_COUNT; i++) {
        if (pr[i].src != NULL) {
            pair_close(&pr[i]);
        }
    }
}

/* ---- sweep ---- */

struct seen_field {
    const char *name;
    uint64_t min, max;
};

static int device_field(const char *name)
{
    return strncmp(name, "pending_", 8u) == 0 || strncmp(name, "active_", 7u) == 0 ||
           strncmp(name, "port_", 5u) == 0 || strncmp(name, "io_glasses", 10u) == 0 ||
           strncmp(name, "input_port", 10u) == 0;
}

/* Every field of the state one past each end of its bound, as state.record_sweep does, with each
   device attached: all of the device fields and one field of every other kind. */
static void test_sweep(void)
{
    static struct seen_field seen[SEEN_MAX];
    size_t swept_total = 0u;
    for (uint32_t ci = 0u; ci < CFG_COUNT; ci++) {
        struct pair pr;
        pair_open(&pr, &cfgs[ci]);
        size_t n = cogmaster__state_record(pr.src, fields, MAX_FIELDS);
        size_t nseen = 0u;
        size_t swept = 0u;
        size_t device = 0u;
        for (size_t i = 0u; i < n && n <= MAX_FIELDS; i++) {
            const struct cogmaster__state_field *fd = &fields[i];
            int is_device = device_field(fd->name);
            if (!is_device) {
                if (i > 0u && fields[i - 1u].name == fd->name && fields[i - 1u].min == fd->min &&
                    fields[i - 1u].max == fd->max) {
                    continue;
                }
                size_t j = 0u;
                while (j < nseen &&
                       !((seen[j].name == fd->name || strcmp(seen[j].name, fd->name) == 0) &&
                         seen[j].min == fd->min && seen[j].max == fd->max)) {
                    j++;
                }
                if (j < nseen) {
                    continue;
                }
                CHECK(nseen < SEEN_MAX);
                if (nseen >= SEEN_MAX) {
                    continue;
                }
                seen[nseen].name = fd->name;
                seen[nseen].min = fd->min;
                seen[nseen].max = fd->max;
                nseen++;
            }
            CHECK(fd->offset + fd->width <= pr.size);
            uint64_t type_max =
                fd->width == 8u ? UINT64_MAX : (UINT64_C(1) << (8u * fd->width)) - 1u;
            const uint64_t outside[2] = {fd->max + 1u, fd->min - 1u};
            const int side[2] = {fd->max < type_max, fd->min > 0u};
            for (uint32_t s = 0u; s < 2u; s++) {
                if (!side[s]) {
                    continue;
                }
                memcpy(bad, good, pr.size);
                put_le(bad + fd->offset, outside[s], fd->width);
                cogmaster__state_seal(bad, pr.size);
                memcpy(snapshot, pr.target, sizeof snapshot);
                if (cogmaster__state_load(pr.target, bad, pr.size) == COGMASTER_STATUS_OK) {
                    fprintf(stderr, "%s: %s accepted outside its bound\n", cfgs[ci].name, fd->name);
                    check_failures++;
                } else if (memcmp(snapshot, pr.target, sizeof snapshot) != 0) {
                    fprintf(stderr, "%s: %s: a refusal changed the instance\n", cfgs[ci].name,
                            fd->name);
                    check_failures++;
                }
                swept++;
                if (is_device) {
                    device++;
                }
            }
        }
        printf("%s: %lu fields, %lu out-of-range values refused, %lu of them device fields\n",
               cfgs[ci].name, (unsigned long)n, (unsigned long)swept, (unsigned long)device);
        CHECK(device >= 30u);
        swept_total += swept;
        pair_close(&pr);
    }
    CHECK(swept_total >= 6u * 100u);
}

/* ---- midsequence ---- */

/* The state at this point of a read sequence loads into a second instance and saves the same
   bytes, and both instances hold the checker's invariants. */
static void step_roundtrip(cogmaster *m, const struct cfg *c, const char *what, uint32_t step0)
{
    CHECK_EQ_U64(m->ports[0].step, step0);
    size_t size = save(m, good);
    cogmaster *other = build(c);
    CHECK_EQ_U64(cogmaster__state_load(other, good, size), COGMASTER_STATUS_OK);
    size_t resaved = save(other, again);
    if (resaved != size || memcmp(good, again, size) != 0) {
        fprintf(stderr, "%s: a loaded state saves other bytes\n", what);
        check_failures++;
    }
    struct snap a, b;
    snap_from_instance(&a, m);
    snap_from_instance(&b, other);
    CHECK(memcmp(&a, &b, sizeof a) == 0);
    CHECK(model_ok(&a) && model_ok(&b));
    frame(other);
    cogmaster_destroy(other);
}

static void midsequence_after_reset(uint32_t kind)
{
    const struct cfg *c = &cfgs[SPORTS];
    cogmaster *m = build(c);
    prep_sports(m);
    CHECK_EQ_U64(cogmaster_reset(m, kind), COGMASTER_STATUS_OK);
    level[0] = level[1] = 1u;
    th(m, 0u, 0u, 0u);
    /* The first fall of a reset instance: the sequence is at step 1 and the edge is still 0. */
    CHECK_EQ_U64(m->ports[0].edge, 0);
    step_roundtrip(m, c, "step 1 after a reset", 1u);
    cogmaster_destroy(m);
}

static void test_midsequence(void)
{
    midsequence_after_reset(COGMASTER_RESET_SOFT);
    midsequence_after_reset(COGMASTER_RESET_POWER);

    /* Every step of one read, each state saved and loaded where it stands. */
    const struct cfg *c = &cfgs[SPORTS];
    cogmaster *m = build(c);
    CHECK_EQ_U64(send(m, D_SPORTS, D_SPORTS, 0x13u, 0x25u, 90, -70, 11, 12), COGMASTER_STATUS_OK);
    frame(m);
    uint64_t t = m->ticks + 20u;
    th(m, 0u, 0u, t);
    step_roundtrip(m, c, "step 1", 1u);
    t += FIRST_WAIT;
    th(m, 0u, 1u, t);
    step_roundtrip(m, c, "step 2", 2u);
    t += NEXT_WAIT;
    th(m, 0u, 0u, t);
    step_roundtrip(m, c, "step 3", 3u);
    t += NEXT_WAIT;
    th(m, 0u, 1u, t);
    step_roundtrip(m, c, "step 4", 4u);
    t += NEXT_WAIT;
    th(m, 0u, 0u, t);
    CHECK(m->ports[0].edge != 0u);
    step_roundtrip(m, c, "step 1 with a stored edge", 1u);
    cogmaster_destroy(m);
}

/* ---- profile_device ---- */

/* A state that a Japanese console wrote and an export console is given is refused by the header;
   one of the export profile's own bytes patched to a Paddle is not, so only the availability
   rule can refuse it. The same patch on a Japanese console is the control. */
static void paddle_patch(const struct cfg *c, int expect_ok)
{
    static const struct {
        const char *what;
        int pending, held;
    } cases[] = {
        {"a Paddle pending", 1, 0}, {"a Paddle in force", 0, 1}, {"a Paddle in both", 1, 1}};
    for (size_t i = 0u; i < sizeof cases / sizeof cases[0]; i++) {
        struct pair pr;
        pair_open(&pr, c);
        /* The patch is made from the record, after the save, as the plan of the test says. */
        memcpy(bad, good, pr.size);
        const struct patch paddle = {"input_port1", 0u, COGMASTER_DEVICE_PADDLE};
        struct patch p = paddle;
        if (cases[i].pending) {
            patch_in(&pr, &p, bad);
        }
        if (cases[i].held) {
            p.k = 1u;
            patch_in(&pr, &p, bad);
        }
        cogmaster__state_seal(bad, pr.size);
        struct counts n = {0u, 0u};
        load_and_judge(cases[i].what, pr.target, &pr.L, pr.size, &n);
        CHECK_EQ_U64(n.accepted, expect_ok ? 1u : 0u);
        CHECK_EQ_U64(n.refused, expect_ok ? 0u : 1u);
        pair_close(&pr);
    }
}

static void test_profile_device(void)
{
    paddle_patch(&cfgs[EYE_B], 0);
    paddle_patch(&cfgs[GG], 0);
    paddle_patch(&cfgs[JAPAN_PAD], 1);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-ports-state <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "mutation") == 0) {
        test_mutation();
    } else if (strcmp(argv[1], "outside") == 0) {
        test_outside();
    } else if (strcmp(argv[1], "sweep") == 0) {
        test_sweep();
    } else if (strcmp(argv[1], "midsequence") == 0) {
        test_midsequence();
    } else if (strcmp(argv[1], "profile_device") == 0) {
        test_profile_device();
    } else {
        return 2;
    }
    CHECK_DONE();
}
