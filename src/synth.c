/* Audio output: the exact rational sample cadence, the frame's PCM and the
   host's drain, and the synthesis behind them. A sound source's level change
   becomes a band-limited step: a 16 tap kernel added into an impulse ring at
   the sample the change falls in, one ring per output channel. A frame
   integrates one ring slot per sample, a leaky estimator removes the DC level
   and the result is saturated to 16 bits. All arithmetic is integer; no value
   is shifted right while negative. */
#include <string.h>

#include "internal.h"

/* The kernel: 32 phases of 16 taps in Q15, each row summing to 32,768. Row r
   is a band-limited impulse delayed r / 32 of a sample, its peak at tap 7;
   the integral of an impulse is the step. Seeded from the nesturbator
   synth source at commit 9f9386c661f6fb8f6063bf756655713aedadae77 (same author,
   MIT); the table only. Phase 32 is phase 0 shifted one tap right. */
static const int16_t kernel[32][COGMASTER__SYNTH_TAPS] = {
    {8, -27, 70, -148, 293, -644, 2606, 28452, 2606, -644, 293, -148, 70, -27, 8, 0},
    {6, -21, 48, -93, 171, -376, 1802, 28411, 3457, -919, 419, -204, 92, -34, 9, 0},
    {5, -14, 27, -40, 52, -118, 1047, 28288, 4351, -1199, 547, -262, 115, -42, 11, 0},
    {4, -8, 7, 11, -62, 129, 343, 28082, 5286, -1481, 675, -319, 138, -49, 12, 0},
    {3, -3, -11, 59, -169, 363, -309, 27797, 6257, -1761, 802, -377, 161, -57, 14, -1},
    {2, 3, -28, 103, -270, 582, -906, 27431, 7261, -2038, 927, -434, 184, -64, 16, -1},
    {1, 7, -44, 145, -364, 785, -1448, 26990, 8294, -2308, 1049, -489, 206, -72, 17, -1},
    {1, 12, -58, 182, -449, 970, -1935, 26471, 9351, -2568, 1166, -542, 228, -79, 19, -1},
    {0, 15, -71, 216, -526, 1136, -2366, 25885, 10427, -2814, 1276, -592, 249, -86, 20, -1},
    {-1, 18, -82, 246, -594, 1284, -2742, 25229, 11518, -3044, 1378, -639, 268, -92, 22, -1},
    {-1, 21, -92, 271, -653, 1411, -3062, 24512, 12617, -3254, 1471, -682, 286, -98, 23, -2},
    {-1, 23, -100, 293, -703, 1518, -3329, 23732, 13720, -3440, 1554, -719, 301, -104, 25, -2},
    {-2, 25, -106, 310, -743, 1605, -3542, 22896, 14822, -3600, 1624, -752, 315, -108, 26, -2},
    {-2, 27, -111, 324, -774, 1672, -3705, 22009, 15916, -3729, 1680, -777, 325, -112, 27, -2},
    {-2, 28, -115, 333, -796, 1720, -3818, 21077, 16997, -3826, 1722, -797, 334, -115, 28, -2},
    {-2, 28, -117, 339, -808, 1747, -3885, 20105, 18059, -3885, 1748, -809, 339, -117, 28, -2},
    {-2, 28, -117, 340, -813, 1757, -3906, 19097, 19097, -3906, 1757, -813, 340, -117, 28, -2},
    {-2, 28, -117, 339, -809, 1748, -3885, 18059, 20105, -3885, 1747, -808, 339, -117, 28, -2},
    {-2, 28, -115, 334, -797, 1722, -3826, 16997, 21077, -3818, 1720, -796, 333, -115, 28, -2},
    {-2, 27, -112, 325, -777, 1680, -3729, 15916, 22009, -3705, 1672, -774, 324, -111, 27, -2},
    {-2, 26, -108, 315, -752, 1624, -3600, 14822, 22896, -3542, 1605, -743, 310, -106, 25, -2},
    {-2, 25, -104, 301, -719, 1554, -3440, 13720, 23732, -3329, 1518, -703, 293, -100, 23, -1},
    {-2, 23, -98, 286, -682, 1471, -3254, 12617, 24512, -3062, 1411, -653, 271, -92, 21, -1},
    {-1, 22, -92, 268, -639, 1378, -3044, 11518, 25229, -2742, 1284, -594, 246, -82, 18, -1},
    {-1, 20, -86, 249, -592, 1276, -2814, 10427, 25885, -2366, 1136, -526, 216, -71, 15, 0},
    {-1, 19, -79, 228, -542, 1166, -2568, 9351, 26471, -1935, 970, -449, 182, -58, 12, 1},
    {-1, 17, -72, 206, -489, 1049, -2308, 8294, 26990, -1448, 785, -364, 145, -44, 7, 1},
    {-1, 16, -64, 184, -434, 927, -2038, 7261, 27431, -906, 582, -270, 103, -28, 3, 2},
    {-1, 14, -57, 161, -377, 802, -1761, 6257, 27797, -309, 363, -169, 59, -11, -3, 3},
    {0, 12, -49, 138, -319, 675, -1481, 5286, 28082, 343, 129, -62, 11, 7, -8, 4},
    {0, 11, -42, 115, -262, 547, -1199, 4351, 28288, 1047, -118, 52, -40, 27, -14, 5},
    {0, 9, -34, 92, -204, 419, -919, 3457, 28411, 1802, -376, 171, -93, 48, -21, 6},
};

