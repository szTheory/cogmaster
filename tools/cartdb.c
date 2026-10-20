/* Bounded converter for the pinned CC0 MAME software lists. This is a
   deliberately restricted XML reader, not a general XML implementation:
   no external entities, DTD expansion, or network access. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#define CAP 4096u
struct fact {
    char sha[41], title[256];
    uint32_t bytes, board, machine, region, video, persistence, declared, geometry;
    uint32_t public_identity;
};
static struct fact facts[CAP];
static size_t count, compound;
static char input[4194305];

static void fail(const char *why)
{
    fprintf(stderr, "cartdb: %s\n", why);
    exit(1);
}

/* Copy an attribute from a single start tag, accepting either quote. */
static int attr(const char *tag, const char *key, char *out, size_t cap)
{
    const char *end = strchr(tag, '>');
    if (!end)
        fail("unterminated tag");
    size_t k = strlen(key);
    for (const char *p = tag; p < end; p++) {
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
            continue;
        p++;
        if ((size_t)(end - p) < k || strncmp(p, key, k)) {
            p--;
            continue;
        }
        p += k;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p++ != '=') {
            p--;
            continue;
        }
        while (*p == ' ' || *p == '\t')
            p++;
        int quote = *p++;
        if (quote != '"' && quote != '\'')
            fail("unquoted attribute");
        const char *q = strchr(p, quote);
        if (!q || q > end || (size_t)(q - p) >= cap)
            fail("attribute too long");
        memcpy(out, p, (size_t)(q - p));
        out[q - p] = 0;
        return 1;
    }
    out[0] = 0;
    return 0;
}

static uint32_t number(const char *s)
{
    char *end;
    if (!*s || *s == '-')
        fail("invalid size");
    unsigned long n = strtoul(s, &end, s[0] == '0' && s[1] == 'x' ? 16 : 10);
    if (*end || n > 1048576ul)
        fail("invalid size");
    return (uint32_t)n;
}

static void title(char *out, const char *p, size_t n)
{
    size_t used = 0;
    for (size_t i = 0; i < n; i++) {
        char c = p[i];
        if (c == '&') {
            static const char *const names[] = {"&amp;", "&quot;", "&apos;", "&lt;", "&gt;"};
            static const char values[] = {'&', '"', '\'', '<', '>'};
            size_t j;
            for (j = 0; j < 5; j++) {
                size_t len = strlen(names[j]);
                if (len <= n - i && !strncmp(p + i, names[j], len)) {
                    c = values[j];
                    i += len - 1u;
                    break;
                }
            }
            if (j == 5)
                fail("unsupported title entity");
        }
        if (used + 1u >= 256u)
            fail("title too long");
        out[used++] = c;
    }
    out[used] = 0;
}

static int compare(const void *a, const void *b)
{
    const struct fact *x = a, *y = b;
    int c = strcmp(x->sha, y->sha);
    if (c)
        return c;
    if (x->machine != y->machine)
        return x->machine < y->machine ? -1 : 1;
    return strcmp(x->title, y->title);
}

static void parse(char *s)
{
    char *p = s;
    while ((p = strstr(p, "<software ")) != NULL) {
        char *end = strstr(p, "</software>");
        if (!end)
            fail("unterminated software");
        *end = 0;
        struct fact f = {0};
        f.public_identity = 1u;
        char *desc = strstr(p, "<description>"), *de = desc ? strstr(desc, "</description>") : NULL;
        if (!desc || !de)
            fail("missing description");
        title(f.title, desc + 13, (size_t)(de - desc - 13));
        char *part = strstr(p, "<part ");
        if (!part || strstr(part + 1, "<part ")) {
            compound++;
            fprintf(stderr, "cartdb: excluded multipart record: %s\n", f.title);
            p = end + 11;
            continue;
        }
        char value[256];
        (void)attr(part, "interface", value, sizeof value);
        f.machine = !strcmp(value, "gamegear_cart")                              ? 2u
                    : (!strcmp(value, "sms_cart") || !strcmp(value, "sms_card")) ? 1u
                                                                                 : 0u;
        f.region = strstr(f.title, "(Japan)") ? 2u : 0u;
        f.video = f.machine == 2u ? 2u : 1u;
        f.persistence = 1u;
        uint32_t areas = 0, roms = 0, romsize = 0, offset = 0;
        char slot[256] = "";
        for (char *t = part; (t = strchr(t, '<')) != NULL; t++) {
            if (!strncmp(t, "<!--", 4)) {
                char *close = strstr(t, "-->");
                if (!close)
                    fail("unterminated comment");
                t = close + 2;
                continue;
            }
            if (!strncmp(t, "<feature ", 9)) {
                char name[256];
                (void)attr(t, "name", name, sizeof name);
                (void)attr(t, "value", value, sizeof value);
                if (!strcmp(name, "slot")) {
                    if (*slot && strcmp(slot, value))
                        fail("conflicting slot");
                    strcpy(slot, value);
                }
                if (!strcmp(name, "battery"))
                    f.persistence = !strcmp(value, "yes") ? 2u : !strcmp(value, "no") ? 1u : 0u;
                if (!strcmp(name, "pin_42"))
                    f.video = !strcmp(value, "sms_mode") ? 1u : 2u;
            }
            if (!strncmp(t, "<dataarea ", 10)) {
                char name[256];
                (void)attr(t, "name", name, sizeof name);
                (void)attr(t, "size", value, sizeof value);
                uint32_t size = number(value);
                if (!strcmp(name, "ram"))
                    f.declared = size;
                if (strcmp(name, "rom"))
                    continue;
                areas++;
                f.bytes = size;
                char *ae = strstr(t, "</dataarea>");
                if (!ae)
                    fail("unterminated dataarea");
                for (char *r = t; (r = strstr(r, "<rom ")) != NULL && r < ae; r++) {
                    roms++;
                    (void)attr(r, "size", value, sizeof value);
                    romsize = number(value);
                    if (attr(r, "offset", value, sizeof value))
                        offset = number(value);
                    if (!attr(r, "sha1", f.sha, sizeof f.sha))
                        f.sha[0] = 0;
                    if (attr(r, "loadflag", value, sizeof value) ||
                        attr(r, "status", value, sizeof value))
                        roms += 2;
                }
            }
        }
        if (areas != 1u || roms != 1u || romsize != f.bytes || offset || strlen(f.sha) != 40u) {
            compound++;
            fprintf(stderr, "cartdb: excluded compound/incomplete record: %s\n", f.title);
            p = end + 11;
            continue;
        }
        if (strspn(f.sha, "0123456789abcdef") != 40u)
            fail("invalid SHA1");
        f.board = f.machine ? (f.bytes <= 49152u ? 1u : 2u) : 0u;
        if (f.machine == 2u && f.bytes == 8192u)
            f.board = 0u;
        if (*slot)
            f.board = !strcmp(slot, "codemasters") ? 3u
                      : !strcmp(slot, "rom")       ? 1u
                      : !strcmp(slot, "sega")      ? 2u
                                                   : 0u;
        if (f.persistence == 2u && !*slot && f.machine)
            f.board = 2u;
        if (f.persistence == 2u) {
            f.geometry = (f.board == 2u &&
                          (f.declared == 8192u || f.declared == 16384u || f.declared == 32768u))
                             ? 1u
                             : 2u;
        }
        if (count == CAP)
            fail("too many facts");
        facts[count++] = f;
        p = end + 11;
    }
}

