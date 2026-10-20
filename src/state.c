/* The internal state visitor over the instance (ARCHITECTURE section 7).

   Layout. A 64-byte header, then the body:
     header  magic, format version, header size, total size, behaviour
             revision, API version, profile (machine, region, TV and a zero
             byte), ROM CRC-32, body CRC-32,
             frame, ticks, 12 reserved zero bytes
     body    the counters, the first-fault record, the Z80, the beam and the
             frame deadline, the VDP with its 16 KiB of VRAM, then the 8 KiB
             of work RAM
   One field list drives every pass (state.h), so they cannot drift apart. A
   struct is never copied as a whole: padding, pointers and the allocator
   never reach the bytes. The field buffer is not saved: every frame of the
   test card redraws it, and a loaded image draws nothing yet. The cartridge
   image is the media, not state; its CRC-32 ties a state to it. The body
   CRC-32 covers every byte after the header. */
#include <string.h>

#include "state.h"

#define STATE_HEADER_SIZE 64u

/* Header offsets, in the order header_io visits them: magic 0, format
   version 8, header size 10, total size 12, behaviour revision 16, API
   version 20, machine 24, region 25, TV 26, profile reserved 27, ROM CRC-32
   28, body CRC-32 32, frame 36, ticks 44, reserved 52. */
#define H_BODY_CRC 32u
#define H_RESERVED 52u
#define H_RESERVED_SIZE 12u
_Static_assert(H_RESERVED + H_RESERVED_SIZE == STATE_HEADER_SIZE, "the header is 64 bytes");

/* A loaded state's ticks are capped at 2^62, far from 2^64 wrapping
   (ARCHITECTURE section 2); the frame cap follows from it on the shorter
   (NTSC) frame. */
#define STATE_TICKS_MAX (UINT64_C(1) << 62)
#define STATE_FRAME_MAX (STATE_TICKS_MAX / COGMASTER__TICKS_PER_FRAME_NTSC)

/* PNG-style magic: a high-bit byte, "CGM", CR LF, DOS end of file, LF. A
   text-mode transfer that strips the high bit or rewrites line ends breaks
   it. */
static const uint8_t state_magic[8] = {0x8Au, 0x43u, 0x47u, 0x4Du, 0x0Du, 0x0Au, 0x1Au, 0x0Au};

/* Tripwires: a field added to one of these structs fails the build until
   its visitor visits it. */
_Static_assert(sizeof(struct cogmaster__counters) == 40u, "counters changed: update counters_io");
_Static_assert(sizeof(struct cogmaster__z80) == 38u, "z80 changed: update z80_io");
_Static_assert(sizeof(struct cogmaster__bus) == 32u, "bus changed: update bus_io");
_Static_assert(sizeof(struct cogmaster__vdp) == 16436u, "vdp changed: update vdp_io");

/* A beam is at most one line short of the frame's lines down and 683 ticks
   into a line, and one Z80 step is at most 69 ticks, so the beam is at most
   that far behind ticks (bus.c). */
#define STATE_T_MAX 683u
#define STATE_STEP_TICKS_MAX 69u

/* The frame and ticks read from the header, for the body's cross checks. */
struct state_time {
    uint64_t frame, ticks;
};

/* One header field whose only valid value is v. */
static void fixed(struct cogmaster__sio *io, const char *name, uint64_t v, uint32_t width)
{
    (void)cogmaster__sio_field(io, name, v, width, v, v);
}

