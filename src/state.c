/* The internal state visitor over the test-card instance (ARCHITECTURE
   section 7).

   Layout. A 64-byte header, then the body:
     header  magic, format version, header size, total size, behaviour
             revision, API version, packed profile, ROM CRC-32, body CRC-32,
             frame, ticks, 12 reserved zero bytes
     body    the counters, then the first-fault record
   One field list drives every pass (state.h), so they cannot drift apart. A
   struct is never copied as a whole: padding, pointers and the allocator
   never reach the bytes. The field buffer is not saved: every frame of the
   test card redraws it. The body CRC-32 covers every byte after the header. */
#include <string.h>

#include "state.h"

#define STATE_HEADER_SIZE 64u

/* Header offsets, in the order header_io visits them: magic 0, format
   version 8, header size 10, total size 12, behaviour revision 16, API
   version 20, profile 24, ROM CRC-32 28, body CRC-32 32, frame 36, ticks 44,
   reserved 52. */
#define H_BODY_CRC 32u
#define H_RESERVED 52u
#define H_RESERVED_SIZE 12u
_Static_assert(H_RESERVED + H_RESERVED_SIZE == STATE_HEADER_SIZE, "the header is 64 bytes");

/* A loaded state's ticks are capped at 2^62, far from 2^64 wrapping
   (ARCHITECTURE section 3); the frame cap follows from it. */
#define STATE_TICKS_MAX (UINT64_C(1) << 62)
#define STATE_FRAME_MAX (STATE_TICKS_MAX / COGMASTER__TICKS_PER_FRAME_NTSC)

/* PNG-style magic: a high-bit byte, "CGM", CR LF, DOS end of file, LF. A
   text-mode transfer that strips the high bit or rewrites line ends breaks
   it. */
static const uint8_t state_magic[8] = {0x8Au, 0x43u, 0x47u, 0x4Du, 0x0Du, 0x0Au, 0x1Au, 0x0Au};

/* Tripwire: a counter added to the struct fails the build until
   counters_io visits it. */
_Static_assert(sizeof(struct cogmaster__counters) == 40u, "counters changed: update counters_io");

/* One header field whose only valid value is v. */
static void fixed(struct cogmaster__sio *io, const char *name, uint64_t v, uint32_t width)
{
    (void)cogmaster__sio_field(io, name, v, width, v, v);
}

static void header_io(struct cogmaster__sio *io, cogmaster *m, uint32_t total)
{
    for (uint32_t i = 0; i < sizeof state_magic; i++) {
        fixed(io, "magic", state_magic[i], 1u);
    }
    fixed(io, "format_version", COGMASTER_STATE_FORMAT_VERSION, 2u);
    fixed(io, "header_size", STATE_HEADER_SIZE, 2u);
    fixed(io, "total_size", total, 4u);
    fixed(io, "behaviour_revision", COGMASTER_BEHAVIOUR_REVISION, 4u);
    fixed(io, "api_version", COGMASTER_API_VERSION, 4u);
    /* The packed profile holds the machine in its low byte; the other
       profile fields are zero until they exist. A state loads only into an
       instance of the same profile. */
    fixed(io, "profile", m->machine, 4u);
    /* The CRC-32 of the ROM image: 0 with no content. */
    fixed(io, "rom_crc32", 0u, 4u);
    /* Checked against the body before VALIDATE runs; any value passes here. */
    (void)cogmaster__sio_field(io, "body_crc32", 0u, 4u, 0u, UINT32_MAX);
    uint64_t frame = cogmaster__sio_u64(io, "frame", &m->frame, 0u, STATE_FRAME_MAX);
    uint64_t ticks = cogmaster__sio_u64(io, "ticks", &m->ticks, 0u, STATE_TICKS_MAX);
    /* Every test-card frame is exactly one NTSC frame of ticks. */
    cogmaster__sio_check(io, ticks == frame * COGMASTER__TICKS_PER_FRAME_NTSC);
    for (uint32_t i = 0; i < H_RESERVED_SIZE; i++) {
        fixed(io, "reserved", 0u, 1u);
    }
}

static void counters_io(struct cogmaster__sio *io, struct cogmaster__counters *c)
{
    cogmaster__sio_u32(io, "lag_frames", &c->lag_frames, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "frame_ints", &c->frame_ints, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "line_ints", &c->line_ints, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "nmis", &c->nmis, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "open_bus_reads", &c->open_bus_reads, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "audio_dropped", &c->audio_dropped, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "vdp_writes_active", &c->vdp_writes_active, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "vdp_fast_accesses", &c->vdp_fast_accesses, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "sprite_overflow_lines", &c->sprite_overflow_lines, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "collision_frames", &c->collision_frames, 0u, UINT32_MAX);
}

/* A line is at most 313 long (PAL) and 342 dots wide (ARCHITECTURE
   section 2). */
#define FAULT_LINE_MAX 312u
#define FAULT_DOT_MAX 341u

