/* The internal state visitor over the instance (ARCHITECTURE section 7).

   Layout. A 64-byte header, then the body:
     header  magic, format version, header size, total size, behaviour
             revision, API version, profile (machine, region, TV and video
             mode), ROM CRC-32, body CRC-32,
             frame, ticks, the FM unit, 11 reserved zero bytes
     body    the counters, the first-fault record, the Z80, the beam and the
             frame deadline, the VDP with its 16 KiB of VRAM, the 8 KiB of
             work RAM, then the board, the mapper registers, on the Sega
             EEPROM board the chip's command state, and the cartridge RAM
             (none, 128 bytes, 8 KiB or 32 KiB, fixed by the image),
             then the I/O ports, the synthesiser (cadence phase, ring
             slots, integrator, DC estimate, levels) and the PSG (step
             origin, periods, counters, flip-flops, attenuations, noise
             control, shift register, latch), then, only with the FM unit,
             the YM2413 at a fixed size (registers, counters, noise, the
             last sum, the user instrument, each of 18 operator slots with
             its derived values and pending requests, the next sample's
             tick)
   One field list drives every pass (state.h), so they cannot drift apart. A
   struct is never copied as a whole: padding, pointers and the allocator
   never reach the bytes. The emitted field and render cursor are saved so a partially drawn
   field survives restoration. The cartridge
   image is the media, not state; its CRC-32 ties a state to it. The body
   CRC-32 covers every byte after the header. */
#include <string.h>

#include "state.h"

#define STATE_HEADER_SIZE 64u

/* Header offsets, in the order header_io visits them: magic 0, format
   version 8, header size 10, total size 12, behaviour revision 16, API
   version 20, machine 24, region 25, TV 26, video mode 27, ROM CRC-32 28,
   body CRC-32 32, frame 36, ticks 44, FM unit 52, reserved 53. */
#define H_BODY_CRC 32u
#define H_RESERVED 53u
#define H_RESERVED_SIZE 11u
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
_Static_assert(sizeof(struct cogmaster__bus) == 40u, "bus changed: update bus_io");
_Static_assert(sizeof(struct cogmaster__vdp) == 16510u, "vdp changed: update vdp_io");
_Static_assert(sizeof(struct cogmaster__mapper) == 12u, "mapper changed: update mapper_io");
_Static_assert(sizeof(struct cogmaster__eeprom) == 8u, "eeprom changed: update eeprom_io");
_Static_assert(sizeof(struct cogmaster__io) == 11u, "io changed: update io_io");
/* The saved synth fields are the ring, integrator, DC estimate, head, levels,
   gained levels and the cadence phase. The trace counters, the stats and the
   PCM the host has not taken are its queue and counters, not state, so a new
   field either joins synth_io or is added to this size. */
_Static_assert(sizeof(struct cogmaster__synth) ==
                   sizeof(int64_t[2][COGMASTER__SYNTH_RING]) + 2u * sizeof(int64_t[2]) +
                       (1u + 4u + 1u + COGMASTER__SYNTH_SOURCES + 2u * COGMASTER__SYNTH_SOURCES +
                        5u) *
                           sizeof(uint32_t) +
                       sizeof(int16_t[COGMASTER__AUDIO_MAX_FRAMES * 2u]),
               "synth changed: update synth_io");

/* The PSG's size is pinned the same way: a new field either joins psg_io or
   changes this. */
_Static_assert(sizeof(struct cogmaster__psg) == 40u, "psg changed: update psg_io");

/* The YM2413's size is pinned too. slot_evals is a counter the cost tests read, not state;
   every other field, and the user instrument of the patches, joins opll_io. The other 36
   instruments are the chip's ROM and load at every reset. */
_Static_assert(sizeof(struct cogmaster__opll) == 1288u, "opll changed: update opll_io");

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
    /* The resolved profile, then a zero byte. A state
       loads only into an instance of the same profile, so the frame length
       below is the instance's. */
    fixed(io, "machine", m->machine, 1u);
    fixed(io, "region", m->region, 1u);
    fixed(io, "tv", m->tv, 1u);
    fixed(io, "video_mode", m->video_mode, 1u);
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
    /* Attached at load and never changed, so a state loads only into an
       instance with the same FM unit. */
    fixed(io, "fm_unit", m->fm_unit, 1u);
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
    uint8_t iff1 = cogmaster__sio_u8(io, "z80_iff1", &z->iff1, 0u, 1u);
    cogmaster__sio_u8(io, "z80_iff2", &z->iff2, 0u, 1u);
    cogmaster__sio_u8(io, "z80_im", &z->im, 0u, 2u);
    cogmaster__sio_u8(io, "z80_q", &z->q, 0u, UINT8_MAX);
    uint8_t p = cogmaster__sio_u8(io, "z80_p", &z->p, 0u, 1u);
    uint8_t ei = cogmaster__sio_u8(io, "z80_ei", &z->ei, 0u, 1u);
    uint8_t idx = cogmaster__sio_u8(io, "z80_idx", &z->idx, 0u, COGMASTER__Z80_IDX_IY);
    uint8_t halted = cogmaster__sio_u8(io, "z80_halted", &z->halted, 0u, 1u);
    uint8_t int_line = cogmaster__sio_u8(io, "z80_int_line", &z->int_line, 0u, 1u);
    cogmaster__sio_u8(io, "z80_nmi_pending", &z->nmi_pending, 0u, 1u);
    /* A prefix is a step of its own that clears the EI and LD A,I/R marks,
       and the step that consumes it clears it again, so HALT, EI or LD A,I/R
       never leaves one pending; EI sets IFF1 in the same step that sets its
       mark (z80.c). A halted CPU with a prefix pending would never accept
       /INT or NMI again. */
    cogmaster__sio_check(io, idx == COGMASTER__Z80_IDX_NONE || (halted | ei | p) == 0u);
    cogmaster__sio_check(io, ei == 0u || iff1 != 0u);
    return int_line;
}

/* The deadline is the end of the frame the header's frame count is in, the
   ticks have not passed it, the beam's stamp is at most one step behind
   them, and the beam is where that stamp puts it. */
