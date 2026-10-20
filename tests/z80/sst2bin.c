/* Converts SingleStepTests z80 v1 JSON files to the sample format of
   sst_stream.h.

     cogmaster-sst2bin --dir V1DIR --count N --out FILE [--files NAMES]

   NAMES is a comma-separated list of upstream file names ("02,dd 34");
   without it every file of the closed name list is read. The tokenizer knows
   the fixed schema only: numbers, null, short strings, arrays and objects.
   A test missing a key fails naming the key; an unknown key is skipped; a
   test whose cycles do not rebuild from the accesses read out of them fails
   naming the file and the test. No test is dropped. Exit status 0, 1 on a
   data or I/O error, 2 on misuse. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sst_stream.h"

#define MAX_CYCLES SST_MAX_T

struct parser {
    const char *p, *end;
    const char *file;
    long test;
};

static void fail(const struct parser *ps, const char *what, const char *detail)
{
    fprintf(stderr, "sst2bin: %s: test %ld: %s%s%s\n", ps->file, ps->test, what,
            detail[0] ? ": " : "", detail);
    exit(1);
}

static void skip_ws(struct parser *ps)
{
    while (ps->p < ps->end &&
           (*ps->p == ' ' || *ps->p == '\n' || *ps->p == '\r' || *ps->p == '\t')) {
        ps->p++;
    }
}

static int peek(struct parser *ps)
{
    skip_ws(ps);
    return ps->p < ps->end ? (unsigned char)*ps->p : -1;
}

static void expect(struct parser *ps, char c)
{
    if (peek(ps) != c) {
        char want[2] = {c, '\0'};
        fail(ps, "malformed JSON, expected", want);
    }
    ps->p++;
}

static unsigned long parse_uint(struct parser *ps, unsigned long max)
{
    if (peek(ps) < '0' || peek(ps) > '9') {
        fail(ps, "malformed JSON", "expected an unsigned integer");
    }
    unsigned long v = 0;
    while (ps->p < ps->end && *ps->p >= '0' && *ps->p <= '9') {
        v = v * 10u + (unsigned long)(*ps->p - '0');
        if (v > max) {
            fail(ps, "number out of range", "");
        }
        ps->p++;
    }
    if (ps->p < ps->end && (*ps->p == '.' || *ps->p == 'e' || *ps->p == 'E')) {
        fail(ps, "malformed JSON", "expected an integer");
    }
    return v;
}

static void parse_string(struct parser *ps, char *out, size_t cap)
{
    expect(ps, '"');
    size_t n = 0;
    while (ps->p < ps->end && *ps->p != '"') {
        if (*ps->p == '\\') {
            ps->p++;
        }
        if (n + 1 < cap && ps->p < ps->end) {
            out[n++] = *ps->p;
        }
        ps->p++;
    }
    if (ps->p >= ps->end) {
        fail(ps, "malformed JSON", "unterminated string");
    }
    ps->p++;
    out[n] = '\0';
}

static int parse_null(struct parser *ps)
{
    if (peek(ps) == 'n' && ps->end - ps->p >= 4 && memcmp(ps->p, "null", 4) == 0) {
        ps->p += 4;
        return 1;
    }
    return 0;
}

static void skip_value(struct parser *ps, int depth)
{
    int c = peek(ps);
    if (depth > 16) {
        fail(ps, "malformed JSON", "nested too deeply");
    }
    if (c == '"') {
        char tmp[8];
        parse_string(ps, tmp, sizeof tmp);
    } else if (c == '[' || c == '{') {
        char close = c == '[' ? ']' : '}';
        ps->p++;
        if (peek(ps) == close) {
            ps->p++;
            return;
        }
        for (;;) {
            if (close == '}') {
                char key[8];
                parse_string(ps, key, sizeof key);
                expect(ps, ':');
            }
            skip_value(ps, depth + 1);
            if (peek(ps) == ',') {
                ps->p++;
                continue;
            }
            expect(ps, close);
            return;
        }
    } else if (c >= '0' && c <= '9') {
        parse_uint(ps, 0xFFFFFFFFul);
    } else if (!parse_null(ps)) {
        if (ps->end - ps->p >= 4 && memcmp(ps->p, "true", 4) == 0) {
            ps->p += 4;
        } else if (ps->end - ps->p >= 5 && memcmp(ps->p, "false", 5) == 0) {
            ps->p += 5;
        } else {
            fail(ps, "malformed JSON", "unexpected character");
        }
    }
}

static unsigned parse_cells(struct parser *ps, struct sst_cell *cells)
{
    unsigned n = 0;
    expect(ps, '[');
    if (peek(ps) == ']') {
        ps->p++;
        return 0;
    }
    for (;;) {
        if (n == SST_MAX_RAM) {
            fail(ps, "too many ram entries", "");
        }
        expect(ps, '[');
        cells[n].addr = (uint16_t)parse_uint(ps, 0xFFFF);
        expect(ps, ',');
        cells[n].value = (uint8_t)parse_uint(ps, 0xFF);
        expect(ps, ']');
        n++;
        if (peek(ps) == ',') {
            ps->p++;
            continue;
        }
        expect(ps, ']');
        return n;
    }
}

static unsigned parse_state(struct parser *ps, uint16_t *fields, struct sst_cell *cells)
{
    uint32_t seen = 0;
    int ram_seen = 0;
    unsigned n = 0;
    expect(ps, '{');
    while (peek(ps) != '}') {
        char key[16];
        parse_string(ps, key, sizeof key);
        expect(ps, ':');
        unsigned k = 0;
        while (k < SST_FIELDS && strcmp(key, sst_field_names[k]) != 0) {
            k++;
        }
        if (k < SST_FIELDS) {
            fields[k] = (uint16_t)parse_uint(ps, sst_field_width[k] == 2 ? 0xFFFFu : 0xFFu);
            seen |= 1u << k;
        } else if (strcmp(key, "ram") == 0) {
            n = parse_cells(ps, cells);
            ram_seen = 1;
        } else {
            skip_value(ps, 0);
        }
        if (peek(ps) == ',') {
            ps->p++;
        }
    }
    ps->p++;
    for (unsigned k = 0; k < SST_FIELDS; k++) {
        if (!(seen & (1u << k))) {
            fail(ps, "missing key", sst_field_names[k]);
        }
    }
    if (!ram_seen) {
        fail(ps, "missing key", "ram");
    }
    return n;
}

static unsigned parse_cycles(struct parser *ps, struct sst_tick *cyc)
{
    unsigned n = 0;
    expect(ps, '[');
    while (peek(ps) != ']') {
        if (n == MAX_CYCLES) {
            fail(ps, "too many cycles", "");
        }
        expect(ps, '[');
        cyc[n].addr = (uint16_t)parse_uint(ps, 0xFFFF);
        expect(ps, ',');
        cyc[n].data = parse_null(ps) ? -1 : (int16_t)parse_uint(ps, 0xFF);
        expect(ps, ',');
        char pins[8];
        parse_string(ps, pins, sizeof pins);
        if (strlen(pins) != 4) {
            fail(ps, "bad pins", pins);
        }
        static const char letters[4] = {'r', 'w', 'm', 'i'};
        cyc[n].pins = 0;
        for (unsigned b = 0; b < 4; b++) {
            if (pins[b] == letters[b]) {
                cyc[n].pins = (uint8_t)(cyc[n].pins | (1u << b));
            } else if (pins[b] != '-') {
                fail(ps, "bad pins", pins);
            }
        }
        expect(ps, ']');
        n++;
        if (peek(ps) == ',') {
            ps->p++;
        }
    }
    ps->p++;
    return n;
}

static unsigned parse_ports(struct parser *ps, struct sst_port *ports)
{
    unsigned n = 0;
    if (parse_null(ps)) {
        return 0;
    }
    expect(ps, '[');
    while (peek(ps) != ']') {
        if (n == SST_MAX_PORTS) {
            fail(ps, "too many ports", "");
        }
        expect(ps, '[');
        ports[n].addr = (uint16_t)parse_uint(ps, 0xFFFF);
        expect(ps, ',');
        ports[n].value = (uint8_t)parse_uint(ps, 0xFF);
        expect(ps, ',');
        char dir[4];
        parse_string(ps, dir, sizeof dir);
        if (strcmp(dir, "r") != 0 && strcmp(dir, "w") != 0) {
            fail(ps, "bad port direction", dir);
        }
        ports[n].dir = (uint8_t)dir[0];
        expect(ps, ']');
        n++;
        if (peek(ps) == ',') {
            ps->p++;
        }
    }
    ps->p++;
    return n;
}

/* Reads the bus cycles back out of the per-T-state stream by the templates
   of sst_stream_build. An M1 and a memory read share their first two
   T-states; the M1 puts the refresh address out on its third. */
