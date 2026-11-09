/* SPDX-License-Identifier: MIT */
/* The YM2413's registers: what a write does, power-on, and the instrument ROM.
   Upstream: https://github.com/digital-sound-antiques/emu2413, emu2413.c at
   commit a1d5120903b75f93d3eb7690e34a29c0b4add903.
   Copyright (C) 2026 Mitsutaka Okazaki (as the upstream source header states it)
   and Copyright (c) 2001-2019 Mitsutaka Okazaki (as its LICENSE states it).
   Changes: integer-only C17 port; the rate converter, pan, channel mask, alternative
   instrument sets, allocation and memory-image save are removed. */
#include <string.h>

#include "opll.h"

#define OPLL_WANT_VOICES
#include "opll_tables.inc"

static uint8_t bits(uint32_t v, uint32_t shift, uint32_t mask)
{
    return (uint8_t)((v >> shift) & mask);
}

/* Splits an eight-byte instrument dump into the modulator and carrier patches. */
static void dump_to_patch(const uint8_t *dump, struct cogmaster__opll_patch *patch)
{
    for (uint32_t i = 0u; i < 2u; i++) {
        patch[i].am = bits(dump[i], 7, 1u);
        patch[i].pm = bits(dump[i], 6, 1u);
        patch[i].eg = bits(dump[i], 5, 1u);
        patch[i].kr = bits(dump[i], 4, 1u);
        patch[i].ml = bits(dump[i], 0, 15u);
        patch[i].ar = bits(dump[4u + i], 4, 15u);
        patch[i].dr = bits(dump[4u + i], 0, 15u);
        patch[i].sl = bits(dump[6u + i], 4, 15u);
        patch[i].rr = bits(dump[6u + i], 0, 15u);
    }
    patch[0].kl = bits(dump[2], 6, 3u);
    patch[1].kl = bits(dump[3], 6, 3u);
    patch[0].tl = bits(dump[2], 0, 63u);
    patch[1].tl = 0u;
    patch[0].fb = bits(dump[3], 0, 7u);
    patch[1].fb = 0u;
    patch[0].ws = bits(dump[3], 3, 1u);
    patch[1].ws = bits(dump[3], 4, 1u);
}

static void reset_slot(struct cogmaster__opll_slot *slot, uint32_t number)
{
    memset(slot, 0, sizeof *slot);
    slot->type = (uint8_t)(number & 1u);
    slot->eg_state = OPLL_RELEASE;
    slot->eg_out = OPLL_EG_MUTE;
}

static void slot_on(struct cogmaster__opll *o, uint32_t i)
{
    struct cogmaster__opll_slot *slot = &o->slot[i];
    slot->key_flag = 1u;
    slot->eg_state = OPLL_DAMP;
    opll_request_update(slot, OPLL_UPDATE_EG);
}

static void slot_off(struct cogmaster__opll *o, uint32_t i)
{
    struct cogmaster__opll_slot *slot = &o->slot[i];
    slot->key_flag = 0u;
    if (slot->type & 1u) {
        slot->eg_state = OPLL_RELEASE;
        opll_request_update(slot, OPLL_UPDATE_EG);
    }
}

/* Keys slots on and off to match registers $20-$28 and the rhythm key bits in $0E. */
static void update_key_status(struct cogmaster__opll *o)
{
    const uint32_t r14 = o->reg[0x0e];
    uint32_t status = 0u;

    for (uint32_t ch = 0u; ch < 9u; ch++) {
        if (o->reg[0x20u + ch] & 0x10u) {
            status |= 3u << (ch * 2u);
        }
    }
    if (r14 & 0x20u) {
        status |= (r14 & 0x10u) ? 3u << OPLL_SLOT_BD1 : 0u;
        status |= (r14 & 0x01u) ? 1u << OPLL_SLOT_HH : 0u;
        status |= (r14 & 0x08u) ? 1u << OPLL_SLOT_SD : 0u;
        status |= (r14 & 0x04u) ? 1u << OPLL_SLOT_TOM : 0u;
        status |= (r14 & 0x02u) ? 1u << OPLL_SLOT_CYM : 0u;
    }
    for (uint32_t i = 0u; i < COGMASTER__OPLL_SLOTS; i++) {
        if (((o->slot_key_status ^ status) >> i) & 1u) {
            if ((status >> i) & 1u) {
                slot_on(o, i);
            } else {
                slot_off(o, i);
            }
        }
    }
    o->slot_key_status = status;
}

