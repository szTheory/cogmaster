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

/* A uint32_t field of an input struct at off, or 0 (the default) when the
   caller's size tag does not cover it. A tag at the first released size
   reads every later field as 0, even one that sits in that struct's tail
   padding. */
static uint32_t tagged_field(const void *s, uint32_t first, size_t off)
{
    uint32_t size = size_tag(s);
    uint32_t v = 0u;
    if (size > first && off + sizeof v <= size) {
        memcpy(&v, (const uint8_t *)s + off, sizeof v);
    }
    return v;
}

/* The BIOS pair of an input struct: both fields or neither. A size tag that does not cover the end
   of bios_size (a caller built against an older header, or one that stops after the pointer) reads
   both as absent, even over the tail padding of the older struct. */
static void tagged_bios(const cogmaster_media *media, const void **ptr, size_t *len)
{
    const uint8_t *raw = (const uint8_t *)media;
    const size_t at_ptr = offsetof(cogmaster_media, bios);
    const size_t at_len = offsetof(cogmaster_media, bios_size);
    *ptr = NULL;
    *len = 0u;
    if ((size_t)size_tag(media) >= at_len + sizeof *len) {
        memcpy(ptr, raw + at_ptr, sizeof *ptr);
        memcpy(len, raw + at_len, sizeof *len);
    }
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
    /* The trace log's size is the caller's number: a cap keeps its byte count
       inside a 32-bit size_t. Load allocates it. */
    uint32_t records =
        tagged_field(cfg, COGMASTER__FIRST_SIZE_CONFIG, offsetof(cogmaster_config, trace_records));
    if (records > COGMASTER__TRACE_MAX) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster *m = (cogmaster *)a.alloc(a.user, sizeof *m);
    if (m == NULL) {
        return COGMASTER_STATUS_ERR_NO_MEMORY;
    }
    memset(m, 0, sizeof *m);
    m->allocator = a;
    m->trace_cap = records;
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
    if (m->trace_log != NULL) {
        a.free(a.user, m->trace_log, (size_t)m->trace_cap * sizeof *m->trace_log);
    }
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
    /* A BIOS is both fields or neither, and exactly 8 KiB, so a hostile size is one compare with no
       allocation. The test card runs no CPU, so it has nothing to boot a BIOS on. */
    const void *bios;
    size_t bios_size;
    tagged_bios(media, &bios, &bios_size);
    if ((bios == NULL) != (bios_size == 0u)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    int has_bios = bios != NULL;
    if (has_bios && bios_size != COGMASTER__BIOS_SIZE) {
        return COGMASTER_STATUS_ERR_UNSUPPORTED_SIZE;
    }
    /* No image (NULL, 0) is the built-in test card. */
    int has_image = media->image != NULL || media->image_size != 0u;
    if (has_bios && !has_image) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
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
        machine = probe.machine_guess != 0u ? probe.machine_guess : COGMASTER_MACHINE_SMS2;
        break;
    case COGMASTER_MACHINE_SMS2:
        machine = COGMASTER_MACHINE_SMS2;
        break;
    case COGMASTER_MACHINE_GG:
        machine = COGMASTER_MACHINE_GG;
        break;
    case COGMASTER_MACHINE_SMS1:
        machine = COGMASTER_MACHINE_SMS1;
        break;
    default:
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    /* The one BIOS supported is the Master System's; no Game Gear BIOS is booted. */
    if (has_bios && machine == COGMASTER_MACHINE_GG) {
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    /* AUTO is an export console on an NTSC set. */
    uint32_t region =
        tagged_field(media, COGMASTER__FIRST_SIZE_MEDIA, offsetof(cogmaster_media, region));
    uint32_t tv = tagged_field(media, COGMASTER__FIRST_SIZE_MEDIA, offsetof(cogmaster_media, tv));
    if (region > COGMASTER_REGION_JAPAN || tv > COGMASTER_TV_PAL) {
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    region = region == COGMASTER_REGION_AUTO
                 ? (probe.region_guess != 0u ? probe.region_guess : COGMASTER_REGION_EXPORT)
                 : region;
    tv = tv == COGMASTER_TV_AUTO ? COGMASTER_TV_NTSC : tv;
    uint32_t video = 0u;
    /* Previous media ended at 40/24 bytes, including its tail padding. */
    if (media->size > (sizeof(void *) == 8u ? 40u : 24u))
        video =
            tagged_field(media, COGMASTER__FIRST_SIZE_MEDIA, offsetof(cogmaster_media, video_mode));
    if (video > COGMASTER_VIDEO_GG ||
        (video == COGMASTER_VIDEO_GG && machine != COGMASTER_MACHINE_GG))
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    if (video == COGMASTER_VIDEO_AUTO)
        video = machine == COGMASTER_MACHINE_GG
                    ? (probe.video_mode != 0u ? probe.video_mode : COGMASTER_VIDEO_GG)
                    : COGMASTER_VIDEO_SMS;
    /* The FM unit is the Japanese console's YM2413; a Game Gear has none, and
       the export console takes one only on request (SMS-AUDIO section 9).
       A size tag at or below the previous struct reads fm as AUTO. */
    uint32_t fm = tagged_field(media, COGMASTER__FIRST_SIZE_MEDIA, offsetof(cogmaster_media, fm));
    if (fm > COGMASTER_FM_ON || (fm == COGMASTER_FM_ON && machine == COGMASTER_MACHINE_GG)) {
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    uint32_t fm_unit =
        fm == COGMASTER_FM_ON || (fm == COGMASTER_FM_AUTO && region == COGMASTER_REGION_JAPAN &&
                                  machine != COGMASTER_MACHINE_GG);
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
    /* Every check is done; only the allocations can still fail, and they touch
       nothing of the instance's state. The trace log is allocated by the first
       load and reused by every later one. */
    if (m->trace_cap > 0u && m->trace_log == NULL) {
        size_t bytes = (size_t)m->trace_cap * sizeof *m->trace_log;
        m->trace_log = (cogmaster_audio_record *)m->allocator.alloc(m->allocator.user, bytes);
        if (m->trace_log == NULL) {
            return COGMASTER_STATUS_ERR_NO_MEMORY;
        }
    }
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
    /* The instance owns its copy, so the caller may free the buffer at once. A load without a BIOS
       clears the last one; the copy has the constant length, never the caller's size. */
    memset(&m->bios, 0, sizeof m->bios);
    if (has_bios) {
        memcpy(m->bios.rom, bios, COGMASTER__BIOS_SIZE);
        m->bios.size = COGMASTER__BIOS_SIZE;
        m->bios.crc32 = cogmaster__crc32(0u, m->bios.rom, COGMASTER__BIOS_SIZE);
    }
    m->machine = machine;
    m->video_mode = video;
    m->region = region;
    m->tv = tv;
    m->fm_unit = fm_unit;
    m->fm_mask = fm_unit ? 3u : 0u; /* plain data: a unit alone would keep bit 0 only */
    m->loaded = 1u;
    memset(&m->pending_input, 0, sizeof m->pending_input);
    memset(&m->input, 0, sizeof m->input);
    m->input_read = 0u;
    m->table_device[0] = (uint8_t)probe.default_port1;
    m->table_device[1] = (uint8_t)probe.default_port2;
    memset(&m->counters, 0, sizeof m->counters);
    m->fault_present = 0u;
    memset(&m->fault, 0, sizeof m->fault);
    memset(m->field, 0, sizeof m->field);
    cogmaster__bus_origin(m);
    if (m->bios.size != 0u) {
        cogmaster__boot_power_on(m, 1);
    } else {
        cogmaster__boot_handoff(m, 1);
    }
    cogmaster__vdp_reset(m, 1);
    cogmaster__synth_reset(m, 1);
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
    info.video_mode = m->video_mode;
    info.field_w = COGMASTER__FIELD_W;
    info.field_h = cogmaster__native_gg(m) ? m->lcd.height : m->completed.height;
    info.max_w = COGMASTER__FIELD_W;
    info.max_h = COGMASTER__FIELD_MAX_H;
    info.region = m->region;
    info.tv = m->tv;
    info.audio_channels = m->machine == COGMASTER_MACHINE_GG ? 2u : 1u;
    info.fm = m->fm_unit ? COGMASTER_FM_ON : COGMASTER_FM_OFF;
    /* A table default the profile cannot take reads as a pad. */
    info.default_port1 = cogmaster__device_ok(m, m->table_device[0]) ? m->table_device[0] : 0u;
    info.default_port2 = cogmaster__device_ok(m, m->table_device[1]) ? m->table_device[1] : 0u;
    /* NTSC: a frame is 262 lines of 684 ticks, 179,208 ticks. A tick is a
       third of a Z80 cycle and the Z80 runs at 315/88 MHz, so the tick clock
       is 315/88 MHz x 3 (about 10,738,636 Hz) and the frame rate reduces to
       1,640,625 / 27,379 Hz (ARCHITECTURE section 2, SMS-AUDIO section 1).
       PAL: 214,092 ticks of 53,203,424 / 5 Hz (SMS-SYSTEM C2), reduced. */
    if (m->tv == COGMASTER_TV_PAL) {
        info.fps_num = 13300856u;
        info.fps_den = 267615u;
    } else {
        info.fps_num = 1640625u;
        info.fps_den = 27379u;
    }
    cogmaster__rect crop = m->completed.crop;
    info.crop_w = crop.w;
    info.crop_h = crop.h;
    info.par_num = m->completed.par_num;
    info.par_den = m->completed.par_den;
    cogmaster__write_out(out, &info, (uint32_t)sizeof info);
    return COGMASTER_STATUS_OK;
}

/* Both kinds return the beam to the frame origin with frame and ticks 0,
   apply the fast-boot hand-off again (or power on at address 0 when a BIOS is attached) and clear
   the first fault (ARCHITECTURE section 9); POWER also clears work RAM, VRAM and CRAM and zeroes
   the counters, which a power cycle resets (section 8). The synthesiser resets last: POWER restarts
   the audio cadence, SOFT keeps its phase and any samples the host has not taken. */
cogmaster_status cogmaster_reset(cogmaster *m, uint32_t kind)
{
    if (m == NULL || (kind != COGMASTER_RESET_SOFT && kind != COGMASTER_RESET_POWER)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    cogmaster__bus_origin(m);
    if (m->bios.size != 0u) {
        cogmaster__boot_power_on(m, kind == COGMASTER_RESET_POWER);
    } else {
        cogmaster__boot_handoff(m, kind == COGMASTER_RESET_POWER);
    }
    cogmaster__vdp_reset(m, kind == COGMASTER_RESET_POWER);
    m->fault_present = 0u;
    memset(&m->fault, 0, sizeof m->fault);
    if (kind == COGMASTER_RESET_POWER) {
        memset(&m->counters, 0, sizeof m->counters);
    }
    cogmaster__synth_reset(m, kind == COGMASTER_RESET_POWER);
    return COGMASTER_STATUS_OK;
}
