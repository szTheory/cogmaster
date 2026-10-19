#include "bench.h"

uint32_t cogmaster__bench_rank(uint32_t n, uint32_t ppm) { (void)n; (void)ppm; return 0u; }
size_t cogmaster__bench_bytes(uint32_t n) { (void)n; return 0u; }
uint32_t cogmaster__bench_us(uint64_t ns) { (void)ns; return 0u; }
void cogmaster__bench_stats(uint32_t *us, uint32_t n, uint32_t out[7])
{
    (void)us;
    (void)n;
    for (int i = 0; i < 7; i++) {
        out[i] = 0u;
    }
}
size_t cogmaster__bench_format(char *buf, size_t cap, uint32_t *us, uint32_t n,
                               uint64_t max_rss_kib)
{
    (void)buf; (void)cap; (void)us; (void)n; (void)max_rss_kib;
    return 0u;
}
