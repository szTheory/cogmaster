/* Instance lifetime, load, the version and info queries, and the size-tag
   helpers. */
#include <string.h>

#include "internal.h"

/* The largest input size tag accepted, far above any struct's planned growth,
   so a garbage tag is refused without reading past the caller's object. */
#define COGMASTER__SIZE_TAG_MAX 4096u

static uint32_t size_tag(const void *s)
{
    uint32_t size;
    memcpy(&size, s, sizeof size); /* the tag is untrusted: read it alone first */
    return size;
}

cogmaster_status cogmaster__check_size(const void *s, uint32_t first, uint32_t ours)
{
    const uint8_t *bytes = (const uint8_t *)s;
    uint32_t size = size_tag(s);
    if (size < first || size > COGMASTER__SIZE_TAG_MAX) {
        return COGMASTER_STATUS_ERR_STRUCT_SIZE;
    }
    /* A newer caller's struct is accepted only if the fields this build does
       not know are all zero. */
    for (uint32_t i = ours; i < size; i++) {
        if (bytes[i] != 0u) {
            return COGMASTER_STATUS_ERR_STRUCT_SIZE;
        }
    }
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster__check_out_size(const void *s, uint32_t first)
{
    return size_tag(s) < first ? COGMASTER_STATUS_ERR_STRUCT_SIZE : COGMASTER_STATUS_OK;
}

void cogmaster__write_out(void *out, const void *full, uint32_t ours)
{
    uint32_t size = size_tag(out);
    memcpy(out, full, size < ours ? size : ours);
}

cogmaster_status cogmaster_get_versions(cogmaster_versions *out)
{
    if (out == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(out, COGMASTER__FIRST_SIZE_VERSIONS);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    cogmaster_versions v;
    memset(&v, 0, sizeof v);
    v.size = out->size;
    v.library_major = COGMASTER_VERSION_MAJOR;
    v.library_minor = COGMASTER_VERSION_MINOR;
    v.library_patch = COGMASTER_VERSION_PATCH;
    v.api = COGMASTER_API_VERSION;
    v.abi = COGMASTER_ABI_VERSION;
    v.behaviour = COGMASTER_BEHAVIOUR_REVISION;
    v.state = COGMASTER_STATE_FORMAT_VERSION;
    cogmaster__write_out(out, &v, (uint32_t)sizeof v);
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster_create(const cogmaster_config *cfg, cogmaster **out)
{
    if (cfg == NULL || out == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st =
        cogmaster__check_size(cfg, COGMASTER__FIRST_SIZE_CONFIG, (uint32_t)sizeof *cfg);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    /* Both hooks are required: the core has no allocator of its own. */
    cogmaster_allocator a = cfg->allocator;
    if (a.alloc == NULL || a.free == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster *m = (cogmaster *)a.alloc(a.user, sizeof *m);
    if (m == NULL) {
        return COGMASTER_STATUS_ERR_NO_MEMORY;
    }
    memset(m, 0, sizeof *m);
    m->allocator = a;
    *out = m;
    return COGMASTER_STATUS_OK;
}

void cogmaster_destroy(cogmaster *m)
{
    if (m == NULL) {
        return;
    }
    cogmaster_allocator a = m->allocator;
    a.free(a.user, m, sizeof *m);
}

cogmaster_status cogmaster_load(cogmaster *m, const cogmaster_media *media)
{
    if (m == NULL || media == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st =
        cogmaster__check_size(media, COGMASTER__FIRST_SIZE_MEDIA, (uint32_t)sizeof *media);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    /* This release loads only the built-in test card. */
    if (media->image != NULL || media->image_size != 0u) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    uint32_t machine;
    switch (media->machine) {
    case COGMASTER_MACHINE_AUTO:
    case COGMASTER_MACHINE_SMS2:
        machine = COGMASTER_MACHINE_SMS2;
        break;
    case COGMASTER_MACHINE_GG:
        machine = COGMASTER_MACHINE_GG;
        break;
    default:
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    m->machine = machine;
    m->loaded = 1u;
    m->frame = 0u;
    m->ticks = 0u;
    memset(m->field, 0, sizeof m->field);
    return COGMASTER_STATUS_OK;
}

cogmaster_status cogmaster_get_info(const cogmaster *m, cogmaster_info *out)
{
    if (m == NULL || out == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(out, COGMASTER__FIRST_SIZE_INFO);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    cogmaster_info info;
    memset(&info, 0, sizeof info);
    info.size = out->size;
    info.machine = m->machine;
    info.field_w = COGMASTER__FIELD_W;
    info.field_h = COGMASTER__FIELD_H;
    info.max_w = COGMASTER__FIELD_W;
    info.max_h = COGMASTER__FIELD_MAX_H;
    /* 179,208 ticks of 10,738,635 Hz per frame (ARCHITECTURE section 2). */
    info.fps_num = 1640625u;
    info.fps_den = 27379u;
    cogmaster__rect crop = cogmaster__crop(m->machine);
    info.crop_w = crop.w;
    info.crop_h = crop.h;
    /* Pixel aspect, reported and never applied: 8:7 for the NTSC SMS, 6:5
       for the Game Gear (ARCHITECTURE section 5). */
    if (m->machine == COGMASTER_MACHINE_GG) {
        info.par_num = 6u;
        info.par_den = 5u;
    } else {
        info.par_num = 8u;
        info.par_den = 7u;
    }
    cogmaster__write_out(out, &info, (uint32_t)sizeof info);
    return COGMASTER_STATUS_OK;
}
