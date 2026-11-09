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
#define STATE_BUF 393216u
#define PORT_ADDRESS 0xF0u
#define PORT_DATA 0xF1u
#define PORT_CTRL 0xF2u
#define SETTLE_FRAMES 12u

static uint8_t image[IMAGE_SIZE];
static int16_t pcm[COGMASTER__AUDIO_MAX_FRAMES * 2u];
static cogmaster_audio_record log_record[TRACE];
static cogmaster_audio_record log_other[TRACE];
static uint8_t state_a[STATE_BUF];
static uint8_t state_b[STATE_BUF];
static uint8_t state_c[STATE_BUF];

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

/* Loads an image of HALTs into m as the profile and the unit as fm say. */
static void load_profile(cogmaster *m, uint32_t machine, uint32_t region, uint32_t video,
                         uint32_t fm)
{
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
    load_profile(m, machine, region, video, fm);
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

/* Tone 0 at period $0FE and full volume, through $7F. */
static void psg_tone(cogmaster *m, uint64_t tick)
{
    out_at(m, tick, 0x7Fu, 0x8Eu);
    out_at(m, tick + 33u, 0x7Fu, 0x0Fu);
    out_at(m, tick + 66u, 0x7Fu, 0x90u);
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

/* Writes of $F2 store the bits the profile keeps: bits 7 to 2 are ignored, a mask of 1 keeps bit
   0, and the value reaches the log as source 6. The drained audio carries what $F2 lets
   through: for each value, four instances take the same writes at the same ticks and differ
   only in what they are given to play. The PSG alone is heard for 0, the FM alone for 1,
   neither for 2 and both for 3, so a mute applied to the wrong source, or not at all, makes one
   of the comparisons fail. */
#define MIX_FRAMES 4u
#define MIX_SAMPLES (COGMASTER__AUDIO_MAX_FRAMES * MIX_FRAMES)

static int16_t mixed[4][MIX_SAMPLES];

/* Plays four frames after $F2 = ctrl right after the load, with the PSG tone and the FM note
   as asked, into out; returns the samples. */
static uint32_t mix_run(uint32_t ctrl, int tone, int note, int16_t *out)
{
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    uint32_t total = 0;

    out_at(m, 10, PORT_CTRL, (uint8_t)ctrl);
    if (tone) {
        psg_tone(m, 400);
    }
    if (note) {
        note_on(m, 600);
    }
    for (uint32_t i = 0; i < MIX_FRAMES; i++) {
        uint32_t peak;
        total += take_frame(m, out + total, &peak);
    }
    cogmaster_destroy(m);
    return total;
}

static int same(const int16_t *a, const int16_t *b, uint32_t n)
{
    return memcmp(a, b, n * sizeof a[0]) == 0;
}

static void f2_write_mask(void)
{
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);

    CHECK_EQ_U64(m->fm_mask, 3);
    (void)drain_trace(m);
    static const struct {
        uint8_t written;
        uint8_t stored;
    } masked[] = {{0xFF, 3}, {0xFC, 0}, {0x02, 2}, {0xF5, 1}, {0x03, 3}, {0x00, 0}};
    for (uint32_t i = 0; i < sizeof masked / sizeof masked[0]; i++) {
        out_at(m, 100u + 40u * i, PORT_CTRL, masked[i].written);
        CHECK_EQ_U64(m->io.audio_ctrl, masked[i].stored);
        CHECK_EQ_U64(m->synth.level[6], masked[i].stored);
    }
    uint32_t n = drain_trace(m);
    CHECK_EQ_U64(count_source(n, 6), sizeof masked / sizeof masked[0]);
    CHECK_EQ_U64(log_record[0].level, 3);
    CHECK_EQ_U64(log_record[n - 1u].level, 0);

    /* The unit alone keeps bit 0; the mask is plain data. */
    m->fm_mask = 1;
    out_at(m, 400, PORT_CTRL, 3);
    CHECK_EQ_U64(m->io.audio_ctrl, 1);
    out_at(m, 440, PORT_CTRL, 2);
    CHECK_EQ_U64(m->io.audio_ctrl, 0);
    cogmaster_destroy(m);

    /* An export console with the unit switched on has it too, as the detector's test hook. */
    m = make_unit(COGMASTER_REGION_EXPORT, COGMASTER_FM_ON);
    CHECK_EQ_U64(m->fm_unit, 1);
    out_at(m, 100, PORT_CTRL, 3);
    CHECK_EQ_U64(m->io.audio_ctrl, 3);
    cogmaster_destroy(m);

    for (uint32_t ctrl = 0; ctrl < 4; ctrl++) {
        const uint32_t n_c = mix_run(ctrl, 1, 1, mixed[0]);
        const uint32_t n_p = mix_run(ctrl, 1, 0, mixed[1]);
        const uint32_t n_f = mix_run(ctrl, 0, 1, mixed[2]);
        const uint32_t n_n = mix_run(ctrl, 0, 0, mixed[3]);
        CHECK(n_c == n_p && n_p == n_f && n_f == n_n);
        const int c_p = same(mixed[0], mixed[1], n_c);
        const int c_f = same(mixed[0], mixed[2], n_c);
        const int c_n = same(mixed[0], mixed[3], n_c);
        const int p_n = same(mixed[1], mixed[3], n_c);
        const int f_n = same(mixed[2], mixed[3], n_c);
        if (ctrl == 0) {
            CHECK(c_p && !p_n);
        } else if (ctrl == 1) {
            CHECK(c_f && !f_n);
        } else if (ctrl == 2) {
            CHECK(c_n && p_n && f_n);
        } else {
            CHECK(!c_p && !c_f && !c_n);
            CHECK(!p_n && !f_n);
        }
    }
}

/* $F2 brings both chips up to date before it gates them, so what they did before the write is
   heard and what they do after is not. Two instances play the same tone and note and mute both at
   tick 99,000; one also makes writes that change nothing, which catch the chips up to just before
   the mute. They must sound the same, and the sound must be the one that stops at the mute: it
   carries the tone and the note up to it. A mute that gated the chips' pending steps instead
   would cut the first of them short. */
#define MUTE_TICK 99000u

static uint32_t late_mute_run(int redundant, int16_t *out)
{
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    uint32_t total = 0;
    uint32_t peak;

    out_at(m, 10, PORT_CTRL, 3);
    psg_tone(m, 400);
    note_on(m, 600);
    if (redundant) {
        out_at(m, MUTE_TICK - 66u, PORT_ADDRESS, 0x10);
        out_at(m, MUTE_TICK - 33u, PORT_DATA, 0x80);
        out_at(m, MUTE_TICK - 3u, 0x7Fu, 0x90u);
    }
    out_at(m, MUTE_TICK, PORT_CTRL, 2);
    for (uint32_t i = 0; i < 3; i++) {
        total += take_frame(m, out + total, &peak);
        CHECK(i > 0 || peak > 0);
    }
    cogmaster_destroy(m);
    return total;
}

static void f2_catches_up(void)
{
    static int16_t plain[3u * COGMASTER__AUDIO_MAX_FRAMES];
    static int16_t caught[3u * COGMASTER__AUDIO_MAX_FRAMES];
    const uint32_t n = late_mute_run(0, plain);

    CHECK_EQ_U64(late_mute_run(1, caught), n);
    CHECK(same(plain, caught, n));
}

/* Reads of $F2 return what was written, bits 7, 6 and 5 follow bits 11, 7 and 3 of the line
   counter (the line index of the tick count since the load or reset) and bits 4 to 2 read 0.
   Reads of the other two ports drive nothing, and no read of any is input. */
static void f2_read_bits(void)
{
    static const struct {
        uint32_t line;
        uint32_t bits;
    } counter[] = {{0, 0x00},    {7, 0x00},    {8, 0x20},    {16, 0x00},   {24, 0x20},
                   {127, 0x20},  {128, 0x40},  {136, 0x60},  {2047, 0x60}, {2048, 0x80},
                   {2056, 0xA0}, {2176, 0xC0}, {2184, 0xE0}, {4095, 0xE0}, {4096, 0x00}};
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);

    for (uint32_t stored = 0; stored < 4; stored++) {
        out_at(m, 100, PORT_CTRL, (uint8_t)stored);
        for (uint32_t i = 0; i < sizeof counter / sizeof counter[0]; i++) {
            /* The first and last tick of the line read the same. */
            for (uint32_t edge = 0; edge < 2; edge++) {
                m->ticks = (uint64_t)counter[i].line * 684u + (edge ? 683u : 0u);
                m->input_read = 0;
                const uint8_t v = cogmaster__io_in(m, PORT_CTRL);
                CHECK_EQ_HEX(v, counter[i].bits | stored);
                CHECK_EQ_U64(v & 0x1Cu, 0);
                CHECK_EQ_U64(m->input_read, 0);
            }
        }
    }
    /* The ports $F0 and $F1 are written and not read: the bus is undriven. */
    m->input_read = 0;
    const uint32_t open_before = m->counters.open_bus_reads;
    CHECK_EQ_HEX(cogmaster__io_in(m, PORT_ADDRESS), 0xFF);
    CHECK_EQ_HEX(cogmaster__io_in(m, PORT_DATA), 0xFF);
    CHECK_EQ_U64(m->counters.open_bus_reads, open_before + 2u);
    CHECK_EQ_U64(m->input_read, 0);
    cogmaster_destroy(m);
}

