/* Fuzz the controller devices on a Master System or Game Gear instance. After the profile byte
   and ten bytes that say which saved fields to change at the end, each operation is one byte that
   picks what to do and the bytes after it that say how: set_input with an adversarial device or
   axis, a write to $3F or $3E, a read of $DC or $DD, a write to $FFF8-$FFFB, a tick advance around
   the Sports Pad's idle limit, a soft or power reset, a frame, a bright backdrop with the display
   off, or a switch to a video mode the core does not draw with the display on. After every
   operation check_pad() holds the bounds and cross-checks of the controller ports, written here
   from the port model and sharing nothing with src/state.c; a set_input must answer what the model
   says (an unexpected OK on an illegal device aborts), a Light Phaser's stored aim must be the one
   the model canonicalises it to, and a refusal must change nothing. Every operation also runs on a
   twin whose time advances are cut into pieces with a poll of $DC, $DD, $7E or $7F between them:
   the two must save the same bytes at every frame, apart from the counters a poll moves and, after
   the unsupported mode, the gun's own fields. At the end the state is saved and loaded into a
   second instance, and then one to three device or input fields of a copy are changed, resealed
   and loaded into a third: a refusal leaves that instance byte-identical, an acceptance holds the
   bounds, saves the same bytes, loads into a fourth instance and runs a frame.
 */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "fuzz.h"
#include "internal.h"
#include "state.h"

#define IMAGE_SIZE 1024u
#define MAX_OPS 2048u
#define MAX_FRAMES 4u
#define MAX_FIELDS 240000u /* the record capacity the core state tests use */
#define MAX_ITEMS 128u
#define PROFILES 4u
#define CHANGE_BYTES 10u

/* The operations, by the low bits of the operation byte. */
enum {
    OP_SET_INPUT,
    OP_TH,
    OP_TH_AGAIN,
    OP_CTRL,
    OP_READ,
    OP_GLASSES,
    OP_TICKS,
    OP_RESET,
    OP_FRAME,
    OP_TICKS_AGAIN,
    OP_PAINT,
    OP_BAD_MODE,
    OP_COUNT
};

static uint8_t image[IMAGE_SIZE];
static struct cogmaster__state_field fields[MAX_FIELDS];
static uint32_t pictures; /* the pictures an input has run through, a bound on the sensor's work */

static const struct {
    uint32_t machine, region;
} profiles[PROFILES] = {
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_SMS1, COGMASTER_REGION_EXPORT},
    {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN},
    {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT},
};

/* The device codes a set_input is tried with, and the numbers an axis is drawn from: in range,
   at the edge, just outside and the extremes of the type. A selector with its top bit set takes
   a raw little-endian word from the stream instead. The aim's edges are the field's: 255 and 256
   across, 240 and 241 down. */
static const uint32_t device_table[8] = {COGMASTER_DEVICE_PAD,
                                         COGMASTER_DEVICE_NONE,
                                         COGMASTER_DEVICE_PADDLE,
                                         COGMASTER_DEVICE_SPORTS_PAD,
                                         COGMASTER_DEVICE_LIGHT_PHASER,
                                         COGMASTER_DEVICE_LIGHT_PHASER,
                                         COGMASTER_DEVICE_LIGHT_PHASER + 1u,
                                         255u};
#define AXES 20u
static const int32_t axis_table[AXES] = {0,         1,         -1,   3,     -3,     127,   128,
                                         255,       -255,      1000, 32767, -32767, 32768, -32768,
                                         INT32_MIN, INT32_MAX, 256,  257,   240,    241};
/* The tick advances that matter: none, one, and either side of the idle limit. */
static const uint32_t tick_table[5] = {0u, 1u, SPORTS_IDLE_TICKS - 1u, SPORTS_IDLE_TICKS,
                                       SPORTS_IDLE_TICKS + 1u};

