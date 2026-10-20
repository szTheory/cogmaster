/* Expectations use the same public per-frame surfaces as embedding hosts. */
#include "expect.h"
#include <stdio.h>
#include <string.h>
#include "exit_status.h"
#include "mailbox.h"
#include "path.h"
#include "sha256.h"

/* Explicit bases and checked accumulation reject signs, whitespace and overflow. */
static int number(const char **p, uint32_t base, uint32_t max, uint32_t *out)
{
    const char *start = *p;
    uint32_t n = 0u;
    for (;;) {
        unsigned char c = (unsigned char)**p;
        uint32_t d = c >= '0' && c <= '9'   ? (uint32_t)(c - '0')
                     : c >= 'a' && c <= 'f' ? (uint32_t)(c - 'a') + 10u
                     : c >= 'A' && c <= 'F' ? (uint32_t)(c - 'A') + 10u
                                            : 16u;
        if (d >= base)
            break;
        if (d > max || n > (max - d) / base)
            return 0;
        n = n * base + d;
        (*p)++;
    }
    *out = n;
    return *p != start;
}

static int parse(cogmaster__expect_case *c, const char *p, uint32_t frames)
{
    static const char *const kinds[] = {"frame@", "ram@", "mailbox@", "boots@"};
    for (c->kind = 0u; c->kind < 4u; c->kind++) {
        size_t n = strlen(kinds[c->kind]);
        if (strncmp(p, kinds[c->kind], n) == 0) {
            p += n;
            break;
        }
    }
    if (c->kind == 4u || !number(&p, 10u, frames, &c->frame) || c->frame == 0u)
        return 0;
    if (c->kind == 0u) {
        if (*p++ != '=' || strlen(p) != 64u)
            return 0;
        for (uint32_t i = 0u; i < 64u; i++) {
            if (!((p[i] >= '0' && p[i] <= '9') || (p[i] >= 'a' && p[i] <= 'f')))
                return 0;
        }
        memcpy(c->hash, p, 65u);
        return 1;
    }
    if (c->kind == 1u) {
        if (strncmp(p, ":$", 2u) != 0)
            return 0;
        p += 2;
        if (!number(&p, 16u, 65535u, &c->address) || c->address < 0xC000u || *p++ != '=' ||
            !number(&p, 16u, 255u, &c->value))
            return 0;
    }
    return *p == '\0';
}

int cogmaster__expect_load(cogmaster__expect *e, const char *path, uint32_t frames)
{
    memset(e, 0, sizeof *e);
    if (frames > COGMASTER__EXPECT_FRAMES)
        return COGMASTER__EXIT_USAGE;
    FILE *f = cogmaster__path_open(path, "rb");
    if (f == NULL)
        return COGMASTER__EXIT_FAILURE;
    int status = COGMASTER__EXIT_OK;
    char line[256];
    size_t n = 0u;
    int ch;
    /* Byte reads make NUL, CR and unterminated overlong records unambiguous. */
    while ((ch = fgetc(f)) != EOF && status == COGMASTER__EXIT_OK) {
        if (ch == '\n') {
            line[n] = '\0';
            if (e->count == COGMASTER__EXPECT_MAX || !parse(&e->cases[e->count], line, frames))
                status = COGMASTER__EXIT_USAGE;
            else
                e->count++;
            n = 0u;
        } else if (ch < 32 || ch > 126 || n == sizeof line - 1u) {
            status = COGMASTER__EXIT_USAGE;
        } else {
            line[n++] = (char)ch;
        }
    }
    if (n != 0u || e->count == 0u)
        status = COGMASTER__EXIT_USAGE;
    if (ferror(f))
        status = COGMASTER__EXIT_FAILURE;
    if (fclose(f) != 0)
        status = COGMASTER__EXIT_FAILURE;
    if (status != COGMASTER__EXIT_OK)
        fprintf(stderr, "cogmaster-run: expectation file error (status %d)\n", status);
    return status;
}

