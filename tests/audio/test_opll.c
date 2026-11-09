/* The YM2413 chip module on its own and on the Japanese console: cogmaster-test-fm <case>
   [file]. The chip is driven through its three entry points and read back through its public
   structure; the unit's ports, control, reset, state and cost go through the bus the Z80
   uses and the instance's own accessors. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "opll.h"
#include "opll_stream.h"
#include "sha256.h"
#include "state.h"
#include "../check.h"

#define LOCKSTEP_SAMPLES 2000000u

/* The port's output over the seeded stream, hashed as the original's int16 samples are:
   two bytes per sample, low byte first. */
static void port_stream_digest(uint32_t n, char hex[65])
{
    static struct cogmaster__opll chip;
    struct opll_stream st;
    cogmaster__sha256 h;
    uint8_t digest[32];
    uint32_t reg;
    uint32_t val;

    cogmaster__opll_reset(&chip);
    opll_stream_init(&st, OPLL_STREAM_SEED);
    cogmaster__sha256_init(&h);
    for (uint32_t i = 0; i < n; i++) {
        if (opll_stream_step(&st, &reg, &val)) {
            cogmaster__opll_write(&chip, reg, val);
        }
        const uint16_t s = (uint16_t)cogmaster__opll_sample(&chip);
        const uint8_t b[2] = {(uint8_t)(s & 0xffu), (uint8_t)(s >> 8)};
        cogmaster__sha256_update(&h, b, 2);
    }
    cogmaster__sha256_final(&h, digest);
    cogmaster__sha256_hex(digest, hex);
}

/* The port over the seeded register stream equals the digest the unmodified original
   produced once; the file names the original's source by its SHA-256. */
static void lockstep(const char *path)
{
    char line[512];
    char frozen[65];
    char hex[65];
    unsigned samples = 0;
    unsigned long long seed = 0;
    FILE *f = fopen(path, "r");

    if (f == NULL || fgets(line, sizeof line, f) == NULL) {
        fprintf(stderr, "lockstep: cannot read %s\n", path);
        check_failures++;
        if (f != NULL) {
            fclose(f);
        }
        return;
    }
    fclose(f);
    CHECK(sscanf(line, "samples %u seed 0x%llx sha256 %64s", &samples, &seed, frozen) == 3);
    CHECK_EQ_U64(samples, LOCKSTEP_SAMPLES);
    CHECK_EQ_HEX(seed, OPLL_STREAM_SEED);
    port_stream_digest(LOCKSTEP_SAMPLES, hex);
    if (strcmp(hex, frozen) != 0) {
        fprintf(stderr, "lockstep: port digest %s, frozen digest %s\n", hex, frozen);
        check_failures++;
    }
}

static struct cogmaster__opll chip;
static struct cogmaster__opll twin;

static void put(struct cogmaster__opll *o, uint32_t reg, uint32_t v)
{
    cogmaster__opll_write(o, reg, v);
}

static void run(struct cogmaster__opll *o, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        (void)cogmaster__opll_sample(o);
    }
}

/* Register $2n with the key on, block 4 and f-number bit 8 set: with $1n = $80 the f-number
   is $180. */
#define KEY_ON_B4 (0x10u | (4u << 1) | 1u)
#define KEY_OFF_B4 ((4u << 1) | 1u)

/* A key-on starts the carrier in its damp state; within two samples the damp is over and
   the attack runs, the voice becomes audible, and a key-off releases it to silence. */
