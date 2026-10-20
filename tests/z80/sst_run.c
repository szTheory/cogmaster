/* The SingleStepTests z80 vector harness. Runs each sampled test through the
   own Z80 and compares every final state field, the final RAM and the
   per-T-state bus stream rebuilt from the logged bus cycles against the
   stream rebuilt from the sample's.

     cogmaster-test-sst --sample FILE [--group base|cb|dd|ddcb|ed|fd|fdcb]
                        [--only NAMES] [--expect-count N]

   NAMES is a comma-separated list of upstream file names ("00,02"). Prints
   a line beginning "mismatch" naming the file, the test index and the first
   differing field or T-state for each failing test, then "executed N".
   Exit status 1 on any mismatch, on an executed count other than
   --expect-count, or on a sample that cannot be read; 2 on misuse. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "sst_bus.h"
#include "sst_stream.h"

/* At most this many steps make one test: prefix bytes are steps of their
   own, and the vectors hold no prefix runs. */
#define MAX_STEPS 4u
/* Mismatch lines printed before the rest are only counted. */
#define MAX_PRINTED 50u

static uint16_t pair(const struct cogmaster__z80 *z, unsigned hi)
{
    return (uint16_t)(z->reg[hi] << 8 | z->reg[hi + 1]);
}

static void set_pair(struct cogmaster__z80 *z, unsigned hi, uint16_t v)
{
    z->reg[hi] = (uint8_t)(v >> 8);
    z->reg[hi + 1] = (uint8_t)v;
}

/* Upstream's af_ is A' in the high byte and F' in the low byte. */
enum {
    ALT_A = COGMASTER__Z80_ALT + COGMASTER__Z80_A,
    ALT_F = COGMASTER__Z80_ALT + COGMASTER__Z80_F
};

static uint16_t get_field(const struct cogmaster__z80 *z, unsigned k)
{
    static const uint8_t byte_reg[8] = {COGMASTER__Z80_A, COGMASTER__Z80_F, COGMASTER__Z80_B,
                                        COGMASTER__Z80_C, COGMASTER__Z80_D, COGMASTER__Z80_E,
                                        COGMASTER__Z80_H, COGMASTER__Z80_L};
    if (k < 8) {
        return z->reg[byte_reg[k]];
    }
    switch (k) {
    case 8:
        return z->i;
    case 9:
        return z->r;
    case 10:
        return (uint16_t)(z->reg[ALT_A] << 8 | z->reg[ALT_F]);
    case 11:
        return pair(z, COGMASTER__Z80_ALT + COGMASTER__Z80_B);
    case 12:
        return pair(z, COGMASTER__Z80_ALT + COGMASTER__Z80_D);
    case 13:
        return pair(z, COGMASTER__Z80_ALT + COGMASTER__Z80_H);
    case 14:
        return pair(z, COGMASTER__Z80_IXH);
    case 15:
        return pair(z, COGMASTER__Z80_IYH);
    case 16:
        return z->pc;
    case 17:
        return z->sp;
    case 18:
        return z->wz;
    case 19:
        return z->ei;
    case 20:
        return z->iff1;
    case 21:
        return z->iff2;
    case 22:
        return z->im;
    case 23:
        return z->p;
    default:
        return z->q;
    }
}

static void set_field(struct cogmaster__z80 *z, unsigned k, uint16_t v)
{
    static const uint8_t byte_reg[8] = {COGMASTER__Z80_A, COGMASTER__Z80_F, COGMASTER__Z80_B,
                                        COGMASTER__Z80_C, COGMASTER__Z80_D, COGMASTER__Z80_E,
                                        COGMASTER__Z80_H, COGMASTER__Z80_L};
    uint8_t b = (uint8_t)v;
    if (k < 8) {
        z->reg[byte_reg[k]] = b;
        return;
    }
    switch (k) {
    case 8:
        z->i = b;
        break;
    case 9:
        z->r = b;
        break;
    case 10:
        z->reg[ALT_A] = (uint8_t)(v >> 8);
        z->reg[ALT_F] = b;
        break;
    case 11:
        set_pair(z, COGMASTER__Z80_ALT + COGMASTER__Z80_B, v);
        break;
    case 12:
        set_pair(z, COGMASTER__Z80_ALT + COGMASTER__Z80_D, v);
        break;
    case 13:
        set_pair(z, COGMASTER__Z80_ALT + COGMASTER__Z80_H, v);
        break;
    case 14:
        set_pair(z, COGMASTER__Z80_IXH, v);
        break;
    case 15:
        set_pair(z, COGMASTER__Z80_IYH, v);
        break;
    case 16:
        z->pc = v;
        break;
    case 17:
        z->sp = v;
        break;
    case 18:
        z->wz = v;
        break;
    case 19:
        z->ei = b;
        break;
    case 20:
        z->iff1 = b;
        break;
    case 21:
        z->iff2 = b;
        break;
    case 22:
        z->im = b;
        break;
    case 23:
        z->p = b;
        break;
    default:
        z->q = b;
        break;
    }
}