static void bus_io(struct cogmaster__sio *io, cogmaster *m, struct state_time time)
{
    struct cogmaster__bus *b = &m->bus;
    uint64_t frame_ticks = cogmaster__frame_ticks(m);
    uint64_t deadline = cogmaster__sio_u64(io, "bus_deadline", &b->deadline, frame_ticks,
                                           STATE_TICKS_MAX + frame_ticks);
    uint64_t at = cogmaster__sio_u64(io, "bus_at", &b->at, 0u, STATE_TICKS_MAX);
    uint64_t vdp_at = cogmaster__sio_u64(io, "bus_vdp_at", &b->vdp_at, 0u, STATE_TICKS_MAX);
    uint32_t line =
        cogmaster__sio_u32(io, "bus_line", &b->line, 0u, cogmaster__frame_lines(m) - 1u);
    uint32_t t = cogmaster__sio_u32(io, "bus_t", &b->t, 0u, STATE_T_MAX);
    cogmaster__sio_u16(io, "bus_op_pc", &b->op_pc, 0u, UINT16_MAX);
    cogmaster__sio_u8(io, "bus_op", &b->op, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "bus_data", &b->data, 0u, UINT8_MAX);
    cogmaster__sio_check(io, deadline == (time.frame + 1u) * frame_ticks);
    cogmaster__sio_check(io, time.ticks < deadline);
    cogmaster__sio_check(io, at <= time.ticks && time.ticks - at <= STATE_STEP_TICKS_MAX);
    cogmaster__sio_check(io, vdp_at <= time.ticks);
    /* The beam is where at puts it, so the frame interrupt stays on the
       frame deadline and the counters agree with ticks. Any at that passes
       the checks above lies within one frame of the frame's start. */
    cogmaster__sio_check(io, line * (STATE_T_MAX + 1u) + t ==
                                 cogmaster__bus_beam_at(m, time.frame, at));
}

/* Status flags are bits 7-5; the 14-bit address; codes 0 to 3 (SMS-VDP
   section 2). Each independent pending source is gated by its enable. */
#define STATE_VDP_FLAGS 0xE0u
#define STATE_VDP_ADDR_MAX 0x3FFFu
#define STATE_VDP_CODE_MAX 3u

static uint8_t vdp_io(struct cogmaster__sio *io, struct cogmaster__vdp *v, uint8_t int_line,
                      int native)
{
    uint8_t height = cogmaster__sio_u8(io, "vdp_height", &v->height, 192u, 240u);
    cogmaster__sio_check(io, height == 192u || height == 224u || height == 240u);
    uint8_t r0 = 0u, r1 = 0u;
    for (uint32_t i = 0; i < COGMASTER__VDP_REGS; i++) {
        uint8_t r = cogmaster__sio_u8(io, "vdp_reg", &v->reg[i], 0u, UINT8_MAX);
        r1 = i == 1u ? r : r1;
        r0 = i == 0u ? r : r0;
    }
    uint8_t status = cogmaster__sio_u8(io, "vdp_status", &v->status, 0u, STATE_VDP_FLAGS);
    cogmaster__sio_u8(io, "vdp_latch", &v->latch, 0u, 1u);
    cogmaster__sio_u8(io, "vdp_first", &v->first, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "vdp_code", &v->code, 0u, STATE_VDP_CODE_MAX);
    cogmaster__sio_u16(io, "vdp_addr", &v->addr, 0u, STATE_VDP_ADDR_MAX);
    cogmaster__sio_u16(io, "vdp_vline", &v->vline, 0u, 262u);
    cogmaster__sio_u8(io, "vdp_buffer", &v->buffer, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "vdp_pause", &v->pause, 0u, 1u);
    cogmaster__sio_u8(io, "vdp_line_counter", &v->line_counter, 0u, UINT8_MAX);
    uint8_t pending = cogmaster__sio_u8(io, "vdp_line_pending", &v->line_pending, 0u, 1u);
    uint8_t h = cogmaster__sio_u8(io, "vdp_hcounter", &v->hcounter, 0u, UINT8_MAX);
    uint8_t held = cogmaster__sio_u8(io, "vdp_hheld", &v->hheld, 0u, 1u);
    cogmaster__sio_check(io, (!held && h == 0u) || (held && (h <= 0x93u || h >= 0xE9u)));
    cogmaster__sio_u8(io, "vdp_hscroll", &v->hscroll, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "vdp_vscroll", &v->vscroll, 0u, UINT8_MAX);
    cogmaster__sio_u8(io, "vdp_display", &v->display, 0u, 1u);
    cogmaster__sio_u8(io, "sprite_line", &v->sprite_line, 0u, COGMASTER__FIELD_MAX_H);
    uint8_t count = cogmaster__sio_u8(io, "sprite_count", &v->sprite_count, 0u, 8u);
    uint8_t scanned = cogmaster__sio_u8(io, "sprite_scanned", &v->sprite_scanned, 0u, 64u);
    cogmaster__sio_check(io, count <= scanned);
    cogmaster__sio_u8(io, "sprite_zoom", &v->sprite_zoom, 0u, 1u);
    cogmaster__sio_u8(io, "sprite_shift", &v->sprite_shift, 0u, 1u);
    uint8_t overflow = cogmaster__sio_u8(io, "sprite_overflow", &v->sprite_overflow, 0u, 1u);
    cogmaster__sio_check(io, !overflow || (count == 8u && scanned >= 9u));
    for (uint32_t i = 0u; i < 8u; i++) {
        cogmaster__sio_u8(io, "sprite_x", &v->sprite[i].x, 0u, UINT8_MAX);
        cogmaster__sio_u8(io, "sprite_tile", &v->sprite[i].tile, 0u, UINT8_MAX);
        cogmaster__sio_u8(io, "sprite_row", &v->sprite[i].row, 0u, 7u);
    }
    cogmaster__sio_u8(io, "cram_latch", &v->cram_latch, 0u, UINT8_MAX);
    for (uint32_t i = 0; i < COGMASTER__CRAM_SIZE; i++) {
        cogmaster__sio_u8(io, "vdp_cram", &v->cram[i], 0u, native && (i & 1u) ? 15u : UINT8_MAX);
    }
    for (uint32_t i = 0; i < COGMASTER__VRAM_SIZE; i++) {
        cogmaster__sio_u8(io, "vdp_vram", &v->vram[i], 0u, UINT8_MAX);
    }
    cogmaster__sio_check(io, (status & ~STATE_VDP_FLAGS) == 0u);
    cogmaster__sio_check(io, int_line ==
                                 (((status >> 7) & (r1 >> 5) & 1u) | (pending & (r0 >> 4) & 1u)));
    return height;
}

