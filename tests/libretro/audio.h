/* The audio proof: the module delivers the runner's PCM however the frontend
 * registers its audio callbacks.
 *
 *   cogmaster-test-libretro-host <module> audio <image> <frames> sms|gg <sha256>
 *
 * <sha256> is the audio-pcm hash cogmaster-run --hash-audio printed for the
 * same image in the same test. The image runs three times from a fresh load:
 * with only the batch callback, with only the single-sample callback and with
 * both (which must use the batch one alone). Every sample is hashed as 16 bit
 * values, low byte first, built byte by byte so the digest does not depend on
 * the host's byte order: mono on the Master System (left equals right), left
 * then right on the Game Gear. Left and right are also hashed apart and must
 * agree across the three runs. Every frame must hold 801 or 802 stereo frames
 * and the running total must equal floor(frames x 3,504,512 / 4,375). */
#define AUDIO_CAP 966u

static struct {
    cogmaster__sha256 all, left, right;
    int stereo;
    uint64_t delivered;
    unsigned batch_calls, single_calls, bad_calls, unequal;
    int16_t kept[AUDIO_CAP * 2u];
} aud;

struct audio_hashes {
    char all[65], left[65], right[65];
};

/* Hashes one stereo frame as the runner hashes it. */
static void audio_put(int16_t left, int16_t right)
{
    uint16_t l = (uint16_t)left, r = (uint16_t)right;
    uint8_t b[4];
    b[0] = (uint8_t)(l & 0xFFu);
    b[1] = (uint8_t)(l >> 8);
    b[2] = (uint8_t)(r & 0xFFu);
    b[3] = (uint8_t)(r >> 8);
    cogmaster__sha256_update(&aud.left, b, 2u);
    cogmaster__sha256_update(&aud.right, b + 2, 2u);
    cogmaster__sha256_update(&aud.all, b, aud.stereo ? 4u : 2u);
    if (left != right) {
        aud.unequal++;
    }
    aud.delivered++;
}

static void RETRO_CALLCONV audio_single(int16_t left, int16_t right)
{
    aud.single_calls++;
    audio_put(left, right);
}

/* The adapter reuses its buffer, so the frames are copied before use. */
static size_t RETRO_CALLCONV audio_batch(const int16_t *data, size_t frames)
{
    size_t i;
    aud.batch_calls++;
    if (frames == 0u || frames > AUDIO_CAP) {
        aud.bad_calls++;
        return frames;
    }
    memcpy(aud.kept, data, frames * 2u * sizeof data[0]);
    for (i = 0; i < frames; i++) {
        audio_put(aud.kept[2u * i], aud.kept[2u * i + 1u]);
    }
    return frames;
}

static void audio_begin(int stereo)
{
    memset(&aud, 0, sizeof aud);
    aud.stereo = stereo;
    cogmaster__sha256_init(&aud.all);
    cogmaster__sha256_init(&aud.left);
    cogmaster__sha256_init(&aud.right);
}

static void audio_hex(cogmaster__sha256 *s, char hex[65])
{
    uint8_t digest[32];
    cogmaster__sha256_final(s, digest);
    cogmaster__sha256_hex(digest, hex);
}

/* Loads the image, sets the frontend's callbacks and returns 0 on success. */
static int audio_load(const uint8_t *image, size_t size, int batch_on, int single_on)
{
    struct retro_game_info game = {0};
    p_set_environment(env);
    p_init();
    p_set_video_refresh(video);
    p_set_audio_sample(single_on ? audio_single : NULL);
    p_set_audio_sample_batch(batch_on ? audio_batch : NULL);
    p_set_input_poll(on_poll);
    p_set_input_state(input);
    game.data = image;
    game.size = size;
    if (!p_load_game(&game)) {
        p_deinit();
        return 1;
    }
    return 0;
}