static unsigned extract(struct parser *ps, const struct sst_tick *c, unsigned nc,
                        struct sst_access *a)
{
    unsigned n = 0;
    for (unsigned k = 0; k < nc;) {
        struct sst_access x = {0, (uint8_t)k, c[k].addr, 0, 0};
        unsigned len = 1;
        if (c[k].pins == 0 && c[k].data < 0 && k + 2 < nc) {
            uint8_t p1 = c[k + 1].pins, p2 = c[k + 2].pins;
            if (p1 == (SST_PIN_R | SST_PIN_M)) {
                int m1 = c[k + 2].addr != c[k].addr;
                x.kind = m1 ? SST_M1 : SST_MR;
                x.data = (uint8_t)c[k + 2].data;
                x.refresh = m1 ? c[k + 2].addr : 0;
                len = m1 ? 4 : 3;
            } else if (p1 == (SST_PIN_W | SST_PIN_M)) {
                x.kind = SST_MW;
                x.data = (uint8_t)c[k + 1].data;
                len = 3;
            } else if (p1 == 0 && p2 == (SST_PIN_R | SST_PIN_I) && k + 3 < nc) {
                x.kind = SST_IOR;
                x.data = (uint8_t)c[k + 3].data;
                len = 4;
            } else if (p1 == 0 && p2 == (SST_PIN_W | SST_PIN_I)) {
                x.kind = SST_IOW;
                x.data = (uint8_t)c[k + 2].data;
                len = 4;
            }
        }
        if (x.kind != 0) {
            if (n == SST_MAX_ACCESS) {
                fail(ps, "too many bus cycles", "");
            }
            a[n++] = x;
        }
        k += len;
    }
    return n;
}

