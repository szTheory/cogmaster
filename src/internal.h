/* Declarations shared by the core's source files. Not installed. */
#ifndef COGMASTER_INTERNAL_H
#define COGMASTER_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "cogmaster.h"
#include "opll.h"
#include "z80.h"

/* The field buffer is 256 dots wide and up to 240 lines tall; the test card
   and the 192-line mode use the first 192 lines (ARCHITECTURE section 5). */
#define COGMASTER__FIELD_W 256u
#define COGMASTER__FIELD_MAX_H 240u
#define COGMASTER__FIELD_H 192u

/* The rectangle of the field a machine shows: the SMS shows all 256x192
   dots, the Game Gear a 160x144 window at (48, 24) (ARCHITECTURE section 5). */
typedef struct cogmaster__rect {
    uint32_t x, y, w, h;
} cogmaster__rect;
cogmaster__rect cogmaster__crop(uint32_t machine);

/* A frame is 262 lines of 684 ticks on NTSC and 313 on PAL (ARCHITECTURE
   section 2, SMS-SYSTEM section 1). */
#define COGMASTER__LINES_NTSC 262u
#define COGMASTER__LINES_PAL 313u
#define COGMASTER__TICKS_PER_FRAME_NTSC 179208u
#define COGMASTER__TICKS_PER_FRAME_PAL 214092u

/* The largest pitch get_frame accepts, so a row offset stays inside a 32-bit
   size_t. */
#define COGMASTER__PITCH_MAX 65536u

/* The counters every instance keeps (ARCHITECTURE section 8). The test card
   runs no CPU and therefore counts as a lag frame. */
struct cogmaster__counters {
    uint32_t lag_frames;
    uint32_t frame_ints;
    uint32_t line_ints;
    uint32_t nmis;
    uint32_t open_bus_reads;
    uint32_t audio_dropped;
    uint32_t vdp_writes_active;
    uint32_t vdp_fast_accesses;
    uint32_t sprite_overflow_lines;
    uint32_t collision_frames;
};

/* The first fault since load or reset, as cogmaster_fault reports it. */
struct cogmaster__fault {
    uint64_t frame;
    uint32_t line, dot;
    uint16_t pc, addr;
    uint8_t opcode, width, device, kind;
};

/* Fault kinds and devices, as the fault record's kind and device bytes.
   Append-only; 0 is none (docs/embedding.md names them). */
enum {
    COGMASTER__FAULT_NONE = 0,
    COGMASTER__FAULT_UNMAPPED_READ = COGMASTER_FAULT_KIND_UNMAPPED_READ,
    COGMASTER__FAULT_UNMAPPED_WRITE = COGMASTER_FAULT_KIND_UNMAPPED_WRITE,
    COGMASTER__FAULT_ROM_WRITE = COGMASTER_FAULT_KIND_ROM_WRITE,
    COGMASTER__FAULT_MAPPER_RANGE = COGMASTER_FAULT_KIND_MAPPER_RANGE,
    COGMASTER__FAULT_ED_UNDEFINED = COGMASTER_FAULT_KIND_ED_UNDEFINED
};
enum {
    COGMASTER__DEVICE_NONE = COGMASTER_FAULT_DEVICE_NONE,
    COGMASTER__DEVICE_CPU = COGMASTER_FAULT_DEVICE_CPU,
    COGMASTER__DEVICE_CART = COGMASTER_FAULT_DEVICE_CART,
    COGMASTER__DEVICE_RAM = COGMASTER_FAULT_DEVICE_RAM,
    COGMASTER__DEVICE_MAPPER = COGMASTER_FAULT_DEVICE_MAPPER,
    COGMASTER__DEVICE_CTRL = COGMASTER_FAULT_DEVICE_CTRL
};

/* The beam and the frame deadline. line and t are the beam position at tick
   at; the bus brings them up to ticks before each access. */
struct cogmaster__bus {
    uint64_t deadline; /* the tick the current frame ends at */
    uint64_t at;       /* the tick line and t describe */
    uint64_t vdp_at;   /* the tick of the last VDP port access, 0 before one */
    uint32_t line;     /* 0 to 261 (NTSC) or 312 (PAL) */
    uint32_t t;        /* tick in the line, 0 to 683 */
    uint16_t op_pc;    /* the running instruction's address, its first prefix included */
    uint8_t op;        /* the running instruction's first byte after any DD or FD */
    uint8_t data;      /* last driven data-bus byte, SMS-SYSTEM C5 */
};

/* VDP registers, ports, status and the stores shared with the renderer. */
#define COGMASTER__VDP_REGS 11u
#define COGMASTER__VRAM_SIZE 16384u
#define COGMASTER__CRAM_SIZE 64u

struct cogmaster__vdp {
    uint8_t height;                     /* 192/224/240, final-physical-line tick-600 latch */
    uint16_t addr;                      /* the 14-bit VRAM or CRAM address */
    uint16_t vline;                     /* independent 0..262 rolling NTSC-240 counter */
    uint8_t reg[COGMASTER__VDP_REGS];   /* R0 to R10 */
    uint8_t status;                     /* the flags: bit 7 frame interrupt pending */
    uint8_t latch;                      /* 1 after the first control byte */
    uint8_t first;                      /* the first control byte */
    uint8_t code;                       /* the control code, 0 to 3 */
    uint8_t buffer;                     /* the data port's read-ahead byte */
    uint8_t pause;                      /* 1 while a Pause press waits for its NMI tick */
    uint8_t line_counter, line_pending; /* R10 reload counter and independent IE1 source */
    uint8_t hcounter, hheld;            /* captured TH count; live until first capture */
    uint8_t hscroll, vscroll, display;  /* line / frame / line latches, docs/hardware/video.md */
    /* Selected line slots; pattern bytes remain live until each dot emits. */
    struct {
        uint8_t x, tile, row;
    } sprite[8];
    uint8_t sprite_line, sprite_count, sprite_scanned;
    uint8_t sprite_zoom, sprite_shift, sprite_overflow;
    uint8_t cram[COGMASTER__CRAM_SIZE];
    uint8_t cram_latch;
    uint8_t vram[COGMASTER__VRAM_SIZE];
};

