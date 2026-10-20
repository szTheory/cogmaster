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

/* One NTSC frame is 262 lines of 684 ticks (ARCHITECTURE section 2). */
#define COGMASTER__TICKS_PER_FRAME_NTSC 179208u

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

struct cogmaster {
    cogmaster_allocator allocator; /* the caller's, copied at create */
    uint64_t frame;                /* frames run since load */
    uint64_t ticks;                /* ticks run since load */
    uint32_t machine;              /* resolved machine: SMS2 or GG; valid while loaded */
    uint32_t loaded;               /* 1 after a successful load */
    struct cogmaster__counters counters;
    uint32_t fault_present;        /* 1 once a fault is recorded */
    struct cogmaster__fault fault; /* valid while fault_present */
    struct cogmaster__z80 z80;
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
_Static_assert(sizeof(cogmaster_media) == 4 * COGMASTER__P, "media size");

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
_Static_assert(sizeof(cogmaster_info) == 48, "info size");

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
_Static_assert(COGMASTER__FIRST_SIZE_CONFIG == sizeof(cogmaster_config), "config first size");
_Static_assert(COGMASTER__FIRST_SIZE_MEDIA == sizeof(cogmaster_media), "media first size");
_Static_assert(COGMASTER__FIRST_SIZE_FRAME == sizeof(cogmaster_frame), "frame first size");
_Static_assert(COGMASTER__FIRST_SIZE_INFO == sizeof(cogmaster_info), "info first size");
_Static_assert(COGMASTER__FIRST_SIZE_VERSIONS == sizeof(cogmaster_versions), "versions first size");
_Static_assert(COGMASTER__FIRST_SIZE_RUN_RESULT == sizeof(cogmaster_run_result),
               "run_result first size");
_Static_assert(COGMASTER__FIRST_SIZE_FAULT == sizeof(cogmaster_fault), "fault first size");

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

/* CRC-32 as in Ethernet and PNG: reflected polynomial 0xEDB88320, initial
   value and final XOR 0xFFFFFFFF. Continue a running CRC by passing it back
   as crc; start from 0. */
uint32_t cogmaster__crc32(uint32_t crc, const uint8_t *p, size_t n);

/* The test card's RGB444 colour at field dot (x, y) for a resolved machine. */
uint16_t cogmaster__testcard_pixel(uint32_t x, uint32_t y, uint32_t machine);

#endif /* COGMASTER_INTERNAL_H */
