/* A frontend owns save bytes; the adapter only exposes classified RAM. */
static int battery_test(const char *path, const char *machine)
{
    static uint8_t image[65536], saved[32768];
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    if (f == NULL)
        return 1;
    CHECK_EQ_U64(fread(image, 1u, sizeof image, f), sizeof image);
    CHECK_EQ_U64(fclose(f), 0u);
    option_machine = machine;
    bool gg = strcmp(machine, "gg") == 0;
    option_mode = gg ? "gg" : "sms";
    want_w = gg ? 160u : 256u;
    want_h = gg ? 144u : 192u;
    p_set_environment(env);
    p_init();
    set_callbacks();
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    struct retro_game_info game = {0};
    game.data = image;
    game.size = sizeof image;
    CHECK(p_load_game(&game));
    uint8_t *data = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(data != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), sizeof saved);
    if (data == NULL) {
        p_deinit();
        return 1;
    }
    for (size_t i = 0; i < sizeof saved; i++)
        CHECK_EQ_HEX(data[i], 0xffu);
    /* The frontend restores before the guest enables RAM, through the
       ordinary writable pointer, with no adapter import or file API. */
    data[0] = 0xa5u;
    data[1] = 0x5au;
    data[2] = 7u;
    for (uint32_t i = 0; i < 4u; i++)
        (void)run_one();
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == data);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), sizeof saved);
    CHECK_EQ_U64(data[2], 8u);
    memcpy(saved, data, sizeof saved);
    p_reset();
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == data);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), sizeof saved);
    CHECK(memcmp(saved, data, sizeof saved) == 0);
    for (uint32_t i = 0; i < 4u; i++)
        (void)run_one();
    CHECK_EQ_U64(data[2], 9u);
    for (size_t i = 3u; i < sizeof saved; i++)
        CHECK_EQ_HEX(data[i], 0xffu);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 0u);
    CHECK(p_get_memory_data(RETRO_MEMORY_RTC) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_RTC), 0u);
    p_unload_game();
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    CHECK(p_load_game(&game));
    data = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(data != NULL);
    if (data != NULL) {
        for (size_t i = 0; i < sizeof saved; i++)
            CHECK_EQ_HEX(data[i], 0xffu);
        memcpy(data, saved, sizeof saved);
        for (uint32_t i = 0; i < 4u; i++)
            (void)run_one();
        CHECK_EQ_U64(data[2], 9u);
    }
    /* Same playable board with an unknown hash must not export guessed RAM. */
    image[sizeof image - 1u] ^= 1u;
    CHECK(p_load_game(&game));
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    CHECK(p_load_game(NULL));
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    /* A refused reload cannot retain a previous game's public span. */
    game.size = 1u;
    CHECK(!p_load_game(&game));
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    CHECK_EQ_U64(video_bad, 0u);
    p_deinit();
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    return check_failures != 0 ? 1 : 0;
}