static void ram_io(struct cogmaster__sio *io, cogmaster *m)
{
    for (uint32_t i = 0; i < COGMASTER__RAM_SIZE; i++) {
        cogmaster__sio_u8(io, "ram", &m->ram[i], 0u, UINT8_MAX);
    }
}

/* The 93C46 of the Sega EEPROM board: phase 0 to 4 (idle, command, data in, read,
   command complete), the bit count within the phase, the flags b0 to b4. Chip
   select low (flag b2) means idle with count 0, and the bits taken never pass the
   phase's width: 7 after the start bit in a command (the 8th completes it), 15 in
   data in and in a read (the 16th wraps the count), none in the other phases. Data
   Out low (b0) only in a read, which holds a READ opcode; data in holds WRITE or
   WRAL; a program waits (b4) only after a complete WRITE, ERASE, WRAL or ERAL with
   writes enabled (b1). */
static void eeprom_io(struct cogmaster__sio *io, struct cogmaster__eeprom *e)
{
    cogmaster__sio_u8(io, "eeprom_last", &e->last, 0u, UINT8_MAX);
    uint8_t phase = cogmaster__sio_u8(io, "eeprom_phase", &e->phase, 0u, 4u);
    uint8_t count = cogmaster__sio_u8(io, "eeprom_count", &e->count, 0u, 15u);
    uint8_t cmd = cogmaster__sio_u8(io, "eeprom_cmd", &e->cmd, 0u, UINT8_MAX);
    cogmaster__sio_u16(io, "eeprom_data", &e->data, 0u, UINT16_MAX);
    uint8_t flags = cogmaster__sio_u8(io, "eeprom_flags", &e->flags, 0u, 0x1Fu);
    uint8_t op = (uint8_t)(cmd >> 6), ext = (uint8_t)(cmd >> 4);
    cogmaster__sio_check(io, phase == 1u || phase == 2u || phase == 3u || count == 0u);
    cogmaster__sio_check(io, phase != 1u || count <= 7u);
    cogmaster__sio_check(io, (flags & 4u) != 0u || (phase == 0u && count == 0u));
    cogmaster__sio_check(io, (flags & 1u) == 0u || phase == 3u);
    cogmaster__sio_check(io, phase != 3u || op == 2u);
    cogmaster__sio_check(io, phase != 2u || op == 1u || ext == 1u);
    cogmaster__sio_check(io, (flags & 0x10u) == 0u || (phase == 4u && (flags & 2u) != 0u &&
                                                       op != 2u && ext != 0u && ext != 3u));
}

/* The board is the image's, so only its own value loads. Each bank is below
   the page count, and the Codemasters board keeps only its RAM bit; the Sega
   EEPROM board keeps the window bit and the reset bit. */
static void mapper_io(struct cogmaster__sio *io, cogmaster *m)
{
    struct cogmaster__mapper *p = &m->mapper;
    uint32_t board = m->cart.board;
    fixed(io, "mapper_board", board, 1u);
    cogmaster__sio_u64(io, "battery_generation", &m->cart.generation, 0u,
                       cogmaster__battery_length(m) != 0u ? UINT64_MAX : 0u);
    uint8_t ctrl_max = board == COGMASTER__BOARD_SEGA          ? UINT8_MAX
                       : board == COGMASTER__BOARD_CODEMASTERS ? 0x80u
                       : board == COGMASTER__BOARD_SEGA_EEPROM ? 0x88u
                                                               : 0u;
    uint8_t ctrl = cogmaster__sio_u8(io, "mapper_ctrl", &p->ctrl, 0u, ctrl_max);
    uint8_t bank_max = (uint8_t)(m->cart.pages != 0u ? m->cart.pages - 1u : 0u);
    for (uint32_t i = 0; i < 3u; i++) {
        cogmaster__sio_u8(io, "mapper_bank", &p->bank[i], 0u, bank_max);
    }
    if (board >= COGMASTER__BOARD_KOREAN && board <= COGMASTER__BOARD_4PAK) {
        uint8_t bank8_max = (uint8_t)(m->cart.pages != 0u ? 2u * m->cart.pages - 1u : 0u);
        for (uint32_t i = 0; i < 6u; i++) {
            cogmaster__sio_u8(io, "mapper_bank8", &p->bank8[i], 0u, bank8_max);
        }
        for (uint32_t i = 0; i < 2u; i++) {
            cogmaster__sio_u8(io, "mapper_raw", &p->raw[i], 0u, UINT8_MAX);
        }
    }
    if (board == COGMASTER__BOARD_SEGA_EEPROM) {
        eeprom_io(io, &m->eeprom);
    }
    for (uint32_t i = 0; i < m->cart.ram_size; i++) {
        cogmaster__sio_u8(io, "cart_ram", &m->cart.ram[i], 0u, UINT8_MAX);
    }
    cogmaster__sio_check(io, board != COGMASTER__BOARD_CODEMASTERS || (ctrl & 0x7Fu) == 0u);
    cogmaster__sio_check(io, board != COGMASTER__BOARD_SEGA_EEPROM || (ctrl & 0x77u) == 0u);
}

/* The Game Gear in native mode decodes no $3E, so its enables stay as the
   hand-off left them; its $05 keeps no status flag. */
