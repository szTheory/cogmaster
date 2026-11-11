/* The anaglyph composition. The case is named on the command line. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "eyes.h"
#include "../check.h"

#define MAX_DOTS 4097u

typedef void (*mix_fn)(const uint16_t *, const uint16_t *, uint16_t *, size_t);

/* A deliberately wrong variant: it leaves the right answer in out but also
   writes the mix into the left picture, which the core may still own. */
static void mix_into_left(const uint16_t *left, const uint16_t *right, uint16_t *out, size_t count)
{
    uint16_t *w = (uint16_t *)(uintptr_t)left;
    for (size_t i = 0; i < count; i++) {
        out[i] = (uint16_t)((left[i] & 0x000Fu) | (right[i] & 0x0FF0u));
        w[i] = out[i];
    }
}

static uint16_t expect_dot(uint16_t l, uint16_t r)
{
    return (uint16_t)((l & 0x000Fu) | (r & 0x0FF0u));
}

/* Runs fn over count dots and returns how many bytes of either input changed. */
static uint32_t inputs_changed(mix_fn fn, size_t count)
{
    static uint16_t l[MAX_DOTS], r[MAX_DOTS], out[MAX_DOTS];
    static uint16_t l0[MAX_DOTS], r0[MAX_DOTS];
    for (size_t i = 0; i < count; i++) {
        l[i] = l0[i] = (uint16_t)((i * 37u + 5u) & 0x0FFFu);
        r[i] = r0[i] = (uint16_t)((i * 91u + 700u) & 0x0FFFu);
        out[i] = 0;
    }
    fn(l, r, out, count);
    return (uint32_t)(memcmp(l, l0, count * sizeof l[0]) != 0) +
           (uint32_t)(memcmp(r, r0, count * sizeof r[0]) != 0);
}

/* Every left value against 16 partners, both ways round: out is the closed
   form, and differs from left whenever right's green or blue differs. */
static void mix(void)
{
    static const uint16_t partner[16] = {0x000, 0x00F, 0x0F0, 0xF00, 0xFFF, 0x123, 0x456, 0x789,
                                         0xABC, 0xDEF, 0xF0F, 0x0FF, 0xFF0, 0x5A5, 0xA5A, 0x321};
    static uint16_t a[4096], b[4096], out[4096];
    uint32_t bad = 0;
    uint32_t differs_ok = 0;
    for (size_t k = 0; k < 16u; k++) {
        for (size_t side = 0; side < 2u; side++) {
            for (uint32_t i = 0; i < 4096u; i++) {
                a[i] = side ? partner[k] : (uint16_t)i;
                b[i] = side ? (uint16_t)i : partner[k];
            }
            cogmaster__eyes_anaglyph(a, b, out, 4096u);
            for (uint32_t i = 0; i < 4096u; i++) {
                bad += (uint32_t)(out[i] != expect_dot(a[i], b[i]));
                bad += (uint32_t)((out[i] >> 12) != 0u);
                if (((a[i] ^ b[i]) & 0x0FF0u) != 0u) {
                    differs_ok += (uint32_t)(out[i] != a[i]);
                } else {
                    differs_ok += (uint32_t)(out[i] == a[i]);
                }
            }
        }
    }
    CHECK_EQ_U64(bad, 0u);
    CHECK_EQ_U64(differs_ok, 16u * 2u * 4096u);

    /* Garbage in the top four bits never reaches the output. */
    uint16_t l = 0xF123u, r = 0xFABCu, o = 0;
    cogmaster__eyes_anaglyph(&l, &r, &o, 1u);
    CHECK_EQ_HEX(o, 0x0AB3u);
    CHECK_EQ_HEX(expect_dot(0x000Fu, 0x0FF0u), 0x0FFFu);
}

/* Inputs stay byte-equal for 1, 2 and 4097 dots; count 0 and NULL change
   nothing; a write past count never happens. */
static void inputs(void)
{
    static const size_t sizes[3] = {1u, 2u, MAX_DOTS};
    for (size_t s = 0; s < 3u; s++) {
        CHECK_EQ_U64(inputs_changed(cogmaster__eyes_anaglyph, sizes[s]), 0u);
    }

    uint16_t l[4] = {0x0111u, 0x0222u, 0x0333u, 0x0444u};
    uint16_t r[4] = {0x0AAAu, 0x0BBBu, 0x0CCCu, 0x0DDDu};
    uint16_t out[4] = {0xDEADu, 0xDEADu, 0xDEADu, 0xDEADu};
    cogmaster__eyes_anaglyph(l, r, out, 0u);
    CHECK_EQ_HEX(out[0], 0xDEADu);
    cogmaster__eyes_anaglyph(NULL, r, out, 4u);
    cogmaster__eyes_anaglyph(l, NULL, out, 4u);
    cogmaster__eyes_anaglyph(l, r, NULL, 4u);
    cogmaster__eyes_anaglyph(NULL, NULL, NULL, 0u);
    for (size_t i = 0; i < 4u; i++) {
        CHECK_EQ_HEX(out[i], 0xDEADu);
    }
    cogmaster__eyes_anaglyph(l, r, out, 3u);
    CHECK_EQ_HEX(out[0], 0x0AA1u);
    CHECK_EQ_HEX(out[2], 0x0CC3u);
    CHECK_EQ_HEX(out[3], 0xDEADu);
}

/* The untouched-inputs comparison must be able to fail: the bad variant has
   to trip it, and the real function must not. */
static void control(void)
{
    CHECK_EQ_U64(inputs_changed(cogmaster__eyes_anaglyph, 64u), 0u);
    CHECK(inputs_changed(mix_into_left, 64u) != 0u);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "eyes") == 0) {
        mix();
        inputs();
        control();
    } else {
        fprintf(stderr, "usage: cogmaster-test-eyes eyes\n");
        return 2;
    }
    CHECK_DONE();
}