static void *fuzz_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void fuzz_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* The bytes of one input, as an operation reads them; a stream that has run out reads zeros. */
struct stream {
    const uint8_t *d;
    size_t n, pos;
};

static uint8_t next8(struct stream *s)
{
    return s->pos < s->n ? s->d[s->pos++] : 0u;
}

static uint32_t next32(struct stream *s)
{
    uint32_t v = 0u;
    for (uint32_t i = 0u; i < 4u; i++) {
        v |= (uint32_t)next8(s) << (8u * i);
    }
    return v;
}

static cogmaster *open_profile(uint32_t index)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = fuzz_alloc;
    cfg.allocator.free = fuzz_free;
    cogmaster *m = NULL;
    if (cogmaster_create(&cfg, &m) != COGMASTER_STATUS_OK) {
        abort();
    }
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = profiles[index].machine;
    media.region = profiles[index].region;
    media.tv = COGMASTER_TV_NTSC;
    if (cogmaster_load(m, &media) != COGMASTER_STATUS_OK) {
        abort();
    }
    return m;
}

/* ---- the checker ---- */

/* What a console of this profile can attach: the Game Gear has no controller ports, and a Paddle
   exists only on a Japanese console. */
static int attachable(const cogmaster *m, uint32_t device)
{
    switch (device) {
    case COGMASTER_DEVICE_PAD:
    case COGMASTER_DEVICE_NONE:
        return 1;
    case COGMASTER_DEVICE_SPORTS_PAD:
    case COGMASTER_DEVICE_LIGHT_PHASER:
        return m->machine != COGMASTER_MACHINE_GG;
    case COGMASTER_DEVICE_PADDLE:
        return m->machine != COGMASTER_MACHINE_GG && m->region == COGMASTER_REGION_JAPAN;
    default:
        return 0;
    }
}

static void expect(int ok)
{
    if (!ok) {
        abort();
    }
}

static int axis_in(int32_t v)
{
    return v >= -32767 && v <= 32767;
}

/* A Light Phaser's aim: a field dot, 1 to 256 across and 1 to 240 down, or 0 0 for none. A pair
   with only one half zero is not one a call or a load can leave. */
static int aim_canonical(int32_t x, int32_t y)
{
    return x >= 0 && x <= 256 && y >= 0 && y <= 240 && (x == 0) == (y == 0);
}

/* The bounds of every device field and the rules that tie them to the device in force, the motion
   to the device pending and the eyes to the machine. A run, a reset and a load reach every pair of
   edge and step the bounds allow, so none is ruled out beyond the edge being a past tick. */