static void key_on(void)
{
    uint32_t audible = 0;

    cogmaster__opll_reset(&chip);
    put(&chip, 0x30, 0x10); /* instrument 1, volume 0 */
    put(&chip, 0x10, 0x80);
    for (uint32_t i = 0; i < 8; i++) {
        CHECK_EQ_U64(cogmaster__opll_sample(&chip), 0);
    }
    put(&chip, 0x20, KEY_ON_B4);
    (void)cogmaster__opll_sample(&chip);
    CHECK_EQ_U64(opll_car(&chip, 0)->eg_state, OPLL_DAMP);
    (void)cogmaster__opll_sample(&chip);
    CHECK(opll_car(&chip, 0)->eg_state != OPLL_DAMP);
    for (uint32_t i = 0; i < 128 && !audible; i++) {
        audible = cogmaster__opll_sample(&chip) != 0;
    }
    CHECK(audible);

    put(&chip, 0x20, KEY_OFF_B4);
    (void)cogmaster__opll_sample(&chip);
    CHECK_EQ_U64(opll_car(&chip, 0)->eg_state, OPLL_RELEASE);
    audible = 0;
    for (uint32_t i = 0; i < 100; i++) {
        audible |= cogmaster__opll_sample(&chip) != 0;
    }
    CHECK(audible); /* a decay, not a cut */
    run(&chip, 20000);
    CHECK_EQ_U64(opll_car(&chip, 0)->eg_out, OPLL_EG_MUTE);
    for (uint32_t i = 0; i < 1000; i++) {
        CHECK_EQ_U64(cogmaster__opll_sample(&chip), 0);
    }
}

/* Each of the five rhythm keys in $0E sounds its own drum channel and no other. */
static void rhythm(void)
{
    static const struct {
        uint32_t key;
        uint32_t channel;
    } drums[5] = {{0x10, 9}, {0x08, 11}, {0x04, 12}, {0x02, 13}, {0x01, 10}};

    for (uint32_t d = 0; d < 5; d++) {
        uint32_t heard[COGMASTER__OPLL_CHANNELS] = {0};

        cogmaster__opll_reset(&chip);
        put(&chip, 0x16, 0x20);
        put(&chip, 0x26, 0x05);
        put(&chip, 0x17, 0x50);
        put(&chip, 0x27, 0x05);
        put(&chip, 0x18, 0xc0);
        put(&chip, 0x28, 0x01);
        put(&chip, 0x0e, 0x20); /* rhythm mode, every drum off */
        CHECK_EQ_U64(chip.rhythm_mode, 1);
        for (uint32_t i = 0; i < 100; i++) {
            (void)cogmaster__opll_sample(&chip);
            for (uint32_t c = 0; c < COGMASTER__OPLL_CHANNELS; c++) {
                CHECK_EQ_U64(chip.ch_out[c], 0);
            }
        }
        put(&chip, 0x0e, 0x20 | drums[d].key);
        for (uint32_t i = 0; i < 5000; i++) {
            (void)cogmaster__opll_sample(&chip);
            for (uint32_t c = 0; c < COGMASTER__OPLL_CHANNELS; c++) {
                heard[c] += chip.ch_out[c] != 0;
            }
        }
        for (uint32_t c = 0; c < COGMASTER__OPLL_CHANNELS; c++) {
            if (c == drums[d].channel) {
                CHECK(heard[c] > 0);
            } else {
                CHECK_EQ_U64(heard[c], 0);
            }
        }
    }
}

/* The noise generator is a 23-bit linear feedback shift register with taps at bits 0 and 9
   that steps 18 times per sample, whether or not rhythm mode is on. */
static uint32_t noise_after_sample(uint32_t n)
{
    for (uint32_t i = 0; i < 18; i++) {
        n = (n & 1u) ? (n ^ 0x800200u) >> 1 : n >> 1;
    }
    return n;
}

static void noise(void)
{
    for (uint32_t mode = 0; mode < 2; mode++) {
        uint32_t want = 1;

        cogmaster__opll_reset(&chip);
        CHECK_EQ_U64(chip.noise, 1);
        put(&chip, 0x0e, mode ? 0x20 : 0x00);
        for (uint32_t i = 0; i < 20000; i++) {
            const uint32_t before = chip.noise;
            (void)cogmaster__opll_sample(&chip);
            want = noise_after_sample(want);
            CHECK_EQ_U64(chip.noise, want);
            CHECK(chip.noise != before);
            CHECK(chip.noise != 0 && chip.noise < (1u << 23));
        }
    }
}

/* The amplitude LFO is a triangle of 0 to 13 held 64 samples per step with 13,440 samples
   to the period, and the pitch LFO counts one per sample, eight steps of 1,024 samples. */