/* On a console with the unit the ports $F0 to $F2 answer ahead of the I/O chip: with $3E bit 2
   set (chip off, cartridge and RAM left on) a write to $F2 still stores and mutes, its read
   still returns the stored value with the counter bits, and the instrument, frequency and
   key-on written through $F0 and $F1 still change the chip's registers and sound; the pad
   ports answer open bus while the chip is off and as before when it is back. A decode behind
   the I/O chip's gate fails each of these. */
static void chip_off(void)
{
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    const uint8_t on = m->io.mem_ctrl;
    const uint8_t dc = cogmaster__bus_in(m, 0xDCu);
    const uint8_t dd = cogmaster__bus_in(m, 0xDDu);

    out_at(m, 100, 0x3Eu, (uint8_t)(on | 0x04u));
    CHECK_EQ_U64(m->io.mem_ctrl, on | 0x04u);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDCu), 0xFF);
    out_at(m, 140, PORT_CTRL, 3);
    CHECK_EQ_U64(m->io.audio_ctrl, 3);
    m->ticks = 8u * 684u;
    CHECK_EQ_HEX(cogmaster__bus_in(m, PORT_CTRL), 0x20u | 3u);
    note_on(m, 8u * 684u + 100u);
    CHECK_EQ_U64(m->opll.reg[0x30], 0x10);
    CHECK_EQ_U64(m->opll.reg[0x10], 0x80);
    CHECK_EQ_U64(m->opll.reg[0x20], KEY_ON_B4);
    CHECK(run_peak(m, 2) > 0);

    /* Muted again with the chip still off, the FM goes silent. */
    out_at(m, in_frame(m, 500), PORT_CTRL, 0);
    CHECK_EQ_U64(m->io.audio_ctrl, 0);
    CHECK(run_peak(m, SETTLE_FRAMES) > 0);
    CHECK_EQ_U64(run_peak(m, 1), 0);

    out_at(m, in_frame(m, 100), 0x3Eu, on);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDCu), dc);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu), dd);
    CHECK_EQ_U64(m->fault_present, 0);
    cogmaster_destroy(m);
}

