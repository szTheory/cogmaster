/* Bounded converter for the pinned CC0 MAME software lists. This is a
   deliberately restricted XML reader, not a general XML implementation:
   no external entities, DTD expansion, or network access. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "cogmaster.h"

#define CAP 4096u
struct fact {
    char sha[41], title[256];
    uint32_t bytes, board, machine, region, video, persistence, declared, geometry;
    uint32_t public_identity, slot, ctrl1, ctrl2;
};
static struct fact facts[CAP];
static size_t count, compound;
static char input[4194305];

/* Every MAME slot value in the pinned lists, with a stable id that is never
   renumbered. A row maps to a board constant, or to board 0 with the reason
   the game is not played. Ids are the row position plus one; id 0 means a
   record without a slot feature. */
struct slot {
    const char *name;
    uint32_t id, board;
    const char *reason;
};
/* The reason a slot carries while its board is being built; no row may ship with it. */
#define NOT_YET "mapper arm not present"
#define SG1000 "SG-1000 board; the SG-1000 is not emulated"
static const struct slot slots[] = {
    {"codemasters", 1u, COGMASTER_BOARD_CODEMASTERS, ""},
    {"eeprom", 2u, COGMASTER_BOARD_SEGA_EEPROM, ""},
    {"mgear", 3u, 0u, "Master Gear converter, not a game"},
    {"xterminator", 4u, 0u, "X-Terminator cheat device"},
    {"cardcatcher", 5u, 0u, SG1000},
    {"castle", 6u, 0u, SG1000},
    {"dahjee_typea", 7u, 0u, SG1000},
    {"dahjee_typeb", 8u, 0u, SG1000},
    {"level3", 9u, 0u, SG1000},
    {"othello", 10u, 0u, SG1000},
    {"terebi", 11u, 0u, SG1000},
    {"4pak", 12u, COGMASTER_BOARD_4PAK, ""},
    {"hicom", 13u, COGMASTER_BOARD_HICOM, ""},
    {"janggun", 14u, 0u, "reverse bit 6 vs 7 unresolved (MAME and SMS Power disagree)"},
    {"korean", 15u, COGMASTER_BOARD_KOREAN, ""},
    {"korean_188in1", 16u, 0u, "menu entries 6 and 13 fail in MAME and ares"},
    {"korean_nb", 17u, COGMASTER_BOARD_KOREAN_NB, ""},
    {"nemesis", 18u, COGMASTER_BOARD_NEMESIS, ""},
    {"seojin", 19u, 0u, "game select at $FFF0 and data bit 7 inverted; no hardware dump evidence"},
    {"zemina", 20u, COGMASTER_BOARD_ZEMINA, ""},
    {"graphic", 21u, 0u, "Sega Graphic Board peripheral is not emulated"},
};
#define SLOTS (sizeof slots / sizeof slots[0])
static uint32_t slot_seen[SLOTS], slot_emitted[SLOTS];
static const char *const board_names[] = {
    [0] = "unsupported",
    [COGMASTER_BOARD_ROM_ONLY] = "none",
    [COGMASTER_BOARD_SEGA] = "sega",
    [COGMASTER_BOARD_CODEMASTERS] = "codemasters",
    [COGMASTER_BOARD_KOREAN] = "korean",
    [COGMASTER_BOARD_KOREAN_NB] = "korean_nb",
    [COGMASTER_BOARD_HICOM] = "hicom",
    [COGMASTER_BOARD_ZEMINA] = "zemina",
    [COGMASTER_BOARD_NEMESIS] = "nemesis",
    [COGMASTER_BOARD_4PAK] = "hes_4pak",
    [COGMASTER_BOARD_SEGA_EEPROM] = "sega_eeprom",
};
#define BOARD_NAMES (sizeof board_names / sizeof board_names[0])

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

/* The table must be self-consistent before any input is read. */
static void check_table(void)
{
    for (size_t i = 0; i < SLOTS; i++) {
        const struct slot *s = &slots[i];
        size_t reason = strlen(s->reason);
        if (s->id != i + 1u || !*s->name || (s->board != 0u) != !reason || reason > 95u ||
            s->board >= BOARD_NAMES || !strcmp(s->reason, NOT_YET))
            fail("slot table row is not consistent");
        for (const char *c = s->reason; *c; c++)
            if (*c < 32 || *c > 126 || *c == '"' || *c == '\\')
                fail("slot table reason is not plain ASCII");
        for (size_t j = 0; j < i; j++)
            if (!strcmp(slots[j].name, s->name))
                fail("slot table has a duplicate name");
    }
}

