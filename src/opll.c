/* SPDX-License-Identifier: MIT */
/* The YM2413's sample path: per sample the amplitude and pitch LFOs, the noise
   generator, the envelope and phase generators of all 18 slots, then the sum of the
   14 channel outputs (SMS-AUDIO section 4).
   Upstream: https://github.com/digital-sound-antiques/emu2413, emu2413.c at
   commit a1d5120903b75f93d3eb7690e34a29c0b4add903.
   Copyright (C) 2026 Mitsutaka Okazaki (as the upstream source header states it)
   and Copyright (c) 2001-2019 Mitsutaka Okazaki (as its LICENSE states it).
   Changes: integer-only C17 port; the rate converter, pan, channel mask, alternative
   instrument sets, allocation and memory-image save are removed; every right shift of a
   value that can be negative goes through opll_floor_shift. */
#include <stddef.h>

#include "opll.h"

#define OPLL_WANT_WAVES
#include "opll_tables.inc"

#define EG_MAX (OPLL_EG_MUTE - 4u)
#define DAMPER_RATE 12u
#define PG_MASK 1023u
#define DP_MASK ((1u << 19) - 1u)
#define AM_PERIOD (210u * 64u)

static inline uint32_t opll_min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

/* x >> n rounding toward negative infinity, so the result does not depend on how the
   compiler shifts a negative value. */
static inline int32_t opll_floor_shift(int32_t x, uint32_t n)
{
    return x >= 0 ? x >> n : ~(~x >> n);
}

static uint32_t parameter_rate(const struct cogmaster__opll_slot *slot,
                               const struct cogmaster__opll_patch *p)
{
    if ((slot->type & 1u) == 0u && slot->key_flag == 0u) {
        return 0u;
    }
    switch (slot->eg_state) {
    case OPLL_ATTACK:
        return p->ar;
    case OPLL_DECAY:
        return p->dr;
    case OPLL_SUSTAIN:
        return p->eg ? 0u : p->rr;
    case OPLL_RELEASE:
        if (slot->sus_flag) {
            return 5u;
        }
        return p->eg ? p->rr : 7u;
    case OPLL_DAMP:
        return DAMPER_RATE;
    default:
        return 0u;
    }
}

/* Recomputes the values a register write or a state change invalidated. */
static void commit_slot_update(struct cogmaster__opll *o, struct cogmaster__opll_slot *slot)
{
    const struct cogmaster__opll_patch *p = &o->patch[slot->patch];
    const uint32_t req = slot->update_requests;

    if (req & OPLL_UPDATE_WS) {
        slot->ws = p->ws;
    }
    if (req & OPLL_UPDATE_TLL) {
        const uint32_t tl = (slot->type & 1u) == 0u ? p->tl : slot->volume;
        slot->tll = (uint16_t)(tll_table[slot->blk_fnum >> 5][p->kl] + (tl << 1));
    }
    if (req & OPLL_UPDATE_RKS) {
        slot->rks = rks_table[slot->blk_fnum >> 8][p->kr];
    }
    if (req & (OPLL_UPDATE_RKS | OPLL_UPDATE_EG)) {
        const uint32_t rate = parameter_rate(slot, p);
        if (rate == 0u) {
            slot->eg_shift = 0u;
            slot->eg_rate_h = 0u;
            slot->eg_rate_l = 0u;
        } else {
            const uint32_t h = opll_min(15u, rate + (slot->rks >> 2));
            slot->eg_rate_h = (uint8_t)h;
            slot->eg_rate_l = slot->rks & 3u;
            if (slot->eg_state == OPLL_ATTACK) {
                slot->eg_shift = (uint8_t)((0u < h && h < 12u) ? 13u - h : 0u);
            } else {
                slot->eg_shift = (uint8_t)(h < 13u ? 13u - h : 0u);
            }
        }
    }
    slot->update_requests = 0u;
}

static void update_ampm(struct cogmaster__opll *o)
{
    if (o->test_flag & 2u) {
        o->pm_phase = 0u;
        o->am_phase = 0u;
    } else {
        o->pm_phase += (o->test_flag & 8u) ? 1024u : 1u;
        o->am_phase += (o->test_flag & 8u) ? 64u : 1u;
        if (o->am_phase >= AM_PERIOD) {
            o->am_phase -= AM_PERIOD;
        }
    }
    o->lfo_am = am_table[o->am_phase >> 6];
}

