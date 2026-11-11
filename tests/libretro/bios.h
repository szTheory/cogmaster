/* The BIOS boot option through the adapter, one leg per call. The cmake script that registers the
   test builds the system folders, asks the runner for the frame digest each leg must give, and
   passes the leg's setup and what it must see: the digest at frame K, how often the adapter asked
   for the system folder, how many lines it logged and a word that line must hold. The folder is
   given as a path, or as @null (the frontend answers NULL), @empty (an empty string), @long (a path
   no 512-byte buffer holds) or @false=PATH (the frontend answers false although it fills the path
   in). The adapter must fall back to the fast boot on every refusal, so the digest of a refusal is
   the fast one. */

static int bios_ends_with(const char *s, const char *tail)
{
    size_t n = strlen(s), t = strlen(tail);
    return n >= t && strcmp(s + n - t, tail) == 0;
}

/* a: cartridge, frames, digest, boot, region, folder, calls, lines, word. */
static int bios_test(char **a)
{
    static uint8_t image[262144];
    static char long_dir[600];
    FILE *f = fopen(a[0], "rb");
    if (f == NULL) {
        return 1;
    }
    size_t n = fread(image, 1u, sizeof image, f);
    CHECK(n != 0u && feof(f));
    CHECK_EQ_U64(fclose(f), 0u);
    int gg = bios_ends_with(a[0], ".gg");
    want_w = gg ? 160u : 256u;
    want_h = gg ? 144u : 192u;
    option_boot = a[3];
    option_region = a[4];
    if (strcmp(a[5], "@null") == 0) {
        system_dir = NULL;
    } else if (strcmp(a[5], "@empty") == 0) {
        system_dir = "";
    } else if (strcmp(a[5], "@long") == 0) {
        memset(long_dir, 'a', sizeof long_dir - 1u);
        system_dir = long_dir;
    } else if (strncmp(a[5], "@false=", 7u) == 0) {
        system_dir = a[5] + 7;
        system_dir_false = 1;
    } else {
        system_dir = a[5];
    }
    p_set_environment(env);
    p_init();
    set_callbacks();
    struct retro_game_info game = {0};
    game.data = image;
    game.size = n;
    bool loaded = p_load_game(&game);
    CHECK(loaded);
    if (loaded) {
        char hex[65];
        for (unsigned i = parse(a[1]); i != 0u; i--) {
            (void)run_one();
        }
        hash_frame(hex);
        check_string("frame digest", hex, a[2]);
        CHECK_EQ_U64(video_bad, 0u);
        CHECK_EQ_U64(system_dir_calls, parse(a[6]));
        CHECK_EQ_U64(log_lines, parse(a[7]));
        if (log_lines != 0u && strcmp(a[8], "-") != 0 && strstr(last_log, a[8]) == NULL) {
            fprintf(stderr, "log line \"%s\" lacks \"%s\"\n", last_log, a[8]);
            check_failures++;
        }
        /* The line names the file and never the folder. */
        if (log_lines != 0u && system_dir != NULL && system_dir[0] != '\0' &&
            strstr(last_log, system_dir) != NULL) {
            fprintf(stderr, "the log line holds the system folder\n");
            check_failures++;
        }
        CHECK_EQ_U64(options_calls, 1u);
        p_unload_game();
    }
    p_deinit();
    return loaded && check_failures == 0 ? 0 : 1;
}
