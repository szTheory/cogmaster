/* The Sega PSG, the SN76489 variant inside the VDP: three square-wave tone
   channels and a noise channel (SMS-AUDIO section 2). The chip steps once
   every 16 Z80 clocks, which is 48 bus ticks. It is stepped lazily: nothing
   runs until a write or the end of a frame asks for the chip at a tick, and
   then every step at a multiple of 48 up to that tick runs, in order. A level
   a channel presents changes only on a step or a write; each change is handed
   to the synthesiser stamped at the tick it happened. */
#include <string.h>

#include "internal.h"

#define PSG_STEP_TICKS 48u
#define PSG_LFSR_RESET 0x8000u
#define PSG_LATCH_POWER 3u
#define PSG_ATT_OFF 15u
#define PSG_NOISE_MODE_TONE2 3u

/* Power-on: every attenuation off, periods and noise control 0, the shift
   register at its reset value, counters and flip-flops clear, register 3 latched
   and the step origin at tick 0, which is where the tick count restarts at a
   load and at both kinds of reset (SMS-AUDIO section 2.1). */
void cogmaster__psg_reset(cogmaster *m)
{
    struct cogmaster__psg *p = &m->psg;
    memset(p, 0, sizeof *p);
    for (uint32_t ch = 0u; ch < 4u; ch++) {
        p->att[ch] = PSG_ATT_OFF;
    }
    p->lfsr = PSG_LFSR_RESET;
    p->latch = PSG_LATCH_POWER;
}

/* What channel ch presents: bit 4 is its output, bits 3 to 0 its attenuation.
   A tone channel whose period is 0 or 1 holds its output high; the integrated
   chip does this and games play 4-bit samples through it (see SMS-AUDIO
   section 2.2). Its flip-flop keeps counting underneath. */
uint32_t cogmaster__psg_level(const cogmaster *m, uint32_t ch)
{
    const struct cogmaster__psg *p = &m->psg;
    uint32_t out = p->out[ch];
    if (ch < 3u && p->period[ch] < 2u) {
        out = 1u;
    }
    return out << 4 | p->att[ch];
}

/* Hands every channel whose level differs from before to the synthesiser. */
static void psg_emit(cogmaster *m, uint64_t tick, const uint32_t before[4])
{
    for (uint32_t ch = 0u; ch < 4u; ch++) {
        uint32_t now = cogmaster__psg_level(m, ch);
        if (now != before[ch]) {
            cogmaster__synth_level(m, tick, ch, now);
        }
    }
}

static void psg_levels(const cogmaster *m, uint32_t levels[4])
{
    for (uint32_t ch = 0u; ch < 4u; ch++) {
        levels[ch] = cogmaster__psg_level(m, ch);
    }
}

/* One step. A tone counter that has reached 0 reloads its period and toggles
   its flip-flop, so the first step toggles every flip-flop from 0 to 1 and a
   period N toggles every N steps. The noise register shifts on the rising edge
   of the noise divider: every 32, 64 or 128 steps for controls 0 to 2, and
   with tone 2's rising edge for control 3, where tone 2 at period 0 counts as
   1. The channel's output is the bit about to be shifted out; white noise
   feeds bits 0 and 3 back into bit 15, periodic noise bit 0 only
   (SMS-AUDIO section 2.4). */