int32_t cogmaster__synth_kernel(uint32_t row, uint32_t tap)
{
    return row < 32u && tap < COGMASTER__SYNTH_TAPS ? kernel[row][tap] : 0;
}

/* The volume law: 2 dB a step, 32767 times ten to the minus (step over ten),
   rounded; step 15 is off. Step 7 is 6538. */
static const uint16_t volume[16] = {32767, 26028, 20675, 16422, 13045, 10362, 8231, 6538,
                                    5193,  4125,  3277,  2603,  2067,  1642,  1304, 0};

/* Q12 gains applied to a source's level before it enters the ring. The PSG
   alone gets 840; beside the FM unit the pair keeps the 4096 to 1434 ratio at
   491 and 1402. Four PSG channels at full level sum to 26,880 and the kernel
   overshoots its final value by under nine percent, so the peak stays under
   29,204, which is 1 dB below full scale; four PSG channels at 3,928 plus the
   FM unit at 11,216 sum to 26,928 and keep the same bound. */
#define SYNTH_G_PSG 840u
#define SYNTH_G_PSG_FM 491u
#define SYNTH_G_FM 1402u

/* The ring and its sums stay inside int64. A kernel row is added with each tap
   scaled by 65,536 (the interpolation below), so a coefficient is at most
   28,452 times 65,536 in magnitude, and the largest taps of the 33 rows
   (the 32 and phase 0 shifted right) sum to 70,842 at most. A slot receives
   at most SYNTH_EVENTS changes in each of the 16 sample positions that reach
   it: a PSG step every 16 ticks changes four channels, a CPU write every 33
   ticks at most four sources, an FM sample every 216 ticks, over about 224
   ticks a sample, under 100. Each change is at most 32,767 in magnitude. */
#define SYNTH_EVENTS 128
#define SYNTH_DELTA_MAX 32767
#define SYNTH_TAP_SUM_MAX 70842
_Static_assert((int64_t)SYNTH_EVENTS * SYNTH_DELTA_MAX * SYNTH_TAP_SUM_MAX * 65536 <
                   COGMASTER__SYNTH_SLOT_MAX,
               "a legitimate ring slot must stay below the load bound");
/* A loaded state may hold the bound in the integrator and in each of its 32
   saved slots; one frame integrates them all and the integrator is clamped
   to the bound at every sample, so the sum stays well inside int64. */
_Static_assert(COGMASTER__SYNTH_INTEG_MAX +
                       (int64_t)COGMASTER__SYNTH_SAVED * COGMASTER__SYNTH_SLOT_MAX <
                   (INT64_C(1) << 62),
               "a loaded ring and integrator must not overflow while integrated");
_Static_assert(COGMASTER__AUDIO_MAX_FRAMES + COGMASTER__SYNTH_SAVED < COGMASTER__SYNTH_RING,
               "a frame plus the saved slots must fit the ring");

/* The most sample index an event is placed at: a change at most 69 ticks past
   the frame's end lands within a sample of the frame's count. */
#define SYNTH_INDEX_MAX 975u

/* The sample interval of the DC estimator is 2^8 samples (a corner near 30 Hz
   at 48 kHz). */