static uint32_t am_level(uint32_t phase)
{
    const uint32_t step = phase >> 6;

    if (step < 104) {
        return step >> 3;
    }
    return step < 107 ? 13 : 12 - ((step - 107) >> 3);
}

static void lfo(void)
{
    cogmaster__opll_reset(&chip);
    CHECK_EQ_U64(chip.pm_phase, 0);
    CHECK_EQ_U64(chip.am_phase, 0);
    for (uint32_t n = 1; n <= 2 * 13440; n++) {
        (void)cogmaster__opll_sample(&chip);
        CHECK_EQ_U64(chip.pm_phase, n);
        CHECK_EQ_U64(chip.am_phase, n % 13440);
        CHECK_EQ_U64(chip.lfo_am, am_level(n % 13440));
    }

    /* The pitch LFO moves the phase of a slot whose patch has the pitch bit: with f-number
       $180 the offsets are 0 for the first 1,023 samples and 3 for the next 1,024, and a
       phase step is 8 per unit of offset. */
    cogmaster__opll_reset(&chip);
    cogmaster__opll_reset(&twin);
    for (uint32_t i = 0; i < 2; i++) {
        struct cogmaster__opll *o = i ? &twin : &chip;
        put(o, 0x00, 0x01 | (i ? 0x40u : 0u)); /* multiple 1, pitch LFO on in twin */
        put(o, 0x10, 0x80);
        put(o, 0x20, KEY_OFF_B4);
        run(o, 2047);
    }
    CHECK_EQ_U64(opll_mod(&chip, 0)->pg_phase, (2047u * 0x1800u) & 0x7ffffu);
    CHECK_EQ_U64((opll_mod(&twin, 0)->pg_phase - opll_mod(&chip, 0)->pg_phase) & 0x7ffffu,
                 1024u * 3u * 8u);
}

/* The test register: bit 0 holds every envelope at full level, bit 1 holds both LFOs at
   zero, bit 2 holds every phase generator at one step, bit 3 speeds the LFOs up. */
static void test_register(void)
{
    /* Bit 1. */
    cogmaster__opll_reset(&chip);
    run(&chip, 100);
    CHECK_EQ_U64(chip.pm_phase, 100);
    put(&chip, 0x0f, 0x02);
    run(&chip, 10);
    CHECK_EQ_U64(chip.pm_phase, 0);
    CHECK_EQ_U64(chip.am_phase, 0);
    put(&chip, 0x0f, 0x00);
    run(&chip, 1);
    CHECK_EQ_U64(chip.pm_phase, 1);
    CHECK_EQ_U64(chip.am_phase, 1);

    /* Bit 3. */
    cogmaster__opll_reset(&chip);
    put(&chip, 0x0f, 0x08);
    run(&chip, 3);
    CHECK_EQ_U64(chip.pm_phase, 3 * 1024);
    CHECK_EQ_U64(chip.am_phase, 3 * 64);

    /* Bit 2: the phase restarts every sample, so it stays at one step of 0x1800. */
    cogmaster__opll_reset(&chip);
    put(&chip, 0x00, 0x01);
    put(&chip, 0x10, 0x80);
    put(&chip, 0x20, KEY_OFF_B4);
    run(&chip, 5);
    CHECK_EQ_U64(opll_mod(&chip, 0)->pg_phase, 5u * 0x1800u);
    put(&chip, 0x0f, 0x04);
    run(&chip, 5);
    CHECK_EQ_U64(opll_mod(&chip, 0)->pg_phase, 0x1800u);
    put(&chip, 0x0f, 0x00);
    run(&chip, 1);
    CHECK_EQ_U64(opll_mod(&chip, 0)->pg_phase, 2u * 0x1800u);

    /* Bit 0: every envelope is at full level while it is set. */
    cogmaster__opll_reset(&chip);
    for (uint32_t i = 0; i < COGMASTER__OPLL_SLOTS; i++) {
        CHECK_EQ_U64(chip.slot[i].eg_out, OPLL_EG_MUTE);
    }
    put(&chip, 0x0f, 0x01);
    run(&chip, 1);
    for (uint32_t i = 0; i < COGMASTER__OPLL_SLOTS; i++) {
        CHECK_EQ_U64(chip.slot[i].eg_out, 0);
    }
}