static void update_noise(struct cogmaster__opll *o, uint32_t cycles)
{
    for (uint32_t i = 0u; i < cycles; i++) {
        if (o->noise & 1u) {
            o->noise ^= 0x800200u;
        }
        o->noise >>= 1;
    }
}

/* The cymbal and hi-hat phase bits that make the short noise bit of the hi-hat. */
static void update_short_noise(struct cogmaster__opll *o)
{
    const uint32_t hh = o->slot[OPLL_SLOT_HH].pg_out;
    const uint32_t cym = o->slot[OPLL_SLOT_CYM].pg_out;
    const uint32_t h2 = (hh >> 2) & 1u;
    const uint32_t h3 = (hh >> 3) & 1u;
    const uint32_t h7 = (hh >> 7) & 1u;
    const uint32_t c3 = (cym >> 3) & 1u;
    const uint32_t c5 = (cym >> 5) & 1u;

    o->short_noise = (uint8_t)((h2 ^ h7) | (h3 ^ c5) | (c3 ^ c5));
}

/* Advances the slot's phase by its f-number, block, multiple and pitch LFO offset.
   The arithmetic is modulo 2^32, as the original's is. */
static void calc_phase(struct cogmaster__opll_slot *slot, const struct cogmaster__opll_patch *p,
                       uint32_t pm_phase, uint32_t reset)
{
    const int32_t pm = p->pm ? pm_table[(slot->fnum >> 6) & 7u][(pm_phase >> 10) & 7u] : 0;
    const int32_t f = (int32_t)(slot->fnum & 0x1ffu) * 2 + pm;

    if (reset) {
        slot->pg_phase = 0u;
    }
    slot->pg_phase += ((uint32_t)f * ml_table[p->ml]) << slot->blk >> 2;
    slot->pg_phase &= DP_MASK;
    slot->pg_out = (uint16_t)(slot->pg_phase >> 9);
}

static uint32_t lookup_attack_step(const struct cogmaster__opll_slot *slot, uint32_t counter)
{
    const uint32_t row = slot->eg_rate_l;

    switch (slot->eg_rate_h) {
    case 12:
        return 4u - eg_step_tables[row][(counter & 0xcu) >> 1];
    case 13:
        return 3u - eg_step_tables[row][(counter & 0xcu) >> 1];
    case 14:
        return 2u - eg_step_tables[row][(counter & 0xcu) >> 1];
    case 0:
    case 15:
        return 0u;
    default:
        return eg_step_tables[row][(counter >> slot->eg_shift) & 7u] ? 4u : 0u;
    }
}

static uint32_t lookup_decay_step(const struct cogmaster__opll_slot *slot, uint32_t counter)
{
    const uint32_t row = slot->eg_rate_l;

    switch (slot->eg_rate_h) {
    case 0:
        return 0u;
    case 13:
        return eg_step_tables[row][((counter & 0xcu) >> 1) | (counter & 1u)];
    case 14:
        return eg_step_tables[row][(counter & 0xcu) >> 1] + 1u;
    case 15:
        return 2u;
    default:
        return eg_step_tables[row][(counter >> slot->eg_shift) & 7u];
    }
}

static void start_envelope(struct cogmaster__opll_slot *slot, const struct cogmaster__opll_patch *p)
{
    if (opll_min(15u, p->ar + (slot->rks >> 2)) == 15u) {
        slot->eg_state = OPLL_DECAY;
        slot->eg_out = 0u;
    } else {
        slot->eg_state = OPLL_ATTACK;
    }
    opll_request_update(slot, OPLL_UPDATE_EG);
}

/* One envelope step. The counter is the low 16 bits of the global envelope counter, as in
   the original. buddy is the other slot of the voice, or NULL. */