/* The VDP's ports after the bus has decoded an address. */
enum {
    COGMASTER__VDP_VCOUNT = 0, /* $7E read */
    COGMASTER__VDP_HCOUNT = 1, /* $7F read */
    COGMASTER__VDP_DATA = 2,   /* $BE */
    COGMASTER__VDP_CONTROL = 3 /* $BF: control on write, status on read */
};

/* The cartridge image, in one allocator buffer padded with $FF to a 16 KiB
   multiple, with the board's cartridge RAM after it in the same buffer. rom
   is NULL when the test card is loaded. Not saved state: it is the media.
   The RAM is state and is saved with the mapper. */
struct cogmaster__cart {
    uint8_t *rom;
    uint8_t *ram;      /* the cartridge RAM, or NULL when the board maps none */
    uint32_t size;     /* bytes in rom, pages times 16 KiB */
    uint32_t pages;    /* 16 KiB pages in rom */
    uint32_t ram_size; /* bytes in ram: 0, 8 KiB or 32 KiB, a power of two */
    uint32_t crc32;    /* of the image after any copier header, before padding */
    uint32_t board;    /* COGMASTER__BOARD_*, 0 for the test card */
    uint32_t persistence, persistent_size, persistence_geometry;
    uint64_t generation;
};

/* Boards, as the probe guesses them. 0 is no supported board. */
enum {
    COGMASTER__BOARD_NONE = 0,
    COGMASTER__BOARD_ROM_ONLY = COGMASTER_BOARD_ROM_ONLY,
    COGMASTER__BOARD_SEGA = COGMASTER_BOARD_SEGA,
    COGMASTER__BOARD_CODEMASTERS = COGMASTER_BOARD_CODEMASTERS,
    COGMASTER__BOARD_KOREAN = COGMASTER_BOARD_KOREAN,
    COGMASTER__BOARD_KOREAN_NB = COGMASTER_BOARD_KOREAN_NB,
    COGMASTER__BOARD_HICOM = COGMASTER_BOARD_HICOM,
    COGMASTER__BOARD_ZEMINA = COGMASTER_BOARD_ZEMINA,
    COGMASTER__BOARD_NEMESIS = COGMASTER_BOARD_NEMESIS,
    COGMASTER__BOARD_4PAK = COGMASTER_BOARD_4PAK,
    COGMASTER__BOARD_SEGA_EEPROM = COGMASTER_BOARD_SEGA_EEPROM
};

/* The mapper's registers. The board is the cartridge's; these are what the
   CPU writes. */
struct cogmaster__mapper {
    uint8_t ctrl;    /* Sega: $FFFC. Codemasters: $80 while RAM is at $A000. */
    uint8_t bank[3]; /* the bank in each 16 KiB slot, each below the page count */
    /* boards 4 and up: the 8 KiB bank in each of the six slots of $0000-$BFFF,
       each below twice the page count */
    uint8_t bank8[6];
    /* boards 4 and up: register values a board needs to recompute a bank */
    uint8_t raw[2];
};

/* The 93C46 serial EEPROM a Game Gear cartridge can carry: 64 words of 16 bits,
   128 bytes in the cartridge RAM, little-endian. The device is a pure state
   machine over the byte a game writes to the cartridge window (b0 data in, b1
   clock, b2 chip select) and shows nothing of the board. Every field is
   bounded so a hostile clock stream costs O(1) per write.
   last: the byte last written to the window.
   phase: 0 idle, 1 command, 2 data in, 3 read, 4 command complete.
   count: bits taken in the phase; at most 8 in a command (after the start
   bit), 16 in WRITE or WRAL data, 15 in READ (the dummy bit shows when the
   command completes, so a read never counts more than the 17 bits it shows).
   cmd: the 2 opcode and 6 address bits, shifted in; in a read it follows the
   address.
   data: the word clocked in for WRITE or WRAL.
   flags: b0 data out is driven low, b1 writes enabled, b2 chip select, b3
   clock, b4 a write-type command is complete and waits for chip select to
   fall. Chip select low means phase 0 and count 0; the zeroed value is the
   powered-up chip. */
struct cogmaster__eeprom {
    uint8_t last;
    uint8_t phase;
    uint8_t count;
    uint8_t cmd;
    uint16_t data;
    uint8_t flags;
};

/* The I/O chip's ports as the CPU last wrote them (SMS-SYSTEM section 4). */
struct cogmaster__io {
    uint8_t mem_ctrl; /* $3E: active-low enables, b6 cartridge, b4 work RAM, b2 I/O chip */
    uint8_t io_ctrl;  /* $3F: b3-0 TR/TH directions (1 = input), b7-4 their output levels */
    uint8_t gg[6];    /* Game Gear $01-$06 */
    /* $F2 as written, below 4: 0 PSG, 1 FM, 2 neither, 3 both; 0 with no FM unit */
    uint8_t audio_ctrl;
};

/* The Master System's work RAM, mirrored over $C000-$FFFF. */
#define COGMASTER__RAM_SIZE 8192u

/* Audio leaves the core at 48 kHz. Per bus tick a frame's sample count grows
   by inc / mod of a sample: the per-Z80-cycle rates 176 / 13,125 (NTSC) and
   9,600 / 709,379 (PAL) with the modulus tripled for three ticks a cycle
   (SMS-AUDIO section 1). Exact, so every frame is 801 or 802 samples (965 or
   966 on PAL) and F frames total floor(F x 3,504,512 / 4,375) (PAL
   floor(F x 685,094,400 / 709,379)). */
