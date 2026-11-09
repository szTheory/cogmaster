/* A seeded stream of YM2413 register writes. One call per output sample: with
   probability 1 in 8 the sample carries one write, to a register address from
   the chip's set ($00-$07, $0E, $0F, $10-$18, $20-$28, $30-$38) with a value
   from the generator. The oracle and the lockstep test feed it to the original
   chip and to the port, so both see the same writes at the same samples. The
   generator is splitmix64, which has no state beyond one 64-bit word. */
#ifndef COGMASTER_TESTS_AUDIO_OPLL_STREAM_H
#define COGMASTER_TESTS_AUDIO_OPLL_STREAM_H

#include <stdint.h>

#define OPLL_STREAM_SEED UINT64_C(0x4f504c4c32343133)
#define OPLL_STREAM_ADDRESSES 37u

struct opll_stream {
    uint64_t state;
};

static inline void opll_stream_init(struct opll_stream *s, uint64_t seed)
{
    s->state = seed;
}

static inline uint64_t opll_stream_next(struct opll_stream *s)
{
    uint64_t z;
    s->state += UINT64_C(0x9e3779b97f4a7c15);
    z = s->state;
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

/* The index-th address of the set, index below OPLL_STREAM_ADDRESSES. */
static inline uint32_t opll_stream_address(uint32_t index)
{
    if (index < 8u) {
        return index;
    }
    if (index < 10u) {
        return 0x0eu + (index - 8u);
    }
    if (index < 19u) {
        return 0x10u + (index - 10u);
    }
    if (index < 28u) {
        return 0x20u + (index - 19u);
    }
    return 0x30u + (index - 28u);
}

/* Advances one sample. Returns 1 and sets *reg and *val when the sample
   carries a write, 0 when it carries none. */
static inline int opll_stream_step(struct opll_stream *s, uint32_t *reg, uint32_t *val)
{
    uint64_t r = opll_stream_next(s);
    if ((r & 7u) != 0u) {
        return 0;
    }
    *reg = opll_stream_address((uint32_t)((r >> 8) % OPLL_STREAM_ADDRESSES));
    *val = (uint32_t)(r >> 40) & 0xffu;
    return 1;
}

#endif /* COGMASTER_TESTS_AUDIO_OPLL_STREAM_H */