static void calc_envelope(struct cogmaster__opll_slot *slot, struct cogmaster__opll_slot *buddy,
                          const struct cogmaster__opll_patch *p, uint32_t counter, uint32_t test)
{
    const uint32_t mask = (1u << slot->eg_shift) - 1u;

    /* Nothing to do while sustaining or releasing at a fixed level. */
    if (!test && (slot->eg_state == OPLL_SUSTAIN || slot->eg_state == OPLL_RELEASE) &&
        (slot->eg_out >= OPLL_EG_MUTE || slot->eg_rate_h == 0u)) {
        return;
    }

    if (slot->eg_state == OPLL_ATTACK) {
        if (0u < slot->eg_out && 0u < slot->eg_rate_h && (counter & mask & ~3u) == 0u) {
            const uint32_t s = lookup_attack_step(slot, counter);
            if (0u < s) {
                slot->eg_out = (uint8_t)(slot->eg_out - (slot->eg_out >> s) - 1u);
            }
        }
    } else if (slot->eg_rate_h > 0u && (counter & mask) == 0u) {
        slot->eg_out =
            (uint8_t)opll_min(OPLL_EG_MUTE, slot->eg_out + lookup_decay_step(slot, counter));
    }

    switch (slot->eg_state) {
    case OPLL_DAMP:
        /* Damp becomes attack at the largest attenuation short of mute, in step with the
           envelope. */
        if (slot->eg_out >= EG_MAX && (counter & mask) == 0u) {
            start_envelope(slot, p);
            if (slot->type & 1u) {
                if (!slot->pg_keep) {
                    slot->pg_phase = 0u;
                }
                if (buddy && !buddy->pg_keep) {
                    buddy->pg_phase = 0u;
                }
            }
        }
        break;
    case OPLL_ATTACK:
        if (slot->eg_out == 0u) {
            slot->eg_state = OPLL_DECAY;
            opll_request_update(slot, OPLL_UPDATE_EG);
        }
        break;
    case OPLL_DECAY:
        /* Decay becomes sustain every sample, whatever the rate or counter. */
        if ((slot->eg_out >> 3) == p->sl) {
            slot->eg_state = OPLL_SUSTAIN;
            opll_request_update(slot, OPLL_UPDATE_EG);
        }
        break;
    default:
        break;
    }

    if (test) {
        slot->eg_out = 0u;
    }
}

static void update_slots(struct cogmaster__opll *o)
{
    o->eg_counter++;
    for (uint32_t i = 0u; i < COGMASTER__OPLL_SLOTS; i++) {
        struct cogmaster__opll_slot *slot = &o->slot[i];
        const struct cogmaster__opll_patch *p = &o->patch[slot->patch];
        struct cogmaster__opll_slot *buddy = NULL;

        if (slot->type == 0u) {
            buddy = &o->slot[i + 1u];
        }
        if (slot->type == 1u) {
            buddy = &o->slot[i - 1u];
        }
        if (slot->update_requests) {
            commit_slot_update(o, slot);
        }
        calc_envelope(slot, buddy, p, o->eg_counter & 0xffffu, o->test_flag & 1u);
        calc_phase(slot, p, o->pm_phase, o->test_flag & 4u);
        o->slot_evals++;
    }
}

/* The envelope-attenuated exponential of a log-sine value, -4086 to 4084. */
static int32_t lookup_exp_table(uint32_t i)
{
    const int32_t t = (int32_t)exp_table[(i & 0xffu) ^ 0xffu] + 1024;
    const int32_t res = t >> ((i & 0x7f00u) >> 8);

    return ((i & 0x8000u) ? ~res : res) * 2;
}

static int32_t to_linear(uint32_t h, const struct cogmaster__opll_slot *slot, uint32_t am)
{
    if (slot->eg_out > EG_MAX) {
        return 0;
    }
    return lookup_exp_table(h + (opll_min(OPLL_EG_MUTE, slot->eg_out + slot->tll + am) << 4));
}

static uint32_t wave(const struct cogmaster__opll_slot *slot, uint32_t index)
{
    return slot->ws ? halfsin_table[index] : fullsin_table[index];
}

static int32_t calc_slot_car(struct cogmaster__opll *o, uint32_t ch, int32_t fm)
{
    struct cogmaster__opll_slot *slot = opll_car(o, ch);
    const uint32_t am = o->patch[slot->patch].am ? o->lfo_am : 0u;
    const uint32_t index = (slot->pg_out + (uint32_t)(2 * opll_floor_shift(fm, 1))) & PG_MASK;

    slot->output[1] = slot->output[0];
    slot->output[0] = (int16_t)to_linear(wave(slot, index), slot, am);
    return slot->output[0];
}