static void psg_step(struct cogmaster__psg *p)
{
    uint32_t rise2 = 0u;
    for (uint32_t ch = 0u; ch < 3u; ch++) {
        if (p->counter[ch] > 0u) {
            p->counter[ch]--;
        }
        if (p->counter[ch] == 0u) {
            p->counter[ch] = p->period[ch];
            p->out[ch] ^= 1u;
            if (ch == 2u) {
                rise2 = p->out[2];
            }
        }
    }
    uint32_t mode = p->noise & 3u;
    uint32_t shift = 0u;
    if (mode == PSG_NOISE_MODE_TONE2) {
        shift = rise2;
    } else {
        if (p->counter[3] > 0u) {
            p->counter[3]--;
        }
        if (p->counter[3] == 0u) {
            p->counter[3] = (uint16_t)(16u << mode);
            p->noise_div ^= 1u;
            shift = p->noise_div;
        }
    }
    if (shift != 0u) {
        uint32_t bit = p->lfsr & 1u;
        uint32_t feedback = (p->noise & 4u) != 0u ? (bit ^ (p->lfsr >> 3)) & 1u : bit;
        p->out[3] = (uint8_t)bit;
        p->lfsr = (uint16_t)(p->lfsr >> 1 | feedback << 15);
    }
}

/* Runs every step at a tick 48k with the last step before it and tick not
   before it. A write stamped exactly 48k therefore sees that step first. */
void cogmaster__psg_catch_up(cogmaster *m, uint64_t tick)
{
    struct cogmaster__psg *p = &m->psg;
    while (p->at + PSG_STEP_TICKS <= tick) {
        uint32_t before[4];
        psg_levels(m, before);
        p->at += PSG_STEP_TICKS;
        psg_step(p);
        m->synth.stats.psg_steps =
            m->synth.stats.psg_steps == UINT32_MAX ? UINT32_MAX : m->synth.stats.psg_steps + 1u;
        psg_emit(m, p->at, before);
    }
}

/* A write to the register the latch names. Register r is channel r / 2; odd
   r is the attenuation, even r the tone period or, for channel 3, the noise
   control, and writing the noise control resets the shift register (SMS-AUDIO
   section 2.1 and section 2.4). data holds the bits the byte supplies: 4 on a
   latch byte, 6 on a data byte, which only a tone period uses beyond 4. */
static void psg_register(struct cogmaster__psg *p, uint32_t data, int latch_byte)
{
    uint32_t reg = p->latch;
    uint32_t ch = reg >> 1;
    if ((reg & 1u) != 0u) {
        p->att[ch] = (uint8_t)(data & 15u);
    } else if (ch < 3u) {
        p->period[ch] = (uint16_t)(latch_byte ? (p->period[ch] & 0x3F0u) | data
                                              : (data << 4) | (p->period[ch] & 15u));
    } else {
        p->noise = (uint8_t)(data & 7u);
        p->lfsr = PSG_LFSR_RESET;
    }
}

/* The chip is brought up to tick first, then the byte applies and its level
   changes are stamped at tick itself: a volume write is heard when it is
   written, not at the next step. The bus stall a write causes on some chips is
   not modelled. */
void cogmaster__psg_write(cogmaster *m, uint64_t tick, uint8_t v)
{
    cogmaster__psg_catch_up(m, tick);
    m->synth.stats.psg_writes =
        m->synth.stats.psg_writes == UINT32_MAX ? UINT32_MAX : m->synth.stats.psg_writes + 1u;
    struct cogmaster__psg *p = &m->psg;
    uint32_t before[4];
    psg_levels(m, before);
    if ((v & 0x80u) != 0u) {
        p->latch = (uint8_t)((v >> 4) & 7u);
        psg_register(p, (uint32_t)v & 15u, 1);
    } else {
        psg_register(p, (uint32_t)v & 0x3Fu, 0);
    }
    psg_emit(m, tick, before);
}

/* Every active source's current level, through the synthesiser, at the
   current tick: the Game Gear's mask only in native mode. Load and both
   resets call it, so the synthesiser's stored levels equal the chip's. */
void cogmaster__psg_snapshot(cogmaster *m)
{
    for (uint32_t ch = 0u; ch < 4u; ch++) {
        cogmaster__synth_level(m, m->ticks, ch, cogmaster__psg_level(m, ch));
    }
    if (cogmaster__native_gg(m)) {
        cogmaster__synth_level(m, m->ticks, 4u, m->io.gg[5]);
    }
}