static uint8_t io_io(struct cogmaster__sio *io, cogmaster *m)
{
    struct cogmaster__io *p = &m->io;
    int gg = cogmaster__native_gg(m);
    cogmaster__sio_u8(io, "io_mem_ctrl", &p->mem_ctrl, gg ? 0xA8u : 0u, gg ? 0xA8u : UINT8_MAX);
    cogmaster__sio_u8(io, "io_ctrl", &p->io_ctrl, 0u, UINT8_MAX);
    for (uint32_t i = 0; i < 6u; i++) {
        uint8_t v = cogmaster__sio_u8(io, "io_gg", &p->gg[i], 0u, UINT8_MAX);
        cogmaster__sio_check(io, i != 4u || (v & 0x07u) == 0u);
    }
    /* The audio control, which only the FM unit's $F2 writes, within the bits the unit keeps. */
    uint8_t audio_ctrl =
        cogmaster__sio_u8(io, "io_audio_ctrl", &p->audio_ctrl, 0u, (uint8_t)m->fm_mask);
    /* The glasses eye is only ever set by a write, together with the eye the frame reports, and a
       Game Gear never records one. */
    uint8_t eye = cogmaster__sio_u8(io, "io_glasses", &p->glasses, 0u, 2u);
    uint8_t eye_frame = cogmaster__sio_u8(io, "io_glasses_frame", &p->glasses_frame, 0u, 2u);
    cogmaster__sio_check(io, eye_frame == 0u || eye != 0u);
    cogmaster__sio_check(io, m->machine != COGMASTER_MACHINE_GG || (eye == 0u && eye_frame == 0u));
    return audio_ctrl;
}

/* The cadence phase is a fraction of a sample, below the modulus of the
   instance's TV (synth.c). The synthesis is saved with it: the ring slots at
   and after the head (an event lands at most a sample past the frame's end, so
   32 hold every pending tap), the integrator, the DC estimate, the head and
   each source's level and gained level. State is saved between frames, so the
   other ring slots are empty and are zeroed on load. The bounds come from the
   kernel table and the mixer gains (synth.c): a slot and the integrator stay
   inside 2^56, the DC estimate inside its share of it, a gained level inside
   the largest the profile's gain allows; a Master System has one output, so
   the second channel is zero. The undrained PCM and the trace log are the
   host's queue, not state: loading a state discards the PCM so the next frame
   does not count it as dropped. */
static void synth_io(struct cogmaster__sio *io, cogmaster *m, uint32_t fm_levels[2])
{
    struct cogmaster__synth *s = &m->synth;
    uint32_t mod = m->tv == COGMASTER_TV_PAL ? COGMASTER__AUDIO_MOD_PAL : COGMASTER__AUDIO_MOD_NTSC;
    cogmaster__sio_u32(io, "synth_phase", &s->phase, 0u, mod - 1u);
    if (io->mode == COGMASTER__SIO_APPLY) {
        memset(s->ring, 0, sizeof s->ring);
    }
    uint32_t head = cogmaster__sio_u32(io, "synth_head", &s->head, 0u, COGMASTER__SYNTH_RING - 1u);
    uint32_t channels = m->machine == COGMASTER_MACHINE_GG ? 2u : 1u;
    for (uint32_t ch = 0u; ch < 2u; ch++) {
        int ok = ch < channels;
        int64_t slot_max = ok ? COGMASTER__SYNTH_SLOT_MAX : 0;
        int64_t integ_max = ok ? COGMASTER__SYNTH_INTEG_MAX : 0;
        int64_t dc_max = ok ? COGMASTER__SYNTH_DC_MAX : 0;
        cogmaster__sio_i64(io, "synth_integ", &s->integ[ch], -integ_max, integ_max);
        cogmaster__sio_i64(io, "synth_dc", &s->dc[ch], -dc_max, dc_max);
        for (uint32_t i = 0u; i < COGMASTER__SYNTH_SAVED; i++) {
            int64_t *slot = &s->ring[ch][(head + i) & (COGMASTER__SYNTH_RING - 1u)];
            cogmaster__sio_i64(io, "synth_ring", slot, -slot_max, slot_max);
        }
    }
    /* Sources 5 and 6 are the FM unit's: its sum as 16 bits and the control it leaves. */
    const uint32_t level_max[COGMASTER__SYNTH_SOURCES] = {
        0x1Fu, 0x1Fu, 0x1Fu, 0x1Fu, 0xFFu, m->fm_unit ? 0xFFFFu : 0u, m->fm_mask};
    for (uint32_t src = 0u; src < COGMASTER__SYNTH_SOURCES; src++) {
        uint32_t level = cogmaster__sio_u32(io, "synth_level", &s->level[src], 0u, level_max[src]);
        if (src >= 5u) {
            fm_levels[src - 5u] = level;
        }
        uint32_t gmax = cogmaster__synth_gained_max(m, src);
        for (uint32_t ch = 0u; ch < 2u; ch++) {
            cogmaster__sio_u32(io, "synth_gained", &s->gained[src][ch], 0u,
                               ch < channels ? gmax : 0u);
        }
    }
    if (io->mode == COGMASTER__SIO_APPLY) {
        s->pcm_frames = 0u;
        s->pcm_read = 0u;
    }
}

/* The PSG. The step origin is a step tick, so a multiple of 48 and at most
   the tick count; the chip runs lazily from it, so a loaded origin more than a
   frame (plus the longest instruction and a step) behind the tick count would
   make the next catch-up run far past one frame's steps and is refused. A tone
   counter holds at most its period's 10 bits; the noise rate counter at most
   64, the slowest reload. The shift register is never 0: it resets to $8000 and
   feeds back from bit 0, so a zero register would stay silent for ever. The
   latch names one of 8 registers (SMS-AUDIO section 2.1). */