#define COGMASTER__AUDIO_INC_NTSC 176u
#define COGMASTER__AUDIO_MOD_NTSC 39375u
#define COGMASTER__AUDIO_INC_PAL 9600u
#define COGMASTER__AUDIO_MOD_PAL 2128137u

/* The most sample frames one video frame produces: the PAL count. */
#define COGMASTER__AUDIO_MAX_FRAMES 966u

/* The most records a host may ask the trace log to hold: 16 MiB. */
#define COGMASTER__TRACE_MAX (1u << 20)

/* Band-limited step synthesis. A level change adds a kernel (16 taps) into an
   impulse ring of 1,024 slots per output channel; each sample integrates one
   slot. Ring slots at and after head are saved: an event lands at most a
   sample past the frame's end, so the 32 saved slots hold every pending tap. */
#define COGMASTER__SYNTH_RING 1024u
#define COGMASTER__SYNTH_TAPS 16u
#define COGMASTER__SYNTH_SAVED 32u
#define COGMASTER__SYNTH_SOURCES 7u

/* Bounds that keep every int64 sum inside range, from the kernel table and
   the mixer (synth.c states the derivation next to its static assertions). A
   slot and the integrator are held at or below 2^56 in magnitude; the DC
   estimate in Q16 sample units at or below the integrator over 2^15. */
#define COGMASTER__SYNTH_SLOT_MAX (INT64_C(1) << 56)
#define COGMASTER__SYNTH_INTEG_MAX (INT64_C(1) << 56)
#define COGMASTER__SYNTH_DC_MAX (COGMASTER__SYNTH_INTEG_MAX / 32768)

/* Counters the cost tests read; never state. */
struct cogmaster__synth_stats {
    uint32_t kernel_apps; /* taps added to a ring */
    uint32_t clamps;      /* samples that left the int16 range */
    uint32_t max_index;   /* the largest sample index plus taps an event reached */
    uint32_t psg_steps;   /* PSG steps taken */
    uint32_t psg_writes;  /* bytes written to the PSG */
};

/* The synthesiser's saved and transient state. The phase is the fraction of
   a sample carried between frames, in [0, mod); the PCM is the last frame's
   output waiting for the host, channels interleaved. level is each source's
   last raw level and gained[source][channel] the gained level last fed to
   the ring, so only the difference enters it. */
struct cogmaster__synth {
    int64_t ring[2][COGMASTER__SYNTH_RING];
    int64_t integ[2];
    int64_t dc[2];
    uint32_t phase;
    uint32_t pcm_frames;    /* sample frames the last frame produced */
    uint32_t pcm_read;      /* of those, the frames the host has taken */
    uint32_t trace_pending; /* records waiting in the instance's trace log */
    uint32_t trace_dropped; /* appends the full log refused since the last drain */
    uint32_t head;          /* ring index of the sample the next frame starts with */
    uint32_t level[COGMASTER__SYNTH_SOURCES];
    uint32_t gained[COGMASTER__SYNTH_SOURCES][2];
    struct cogmaster__synth_stats stats;
    int16_t pcm[COGMASTER__AUDIO_MAX_FRAMES * 2u];
};

/* The Sega PSG's registers and counters (psg.c). at is the tick of the last
   step taken, a multiple of 48. period holds the three 10-bit tone periods;
   counter the three tone down-counters and, in [3], the noise rate counter;
   out the tone flip-flops and, in [3], the noise output bit; noise_div the
   noise divider's flip-flop; latch the register index (channel x 2, plus 1
   for an attenuation) the next data byte writes. */
struct cogmaster__psg {
    uint64_t at;
    uint16_t period[3];
    uint16_t counter[4];
    uint16_t lfsr;
    uint8_t out[4];
    uint8_t att[4];
    uint8_t noise;
    uint8_t noise_div;
    uint8_t latch;
};

struct cogmaster {
    cogmaster_allocator allocator; /* the caller's, copied at create */
    uint64_t frame;                /* frames run since load */
    uint64_t ticks;                /* ticks run since load */
    uint32_t machine;              /* resolved machine: SMS1, SMS2 or GG; valid while loaded */
    uint32_t video_mode;
    uint32_t region;  /* resolved region: EXPORT or JAPAN; valid while loaded */
    uint32_t tv;      /* resolved TV: NTSC or PAL; valid while loaded */
    uint32_t loaded;  /* 1 after a successful load */
    uint32_t fm_unit; /* resolved fm: 1 when the YM2413 is attached, else 0; valid while loaded */
    uint32_t fm_mask; /* the $F2 bits the profile keeps: 3 with the unit, else 0 */
    struct cogmaster__counters counters;
    uint32_t fault_present;        /* 1 once a fault is recorded */
    struct cogmaster__fault fault; /* valid while fault_present */
    struct cogmaster__z80 z80;
    struct cogmaster__bus bus;
    struct cogmaster__vdp vdp;
    struct cogmaster__cart cart;
    struct cogmaster__mapper mapper;
    /* The 93C46 on the Sega EEPROM board; zero and unused on every other board. */
    struct cogmaster__eeprom eeprom;
    struct cogmaster__io io;
    uint32_t render_line;
    uint32_t render_x; /* next unrendered dot of render_line */
    cogmaster_input pending_input, input;
    uint8_t input_read;
    struct cogmaster__synth synth;
    struct cogmaster__psg psg;
    /* The YM2413, reset at every load and reset and run only with fm_unit; fm_next is the tick
       its next sample completes at, a multiple of 216. */
    struct cogmaster__opll opll;
    uint64_t fm_next;
    uint32_t trace_cap;                /* the host's trace_records, kept across loads */
    cogmaster_audio_record *trace_log; /* trace_cap records, allocated by the first load */
    uint8_t ram[COGMASTER__RAM_SIZE];
    uint16_t field[COGMASTER__FIELD_MAX_H][COGMASTER__FIELD_W]; /* RGB444 per dot */
    struct {
        uint32_t height, backdrop, par_num, par_den;
        cogmaster__rect crop;
        uint16_t pixels[COGMASTER__FIELD_MAX_H][COGMASTER__FIELD_W];
    } completed;
    struct {
        uint32_t height, y, backdrop;
        uint8_t frozen;
        uint16_t pixels[144][160];
    } lcd;
};

