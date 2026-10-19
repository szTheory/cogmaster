/* Declarations shared by the core's source files. Not installed. */
#ifndef COGMASTER_INTERNAL_H
#define COGMASTER_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "cogmaster.h"

/* The field buffer is 256 dots wide and up to 240 lines tall; the test card
   and the 192-line mode use the first 192 lines (ARCHITECTURE section 5). */
#define COGMASTER__FIELD_W 256u
#define COGMASTER__FIELD_MAX_H 240u
#define COGMASTER__FIELD_H 192u

/* One NTSC frame is 262 lines of 684 ticks (ARCHITECTURE section 2). */
#define COGMASTER__TICKS_PER_FRAME_NTSC 179208u

/* The largest pitch get_frame accepts, so a row offset stays inside a 32-bit
   size_t. */
#define COGMASTER__PITCH_MAX 65536u

struct cogmaster {
    cogmaster_allocator allocator; /* the caller's, copied at create */
    uint64_t frame;                /* frames run since load */
    uint64_t ticks;                /* ticks run since load */
    uint32_t machine;              /* resolved machine: SMS2 or GG; valid while loaded */
    uint32_t loaded;               /* 1 after a successful load */
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

/* Input structs: OK when the size tag at s is at least first and at most
   4096, and every byte past ours is zero; ERR_STRUCT_SIZE otherwise. */
cogmaster_status cogmaster__check_size(const void *s, uint32_t first, uint32_t ours);

/* Output structs: OK when the size tag at s is at least first, with no upper
   bound; ERR_STRUCT_SIZE otherwise. */
cogmaster_status cogmaster__check_out_size(const void *s, uint32_t first);

/* Writes min(caller's size tag, ours) bytes of the fully built struct full to
   out; bytes past ours are left as the caller set them. */
void cogmaster__write_out(void *out, const void *full, uint32_t ours);

/* The test card's RGB444 colour at field dot (x, y) for a resolved machine. */
uint16_t cogmaster__testcard_pixel(uint32_t x, uint32_t y, uint32_t machine);

#endif /* COGMASTER_INTERNAL_H */