static void fault_io(struct cogmaster__sio *io, cogmaster *m)
{
    struct cogmaster__fault *f = &m->fault;
    uint32_t present = cogmaster__sio_u32(io, "fault_present", &m->fault_present, 0u, 1u);
    uint64_t frame = cogmaster__sio_u64(io, "fault_frame", &f->frame, 0u, STATE_FRAME_MAX);
    uint32_t line = cogmaster__sio_u32(io, "fault_line", &f->line, 0u, FAULT_LINE_MAX);
    uint32_t dot = cogmaster__sio_u32(io, "fault_dot", &f->dot, 0u, FAULT_DOT_MAX);
    uint16_t pc = cogmaster__sio_u16(io, "fault_pc", &f->pc, 0u, UINT16_MAX);
    uint16_t addr = cogmaster__sio_u16(io, "fault_addr", &f->addr, 0u, UINT16_MAX);
    uint8_t opcode = cogmaster__sio_u8(io, "fault_opcode", &f->opcode, 0u, UINT8_MAX);
    uint8_t width = cogmaster__sio_u8(io, "fault_width", &f->width, 0u, UINT8_MAX);
    uint8_t device = cogmaster__sio_u8(io, "fault_device", &f->device, 0u, UINT8_MAX);
    uint8_t kind = cogmaster__sio_u8(io, "fault_kind", &f->kind, 0u, UINT8_MAX);
    /* With no fault recorded every field is zero. */
    cogmaster__sio_check(io, present != 0u || (frame | line | dot | pc | addr | opcode | width |
                                               device | kind) == 0u);
}

/* The whole state, in the order the format fixes. */
static void state_io(struct cogmaster__sio *io, cogmaster *m, uint32_t total)
{
    header_io(io, m, total);
    counters_io(io, &m->counters);
    fault_io(io, m);
}

/* SIZE, WRITE and RECORD only read through the field pointers, and an
   instance is never a const object, so walking a const instance is defined.
   The pointer is copied, not cast, to say so once. */
static cogmaster *walkable(const cogmaster *m)
{
    cogmaster *w;
    memcpy(&w, &m, sizeof w);
    return w;
}

static uint32_t body_crc(const uint8_t *b, size_t size)
{
    return cogmaster__crc32(0u, b + STATE_HEADER_SIZE, size - STATE_HEADER_SIZE);
}

size_t cogmaster__state_size(const cogmaster *m)
{
    if (m == NULL || !m->loaded) {
        return 0u;
    }
    struct cogmaster__sio io;
    memset(&io, 0, sizeof io);
    io.mode = COGMASTER__SIO_SIZE;
    state_io(&io, walkable(m), 0u);
    return io.n;
}

size_t cogmaster__state_record(const cogmaster *m, struct cogmaster__state_field *out, size_t cap)
{
    size_t size = cogmaster__state_size(m);
    if (size == 0u) {
        return 0u;
    }
    struct cogmaster__sio io;
    memset(&io, 0, sizeof io);
    io.mode = COGMASTER__SIO_RECORD;
    io.rec = out;
    io.rec_cap = cap;
    state_io(&io, walkable(m), (uint32_t)size);
    return io.rec_n;
}

void cogmaster__state_seal(void *buf, size_t size)
{
    uint8_t *b = (uint8_t *)buf;
    if (b == NULL || size < STATE_HEADER_SIZE) {
        return;
    }
    uint32_t crc = body_crc(b, size);
    for (uint32_t i = 0; i < 4u; i++) {
        b[H_BODY_CRC + i] = (uint8_t)(crc >> (8u * i));
    }
}

cogmaster_status cogmaster__state_save(const cogmaster *m, void *buf, size_t capacity)
{
    if (m == NULL || buf == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    size_t size = cogmaster__state_size(m);
    if (capacity < size) {
        return COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL;
    }
    struct cogmaster__sio io;
    memset(&io, 0, sizeof io);
    io.mode = COGMASTER__SIO_WRITE;
    io.w = (uint8_t *)buf;
    state_io(&io, walkable(m), (uint32_t)size);
    cogmaster__state_seal(buf, size);
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster__state_load(cogmaster *m, const void *buf, size_t size)
{
    const uint8_t *b = (const uint8_t *)buf;
    if (m == NULL || b == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    if (size != cogmaster__state_size(m)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    uint32_t crc = 0u;
    for (uint32_t i = 0; i < 4u; i++) {
        crc |= (uint32_t)b[H_BODY_CRC + i] << (8u * i);
    }
    if (crc != body_crc(b, size)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    /* VALIDATE walks the same fields over the same bytes that APPLY reads
       and writes nothing, so a refused state has changed nothing. */
    struct cogmaster__sio io;
    memset(&io, 0, sizeof io);
    io.mode = COGMASTER__SIO_VALIDATE;
    io.r = b;
    state_io(&io, m, (uint32_t)size);
    if (io.bad) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    /* APPLY cannot fail. */
    io.mode = COGMASTER__SIO_APPLY;
    io.r = b;
    state_io(&io, m, (uint32_t)size);
    return COGMASTER_STATUS_OK;
}
