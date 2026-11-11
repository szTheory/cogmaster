/* Fixed-size parsing; no allocation, locale or unbounded string conversion. */
#include <string.h>
#include "input.h"

static int input_hex(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* Parses the ten bytes " x1=+DDDDD" at line + at: a space, the two-letter name, '=', a sign that
   is always given, and five decimal digits. The value must lie in -32767..32767. Returns 0 for any
   other bytes. */
static int input_axis(const char *line, size_t at, const char *name, int32_t *out)
{
    const char *p = line + at;
    if (p[0] != ' ' || p[1] != name[0] || p[2] != name[1] || p[3] != '=' ||
        (p[4] != '+' && p[4] != '-')) {
        return 0;
    }
    int32_t v = 0;
    for (size_t i = 5u; i < 10u; i++) {
        if (p[i] < '0' || p[i] > '9') {
            return 0;
        }
        v = v * 10 + (p[i] - '0');
    }
    if (v > 32767) {
        return 0;
    }
    *out = p[4] == '-' ? -v : v;
    return 1;
}

int cogmaster__input_read(FILE *f, cogmaster_input *out)
{
    char line[96];
    size_t n = 0u;
    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (ch == 0 || n + 1u >= sizeof line) {
            return 2;
        }
        line[n++] = (char)ch;
        if (ch == '\n') {
            break;
        }
    }
    if (ferror(f)) {
        return 1;
    }
    line[n] = '\0';
    if (n != 0u && line[n - 1u] == '\n') {
        n--;
        if (n != 0u && line[n - 1u] == '\r') {
            n--;
        }
    } else if (!feof(f)) {
        return 2;
    }
    if ((n != 35u && n != 75u) || memcmp(line, "p1=", 3u) != 0 ||
        memcmp(line + 5u, " p2=", 4u) != 0 || memcmp(line + 11u, " pause=", 7u) != 0 ||
        memcmp(line + 19u, " reset=", 7u) != 0 || memcmp(line + 27u, " start=", 7u) != 0) {
        return 2;
    }
    int a = input_hex(line[3]), b = input_hex(line[4]);
    int c = input_hex(line[9]), d = input_hex(line[10]);
    if (a < 0 || b < 0 || c < 0 || d < 0 || a > 3 || c > 3 ||
        (line[18] != '0' && line[18] != '1') || (line[26] != '0' && line[26] != '1') ||
        (line[34] != '0' && line[34] != '1')) {
        return 2;
    }
    cogmaster_input in = {0};
    in.size = sizeof in;
    if (n == 75u &&
        (!input_axis(line, 35u, "x1", &in.x1) || !input_axis(line, 45u, "y1", &in.y1) ||
         !input_axis(line, 55u, "x2", &in.x2) || !input_axis(line, 65u, "y2", &in.y2))) {
        return 2;
    }
    in.p1 = (uint32_t)(a * 16 + b);
    in.p2 = (uint32_t)(c * 16 + d);
    in.pause = (uint32_t)(line[18] - '0');
    in.reset = (uint32_t)(line[26] - '0');
    in.start = (uint32_t)(line[34] - '0');
    *out = in;
    return 0;
}

/* Only a Sports Pad moves on both axes and a Paddle on x alone (its y is accepted and ignored by
   the core); a pad or an empty port takes no motion. */
static int input_motion_ok(uint32_t device, int32_t x, int32_t y)
{
    return device == COGMASTER_DEVICE_SPORTS_PAD || device == COGMASTER_DEVICE_PADDLE ||
           (x == 0 && y == 0);
}

int cogmaster__input_validate(FILE *f, uint32_t frames, uint32_t port1, uint32_t port2,
                              int *bad_port)
{
    cogmaster_input in;
    *bad_port = 0;
    for (uint64_t i = 0u; i < frames; i++) {
        int rc = cogmaster__input_read(f, &in);
        if (rc != 0) {
            return rc;
        }
        if (!input_motion_ok(port1, in.x1, in.y1)) {
            *bad_port = 1;
            return 2;
        }
        if (!input_motion_ok(port2, in.x2, in.y2)) {
            *bad_port = 2;
            return 2;
        }
    }
    int c = fgetc(f);
    if (ferror(f)) {
        return 1;
    }
    if (c != EOF) {
        return 2;
    }
    return fseek(f, 0, SEEK_SET) == 0 ? 0 : 1;
}