static inline int cogmaster__native_gg(const cogmaster *m)
{
    return m->video_mode == COGMASTER_VIDEO_GG;
}

/* Shared revision facts: the 315-5124 masks VDP addresses and has no
   pull-ups on an undriven bus (SMS-VDP section 1; SMS-SYSTEM C5). */
struct cogmaster__profile {
    uint8_t vdp_masks, bus_pullup;
};
static inline struct cogmaster__profile cogmaster__profile(const cogmaster *m)
{
    struct cogmaster__profile p;
    p.vdp_masks = (uint8_t)(m->machine == COGMASTER_MACHINE_SMS1);
    p.bus_pullup = p.vdp_masks ? 0u : 0xFFu;
    return p;
}

/* The public layouts, pinned on every target. P is the pointer size; on
   64-bit targets a pointer after a leading size field starts at P, leaving 4
   alignment bytes. */
#define COGMASTER__P sizeof(void *)
_Static_assert(offsetof(cogmaster_allocator, alloc) == 0, "allocator.alloc");
_Static_assert(offsetof(cogmaster_allocator, free) == COGMASTER__P, "allocator.free");
_Static_assert(offsetof(cogmaster_allocator, user) == 2 * COGMASTER__P, "allocator.user");
_Static_assert(sizeof(cogmaster_allocator) == 3 * COGMASTER__P, "allocator size");

_Static_assert(offsetof(cogmaster_config, size) == 0, "config.size");
_Static_assert(offsetof(cogmaster_config, allocator) == COGMASTER__P, "config.allocator");
_Static_assert(offsetof(cogmaster_config, trace_records) == 4 * COGMASTER__P,
               "config.trace_records");
/* The field after the allocator, and on 64-bit an explicit pad to a multiple of P. */
_Static_assert(sizeof(cogmaster_config) == 5 * COGMASTER__P, "config size");

_Static_assert(offsetof(cogmaster_versions, size) == 0, "versions.size");
_Static_assert(offsetof(cogmaster_versions, library_major) == 4, "versions.library_major");
_Static_assert(offsetof(cogmaster_versions, library_minor) == 8, "versions.library_minor");
_Static_assert(offsetof(cogmaster_versions, library_patch) == 12, "versions.library_patch");
_Static_assert(offsetof(cogmaster_versions, api) == 16, "versions.api");
_Static_assert(offsetof(cogmaster_versions, abi) == 20, "versions.abi");
_Static_assert(offsetof(cogmaster_versions, behaviour) == 24, "versions.behaviour");
_Static_assert(offsetof(cogmaster_versions, state) == 28, "versions.state");
_Static_assert(sizeof(cogmaster_versions) == 32, "versions size");

_Static_assert(offsetof(cogmaster_media, size) == 0, "media.size");
_Static_assert(offsetof(cogmaster_media, image) == COGMASTER__P, "media.image");
_Static_assert(offsetof(cogmaster_media, image_size) == 2 * COGMASTER__P, "media.image_size");
_Static_assert(offsetof(cogmaster_media, machine) == 3 * COGMASTER__P, "media.machine");
_Static_assert(offsetof(cogmaster_media, region) == 3 * COGMASTER__P + 4, "media.region");
_Static_assert(offsetof(cogmaster_media, tv) == 3 * COGMASTER__P + 8, "media.tv");
_Static_assert(offsetof(cogmaster_media, video_mode) == (COGMASTER__P == 8 ? 40 : 24),
               "media.video_mode");
/* Explicit pad to the next word, so fm starts at 48 on 64-bit and 28 on 32-bit. */
_Static_assert(offsetof(cogmaster_media, fm) == (COGMASTER__P == 8 ? 48 : 28), "media.fm");
_Static_assert(sizeof(cogmaster_media) == (COGMASTER__P == 8 ? 56 : 32), "media size");

_Static_assert(offsetof(cogmaster_info, size) == 0, "info.size");
_Static_assert(offsetof(cogmaster_info, machine) == 4, "info.machine");
_Static_assert(offsetof(cogmaster_info, crop_w) == 8, "info.crop_w");
_Static_assert(offsetof(cogmaster_info, crop_h) == 12, "info.crop_h");
_Static_assert(offsetof(cogmaster_info, field_w) == 16, "info.field_w");
_Static_assert(offsetof(cogmaster_info, field_h) == 20, "info.field_h");
_Static_assert(offsetof(cogmaster_info, max_w) == 24, "info.max_w");
_Static_assert(offsetof(cogmaster_info, max_h) == 28, "info.max_h");
_Static_assert(offsetof(cogmaster_info, fps_num) == 32, "info.fps_num");
_Static_assert(offsetof(cogmaster_info, fps_den) == 36, "info.fps_den");
_Static_assert(offsetof(cogmaster_info, par_num) == 40, "info.par_num");
_Static_assert(offsetof(cogmaster_info, par_den) == 44, "info.par_den");
_Static_assert(offsetof(cogmaster_info, region) == 48, "info.region");
_Static_assert(offsetof(cogmaster_info, tv) == 52, "info.tv");
_Static_assert(offsetof(cogmaster_info, video_mode) == 56, "info.video_mode");
_Static_assert(offsetof(cogmaster_info, audio_channels) == 60, "info.audio_channels");
_Static_assert(offsetof(cogmaster_info, fm) == 64, "info.fm");
_Static_assert(offsetof(cogmaster_info, default_port1) == 68, "info.default_port1");
_Static_assert(offsetof(cogmaster_info, default_port2) == 72, "info.default_port2");
_Static_assert(sizeof(cogmaster_info) == 76, "info size");