/* Slot rows in name order, for the listing and the counts. */
static void by_name(size_t order[SLOTS])
{
    for (size_t i = 0; i < SLOTS; i++) {
        size_t j = i;
        for (; j && strcmp(slots[order[j - 1u]].name, slots[i].name) > 0; j--)
            order[j] = order[j - 1u];
        order[j] = i;
    }
}

static const struct slot *find_slot(const char *name)
{
    for (size_t i = 0; i < SLOTS; i++)
        if (!strcmp(slots[i].name, name))
            return &slots[i];
    fprintf(stderr, "cartdb: unmapped slot %s\n", name);
    exit(1);
}

/* The controller code of the Sega Graphic Board; a record that names it as a default controller is
   the graphic slot. Fault prevented: a Graphic Board dump loaded as a plain pad, so a pad boot
   result would be recorded as a claim about a peripheral the core does not emulate. */
#define GRAPHIC_CODE 4u

static uint32_t controller(const char *value)
{
    static const char *const names[] = {"lphaser", "paddle", "sports", "graphic"};
    for (uint32_t i = 0; i < 4u; i++)
        if (!strcmp(value, names[i]))
            return i + 1u;
    fprintf(stderr, "cartdb: unmapped controller %s\n", value);
    exit(1);
}

/* Every slot and default-controller feature of one software record, before
   any multipart or compound exclusion, so the counts see excluded records and
   an unknown value stops the run. */
static void scan_features(char *p, uint32_t *ctrl1, uint32_t *ctrl2)
{
    uint32_t counted = 0;
    for (char *t = p; (t = strchr(t, '<')) != NULL; t++) {
        char name[256], value[256];
        if (!strncmp(t, "<!--", 4)) {
            char *close = strstr(t, "-->");
            if (!close)
                fail("unterminated comment");
            t = close + 2;
            continue;
        }
        int shared = !strncmp(t, "<sharedfeat ", 12);
        if (!shared && strncmp(t, "<feature ", 9))
            continue;
        (void)attr(t, "name", name, sizeof name);
        (void)attr(t, "value", value, sizeof value);
        if (!shared && !strcmp(name, "slot")) {
            const struct slot *s = find_slot(value);
            if (!(counted & 1u << s->id)) {
                counted |= 1u << s->id;
                slot_seen[s->id - 1u]++;
            }
        } else if (shared && !strcmp(name, "ctrl1_default"))
            *ctrl1 = controller(value);
        else if (shared && !strcmp(name, "ctrl2_default"))
            *ctrl2 = controller(value);
    }
    if (*ctrl1 == GRAPHIC_CODE || *ctrl2 == GRAPHIC_CODE)
        slot_seen[find_slot("graphic")->id - 1u]++;
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
        scan_features(p, &f.ctrl1, &f.ctrl2);
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
        if (f.declared > 65535u)
            fail("invalid ram size");
        f.board = f.machine ? (f.bytes <= 49152u ? 1u : 2u) : 0u;
        if (f.machine == 2u && f.bytes == 8192u)
            f.board = 0u;
        if (*slot) {
            const struct slot *row = find_slot(slot);
            f.slot = row->id;
            f.board = row->board;
            /* The EEPROM board keeps 128 bytes, the 93C46's 64 words of 16 bits. */
            if (row->board == COGMASTER_BOARD_SEGA_EEPROM) {
                f.persistence = 2u;
                f.declared = 128u;
            }
        }
        if (f.ctrl1 == GRAPHIC_CODE || f.ctrl2 == GRAPHIC_CODE) {
            if (*slot)
                fail("graphic controller on a slot cartridge");
            f.slot = find_slot("graphic")->id;
            f.board = 0u;
        }
        if (f.persistence == 2u && !*slot && f.machine && f.board)
            f.board = 2u;
        if (f.persistence == 2u) {
            f.geometry = ((f.board == 2u &&
                           (f.declared == 8192u || f.declared == 16384u || f.declared == 32768u)) ||
                          (f.board == COGMASTER_BOARD_SEGA_EEPROM && f.declared == 128u))
                             ? 1u
                             : 2u;
        }
        if (count == CAP)
            fail("too many facts");
        facts[count++] = f;
        p = end + 11;
    }
}