static void audio_session(const char *name, const uint8_t *image, size_t size, unsigned frames,
                          int batch_on, int single_on, struct audio_hashes *out)
{
    unsigned f, bad_counts = 0u, bad_sums = 0u, bad_calls = 0u;
    audio_begin(want_w == 160u);
    CHECK_EQ_U64(audio_load(image, size, batch_on, single_on), 0u);
    for (f = 1u; f <= frames; f++) {
        uint64_t before = aud.delivered;
        unsigned calls = aud.batch_calls;
        (void)run_one();
        uint64_t n = aud.delivered - before;
        if (n != 801u && n != 802u) {
            bad_counts++;
        }
        if (aud.delivered != (uint64_t)f * 3504512u / 4375u) {
            bad_sums++;
        }
        if (batch_on && aud.batch_calls != calls + 1u) {
            bad_calls++;
        }
    }
    p_unload_game();
    p_deinit();
    if (bad_counts != 0u || bad_sums != 0u || bad_calls != 0u || aud.bad_calls != 0u) {
        fprintf(stderr,
                "%s: %u frames off 801 or 802, %u running totals off the formula, %u frames "
                "without exactly one batch call, %u empty or oversized batches\n",
                name, bad_counts, bad_sums, bad_calls, aud.bad_calls);
        check_failures++;
    }
    if (!batch_on && aud.batch_calls != 0u) {
        fprintf(stderr, "%s: the batch callback was called\n", name);
        check_failures++;
    }
    if (batch_on && single_on && aud.single_calls != 0u) {
        fprintf(stderr, "%s: the single-sample callback was called beside the batch one\n", name);
        check_failures++;
    }
    if (!aud.stereo && aud.unequal != 0u) {
        fprintf(stderr, "%s: %u frames with left and right apart on a mono machine\n", name,
                aud.unequal);
        check_failures++;
    }
    audio_hex(&aud.all, out->all);
    audio_hex(&aud.left, out->left);
    audio_hex(&aud.right, out->right);
}

/* No audio callback at all: the library is still drained, so the frame that
   follows holds a normal count and not the backlog. */
static void audio_neither(const uint8_t *image, size_t size)
{
    unsigned f;
    audio_begin(want_w == 160u);
    CHECK_EQ_U64(audio_load(image, size, 0, 0), 0u);
    for (f = 1u; f <= 10u; f++) {
        (void)run_one();
    }
    p_set_audio_sample_batch(audio_batch);
    (void)run_one();
    CHECK(aud.delivered == 801u || aud.delivered == 802u);
    p_unload_game();
    p_deinit();
}

static int audio_test(const char *path, const char *frames_arg, const char *machine,
                      const char *expected)
{
    static const struct {
        const char *name;
        int batch_on, single_on;
    } sessions[3] = {{"batch only", 1, 0}, {"single only", 0, 1}, {"both registered", 1, 1}};
    struct audio_hashes h[3];
    uint8_t image[32768];
    unsigned frames = parse(frames_arg), i;
    FILE *file = fopen(path, "rb");
    size_t n;
    if (file == NULL || frames == 0u ||
        (strcmp(machine, "sms") != 0 && strcmp(machine, "gg") != 0)) {
        fprintf(stderr, "usage: audio <image> <frames> sms|gg <sha256>\n");
        return 2;
    }
    n = fread(image, 1u, sizeof image, file);
    CHECK_EQ_U64(n, sizeof image);
    CHECK_EQ_U64(fclose(file), 0u);
    want_w = strcmp(machine, "gg") == 0 ? 160u : 256u;
    want_h = want_w == 160u ? 144u : 192u;
    for (i = 0u; i < 3u; i++) {
        audio_session(sessions[i].name, image, n, frames, sessions[i].batch_on,
                      sessions[i].single_on, &h[i]);
        check_string(sessions[i].name, h[i].all, expected);
        check_string(sessions[i].name, h[i].left, h[0].left);
        check_string(sessions[i].name, h[i].right, h[0].right);
    }
    CHECK_EQ_U64(video_bad, 0u);
    audio_neither(image, n);
    return check_failures != 0 ? 1 : 0;
}

/* Loads the image with the region and FM options given, runs it and prints the
   hash of the PCM the module delivered, one batch call per frame. A refused
   load fails. The test script compares the hash with the runner's. */
static int fm_test(const char *path, const char *frames_arg, const char *machine,
                   const char *region, const char *fm)
{
    uint8_t image[32768];
    char hex[65];
    unsigned frames = parse(frames_arg), f;
    FILE *file = fopen(path, "rb");
    size_t n;
    if (file == NULL || frames == 0u ||
        (strcmp(machine, "sms") != 0 && strcmp(machine, "gg") != 0)) {
        fprintf(stderr, "usage: fm_option <image> <frames> sms|gg <region> <fm>\n");
        return 2;
    }
    n = fread(image, 1u, sizeof image, file);
    CHECK_EQ_U64(n, sizeof image);
    CHECK_EQ_U64(fclose(file), 0u);
    want_w = strcmp(machine, "gg") == 0 ? 160u : 256u;
    want_h = want_w == 160u ? 144u : 192u;
    option_region = region;
    option_fm = fm;
    audio_begin(want_w == 160u);
    if (audio_load(image, n, 1, 0) != 0) {
        fprintf(stderr, "the module refused the content with fm %s\n", fm);
        return 1;
    }
    for (f = 1u; f <= frames; f++) {
        (void)run_one();
    }
    p_unload_game();
    p_deinit();
    CHECK_EQ_U64(aud.delivered, (uint64_t)frames * 3504512u / 4375u);
    audio_hex(&aud.all, hex);
    printf("%s\n", hex);
    return check_failures != 0 ? 1 : 0;
}