_Static_assert(offsetof(cogmaster_frame, size) == 0, "frame.size");
_Static_assert(offsetof(cogmaster_frame, pixels) == COGMASTER__P, "frame.pixels");
_Static_assert(offsetof(cogmaster_frame, pitch_bytes) == 2 * COGMASTER__P, "frame.pitch_bytes");
_Static_assert(offsetof(cogmaster_frame, flags) == 2 * COGMASTER__P + 4, "frame.flags");
_Static_assert(offsetof(cogmaster_frame, x) == 2 * COGMASTER__P + 8, "frame.x");
_Static_assert(offsetof(cogmaster_frame, y) == 2 * COGMASTER__P + 12, "frame.y");
_Static_assert(offsetof(cogmaster_frame, w) == 2 * COGMASTER__P + 16, "frame.w");
_Static_assert(offsetof(cogmaster_frame, h) == 2 * COGMASTER__P + 20, "frame.h");
_Static_assert(offsetof(cogmaster_frame, field_h) == 2 * COGMASTER__P + 24, "frame.field_h");
_Static_assert(offsetof(cogmaster_frame, backdrop_rgb444) == 2 * COGMASTER__P + 28,
               "frame.backdrop_rgb444");
_Static_assert(sizeof(cogmaster_frame) == 2 * COGMASTER__P + 32, "frame size");

_Static_assert(offsetof(cogmaster_run_result, size) == 0, "run_result.size");
_Static_assert(offsetof(cogmaster_run_result, stop) == 4, "run_result.stop");
_Static_assert(offsetof(cogmaster_run_result, overshoot_ticks) == 8, "run_result.overshoot_ticks");
_Static_assert(offsetof(cogmaster_run_result, lagged) == 12, "run_result.lagged");
_Static_assert(offsetof(cogmaster_run_result, display_enabled) == 16, "run_result.display_enabled");
_Static_assert(offsetof(cogmaster_run_result, glasses) == 20, "run_result.glasses");
_Static_assert(sizeof(cogmaster_run_result) == 24, "run_result size");
_Static_assert(sizeof(cogmaster_input) == 48, "input size");
_Static_assert(offsetof(cogmaster_input, p1) == 4, "input.p1");
_Static_assert(offsetof(cogmaster_input, p2) == 8, "input.p2");
_Static_assert(offsetof(cogmaster_input, pause) == 12, "input.pause");
_Static_assert(offsetof(cogmaster_input, reset) == 16, "input.reset");
_Static_assert(offsetof(cogmaster_input, start) == 20, "input.start");
_Static_assert(offsetof(cogmaster_input, port1) == 24, "input.port1");
_Static_assert(offsetof(cogmaster_input, port2) == 28, "input.port2");
_Static_assert(offsetof(cogmaster_input, x1) == 32, "input.x1");
_Static_assert(offsetof(cogmaster_input, y1) == 36, "input.y1");
_Static_assert(offsetof(cogmaster_input, x2) == 40, "input.x2");
_Static_assert(offsetof(cogmaster_input, y2) == 44, "input.y2");

_Static_assert(offsetof(cogmaster_fault, size) == 0, "fault.size");
_Static_assert(offsetof(cogmaster_fault, present) == 4, "fault.present");
_Static_assert(offsetof(cogmaster_fault, frame) == 8, "fault.frame");
_Static_assert(offsetof(cogmaster_fault, line) == 16, "fault.line");
_Static_assert(offsetof(cogmaster_fault, dot) == 20, "fault.dot");
_Static_assert(offsetof(cogmaster_fault, pc) == 24, "fault.pc");
_Static_assert(offsetof(cogmaster_fault, addr) == 26, "fault.addr");
_Static_assert(offsetof(cogmaster_fault, opcode) == 28, "fault.opcode");
_Static_assert(offsetof(cogmaster_fault, width) == 29, "fault.width");
_Static_assert(offsetof(cogmaster_fault, device) == 30, "fault.device");
_Static_assert(offsetof(cogmaster_fault, kind) == 31, "fault.kind");
_Static_assert(sizeof(cogmaster_fault) == 32, "fault size");

_Static_assert(offsetof(cogmaster_cart_info, size) == 0, "cart_info.size");
_Static_assert(offsetof(cogmaster_cart_info, image_size) == 4, "cart_info.image_size");
_Static_assert(offsetof(cogmaster_cart_info, body_offset) == 8, "cart_info.body_offset");
_Static_assert(offsetof(cogmaster_cart_info, header_at) == 12, "cart_info.header_at");
_Static_assert(offsetof(cogmaster_cart_info, checksum) == 16, "cart_info.checksum");
_Static_assert(offsetof(cogmaster_cart_info, checksum_ok) == 20, "cart_info.checksum_ok");
_Static_assert(offsetof(cogmaster_cart_info, product_code) == 24, "cart_info.product_code");
_Static_assert(offsetof(cogmaster_cart_info, version) == 28, "cart_info.version");
_Static_assert(offsetof(cogmaster_cart_info, region_code) == 32, "cart_info.region_code");
_Static_assert(offsetof(cogmaster_cart_info, rom_size_code) == 36, "cart_info.rom_size_code");
_Static_assert(offsetof(cogmaster_cart_info, machine_guess) == 40, "cart_info.machine_guess");
_Static_assert(offsetof(cogmaster_cart_info, region_guess) == 44, "cart_info.region_guess");
_Static_assert(offsetof(cogmaster_cart_info, board) == 48, "cart_info.board");
_Static_assert(offsetof(cogmaster_cart_info, identified) == 52, "cart_info extension");
_Static_assert(offsetof(cogmaster_cart_info, default_port1) == 528, "cart_info.default_port1");
_Static_assert(offsetof(cogmaster_cart_info, default_port2) == 532, "cart_info.default_port2");
_Static_assert(sizeof(cogmaster_cart_info) == 536, "cart_info size");