/* Without the unit the three ports do nothing and cost nothing. The export console reads them
   as $DC (even) and $DD (odd) without counting a read of input; a Japanese console and a Game
   Gear without the unit read open bus; a write changes nothing and is no fault. */
static void no_unit_ports(void)
{
    cogmaster *m = make_unit(COGMASTER_REGION_EXPORT, COGMASTER_FM_AUTO);

    CHECK_EQ_U64(m->fm_unit, 0);
    m->input.p1 = 0x15u;
    m->input.p2 = 0x0Cu;
    for (uint32_t port = PORT_ADDRESS; port <= PORT_CTRL; port++) {
        m->input_read = 0;
        CHECK_EQ_HEX(cogmaster__io_in(m, (uint16_t)port), (port & 1u) ? 0xFCu : 0xEAu);
        CHECK_EQ_U64(m->input_read, 0);
    }
    m->input_read = 0;
    CHECK_EQ_HEX(cogmaster__io_in(m, 0xDCu), 0xEA);
    CHECK_EQ_U64(m->input_read, 1);

    const struct cogmaster__opll quiet = m->opll;
    out_at(m, 100, PORT_ADDRESS, 0x30);
    out_at(m, 140, PORT_DATA, 0x10);
    out_at(m, 180, PORT_CTRL, 3);
    CHECK_EQ_U64(m->io.audio_ctrl, 0);
    CHECK_EQ_U64(m->synth.level[6], 0);
    CHECK(memcmp(&quiet, &m->opll, sizeof quiet) == 0);
    CHECK_EQ_U64(m->fault_present, 0);
    CHECK_EQ_U64(drain_trace(m), 4);
    cogmaster_destroy(m);

    for (uint32_t k = 0; k < 2; k++) {
        m = k == 0 ? make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_OFF)
                   : make_profile(COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, COGMASTER_VIDEO_GG,
                                  COGMASTER_FM_AUTO);
        CHECK_EQ_U64(m->fm_unit, 0);
        for (uint32_t port = PORT_ADDRESS; port <= PORT_CTRL; port++) {
            const uint32_t open_before = m->counters.open_bus_reads;
            m->input_read = 0;
            CHECK_EQ_HEX(cogmaster__io_in(m, (uint16_t)port), 0xFF);
            CHECK_EQ_U64(m->counters.open_bus_reads, open_before + 1u);
            CHECK_EQ_U64(m->input_read, 0);
        }
        out_at(m, 100, PORT_ADDRESS, 0x30);
        out_at(m, 140, PORT_DATA, 0x10);
        out_at(m, 180, PORT_CTRL, 3);
        CHECK_EQ_U64(m->io.audio_ctrl, 0);
        CHECK_EQ_U64(m->opll.latch, 0);
        CHECK_EQ_U64(m->opll.reg[0x30], 0);
        CHECK_EQ_U64(m->fault_present, 0);
        cogmaster_destroy(m);
    }
}

