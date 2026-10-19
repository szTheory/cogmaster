/* The first-fault record and cogmaster_get_fault (ARCHITECTURE section 8).
   Recording costs one branch; nothing in this release records a fault. */
#include <string.h>

#include "internal.h"

void cogmaster__fault_record(cogmaster *m, const struct cogmaster__fault *f)
{
    if (m->fault_present) {
        return;
    }
    m->fault = *f;
    m->fault_present = 1u;
}

cogmaster_status cogmaster_get_fault(const cogmaster *m, cogmaster_fault *out)
{
    if (m == NULL || out == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_out_size(out, COGMASTER__FIRST_SIZE_FAULT);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    cogmaster_fault r;
    memset(&r, 0, sizeof r);
    r.size = out->size;
    if (m->fault_present) {
        r.present = 1u;
        r.frame = m->fault.frame;
        r.line = m->fault.line;
        r.dot = m->fault.dot;
        r.pc = m->fault.pc;
        r.addr = m->fault.addr;
        r.opcode = m->fault.opcode;
        r.width = m->fault.width;
        r.device = m->fault.device;
        r.kind = m->fault.kind;
    }
    cogmaster__write_out(out, &r, (uint32_t)sizeof r);
    return COGMASTER_STATUS_OK;
}
