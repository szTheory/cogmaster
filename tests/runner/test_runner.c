/* The runner's bench statistics, stop flag and probe digests, one case per
   argv[1]: percentile, bytes, format, stop or digests. */
#include <signal.h>
#include <stdint.h>
#include <string.h>

#include "../../runner/bench.h"
#include "../../runner/md5.h"
#include "../../runner/sha1.h"
#include "../../runner/stop.h"
#include "../check.h"

static const uint32_t all_ppm[] = {500000u, 900000u, 990000u, 999000u, 999900u, 999990u, 1000000u};

/* 300,000 samples, the nightly trend run's size; static, not on the stack. */
#define BIG 300000u
static uint32_t big[BIG];

/* Sorts n samples and checks the seven reported values against want. */
static void expect_stats(const char *what, uint32_t *us, uint32_t n, const uint32_t want[7])
{
    uint32_t got[7];
    cogmaster__bench_stats(us, n, got);
    for (int i = 0; i < 7; i++) {
        if (got[i] != want[i]) {
            fprintf(stderr, "%s: value %d is %lu, expected %lu\n", what, i, (unsigned long)got[i],
                    (unsigned long)want[i]);
            check_failures++;
        }
    }
}

/* Nearest rank is ceil(n * p), clamped to 1..n, and every reported value is
   a sample at that rank of the sorted input, never an interpolation. */
static void percentile(void)
{
    for (size_t i = 0; i < sizeof all_ppm / sizeof all_ppm[0]; i++) {
        CHECK_EQ_U64(cogmaster__bench_rank(1u, all_ppm[i]), 1u);
    }
    CHECK_EQ_U64(cogmaster__bench_rank(2u, 500000u), 1u);
    CHECK_EQ_U64(cogmaster__bench_rank(2u, 900000u), 2u);
    CHECK_EQ_U64(cogmaster__bench_rank(100u, 990000u), 99u);
    CHECK_EQ_U64(cogmaster__bench_rank(100u, 999000u), 100u);
    CHECK_EQ_U64(cogmaster__bench_rank(BIG, 999990u), 299997u);
    CHECK_EQ_U64(cogmaster__bench_rank(BIG, 500000u), 150000u);
    CHECK_EQ_U64(cogmaster__bench_rank(BIG, 1000000u), BIG);
    CHECK_EQ_U64(cogmaster__bench_rank(7u, 500000u), 4u);
    CHECK_EQ_U64(cogmaster__bench_rank(0xffffffffu, 999990u), 4294924346u);
    CHECK_EQ_U64(cogmaster__bench_us(1999u), 1u);
    CHECK_EQ_U64(cogmaster__bench_us(0xffffffffffffffffu), 0xffffffffu);

    /* One sample: every percentile is that sample. */
    uint32_t one[1] = {7u};
    const uint32_t want_one[7] = {7u, 7u, 7u, 7u, 7u, 7u, 7u};
    expect_stats("one sample", one, 1u, want_one);

    /* Two samples, given in descending order. */
    uint32_t two[2] = {5u, 3u};
    const uint32_t want_two[7] = {3u, 5u, 5u, 5u, 5u, 5u, 5u};
    expect_stats("two samples", two, 2u, want_two);

    /* 100 samples 100 down to 1: the p-th percentile is p itself. */
    uint32_t hundred[100];
    for (uint32_t i = 0; i < 100u; i++) {
        hundred[i] = 100u - i;
    }
    const uint32_t want_hundred[7] = {50u, 90u, 99u, 100u, 100u, 100u, 100u};
    expect_stats("100 descending", hundred, 100u, want_hundred);

    /* 300,000 equal samples. */
    for (uint32_t i = 0; i < BIG; i++) {
        big[i] = 42u;
    }
    const uint32_t want_equal[7] = {42u, 42u, 42u, 42u, 42u, 42u, 42u};
    expect_stats("300000 equal", big, BIG, want_equal);

    /* 300,000 samples BIG down to 1: each percentile is its rank. */
    for (uint32_t i = 0; i < BIG; i++) {
        big[i] = BIG - i;
    }
    const uint32_t want_big[7] = {150000u, 270000u, 297000u, 299700u, 299970u, 299997u, BIG};
    expect_stats("300000 descending", big, BIG, want_big);
}

/* The sample buffer's size is exact up to the cap and 0 above it, never a
   wrapped product: on 32 bits, 2^30 frames would wrap to 0 bytes. */
static void bytes(void)
{
    CHECK_EQ_U64(cogmaster__bench_bytes(0u), 0u);
    CHECK_EQ_U64(cogmaster__bench_bytes(1u), 4u);
    CHECK_EQ_U64(cogmaster__bench_bytes(BIG), 1200000u);
    CHECK_EQ_U64(cogmaster__bench_bytes(COGMASTER__BENCH_MAX_FRAMES), 0xfffffffcu);
    CHECK_EQ_U64(cogmaster__bench_bytes(COGMASTER__BENCH_MAX_FRAMES + 1u), 0u);
    CHECK_EQ_U64(cogmaster__bench_bytes(0xffffffffu), 0u);
}