static int is_hex40(const char *s)
{
    return strlen(s) == 40u && strspn(s, "0123456789abcdef") == 40u;
}

/* Merge two rows that share a SHA-1. Card and cart title aliases are not hardware
   conflicts. A shared SMS image also distributed for GG SMS mode defaults to SMS.
   A controller named on one alias and not the other keeps the device. */
static void coalesce(struct fact *a, const struct fact *b)
{
    if (a->bytes != b->bytes || a->board != b->board || a->region != b->region ||
        a->video != b->video || a->persistence != b->persistence || a->declared != b->declared ||
        a->geometry != b->geometry || a->public_identity != b->public_identity ||
        a->slot != b->slot || (a->ctrl1 && b->ctrl1 && a->ctrl1 != b->ctrl1) ||
        (a->ctrl2 && b->ctrl2 && a->ctrl2 != b->ctrl2) ||
        (a->machine != b->machine && a->video != 1u)) {
        fprintf(stderr, "%s: %s / %s\n", a->sha, a->title, b->title);
        fail("conflicting identity");
    }
    if (!a->ctrl1)
        a->ctrl1 = b->ctrl1;
    if (!a->ctrl2)
        a->ctrl2 = b->ctrl2;
}

static void print_slots(void)
{
    size_t order[SLOTS];
    by_name(order);
    for (size_t i = 0; i < SLOTS; i++) {
        const struct slot *s = &slots[order[i]];
        fprintf(stderr, "cartdb: slot %s seen=%" PRIu32 " emitted=%" PRIu32 " -> ", s->name,
                slot_seen[order[i]], slot_emitted[order[i]]);
        if (s->board)
            fprintf(stderr, "%s\n", board_names[s->board]);
        else
            fprintf(stderr, "unsupported (%s)\n", s->reason);
    }
}

static void write_catalog(const char *path)
{
    static const char *const machines[] = {"sg", "sms", "gg"};
    FILE *out = fopen(path, "wb");
    if (!out)
        fail("cannot open catalog");
    for (size_t i = 0; i < count; i++) {
        const struct fact *f = &facts[i];
        if (!(f->public_identity & 1u))
            continue;
        if (strpbrk(f->title, "\t\r\n"))
            fail("title holds a tab or newline");
        fprintf(out, "%s\t%s\t%s\t%s\t%s\n", f->sha, machines[f->machine], board_names[f->board],
                f->slot ? slots[f->slot - 1u].name : "-", f->title);
    }
    if (ferror(out) || fclose(out))
        fail("cannot write catalog");
}

/* The four owned fixtures, each with its own shape: the Sega battery image (64 KiB, 32 KiB of
   cart RAM) and the Game Gear 93C46 EEPROM image (64 KiB, 128 bytes), neither naming a controller;
   and the Sports Pad and Paddle images (32 KiB, a plain ROM board, no save, region left to the
   header), which name the controller each defaults to. Any other row is not an owned fixture. */
static int owned_fixture(const struct fact *f)
{
    if (!is_hex40(f->sha))
        return 0;
    if (f->bytes == 65536u && f->persistence == 2u && f->geometry == 1u && !f->ctrl1 && !f->ctrl2) {
        if (f->board == 2u && f->declared == 32768u)
            return 1;
        return f->board == 10u && f->machine == COGMASTER_MACHINE_GG && f->declared == 128u;
    }
    if (f->bytes != 32768u || f->board != 1u || f->machine != COGMASTER_MACHINE_SMS2 || f->region ||
        f->persistence || f->declared || f->geometry)
        return 0;
    return (f->ctrl1 == 3u && f->ctrl2 == 3u) || (f->ctrl1 == 2u && !f->ctrl2);
}