/* cogmaster_audio: the buffer pointer first, then four counters. */
_Static_assert(offsetof(cogmaster_audio, size) == 0, "audio.size");
_Static_assert(offsetof(cogmaster_audio, samples) == COGMASTER__P, "audio.samples");
_Static_assert(offsetof(cogmaster_audio, capacity) == 2 * COGMASTER__P, "audio.capacity");
_Static_assert(offsetof(cogmaster_audio, written) == 2 * COGMASTER__P + 4, "audio.written");
_Static_assert(offsetof(cogmaster_audio, channels) == 2 * COGMASTER__P + 8, "audio.channels");
_Static_assert(offsetof(cogmaster_audio, dropped) == 2 * COGMASTER__P + 12, "audio.dropped");
_Static_assert(sizeof(cogmaster_audio) == 2 * COGMASTER__P + 16, "audio size");

/* One trace record is 16 bytes, never hashed as memory; the trace struct is
   the buffer pointer between its size and three counters. */
_Static_assert(offsetof(cogmaster_audio_record, tick) == 0, "audio_record.tick");
_Static_assert(offsetof(cogmaster_audio_record, source) == 8, "audio_record.source");
_Static_assert(offsetof(cogmaster_audio_record, level) == 12, "audio_record.level");
_Static_assert(sizeof(cogmaster_audio_record) == 16, "audio_record size");
_Static_assert(offsetof(cogmaster_audio_trace, size) == 0, "audio_trace.size");
_Static_assert(offsetof(cogmaster_audio_trace, records) == COGMASTER__P, "audio_trace.records");
_Static_assert(offsetof(cogmaster_audio_trace, capacity) == 2 * COGMASTER__P,
               "audio_trace.capacity");
_Static_assert(offsetof(cogmaster_audio_trace, written) == 2 * COGMASTER__P + 4,
               "audio_trace.written");
_Static_assert(offsetof(cogmaster_audio_trace, dropped) == 2 * COGMASTER__P + 8,
               "audio_trace.dropped");
/* 2P + 12 bytes rounded up to a multiple of P: 32 on 64-bit, 20 on 32-bit. */
_Static_assert(sizeof(cogmaster_audio_trace) == (COGMASTER__P == 8 ? 32 : 20), "audio_trace size");

/* The size of each struct as first released. A caller's size tag below it
   is refused; each is pinned to the layout above, so appending a field later
   cannot move it. */
#define COGMASTER__FIRST_SIZE_CONFIG ((uint32_t)(4 * COGMASTER__P))
#define COGMASTER__FIRST_SIZE_MEDIA ((uint32_t)(4 * COGMASTER__P))
#define COGMASTER__FIRST_SIZE_FRAME ((uint32_t)(2 * COGMASTER__P + 32))
#define COGMASTER__FIRST_SIZE_INFO 48u
#define COGMASTER__FIRST_SIZE_VERSIONS 32u
#define COGMASTER__FIRST_SIZE_RUN_RESULT 12u
#define COGMASTER__FIRST_SIZE_FAULT 32u
#define COGMASTER__FIRST_SIZE_CART_INFO 52u
#define COGMASTER__FIRST_SIZE_AUDIO ((uint32_t)(2 * COGMASTER__P + 16))
#define COGMASTER__FIRST_SIZE_AUDIO_TRACE ((uint32_t)(COGMASTER__P == 8 ? 32 : 20))
_Static_assert(COGMASTER__FIRST_SIZE_CONFIG == offsetof(cogmaster_config, trace_records),
               "config first size");
_Static_assert(COGMASTER__FIRST_SIZE_MEDIA == 4 * COGMASTER__P, "media first size");
_Static_assert(COGMASTER__FIRST_SIZE_FRAME == sizeof(cogmaster_frame), "frame first size");
_Static_assert(COGMASTER__FIRST_SIZE_INFO == offsetof(cogmaster_info, region), "info first size");
_Static_assert(COGMASTER__FIRST_SIZE_VERSIONS == sizeof(cogmaster_versions), "versions first size");
_Static_assert(COGMASTER__FIRST_SIZE_RUN_RESULT == offsetof(cogmaster_run_result, lagged),
               "run_result first size");
_Static_assert(COGMASTER__FIRST_SIZE_FAULT == sizeof(cogmaster_fault), "fault first size");
_Static_assert(COGMASTER__FIRST_SIZE_CART_INFO == offsetof(cogmaster_cart_info, identified),
               "cart_info first size");
_Static_assert(COGMASTER__FIRST_SIZE_AUDIO == sizeof(cogmaster_audio), "audio first size");
_Static_assert(COGMASTER__FIRST_SIZE_AUDIO_TRACE == sizeof(cogmaster_audio_trace),
               "audio_trace first size");

/* Input structs: OK when the size tag at s is at least first and at most
   4096, and every byte past ours is zero; ERR_STRUCT_SIZE otherwise. */
cogmaster_status cogmaster__check_size(const void *s, uint32_t first, uint32_t ours);