static void quoted(const char *s)
{
    putchar('"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            putchar('\\');
            putchar(c);
        } else if (c < 32u || c >= 127u)
            printf("\\%03o", (unsigned int)c);
        else
            putchar(c);
    }
    putchar('"');
}

int main(int argc, char **argv)
{
    if (argc < 2)
        fail("supply CC0 XML files");
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--fixture")) {
            if (++i == argc || count == CAP)
                fail("missing fixture facts");
            FILE *file = fopen(argv[i], "rb");
            if (!file)
                fail("cannot open fixture facts");
            struct fact f = {0};
            int fields = fscanf(file,
                                "%40s %" SCNu32 " %" SCNu32 " %" SCNu32 " %" SCNu32 " %" SCNu32
                                " %" SCNu32 " %" SCNu32 " %" SCNu32 " %255[^\n]",
                                f.sha, &f.bytes, &f.board, &f.machine, &f.region, &f.video,
                                &f.persistence, &f.declared, &f.geometry, f.title);
            if (fields != 10 || strlen(f.sha) != 40u || strspn(f.sha, "0123456789abcdef") != 40u ||
                f.bytes != 65536u || f.board != 2u || f.persistence != 2u || f.declared != 32768u ||
                f.geometry != 1u)
                fail("invalid owned fixture facts");
            while (fgets(input, sizeof input, file)) {
                if (strspn(input, " \t\r\n") != strlen(input))
                    fail("extra fixture facts");
            }
            if (ferror(file))
                fail("unreadable fixture facts");
            fclose(file);
            facts[count++] = f;
            continue;
        }
        FILE *file = fopen(argv[i], "rb");
        if (!file)
            fail("cannot open input");
        size_t n = fread(input, 1, sizeof input - 1u, file);
        if (ferror(file) || fgetc(file) != EOF)
            fail("input too large or unreadable");
        fclose(file);
        input[n] = 0;
        if (!strstr(input, "license:CC0-1.0"))
            fail("missing per-file CC0 license");
        parse(input);
    }
    qsort(facts, count, sizeof facts[0], compare);
    puts("/* Generated from pinned MAME CC0 and optional owned MIT fixture facts; see "
         "PROVENANCE.md. */");
    for (size_t i = 0; i < count; i++) {
        if (i && !strcmp(facts[i - 1u].sha, facts[i].sha)) {
            const struct fact *a = &facts[i - 1u], *b = &facts[i];
            /* Card/cart title aliases are not hardware conflicts. A shared
               SMS image also distributed for GG SMS mode defaults to SMS. */
            if (a->bytes != b->bytes || a->board != b->board || a->region != b->region ||
                a->video != b->video || a->persistence != b->persistence ||
                a->declared != b->declared || a->geometry != b->geometry ||
                a->public_identity != b->public_identity ||
                (a->machine != b->machine && a->video != 1u)) {
                fprintf(stderr, "%s: %s / %s\n", a->sha, a->title, b->title);
                fail("conflicting identity");
            }
            continue;
        }
        struct fact *f = &facts[i];
        printf("{\"%s\",", f->sha);
        quoted(f->title);
        printf(",%uu,%uu,%uu,%uu,%uu,%uu,%uu,%uu,%uu},\n", f->bytes, f->board, f->machine,
               f->region, f->video, f->persistence, f->declared, f->geometry, f->public_identity);
    }
    fprintf(stderr, "cartdb: %zu candidate facts; %zu compound/incomplete records excluded\n",
            count, compound);
    return ferror(stdout) ? 1 : 0;
}