static void check_pad(const cogmaster *m)
{
    const cogmaster_input *pending = &m->pending_input;
    const cogmaster_input *active = &m->input;
    const uint32_t held[2] = {active->port1, active->port2};
    const uint32_t next[2] = {pending->port1, pending->port2};
    const int32_t moved[2][2] = {{pending->x1, pending->y1}, {pending->x2, pending->y2}};
    const int32_t aimed[2][2] = {{active->x1, active->y1}, {active->x2, active->y2}};
    for (uint32_t p = 0u; p < 2u; p++) {
        const struct cogmaster__port *b = &m->ports[p];
        if (held[p] == COGMASTER_DEVICE_LIGHT_PHASER) {
            expect(aim_canonical(aimed[p][0], aimed[p][1]));
        } else {
            expect(aimed[p][0] == 0 && aimed[p][1] == 0);
        }
        expect(axis_in(moved[p][0]) && axis_in(moved[p][1]));
        expect(attachable(m, held[p]) && attachable(m, next[p]));
        expect(b->edge <= m->ticks && b->step <= 4u);
        int gathered = b->edge != 0u || b->step != 0u;
        for (uint32_t a = 0u; a < 2u; a++) {
            expect(b->acc[a] >= -1023 && b->acc[a] <= 1023);
            /* val is int8_t, so 127 is its top and only the low end is a bound to test. */
            expect(b->val[a] >= -127);
            gathered |= b->acc[a] != 0 || b->val[a] != 0;
        }
        switch (held[p]) {
        case COGMASTER_DEVICE_SPORTS_PAD:
            expect(b->pos == 0u);
            break;
        case COGMASTER_DEVICE_PADDLE:
            expect(!gathered);
            break;
        default:
            expect(!gathered && b->pos == 0u);
            break;
        }
        if (b->step == 0u) {
            expect(b->val[0] == 0 && b->val[1] == 0);
        }
        /* The gun's six fields: all zero unless a phaser is in force, then inside their bounds with
           a cursor only while a scan runs. */
        uint32_t aim = (uint32_t)b->g_ax + b->g_ay;
        uint32_t cursor = (uint32_t)b->g_dot + b->g_line + (b->g_sense & 1u);
        if (held[p] != COGMASTER_DEVICE_LIGHT_PHASER) {
            expect(aim == 0u && cursor == 0u && b->g_sense == 0u && b->g_scan == 0u);
        } else {
            expect(b->g_ax <= 256u && b->g_dot <= 256u && b->g_ay <= 240u && b->g_line <= 239u);
            expect(b->g_sense <= 3u && b->g_scan <= 1u && (b->g_ax == 0u) == (b->g_ay == 0u));
            expect(b->g_scan != 0u ? (b->g_ax != 0u && b->g_line < m->vdp.height) : cursor == 0u);
        }
        if (next[p] == COGMASTER_DEVICE_PADDLE) {
            expect(moved[p][1] == 0);
        } else if (next[p] == COGMASTER_DEVICE_LIGHT_PHASER) {
            expect(aim_canonical(moved[p][0], moved[p][1]));
        } else if (next[p] != COGMASTER_DEVICE_SPORTS_PAD) {
            expect(moved[p][0] == 0 && moved[p][1] == 0);
        }
    }
    /* A gun looks at each dot of its disc once a picture: a disc of 113 dots at its radius, counted
       here by distance, and the dot that closes each of its 2 R + 1 rows. */
    uint32_t disc = 0u;
    for (int32_t dy = -(int32_t)GUN_RADIUS; dy <= (int32_t)GUN_RADIUS; dy++) {
        for (int32_t dx = -(int32_t)GUN_RADIUS; dx <= (int32_t)GUN_RADIUS; dx++) {
            disc += dx * dx + dy * dy <= (int32_t)(GUN_RADIUS * GUN_RADIUS) ? 1u : 0u;
        }
    }
    expect(m->gun_stats.samples <= 2u * (disc + 2u * GUN_RADIUS + 1u) * (pictures + 2u));
    expect(m->io.glasses <= 2u && m->io.glasses_frame <= 2u);
    expect(m->io.glasses_frame == 0u || m->io.glasses != 0u);
    expect(m->machine != COGMASTER_MACHINE_GG ||
           (m->io.glasses == 0u && m->io.glasses_frame == 0u));
}

/* ---- operations ---- */

static uint32_t pick_device(struct stream *s)
{
    uint8_t sel = next8(s);
    return (sel & 0x80u) != 0u ? next32(s) : device_table[sel & 7u];
}

static int32_t pick_axis(struct stream *s)
{
    uint8_t sel = next8(s);
    return (sel & 0x80u) != 0u ? (int32_t)next32(s) : axis_table[(sel & 0x7Fu) % AXES];
}

/* What a call leaves on a Light Phaser's port: the pair if it is a field dot, 1 to 256 across and
   1 to 240 down, and 0 0 otherwise, whatever was there before. */
static void stored_aim(uint32_t device, int32_t x, int32_t y, int32_t got_x, int32_t got_y)
{
    if (device != COGMASTER_DEVICE_LIGHT_PHASER) {
        return;
    }
    int field = x >= 1 && x <= 256 && y >= 1 && y <= 240;
    expect(got_x == (field ? x : 0) && got_y == (field ? y : 0));
}