/* One call evaluates exactly 18 slots whatever the registers say, so a ROM that hammers
   the chip cannot make a sample cost more. */
static void slot_count(void)
{
    struct opll_stream st;

    cogmaster__opll_reset(&chip);
    CHECK_EQ_U64(chip.slot_evals, 0);
    opll_stream_init(&st, OPLL_STREAM_SEED);
    for (uint32_t i = 0; i < 10000; i++) {
        const uint64_t r = opll_stream_next(&st);
        put(&chip, opll_stream_address((uint32_t)((r >> 8) % OPLL_STREAM_ADDRESSES)),
            (uint32_t)(r >> 40) & 0xffu);
        (void)cogmaster__opll_sample(&chip);
        CHECK_EQ_U64(chip.slot_evals, 18u * (i + 1u));
    }
}

/* Every tone voice plays the user instrument at full level: a sustained carrier at volume
   0, and a modulator at the least level so the carrier is nearly a sine. */
static void nine_tones_at_full_level(struct cogmaster__opll *o)
{
    cogmaster__opll_reset(o);
    put(o, 0x00, 0x21);
    put(o, 0x01, 0x21);
    put(o, 0x02, 0x3f);
    put(o, 0x03, 0x00);
    put(o, 0x04, 0xf0);
    put(o, 0x05, 0xf0);
    put(o, 0x06, 0x00);
    put(o, 0x07, 0x00);
    for (uint32_t ch = 0; ch < 9; ch++) {
        put(o, 0x30 + ch, 0x00);
        put(o, 0x10 + ch, 0x80);
        put(o, 0x20 + ch, KEY_ON_B4);
    }
}

/* The voices that sound together are nine tone voices, or six tone voices and five drums.
   At full level a tone voice peaks at COGMASTER__OPLL_TONE_PEAK and a drum at
   COGMASTER__OPLL_DRUM_PEAK, so the sum of the sounding voices fits 16 bits. The sum the chip
   returns also adds the last outputs of the group that is not sounding, as the original
   does; that total is bounded by all fourteen peaks. */