/* Output structs: OK when the size tag at s is at least first, with no upper
   bound; ERR_STRUCT_SIZE otherwise. */
cogmaster_status cogmaster__check_out_size(const void *s, uint32_t first);

/* Writes min(caller's size tag, ours) bytes of the fully built struct full to
   out; bytes past ours are left as the caller set them. */
void cogmaster__write_out(void *out, const void *full, uint32_t ours);

/* Records f as the instance's fault unless one is already recorded since
   load or reset: the first fault wins. */
void cogmaster__fault_record(cogmaster *m, const struct cogmaster__fault *f);

/* Records a fault of kind on device at the bus address addr, stamped with
   the beam and the running instruction. Call sites are cold arms only. */
void cogmaster__bus_fault(cogmaster *m, uint16_t addr, uint8_t device, uint8_t kind);

/* CRC-32 as in Ethernet and PNG: reflected polynomial 0xEDB88320, initial
   value and final XOR 0xFFFFFFFF. Continue a running CRC by passing it back
   as crc; start from 0. */
uint32_t cogmaster__crc32(uint32_t crc, const uint8_t *p, size_t n);

/* The beam at the frame origin with ticks 0 and the first frame's deadline
   one frame away. Load and both resets call it. */
void cogmaster__bus_origin(cogmaster *m);

/* Where the beam is, as line x 684 + tick, when the bus was last synced at
   tick at in frame frame; at must lie within one frame of the frame's
   start. */
uint32_t cogmaster__bus_beam_at(const cogmaster *m, uint64_t frame, uint64_t at);

/* The loaded TV's frame length in ticks and in lines. */
uint32_t cogmaster__frame_ticks(const cogmaster *m);
uint32_t cogmaster__frame_lines(const cogmaster *m);

/* Brings the beam up to m->ticks. */
void cogmaster__bus_sync(cogmaster *m);

/* Runs the Z80 until the frame deadline and returns the overshoot in ticks;
   the next frame's deadline is one frame after this one's. */
uint32_t cogmaster__bus_run_frame(cogmaster *m);

/* The VDP as load and reset leave it: registers zero except R2 $0E and R10
   $FF, status, latch and Pause clear; power also zeroes VRAM and CRAM. */
void cogmaster__vdp_reset(cogmaster *m, int power);

/* The tick of the next VDP event on line after tick t, or the line's
   length when none is left. */
uint32_t cogmaster__vdp_next_event(const cogmaster *m, uint32_t line, uint32_t t);

/* Runs the event at tick t of line; the bus calls it when the beam reaches
   a tick cogmaster__vdp_next_event named. */
void cogmaster__vdp_event(cogmaster *m, uint32_t line, uint32_t t);

/* A CPU access to a decoded VDP port at the beam position. */
uint8_t cogmaster__vdp_read(cogmaster *m, uint32_t port);
void cogmaster__vdp_write(cogmaster *m, uint32_t port, uint8_t v);
void cogmaster__vdp_latch_h(cogmaster *m);
void cogmaster__vdp_render_to(cogmaster *m, uint32_t end);
void cogmaster__vdp_sprite_scan(cogmaster *m, uint32_t line);
void cogmaster__vdp_render_until(cogmaster *m, uint32_t line, uint32_t x);
#define COGMASTER__VDP_PRIORITY 32u
uint32_t cogmaster__vdp_background(const cogmaster *m, uint32_t x, uint32_t y);
uint32_t cogmaster__vdp_pattern_address(const cogmaster *m, uint32_t address);
uint32_t cogmaster__vdp_sat_address(const cogmaster *m, uint32_t offset);
uint32_t cogmaster__vdp_sprite_address(const cogmaster *m, uint32_t tile, uint32_t row);
void cogmaster__vdp_finish_field(cogmaster *m);
uint16_t cogmaster__vdp_backdrop(const cogmaster *m);

/* cogmaster_probe without the size tag: fills all of out (size 0) and
   returns OK, ERR_ARGUMENT for a NULL image, ERR_UNSUPPORTED_SIZE outside
   the load bounds (image_size filled, the rest zero), or
   ERR_UNSUPPORTED_BOARD with board 0 and every other field filled. Reads
   only image[0, n). */
cogmaster_status cogmaster__cart_probe(const void *image, size_t n, cogmaster_cart_info *out);

/* Store sites to each board's registers found by the board scan: count[]
   indexed by the family. Sega is the mapper registers; the others are the
   boards the scan can tell from a store alone. */
enum {
    COGMASTER__SCAN_SEGA,
    COGMASTER__SCAN_KOREAN,
    COGMASTER__SCAN_ZEMINA,
    COGMASTER__SCAN_4PAK,
    COGMASTER__SCAN_JANGGUN,
    COGMASTER__SCAN_FAMILIES
};
struct cogmaster__scan {
    uint32_t count[COGMASTER__SCAN_FAMILIES];
};
/* Counts LD (nn),A stores at instruction boundaries over the first 32 KiB of
   b[0, body), by family. Reads only that span; no allocation. */
void cogmaster__cart_scan(const uint8_t *b, size_t body, struct cogmaster__scan *out);

/* Copies a probed image into a new allocator buffer described by out, which
   the caller installs with cogmaster__cart_install. ERR_NO_MEMORY changes
   nothing. */
cogmaster_status cogmaster__cart_copy(cogmaster *m, const uint8_t *image, size_t n,
                                      const cogmaster_cart_info *p, struct cogmaster__cart *out);

/* Frees the instance's image, if any, and installs c (which may be empty). */
void cogmaster__cart_install(cogmaster *m, const struct cogmaster__cart *c);
uint32_t cogmaster__battery_length(const cogmaster *m);
void cogmaster__battery_write(cogmaster *m, uint32_t offset, uint8_t value);