/* One set_input, and the answer the model gives for it: the range first, then the profile. */
static void op_set_input(cogmaster *m, struct stream *s)
{
    cogmaster_input in;
    memset(&in, 0, sizeof in);
    in.size = (uint32_t)sizeof in;
    in.port1 = pick_device(s);
    in.port2 = pick_device(s);
    in.p1 = next8(s) % 72u;
    in.p2 = next8(s) % 72u;
    uint8_t flags = next8(s);
    in.pause = flags & 3u;
    in.reset = (flags >> 2) & 3u;
    in.start = (flags >> 4) & 3u;
    in.x1 = pick_axis(s);
    in.y1 = pick_axis(s);
    in.x2 = pick_axis(s);
    in.y2 = pick_axis(s);
    int range = in.p1 <= 63u && in.p2 <= 63u && in.pause <= 1u && in.reset <= 1u &&
                in.start <= 1u && in.port1 <= COGMASTER_DEVICE_LIGHT_PHASER &&
                in.port2 <= COGMASTER_DEVICE_LIGHT_PHASER && axis_in(in.x1) && axis_in(in.y1) &&
                axis_in(in.x2) && axis_in(in.y2);
    cogmaster_status want = !range ? COGMASTER_STATUS_ERR_ARGUMENT
                            : !attachable(m, in.port1) || !attachable(m, in.port2)
                                ? COGMASTER_STATUS_ERR_PROFILE_REJECTED
                                : COGMASTER_STATUS_OK;
    cogmaster_input before = m->pending_input;
    struct cogmaster__port blocks[2];
    memcpy(blocks, m->ports, sizeof blocks);
    cogmaster_status got = cogmaster_set_input(m, &in);
    if (got != want ||
        (got != COGMASTER_STATUS_OK && (memcmp(&before, &m->pending_input, sizeof before) != 0 ||
                                        memcmp(blocks, m->ports, sizeof blocks) != 0))) {
        abort();
    }
    if (got == COGMASTER_STATUS_OK) {
        stored_aim(in.port1, in.x1, in.y1, m->pending_input.x1, m->pending_input.y1);
        stored_aim(in.port2, in.x2, in.y2, m->pending_input.x2, m->pending_input.y2);
    }
}

/* A read of one of the four ports a game polls, picked by the time; the ports that bring the
   sensor up to the beam ($DD and $7F) are drawn more often than the two that do not. */
static void poll(cogmaster *m)
{
    switch ((m->ticks >> 1) & 7u) {
    case 0u:
        (void)cogmaster__io_in(m, 0xDCu);
        break;
    case 1u:
        (void)cogmaster__vdp_read(m, COGMASTER__VDP_VCOUNT);
        break;
    case 2u:
    case 3u:
    case 4u:
        (void)cogmaster__io_in(m, 0xDDu);
        break;
    default:
        (void)cogmaster__vdp_read(m, COGMASTER__VDP_HCOUNT);
        break;
    }
}

/* Moves time on inside the frame's window, so the instance stays one a run could have saved. The
   twin takes the same step in at most 16 pieces with a poll after each. */
static void advance(cogmaster *m, uint64_t delta, int split)
{
    uint64_t room = m->bus.deadline - 1u - m->ticks;
    uint64_t left = delta < room ? delta : room;
    uint32_t pieces = split ? 16u : 1u;
    while (left != 0u) {
        uint64_t step = (left + pieces - 1u) / pieces;
        m->ticks += step;
        cogmaster__bus_sync(m);
        left -= step;
        if (split) {
            poll(m);
            pieces -= pieces > 1u ? 1u : 0u;
        }
    }
}

static void op_ticks(cogmaster *m, struct stream *s, int split)
{
    uint8_t sel = next8(s);
    advance(m, sel < 5u ? tick_table[sel] : next32(s) % 2001u, split);
}