#define SYNTH_DC_DIV 256u

/* The 48 kHz rate for the loaded TV standard: per tick, inc over mod. */
static void synth_rate(const cogmaster *m, uint32_t *inc, uint32_t *mod)
{
    if (m->tv == COGMASTER_TV_PAL) {
        *inc = COGMASTER__AUDIO_INC_PAL;
        *mod = COGMASTER__AUDIO_MOD_PAL;
    } else {
        *inc = COGMASTER__AUDIO_INC_NTSC;
        *mod = COGMASTER__AUDIO_MOD_NTSC;
    }
}

/* The Game Gear is stereo in either video mode; the Master System is mono. */
static uint32_t synth_channels(const cogmaster *m)
{
    return m->machine == COGMASTER_MACHINE_GG ? 2u : 1u;
}

/* A power reset or a load silences everything. A SOFT reset keeps the ring,
   its head, the integrator, the DC estimate, the stored levels and the
   undrained PCM, so the stream stays continuous; ring positions are relative
   to the head and an event is placed from its own frame's start, so nothing
   lands at a stale position when the tick count restarts. Only the trace log's
   counts, whose ticks restart, are cleared. The chips return to their power-on
   values either way, their step origin included. */
void cogmaster__synth_reset(cogmaster *m, int power)
{
    if (power) {
        memset(&m->synth, 0, sizeof m->synth);
    }
    m->synth.trace_pending = 0u;
    m->synth.trace_dropped = 0u;
    cogmaster__psg_reset(m);
}

/* Division of a signed value by d, truncating toward zero, built on unsigned
   division so no signed shift or signed 64 bit division is involved. */
static int64_t synth_div(int64_t v, uint32_t d)
{
    uint64_t mag = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v;
    uint64_t q = mag / d;
    return v < 0 ? -(int64_t)q : (int64_t)q;
}

static uint32_t synth_sat_add(uint32_t a, uint32_t b)
{
    return b > UINT32_MAX - a ? UINT32_MAX : a + b;
}

/* One sample of one channel: add the slot to the integrator (held to its
   bound), take its Q16 value, move the DC estimate a 256th of the way to it
   and output the difference, saturated to 16 bits. */
static int16_t synth_sample(struct cogmaster__synth *s, uint32_t ch, int64_t *slot)
{
    int64_t integ = s->integ[ch] + *slot;
    *slot = 0;
    if (integ > COGMASTER__SYNTH_INTEG_MAX) {
        integ = COGMASTER__SYNTH_INTEG_MAX;
    } else if (integ < -COGMASTER__SYNTH_INTEG_MAX) {
        integ = -COGMASTER__SYNTH_INTEG_MAX;
    }
    s->integ[ch] = integ;
    int64_t x = synth_div(integ, 32768u);
    s->dc[ch] += synth_div(x - s->dc[ch], SYNTH_DC_DIV);
    int64_t out = synth_div(x - s->dc[ch], 65536u);
    if (out > INT16_MAX) {
        s->stats.clamps = synth_sat_add(s->stats.clamps, 1u);
        return INT16_MAX;
    }
    if (out < INT16_MIN) {
        s->stats.clamps = synth_sat_add(s->stats.clamps, 1u);
        return INT16_MIN;
    }
    return (int16_t)out;
}

void cogmaster__synth_frame_end(cogmaster *m)
{
    struct cogmaster__synth *s = &m->synth;
    uint32_t inc, mod;
    synth_rate(m, &inc, &mod);

    /* Frames the host did not take before this one ends are lost; count
       them, never keep them (saturating, so the counter cannot wrap). */
    uint32_t left = s->pcm_frames - s->pcm_read;
    uint32_t room = UINT32_MAX - m->counters.audio_dropped;
    m->counters.audio_dropped += left < room ? left : room;

    /* The chip runs to this frame's deadline, inclusive, before the frame is
       summed; a write that overshot the deadline has already taken it further.
       The frame has counted itself by now, so its events are placed from the
       start it had while it ran. */
    if (m->frame > 0u) {
        m->frame--;
        cogmaster__psg_catch_up(m, (m->frame + 1u) * cogmaster__frame_ticks(m));
        m->frame++;
    }

    uint64_t total = (uint64_t)s->phase + (uint64_t)cogmaster__frame_ticks(m) * inc;
    uint32_t n = (uint32_t)(total / mod);
    s->phase = (uint32_t)(total % mod);

    /* Only the nominal count integrates; an event past the deadline sits in
       slots after it and is heard next frame. */
    uint32_t channels = synth_channels(m);
    for (uint32_t i = 0u; i < n; i++) {
        for (uint32_t ch = 0u; ch < channels; ch++) {
            int64_t *slot = &s->ring[ch][(s->head + i) & (COGMASTER__SYNTH_RING - 1u)];
            s->pcm[(size_t)i * channels + ch] = synth_sample(s, ch, slot);
        }
    }
    s->head = (s->head + n) & (COGMASTER__SYNTH_RING - 1u);
    s->pcm_frames = n;
    s->pcm_read = 0u;
}

