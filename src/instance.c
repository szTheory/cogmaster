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
    struct cogmaster__cart none;
    memset(&none, 0, sizeof none);
    cogmaster__cart_install(m, &none);
    cogmaster_allocator a = m->allocator;
    a.free(a.user, m, sizeof *m);
}

/* A uint32_t media field at off, or 0 (AUTO) when the caller's size tag
   does not cover it. A tag at the first released size reads every later
   field as 0, even one that sits in that struct's tail padding. */
static uint32_t media_field(const cogmaster_media *media, size_t off)
{
    uint32_t size = size_tag(media);
    uint32_t v = 0u;
    if (size > COGMASTER__FIRST_SIZE_MEDIA && off + sizeof v <= size) {
        memcpy(&v, (const uint8_t *)media + off, sizeof v);
    }
    return v;
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
    /* No image (NULL, 0) is the built-in test card. */
    int has_image = media->image != NULL || media->image_size != 0u;
    cogmaster_cart_info probe;
    memset(&probe, 0, sizeof probe);
    if (has_image) {
        st = cogmaster__cart_probe(media->image, media->image_size, &probe);
        if (st != COGMASTER_STATUS_OK) {
            return st;
        }
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
    /* AUTO is an export console on an NTSC set. */
    uint32_t region = media_field(media, offsetof(cogmaster_media, region));
    uint32_t tv = media_field(media, offsetof(cogmaster_media, tv));
    if (region > COGMASTER_REGION_JAPAN || tv > COGMASTER_TV_PAL) {
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    region = region == COGMASTER_REGION_AUTO ? COGMASTER_REGION_EXPORT : region;
    tv = tv == COGMASTER_TV_AUTO ? COGMASTER_TV_NTSC : tv;
    /* No Game Gear and no Japanese console was made for PAL (SMS-SYSTEM C6). */
    if (tv == COGMASTER_TV_PAL &&
        (machine == COGMASTER_MACHINE_GG || region == COGMASTER_REGION_JAPAN)) {
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    /* An 8 KiB image on a Game Gear is the X-Terminator cheat device, which
       needs a cartridge in its own slot (SMS-CARTRIDGES section 1.4). */
    if (has_image && machine == COGMASTER_MACHINE_GG &&
        media->image_size - probe.body_offset == 8192u) {
        return COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD;
    }
    /* Every check is done; only the copy can still fail, and it touches
       nothing of the instance until it has succeeded. */
    struct cogmaster__cart cart;
    memset(&cart, 0, sizeof cart);
    if (has_image) {
        st = cogmaster__cart_copy(m, (const uint8_t *)media->image, media->image_size, &probe,
                                  &cart);
        if (st != COGMASTER_STATUS_OK) {
            return st;
        }
    }
    cogmaster__cart_install(m, &cart);
    m->machine = machine;
    m->region = region;
    m->tv = tv;
    m->loaded = 1u;
    memset(&m->pending_input, 0, sizeof m->pending_input);
    memset(&m->input, 0, sizeof m->input);
    m->input_read = 0u;
    memset(&m->counters, 0, sizeof m->counters);
    m->fault_present = 0u;
    memset(&m->fault, 0, sizeof m->fault);
    memset(m->field, 0, sizeof m->field);
    cogmaster__bus_origin(m);
    cogmaster__boot_handoff(m, 1);
    cogmaster__vdp_reset(m, 1);
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
    info.region = m->region;
    info.tv = m->tv;
    /* NTSC: 179,208 ticks of 10,738,635 Hz per frame (ARCHITECTURE section
       2). PAL: 214,092 ticks of 53,203,424 / 5 Hz (SMS-SYSTEM C2), reduced. */
    if (m->tv == COGMASTER_TV_PAL) {
        info.fps_num = 13300856u;
        info.fps_den = 267615u;
    } else {
        info.fps_num = 1640625u;
        info.fps_den = 27379u;
    }
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

/* Both kinds return the beam to the frame origin with frame and ticks 0,
   apply the fast-boot hand-off again and clear the first fault (ARCHITECTURE
   section 9); POWER also clears work RAM, VRAM and CRAM and zeroes the
   counters, which a power cycle resets (section 8). */
cogmaster_status cogmaster_reset(cogmaster *m, uint32_t kind)
{
    if (m == NULL || (kind != COGMASTER_RESET_SOFT && kind != COGMASTER_RESET_POWER)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    cogmaster__bus_origin(m);
    cogmaster__boot_handoff(m, kind == COGMASTER_RESET_POWER);
    cogmaster__vdp_reset(m, kind == COGMASTER_RESET_POWER);
    m->fault_present = 0u;
    memset(&m->fault, 0, sizeof m->fault);
    if (kind == COGMASTER_RESET_POWER) {
        memset(&m->counters, 0, sizeof m->counters);
    }
    return COGMASTER_STATUS_OK;
}
