/* Pin mixing and input history through the public frame boundary. */
#include "state.h"

static void input_frame(cogmaster *m, const cogmaster_input *in)
{
    CHECK_EQ_U64(cogmaster_set_input(m, in), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster_run_frame(m, NULL), COGMASTER_STATUS_OK);
}

static void test_pad_pins(void)
{
    const uint32_t machines[] = {COGMASTER_MACHINE_SMS1, COGMASTER_MACHINE_SMS2,
                                 COGMASTER_MACHINE_GG};
    for (uint32_t model = 0u; model < 3u; model++) {
        for (uint32_t region = COGMASTER_REGION_EXPORT; region <= COGMASTER_REGION_JAPAN;
             region++) {
            cogmaster *m = load(machines[model], region, COGMASTER_TV_NTSC);
            cogmaster_input in = {0};
            in.size = sizeof in;
            for (uint32_t pad = 0u; pad < 2u; pad++) {
                for (uint32_t bit = 0u; bit < 6u; bit++) {
                    in.p1 = pad == 0u ? 1u << bit : 0u;
                    in.p2 = pad == 1u ? 1u << bit : 0u;
                    input_frame(m, &in);
                    uint32_t dc =
                        pad == 0u ? 1u << bit : (model != 2u && bit < 2u ? 1u << (bit + 6u) : 0u);
                    uint32_t dd = pad == 1u && model != 2u && bit >= 2u ? 1u << (bit - 2u) : 0u;
                    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDCu), 255u ^ dc);
                    CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu), 255u ^ dd);
                }
            }
            in.p1 = 63u;
            in.p2 = 63u;
            in.reset = 1u;
            input_frame(m, &in);
            CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDCu), model == 2u ? 0xC0u : 0u);
            CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu),
                         model == 2u
                             ? 0xFFu
                             : (model == 0u && region == COGMASTER_REGION_EXPORT ? 0xE0u : 0xF0u));
            if (model != 2u) {
                /* Both TR pins output high override pressed button 2. */
                cogmaster__bus_out(m, 0x3Fu, 0xFAu);
                CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDCu) & 0x20u,
                             region == COGMASTER_REGION_EXPORT ? 0x20u : 0u);
                CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu) & 8u,
                             region == COGMASTER_REGION_EXPORT ? 8u : 0u);
                in.p1 = in.p2 = 0u;
                input_frame(m, &in);
                cogmaster__bus_out(m, 0x3Fu, 0xAAu);
                CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDCu) & 0x20u, 0u);
                CHECK_EQ_HEX(cogmaster__bus_in(m, 0xDDu) & 8u, 0u);
            }
            cogmaster_destroy(m);
        }
    }
}