static void set_patch(struct cogmaster__opll *o, uint32_t ch, uint32_t num)
{
    o->patch_number[ch] = (uint8_t)num;
    opll_mod(o, ch)->patch = (uint8_t)(num * 2u);
    opll_car(o, ch)->patch = (uint8_t)(num * 2u + 1u);
    opll_request_update(opll_mod(o, ch), OPLL_UPDATE_ALL);
    opll_request_update(opll_car(o, ch), OPLL_UPDATE_ALL);
}

static void set_slot_volume(struct cogmaster__opll_slot *slot, uint32_t volume)
{
    slot->volume = (uint8_t)volume;
    opll_request_update(slot, OPLL_UPDATE_TLL);
}

/* Sets the f-number and block of both slots of voice ch and asks for their derived values. */
static void set_pitch(struct cogmaster__opll *o, uint32_t ch, uint32_t fnum, uint32_t blk)
{
    struct cogmaster__opll_slot *pair[2] = {opll_mod(o, ch), opll_car(o, ch)};
    for (uint32_t i = 0u; i < 2u; i++) {
        pair[i]->fnum = (uint16_t)fnum;
        pair[i]->blk = (uint8_t)blk;
        pair[i]->blk_fnum = (uint16_t)(((blk & 7u) << 9) | (fnum & 0x1ffu));
        opll_request_update(pair[i], OPLL_UPDATE_EG | OPLL_UPDATE_RKS | OPLL_UPDATE_TLL);
    }
}

static void set_sus_flag(struct cogmaster__opll *o, uint32_t ch, uint32_t flag)
{
    opll_car(o, ch)->sus_flag = (uint8_t)flag;
    opll_request_update(opll_car(o, ch), OPLL_UPDATE_EG);
    if (opll_mod(o, ch)->type & 1u) {
        opll_mod(o, ch)->sus_flag = (uint8_t)flag;
        opll_request_update(opll_mod(o, ch), OPLL_UPDATE_EG);
    }
}

/* Switches the three shared voices between melodic and rhythm use. */
static void update_rhythm_mode(struct cogmaster__opll *o)
{
    const uint8_t mode = bits(o->reg[0x0e], 5, 1u);
    struct cogmaster__opll_slot *hh = &o->slot[OPLL_SLOT_HH];
    struct cogmaster__opll_slot *sd = &o->slot[OPLL_SLOT_SD];
    struct cogmaster__opll_slot *tom = &o->slot[OPLL_SLOT_TOM];
    struct cogmaster__opll_slot *cym = &o->slot[OPLL_SLOT_CYM];

    if (o->rhythm_mode != mode) {
        hh->type = mode ? 3u : 0u;
        hh->pg_keep = mode;
        sd->type = mode ? 3u : 1u;
        tom->type = mode ? 3u : 0u;
        cym->type = mode ? 3u : 1u;
        cym->pg_keep = mode;
        if (mode) {
            set_patch(o, 6u, 16u);
            set_patch(o, 7u, 17u);
            set_patch(o, 8u, 18u);
            set_slot_volume(hh, (uint32_t)bits(o->reg[0x37], 4, 15u) << 2);
            set_slot_volume(tom, (uint32_t)bits(o->reg[0x38], 4, 15u) << 2);
        } else {
            set_patch(o, 6u, o->reg[0x36] >> 4);
            set_patch(o, 7u, o->reg[0x37] >> 4);
            set_patch(o, 8u, o->reg[0x38] >> 4);
        }
    }
    o->rhythm_mode = mode;
}

/* Asks every voice that plays the user instrument to recompute from its modulator
   (parity 0) or carrier (parity 1) patch. */
static void request_user(struct cogmaster__opll *o, uint32_t parity, uint32_t flags)
{
    for (uint32_t i = 0u; i < 9u; i++) {
        if (o->patch_number[i] == 0u) {
            opll_request_update(&o->slot[(i << 1) | parity], flags);
        }
    }
}