#define STATE_PSG_STEP 48u
static void psg_io(struct cogmaster__sio *io, cogmaster *m, struct state_time time)
{
    struct cogmaster__psg *p = &m->psg;
    uint64_t at = cogmaster__sio_u64(io, "psg_at", &p->at, 0u, STATE_TICKS_MAX);
    for (uint32_t i = 0u; i < 3u; i++) {
        cogmaster__sio_u16(io, "psg_period", &p->period[i], 0u, 1023u);
    }
    for (uint32_t i = 0u; i < 4u; i++) {
        cogmaster__sio_u16(io, "psg_counter", &p->counter[i], 0u, i < 3u ? 1023u : 64u);
    }
    uint16_t lfsr = cogmaster__sio_u16(io, "psg_lfsr", &p->lfsr, 0u, UINT16_MAX);
    for (uint32_t i = 0u; i < 4u; i++) {
        cogmaster__sio_u8(io, "psg_out", &p->out[i], 0u, 1u);
    }
    for (uint32_t i = 0u; i < 4u; i++) {
        cogmaster__sio_u8(io, "psg_att", &p->att[i], 0u, 15u);
    }
    cogmaster__sio_u8(io, "psg_noise", &p->noise, 0u, 7u);
    cogmaster__sio_u8(io, "psg_noise_div", &p->noise_div, 0u, 1u);
    cogmaster__sio_u8(io, "psg_latch", &p->latch, 0u, 7u);
    cogmaster__sio_check(io, lfsr != 0u);
    cogmaster__sio_check(io, at % STATE_PSG_STEP == 0u && at <= time.ticks);
    cogmaster__sio_check(io, time.ticks - at <= (uint64_t)cogmaster__frame_ticks(m) +
                                                    STATE_STEP_TICKS_MAX + STATE_PSG_STEP);
}

/* A signed field of width bytes inside [min, max]. It rides the unsigned machinery with half
   the width's range added, so the stored value and its bounds compare as unsigned and no
   signed shift or overflow is involved. */
static int32_t signed_field(struct cogmaster__sio *io, const char *name, int32_t v, uint32_t width,
                            int32_t min, int32_t max)
{
    int64_t bias = INT64_C(1) << (8u * width - 1u);
    uint64_t u = cogmaster__sio_field(io, name, (uint64_t)(v + bias), width, (uint64_t)(min + bias),
                                      (uint64_t)(max + bias));
    return (int32_t)((int64_t)u - bias);
}

static int32_t i16_io(struct cogmaster__sio *io, const char *name, int16_t *v, int32_t min,
                      int32_t max)
{
    int32_t x = signed_field(io, name, *v, 2u, min, max);
    if (io->mode == COGMASTER__SIO_APPLY) {
        *v = (int16_t)x;
    }
    return x;
}

/* The user instrument, patches 0 (modulator) and 1 (carrier); the carrier's total level and
   feedback are never written, so they stay 0. Every field indexes a table or a shift, so each
   is held to the bits a register write can give it. */
static void opll_patch_io(struct cogmaster__sio *io, struct cogmaster__opll_patch *p, int carrier)
{
    cogmaster__sio_u8(io, "opll_patch_tl", &p->tl, 0u, carrier ? 0u : 63u);
    cogmaster__sio_u8(io, "opll_patch_fb", &p->fb, 0u, carrier ? 0u : 7u);
    cogmaster__sio_u8(io, "opll_patch_eg", &p->eg, 0u, 1u);
    cogmaster__sio_u8(io, "opll_patch_ml", &p->ml, 0u, 15u);
    cogmaster__sio_u8(io, "opll_patch_ar", &p->ar, 0u, 15u);
    cogmaster__sio_u8(io, "opll_patch_dr", &p->dr, 0u, 15u);
    cogmaster__sio_u8(io, "opll_patch_sl", &p->sl, 0u, 15u);
    cogmaster__sio_u8(io, "opll_patch_rr", &p->rr, 0u, 15u);
    cogmaster__sio_u8(io, "opll_patch_kr", &p->kr, 0u, 1u);
    cogmaster__sio_u8(io, "opll_patch_kl", &p->kl, 0u, 3u);
    cogmaster__sio_u8(io, "opll_patch_am", &p->am, 0u, 1u);
    cogmaster__sio_u8(io, "opll_patch_pm", &p->pm, 0u, 1u);
    cogmaster__sio_u8(io, "opll_patch_ws", &p->ws, 0u, 1u);
}

/* One operator slot: the registers' effect and the values derived from them, which a load
   must not recompute (the first sample would change). It returns the instrument index as read,
   for the cross check against the voice's number. Bounds follow the tables the values
   index: the key scale table has 128 rows, the rate tables 4 and the shifts at most 13. The
   largest total level is the table's 112 plus twice the largest level, 63. */
#define STATE_OPLL_TLL_MAX 238u
#define STATE_OPLL_VOLUME_MAX 60u
static uint8_t opll_slot_io(struct cogmaster__sio *io, struct cogmaster__opll_slot *s)
{
    cogmaster__sio_u8(io, "opll_slot_type", &s->type, 0u, 3u);
    uint8_t patch =
        cogmaster__sio_u8(io, "opll_slot_patch", &s->patch, 0u, COGMASTER__OPLL_PATCHES - 1u);
    cogmaster__sio_u8(io, "opll_slot_ws", &s->ws, 0u, 1u);
    for (uint32_t i = 0u; i < 2u; i++) {
        i16_io(io, "opll_slot_output", &s->output[i], -(int32_t)COGMASTER__OPLL_DRUM_PEAK,
               (int32_t)COGMASTER__OPLL_DRUM_PEAK);
    }
    cogmaster__sio_u32(io, "opll_slot_pg_phase", &s->pg_phase, 0u, (1u << 19) - 1u);
    cogmaster__sio_u16(io, "opll_slot_pg_out", &s->pg_out, 0u, 1023u);
    cogmaster__sio_u8(io, "opll_slot_pg_keep", &s->pg_keep, 0u, 1u);
    uint16_t blk_fnum = cogmaster__sio_u16(io, "opll_slot_blk_fnum", &s->blk_fnum, 0u, 0xFFFu);
    uint16_t fnum = cogmaster__sio_u16(io, "opll_slot_fnum", &s->fnum, 0u, 0x1FFu);
    uint8_t blk = cogmaster__sio_u8(io, "opll_slot_blk", &s->blk, 0u, 7u);
    cogmaster__sio_u8(io, "opll_slot_eg_state", &s->eg_state, 0u, OPLL_DAMP);
    cogmaster__sio_u8(io, "opll_slot_volume", &s->volume, 0u, STATE_OPLL_VOLUME_MAX);
    cogmaster__sio_u8(io, "opll_slot_key_flag", &s->key_flag, 0u, 1u);
    cogmaster__sio_u8(io, "opll_slot_sus_flag", &s->sus_flag, 0u, 1u);
    cogmaster__sio_u16(io, "opll_slot_tll", &s->tll, 0u, STATE_OPLL_TLL_MAX);
    cogmaster__sio_u8(io, "opll_slot_rks", &s->rks, 0u, 15u);
    cogmaster__sio_u8(io, "opll_slot_eg_rate_h", &s->eg_rate_h, 0u, 15u);
    cogmaster__sio_u8(io, "opll_slot_eg_rate_l", &s->eg_rate_l, 0u, 3u);
    cogmaster__sio_u8(io, "opll_slot_eg_shift", &s->eg_shift, 0u, 13u);
    cogmaster__sio_u8(io, "opll_slot_eg_out", &s->eg_out, 0u, OPLL_EG_MUTE);
    cogmaster__sio_u8(io, "opll_slot_update", &s->update_requests, 0u, OPLL_UPDATE_ALL);
    /* The block and f-number are set together, so the pair of them is what the slot holds. */
    cogmaster__sio_check(io, (uint32_t)blk_fnum == ((uint32_t)blk << 9 | (uint32_t)fnum));
    return patch;
}