/* Where a change falls: the sample of its frame, and the kernel row for the
   fraction of a sample, with the taps already interpolated. The position is
   the carried phase plus the ticks into the frame times the rate; one
   division gives the row, each tap is a(65536 - f) + b f between two rows
   (exact), and phase 32 is phase 0 one tap to the right. */
struct synth_place {
    uint32_t index;
    int64_t tap[COGMASTER__SYNTH_TAPS];
};

static void synth_place_at(const cogmaster *m, uint64_t tick, struct synth_place *p)
{
    uint32_t inc, mod;
    synth_rate(m, &inc, &mod);
    uint64_t start = m->frame * cogmaster__frame_ticks(m);
    uint64_t into = tick > start ? tick - start : 0u;
    uint64_t pos = (uint64_t)m->synth.phase + into * inc;
    uint64_t index = pos / mod;
    uint64_t frac = pos % mod;
    /* frac < mod < 2^22, so frac shifted by 21 is below 2^43 and row < 32. */
    uint64_t t = (frac << 21) / mod;
    uint32_t row = (uint32_t)(t >> 16);
    int64_t f = (int64_t)(t & 0xFFFFu);
    p->index = index > SYNTH_INDEX_MAX ? SYNTH_INDEX_MAX : (uint32_t)index;
    for (uint32_t k = 0u; k < COGMASTER__SYNTH_TAPS; k++) {
        int64_t a = kernel[row][k];
        int64_t b = row < 31u ? kernel[row + 1u][k] : (k == 0u ? 0 : kernel[0][k - 1u]);
        p->tap[k] = a * (65536 - f) + b * f;
    }
}

static void synth_add(struct cogmaster__synth *s, const struct synth_place *p, uint32_t ch,
                      int32_t delta)
{
    for (uint32_t k = 0u; k < COGMASTER__SYNTH_TAPS; k++) {
        s->ring[ch][(s->head + p->index + k) & (COGMASTER__SYNTH_RING - 1u)] +=
            (int64_t)delta * p->tap[k];
    }
    s->stats.kernel_apps = synth_sat_add(s->stats.kernel_apps, COGMASTER__SYNTH_TAPS);
    if (p->index + COGMASTER__SYNTH_TAPS > s->stats.max_index) {
        s->stats.max_index = p->index + COGMASTER__SYNTH_TAPS;
    }
}

/* Whether PSG channel n reaches output channel ch. The Master System has one
   output. On the Game Gear in native mode the $06 mask sends channel n to the
   left with bit n + 4 and to the right with bit n; in SMS mode the port is not
   decoded and both outputs carry every channel (SMS-AUDIO section 3). */
static int synth_routed(const cogmaster *m, uint32_t source, uint32_t ch)
{
    if (m->machine != COGMASTER_MACHINE_GG) {
        return ch == 0u;
    }
    if (!cogmaster__native_gg(m)) {
        return 1;
    }
    return ((m->synth.level[4] >> (source + (ch == 0u ? 4u : 0u))) & 1u) != 0u;
}

static uint32_t synth_gain(const cogmaster *m, uint32_t source)
{
    if (source < 4u) {
        return m->fm_unit ? SYNTH_G_PSG_FM : SYNTH_G_PSG;
    }
    return SYNTH_G_FM;
}

uint32_t cogmaster__synth_gained_max(const cogmaster *m, uint32_t source)
{
    if (source < 4u || (source == 5u && m->fm_unit)) {
        return (volume[0] * synth_gain(m, source) + 2048u) / 4096u;
    }
    return 0u;
}