/* The 93C46. reset gives the powered-up chip: writes disabled, idle, chip select
   low. write takes the byte a game wrote to the cartridge window and changes
   image, 128 bytes, only when it completes a WRITE, ERASE, WRAL or ERAL that
   leaves a byte different; it returns 1 then and 0 otherwise. read is the byte
   a game reads back from the window. */
void cogmaster__eeprom_reset(struct cogmaster__eeprom *e);
uint32_t cogmaster__eeprom_write(struct cogmaster__eeprom *e, uint8_t *image, uint8_t v);
uint8_t cogmaster__eeprom_read(const struct cogmaster__eeprom *e);

/* The mapper. read and write take an address below $C000; write also takes
   the Sega registers at $FFFC-$FFFF, which the bus stores in work RAM too.
   reset sets the registers the board powers up with. */
uint8_t cogmaster__mapper_read(const cogmaster *m, uint16_t addr);
void cogmaster__mapper_write(cogmaster *m, uint16_t addr, uint8_t v);
void cogmaster__mapper_reset(cogmaster *m);
/* The boards outside the mapper's three switch arms: same contract, same
   address range. reset runs after the registers are cleared. */
uint8_t cogmaster__mapext_read(const cogmaster *m, uint16_t addr);
void cogmaster__mapext_write(cogmaster *m, uint16_t addr, uint8_t v);
void cogmaster__mapext_reset(cogmaster *m);

/* The I/O ports outside the VDP's: the bus calls these for every other
   port address (the full 16-bit address). open_bus returns what an undriven
   read sees and counts it. reset sets the power-on port values. */
uint8_t cogmaster__io_in(cogmaster *m, uint16_t port);
void cogmaster__io_out(cogmaster *m, uint16_t port, uint8_t v);
uint8_t cogmaster__io_open_bus(cogmaster *m);
void cogmaster__io_reset(cogmaster *m);
/* Every $3F write: the latch of the H counter on a rising TH lives here. */
void cogmaster__io_th_changed(cogmaster *m, uint8_t old_ctrl, uint8_t new_ctrl, uint64_t tick);
/* The six pins a port's device drives (b0-b3 D0-D3, b4 TL, b5 TR), 0 = low.
   Port is 0 or 1. Pure. */
uint8_t cogmaster__ports_pins(const cogmaster *m, uint32_t port, uint64_t tick);

/* The fast-boot hand-off: the CPU and RAM as the BIOS leaves them. power
   also clears work RAM first. */
void cogmaster__boot_handoff(cogmaster *m, int power);

/* The synthesiser. reset runs last in load and in both resets, after the bus,
   hand-off, I/O and VDP resets: power zeroes everything, a soft reset keeps
   the phase, ring, integrator, DC estimate, levels and the undrained PCM.
   frame_end runs when a frame's ticks are final and replaces the pending PCM
   with the frame's samples, counting any frames the host never took in the
   dropped counter. */
void cogmaster__synth_reset(cogmaster *m, int power);
void cogmaster__synth_frame_end(cogmaster *m);

/* Appends a level change to the host's trace log, if it asked for one, a full
   log counts the append in its dropped total instead. Records of one source
   are in tick order; across sources they are in the order the chips were
   brought up to date, so a consumer sorts by tick, source and emission index.
   Load and both resets empty the log, because ticks restart there. */
void cogmaster__synth_trace(cogmaster *m, uint64_t tick, uint32_t source, uint32_t level);

/* A sound source's level changed at tick (counted from the last load or reset)
   in the frame being run. Traces the raw level, gains it, and feeds the change
   to the ring as a band-limited step. Sources: 0 to 3 the PSG channels (bit 4
   output, bits 3 to 0 attenuation), 4 the Game Gear stereo mask; 5 and 6 are
   the FM unit's. Call only for frames already begun: state is saved between
   frames. */
void cogmaster__synth_level(cogmaster *m, uint64_t tick, uint32_t source, uint32_t level);

/* The PSG. reset gives the power-on constants with the step origin at tick 0;
   catch_up runs every step at a multiple of 48 ticks up to tick, inclusive;
   write catches up to tick, applies one byte and stamps the level changes at
   tick; level is what channel ch presents (bit 4 output, bits 3 to 0
   attenuation); snapshot hands every active source's level to the synthesiser
   at the current tick. */
void cogmaster__psg_reset(cogmaster *m);
void cogmaster__psg_catch_up(cogmaster *m, uint64_t tick);
void cogmaster__psg_write(cogmaster *m, uint64_t tick, uint8_t v);
uint32_t cogmaster__psg_level(const cogmaster *m, uint32_t ch);
void cogmaster__psg_snapshot(cogmaster *m);

/* The YM2413 unit, attached when fm_unit is set. A sample completes every 216 ticks
   (72 Z80 clocks) from tick 216 at a load or a reset, counted like the PSG's steps;
   catch_up runs every sample at or before tick, in order, and hands the sum, as 16 bits,
   to the synthesiser as source 5 when it changes; reset gives the power-on chip, the next
   sample at tick 216 and $F2 at 0. */
void cogmaster__fm_catch_up(cogmaster *m, uint64_t tick);
void cogmaster__fm_reset(cogmaster *m);

/* The largest gained level a source may hold in this profile, for the state
   bounds. */
uint32_t cogmaster__synth_gained_max(const cogmaster *m, uint32_t source);

/* One kernel table entry, Q15; every row sums to 32,768. For tests. */
int32_t cogmaster__synth_kernel(uint32_t row, uint32_t tap);

/* The test card's RGB444 colour at field dot (x, y) for a resolved machine. */
uint16_t cogmaster__testcard_pixel(uint32_t x, uint32_t y, uint32_t machine);

#endif /* COGMASTER_INTERNAL_H */
