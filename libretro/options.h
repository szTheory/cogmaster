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
    {.key = "cogmaster_fm",
     .desc = "FM sound unit (reload content)",
     .info =
         "Automatic enables the YM2413 for Japanese Master System content only. Takes effect on "
         "content load; On is Automatic for Game Gear content.",
     .values = {{"auto", "Automatic"}, {"off", "Off"}, {"on", "On"}, {NULL, NULL}},
     .default_value = "auto"},
    {.key = "cogmaster_port1",
     .desc = "Port 1 device (reload content)",
     .info = "Automatic takes the controller the game was made for. Choose Control Pad to force a "
             "plain pad. A device this console cannot take falls back to the pad. Takes effect on "
             "content load.",
     .values = {{"auto", "Automatic"},
                {"pad", "Control Pad"},
                {"paddle", "Paddle"},
                {"sports", "Sports Pad"},
                {"lphaser", "Light Phaser"},
                {"none", "Nothing"},
                {NULL, NULL}},
     .default_value = "auto"},
    {.key = "cogmaster_port2",
     .desc = "Port 2 device (reload content)",
     .info =
         "As port 1. A Sports Pad, Paddle or Light Phaser moves with the mouse, the left stick and "
         "the D-pad. Takes effect on content load.",
     .values = {{"auto", "Automatic"},
                {"pad", "Control Pad"},
                {"paddle", "Paddle"},
                {"sports", "Sports Pad"},
                {"lphaser", "Light Phaser"},
                {"none", "Nothing"},
                {NULL, NULL}},
     .default_value = "auto"},
    {.key = "cogmaster_3d_view",
     .desc = "3-D glasses view",
     .info = "Applies only to a game that alternates the shutter every frame. Left or Right "
             "shows one eye, Alternate and Off send every frame as it is, Anaglyph mixes both "
             "eyes for red-cyan glasses.",
     .values = {{"left", "Left eye"},
                {"right", "Right eye"},
                {"alternate", "Alternate (flicker)"},
                {"anaglyph", "Red-cyan anaglyph"},
                {"off", "Off"},
                {NULL, NULL}},
     .default_value = "left"},
    {.key = "cogmaster_crosshair",
     .desc = "Light Phaser crosshair",
     .info = "Presentation only. Marks the aim of a Light Phaser on the picture; the game and "
             "every recording never see it.",
     .values = {{"on", "On"}, {"off", "Off"}, {NULL, NULL}},
     .default_value = "on"},
    {.key = "cogmaster_crosshair_colour",
     .desc = "Crosshair colour",
     .info = "Presentation only. The colour of the Light Phaser crosshair.",
     .values = {{"white", "White"},
                {"red", "Red"},
                {"green", "Green"},
                {"yellow", "Yellow"},
                {NULL, NULL}},
     .default_value = "white"},
    {0}};
/* The options every table below must hold; the v2, v1 and v0 tables are separate, and the
   host exercises only v2, so a later option cannot miss one without failing the build. */
#define LR_OPTION_COUNT 11u
_Static_assert(sizeof lr_definitions / sizeof lr_definitions[0] == LR_OPTION_COUNT + 1u,
               "v2 option table");
static struct retro_core_options_v2 lr_options = {NULL, lr_definitions};

static struct retro_core_option_definition lr_v1_definitions[LR_OPTION_COUNT + 1u];
static struct retro_variable lr_variables[] = {
    {"cogmaster_machine", "Machine (reload content); auto|sms2|sms1|gg"},
    {"cogmaster_gg_mode", "Game Gear mode (reload content); auto|gg|sms"},
    {"cogmaster_gg_aspect", "Game Gear pixel aspect; 6:5|square"},
    {"cogmaster_gg_view", "Game Gear view; native|full"},
    {"cogmaster_region", "Region and timing (reload content); auto|ntsc-u|ntsc-j|pal"},
    {"cogmaster_fm", "FM sound unit (reload content); auto|off|on"},
    {"cogmaster_port1", "Port 1 device (reload content); auto|pad|paddle|sports|lphaser|none"},
    {"cogmaster_port2", "Port 2 device (reload content); auto|pad|paddle|sports|lphaser|none"},
    {"cogmaster_3d_view", "3-D glasses view; left|right|alternate|anaglyph|off"},
    {"cogmaster_crosshair", "Light Phaser crosshair; on|off"},
    {"cogmaster_crosshair_colour", "Crosshair colour; white|red|green|yellow"},
    {NULL, NULL}};
_Static_assert(sizeof lr_variables / sizeof lr_variables[0] == LR_OPTION_COUNT + 1u,
               "v0 option table");

static void lr_register_options(retro_environment_t cb)
{
    unsigned version = 0u;
    if (cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2u) {
        (void)cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &lr_options);
    } else if (version >= 1u) {
        for (unsigned i = 0u; i < LR_OPTION_COUNT; i++) {
            lr_v1_definitions[i].key = lr_definitions[i].key;
            lr_v1_definitions[i].desc = lr_definitions[i].desc;
            lr_v1_definitions[i].info = lr_definitions[i].info;
            memcpy(lr_v1_definitions[i].values, lr_definitions[i].values,
                   sizeof lr_v1_definitions[i].values);
            lr_v1_definitions[i].default_value = lr_definitions[i].default_value;
        }
        (void)cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, lr_v1_definitions);
    } else {
        (void)cb(RETRO_ENVIRONMENT_SET_VARIABLES, lr_variables);
    }
}

static inline int lr_option(const char *key, const char *value)
{
    struct retro_variable variable = {key, NULL};
    return env_cb != NULL && env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) &&
           variable.value != NULL && strcmp(variable.value, value) == 0;
}
#endif
