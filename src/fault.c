/* The first-fault record and cogmaster_get_fault. */
#include "internal.h"

void cogmaster__fault_record(cogmaster *m, const struct cogmaster__fault *f)
{
    (void)m;
    (void)f;
}

cogmaster_status cogmaster_get_fault(const cogmaster *m, cogmaster_fault *out)
{
    (void)m;
    (void)out;
    return COGMASTER_STATUS_ERR_ARGUMENT;
}
