/* API version 1, as first released. FROZEN: once released this file never
   changes. It calls every function, assigns every field of every struct and
   checks every constant of version 1, so a rename, a removal or a changed
   value fails to build or fails here. Each later API version adds its own
   file, v2.c and so on, beside this one. */
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "cogmaster_version.h"
#include "../check.h"

static void *v1_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v1_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

int main(void)
{
    CHECK_EQ_U64(COGMASTER_API_VERSION, 1);
    CHECK(COGMASTER_ABI_VERSION >= 1);
    CHECK(COGMASTER_BEHAVIOUR_REVISION >= 1);
    CHECK(COGMASTER_STATE_FORMAT_VERSION >= 1);
    CHECK_EQ_U64(COGMASTER_STATUS_OK, 0);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_ARGUMENT, 1);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_STRUCT_SIZE, 2);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_NO_MEMORY, 3);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL, 4);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_NO_CONTENT, 5);
    CHECK_EQ_U64(COGMASTER_STATUS_ERR_PROFILE_REJECTED, 6);
    CHECK_EQ_U64(COGMASTER_MACHINE_AUTO, 0);
    CHECK_EQ_U64(COGMASTER_MACHINE_SMS2, 1);
    CHECK_EQ_U64(COGMASTER_MACHINE_GG, 2);
    CHECK_EQ_U64(COGMASTER_STOP_FRAME, 0);
    CHECK_EQ_U64(COGMASTER_FRAME_FULL_FIELD, 1);
    CHECK_EQ_U64(COGMASTER_RESET_SOFT, 0);
    CHECK_EQ_U64(COGMASTER_RESET_POWER, 1);
    CHECK_EQ_U64(sizeof(cogmaster_status), 4);

    cogmaster_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    v.library_major = 0u;
    v.library_minor = 0u;
    v.library_patch = 0u;
    v.api = 0u;
    v.abi = 0u;
    v.behaviour = 0u;
    v.state = 0u;
    CHECK_EQ_U64(cogmaster_get_versions(&v), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(v.api, 1);
    CHECK_EQ_U64(v.abi, COGMASTER_ABI_VERSION);
    CHECK_EQ_U64(v.behaviour, COGMASTER_BEHAVIOUR_REVISION);
    CHECK_EQ_U64(v.state, COGMASTER_STATE_FORMAT_VERSION);
    CHECK_EQ_U64(v.library_major, COGMASTER_VERSION_MAJOR);
    CHECK_EQ_U64(v.library_minor, COGMASTER_VERSION_MINOR);
    CHECK_EQ_U64(v.library_patch, COGMASTER_VERSION_PATCH);

    cogmaster_allocator allocator;
    memset(&allocator, 0, sizeof allocator);
    allocator.alloc = v1_alloc;
    allocator.free = v1_free;
    allocator.user = NULL;
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator = allocator;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    CHECK(m != NULL);
    if (m == NULL) {
        CHECK_DONE();
    }

    /* No image: the built-in Master System test card. */
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = NULL;
    media.image_size = 0u;
    media.machine = COGMASTER_MACHINE_SMS2;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);

    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.machine = 0u;
    info.crop_w = 0u;
    info.crop_h = 0u;
    info.field_w = 0u;
    info.field_h = 0u;
    info.max_w = 0u;
    info.max_h = 0u;
    info.fps_num = 0u;
    info.fps_den = 0u;
    info.par_num = 0u;
    info.par_den = 0u;
    CHECK_EQ_U64(cogmaster_get_info(m, &info), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(info.machine, COGMASTER_MACHINE_SMS2);
    CHECK_EQ_U64(info.crop_w, 256);
    CHECK_EQ_U64(info.crop_h, 192);
    CHECK(info.field_w >= info.crop_w && info.field_h >= info.crop_h);
    CHECK(info.max_w >= info.field_w && info.max_h >= info.field_h);
    CHECK(info.fps_num != 0u && info.fps_den != 0u);
    CHECK(info.par_num != 0u && info.par_den != 0u);

    cogmaster_run_result run;
    memset(&run, 0, sizeof run);
    run.size = (uint32_t)sizeof run;
    run.stop = 0u;
    run.overshoot_ticks = 0u;
    CHECK_EQ_U64(cogmaster_run_frame(m, &run), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(run.stop, COGMASTER_STOP_FRAME);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);

    size_t count = (size_t)info.max_w * info.max_h;
    uint16_t *pixels = (uint16_t *)malloc(count * sizeof *pixels);
    CHECK(pixels != NULL);
    cogmaster_frame io;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.pixels = pixels;
    io.pitch_bytes = info.max_w * 2u;
    io.flags = COGMASTER_FRAME_FULL_FIELD;
    io.x = 0u;
    io.y = 0u;
    io.w = 0u;
    io.h = 0u;
    io.field_h = 0u;
    io.backdrop_rgb444 = 0u;
    if (pixels != NULL) {
        CHECK_EQ_U64(cogmaster_get_frame(m, &io), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(io.w, info.field_w);
        CHECK_EQ_U64(io.h, info.field_h);
        CHECK_EQ_U64(io.field_h, info.field_h);
        CHECK(io.backdrop_rgb444 <= 0x0fffu);
    }

    cogmaster_fault fault;
    memset(&fault, 0, sizeof fault);
    fault.size = (uint32_t)sizeof fault;
    fault.present = 0u;
    fault.frame = 0u;
    fault.line = 0u;
    fault.dot = 0u;
    fault.pc = 0u;
    fault.addr = 0u;
    fault.opcode = 0u;
    fault.width = 0u;
    fault.device = 0u;
    fault.kind = 0u;
    CHECK_EQ_U64(cogmaster_get_fault(m, &fault), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(fault.present, 0);

    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_SOFT), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
    media.machine = COGMASTER_MACHINE_AUTO;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    media.machine = COGMASTER_MACHINE_GG;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);

    free(pixels);
    cogmaster_destroy(m);
    cogmaster_destroy(NULL);
    CHECK_DONE();
}