/* A load and both resets leave the unit at its power-on state: $F2 at 0, every YM register 0,
   the PSG silent, the next sample at tick 216 and the tick-0 snapshot of sources 0 to 3, 5 and
   6 in the log. The synthesiser holds the chip's levels, so after a SOFT reset the unit is
   muted (a stale mute or FM level would leave the note audible without its $F2 write), and
   the note written again is heard once $F2 is. The sample count of a SOFT reset stays on the
   cadence, and a POWER reset restarts it. */
static void expect_unit_snapshot(cogmaster *m, const char *what)
{
    static const uint32_t sources[6] = {0, 1, 2, 3, 5, 6};
    const uint32_t n = drain_trace(m);

    if (n != 6u) {
        fprintf(stderr, "%s: %lu records, wanted 6\n", what, (unsigned long)n);
        check_failures++;
        return;
    }
    for (uint32_t i = 0; i < n; i++) {
        CHECK_EQ_U64(log_record[i].tick, 0);
        CHECK_EQ_U64(log_record[i].source, sources[i]);
    }
    CHECK_EQ_U64(log_record[4].level, 0);
    CHECK_EQ_U64(log_record[5].level, 0);
}

static void reset_kinds(void)
{
    for (uint32_t kind = COGMASTER_RESET_SOFT; kind <= COGMASTER_RESET_POWER; kind++) {
        cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
        uint64_t frames = 0;
        uint64_t total = 0;
        uint32_t peak;

        expect_unit_snapshot(m, "load");
        CHECK_EQ_U64(m->fm_next, 216);
        out_at(m, 100, PORT_CTRL, 1);
        note_on(m, 200);
        for (uint32_t i = 0; i < 3; i++) {
            total += take_frame(m, pcm, &peak);
            frames++;
        }
        CHECK(peak > 0);
        CHECK_EQ_U64(m->synth.level[5], (uint16_t)m->opll.sum);
        CHECK_EQ_U64(m->synth.level[6], 1);
        CHECK(m->synth.gained[5][0] > 0);
        CHECK(m->fm_next > 3u * 179208u);
        (void)drain_trace(m);

        CHECK_EQ_U64(cogmaster_reset(m, kind), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(m->io.audio_ctrl, 0);
        CHECK_EQ_U64(m->opll.latch, 0);
        for (uint32_t r = 0; r < 0x40u; r++) {
            CHECK_EQ_U64(m->opll.reg[r], 0);
        }
        for (uint32_t ch = 0; ch < 4; ch++) {
            CHECK_EQ_U64(m->psg.att[ch], 15);
        }
        CHECK_EQ_U64(m->fm_next, 216);
        CHECK_EQ_U64(m->synth.level[5], (uint16_t)m->opll.sum);
        CHECK_EQ_U64(m->synth.level[5], 0);
        CHECK_EQ_U64(m->synth.level[6], 0);
        CHECK_EQ_U64(m->synth.gained[5][0], 0);
        CHECK_EQ_HEX(cogmaster__io_in(m, PORT_CTRL), 0);
        m->ticks = 8u * 684u;
        CHECK_EQ_HEX(cogmaster__io_in(m, PORT_CTRL), 0x20);
        m->ticks = 0;
        expect_unit_snapshot(m, kind == COGMASTER_RESET_SOFT ? "soft reset" : "power reset");

        /* The old note again, with $F2 as the reset left it: muted. */
        note_on(m, 200);
        CHECK_EQ_U64(m->opll.reg[0x20], KEY_ON_B4);
        for (uint32_t i = 0; i < SETTLE_FRAMES; i++) {
            const uint64_t n = take_frame(m, pcm, &peak);
            if (kind == COGMASTER_RESET_SOFT) {
                total += n;
            }
            frames++;
        }
        CHECK_EQ_U64(peak, 0);
        /* And with $F2 written, heard. */
        out_at(m, in_frame(m, 100), PORT_CTRL, 1);
        uint32_t heard = 0;
        for (uint32_t i = 0; i < 3; i++) {
            total += take_frame(m, pcm, &peak);
            frames++;
            heard = peak > heard ? peak : heard;
        }
        CHECK(heard > 0);
        if (kind == COGMASTER_RESET_SOFT) {
            CHECK_EQ_U64(total, frames * 3504512u / 4375u);
        }
        cogmaster_destroy(m);
    }
}

/* Loading content again, with a note sounding and its frame's audio not taken, leaves nothing
   of the unit behind: the control, the chip, its sample tick and the pending audio are as at a
   first load, and a load without the unit has no chip to hold any of it. */
static void reload_zeroes(void)
{
    for (uint32_t fm = COGMASTER_FM_AUTO; fm <= COGMASTER_FM_OFF; fm += COGMASTER_FM_OFF) {
        cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
        cogmaster_audio a;
        uint32_t peak;

        out_at(m, 100, PORT_CTRL, 3);
        note_on(m, 200);
        CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
        CHECK(m->opll.reg[0x20] != 0 && m->io.audio_ctrl == 3 && m->fm_next > 179208u);
        load_profile(m, COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_VIDEO_SMS, fm);
        CHECK_EQ_U64(m->fm_unit, fm == COGMASTER_FM_AUTO);
        CHECK_EQ_U64(m->io.audio_ctrl, 0);
        CHECK_EQ_U64(m->fm_next, 216);
        CHECK_EQ_U64(m->opll.latch, 0);
        for (uint32_t r = 0; r < 0x40u; r++) {
            CHECK_EQ_U64(m->opll.reg[r], 0);
        }
        memset(&a, 0, sizeof a);
        a.size = (uint32_t)sizeof a;
        a.samples = pcm;
        a.capacity = COGMASTER__AUDIO_MAX_FRAMES;
        CHECK_EQ_U64(cogmaster_get_audio(m, &a), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(a.written, 0);
        if (fm == COGMASTER_FM_AUTO) {
            expect_unit_snapshot(m, "reload");
        } else {
            CHECK_EQ_U64(drain_trace(m), 4);
        }
        (void)take_frame(m, pcm, &peak);
        CHECK_EQ_U64(peak, 0);
        cogmaster_destroy(m);
    }
}

static uint64_t get_le(const uint8_t *p, uint32_t width)
{
    uint64_t v = 0;

    for (uint32_t i = 0; i < width; i++) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

static const struct cogmaster__state_field *find_field(const struct cogmaster__state_field *f,
                                                       size_t n, const char *name)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(f[i].name, name) == 0) {
            return &f[i];
        }
    }
    fprintf(stderr, "no state field named %s\n", name);
    check_failures++;
    return NULL;
}

