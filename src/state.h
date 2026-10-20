/* The internal state visitor. One field list drives five passes over the
   same bytes: SIZE counts them, WRITE stores the fields, VALIDATE reads the
   bytes and range-checks every field without touching the instance, APPLY
   reads them into the fields, and RECORD emits each field's name, offset,
   width and bounds for the tests' rejection sweep. VALIDATE and APPLY walk
   the same list over the same buffer, so once VALIDATE passes, APPLY cannot
   fail (ARCHITECTURE section 7).

   Every multi-byte value is written one byte at a time, least significant
   first, so the bytes are the same on every platform. Not installed; there
   is no public state API yet. */
#ifndef COGMASTER_STATE_H
#define COGMASTER_STATE_H

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

/* One field as RECORD reports it: offset from the start of the state, width
   in bytes, and the inclusive range VALIDATE accepts. */
struct cogmaster__state_field {
    const char *name;
    uint32_t offset;
    uint32_t width;
    uint64_t min, max;
};

/* The size of the instance's state in bytes, or 0 before a successful load.
   Constant from load to unload. */
size_t cogmaster__state_size(const cogmaster *m);

/* Writes the state into buf. ERR_ARGUMENT for NULL, ERR_NO_CONTENT before a
   load, ERR_BUFFER_TOO_SMALL when capacity is below the state size. */
cogmaster_status cogmaster__state_save(const cogmaster *m, void *buf, size_t capacity);

/* Restores a state of exactly cogmaster__state_size bytes. ERR_NO_CONTENT
   before a load; ERR_ARGUMENT for NULL or for any state that is the wrong
   size, fails its checksum or holds a field out of range. A refused state
   changes nothing. */
cogmaster_status cogmaster__state_load(cogmaster *m, const void *buf, size_t size);

/* Writes up to cap field records to out and returns how many fields the
   state has (which may exceed cap); 0 before a load. */
size_t cogmaster__state_record(const cogmaster *m, struct cogmaster__state_field *out, size_t cap);

/* Recomputes the body checksum of a state buffer of size bytes, so a test
   can change one field and still reach the range checks. */
void cogmaster__state_seal(void *buf, size_t size);

enum {
    COGMASTER__SIO_SIZE,
    COGMASTER__SIO_WRITE,
    COGMASTER__SIO_VALIDATE,
    COGMASTER__SIO_APPLY,
    COGMASTER__SIO_RECORD
};

struct cogmaster__sio {
    int mode;
    uint8_t *w;                         /* WRITE cursor */
    const uint8_t *r;                   /* VALIDATE and APPLY cursor */
    size_t n;                           /* SIZE and RECORD: bytes counted so far */
    int bad;                            /* VALIDATE: a range or cross-field rule failed */
    struct cogmaster__state_field *rec; /* RECORD: output, or NULL to only count */
    size_t rec_cap;                     /* RECORD: capacity of rec */
    size_t rec_n;                       /* RECORD: fields seen, which may exceed rec_cap */
};

/* Moves one field of width bytes, little-endian, bounded to [min, max].
   Returns the field's value: v for SIZE, WRITE and RECORD, the decoded value
   for VALIDATE and APPLY. */
static inline uint64_t cogmaster__sio_field(struct cogmaster__sio *io, const char *name, uint64_t v,
                                            uint32_t width, uint64_t min, uint64_t max)
{
    switch (io->mode) {
    case COGMASTER__SIO_RECORD:
        if (io->rec != NULL && io->rec_n < io->rec_cap) {
            struct cogmaster__state_field *f = &io->rec[io->rec_n];
            f->name = name;
            f->offset = (uint32_t)io->n;
            f->width = width;
            f->min = min;
            f->max = max;
        }
        io->rec_n++;
        io->n += width;
        return v;
    case COGMASTER__SIO_SIZE:
        io->n += width;
        return v;
    case COGMASTER__SIO_WRITE:
        for (uint32_t i = 0; i < width; i++) {
            io->w[i] = (uint8_t)(v >> (8u * i));
        }
        io->w += width;
        return v;
    default:
        v = 0u;
        for (uint32_t i = 0; i < width; i++) {
            v |= (uint64_t)io->r[i] << (8u * i);
        }
        io->r += width;
        if (io->mode == COGMASTER__SIO_VALIDATE && (v < min || v > max)) {
            io->bad = 1;
        }
        return v;
    }
}

/* A cross-field rule: in VALIDATE, a false ok marks the state invalid. */
static inline void cogmaster__sio_check(struct cogmaster__sio *io, int ok)
{
    if (io->mode == COGMASTER__SIO_VALIDATE && !ok) {
        io->bad = 1;
    }
}

static inline uint8_t cogmaster__sio_u8(struct cogmaster__sio *io, const char *name, uint8_t *v,
                                        uint8_t min, uint8_t max)
{
    uint8_t x = (uint8_t)cogmaster__sio_field(io, name, *v, 1u, min, max);
    if (io->mode == COGMASTER__SIO_APPLY) {
        *v = x;
    }
    return x;
}

static inline uint16_t cogmaster__sio_u16(struct cogmaster__sio *io, const char *name, uint16_t *v,
                                          uint16_t min, uint16_t max)
{
    uint16_t x = (uint16_t)cogmaster__sio_field(io, name, *v, 2u, min, max);
    if (io->mode == COGMASTER__SIO_APPLY) {
        *v = x;
    }
    return x;
}

static inline uint32_t cogmaster__sio_u32(struct cogmaster__sio *io, const char *name, uint32_t *v,
                                          uint32_t min, uint32_t max)
{
    uint32_t x = (uint32_t)cogmaster__sio_field(io, name, *v, 4u, min, max);
    if (io->mode == COGMASTER__SIO_APPLY) {
        *v = x;
    }
    return x;
}

static inline uint64_t cogmaster__sio_u64(struct cogmaster__sio *io, const char *name, uint64_t *v,
                                          uint64_t min, uint64_t max)
{
    uint64_t x = cogmaster__sio_field(io, name, *v, 8u, min, max);
    if (io->mode == COGMASTER__SIO_APPLY) {
        *v = x;
    }
    return x;
}

#endif /* COGMASTER_STATE_H */