static void sum_bound(void)
{
    int32_t tone_hi = 0;
    int32_t tone_lo = 0;
    int32_t drum_hi = 0;
    int32_t drum_lo = 0;
    int32_t sum_hi = 0;
    int32_t sum_lo = 0;

    CHECK_EQ_U64(COGMASTER__OPLL_SUM_MAX,
                 6u * COGMASTER__OPLL_TONE_PEAK + 5u * COGMASTER__OPLL_DRUM_PEAK);
    CHECK(COGMASTER__OPLL_SUM_MAX <= INT16_MAX);

    /* Nine tone voices in phase. */
    nine_tones_at_full_level(&chip);
    run(&chip, 200);
    for (uint32_t i = 0; i < 60000; i++) {
        const int32_t sum = cogmaster__opll_sample(&chip);
        sum_hi = sum > sum_hi ? sum : sum_hi;
        sum_lo = sum < sum_lo ? sum : sum_lo;
        for (uint32_t c = 0; c < 9; c++) {
            tone_hi = chip.ch_out[c] > tone_hi ? chip.ch_out[c] : tone_hi;
            tone_lo = chip.ch_out[c] < tone_lo ? chip.ch_out[c] : tone_lo;
        }
    }
    CHECK_EQ_U64(tone_hi, COGMASTER__OPLL_TONE_PEAK);
    CHECK(tone_lo >= -(int32_t)COGMASTER__OPLL_TONE_PEAK);
    CHECK_EQ_U64(sum_hi, 9 * tone_hi);
    CHECK_EQ_U64(sum_lo, 9 * tone_lo);
    CHECK(sum_hi <= (int32_t)COGMASTER__OPLL_SUM_MAX);
    CHECK(sum_lo >= -(int32_t)COGMASTER__OPLL_SUM_MAX);

    /* The last outputs of the tone voices stay in the sum after rhythm mode takes three of
       them: the three held values do not change and the sum is the total of all fourteen. */
    const int16_t held[3] = {chip.ch_out[6], chip.ch_out[7], chip.ch_out[8]};
    put(&chip, 0x0e, 0x20);
    for (uint32_t i = 0; i < 50; i++) {
        const int32_t sum = cogmaster__opll_sample(&chip);
        int32_t all = 0;
        for (uint32_t c = 0; c < COGMASTER__OPLL_CHANNELS; c++) {
            all += chip.ch_out[c];
        }
        CHECK_EQ_U64(sum, all);
        for (uint32_t c = 0; c < 3; c++) {
            CHECK_EQ_U64(chip.ch_out[6 + c], held[c]);
        }
        CHECK(sum <= (int32_t)(9u * COGMASTER__OPLL_TONE_PEAK + 5u * COGMASTER__OPLL_DRUM_PEAK));
    }

    /* Six tone voices and all five drums at full level: the test register holds every
       envelope at zero attenuation. */
    nine_tones_at_full_level(&chip);
    for (uint32_t ch = 6; ch < 9; ch++) {
        put(&chip, 0x20 + ch, KEY_OFF_B4);
    }
    put(&chip, 0x0f, 0x01);
    put(&chip, 0x0e, 0x3f);
    run(&chip, 200);
    sum_hi = 0;
    sum_lo = 0;
    for (uint32_t i = 0; i < 400000; i++) {
        int32_t sounding = 0;

        (void)cogmaster__opll_sample(&chip);
        for (uint32_t c = 0; c < 6; c++) {
            sounding += chip.ch_out[c];
        }
        for (uint32_t c = 9; c < COGMASTER__OPLL_CHANNELS; c++) {
            sounding += chip.ch_out[c];
            drum_hi = chip.ch_out[c] > drum_hi ? chip.ch_out[c] : drum_hi;
            drum_lo = chip.ch_out[c] < drum_lo ? chip.ch_out[c] : drum_lo;
        }
        sum_hi = sounding > sum_hi ? sounding : sum_hi;
        sum_lo = sounding < sum_lo ? sounding : sum_lo;
    }
    CHECK(drum_hi <= (int32_t)COGMASTER__OPLL_DRUM_PEAK);
    CHECK_EQ_U64(-drum_lo, COGMASTER__OPLL_DRUM_PEAK);
    CHECK(sum_hi <= (int32_t)COGMASTER__OPLL_SUM_MAX);
    CHECK(sum_lo >= -(int32_t)COGMASTER__OPLL_SUM_MAX);
}

/* ---- The unit on the console: ports, control, reset, state and cost ---- */

#define IMAGE_SIZE 1024u
#define TRACE 32768u
#define PORT_ADDRESS 0xF0u
#define PORT_DATA 0xF1u
#define PORT_CTRL 0xF2u
#define SETTLE_FRAMES 12u

static uint8_t image[IMAGE_SIZE];
static int16_t pcm[COGMASTER__AUDIO_MAX_FRAMES * 2u];
static cogmaster_audio_record log_record[TRACE];

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* A new instance of the profile running an image of HALTs, with a trace log of TRACE records
   and the unit as fm says. */
static cogmaster *make_profile(uint32_t machine, uint32_t region, uint32_t video, uint32_t fm)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cfg.trace_records = TRACE;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    memset(image, 0x76, sizeof image);
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    media.region = region;
    media.tv = COGMASTER_TV_NTSC;
    media.video_mode = video;
    media.fm = fm;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

static cogmaster *make_unit(uint32_t region, uint32_t fm)
{
    return make_profile(COGMASTER_MACHINE_SMS2, region, COGMASTER_VIDEO_SMS, fm);
}

/* A Z80 OUT of v to port at tick: the bus is the one the CPU drives. */
static void out_at(cogmaster *m, uint64_t tick, uint16_t port, uint8_t v)
{
    m->ticks = tick;
    cogmaster__bus_out(m, port, v);
}

