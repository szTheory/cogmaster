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
    if (n != 35u || memcmp(line, "p1=", 3u) != 0 || memcmp(line + 5u, " p2=", 4u) != 0 ||
        memcmp(line + 11u, " pause=", 7u) != 0 || memcmp(line + 19u, " reset=", 7u) != 0 ||
        memcmp(line + 27u, " start=", 7u) != 0) {
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
    in.p1 = (uint32_t)(a * 16 + b);
    in.p2 = (uint32_t)(c * 16 + d);
    in.pause = (uint32_t)(line[18] - '0');
    in.reset = (uint32_t)(line[26] - '0');
    in.start = (uint32_t)(line[34] - '0');
    *out = in;
    return 0;
}

int cogmaster__input_validate(FILE *f, uint32_t frames)
{
    cogmaster_input in;
    for (uint64_t i = 0u; i < frames; i++) {
        int rc = cogmaster__input_read(f, &in);
        if (rc != 0) {
            return rc;
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
