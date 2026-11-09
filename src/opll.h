/* SPDX-License-Identifier: MIT */
/* The Yamaha YM2413 (OPLL), nine melodic voices or six and five rhythm
   sounds, two operator slots per voice (SMS-AUDIO section 4).
   Upstream: https://github.com/digital-sound-antiques/emu2413, emu2413.h at
   commit a1d5120903b75f93d3eb7690e34a29c0b4add903.
   Copyright (C) 2026 Mitsutaka Okazaki (as the upstream source header states it)
   and Copyright (c) 2001-2019 Mitsutaka Okazaki (as its LICENSE states it).
   Changes: integer-only C17 port that holds all state in one structure and
   keeps only the YM2413 instrument set; see THIRD-PARTY-NOTICES.md. */
#ifndef COGMASTER_OPLL_H
#define COGMASTER_OPLL_H

#include <stdint.h>

#define COGMASTER__OPLL_SLOTS 18u
#define COGMASTER__OPLL_PATCHES 38u
#define COGMASTER__OPLL_CHANNELS 14u

/* An instrument: the eight bytes of a patch dump, split into fields. */
struct cogmaster__opll_patch {
    uint8_t tl, fb, eg, ml, ar, dr, sl, rr, kr, kl, am, pm, ws;
};

/* One operator slot: even slots are modulators, odd slots carriers. */
struct cogmaster__opll_slot {
    uint8_t type;      /* bit 0: carrier; bit 1: single slot (snare, tom, hi-hat, cymbal) */
    uint8_t patch;     /* index into the chip's patches */
    uint8_t ws;        /* wave table: 0 full sine, 1 half sine */
    int16_t output[2]; /* last two outputs */
    uint32_t pg_phase; /* phase generator accumulator, 19 bits */
    uint16_t pg_out;   /* its output, an index into the wave table */
    uint8_t pg_keep;   /* 1: the phase survives a key-on */
    uint16_t blk_fnum; /* block (3 bits) over f-number (9 bits) */
    uint16_t fnum;
    uint8_t blk;
    uint8_t eg_state; /* attack, decay, sustain, release or damp */
    uint8_t volume;   /* carrier volume, 6 bits */
    uint8_t key_flag;
    uint8_t sus_flag;
    uint16_t tll; /* total level plus key scale level */
    uint8_t rks;
    uint8_t eg_rate_h; /* envelope speed: high 4 bits and low 2 bits */
    uint8_t eg_rate_l;
    uint8_t eg_shift;
    uint8_t eg_out;
    uint8_t update_requests; /* derived values to recompute at the next sample */
};

struct cogmaster__opll {
    uint8_t reg[0x40];
    uint8_t test_flag;
    uint8_t rhythm_mode;
    uint8_t lfo_am;
    uint8_t short_noise;
    uint32_t slot_key_status;
    uint32_t eg_counter;
    uint32_t pm_phase;
    uint32_t am_phase; /* kept below 13,440, which is 210 steps of 64 */
    uint32_t noise;
    int32_t sum;         /* the last sample returned */
    uint64_t slot_evals; /* slots evaluated since reset: 18 for every sample */
    uint8_t patch_number[9];
    int16_t ch_out[COGMASTER__OPLL_CHANNELS]; /* 0-8 tone, 9 bass drum, 10 hi-hat, 11 snare,
                                                 12 tom, 13 cymbal */
    struct cogmaster__opll_slot slot[COGMASTER__OPLL_SLOTS];
    struct cogmaster__opll_patch patch[COGMASTER__OPLL_PATCHES];
};

enum { OPLL_ATTACK, OPLL_DECAY, OPLL_SUSTAIN, OPLL_RELEASE, OPLL_DAMP };

/* Derived values a write or a state change asks the next sample to recompute. */
enum {
    OPLL_UPDATE_WS = 1,
    OPLL_UPDATE_TLL = 2,
    OPLL_UPDATE_RKS = 4,
    OPLL_UPDATE_EG = 8,
    OPLL_UPDATE_ALL = 255
};

#define OPLL_EG_MUTE 127u

#define OPLL_SLOT_BD1 12u
#define OPLL_SLOT_HH 14u
#define OPLL_SLOT_SD 15u
#define OPLL_SLOT_TOM 16u
#define OPLL_SLOT_CYM 17u

/* The modulator and the carrier slot of voice ch. */
static inline struct cogmaster__opll_slot *opll_mod(struct cogmaster__opll *o, uint32_t ch)
{
    return &o->slot[ch << 1];
}

static inline struct cogmaster__opll_slot *opll_car(struct cogmaster__opll *o, uint32_t ch)
{
    return &o->slot[(ch << 1) | 1u];
}

static inline void opll_request_update(struct cogmaster__opll_slot *slot, uint32_t flags)
{
    slot->update_requests = (uint8_t)(slot->update_requests | flags);
}

/* Power-on state: every register zero, the instrument ROM loaded. */
void cogmaster__opll_reset(struct cogmaster__opll *o);

/* A write to register reg (0 to 0x3F, mirrors folded in) with the low byte of v. */
void cogmaster__opll_write(struct cogmaster__opll *o, uint32_t reg, uint32_t v);

/* One sample, 72 Z80 clocks of chip time: the sum of all fourteen channels. */
int32_t cogmaster__opll_sample(struct cogmaster__opll *o);

#endif /* COGMASTER_OPLL_H */
