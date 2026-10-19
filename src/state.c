/* The internal state visitor over the test-card instance. */
#include "state.h"

size_t cogmaster__state_size(const cogmaster *m)
{
    (void)m;
    return 0u;
}

cogmaster_status cogmaster__state_save(const cogmaster *m, void *buf, size_t capacity)
{
    (void)m;
    (void)buf;
    (void)capacity;
    return COGMASTER_STATUS_ERR_ARGUMENT;
}

cogmaster_status cogmaster__state_load(cogmaster *m, const void *buf, size_t size)
{
    (void)m;
    (void)buf;
    (void)size;
    return COGMASTER_STATUS_ERR_ARGUMENT;
}

size_t cogmaster__state_record(const cogmaster *m, struct cogmaster__state_field *out, size_t cap)
{
    (void)m;
    (void)out;
    (void)cap;
    return 0u;
}

void cogmaster__state_seal(void *buf, size_t size)
{
    (void)buf;
    (void)size;
}