/* Registers $00-$07: the user instrument. */
static void write_user_patch(struct cogmaster__opll *o, uint32_t reg, uint32_t data)
{
    struct cogmaster__opll_patch *p = &o->patch[reg & 1u];
    struct cogmaster__opll_patch *m = &o->patch[0];
    struct cogmaster__opll_patch *c = &o->patch[1];

    if (reg < 2u) {
        p->am = bits(data, 7, 1u);
        p->pm = bits(data, 6, 1u);
        p->eg = bits(data, 5, 1u);
        p->kr = bits(data, 4, 1u);
        p->ml = bits(data, 0, 15u);
        request_user(o, reg & 1u, OPLL_UPDATE_RKS | OPLL_UPDATE_EG);
    } else if (reg == 2u) {
        m->kl = bits(data, 6, 3u);
        m->tl = bits(data, 0, 63u);
        request_user(o, 0u, OPLL_UPDATE_TLL);
    } else if (reg == 3u) {
        c->kl = bits(data, 6, 3u);
        c->ws = bits(data, 4, 1u);
        m->ws = bits(data, 3, 1u);
        m->fb = bits(data, 0, 7u);
        request_user(o, 0u, OPLL_UPDATE_WS);
        request_user(o, 1u, OPLL_UPDATE_WS | OPLL_UPDATE_TLL);
    } else if (reg < 6u) {
        p->ar = bits(data, 4, 15u);
        p->dr = bits(data, 0, 15u);
        request_user(o, reg & 1u, OPLL_UPDATE_EG);
    } else {
        p->sl = bits(data, 4, 15u);
        p->rr = bits(data, 0, 15u);
        request_user(o, reg & 1u, OPLL_UPDATE_EG);
    }
}

/* Registers $30-$38: instrument and volume, or the rhythm volumes for voices 7 to 9. */
static void write_voice(struct cogmaster__opll *o, uint32_t reg, uint32_t data)
{
    const uint32_t ch = reg - 0x30u;

    if ((o->reg[0x0e] & 32u) && reg >= 0x36u) {
        if (reg == 0x37u) {
            set_slot_volume(opll_mod(o, 7u), (uint32_t)bits(data, 4, 15u) << 2);
        } else if (reg == 0x38u) {
            set_slot_volume(opll_mod(o, 8u), (uint32_t)bits(data, 4, 15u) << 2);
        }
    } else {
        set_patch(o, ch, bits(data, 4, 15u));
    }
    opll_car(o, ch)->volume = (uint8_t)((data & 15u) << 2);
    opll_request_update(opll_car(o, ch), OPLL_UPDATE_TLL);
}

void cogmaster__opll_write(struct cogmaster__opll *o, uint32_t reg, uint32_t v)
{
    const uint32_t data = v & 0xffu;

    if (reg >= 0x40u) {
        return;
    }
    /* $19-$1F, $29-$2F and $39-$3F mirror $10-$16, $20-$26 and $30-$36. */
    if ((reg >= 0x19u && reg <= 0x1fu) || (reg >= 0x29u && reg <= 0x2fu) ||
        (reg >= 0x39u && reg <= 0x3fu)) {
        reg -= 9u;
    }
    o->reg[reg] = (uint8_t)data;

    if (reg < 8u) {
        write_user_patch(o, reg, data);
    } else if (reg == 0x0eu) {
        update_rhythm_mode(o);
        update_key_status(o);
    } else if (reg == 0x0fu) {
        o->test_flag = (uint8_t)data;
    } else if (reg >= 0x10u && reg <= 0x18u) {
        const uint32_t ch = reg - 0x10u;
        set_pitch(o, ch, data + ((o->reg[0x20u + ch] & 1u) << 8), opll_car(o, ch)->blk);
    } else if (reg >= 0x20u && reg <= 0x28u) {
        const uint32_t ch = reg - 0x20u;
        set_pitch(o, ch, ((data & 1u) << 8) + o->reg[0x10u + ch], bits(data, 1, 7u));
        set_sus_flag(o, ch, bits(data, 5, 1u));
        update_key_status(o);
    } else if (reg >= 0x30u && reg <= 0x38u) {
        write_voice(o, reg, data);
    }
}

void cogmaster__opll_reset(struct cogmaster__opll *o)
{
    memset(o, 0, sizeof *o);
    for (uint32_t i = 0u; i < 19u; i++) {
        dump_to_patch(&default_inst[i * 8u], &o->patch[i * 2u]);
    }
    o->noise = 1u;
    for (uint32_t i = 0u; i < COGMASTER__OPLL_SLOTS; i++) {
        reset_slot(&o->slot[i], i);
    }
    for (uint32_t ch = 0u; ch < 9u; ch++) {
        set_patch(o, ch, 0u);
    }
    for (uint32_t reg = 0u; reg < 0x40u; reg++) {
        cogmaster__opll_write(o, reg, 0u);
    }
}
