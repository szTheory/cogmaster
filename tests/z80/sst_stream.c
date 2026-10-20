/* The shared stream builder, file naming and sample codec for the
   SingleStepTests z80 vectors; see sst_stream.h. */
#include "sst_stream.h"

#include <stdio.h>
#include <string.h>

const char *const sst_field_names[SST_FIELDS] = {
    "a",   "f",  "b",  "c",  "d",  "e",  "h",  "l",    "i",    "r",  "af_", "bc_", "de_",
    "hl_", "ix", "iy", "pc", "sp", "wz", "ei", "iff1", "iff2", "im", "p",   "q"};

const uint8_t sst_field_width[SST_FIELDS] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
                                             2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1};

/* The T-states of each kind's template, indexed by kind. */
static const uint8_t kind_len[6] = {0, 4, 3, 3, 4, 4};

int sst_stream_build(const struct sst_access *a, unsigned n, unsigned total, struct sst_tick *out)
{
    if (total > SST_MAX_T) {
        return -1;
    }
    unsigned t = 0;
    uint16_t prev = 0;
    for (unsigned k = 0; k <= n; k++) {
        unsigned start = k < n ? a[k].t : total;
        if (start < t || start > total) {
            return -1;
        }
        for (; t < start; t++) {
            out[t] = (struct sst_tick){prev, -1, 0};
        }
        if (k == n) {
            break;
        }
        if (a[k].kind < SST_M1 || a[k].kind > SST_IOW || start + kind_len[a[k].kind] > total) {
            return -1;
        }
        uint16_t ad = a[k].addr;
        int16_t d = (int16_t)a[k].data;
        struct sst_tick *o = &out[t];
        switch (a[k].kind) {
        case SST_M1:
            o[0] = (struct sst_tick){ad, -1, 0};
            o[1] = (struct sst_tick){ad, -1, SST_PIN_R | SST_PIN_M};
            o[2] = (struct sst_tick){a[k].refresh, d, 0};
            o[3] = (struct sst_tick){a[k].refresh, -1, 0};
            break;
        case SST_MR:
            o[0] = (struct sst_tick){ad, -1, 0};
            o[1] = (struct sst_tick){ad, -1, SST_PIN_R | SST_PIN_M};
            o[2] = (struct sst_tick){ad, d, 0};
            break;
        case SST_MW:
            o[0] = (struct sst_tick){ad, -1, 0};
            o[1] = (struct sst_tick){ad, d, SST_PIN_W | SST_PIN_M};
            o[2] = (struct sst_tick){ad, -1, 0};
            break;
        case SST_IOR:
            o[0] = (struct sst_tick){ad, -1, 0};
            o[1] = (struct sst_tick){ad, -1, 0};
            o[2] = (struct sst_tick){ad, -1, SST_PIN_R | SST_PIN_I};
            o[3] = (struct sst_tick){ad, d, 0};
            break;
        default: /* SST_IOW */
            o[0] = (struct sst_tick){ad, -1, 0};
            o[1] = (struct sst_tick){ad, -1, 0};
            o[2] = (struct sst_tick){ad, d, SST_PIN_W | SST_PIN_I};
            o[3] = (struct sst_tick){ad, -1, 0};
            break;
        }
        t += kind_len[a[k].kind];
        prev = out[t - 1].addr;
    }
    return 0;
}

void sst_pins_text(uint8_t pins, char out[5])
{
    out[0] = (pins & SST_PIN_R) ? 'r' : '-';
    out[1] = (pins & SST_PIN_W) ? 'w' : '-';
    out[2] = (pins & SST_PIN_M) ? 'm' : '-';
    out[3] = (pins & SST_PIN_I) ? 'i' : '-';
    out[4] = '\0';
}

/* The groups in sample order, each with its name prefix and the opcodes it
   holds: base, dd and fd leave out the prefix bytes cb dd ed fd; ed holds
   40-7f and the block instructions a0-a3 a8-ab b0-b3 b8-bb. */
static const char *const group_name[7] = {"base", "cb", "dd", "ddcb", "ed", "fd", "fdcb"};
static const char *const group_prefix[7] = {"",    "cb ", "dd ",      "dd cb __ ",
                                            "ed ", "fd ", "fd cb __ "};

static int in_group(unsigned g, unsigned op)
{
    switch (g) {
    case 0:
    case 2:
    case 5:
        return op != 0xCB && op != 0xDD && op != 0xED && op != 0xFD;
    case 4:
        return (op >= 0x40 && op <= 0x7F) || (op >= 0xA0 && op <= 0xBB && (op & 0x04) == 0);
    default:
        return 1;
    }
}

/* Finds the group and opcode of file index; returns the group. */
static unsigned locate(unsigned index, unsigned *op)
{
    for (unsigned g = 0; g < 7; g++) {
        for (unsigned o = 0; o < 256; o++) {
            if (in_group(g, o)) {
                if (index == 0) {
                    *op = o;
                    return g;
                }
                index--;
            }
        }
    }
    *op = 0;
    return 7;
}

void sst_file_name(unsigned index, char out[SST_NAME_MAX])
{
    unsigned op;
    unsigned g = locate(index, &op);
    if (g == 7) {
        out[0] = '\0';
        return;
    }
    snprintf(out, SST_NAME_MAX, "%s%02x", group_prefix[g], op);
}

const char *sst_file_group(unsigned index)
{
    unsigned op;
    unsigned g = locate(index, &op);
    return g == 7 ? "" : group_name[g];
}

static size_t put16(uint8_t *o, uint16_t v)
{
    o[0] = (uint8_t)v;
    o[1] = (uint8_t)(v >> 8);
    return 2;
}

