/* Declarations shared by the core's source files. Not installed. */
#ifndef COGMASTER_INTERNAL_H
#define COGMASTER_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "cogmaster.h"
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
   runs no CPU or VDP, so all of them stay zero until those exist. */
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
};

/* The VDP's timing half: registers, ports, status and the stores the CPU
   writes. No renderer reads them yet. */
#define COGMASTER__VDP_REGS 11u
#define COGMASTER__VRAM_SIZE 16384u
#define COGMASTER__CRAM_SIZE 32u

struct cogmaster__vdp {
    uint16_t addr;                    /* the 14-bit VRAM or CRAM address */
    uint8_t reg[COGMASTER__VDP_REGS]; /* R0 to R10 */
    uint8_t status;                   /* the flags: bit 7 frame interrupt pending */
    uint8_t latch;                    /* 1 after the first control byte */
    uint8_t first;                    /* the first control byte */
    uint8_t code;                     /* the control code, 0 to 3 */
    uint8_t buffer;                   /* the data port's read-ahead byte */
    uint8_t pause;                    /* 1 while a Pause press waits for its NMI tick */
    uint8_t cram[COGMASTER__CRAM_SIZE];
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
};

/* Boards, as the probe guesses them. 0 is no supported board. */
enum {
    COGMASTER__BOARD_NONE = 0,
    COGMASTER__BOARD_ROM_ONLY = COGMASTER_BOARD_ROM_ONLY,
    COGMASTER__BOARD_SEGA = COGMASTER_BOARD_SEGA,
    COGMASTER__BOARD_CODEMASTERS = COGMASTER_BOARD_CODEMASTERS
};

/* The mapper's registers. The board is the cartridge's; these are what the
   CPU writes. */
struct cogmaster__mapper {
    uint8_t ctrl;    /* Sega: $FFFC. Codemasters: $80 while RAM is at $A000. */
    uint8_t bank[3]; /* the bank in each 16 KiB slot, each below the page count */
};

/* The I/O chip's ports as the CPU last wrote them (SMS-SYSTEM section 4). */
struct cogmaster__io {
    uint8_t mem_ctrl; /* $3E: active-low enables, b6 cartridge, b4 work RAM, b2 I/O chip */
    uint8_t io_ctrl;  /* $3F: b3-0 TR/TH directions (1 = input), b7-4 their output levels */
    uint8_t gg[6];    /* Game Gear $01-$06 */
};

/* The Master System's work RAM, mirrored over $C000-$FFFF. */
#define COGMASTER__RAM_SIZE 8192u

struct cogmaster {
    cogmaster_allocator allocator; /* the caller's, copied at create */
    uint64_t frame;                /* frames run since load */
    uint64_t ticks;                /* ticks run since load */
    uint32_t machine;              /* resolved machine: SMS2 or GG; valid while loaded */
    uint32_t region;               /* resolved region: EXPORT or JAPAN; valid while loaded */
    uint32_t tv;                   /* resolved TV: NTSC or PAL; valid while loaded */
    uint32_t loaded;               /* 1 after a successful load */
    struct cogmaster__counters counters;
    uint32_t fault_present;        /* 1 once a fault is recorded */
    struct cogmaster__fault fault; /* valid while fault_present */
    struct cogmaster__z80 z80;
    struct cogmaster__bus bus;
    struct cogmaster__vdp vdp;
    struct cogmaster__cart cart;
    struct cogmaster__mapper mapper;
    struct cogmaster__io io;
    uint8_t ram[COGMASTER__RAM_SIZE];
    uint16_t field[COGMASTER__FIELD_MAX_H][COGMASTER__FIELD_W]; /* RGB444 per dot */
};

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
_Static_assert(sizeof(cogmaster_config) == 4 * COGMASTER__P, "config size");

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
/* 3P + 12 bytes rounded up to a multiple of P: 40 on 64-bit, 24 on 32-bit. */
_Static_assert(sizeof(cogmaster_media) == (4 * COGMASTER__P + 11) / COGMASTER__P * COGMASTER__P,
               "media size");

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
_Static_assert(sizeof(cogmaster_info) == 56, "info size");

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
_Static_assert(sizeof(cogmaster_run_result) == 12, "run_result size");

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
_Static_assert(sizeof(cogmaster_cart_info) == 52, "cart_info size");

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
_Static_assert(COGMASTER__FIRST_SIZE_CONFIG == sizeof(cogmaster_config), "config first size");
_Static_assert(COGMASTER__FIRST_SIZE_MEDIA == 4 * COGMASTER__P, "media first size");
_Static_assert(COGMASTER__FIRST_SIZE_FRAME == sizeof(cogmaster_frame), "frame first size");
_Static_assert(COGMASTER__FIRST_SIZE_INFO == offsetof(cogmaster_info, region), "info first size");
_Static_assert(COGMASTER__FIRST_SIZE_VERSIONS == sizeof(cogmaster_versions), "versions first size");
_Static_assert(COGMASTER__FIRST_SIZE_RUN_RESULT == sizeof(cogmaster_run_result),
               "run_result first size");
_Static_assert(COGMASTER__FIRST_SIZE_FAULT == sizeof(cogmaster_fault), "fault first size");
_Static_assert(COGMASTER__FIRST_SIZE_CART_INFO == sizeof(cogmaster_cart_info),
               "cart_info first size");

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

/* cogmaster_probe without the size tag: fills all of out (size 0) and
   returns OK, ERR_ARGUMENT for a NULL image, ERR_UNSUPPORTED_SIZE outside
   the load bounds (image_size filled, the rest zero), or
   ERR_UNSUPPORTED_BOARD with board 0 and every other field filled. Reads
   only image[0, n). */
cogmaster_status cogmaster__cart_probe(const void *image, size_t n, cogmaster_cart_info *out);

/* Copies a probed image into a new allocator buffer described by out, which
   the caller installs with cogmaster__cart_install. ERR_NO_MEMORY changes
   nothing. */
cogmaster_status cogmaster__cart_copy(cogmaster *m, const uint8_t *image, size_t n,
                                      const cogmaster_cart_info *p, struct cogmaster__cart *out);

/* Frees the instance's image, if any, and installs c (which may be empty). */
void cogmaster__cart_install(cogmaster *m, const struct cogmaster__cart *c);

/* The mapper. read and write take an address below $C000; write also takes
   the Sega registers at $FFFC-$FFFF, which the bus stores in work RAM too.
   reset sets the registers the board powers up with. */
uint8_t cogmaster__mapper_read(const cogmaster *m, uint16_t addr);
void cogmaster__mapper_write(cogmaster *m, uint16_t addr, uint8_t v);
void cogmaster__mapper_reset(cogmaster *m);

/* The I/O ports outside the VDP's: the bus calls these for every other
   port address (the full 16-bit address). open_bus returns what an undriven
   read sees and counts it. reset sets the power-on port values. */
uint8_t cogmaster__io_in(cogmaster *m, uint16_t port);
void cogmaster__io_out(cogmaster *m, uint16_t port, uint8_t v);
uint8_t cogmaster__io_open_bus(cogmaster *m);
void cogmaster__io_reset(cogmaster *m);

/* The fast-boot hand-off: the CPU and RAM as the BIOS leaves them. power
   also clears work RAM first. */
void cogmaster__boot_handoff(cogmaster *m, int power);

/* The test card's RGB444 colour at field dot (x, y) for a resolved machine. */
uint16_t cogmaster__testcard_pixel(uint32_t x, uint32_t y, uint32_t machine);

#endif /* COGMASTER_INTERNAL_H */
