/* Host presentation is outside native pixels and state. libretro.h's v2
   descriptors are static storage because frontends retain their pointers. */
#ifndef COGMASTER_LR_OPTIONS_H
#define COGMASTER_LR_OPTIONS_H
static struct retro_core_option_v2_definition lr_definitions[] = {
    {.key = "cogmaster_machine",
     .desc = "Machine (reload content)",
     .info = "Select the console profile. Takes effect when content is loaded.",
     .values = {{"auto", "Automatic"},
                {"sms2", "Master System II"},
                {"sms1", "Master System"},
                {"gg", "Game Gear"},
                {NULL, NULL}},
     .default_value = "auto"},
    {.key = "cogmaster_gg_mode",
     .desc = "Game Gear mode (reload content)",
     .info = "Select native Game Gear or SMS compatibility. Takes effect on content load.",
     .values = {{"auto", "Automatic"},
                {"gg", "Native Game Gear"},
                {"sms", "SMS compatibility"},
                {NULL, NULL}},
     .default_value = "auto"},
    {.key = "cogmaster_gg_aspect",
     .desc = "Game Gear pixel aspect",
     .info = "Presentation only. Native LCD crop displays at 4:3 or square-pixel 10:9.",
     .values = {{"6:5", "6:5 pixels (4:3 LCD)"},
                {"square", "Square pixels (10:9 LCD)"},
                {NULL, NULL}},
     .default_value = "6:5"},
    {.key = "cogmaster_gg_view",
     .desc = "Game Gear view",
     .info = "Full field exposes normally hidden content without changing emulation mode.",
     .values = {{"native", "Native LCD crop"},
                {"full", "Full VDP field (hidden content)"},
                {NULL, NULL}},
     .default_value = "native"},
    {.key = "cogmaster_region",
     .desc = "Region and timing (reload content)",
     .info = "Takes effect on content load. PAL is unavailable on Game Gear or Japanese consoles.",
     .values = {{"auto", "Automatic"},
                {"ntsc-u", "Export NTSC"},
                {"ntsc-j", "Japan NTSC"},
                {"pal", "Export PAL"},
                {NULL, NULL}},
     .default_value = "auto"},
    {0}};
static struct retro_core_options_v2 lr_options = {NULL, lr_definitions};

static void lr_register_options(retro_environment_t cb)
{
    unsigned version = 0u;
    if (cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2u) {
        (void)cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &lr_options);
    } else if (version >= 1u) {
        static struct retro_core_option_definition definitions[6];
        for (unsigned i = 0u; i < 5u; i++) {
            definitions[i].key = lr_definitions[i].key;
            definitions[i].desc = lr_definitions[i].desc;
            definitions[i].info = lr_definitions[i].info;
            memcpy(definitions[i].values, lr_definitions[i].values, sizeof definitions[i].values);
            definitions[i].default_value = lr_definitions[i].default_value;
        }
        (void)cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, definitions);
    } else {
        static struct retro_variable variables[] = {
            {"cogmaster_machine", "Machine (reload content); auto|sms2|sms1|gg"},
            {"cogmaster_gg_mode", "Game Gear mode (reload content); auto|gg|sms"},
            {"cogmaster_gg_aspect", "Game Gear pixel aspect; 6:5|square"},
            {"cogmaster_gg_view", "Game Gear view; native|full"},
            {"cogmaster_region", "Region and timing (reload content); auto|ntsc-u|ntsc-j|pal"},
            {NULL, NULL}};
        (void)cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
    }
}

static inline int lr_option(const char *key, const char *value)
{
    struct retro_variable variable = {key, NULL};
    return env_cb != NULL && env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) &&
           variable.value != NULL && strcmp(variable.value, value) == 0;
}
#endif