static void test_pause_history(void)
{
    cogmaster *a = export_sms(), *b = export_sms();
    /* A guest NMI handler increments work RAM then returns. */
    static const uint8_t handler[] = {0x21u, 0x00u, 0xC1u, 0x34u, 0xEDu, 0x45u};
    for (uint32_t k = 0u; k < sizeof handler; k++) {
        a->cart.rom[0x66u + k] = handler[k];
        b->cart.rom[0x66u + k] = handler[k];
    }
    cogmaster_input in = {0};
    in.size = sizeof in;
    in.pause = 1u;
    input_frame(a, &in);
    input_frame(a, &in);
    CHECK_EQ_U64(a->ram[0x100], 1u);
    CHECK_EQ_U64(b->ram[0x100], 0u);
    in.port1 = COGMASTER_DEVICE_NONE;
    input_frame(a, &in);
    CHECK_EQ_U64(a->ram[0x100], 1u);
    in.pause = 0u;
    input_frame(a, &in);
    in.port1 = COGMASTER_DEVICE_PAD;
    in.pause = 1u;
    input_frame(a, &in);
    input_frame(a, &in);
    CHECK_EQ_U64(a->ram[0x100], 1u);
    /* Interleaving another instance cannot rearm a held edge. */
    input_frame(b, &in);
    CHECK_EQ_U64(b->ram[0x100], 1u);
    in.pause = 0u;
    input_frame(a, &in);
    in.pause = 1u;
    input_frame(a, &in);
    CHECK_EQ_U64(a->ram[0x100], 2u);
    CHECK_EQ_U64(b->ram[0x100], 1u);
    static uint8_t state[400000];
    size_t n = cogmaster__state_size(a);
    CHECK(n <= sizeof state);
    CHECK_EQ_U64(cogmaster__state_save(a, state, sizeof state), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(cogmaster__state_load(b, state, n), COGMASTER_STATUS_OK);
    input_frame(b, &in);
    CHECK_EQ_U64(b->ram[0x100], 2u);
    /* Corrupt each serialized input/lag field, reseal, and prove refusal
       does not partially apply any state, including pending input. */
    size_t count = cogmaster__state_record(b, NULL, 0u);
    struct cogmaster__state_field *fields = malloc(count * sizeof *fields);
    CHECK(fields != NULL);
    if (fields != NULL) {
        cogmaster__state_record(b, fields, count);
        static uint8_t snapshot[sizeof(struct cogmaster)];
        memcpy(snapshot, b, sizeof *b);
        for (size_t k = 0u; k < count; k++) {
            if (strncmp(fields[k].name, "input_", 6u) == 0) {
                size_t off = fields[k].offset;
                uint8_t old = state[off];
                state[off] = 255u;
                cogmaster__state_seal(state, n);
                CHECK_EQ_U64(cogmaster__state_load(b, state, n), COGMASTER_STATUS_ERR_ARGUMENT);
                CHECK(memcmp(snapshot, b, sizeof *b) == 0);
                state[off] = old;
            }
        }
        free(fields);
    }
    /* An invalid absolute input leaves pending input and history untouched. */
    in.p2 = 64u;
    CHECK_EQ_U64(cogmaster_set_input(b, &in), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_run_frame(b, NULL), COGMASTER_STATUS_OK);
    CHECK_EQ_U64(b->ram[0x100], 2u);
    cogmaster_destroy(a);
    cogmaster_destroy(b);
}

static void test_lag_decode(void)
{
    for (uint32_t profile = 0u; profile < 4u; profile++) {
        cogmaster *m = profile == 0u   ? export_sms()
                       : profile == 1u ? japan_sms()
                                       : gg(COGMASTER_REGION_EXPORT);
        if (profile == 3u)
            m->video_mode = COGMASTER_VIDEO_SMS;
        for (uint32_t p = 0u; p < 256u; p++) {
            m->input_read = 0u;
            (void)cogmaster__bus_in(m, (uint16_t)(0x1200u | p));
            int decoded = profile == 0u ? p >= 0xC0u
                                        : p == 0xDCu || p == 0xDDu ||
                                              (profile >= 2u && (p == 0xC0u || p == 0xC1u)) ||
                                              (profile == 2u && p == 0u);
            CHECK_EQ_U64(m->input_read, decoded ? 1u : 0u);
        }
        m->input_read = 0u;
        (void)cogmaster_peek(m, COGMASTER_SPACE_BUS, 0xDCu);
        CHECK_EQ_U64(m->input_read, 0u);
        if (profile < 2u) {
            cogmaster__bus_out(m, 0x3Eu, 0xAFu);
            (void)cogmaster__bus_in(m, 0xDCu);
            CHECK_EQ_U64(m->input_read, 0u);
        }
        cogmaster_run_result r = {0};
        r.size = sizeof r;
        CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(r.lagged, 1u);
        /* A guest IN loop, rather than host polling, clears the flag. */
        static const uint8_t poll[] = {0xDBu, 0xDCu, 0xC3u, 0u, 0u};
        memcpy(m->cart.rom, poll, sizeof poll);
        CHECK_EQ_U64(cogmaster_reset(m, COGMASTER_RESET_POWER), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(cogmaster_run_frame(m, &r), COGMASTER_STATUS_OK);
        CHECK_EQ_U64(r.lagged, 0u);
        cogmaster_destroy(m);
    }
}
