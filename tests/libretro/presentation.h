/* Public-API reference instance: presentation must never alter its native
   pixel sequence, machine identity, frame timing or emulation mode. */
static void *presentation_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}
static void presentation_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}
static int presentation_test(void)
{
    uint8_t image[32768] = {0};
    uint16_t reference[MAX_W * MAX_H];
    /* Mode-4 register writes, followed by a bounded JP loop. Registers
       R0/R1 choose the active height (SMS-VDP sections 2 and 4). */
    const uint8_t program[] = {0x3E, 0x04, 0xD3, 0xBF, 0x3E, 0x80, 0xD3, 0xBF, 0x3E, 0x40,
                               0xD3, 0xBF, 0x3E, 0x81, 0xD3, 0xBF, 0xC3, 0x10, 0x00};
    memcpy(image, program, sizeof program);
    memcpy(image + 0x7FF0u, "TMR SEGA", 8u);
    image[0x7FFFu] = 0x4Cu;
    cogmaster_config cfg = {0};
    cfg.size = sizeof cfg;
    cfg.allocator.alloc = presentation_alloc;
    cfg.allocator.free = presentation_free;
    cogmaster *ref = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &ref), COGMASTER_STATUS_OK);
    p_set_environment(env);
    p_init();
    set_callbacks();
    for (unsigned machine = 0u; machine < 3u; machine++) {
        for (unsigned height = 0u; height < 3u; height++) {
            for (unsigned pal = 0u; pal < (machine == 0u ? 2u : 1u); pal++) {
                image[1] = height == 0u ? 0x04u : 0x06u;
                image[9] = height == 1u ? 0x50u : (height == 2u ? 0x48u : 0x40u);
                option_machine = machine == 0u ? "sms1" : "gg";
                /* SMS1 cannot extend height; SMS2 exercises the full set. */
                if (height != 0u && machine == 0u)
                    option_machine = "sms2";
                option_mode = machine == 2u ? "sms" : "auto";
                option_region = pal ? "pal" : "auto";
                option_view = "native";
                option_aspect = "6:5";
                cogmaster_media media = {0};
                media.size = sizeof media;
                media.image = image;
                media.image_size = sizeof image;
                media.machine = machine == 0u
                                    ? (height ? COGMASTER_MACHINE_SMS2 : COGMASTER_MACHINE_SMS1)
                                    : COGMASTER_MACHINE_GG;
                media.tv = pal ? COGMASTER_TV_PAL : COGMASTER_TV_AUTO;
                media.video_mode = machine == 2u ? COGMASTER_VIDEO_SMS : COGMASTER_VIDEO_AUTO;
                CHECK_EQ_U64(cogmaster_load(ref, &media), COGMASTER_STATUS_OK);
                struct retro_game_info game = {0};
                game.data = image;
                game.size = sizeof image;
                CHECK(p_load_game(&game));
                CHECK_EQ_U64(p_get_region(), pal ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
                struct retro_system_av_info av;
                p_get_system_av_info(&av);
                float last_aspect = av.geometry.aspect_ratio;
                unsigned last_w = av.geometry.base_width, last_h = av.geometry.base_height;
                size_t samples = 0u;
                /* PAL exercises enough remainders to detect a clamped 802-frame
                   buffer; both callback fallbacks read every reported sample. */
                unsigned frames = pal ? 80u : 8u;
                for (unsigned n = 1u; n <= frames; n++) {
                    if (n == 3u) {
                        option_aspect = "square";
                        option_update = true;
                    }
                    if (n == 4u) {
                        option_view = "full";
                        option_update = true;
                    }
                    if (n == 6u) {
                        option_view = "native";
                        option_update = true;
                    }
                    /* Load-time changes must not switch the running machine. */
                    if (n == 7u) {
                        option_machine = "sms2";
                        option_mode = "sms";
                        option_update = true;
                    }
                    CHECK_EQ_U64(cogmaster_run_frame(ref, NULL), COGMASTER_STATUS_OK);
                    cogmaster_info ri = {0};
                    ri.size = sizeof ri;
                    CHECK_EQ_U64(cogmaster_get_info(ref, &ri), COGMASTER_STATUS_OK);
                    bool full = machine == 1u && strcmp(option_view, "full") == 0;
                    cogmaster_frame rf = {0};
                    rf.size = sizeof rf;
                    rf.pixels = reference;
                    rf.pitch_bytes = MAX_W * 2u;
                    rf.flags = full ? COGMASTER_FRAME_FULL_FIELD : 0u;
                    CHECK_EQ_U64(cogmaster_get_frame(ref, &rf), COGMASTER_STATUS_OK);
                    want_w = rf.w;
                    want_h = rf.h;
                    uint32_t pn = ri.par_num, pd = ri.par_den;
                    if (machine == 1u && strcmp(option_aspect, "square") == 0)
                        pn = pd = 1u;
                    float aspect = (float)((double)rf.w * pn / ((double)rf.h * pd));
                    unsigned before = geometry_calls, vb = video_calls;
                    samples += run_one();
                    CHECK_EQ_U64(video_calls, vb + 1u);
                    bool changed = rf.w != last_w || rf.h != last_h || aspect != last_aspect;
                    CHECK_EQ_U64(geometry_calls, before + (changed ? 1u : 0u));
                    if (changed) {
                        CHECK_EQ_U64(notified.base_width, rf.w);
                        CHECK_EQ_U64(notified.base_height, rf.h);
                        CHECK_EQ_DBL(notified.aspect_ratio, aspect);
                    }
                    p_get_system_av_info(&av);
                    CHECK_EQ_DBL(av.geometry.aspect_ratio, aspect);
                    CHECK_EQ_DBL(av.timing.fps, (double)ri.fps_num / ri.fps_den);
                    CHECK_EQ_U64(samples, (uint64_t)n * 48000u * ri.fps_den / ri.fps_num);
                    for (uint32_t y = 0u; y < rf.h; y++) {
                        for (uint32_t x = 0u; x < rf.w; x++) {
                            uint16_t dot = 0u;
                            CHECK(to_rgb444(y * rf.w + x, &dot));
                            CHECK_EQ_HEX(dot, reference[y * MAX_W + x]);
                        }
                    }
                    last_w = rf.w;
                    last_h = rf.h;
                    last_aspect = aspect;
                }
            }
        }
    }
    CHECK_EQ_U64(video_bad, 0u);
    CHECK_EQ_U64(batch_nonzero + sample_nonzero, 0u);
    p_deinit();
    cogmaster_destroy(ref);
    return check_failures != 0 ? 1 : 0;
}