/* Re-evaluates PSG channels first to last at tick: the gained level is the
   level's amplitude through the volume law and the gain, non-negative; the
   difference from the one last fed to the ring is what enters it. */
static void synth_psg(cogmaster *m, uint64_t tick, uint32_t first, uint32_t last)
{
    struct cogmaster__synth *s = &m->synth;
    struct synth_place place;
    int placed = 0;
    uint32_t channels = synth_channels(m);
    for (uint32_t src = first; src <= last; src++) {
        uint32_t lv = s->level[src];
        uint32_t amp = (lv & 0x10u) != 0u ? volume[lv & 15u] : 0u;
        uint32_t gained = (amp * synth_gain(m, src) + 2048u) / 4096u;
        for (uint32_t ch = 0u; ch < channels; ch++) {
            uint32_t want = synth_routed(m, src, ch) ? gained : 0u;
            int32_t delta = (int32_t)want - (int32_t)s->gained[src][ch];
            if (delta == 0) {
                continue;
            }
            s->gained[src][ch] = want;
            if (!placed) {
                synth_place_at(m, tick, &place);
                placed = 1;
            }
            synth_add(s, &place, ch, delta);
        }
    }
}

void cogmaster__synth_level(cogmaster *m, uint64_t tick, uint32_t source, uint32_t level)
{
    cogmaster__synth_trace(m, tick, source, level);
    if (source >= COGMASTER__SYNTH_SOURCES) {
        return;
    }
    static const uint32_t width[COGMASTER__SYNTH_SOURCES] = {0x1Fu, 0x1Fu,      0x1Fu, 0x1Fu,
                                                             0xFFu, UINT32_MAX, 0xFFu};
    m->synth.level[source] = level & width[source];
    if (source < 4u) {
        synth_psg(m, tick, source, source);
    } else if (source == 4u) {
        synth_psg(m, tick, 0u, 3u);
    }
}

cogmaster_status cogmaster_get_audio(cogmaster *m, cogmaster_audio *io)
{
    if (m == NULL || io == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(io, COGMASTER__FIRST_SIZE_AUDIO);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    if (io->samples == NULL && io->capacity > 0u) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    struct cogmaster__synth *s = &m->synth;
    uint32_t channels = synth_channels(m);
    uint32_t n = s->pcm_frames - s->pcm_read;
    if (n > io->capacity) {
        n = io->capacity;
    }
    if (n > 0u) {
        memcpy(io->samples, &s->pcm[(size_t)s->pcm_read * channels],
               (size_t)n * channels * sizeof s->pcm[0]);
        s->pcm_read += n;
    }
    io->written = n;
    io->channels = channels;
    io->dropped = m->counters.audio_dropped;
    return COGMASTER_STATUS_OK;
}

void cogmaster__synth_trace(cogmaster *m, uint64_t tick, uint32_t source, uint32_t level)
{
    struct cogmaster__synth *s = &m->synth;
    if (m->trace_log == NULL) {
        return;
    }
    if (s->trace_pending >= m->trace_cap) {
        if (s->trace_dropped < UINT32_MAX) {
            s->trace_dropped++;
        }
        return;
    }
    cogmaster_audio_record *r = &m->trace_log[s->trace_pending++];
    r->tick = tick;
    r->source = source;
    r->level = level;
}

/* Copies the oldest records that fit, reports them and what was lost (the
   log's overflow plus those past capacity), and empties the log. */
cogmaster_status cogmaster_get_audio_trace(cogmaster *m, cogmaster_audio_trace *io)
{
    if (m == NULL || io == NULL || m->trace_cap == 0u) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(io, COGMASTER__FIRST_SIZE_AUDIO_TRACE);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    if (io->records == NULL && io->capacity > 0u) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    struct cogmaster__synth *s = &m->synth;
    uint32_t n = s->trace_pending < io->capacity ? s->trace_pending : io->capacity;
    if (n > 0u) {
        memcpy(io->records, m->trace_log, (size_t)n * sizeof *io->records);
    }
    uint64_t lost = (uint64_t)s->trace_dropped + (s->trace_pending - n);
    io->written = n;
    io->dropped = lost > UINT32_MAX ? UINT32_MAX : (uint32_t)lost;
    s->trace_pending = 0u;
    s->trace_dropped = 0u;
    return COGMASTER_STATUS_OK;
}