/* The YM2413 unit, present only with fm_unit. Every field of the chip but its slot-evaluation
   counter, and the nine voices' instrument numbers, are saved, the pointers of the original
   being indices here already. The chip's last sum and the synthesiser's last FM level are one
   value, and the control the synthesiser holds is the one the I/O ports hold. The next
   sample's tick is a multiple of 216 past the last frame's deadline and at most one sample
   past the tick count, which keeps the next catch-up to a frame's worth of samples. */
#define STATE_OPLL_SAMPLE_TICKS 216u
static void opll_io(struct cogmaster__sio *io, cogmaster *m, struct state_time time,
                    const uint32_t levels[2], uint8_t ctrl)
{
    struct cogmaster__opll *o = &m->opll;
    uint8_t reg14 = 0u;
    uint8_t reg15 = 0u;
    for (uint32_t i = 0u; i < 0x40u; i++) {
        uint8_t r = cogmaster__sio_u8(io, "opll_reg", &o->reg[i], 0u, UINT8_MAX);
        reg14 = i == 0x0Eu ? r : reg14;
        reg15 = i == 0x0Fu ? r : reg15;
    }
    uint8_t test = cogmaster__sio_u8(io, "opll_test_flag", &o->test_flag, 0u, UINT8_MAX);
    uint8_t rhythm = cogmaster__sio_u8(io, "opll_rhythm_mode", &o->rhythm_mode, 0u, 1u);
    cogmaster__sio_u8(io, "opll_lfo_am", &o->lfo_am, 0u, 13u);
    cogmaster__sio_u8(io, "opll_short_noise", &o->short_noise, 0u, 1u);
    cogmaster__sio_u8(io, "opll_latch", &o->latch, 0u, UINT8_MAX);
    cogmaster__sio_u32(io, "opll_key_status", &o->slot_key_status, 0u, (1u << 18) - 1u);
    cogmaster__sio_u32(io, "opll_eg_counter", &o->eg_counter, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "opll_pm_phase", &o->pm_phase, 0u, UINT32_MAX);
    cogmaster__sio_u32(io, "opll_am_phase", &o->am_phase, 0u, 210u * 64u - 1u);
    cogmaster__sio_u32(io, "opll_noise", &o->noise, 1u, (1u << 23) - 1u);
    int32_t sum = 0;
    int32_t held = signed_field(io, "opll_sum", o->sum, 4u, -(int32_t)COGMASTER__OPLL_SUM_ALL,
                                (int32_t)COGMASTER__OPLL_SUM_ALL);
    if (io->mode == COGMASTER__SIO_APPLY) {
        o->sum = held;
    }
    uint8_t numbers[9];
    for (uint32_t i = 0u; i < 9u; i++) {
        numbers[i] = cogmaster__sio_u8(io, "opll_patch_number", &o->patch_number[i], 0u, 18u);
    }
    for (uint32_t i = 0u; i < COGMASTER__OPLL_CHANNELS; i++) {
        int32_t peak = (int32_t)(i < 9u ? COGMASTER__OPLL_TONE_PEAK : COGMASTER__OPLL_DRUM_PEAK);
        sum += i16_io(io, "opll_ch_out", &o->ch_out[i], -peak, peak);
    }
    uint8_t patch[COGMASTER__OPLL_SLOTS];
    for (uint32_t i = 0u; i < COGMASTER__OPLL_SLOTS; i++) {
        patch[i] = opll_slot_io(io, &o->slot[i]);
    }
    for (uint32_t i = 0u; i < 2u; i++) {
        opll_patch_io(io, &o->patch[i], (int)i);
    }
    uint64_t next = cogmaster__sio_u64(io, "fm_next", &m->fm_next, STATE_OPLL_SAMPLE_TICKS,
                                       STATE_TICKS_MAX + STATE_OPLL_SAMPLE_TICKS);
    cogmaster__sio_check(io, test == reg15 && (uint32_t)rhythm == (((uint32_t)reg14 >> 5) & 1u));
    cogmaster__sio_check(io, held == sum);
    cogmaster__sio_check(io, levels[0] == (uint32_t)(uint16_t)held && levels[1] == (uint32_t)ctrl);
    /* A voice's two slots play the two halves of its instrument. */
    for (uint32_t i = 0u; i < 9u; i++) {
        cogmaster__sio_check(io,
                             (uint32_t)patch[2u * i] == 2u * (uint32_t)numbers[i] &&
                                 (uint32_t)patch[2u * i + 1u] == 2u * (uint32_t)numbers[i] + 1u);
    }
    cogmaster__sio_check(io, next % STATE_OPLL_SAMPLE_TICKS == 0u);
    cogmaster__sio_check(io, next > time.frame * cogmaster__frame_ticks(m));
    cogmaster__sio_check(io, next <= time.ticks + STATE_OPLL_SAMPLE_TICKS);
}