static int parse_test(struct parser *ps, struct sst_test *t)
{
    struct sst_tick cyc[MAX_CYCLES];
    unsigned nc = 0;
    int have_initial = 0, have_final = 0, have_cycles = 0;
    memset(t, 0, sizeof *t);
    expect(ps, '{');
    while (peek(ps) != '}') {
        char key[16];
        parse_string(ps, key, sizeof key);
        expect(ps, ':');
        if (strcmp(key, "initial") == 0) {
            t->n_ram_in = parse_state(ps, t->initial, t->ram_in);
            have_initial = 1;
        } else if (strcmp(key, "final") == 0) {
            t->n_ram_out = parse_state(ps, t->final, t->ram_out);
            have_final = 1;
        } else if (strcmp(key, "cycles") == 0) {
            nc = parse_cycles(ps, cyc);
            have_cycles = 1;
        } else if (strcmp(key, "ports") == 0) {
            t->n_ports = parse_ports(ps, t->ports);
        } else {
            skip_value(ps, 0);
        }
        if (peek(ps) == ',') {
            ps->p++;
        }
    }
    ps->p++;
    if (!have_initial) {
        fail(ps, "missing key", "initial");
    }
    if (!have_final) {
        fail(ps, "missing key", "final");
    }
    if (!have_cycles) {
        fail(ps, "missing key", "cycles");
    }

    t->total_t = nc;
    t->n_access = extract(ps, cyc, nc, t->access);
    struct sst_tick re[SST_MAX_T];
    int ok = sst_stream_build(t->access, t->n_access, nc, re) == 0;
    for (unsigned k = 0; ok && k < nc; k++) {
        ok = re[k].addr == cyc[k].addr && re[k].data == cyc[k].data && re[k].pins == cyc[k].pins;
        if (!ok) {
            char at[32];
            snprintf(at, sizeof at, "T-state %u", k);
            fail(ps, "cycles do not rebuild from their bus accesses", at);
        }
    }
    if (!ok) {
        fail(ps, "cycles do not rebuild from their bus accesses", "");
    }
    return 0;
}