static struct state_time header_io(struct cogmaster__sio *io, cogmaster *m, uint32_t total)
{
    for (uint32_t i = 0; i < sizeof state_magic; i++) {
        fixed(io, "magic", state_magic[i], 1u);
    }
    fixed(io, "format_version", COGMASTER_STATE_FORMAT_VERSION, 2u);
    fixed(io, "header_size", STATE_HEADER_SIZE, 2u);
    fixed(io, "total_size", total, 4u);
    fixed(io, "behaviour_revision", COGMASTER_BEHAVIOUR_REVISION, 4u);
    fixed(io, "api_version", COGMASTER_API_VERSION, 4u);
    /* The resolved profile, each field 1 or 2, then a zero byte. A state
       loads only into an instance of the same profile, so the frame length
       below is the instance's. */
    fixed(io, "machine", m->machine, 1u);
    fixed(io, "region", m->region, 1u);
    fixed(io, "tv", m->tv, 1u);
    fixed(io, "profile_reserved", 0u, 1u);
    /* The CRC-32 of the image after any copier header: 0 for the test
       card. */
    fixed(io, "rom_crc32", m->cart.crc32, 4u);
    /* Checked against the body before VALIDATE runs; any value passes here. */
    (void)cogmaster__sio_field(io, "body_crc32", 0u, 4u, 0u, UINT32_MAX);
    struct state_time time;
    time.frame = cogmaster__sio_u64(io, "frame", &m->frame, 0u, STATE_FRAME_MAX);
    time.ticks = cogmaster__sio_u64(io, "ticks", &m->ticks, 0u, STATE_TICKS_MAX);
    /* Every test-card frame is exactly one frame of ticks; an image's frame
       starts at most one step late, its overshoot. */
    uint64_t start = time.frame * cogmaster__frame_ticks(m);
    if (m->cart.rom == NULL) {
        cogmaster__sio_check(io, time.ticks == start);
    } else {
        cogmaster__sio_check(io, time.ticks >= start);
    }
    for (uint32_t i = 0; i < H_RESERVED_SIZE; i++) {
        fixed(io, "reserved", 0u, 1u);
    }
    return time;
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

/* Returns /INT as saved, for the VDP's cross check. */
static uint8_t z80_io(struct cogmaster__sio *io, struct cogmaster__z80 *z)
{
    for (uint32_t i = 0; i < COGMASTER__Z80_REGS; i++) {
        cogmaster__sio_u8(io, "z80_reg", &z->reg[i], 0u, UINT8_MAX);
    }
    cogmaster__sio_u16(io, "z80_pc", &z->pc, 0u, UINT16_MAX);
    cogmaster__sio_u16(io, "z80_sp", &z->sp, 0u, UINT16_MAX);
    cogmaster__sio_u16(io, "z80_wz", &z->wz, 0u, UINT16_MAX);
    cogmaster__sio_u8(io, "z80_i", &z->i, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "z80_r", &z->r, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "z80_iff1", &z->iff1, 0u, 1u);
    cogmaster__sio_u8(io, "z80_iff2", &z->iff2, 0u, 1u);
    cogmaster__sio_u8(io, "z80_im", &z->im, 0u, 2u);
    cogmaster__sio_u8(io, "z80_q", &z->q, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "z80_p", &z->p, 0u, 1u);
    cogmaster__sio_u8(io, "z80_ei", &z->ei, 0u, 1u);
    cogmaster__sio_u8(io, "z80_idx", &z->idx, 0u, COGMASTER__Z80_IDX_IY);
    cogmaster__sio_u8(io, "z80_halted", &z->halted, 0u, 1u);
    uint8_t int_line = cogmaster__sio_u8(io, "z80_int_line", &z->int_line, 0u, 1u);
    cogmaster__sio_u8(io, "z80_nmi_pending", &z->nmi_pending, 0u, 1u);
    return int_line;
}

/* The deadline is the end of the frame the header's frame count is in, the
   ticks have not passed it, and the beam is at most one step behind them. */
static void bus_io(struct cogmaster__sio *io, cogmaster *m, struct state_time time)
{
    struct cogmaster__bus *b = &m->bus;
    uint64_t frame_ticks = cogmaster__frame_ticks(m);
    uint64_t deadline = cogmaster__sio_u64(io, "bus_deadline", &b->deadline, frame_ticks,
                                           STATE_TICKS_MAX + frame_ticks);
    uint64_t at = cogmaster__sio_u64(io, "bus_at", &b->at, 0u, STATE_TICKS_MAX);
    uint64_t vdp_at = cogmaster__sio_u64(io, "bus_vdp_at", &b->vdp_at, 0u, STATE_TICKS_MAX);
    cogmaster__sio_u32(io, "bus_line", &b->line, 0u, cogmaster__frame_lines(m) - 1u);
    cogmaster__sio_u32(io, "bus_t", &b->t, 0u, STATE_T_MAX);
    cogmaster__sio_check(io, deadline == (time.frame + 1u) * frame_ticks);
    cogmaster__sio_check(io, time.ticks < deadline);
    cogmaster__sio_check(io, at <= time.ticks && time.ticks - at <= STATE_STEP_TICKS_MAX);
    cogmaster__sio_check(io, vdp_at <= time.ticks);
}

/* Status flags are bits 7-5; the 14-bit address; codes 0 to 3 (SMS-VDP
   section 2). Only the frame flag drives /INT yet, gated by R1 bit 5. */
#define STATE_VDP_FLAGS 0xE0u
#define STATE_VDP_ADDR_MAX 0x3FFFu
#define STATE_VDP_CODE_MAX 3u

static void vdp_io(struct cogmaster__sio *io, struct cogmaster__vdp *v, uint8_t int_line)
{
    uint8_t r1 = 0u;
    for (uint32_t i = 0; i < COGMASTER__VDP_REGS; i++) {
        uint8_t r = cogmaster__sio_u8(io, "vdp_reg", &v->reg[i], 0u, UINT8_MAX);
        r1 = i == 1u ? r : r1;
    }
    uint8_t status = cogmaster__sio_u8(io, "vdp_status", &v->status, 0u, STATE_VDP_FLAGS);
    cogmaster__sio_u8(io, "vdp_latch", &v->latch, 0u, 1u);
    cogmaster__sio_u8(io, "vdp_first", &v->first, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "vdp_code", &v->code, 0u, STATE_VDP_CODE_MAX);
    cogmaster__sio_u16(io, "vdp_addr", &v->addr, 0u, STATE_VDP_ADDR_MAX);
    cogmaster__sio_u8(io, "vdp_buffer", &v->buffer, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "vdp_pause", &v->pause, 0u, 1u);
    for (uint32_t i = 0; i < COGMASTER__CRAM_SIZE; i++) {
        cogmaster__sio_u8(io, "vdp_cram", &v->cram[i], 0u, UINT8_MAX);
    }
    for (uint32_t i = 0; i < COGMASTER__VRAM_SIZE; i++) {
        cogmaster__sio_u8(io, "vdp_vram", &v->vram[i], 0u, UINT8_MAX);
    }
    cogmaster__sio_check(io, (status & ~STATE_VDP_FLAGS) == 0u);
    cogmaster__sio_check(io, int_line == ((status >> 7) & (r1 >> 5) & 1u));
}

static void ram_io(struct cogmaster__sio *io, cogmaster *m)
{
    for (uint32_t i = 0; i < COGMASTER__RAM_SIZE; i++) {
        cogmaster__sio_u8(io, "ram", &m->ram[i], 0u, UINT8_MAX);
    }
}

/* The whole state, in the order the format fixes. */
static void state_io(struct cogmaster__sio *io, cogmaster *m, uint32_t total)
{
    struct state_time time = header_io(io, m, total);
    counters_io(io, &m->counters);
    fault_io(io, m);
    uint8_t int_line = z80_io(io, &m->z80);
    bus_io(io, m, time);
    vdp_io(io, &m->vdp, int_line);
    ram_io(io, m);
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