/* A write to a VDP register: the value, then the register number with its top bit set. */
static void vdp_register(cogmaster *m, uint8_t reg, uint8_t value)
{
    cogmaster__bus_out(m, 0xBFu, value);
    cogmaster__bus_out(m, 0xBFu, (uint8_t)(0x80u | reg));
}

/* White in the backdrop colour (colour 14 of the second palette) with the display off, so a
   picture the all-HALT image never draws is bright all over and a gun has light to find. */
static void op_paint(cogmaster *m)
{
    cogmaster__bus_out(m, 0xBFu, 30u);
    cogmaster__bus_out(m, 0xBFu, 0xC0u);
    cogmaster__bus_out(m, 0xBEu, 0x3Fu);
    vdp_register(m, 7u, 14u);
    vdp_register(m, 1u, 0u);
}

/* A video mode the core does not draw, selected with the display on: the first fault is recorded
   when the picture is rendered, and a gun must not move it. */
static void op_bad_mode(cogmaster *m)
{
    vdp_register(m, 0u, 0u);
    vdp_register(m, 1u, 0x40u);
}

static void op_reset(cogmaster *m, struct stream *s)
{
    uint32_t kind = (next8(s) & 1u) != 0u ? COGMASTER_RESET_POWER : COGMASTER_RESET_SOFT;
    if (cogmaster_reset(m, kind) != COGMASTER_STATUS_OK) {
        abort();
    }
}

static void run_one_frame(cogmaster *m)
{
    if (cogmaster_run_frame(m, NULL) != COGMASTER_STATUS_OK) {
        abort();
    }
}

/* The frame's last tick is reached first, so the twin, which gets there in pieces, and the
   instance, which gets there in one step, begin the frame at the same tick. A frame a host runs
   begins where the last one ended, with the sensor already brought up to the beam; a device
   change takes effect at that start and would otherwise drop the dots a lazy sensor has not yet
   looked at, so the catch-up is done here, as the end of a frame does it. */
static void op_frame(cogmaster *m, int split)
{
    advance(m, UINT64_MAX, split);
    if (cogmaster__gun_attached(m)) {
        cogmaster__gun_sync(m);
    }
    run_one_frame(m);
}

/* One operation of the stream, on the instance or on its twin; returns whether it ran a frame and
   whether it was the switch to the unsupported mode. */
struct done {
    int frame, bad_mode;
};

static struct done one_op(cogmaster *m, struct stream *s, uint32_t *frames, int split)
{
    struct done d = {0, 0};
    switch (next8(s) % OP_COUNT) {
    case OP_SET_INPUT:
        op_set_input(m, s);
        break;
    case OP_TH:
    case OP_TH_AGAIN:
        cogmaster__io_out(m, 0x3Fu, next8(s));
        break;
    case OP_CTRL:
        cogmaster__io_out(m, 0x3Eu, next8(s));
        break;
    case OP_READ:
        (void)cogmaster__io_in(m, (next8(s) & 1u) != 0u ? 0xDDu : 0xDCu);
        break;
    case OP_GLASSES: {
        uint8_t where = next8(s);
        cogmaster__bus_write(m, (uint16_t)(0xFFF8u + (where & 3u)), next8(s));
        break;
    }
    case OP_RESET:
        op_reset(m, s);
        pictures += split ? 0u : 1u;
        break;
    case OP_FRAME:
        if (*frames < MAX_FRAMES) {
            (*frames)++;
            op_frame(m, split);
            pictures += split ? 0u : 1u;
            d.frame = 1;
        }
        break;
    case OP_PAINT:
        op_paint(m);
        break;
    case OP_BAD_MODE:
        op_bad_mode(m);
        d.bad_mode = 1;
        break;
    default:
        op_ticks(m, s, split);
        pictures += split ? 0u : 1u;
        break;
    }
    return d;
}

static void same_after_frame(uint32_t index, const cogmaster *m, const cogmaster *twin,
                             int bad_mode);

/* Every operation on the instance and, in step, on its twin, so the stream is read twice and the
   two read it alike. */