static char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    char *buf = NULL;
    size_t cap = 0, n = 0;
    for (;;) {
        if (n == cap) {
            cap = cap ? cap * 2 : 1u << 20;
            char *grown = realloc(buf, cap);
            if (!grown) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = grown;
        }
        size_t got = fread(buf + n, 1, cap - n, f);
        n += got;
        if (got == 0) {
            break;
        }
    }
    int err = ferror(f);
    fclose(f);
    if (err) {
        free(buf);
        return NULL;
    }
    *len = n;
    return buf;
}

static void put32(uint8_t *o, uint32_t v)
{
    for (unsigned b = 0; b < 4; b++) {
        o[b] = (uint8_t)(v >> (8 * b));
    }
}

/* True if name is one of the comma-separated names of list. */
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

static int usage(void)
{
    fprintf(stderr, "usage: cogmaster-sst2bin --dir V1DIR --count N --out FILE [--files NAMES]\n");
    return 2;
}

int main(int argc, char **argv)
{
    const char *dir = NULL, *out = NULL, *files = NULL;
    long count = 0;
    for (int k = 1; k < argc; k++) {
        if (k + 1 >= argc) {
            return usage();
        }
        if (strcmp(argv[k], "--dir") == 0) {
            dir = argv[++k];
        } else if (strcmp(argv[k], "--out") == 0) {
            out = argv[++k];
        } else if (strcmp(argv[k], "--files") == 0) {
            files = argv[++k];
        } else if (strcmp(argv[k], "--count") == 0) {
            char *end;
            count = strtol(argv[++k], &end, 10);
            if (*end != '\0' || count < 1 || count > 100000) {
                return usage();
            }
        } else {
            return usage();
        }
    }
    if (!dir || !out || count == 0) {
        return usage();
    }

    unsigned n_files = 0;
    for (unsigned f = 0; f < SST_FILES; f++) {
        char name[SST_NAME_MAX];
        sst_file_name(f, name);
        if (!files || listed(files, name)) {
            n_files++;
        }
    }
    if (n_files == 0) {
        fprintf(stderr, "sst2bin: --files names no known file\n");
        return 1;
    }

    FILE *o = fopen(out, "wb");
    if (!o) {
        fprintf(stderr, "sst2bin: cannot write %s\n", out);
        return 1;
    }
    uint8_t head[20];
    memcpy(head, SST_MAGIC, 8);
    put32(head + 8, SST_VERSION);
    put32(head + 12, (uint32_t)count);
    put32(head + 16, n_files);
    int werr = fwrite(head, 1, sizeof head, o) != sizeof head;

    unsigned long tests = 0;
    for (unsigned f = 0; f < SST_FILES && !werr; f++) {
        char name[SST_NAME_MAX];
        sst_file_name(f, name);
        if (files && !listed(files, name)) {
            continue;
        }
        char path[4096];
        snprintf(path, sizeof path, "%s/%s.json", dir, name);
        size_t len = 0;
        char *text = read_file(path, &len);
        if (!text) {
            fprintf(stderr, "sst2bin: cannot read %s\n", path);
            fclose(o);
            return 1;
        }
        uint8_t idx[2] = {(uint8_t)f, (uint8_t)(f >> 8)};
        werr |= fwrite(idx, 1, 2, o) != 2;
        struct parser ps = {text, text + len, name, 0};
        expect(&ps, '[');
        for (; ps.test < count && !werr; ps.test++) {
            if (ps.test > 0) {
                if (peek(&ps) != ',') {
                    fail(&ps, "file holds fewer tests than --count", "");
                }
                ps.p++;
            }
            struct sst_test t;
            parse_test(&ps, &t);
            uint8_t rec[1024];
            size_t n = sst_encode(&t, rec);
            werr |= fwrite(rec, 1, n, o) != n;
            tests++;
        }
        free(text);
    }
    if (fclose(o) != 0 || werr) {
        fprintf(stderr, "sst2bin: cannot write %s\n", out);
        return 1;
    }
    printf("sst2bin: %u files, %lu tests\n", n_files, tests);
    return 0;
}
