/* SHA-1 as specified in RFC 3174, sections 4, 5 and 6.1. Written from the
   RFC. */
#include "sha1.h"

#include <string.h>

static uint32_t rotl(uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (32u - n));
}

/* RFC 3174 section 6.1: process one 512-bit block. */
static void compress(uint32_t h[5], const uint8_t block[64])
{
    uint32_t w[80];
    for (uint32_t t = 0; t < 16u; t++) {
        w[t] = ((uint32_t)block[4u * t] << 24) | ((uint32_t)block[4u * t + 1u] << 16) |
               ((uint32_t)block[4u * t + 2u] << 8) | (uint32_t)block[4u * t + 3u];
    }
    for (uint32_t t = 16; t < 80u; t++) {
        w[t] = rotl(w[t - 3u] ^ w[t - 8u] ^ w[t - 14u] ^ w[t - 16u], 1);
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (uint32_t t = 0; t < 80u; t++) {
        /* Section 5: the function and constant for each group of 20 rounds. */
        uint32_t f, k;
        if (t < 20u) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (t < 40u) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (t < 60u) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        uint32_t temp = rotl(a, 5) + f + e + w[t] + k;
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

void cogmaster__sha1_init(cogmaster__sha1 *s)
{
    /* RFC 3174 section 6.1: the initial H0 to H4. */
    static const uint32_t H0[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    memcpy(s->h, H0, sizeof H0);
    s->used = 0;
    s->total = 0;
}

void cogmaster__sha1_update(cogmaster__sha1 *s, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    s->total += (uint64_t)len;
    while (len > 0u) {
        size_t take = 64u - (size_t)s->used;
        if (take > len) {
            take = len;
        }
        memcpy(s->block + s->used, p, take);
        s->used += (uint32_t)take;
        p += take;
        len -= take;
        if (s->used == 64u) {
            compress(s->h, s->block);
            s->used = 0;
        }
    }
}

void cogmaster__sha1_final(cogmaster__sha1 *s, uint8_t digest[20])
{
    /* RFC 3174 section 4: a 1 bit, zeros, then the 64-bit length in bits,
       big-endian, ending on a block boundary. */
    uint64_t bits = s->total * 8u;
    s->block[s->used++] = 0x80u;
    if (s->used > 56u) {
        memset(s->block + s->used, 0, 64u - (size_t)s->used);
        compress(s->h, s->block);
        s->used = 0;
    }
    memset(s->block + s->used, 0, 56u - (size_t)s->used);
    for (uint32_t i = 0; i < 8u; i++) {
        s->block[56u + i] = (uint8_t)(bits >> (56u - 8u * i));
    }
    compress(s->h, s->block);
    for (uint32_t i = 0; i < 5u; i++) {
        digest[4u * i] = (uint8_t)(s->h[i] >> 24);
        digest[4u * i + 1u] = (uint8_t)(s->h[i] >> 16);
        digest[4u * i + 2u] = (uint8_t)(s->h[i] >> 8);
        digest[4u * i + 3u] = (uint8_t)s->h[i];
    }
}