static void operate(uint32_t index, cogmaster *m, cogmaster *twin, struct stream *s)
{
    struct stream cut = *s;
    uint32_t frames = 0u;
    uint32_t cut_frames = 0u;
    int bad_mode = 0;
    for (uint32_t i = 0u; i < MAX_OPS && s->pos < s->n; i++) {
        struct done d = one_op(m, s, &frames, 0);
        (void)one_op(twin, &cut, &cut_frames, 1);
        bad_mode |= d.bad_mode;
        check_pad(m);
        check_pad(twin);
        if (d.frame) {
            same_after_frame(index, m, twin, bad_mode);
        }
    }
}

/* ---- the saved state ---- */

/* Where the device and input fields sit in a state of each profile, from its record; the layout
   does not move between instances of a profile. */
struct item {
    uint32_t off, width;
};

enum { EXCLUDE_ALWAYS = 1, EXCLUDE_GUN = 2, EXCLUDE_SEEN = 4 };

static struct {
    int ready;
    size_t n;
    struct item it[MAX_ITEMS];
    size_t size;
    uint8_t *exclude; /* per byte of the state: the bits above, from the record's names */
} cache[PROFILES];

static int device_field(const char *name)
{
    return strncmp(name, "pending_", 8u) == 0 || strncmp(name, "active_", 7u) == 0 ||
           strncmp(name, "port_", 5u) == 0 || strncmp(name, "io_glasses", 10u) == 0 ||
           strncmp(name, "input_", 6u) == 0;
}

/* The saved fields a poll may move: the read flag, the lag and open-bus counts, the checksum that
   follows them and the bit of the gun's sense byte that a read of $DD takes (the light seen since
   the last read, which is the one thing a poll is for); and, once a mode the core does not draw has
   been selected, the gun's cursor. */
static int exclusion(const char *name)
{
    if (strcmp(name, "input_read") == 0 || strcmp(name, "lag_frames") == 0 ||
        strcmp(name, "open_bus_reads") == 0 || strcmp(name, "body_crc32") == 0) {
        return EXCLUDE_ALWAYS;
    }
    if (strcmp(name, "port_gun_sense") == 0) {
        return EXCLUDE_GUN | EXCLUDE_SEEN;
    }
    return strncmp(name, "port_gun_", 9u) == 0 ? EXCLUDE_GUN : 0;
}

static void load_items(uint32_t index, const cogmaster *m)
{
    if (cache[index].ready) {
        return;
    }
    size_t n = cogmaster__state_record(m, fields, MAX_FIELDS);
    if (n == 0u || n > MAX_FIELDS) {
        abort();
    }
    cache[index].n = 0u;
    cache[index].size = cogmaster__state_size(m);
    cache[index].exclude = (uint8_t *)calloc(cache[index].size, 1u);
    if (cache[index].exclude == NULL) {
        abort();
    }
    for (size_t i = 0u; i < n; i++) {
        if (cache[index].n < MAX_ITEMS && device_field(fields[i].name)) {
            cache[index].it[cache[index].n].off = fields[i].offset;
            cache[index].it[cache[index].n].width = fields[i].width;
            cache[index].n++;
        }
        for (uint32_t b = 0u; b < fields[i].width; b++) {
            cache[index].exclude[fields[i].offset + b] |= (uint8_t)exclusion(fields[i].name);
        }
    }
    cache[index].ready = 1;
}

static uint8_t *saved(const cogmaster *m, size_t n)
{
    uint8_t *buf = (uint8_t *)malloc(n);
    if (buf == NULL || cogmaster__state_save(m, buf, n) != COGMASTER_STATUS_OK) {
        abort();
    }
    return buf;
}

/* The instance and its twin hold the same state at the end of a frame, byte for byte, apart from
   what a poll moves. */