/* The first tick of the frame about to run, plus an offset inside it. */
static uint64_t in_frame(const cogmaster *m, uint64_t offset)
{
    return m->frame * cogmaster__frame_ticks(m) + offset;
}

/* Voice 0 plays instrument 1 at full volume, f-number $180 in block 4, keyed on, through $F0 and
   $F1 at 33-tick spacing from tick. */
static void note_on(cogmaster *m, uint64_t tick)
{
    static const uint8_t writes[6] = {0x30, 0x10, 0x10, 0x80, 0x20, (uint8_t)KEY_ON_B4};

    for (uint32_t i = 0; i < 6; i += 2) {
        out_at(m, tick + 33u * i, PORT_ADDRESS, writes[i]);
        out_at(m, tick + 33u * i + 33u, PORT_DATA, writes[i + 1]);
    }
}

/* The records waiting in the log, oldest first, copied to dst; empties the log. */
static uint32_t drain_into(cogmaster *m, cogmaster_audio_record *dst)
{
    cogmaster_audio_trace t;
    memset(&t, 0, sizeof t);
    t.size = (uint32_t)sizeof t;
    t.records = dst;
    t.capacity = TRACE;
    CHECK_EQ_U64(cogmaster_get_audio_trace(m, &t), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(t.dropped, 0u);
    return t.written;
}

static uint32_t drain_trace(cogmaster *m)
{
    return drain_into(m, log_record);
}

/* Runs one frame and takes its audio into out, as a host does; returns the sample frames and
   sets *peak to the largest magnitude among them. */
static uint32_t take_frame(cogmaster *m, int16_t *out, uint32_t *peak)
{
    cogmaster_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = out;
    a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
    *peak = 0u;
    for (uint32_t i = 0; i < a.written * a.channels; i++) {
        const uint32_t mag = out[i] < 0 ? (uint32_t)-(int32_t)out[i] : (uint32_t)out[i];
        *peak = mag > *peak ? mag : *peak;
    }
    return a.written;
}

/* The largest magnitude of n frames' audio. */
static uint32_t run_peak(cogmaster *m, uint32_t n)
{
    uint32_t most = 0;

    for (uint32_t i = 0; i < n; i++) {
        uint32_t peak;
        (void)take_frame(m, pcm, &peak);
        most = peak > most ? peak : most;
    }
    return most;
}

static uint32_t count_source(uint32_t n, uint32_t source)
{
    uint32_t found = 0;

    for (uint32_t i = 0; i < n; i++) {
        found += log_record[i].source == source;
    }
    return found;
}

/* The chip as the original's own cadence says it runs: sample k completes at tick 216k, and a
   write at a tick applies after every sample due at or before that tick and before the next.
   The model takes the writes (tick, register, value) in order and gives the 16-bit level of each
   sample from 1 to n. */
struct model_write {
    uint64_t tick;
    uint32_t reg;
    uint32_t value;
};

static void model_levels(const struct model_write *w, uint32_t writes, uint32_t n, uint16_t *out)
{
    uint32_t next = 0;

    cogmaster__opll_reset(&twin);
    for (uint32_t k = 1; k <= n; k++) {
        while (next < writes && w[next].tick < 216u * k) {
            cogmaster__opll_write(&twin, w[next].reg, w[next].value);
            next++;
        }
        out[k - 1u] = (uint16_t)cogmaster__opll_sample(&twin);
    }
}

/* The level of source 5 that the first n records of the log give at tick: the last record at or
   before it, or 0, the level the chip starts at, before the first. *at is where to look from. */
static uint32_t logged_level(uint32_t n, uint64_t tick, uint32_t *at)
{
    while (*at < n && log_record[*at].tick <= tick) {
        (*at)++;
    }
    return *at == 0 ? 0u : log_record[*at - 1u].level;
}

/* A note written through $F0 and $F1 reaches the chip, is synthesised and mixed, and $F2 mutes
   it at the mixer while the chip runs on: the registers hold what was written, the log holds a
   record of source 5 at every change of the sum on the 216-tick grid and one of source 6 at the
   write, and the level at every sample is what the chip gives when each write applies after the
   samples due at its tick (one write lands just after a sample, so a catch-up that came after
   the write would change that sample). The first frames carry the note, and after $F2 = 0 the
   stream goes silent while the chip keeps evaluating 18 slots a sample and keeps reporting. */
#define MODEL_SAMPLES 1659u

static void through_bus(void)
{
    static const struct model_write plan[] = {{233, 0x30, 0x10},
                                              {299, 0x10, 0x80},
                                              {365, 0x20, KEY_ON_B4},
                                              {216u * 300u + 10u, 0x30, 0x11}};
    static uint16_t want[MODEL_SAMPLES];
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    uint32_t n;

    CHECK_EQ_U64(m->fm_unit, 1);
    (void)drain_trace(m);
    out_at(m, 100, PORT_CTRL, 1);
    note_on(m, 200);
    out_at(m, plan[3].tick - 33u, PORT_ADDRESS, 0x30);
    out_at(m, plan[3].tick, PORT_DATA, 0x11);
    CHECK_EQ_U64(m->io.audio_ctrl, 1);
    CHECK_EQ_U64(m->opll.reg[0x30], 0x11);
    CHECK_EQ_U64(m->opll.reg[0x10], 0x80);
    CHECK_EQ_U64(m->opll.reg[0x20], KEY_ON_B4);
    CHECK_EQ_U64(m->opll.latch, 0x30);

    CHECK(run_peak(m, 2) > 0);
    n = drain_trace(m);
    CHECK(count_source(n, 5) > 100);
    uint32_t off_grid = 0;
    uint32_t control = 0;
    for (uint32_t i = 0; i < n; i++) {
        off_grid += log_record[i].source == 5 && log_record[i].tick % 216u != 0u;
        if (log_record[i].source == 6) {
            control++;
            CHECK_EQ_U64(log_record[i].tick, 100);
            CHECK_EQ_U64(log_record[i].level, 1);
        }
    }
    CHECK_EQ_U64(off_grid, 0);
    CHECK_EQ_U64(control, 1);

    /* The level at every sample against the model of the cadence. */
    uint32_t sum_records = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (log_record[i].source == 5) {
            log_record[sum_records++] = log_record[i];
        }
    }
    model_levels(plan, sizeof plan / sizeof plan[0], MODEL_SAMPLES, want);
    uint32_t at = 0;
    uint32_t differ = 0;
    uint32_t moving = 0;
    for (uint32_t k = 1; k <= MODEL_SAMPLES; k++) {
        const uint32_t got = logged_level(sum_records, 216u * k, &at);
        differ += got != want[k - 1u];
        moving += want[k - 1u] != 0;
    }
    CHECK_EQ_U64(differ, 0);
    CHECK(moving > 400);

    /* $F2 = 0 mutes the FM: the stream settles to silence, the chip does not stop. */
    out_at(m, in_frame(m, 500), PORT_CTRL, 0);
    const uint64_t before = m->opll.slot_evals;
    CHECK(run_peak(m, SETTLE_FRAMES) > 0);
    CHECK_EQ_U64(run_peak(m, 1), 0);
    CHECK(m->opll.slot_evals > before + 18u * 800u * SETTLE_FRAMES);
    n = drain_trace(m);
    CHECK(count_source(n, 5) > 100);
    CHECK(m->opll.slot[1].eg_state != OPLL_RELEASE);
    cogmaster_destroy(m);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"key_on", key_on},
    {"rhythm", rhythm},
    {"noise", noise},
    {"lfo", lfo},
    {"test_register", test_register},
    {"slot_count", slot_count},
    {"sum_bound", sum_bound},
    {"through_bus", through_bus},
};

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "lockstep") == 0) {
        lockstep(argv[2]);
        CHECK_DONE();
    }
    for (size_t i = 0; argc == 2 && i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "usage: cogmaster-test-fm lockstep <digest file> | <case>\n");
    return 2;
}