static int mailbox(const cogmaster *m)
{
    uint8_t mb[256];
    for (uint32_t i = 0u; i < sizeof mb; i++)
        mb[i] = (uint8_t)cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xC100u + i);
    if (memcmp(mb, "CMDG", 4u) != 0 || mb[4] != 1u || mb[5] != 0u || mb[6] == 0u || mb[6] > 60u ||
        cogmaster__crc16_ccitt(mb + 16, 240u) != ((uint32_t)mb[8] | (uint32_t)mb[9] << 8))
        return 0;
    for (uint32_t i = 0u; i < mb[6]; i++) {
        if ((mb[17u + i * 4u] & 3u) != 1u || (mb[17u + i * 4u] & ~7u) != 0u)
            return 0;
        for (uint32_t j = 0u; j < i; j++) {
            if (mb[16u + i * 4u] == mb[16u + j * 4u])
                return 0;
        }
    }
    return 1;
}

int cogmaster__expect_frame(cogmaster__expect *e, const cogmaster *m, uint32_t frame,
                            uint32_t display, uint16_t *pixels)
{
    cogmaster_info info = {0};
    info.size = sizeof info;
    if (cogmaster_get_info(m, &info) != COGMASTER_STATUS_OK)
        return COGMASTER__EXIT_FAILURE;
    cogmaster_frame f = {0};
    f.size = sizeof f;
    f.pixels = pixels;
    f.pitch_bytes = info.crop_w * 2u;
    cogmaster_fault fault = {0};
    fault.size = sizeof fault;
    if (cogmaster_get_frame(m, &f) != COGMASTER_STATUS_OK ||
        cogmaster_get_fault(m, &fault) != COGMASTER_STATUS_OK)
        return COGMASTER__EXIT_FAILURE;
    cogmaster__sha256 s;
    cogmaster__sha256_init(&s);
    for (uint32_t y = 0u; y < f.h; y++) {
        for (uint32_t x = 0u; x < f.w; x++) {
            uint16_t v = pixels[(size_t)y * (f.pitch_bytes / 2u) + x];
            uint8_t bytes[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
            cogmaster__sha256_update(&s, bytes, sizeof bytes);
        }
    }
    uint8_t digest[32];
    char hex[65];
    cogmaster__sha256_final(&s, digest);
    cogmaster__sha256_hex(digest, hex);
    e->display |= display;
    e->fault |= fault.present;
    if (e->distinct == 0u) {
        memcpy(e->first, hex, sizeof e->first);
        e->distinct = 1u;
    } else if (strcmp(e->first, hex) != 0) {
        e->distinct = 2u;
    }
    for (uint32_t i = 0u; i < e->count; i++) {
        cogmaster__expect_case *c = &e->cases[i];
        if (c->frame != frame)
            continue;
        c->seen = 1u;
        c->pass = c->kind == 0u ? (uint32_t)(strcmp(c->hash, hex) == 0)
                  : c->kind == 1u
                      ? (uint32_t)(cogmaster_peek(m, COGMASTER_SPACE_BUS, c->address) == c->value)
                  : c->kind == 2u ? (uint32_t)mailbox(m)
                                  : (uint32_t)(e->display && e->distinct >= 2u && !e->fault);
        printf("predicate kind=%lu frame=%lu address=%04lX %s observed_frame=%s display=%lu "
               "distinct_at_least=%lu first_fault=%lu\n",
               (unsigned long)c->kind, (unsigned long)frame, (unsigned long)c->address,
               c->pass ? "pass" : "fail", hex, (unsigned long)e->display,
               (unsigned long)e->distinct, (unsigned long)e->fault);
    }
    return COGMASTER__EXIT_OK;
}

int cogmaster__expect_finish(const cogmaster__expect *e)
{
    int status = COGMASTER__EXIT_OK;
    for (uint32_t i = 0u; i < e->count; i++) {
        if (!e->cases[i].seen)
            return COGMASTER__EXIT_FAILURE;
        if (!e->cases[i].pass)
            status = COGMASTER__EXIT_MISMATCH;
    }
    return status;
}