int main(int argc, char **argv)
{
    const char *pin = NULL, *catalog = NULL;
    check_table();
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--list-slots")) {
            size_t order[SLOTS];
            by_name(order);
            for (size_t j = 0; j < SLOTS; j++)
                puts(slots[order[j]].name);
            return ferror(stdout) ? 1 : 0;
        }
    }
    if (argc < 2)
        fail("supply CC0 XML files");
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--pin") || !strcmp(argv[i], "--catalog")) {
            const char **dest = argv[i][2] == 'p' ? &pin : &catalog;
            if (++i == argc)
                fail("missing option value");
            *dest = argv[i];
            continue;
        }
        if (!strcmp(argv[i], "--fixture")) {
            if (++i == argc)
                fail("missing fixture facts");
            FILE *file = fopen(argv[i], "rb");
            if (!file)
                fail("cannot open fixture facts");
            size_t rows = 0;
            for (;;) {
                if (count == CAP)
                    fail("missing fixture facts");
                struct fact f = {0};
                int fields =
                    fscanf(file,
                           "%40s %" SCNu32 " %" SCNu32 " %" SCNu32 " %" SCNu32 " %" SCNu32
                           " %" SCNu32 " %" SCNu32 " %" SCNu32 " %" SCNu32 " %" SCNu32 " %255[^\n]",
                           f.sha, &f.bytes, &f.board, &f.machine, &f.region, &f.video,
                           &f.persistence, &f.declared, &f.geometry, &f.ctrl1, &f.ctrl2, f.title);
                if (fields == EOF && rows)
                    break;
                if (fields != 12)
                    fail(rows ? "extra fixture facts" : "invalid owned fixture facts");
                if (!owned_fixture(&f))
                    fail("invalid owned fixture facts");
                facts[count++] = f;
                rows++;
            }
            if (ferror(file))
                fail("unreadable fixture facts");
            fclose(file);
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
    if (!pin || !is_hex40(pin))
        fail("--pin needs the 40 hex digit MAME commit");
    qsort(facts, count, sizeof facts[0], compare);
    size_t candidates = count, kept = 0;
    for (size_t i = 0; i < count; i++) {
        if (kept && !strcmp(facts[kept - 1u].sha, facts[i].sha))
            coalesce(&facts[kept - 1u], &facts[i]);
        else
            facts[kept++] = facts[i];
    }
    count = kept;
    printf("/* Generated by tools/cartdb.c from the MAME hash lists at %s; CC0 facts, see "
           "PROVENANCE.md. */\n",
           pin);
    puts("static const struct cogmaster__cart_slot cartdb_slots[] = {");
    for (size_t i = 0; i < SLOTS; i++)
        printf("{%" PRIu32 "u, %" PRIu32 "u, \"%s\", \"%s\"},\n", slots[i].id, slots[i].board,
               slots[i].name, slots[i].reason);
    puts("};");
    puts("static const struct cogmaster__cart_fact cartdb_facts[] = {");
    for (size_t i = 0; i < count; i++) {
        const struct fact *f = &facts[i];
        fputs("{.sha1 = {", stdout);
        for (size_t b = 0; b < 20u; b++) {
            unsigned int v;
            (void)sscanf(f->sha + 2u * b, "%2x", &v);
            printf(b ? ",0x%02x" : "0x%02x", v);
        }
        printf("}, .bytes = %" PRIu32 "u, .ram_bytes = %" PRIu32 "u, .board = %" PRIu32
               "u, .slot = %" PRIu32 "u, .machine = %" PRIu32 "u, .region = %" PRIu32
               "u, .video = %" PRIu32 "u, .persistence = %" PRIu32 "u, .geometry = %" PRIu32
               "u, .ctrl1 = %" PRIu32 "u, .ctrl2 = %" PRIu32 "u, .flags = %" PRIu32 "u},\n",
               f->bytes, f->declared, f->board, f->slot, f->machine, f->region, f->video,
               f->persistence, f->geometry, f->ctrl1, f->ctrl2, f->public_identity & 1u);
        if (f->slot)
            slot_emitted[f->slot - 1u]++;
    }
    puts("};");
    if (catalog)
        write_catalog(catalog);
    print_slots();
    fprintf(stderr, "cartdb: %zu candidate facts; %zu compound/incomplete records excluded\n",
            candidates, compound);
    return ferror(stdout) ? 1 : 0;
}