/* The T-state of the data within each kind's bus cycle, at which
   z80_cycle.h calls the seam. */
static const uint8_t stamp_offset[6] = {0, 2, 2, 2, 3, 3};

struct run {
    const char *file;
    long test;
    unsigned long failed;
};

static void mismatch(struct run *r, const char *field, const char *expected, const char *actual)
{
    if (r->failed < MAX_PRINTED) {
        printf("mismatch \"%s\" test %ld %s: expected %s, actual %s\n", r->file, r->test, field,
               expected, actual);
    }
    r->failed++;
}

static void tick_text(const struct sst_tick *t, char *out, size_t cap)
{
    char pins[5];
    sst_pins_text(t->pins, pins);
    if (t->data < 0) {
        snprintf(out, cap, "(%u, null, %s)", t->addr, pins);
    } else {
        snprintf(out, cap, "(%u, %d, %s)", t->addr, t->data, pins);
    }
}

/* Runs one test; returns 0 when everything matches. */
static int run_test(cogmaster *m, const struct sst_test *t, struct run *r)
{
    struct cogmaster__z80 *z = &m->z80;
    memset(z, 0, sizeof *z);
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        set_field(z, k, t->initial[k]);
    }
    sst_bus_load(t);
    m->ticks = 0;
    unsigned steps = 0;
    do {
        cogmaster__z80_step(m);
        steps++;
    } while (z->idx != COGMASTER__Z80_IDX_NONE && steps < MAX_STEPS);

    char e[64], a[64], field[32];
    if (z->idx != COGMASTER__Z80_IDX_NONE) {
        snprintf(a, sizeof a, "%u steps", steps);
        mismatch(r, "steps", "an instruction boundary", a);
        return 1;
    }
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        uint16_t got = get_field(z, k);
        if (got != t->final[k]) {
            snprintf(e, sizeof e, "%u", t->final[k]);
            snprintf(a, sizeof a, "%u", got);
            mismatch(r, sst_field_names[k], e, a);
            return 1;
        }
    }
    for (unsigned k = 0; k < t->n_ram_out; k++) {
        uint8_t got = sst_bus.mem[t->ram_out[k].addr];
        if (got != t->ram_out[k].value) {
            snprintf(field, sizeof field, "ram[%u]", t->ram_out[k].addr);
            snprintf(e, sizeof e, "%u", t->ram_out[k].value);
            snprintf(a, sizeof a, "%u", got);
            mismatch(r, field, e, a);
            return 1;
        }
    }

    /* The actual stream, rebuilt from the logged cycles by the same
       templates as the expected one. */
    struct sst_access got[SST_MAX_ACCESS];
    for (unsigned k = 0; k < sst_bus.n_log; k++) {
        const struct sst_logged *l = &sst_bus.log[k];
        uint64_t ts = l->tick / 3u;
        if (l->tick % 3u != 0 || ts < stamp_offset[l->kind] || ts - stamp_offset[l->kind] > 255u) {
            snprintf(a, sizeof a, "tick %llu", (unsigned long long)l->tick);
            mismatch(r, "stamp", "a T-state boundary", a);
            return 1;
        }
        got[k] = (struct sst_access){l->kind, (uint8_t)(ts - stamp_offset[l->kind]), l->addr,
                                     l->data, l->refresh};
    }
    uint64_t total = m->ticks / 3u;
    if (sst_bus.overflow || m->ticks % 3u != 0 || total != t->total_t) {
        snprintf(e, sizeof e, "%u T-states", t->total_t);
        snprintf(a, sizeof a, "%llu ticks, %u bus cycles", (unsigned long long)m->ticks,
                 sst_bus.n_log + sst_bus.overflow);
        mismatch(r, "length", e, a);
        return 1;
    }
    struct sst_tick want[SST_MAX_T], have[SST_MAX_T];
    if (sst_stream_build(t->access, t->n_access, t->total_t, want) != 0) {
        mismatch(r, "stream", "a sample stream that builds", "a bad sample record");
        return 1;
    }
    if (sst_stream_build(got, sst_bus.n_log, t->total_t, have) != 0) {
        mismatch(r, "stream", "bus cycles in order within the instruction", "overlapping cycles");
        return 1;
    }
    for (unsigned k = 0; k < t->total_t; k++) {
        if (want[k].addr != have[k].addr || want[k].data != have[k].data ||
            want[k].pins != have[k].pins) {
            snprintf(field, sizeof field, "T%u", k + 1);
            tick_text(&want[k], e, sizeof e);
            tick_text(&have[k], a, sizeof a);
            mismatch(r, field, e, a);
            return 1;
        }
    }
    return 0;
}