/* The report is nine "key value" lines in a fixed order, from sorted
   samples. */
static void format(void)
{
    uint32_t us[4] = {40u, 10u, 30u, 20u};
    char buf[512];
    const char *want = "frames 4\n"
                       "p50_us 20\n"
                       "p90_us 40\n"
                       "p99_us 40\n"
                       "p99.9_us 40\n"
                       "p99.99_us 40\n"
                       "p99.999_us 40\n"
                       "max_us 40\n"
                       "max_rss_kib 123\n";
    size_t len = cogmaster__bench_format(buf, sizeof buf, us, 4u, 123u);
    CHECK_EQ_U64(len, strlen(want));
    CHECK(len == strlen(want) && strcmp(buf, want) == 0);
    if (len != strlen(want) || strcmp(buf, want) != 0) {
        fprintf(stderr, "want:\n%sgot:\n%.*s", want, (int)len, buf);
    }
    CHECK_EQ_U64(us[0], 10u);
    CHECK_EQ_U64(cogmaster__bench_format(buf, 20u, us, 4u, 123u), 0u);
}

/* A fresh process has no stop request; SIGINT records one. */
static void stop(void)
{
    CHECK_EQ_U64(cogmaster__stop_requested(), 0u);
    cogmaster__stop_install();
    CHECK_EQ_U64(cogmaster__stop_requested(), 0u);
    CHECK_EQ_U64(raise(SIGINT), 0u);
    CHECK_EQ_U64(cogmaster__stop_requested(), 1u);
}

/* Compares a digest with the expected lowercase hex text. */
static void expect_hex(const char *what, const uint8_t *d, size_t n, const char *want)
{
    static const char digits[] = "0123456789abcdef";
    char got[41];
    for (size_t i = 0; i < n; i++) {
        got[2u * i] = digits[d[i] >> 4];
        got[2u * i + 1u] = digits[d[i] & 0x0fu];
    }
    got[2u * n] = '\0';
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "%s: got %s, expected %s\n", what, got, want);
        check_failures++;
    }
}

static void sha1_of(const char *text, size_t repeat, const char *want)
{
    cogmaster__sha1 s;
    uint8_t d[20];
    cogmaster__sha1_init(&s);
    for (size_t i = 0; i < repeat; i++) {
        cogmaster__sha1_update(&s, text, strlen(text));
    }
    cogmaster__sha1_final(&s, d);
    expect_hex(text, d, sizeof d, want);
}

static void md5_of(const char *text, size_t repeat, const char *want)
{
    cogmaster__md5 s;
    uint8_t d[16];
    cogmaster__md5_init(&s);
    for (size_t i = 0; i < repeat; i++) {
        cogmaster__md5_update(&s, text, strlen(text));
    }
    cogmaster__md5_final(&s, d);
    expect_hex(text, d, sizeof d, want);
}

/* The test vectors of RFC 3174 section 7.3 and RFC 1321 appendix A.5: the
   empty string, "abc", the 56-byte string whose padding spills into a
   second block, and inputs of 64 bytes and more. */
static void digests(void)
{
    sha1_of("", 1u, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    sha1_of("abc", 1u, "a9993e364706816aba3e25717850c26c9cd0d89d");
    sha1_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 1u,
            "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    sha1_of("0123456701234567012345670123456701234567012345670123456701234567", 10u,
            "dea356a2cddd90c7a7ecedc5ebb563934f460452");
    sha1_of("a", 1000000u, "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
    md5_of("", 1u, "d41d8cd98f00b204e9800998ecf8427e");
    md5_of("abc", 1u, "900150983cd24fb0d6963f7d28e17f72");
    md5_of("abcdefghijklmnopqrstuvwxyz", 1u, "c3fcd3d76192e4007dfb496cca67e13b");
    md5_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 1u,
           "d174ab98d277d9f5a5611c2c9f419d9f");
    md5_of("1234567890", 8u, "57edf4a22be3c955ac49da2e2107b67a");
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-bench percentile|bytes|format|stop|digests\n");
        return 2;
    }
    if (strcmp(argv[1], "percentile") == 0) {
        percentile();
    } else if (strcmp(argv[1], "bytes") == 0) {
        bytes();
    } else if (strcmp(argv[1], "format") == 0) {
        format();
    } else if (strcmp(argv[1], "stop") == 0) {
        stop();
    } else if (strcmp(argv[1], "digests") == 0) {
        digests();
    } else {
        fprintf(stderr, "unknown case %s\n", argv[1]);
        return 2;
    }
    CHECK_DONE();
}