/* The motion waiting for the next frame, and the two ports' device blocks, which follow the input
   records because the cross-checks read the devices there. A Sports Pad or Paddle's pending axes
   are motion within the call's range; a frame takes them and zeroes both records, so those the
   input record holds are 0. A Light Phaser's axes are a position instead: the pending pair of a
   port whose next device is the phaser and the active pair of a port whose held device is the
   phaser are each a canonical aim (0..256 by 0..240, both zero or both non-zero), and every other
   port holds 0 0 in the active record and the existing rule in the pending one. A port's block
   belongs to the device in force: a pad and nothing hold none; a Sports Pad holds no position; a
   Paddle only its position; a phaser only its six gun fields, which are all zero while the device
   is another one. A block's edge is a past tick and is otherwise free, because after a reset the
   first fall leaves edge 0 with step 1 and a rise at tick 0 gives step 2 with edge 0. A val of 0
   is the one a step 0 holds, acc is free there: it gathers at frame start. The gun's cursor is a
   row below the picture height while a scan runs, with an aim snapshot, and is zero with the lit
   bit clear while none runs; the seen bit is free, because a scan can end and the bit stay set
   until the wrap. No rule ties the snapshot to the active pair: it is the previous picture's. */
#define STATE_AXIS_MAX 32767
#define STATE_ACC_MAX 1023
#define STATE_AIM_X 256
#define STATE_AIM_Y 240
static int aim_ok(int32_t x, int32_t y)
{
    return (x == 0) == (y == 0) && x >= 0 && x <= STATE_AIM_X && y >= 0 && y <= STATE_AIM_Y;
}