static void same_after_frame(uint32_t index, const cogmaster *m, const cogmaster *twin,
                             int bad_mode)
{
    size_t n = cache[index].size;
    uint8_t *a = saved(m, n);
    uint8_t *b = saved(twin, n);
    for (size_t i = 0u; i < n; i++) {
        uint8_t skip = cache[index].exclude[i];
        uint8_t differ = (uint8_t)(a[i] ^ b[i]);
        differ &= (skip & EXCLUDE_SEEN) != 0u ? 0xFDu : 0xFFu;
        if ((skip & EXCLUDE_ALWAYS) == 0u && ((skip & EXCLUDE_GUN) == 0u || !bad_mode) &&
            differ != 0u) {
            abort();
        }
    }
    free(a);
    free(b);
}

/* Loading what m saved into a new instance of the profile and saving it again gives the same
   bytes. */
static void same_bytes(uint32_t index, const uint8_t *bytes, size_t n)
{
    cogmaster *other = open_profile(index);
    uint8_t *again = (uint8_t *)malloc(n);
    if (again == NULL || cogmaster__state_size(other) != n ||
        cogmaster__state_load(other, bytes, n) != COGMASTER_STATUS_OK ||
        cogmaster__state_save(other, again, n) != COGMASTER_STATUS_OK ||
        memcmp(bytes, again, n) != 0) {
        abort();
    }
    check_pad(other);
    free(again);
    cogmaster_destroy(other);
}

/* One to three bytes of device or input fields changed in a copy, sealed and loaded. */
static void mutate_leg(uint32_t index, cogmaster *m, const uint8_t *good, size_t n,
                       struct stream *s)
{
    load_items(index, m);
    uint8_t *bad = (uint8_t *)malloc(n);
    if (bad == NULL || cache[index].n == 0u) {
        abort();
    }
    memcpy(bad, good, n);
    uint32_t count = 1u + next8(s) % 3u;
    for (uint32_t i = 0u; i < count; i++) {
        const struct item *it = &cache[index].it[next8(s) % cache[index].n];
        uint8_t at = next8(s);
        uint8_t value = next8(s);
        uint8_t *where = &bad[it->off + at % it->width];
        *where = (value & 3u) != 0u ? value : (uint8_t)~*where;
    }
    cogmaster__state_seal(bad, n);
    cogmaster *target = open_profile(index);
    static uint8_t snapshot[sizeof(struct cogmaster)];
    memcpy(snapshot, target, sizeof snapshot);
    cogmaster_status st = cogmaster__state_load(target, bad, n);
    if (st != COGMASTER_STATUS_OK) {
        if (st != COGMASTER_STATUS_ERR_ARGUMENT || memcmp(snapshot, target, sizeof snapshot) != 0) {
            abort();
        }
    } else {
        check_pad(target);
        uint8_t *resaved = saved(target, n);
        if (memcmp(resaved, bad, n) != 0) {
            abort();
        }
        same_bytes(index, resaved, n);
        free(resaved);
        run_one_frame(target);
        check_pad(target);
    }
    free(bad);
    cogmaster_destroy(target);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int ready;
    if (!ready) {
        memset(image, 0x76u, sizeof image);
        ready = 1;
    }
    /* The profile byte, then ten bytes that name the fields to change at the end (so the
       operations cannot use them up), then the operations. */
    struct stream s = {data, size, 0u};
    uint32_t index = next8(&s) % PROFILES;
    struct stream change = {data + s.pos, size - s.pos < CHANGE_BYTES ? size - s.pos : CHANGE_BYTES,
                            0u};
    s.pos += change.n;
    cogmaster *m = open_profile(index);
    cogmaster *twin = open_profile(index);
    pictures = 0u;
    load_items(index, m);
    check_pad(m);
    operate(index, m, twin, &s);
    cogmaster_destroy(twin);

    size_t n = cogmaster__state_size(m);
    uint8_t *good = saved(m, n);
    same_bytes(index, good, n);
    mutate_leg(index, m, good, n, &change);
    free(good);
    cogmaster_destroy(m);
    return 0;
}
