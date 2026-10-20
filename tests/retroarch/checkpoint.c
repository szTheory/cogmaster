/* Test-only replay checkpoint forwarding; input/video remain production source. */
#define retro_serialize_size replay_unsupported_size
#define retro_serialize replay_unsupported_save
#define retro_unserialize replay_unsupported_load
#include "../../libretro/libretro.c"
#include "../../src/state.h"
#undef retro_serialize_size
#undef retro_serialize
#undef retro_unserialize
RETRO_API size_t retro_serialize_size(void);
RETRO_API bool retro_serialize(void *data, size_t size);
RETRO_API bool retro_unserialize(const void *data, size_t size);
size_t retro_serialize_size(void)
{
    return cogmaster__state_size(inst);
}
bool retro_serialize(void *data, size_t size)
{
    return cogmaster__state_save(inst, data, size) == COGMASTER_STATUS_OK;
}
bool retro_unserialize(const void *data, size_t size)
{
    return cogmaster__state_load(inst, data, size) == COGMASTER_STATUS_OK;
}