/* Loads the size bytes of state_c into target, which must refuse them and save the bytes it
   saved before. */
static void expect_refused(cogmaster *target, size_t size, const char *what)
{
    const size_t target_size = cogmaster__state_size(target);

    CHECK(target_size > 64u && target_size <= STATE_BUF);
    CHECK_EQ_U64(cogmaster__state_save(target, state_a, target_size), COGMASTER_STATUS_OK);
    if (cogmaster__state_load(target, state_c, size) == COGMASTER_STATUS_OK) {
        fprintf(stderr, "%s: accepted\n", what);
        check_failures++;
    }
    CHECK_EQ_U64(cogmaster__state_save(target, state_b, target_size), COGMASTER_STATUS_OK);
    if (memcmp(state_a, state_b, target_size) != 0) {
        fprintf(stderr, "%s: the refused load changed the instance\n", what);
        check_failures++;
    }
}

/* The state of an instance with the unit holds the chip, its sample tick and the control, and a
   state loaded into a second instance runs on as the first does: the same samples, the same
   log records and the same bytes after ten frames. A state is refused when it carries what the
   instance cannot have: a control with no unit, a chip block for a console without one, a
   sample tick off the grid or behind the frame, or a chip whose sum is not its channels'. */
static void state_round_trip(void)
{
    static int16_t pcm_other[COGMASTER__AUDIO_MAX_FRAMES * 2u];
    cogmaster *a = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    uint32_t peak;

    out_at(a, 100, PORT_CTRL, 3);
    psg_tone(a, 400);
    note_on(a, 600);
    for (uint32_t i = 0; i < 3; i++) {
        (void)take_frame(a, pcm, &peak);
    }
    CHECK(peak > 0);
    (void)drain_trace(a);
    const size_t size = cogmaster__state_size(a);
    CHECK(size > 64u && size <= STATE_BUF);
    CHECK_EQ_U64(cogmaster__state_save(a, state_a, size), COGMASTER_STATUS_OK);

    cogmaster *b = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    out_at(b, 100, PORT_CTRL, 2);
    out_at(b, 140, PORT_ADDRESS, 0x30);
    out_at(b, 180, PORT_DATA, 0x70);
    (void)take_frame(b, pcm_other, &peak);
    (void)drain_into(b, log_other);
    CHECK_EQ_U64(cogmaster__state_load(b, state_a, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(b->fm_next, a->fm_next);
    /* Every field of the chip but its evaluation counter came across. */
    struct cogmaster__opll chip_a = a->opll;
    struct cogmaster__opll chip_b = b->opll;
    chip_a.slot_evals = 0;
    chip_b.slot_evals = 0;
    CHECK(memcmp(&chip_a, &chip_b, sizeof chip_a) == 0);

    for (uint32_t i = 0; i < 10; i++) {
        uint32_t peak_b;
        const uint32_t na = take_frame(a, pcm, &peak);
        const uint32_t nb = take_frame(b, pcm_other, &peak_b);
        CHECK_EQ_U64(na, nb);
        CHECK(memcmp(pcm, pcm_other, (size_t)na * sizeof pcm[0]) == 0);
        const uint32_t ra = drain_into(a, log_record);
        const uint32_t rb = drain_into(b, log_other);
        CHECK_EQ_U64(ra, rb);
        CHECK(ra > 100u && memcmp(log_record, log_other, (size_t)ra * sizeof log_record[0]) == 0);
        CHECK(peak > 0);
    }
    CHECK_EQ_U64(cogmaster__state_save(a, state_b, size), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_save(b, state_c, size), COGMASTER_STATUS_OK);
    CHECK(memcmp(state_b, state_c, size) == 0);
    cogmaster_destroy(b);

    /* Poked into a copy of the first state, a field the instance cannot hold is refused. */
    const size_t n = cogmaster__state_record(a, NULL, 0);
    struct cogmaster__state_field *fields = malloc(n * sizeof *fields);
    if (fields == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_record(a, fields, n), n);
    CHECK_EQ_U64(cogmaster__state_save(a, state_a, size), COGMASTER_STATUS_OK);
    static const struct {
        const char *field;
        int64_t add;
        uint64_t set;
        const char *what;
    } hostile[] = {
        {"io_audio_ctrl", 0, 4, "a control above the unit's bits"},
        {"fm_next", 1, 0, "a sample tick off the 216 grid"},
        {"fm_next", 0, 216, "a sample tick behind the last frame"},
        {"fm_next", 216 * 100000, 0, "a sample tick far ahead of the ticks"},
        {"opll_sum", 1, 0, "a sum that is not the channels' total"},
        {"opll_slot_eg_out", 0, 128, "an envelope level past mute"},
        {"opll_slot_patch", 0, 38, "an instrument past the table"},
        {"opll_slot_pg_out", 0, 1024, "a phase index past the wave table"},
        {"opll_slot_eg_shift", 0, 14, "an envelope shift past 13"},
        {"opll_noise", 0, 0, "a noise register of zero, which never changes"},
        {"opll_patch_number", 0, 19, "an instrument number past the rhythm ones"},
        {"fm_unit", 0, 0, "the header without the unit"},
    };
    for (size_t i = 0; i < sizeof hostile / sizeof hostile[0]; i++) {
        const struct cogmaster__state_field *f = find_field(fields, n, hostile[i].field);
        if (f == NULL) {
            continue;
        }
        memcpy(state_c, state_a, size);
        put_le(state_c + f->offset,
               hostile[i].set != 0 || hostile[i].add == 0
                   ? hostile[i].set
                   : get_le(state_a + f->offset, f->width) + (uint64_t)hostile[i].add,
               f->width);
        cogmaster__state_seal(state_c, size);
        expect_refused(a, size, hostile[i].what);
    }
    /* The control: the unchanged state, resealed, loads. */
    memcpy(state_c, state_a, size);
    cogmaster__state_seal(state_c, size);
    CHECK_EQ_U64(cogmaster__state_load(a, state_c, size), COGMASTER_STATUS_OK);
    free(fields);

    /* A console without the unit has no chip block: a state with one is refused by its size,
       and its own state, with the control set, is refused by the field. */
    cogmaster *plain = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_OFF);
    (void)take_frame(plain, pcm, &peak);
    const size_t plain_size = cogmaster__state_size(plain);
    CHECK(plain_size > 64u && plain_size < size);
    memcpy(state_c, state_a, size);
    expect_refused(plain, size, "a chip block without the unit");
    const size_t pn = cogmaster__state_record(plain, NULL, 0);
    fields = malloc(pn * sizeof *fields);
    if (fields == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster__state_record(plain, fields, pn), pn);
    CHECK_EQ_U64(cogmaster__state_save(plain, state_c, plain_size), COGMASTER_STATUS_OK);
    const struct cogmaster__state_field *ctrl = find_field(fields, pn, "io_audio_ctrl");
    const struct cogmaster__state_field *unit = find_field(fields, pn, "fm_unit");
    if (ctrl != NULL && unit != NULL) {
        CHECK_EQ_U64(ctrl->max, 0);
        put_le(state_c + ctrl->offset, 1, ctrl->width);
        cogmaster__state_seal(state_c, plain_size);
        expect_refused(plain, plain_size, "a control with no unit");
        CHECK_EQ_U64(cogmaster__state_save(plain, state_c, plain_size), COGMASTER_STATUS_OK);
        put_le(state_c + unit->offset, 1, unit->width);
        cogmaster__state_seal(state_c, plain_size);
        expect_refused(plain, plain_size, "the unit byte of a console without one");
    }
    free(fields);
    cogmaster_destroy(plain);
    cogmaster_destroy(a);
}

/* A ROM that writes the unit's ports as fast as the Z80 can issue OUT, 11 T-states or 33 ticks
   apart, cannot make a frame cost more than its bound. The chip takes one sample per 216 ticks
   and evaluates 18 slots in it whatever the registers say. A write moves at most four PSG
   channels or, for $F2, four PSG sources and the FM source, one kernel addition each on every
   output, and a PSG step moves at most four; so a frame adds at most 16 taps a ring for each
   of four times the steps, five times the writes and the FM samples. */
static void cost(void)
{
    static const uint8_t psg_bytes[12] = {0x82u, 0x02u, 0xA2u, 0x02u, 0xC2u, 0x02u,
                                          0x90u, 0xB0u, 0xD0u, 0xF0u, 0xE4u, 0xE5u};
    cogmaster *m = make_unit(COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO);
    const uint64_t frame_ticks = cogmaster__frame_ticks(m);
    const uint32_t rings = 1;
    uint32_t seed = 0x2413u;
    uint32_t writes = 0;
    uint32_t psg_writes = 0;
    uint32_t f2_writes = 0;

    (void)drain_trace(m);
    for (uint64_t t = 33, i = 0; t < frame_ticks; t += 33, i++) {
        seed = seed * 1664525u + 1013904223u;
        switch (i % 6u) {
        case 0:
            out_at(m, t, PORT_ADDRESS, (uint8_t)((seed >> 8) % 0x40u));
            break;
        case 2:
            out_at(m, t, PORT_CTRL, (uint8_t)((i / 6u) % 4u));
            f2_writes++;
            break;
        case 3:
        case 5:
            out_at(m, t, 0x7Fu, psg_bytes[psg_writes++ % 12u]);
            break;
        default:
            out_at(m, t, PORT_DATA, (uint8_t)(seed >> 16));
            break;
        }
        writes++;
    }
    uint32_t peak;
    (void)take_frame(m, pcm, &peak);

    const struct cogmaster__synth_stats *st = &m->synth.stats;
    const uint64_t samples = m->opll.slot_evals / 18u;
    CHECK_EQ_U64(m->opll.slot_evals, 18u * samples);
    CHECK_EQ_U64(samples, (m->fm_next - 216u) / 216u);
    CHECK(samples <= (frame_ticks + 215u) / 216u);
    CHECK(samples + 2u >= frame_ticks / 216u);
    CHECK(st->psg_steps <= (frame_ticks + 47u) / 48u);
    CHECK_EQ_U64(st->psg_writes, psg_writes);
    const uint64_t bound =
        (uint64_t)COGMASTER__SYNTH_TAPS * rings * (4u * st->psg_steps + 5u * writes + samples);
    CHECK(st->kernel_apps <= bound);
    /* The hammer changed levels, so the bound is not met by a chip that stopped. */
    CHECK(st->kernel_apps > COGMASTER__SYNTH_TAPS * 1000u);
    CHECK(st->max_index + 32u < COGMASTER__SYNTH_RING);
    const uint32_t n = drain_trace(m);
    CHECK(count_source(n, 5) > 100u);
    CHECK_EQ_U64(count_source(n, 6), f2_writes);
    cogmaster_destroy(m);
}

/* ---- The detect routine of the fm_sweep fixture, under each unit setting ---- */

#define FIXTURE_SIZE 32768u
#define MB_BASE 0xC100u
#define MB_RUNNING 0x80u
#define MB_DONE_FRAMES 300u
#define MB_RECORD_COUNT 7u

static uint8_t fixture[FIXTURE_SIZE];

static uint32_t mailbox_byte(const cogmaster *m, uint32_t offset)
{
    return cogmaster_peek(m, COGMASTER_SPACE_BUS, MB_BASE + offset) & 0xFFu;
}

/* Runs the built fm_sweep image on the console and unit setting given, as the runner does: until
   its mailbox reports done, with each frame's sound taken. The program's detect record (the
   first, id 1) must hold want, and its script must have run to its seventh record whatever the
   unit setting. */
static void detect_case(const char *what, uint32_t region, uint32_t fm, uint32_t want)
{
    cogmaster_media media;
    cogmaster_config cfg;
    cogmaster *m = NULL;
    uint32_t frames = 0;
    int running = 0;

    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = fixture;
    media.image_size = FIXTURE_SIZE;
    media.machine = COGMASTER_MACHINE_SMS2;
    media.region = region;
    media.tv = COGMASTER_TV_NTSC;
    media.video_mode = COGMASTER_VIDEO_SMS;
    media.fm = fm;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    while (frames < MB_DONE_FRAMES && !(running && mailbox_byte(m, 5u) == 0u)) {
        uint32_t peak;

        (void)take_frame(m, pcm, &peak);
        frames++;
        running = running || mailbox_byte(m, 5u) == MB_RUNNING;
    }
    if (!running || mailbox_byte(m, 5u) != 0u) {
        fprintf(stderr, "%s: the mailbox is not done after %u frames\n", what, (unsigned)frames);
    }
    CHECK_EQ_U64(mailbox_byte(m, 5u), 0u);
    CHECK_EQ_U64(mailbox_byte(m, 6u), MB_RECORD_COUNT);
    CHECK_EQ_U64(mailbox_byte(m, 0x10u), 1u);
    CHECK_EQ_U64(mailbox_byte(m, 0x11u), 1u);
    if (mailbox_byte(m, 0x12u) != want || mailbox_byte(m, 0x13u) != 0u) {
        fprintf(stderr, "%s: detect record %u, want %u\n", what, (unsigned)mailbox_byte(m, 0x12u),
                (unsigned)want);
        check_failures++;
    }
    /* The last record is the frame count of the third segment: the script ran to its end. */
    CHECK_EQ_U64(mailbox_byte(m, 0x10u + 4u * 5u + 2u), 0xD6u);
    cogmaster_destroy(m);
}

/* An export console with the unit switched on finds it, and a Japanese console with the unit
   switched off does not; the automatic setting follows the region. The routine reads port $F2
   itself, so this is the program's own answer, not the runner's. */
static void detect(const char *path)
{
    FILE *f = fopen(path, "rb");

    if (f == NULL || fread(fixture, 1u, FIXTURE_SIZE, f) != FIXTURE_SIZE) {
        fprintf(stderr, "cannot read %s as %u bytes\n", path, (unsigned)FIXTURE_SIZE);
        exit(1);
    }
    (void)fclose(f);
    detect_case("export, unit on", COGMASTER_REGION_EXPORT, COGMASTER_FM_ON, 1u);
    detect_case("japan, unit off", COGMASTER_REGION_JAPAN, COGMASTER_FM_OFF, 0u);
    detect_case("japan, automatic", COGMASTER_REGION_JAPAN, COGMASTER_FM_AUTO, 1u);
    detect_case("export, automatic", COGMASTER_REGION_EXPORT, COGMASTER_FM_AUTO, 0u);
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
    {"f2_write_mask", f2_write_mask},
    {"f2_read_bits", f2_read_bits},
    {"f2_catches_up", f2_catches_up},
    {"chip_off", chip_off},
    {"no_unit_ports", no_unit_ports},
    {"reset_kinds", reset_kinds},
    {"reload_zeroes", reload_zeroes},
    {"state_round_trip", state_round_trip},
    {"cost", cost},
};

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "lockstep") == 0) {
        lockstep(argv[2]);
        CHECK_DONE();
    }
    if (argc == 3 && strcmp(argv[1], "detect") == 0) {
        detect(argv[2]);
        CHECK_DONE();
    }
    for (size_t i = 0; argc == 2 && i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr,
            "usage: cogmaster-test-fm lockstep <digest file> | detect <fm_sweep image> | <case>\n");
    return 2;
}
