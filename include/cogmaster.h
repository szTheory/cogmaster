/* cogmaster: a Sega Master System and Game Gear emulator core. Behaviour is described in docs/. */
#ifndef COGMASTER_H
#define COGMASTER_H

#include <stddef.h>
#include <stdint.h>

#include "cogmaster_version.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COGMASTER_API_VERSION 2
#define COGMASTER_ABI_VERSION 1
#define COGMASTER_BEHAVIOUR_REVISION 1
#define COGMASTER_STATE_FORMAT_VERSION 2

typedef uint32_t cogmaster_status;
#define COGMASTER_STATUS_OK 0u
#define COGMASTER_STATUS_ERR_ARGUMENT 1u
#define COGMASTER_STATUS_ERR_STRUCT_SIZE 2u
#define COGMASTER_STATUS_ERR_NO_MEMORY 3u
#define COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL 4u
#define COGMASTER_STATUS_ERR_NO_CONTENT 5u
#define COGMASTER_STATUS_ERR_PROFILE_REJECTED 6u
#define COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE 7u

#define COGMASTER_MACHINE_AUTO 0u
#define COGMASTER_MACHINE_SMS2 1u
#define COGMASTER_MACHINE_GG 2u

#define COGMASTER_REGION_AUTO 0u
#define COGMASTER_REGION_EXPORT 1u
#define COGMASTER_REGION_JAPAN 2u

#define COGMASTER_TV_AUTO 0u
#define COGMASTER_TV_NTSC 1u
#define COGMASTER_TV_PAL 2u

#define COGMASTER_STOP_FRAME 0u

#define COGMASTER_FRAME_FULL_FIELD 1u

#define COGMASTER_RESET_SOFT 0u
#define COGMASTER_RESET_POWER 1u

typedef struct cogmaster cogmaster;

typedef struct cogmaster_allocator {
    void *(*alloc)(void *user, size_t size); /* max_align_t-aligned */
    void (*free)(void *user, void *ptr, size_t size);
    void *user;
} cogmaster_allocator;

typedef struct cogmaster_config {
    uint32_t size;
    cogmaster_allocator allocator; /* alloc and free required */
} cogmaster_config;

typedef struct cogmaster_versions {
    uint32_t size;
    uint32_t library_major, library_minor, library_patch;
    uint32_t api, abi, behaviour, state;
} cogmaster_versions;

typedef struct cogmaster_media {
    uint32_t size;
    const void *image; /* NULL with image_size 0: the built-in test card */
    size_t image_size;
    uint32_t machine; /* COGMASTER_MACHINE_* */
    uint32_t region;  /* COGMASTER_REGION_* */
    uint32_t tv;      /* COGMASTER_TV_* */
} cogmaster_media;

typedef struct cogmaster_info {
    uint32_t size;
    uint32_t machine;
    uint32_t crop_w, crop_h;
    uint32_t field_w, field_h;
    uint32_t max_w, max_h;
    uint32_t fps_num, fps_den;
    uint32_t par_num, par_den;
    uint32_t region, tv;
} cogmaster_info;

typedef struct cogmaster_frame {
    uint32_t size;
    uint16_t *pixels;         /* in: RGB444 0000BBBBGGGGRRRR */
    uint32_t pitch_bytes;     /* in */
    uint32_t flags;           /* in */
    uint32_t x, y, w, h;      /* out: the rectangle copied, in field coordinates */
    uint32_t field_h;         /* out */
    uint32_t backdrop_rgb444; /* out */
} cogmaster_frame;

typedef struct cogmaster_run_result {
    uint32_t size;
    uint32_t stop; /* COGMASTER_STOP_* */
    uint32_t overshoot_ticks;
} cogmaster_run_result;

typedef struct cogmaster_fault {
    uint32_t size;
    uint32_t present; /* 1 when a fault was recorded since load or reset */
    uint64_t frame;
    uint32_t line, dot;
    uint16_t pc, addr;
    uint8_t opcode, width;
    uint8_t device, kind; /* opaque codes, see docs/embedding.md */
} cogmaster_fault;

cogmaster_status cogmaster_get_versions(cogmaster_versions *out);
cogmaster_status cogmaster_create(const cogmaster_config *cfg, cogmaster **out);
void cogmaster_destroy(cogmaster *m); /* NULL-safe */
cogmaster_status cogmaster_load(cogmaster *m, const cogmaster_media *media);
cogmaster_status cogmaster_run_frame(cogmaster *m, cogmaster_run_result *out); /* out may be NULL */
cogmaster_status cogmaster_get_frame(const cogmaster *m, cogmaster_frame *io);
cogmaster_status cogmaster_get_info(const cogmaster *m, cogmaster_info *out);
cogmaster_status cogmaster_reset(cogmaster *m, uint32_t kind); /* COGMASTER_RESET_* */
cogmaster_status cogmaster_get_fault(const cogmaster *m, cogmaster_fault *out);

#ifdef __cplusplus
}
#endif

#endif /* COGMASTER_H */