static size_t put_field(uint8_t *o, unsigned k, uint16_t v)
{
    if (sst_field_width[k] == 2) {
        return put16(o, v);
    }
    o[0] = (uint8_t)v;
    return 1;
}

static int same_addresses(const struct sst_test *t)
{
    if (t->n_ram_in != t->n_ram_out) {
        return 0;
    }
    for (unsigned k = 0; k < t->n_ram_in; k++) {
        if (t->ram_in[k].addr != t->ram_out[k].addr) {
            return 0;
        }
    }
    return 1;
}

size_t sst_encode(const struct sst_test *t, uint8_t *out)
{
    size_t n = 0;
    uint32_t mask = 0;
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        n += put_field(out + n, k, t->initial[k]);
        if (t->final[k] != t->initial[k]) {
            mask |= 1u << k;
        }
    }
    for (unsigned b = 0; b < 4; b++) {
        out[n++] = (uint8_t)(mask >> (8 * b));
    }
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        if (mask & (1u << k)) {
            n += put_field(out + n, k, t->final[k]);
        }
    }
    out[n++] = (uint8_t)t->n_ram_in;
    for (unsigned k = 0; k < t->n_ram_in; k++) {
        n += put16(out + n, t->ram_in[k].addr);
        out[n++] = t->ram_in[k].value;
    }
    int same = same_addresses(t);
    out[n++] = (uint8_t)(t->n_ram_out | (same ? 0x80u : 0u));
    for (unsigned k = 0; k < t->n_ram_out; k++) {
        if (!same) {
            n += put16(out + n, t->ram_out[k].addr);
        }
        out[n++] = t->ram_out[k].value;
    }
    out[n++] = (uint8_t)t->n_ports;
    for (unsigned k = 0; k < t->n_ports; k++) {
        n += put16(out + n, t->ports[k].addr);
        out[n++] = t->ports[k].value;
        out[n++] = t->ports[k].dir;
    }
    out[n++] = (uint8_t)t->total_t;
    out[n++] = (uint8_t)t->n_access;
    unsigned end = 0;
    for (unsigned k = 0; k < t->n_access; k++) {
        const struct sst_access *a = &t->access[k];
        int explicit_t = a->t != end;
        out[n++] = (uint8_t)(a->kind | (explicit_t ? 0x80u : 0u));
        if (explicit_t) {
            out[n++] = a->t;
        }
        n += put16(out + n, a->addr);
        out[n++] = a->data;
        if (a->kind == SST_M1) {
            n += put16(out + n, a->refresh);
        }
        end = a->t + kind_len[a->kind];
    }
    return n;
}

/* A bounds-checked reader over the sample. */
struct reader {
    const uint8_t *buf;
    size_t len, pos;
    int bad;
};

static uint8_t get8(struct reader *r)
{
    if (r->pos >= r->len) {
        r->bad = 1;
        return 0;
    }
    return r->buf[r->pos++];
}

static uint16_t get16(struct reader *r)
{
    uint16_t lo = get8(r);
    return (uint16_t)(lo | (uint16_t)(get8(r) << 8));
}

static uint16_t get_field(struct reader *r, unsigned k)
{
    return sst_field_width[k] == 2 ? get16(r) : get8(r);
}

int sst_decode(const uint8_t *buf, size_t len, size_t *pos, struct sst_test *t)
{
    struct reader r = {buf, len, *pos, 0};
    memset(t, 0, sizeof *t);
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        t->initial[k] = get_field(&r, k);
    }
    uint32_t mask = 0;
    for (unsigned b = 0; b < 4; b++) {
        mask |= (uint32_t)get8(&r) << (8 * b);
    }
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        t->final[k] = (mask & (1u << k)) ? get_field(&r, k) : t->initial[k];
    }
    t->n_ram_in = get8(&r);
    if (t->n_ram_in > SST_MAX_RAM) {
        return -1;
    }
    for (unsigned k = 0; k < t->n_ram_in; k++) {
        t->ram_in[k].addr = get16(&r);
        t->ram_in[k].value = get8(&r);
    }
    unsigned c = get8(&r);
    int same = (c & 0x80u) != 0;
    t->n_ram_out = c & 0x7Fu;
    if (t->n_ram_out > SST_MAX_RAM || (same && t->n_ram_out != t->n_ram_in)) {
        return -1;
    }
    for (unsigned k = 0; k < t->n_ram_out; k++) {
        t->ram_out[k].addr = same ? t->ram_in[k].addr : get16(&r);
        t->ram_out[k].value = get8(&r);
    }
    t->n_ports = get8(&r);
    if (t->n_ports > SST_MAX_PORTS) {
        return -1;
    }
    for (unsigned k = 0; k < t->n_ports; k++) {
        t->ports[k].addr = get16(&r);
        t->ports[k].value = get8(&r);
        t->ports[k].dir = get8(&r);
    }
    t->total_t = get8(&r);
    t->n_access = get8(&r);
    if (t->n_access > SST_MAX_ACCESS || t->total_t > SST_MAX_T) {
        return -1;
    }
    unsigned end = 0;
    for (unsigned k = 0; k < t->n_access; k++) {
        struct sst_access *a = &t->access[k];
        uint8_t kind = get8(&r);
        a->kind = kind & 0x7Fu;
        if (a->kind < SST_M1 || a->kind > SST_IOW) {
            return -1;
        }
        a->t = (kind & 0x80u) ? get8(&r) : (uint8_t)end;
        a->addr = get16(&r);
        a->data = get8(&r);
        a->refresh = a->kind == SST_M1 ? get16(&r) : 0;
        end = a->t + kind_len[a->kind];
    }
    if (r.bad) {
        return -1;
    }
    *pos = r.pos;
    return 0;
}
