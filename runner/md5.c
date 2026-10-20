/* MD5 as specified in RFC 1321, section 3. Written from the RFC. */
#include "md5.h"

#include <string.h>

/* Section 3.4: T[i] is the integer part of 4294967296 * abs(sin(i)), i from
   1 to 64. */
static const uint32_t T[64] = {
    0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu, 0xf57c0fafu, 0x4787c62au, 0xa8304613u,
    0xfd469501u, 0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu, 0x6b901122u, 0xfd987193u,
    0xa679438eu, 0x49b40821u, 0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau, 0xd62f105du,
    0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u, 0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
    0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au, 0xfffa3942u, 0x8771f681u, 0x6d9d6122u,
    0xfde5380cu, 0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u, 0x289b7ec6u, 0xeaa127fau,
    0xd4ef3085u, 0x04881d05u, 0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u, 0xf4292244u,
    0x432aff97u, 0xab9423a7u, 0xfc93a039u, 0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
    0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u, 0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu,
    0xeb86d391u,
};

/* Section 3.4: the left rotation of each step, four per round. */
static const uint8_t S[16] = {7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21};

static uint32_t rotl(uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (32u - n));
}

/* Section 3.4: process one 512-bit block, words little-endian. */
static void compress(uint32_t h[4], const uint8_t block[64])
{
    uint32_t x[16];
    for (uint32_t i = 0; i < 16u; i++) {
        x[i] = (uint32_t)block[4u * i] | ((uint32_t)block[4u * i + 1u] << 8) |
               ((uint32_t)block[4u * i + 2u] << 16) | ((uint32_t)block[4u * i + 3u] << 24);
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    for (uint32_t i = 0; i < 64u; i++) {
        uint32_t round = i / 16u;
        uint32_t f, k;
        if (round == 0u) {
            f = (b & c) | (~b & d);
            k = i;
        } else if (round == 1u) {
            f = (b & d) | (c & ~d);
            k = (5u * i + 1u) & 15u;
        } else if (round == 2u) {
            f = b ^ c ^ d;
            k = (3u * i + 5u) & 15u;
        } else {
            f = c ^ (b | ~d);
            k = (7u * i) & 15u;
        }
        uint32_t temp = d;
        d = c;
        c = b;
        b = b + rotl(a + f + x[k] + T[i], S[4u * round + (i & 3u)]);
        a = temp;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
}

void cogmaster__md5_init(cogmaster__md5 *s)
{
    /* Section 3.3: words A, B, C, D, low-order bytes first. */
    static const uint32_t H0[4] = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u};
    memcpy(s->h, H0, sizeof H0);
    s->used = 0;
    s->total = 0;
}

void cogmaster__md5_update(cogmaster__md5 *s, const void *data, size_t len)
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

void cogmaster__md5_final(cogmaster__md5 *s, uint8_t digest[16])
{
    /* Sections 3.1 and 3.2: a 1 bit, zeros, then the 64-bit length in bits,
       little-endian, ending on a block boundary. */
    uint64_t bits = s->total * 8u;
    s->block[s->used++] = 0x80u;
    if (s->used > 56u) {
        memset(s->block + s->used, 0, 64u - (size_t)s->used);
        compress(s->h, s->block);
        s->used = 0;
    }
    memset(s->block + s->used, 0, 56u - (size_t)s->used);
    for (uint32_t i = 0; i < 8u; i++) {
        s->block[56u + i] = (uint8_t)(bits >> (8u * i));
    }
    compress(s->h, s->block);
    /* Section 3.5: A, B, C, D, each low-order byte first. */
    for (uint32_t i = 0; i < 4u; i++) {
        digest[4u * i] = (uint8_t)s->h[i];
        digest[4u * i + 1u] = (uint8_t)(s->h[i] >> 8);
        digest[4u * i + 2u] = (uint8_t)(s->h[i] >> 16);
        digest[4u * i + 3u] = (uint8_t)(s->h[i] >> 24);
    }
}