static int listed(const char *list, const char *name)
{
    size_t len = strlen(name);
    for (const char *p = list; *p;) {
        const char *comma = strchr(p, ',');
        size_t n = comma ? (size_t)(comma - p) : strlen(p);
        if (n == len && memcmp(p, name, n) == 0) {
            return 1;
        }
        p += n + (comma ? 1 : 0);
    }
    return 0;
}

static uint8_t *read_sample(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    uint8_t *buf = NULL;
    long size = -1;
    if (fseek(f, 0, SEEK_END) == 0) {
        size = ftell(f);
    }
    if (size > 0 && fseek(f, 0, SEEK_SET) == 0) {
        buf = malloc((size_t)size);
        if (buf && fread(buf, 1, (size_t)size, f) != (size_t)size) {
            free(buf);
            buf = NULL;
        }
    }
    fclose(f);
    *len = (size_t)size;
    return buf;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int usage(void)
{
    fprintf(stderr, "usage: cogmaster-test-sst --sample FILE [--group base|cb|dd|ddcb|ed|fd|fdcb]"
                    " [--only NAMES] [--expect-count N]\n");
    return 2;
}

int main(int argc, char **argv)
{
    const char *sample = NULL, *group = NULL, *only = NULL;
    long expect = -1;
    for (int k = 1; k < argc; k++) {
        if (k + 1 >= argc) {
            return usage();
        }
        if (strcmp(argv[k], "--sample") == 0) {
            sample = argv[++k];
        } else if (strcmp(argv[k], "--group") == 0) {
            group = argv[++k];
        } else if (strcmp(argv[k], "--only") == 0) {
            only = argv[++k];
        } else if (strcmp(argv[k], "--expect-count") == 0) {
            char *end;
            expect = strtol(argv[++k], &end, 10);
            if (*end != '\0' || expect < 0) {
                return usage();
            }
        } else {
            return usage();
        }
    }
    if (!sample) {
        return usage();
    }

    size_t len = 0;
    uint8_t *buf = read_sample(sample, &len);
    if (!buf || len < 20 || memcmp(buf, SST_MAGIC, 8) != 0 || get32(buf + 8) != SST_VERSION) {
        fprintf(stderr, "sst: %s is not a readable vector sample\n", sample);
        free(buf);
        return 1;
    }
    uint32_t per_file = get32(buf + 12);
    uint32_t files = get32(buf + 16);
    cogmaster *m = calloc(1, sizeof *m);
    struct sst_test *t = malloc(sizeof *t);
    if (!m || !t) {
        fprintf(stderr, "sst: out of memory\n");
        return 1;
    }

    struct run r = {NULL, 0, 0};
    unsigned long executed = 0;
    unsigned matched_only = 0;
    int bad = files > SST_FILES;
    size_t pos = 20;
    for (uint32_t f = 0; f < files && !bad; f++) {
        if (pos + 2 > len) {
            bad = 1;
            break;
        }
        unsigned index = (unsigned)(buf[pos] | buf[pos + 1] << 8);
        pos += 2;
        char name[SST_NAME_MAX];
        sst_file_name(index, name);
        int run =
            (!group || strcmp(group, sst_file_group(index)) == 0) && (!only || listed(only, name));
        if (only && listed(only, name)) {
            matched_only++;
        }
        r.file = name;
        for (r.test = 0; r.test < (long)per_file; r.test++) {
            if (sst_decode(buf, len, &pos, t) != 0) {
                bad = 1;
                break;
            }
            if (run) {
                run_test(m, t, &r);
                executed++;
            }
        }
    }
    if (bad) {
        fprintf(stderr, "sst: %s: truncated or malformed record\n", sample);
    }
    if (only) {
        unsigned names = 0;
        for (const char *p = only; *p; names++) {
            const char *comma = strchr(p, ',');
            p += comma ? (size_t)(comma - p) + 1 : strlen(p);
        }
        if (matched_only != names) {
            fprintf(stderr, "sst: --only names a file the sample does not hold\n");
            bad = 1;
        }
    }
    if (r.failed > MAX_PRINTED) {
        printf("failures past the first %u not printed\n", MAX_PRINTED);
    }
    printf("failed %lu\n", r.failed);
    printf("executed %lu\n", executed);
    if (expect >= 0 && (unsigned long)expect != executed) {
        fprintf(stderr, "sst: executed %lu tests, expected %ld\n", executed, expect);
        bad = 1;
    }
    free(t);
    free(m);
    free(buf);
    return bad || r.failed ? 1 : 0;
}