static void ports_io(struct cogmaster__sio *io, cogmaster *m, struct state_time time,
                     uint32_t device[2][2], uint8_t height)
{
    cogmaster_input *pi = &m->pending_input;
    cogmaster_input *ai = &m->input;
    int32_t pending[2][2];
    int32_t active[2][2];
    pending[0][0] = cogmaster__sio_i32(io, "pending_x1", &pi->x1, -STATE_AXIS_MAX, STATE_AXIS_MAX);
    pending[0][1] = cogmaster__sio_i32(io, "pending_y1", &pi->y1, -STATE_AXIS_MAX, STATE_AXIS_MAX);
    pending[1][0] = cogmaster__sio_i32(io, "pending_x2", &pi->x2, -STATE_AXIS_MAX, STATE_AXIS_MAX);
    pending[1][1] = cogmaster__sio_i32(io, "pending_y2", &pi->y2, -STATE_AXIS_MAX, STATE_AXIS_MAX);
    active[0][0] = cogmaster__sio_i32(io, "active_x1", &ai->x1, 0, STATE_AIM_X);
    active[0][1] = cogmaster__sio_i32(io, "active_y1", &ai->y1, 0, STATE_AIM_Y);
    active[1][0] = cogmaster__sio_i32(io, "active_x2", &ai->x2, 0, STATE_AIM_X);
    active[1][1] = cogmaster__sio_i32(io, "active_y2", &ai->y2, 0, STATE_AIM_Y);
    for (uint32_t port = 0u; port < 2u; port++) {
        struct cogmaster__port *b = &m->ports[port];
        uint64_t edge = cogmaster__sio_u64(io, "port_edge", &b->edge, 0u, time.ticks);
        int16_t acc0 =
            cogmaster__sio_i16(io, "port_acc", &b->acc[0], -STATE_ACC_MAX, STATE_ACC_MAX);
        int16_t acc1 =
            cogmaster__sio_i16(io, "port_acc", &b->acc[1], -STATE_ACC_MAX, STATE_ACC_MAX);
        int8_t val0 = cogmaster__sio_i8(io, "port_val", &b->val[0], -127, 127);
        int8_t val1 = cogmaster__sio_i8(io, "port_val", &b->val[1], -127, 127);
        uint8_t step = cogmaster__sio_u8(io, "port_step", &b->step, 0u, 4u);
        uint8_t pos = cogmaster__sio_u8(io, "port_pos", &b->pos, 0u, UINT8_MAX);
        uint16_t gun_ax = cogmaster__sio_u16(io, "port_gun_ax", &b->g_ax, 0u, STATE_AIM_X);
        uint16_t gun_dot = cogmaster__sio_u16(io, "port_gun_dot", &b->g_dot, 0u, STATE_AIM_X);
        uint8_t gun_ay = cogmaster__sio_u8(io, "port_gun_ay", &b->g_ay, 0u, STATE_AIM_Y);
        uint8_t gun_line = cogmaster__sio_u8(io, "port_gun_line", &b->g_line, 0u, STATE_AIM_Y - 1u);
        uint8_t gun_sense = cogmaster__sio_u8(io, "port_gun_sense", &b->g_sense, 0u, 3u);
        uint8_t gun_scan = cogmaster__sio_u8(io, "port_gun_scan", &b->g_scan, 0u, 1u);
        uint32_t held = device[1][port];
        uint32_t next = device[0][port];
        int none = held == COGMASTER_DEVICE_PAD || held == COGMASTER_DEVICE_NONE;
        int gun = held == COGMASTER_DEVICE_LIGHT_PHASER;
        int gun_zero = (gun_ax | gun_dot | gun_ay | gun_line | gun_sense | gun_scan) == 0u;
        cogmaster__sio_check(
            io, !none || (edge | (uint64_t)(acc0 | acc1 | val0 | val1) | step | pos) == 0u);
        cogmaster__sio_check(io, held != COGMASTER_DEVICE_SPORTS_PAD || pos == 0u);
        cogmaster__sio_check(io, held != COGMASTER_DEVICE_PADDLE ||
                                     (edge | (uint64_t)(acc0 | acc1 | val0 | val1) | step) == 0u);
        cogmaster__sio_check(
            io, !gun || (edge | (uint64_t)(acc0 | acc1 | val0 | val1) | step | pos) == 0u);
        cogmaster__sio_check(io, step != 0u || (val0 == 0 && val1 == 0));
        cogmaster__sio_check(io, cogmaster__device_ok(m, held) && cogmaster__device_ok(m, next));
        int takes_x = next == COGMASTER_DEVICE_SPORTS_PAD || next == COGMASTER_DEVICE_PADDLE;
        cogmaster__sio_check(io, takes_x || next == COGMASTER_DEVICE_LIGHT_PHASER ||
                                     pending[port][0] == 0);
        cogmaster__sio_check(io, next == COGMASTER_DEVICE_SPORTS_PAD ||
                                     next == COGMASTER_DEVICE_LIGHT_PHASER ||
                                     pending[port][1] == 0);
        cogmaster__sio_check(io, next != COGMASTER_DEVICE_LIGHT_PHASER ||
                                     aim_ok(pending[port][0], pending[port][1]));
        cogmaster__sio_check(io, gun ? aim_ok(active[port][0], active[port][1])
                                     : (active[port][0] | active[port][1]) == 0);
        cogmaster__sio_check(io, gun || gun_zero);
        cogmaster__sio_check(io, (gun_ax == 0u) == (gun_ay == 0u));
        cogmaster__sio_check(io, gun_scan != 0u || (gun_line | gun_dot | (gun_sense & 1u)) == 0u);
        cogmaster__sio_check(io, gun_scan == 0u || (gun_ax != 0u && gun_line < height));
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
    uint8_t height = vdp_io(io, &m->vdp, int_line, cogmaster__native_gg(m));
    cogmaster__sio_check(io, m->machine != COGMASTER_MACHINE_SMS1 || height == 192u);
    ram_io(io, m);
    mapper_io(io, m);
    uint8_t audio_ctrl = io_io(io, m);
    uint32_t fm_levels[2];
    synth_io(io, m, fm_levels);
    psg_io(io, m, time);
    if (m->fm_unit) {
        opll_io(io, m, time, fm_levels, audio_ctrl);
    }
    uint32_t device[2][2];
    cogmaster_input *inputs[2] = {&m->pending_input, &m->input};
    for (uint32_t i = 0u; i < 2u; i++) {
        cogmaster_input *p = inputs[i];
        cogmaster__sio_u32(io, "input_p1", &p->p1, 0u, 63u);
        cogmaster__sio_u32(io, "input_p2", &p->p2, 0u, 63u);
        cogmaster__sio_u32(io, "input_pause", &p->pause, 0u, 1u);
        cogmaster__sio_u32(io, "input_reset", &p->reset, 0u, 1u);
        cogmaster__sio_u32(io, "input_start", &p->start, 0u, 1u);
        device[i][0] = cogmaster__sio_u32(io, "input_port1", &p->port1, 0u, COGMASTER__DEVICE_LAST);
        device[i][1] = cogmaster__sio_u32(io, "input_port2", &p->port2, 0u, COGMASTER__DEVICE_LAST);
    }
    ports_io(io, m, time, device, height);
    cogmaster__sio_u8(io, "input_read", &m->input_read, 0u, 1u);
    uint32_t line =
        cogmaster__sio_u32(io, "render_line", &m->render_line, 0u, COGMASTER__FIELD_MAX_H);
    uint32_t dot = cogmaster__sio_u32(io, "render_x", &m->render_x, 0u, COGMASTER__FIELD_W - 1u);
    cogmaster__sio_check(io, line <= height && (line < height || dot == 0u));
    uint32_t completed =
        cogmaster__sio_u32(io, "completed_height", &m->completed.height, 192u, 240u);
    cogmaster__sio_check(io, completed == 192u || completed == 224u || completed == 240u);
    cogmaster__sio_check(io, m->machine != COGMASTER_MACHINE_SMS1 || completed == 192u);
    cogmaster__sio_u32(io, "completed_backdrop", &m->completed.backdrop, 0u, 0xFFFu);
    if (io->mode == COGMASTER__SIO_APPLY) {
        m->completed.crop = cogmaster__crop(cogmaster__native_gg(m) ? COGMASTER_MACHINE_GG
                                                                    : COGMASTER_MACHINE_SMS2);
        if (!cogmaster__native_gg(m))
            m->completed.crop.h = completed;
        else if (completed == 224u)
            m->completed.crop.y = 40u;
    }
    uint32_t lcd_h = cogmaster__sio_u32(io, "lcd_height", &m->lcd.height, 0u, 224u);
    cogmaster__sio_check(io, lcd_h == 192u || (cogmaster__native_gg(m) && lcd_h == 224u));
    uint32_t lcd_y = cogmaster__sio_u32(io, "lcd_y", &m->lcd.y, 0u, 40u);
    cogmaster__sio_check(io, lcd_y == (lcd_h == 224u ? 40u : 24u));
    cogmaster__sio_u32(io, "lcd_backdrop", &m->lcd.backdrop, 0u, 0xFFFu);
    cogmaster__sio_u8(io, "lcd_frozen", &m->lcd.frozen, 0u, cogmaster__native_gg(m) ? 1u : 0u);
    for (uint32_t y = 0u; y < 144u; y++)
        for (uint32_t x = 0u; x < 160u; x++)
            cogmaster__sio_u16(io, "lcd_pixel", &m->lcd.pixels[y][x], 0u, 0xFFFu);
    for (uint32_t buffer = 0u; buffer < 2u; buffer++) {
        for (uint32_t y = 0u; y < COGMASTER__FIELD_MAX_H; y++) {
            for (uint32_t x = 0u; x < COGMASTER__FIELD_W; x++) {
                uint16_t *pixel = buffer == 0u ? &m->field[y][x] : &m->completed.pixels[y][x];
                cogmaster__sio_u16(io, buffer == 0u ? "field_pixel" : "completed_pixel", pixel, 0u,
                                   0xFFFu);
            }
        }
    }
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