static int32_t calc_slot_mod(struct cogmaster__opll *o, uint32_t ch)
{
    struct cogmaster__opll_slot *slot = opll_mod(o, ch);
    const struct cogmaster__opll_patch *p = &o->patch[slot->patch];
    const uint32_t am = p->am ? o->lfo_am : 0u;
    const int32_t fm =
        p->fb > 0u ? opll_floor_shift(slot->output[1] + slot->output[0], 9u - p->fb) : 0;
    const uint32_t index = (slot->pg_out + (uint32_t)fm) & PG_MASK;

    slot->output[1] = slot->output[0];
    slot->output[0] = (int16_t)to_linear(wave(slot, index), slot, am);
    return slot->output[0];
}

static int32_t calc_slot_tom(struct cogmaster__opll *o)
{
    const struct cogmaster__opll_slot *slot = opll_mod(o, 8u);

    return to_linear(wave(slot, slot->pg_out), slot, 0u);
}

static int32_t calc_slot_snare(struct cogmaster__opll *o)
{
    const struct cogmaster__opll_slot *slot = opll_car(o, 7u);
    const uint32_t noise = o->noise & 1u;
    uint32_t phase;

    if ((slot->pg_out >> 8) & 1u) {
        phase = noise ? 0x300u : 0x200u;
    } else {
        phase = noise ? 0x0u : 0x100u;
    }
    return to_linear(wave(slot, phase), slot, 0u);
}

static int32_t calc_slot_cym(struct cogmaster__opll *o)
{
    const struct cogmaster__opll_slot *slot = opll_car(o, 8u);

    return to_linear(wave(slot, o->short_noise ? 0x300u : 0x100u), slot, 0u);
}

static int32_t calc_slot_hat(struct cogmaster__opll *o)
{
    const struct cogmaster__opll_slot *slot = opll_mod(o, 7u);
    const uint32_t noise = o->noise & 1u;
    uint32_t phase;

    if (o->short_noise) {
        phase = noise ? 0x2d0u : 0x234u;
    } else {
        phase = noise ? 0x34u : 0xd0u;
    }
    return to_linear(wave(slot, phase), slot, 0u);
}

/* A tone channel is the carrier's output, inverted and halved; a rhythm channel is as is. */
static int16_t tone_out(int32_t x)
{
    return (int16_t)opll_floor_shift(-x, 1);
}

/* Channel ch as a melodic voice: its modulator drives its carrier. */
static int16_t voice_out(struct cogmaster__opll *o, uint32_t ch)
{
    const int32_t fm = calc_slot_mod(o, ch);

    return tone_out(calc_slot_car(o, ch, fm));
}

int32_t cogmaster__opll_sample(struct cogmaster__opll *o)
{
    int16_t *out = o->ch_out;
    int32_t sum = 0;

    update_ampm(o);
    update_short_noise(o);
    update_slots(o);

    for (uint32_t i = 0u; i < 6u; i++) {
        out[i] = voice_out(o, i);
    }
    if (!o->rhythm_mode) {
        out[6] = voice_out(o, 6u);
    } else {
        const int32_t fm = calc_slot_mod(o, 6u);
        out[9] = (int16_t)calc_slot_car(o, 6u, fm);
    }
    update_noise(o, 14u);
    if (!o->rhythm_mode) {
        out[7] = voice_out(o, 7u);
    } else {
        out[10] = (int16_t)calc_slot_hat(o);
        out[11] = (int16_t)calc_slot_snare(o);
    }
    update_noise(o, 2u);
    if (!o->rhythm_mode) {
        out[8] = voice_out(o, 8u);
    } else {
        out[12] = (int16_t)calc_slot_tom(o);
        out[13] = (int16_t)calc_slot_cym(o);
    }
    update_noise(o, 2u);

    for (uint32_t i = 0u; i < COGMASTER__OPLL_CHANNELS; i++) {
        sum += out[i];
    }
    o->sum = sum;
    return sum;
}
