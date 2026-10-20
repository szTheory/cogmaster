/* Classified persistence is independent of mapper enable registers and RAM
   decoding. See docs/hardware/cartridges.md and docs/embedding.md. */
#include <string.h>
#include "internal.h"

uint32_t cogmaster__battery_length(const cogmaster *m)
{
    const struct cogmaster__cart *c = &m->cart;
    if (c->persistence != COGMASTER_PERSISTENCE_BATTERY ||
        c->persistence_geometry != COGMASTER_GEOMETRY_SUPPORTED || c->ram == NULL ||
        c->persistent_size > c->ram_size)
        return 0u;
    return c->persistent_size;
}

void cogmaster__battery_write(cogmaster *m, uint32_t offset, uint8_t value)
{
    m->cart.ram[offset] = value;
    if (offset < cogmaster__battery_length(m))
        m->cart.generation++;
}

cogmaster_status cogmaster_get_battery(const cogmaster *m, cogmaster_battery *out)
{
    if (m == NULL || out == NULL)
        return COGMASTER_STATUS_ERR_ARGUMENT;
    cogmaster_status st = cogmaster__check_out_size(out, (uint32_t)sizeof *out);
    if (st != COGMASTER_STATUS_OK)
        return st;
    if (!m->loaded)
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    cogmaster_battery b;
    memset(&b, 0, sizeof b);
    b.size = out->size;
    b.length = cogmaster__battery_length(m);
    if (b.length != 0u) {
        b.data = m->cart.ram;
        b.knowledge = COGMASTER_BATTERY_KNOWN;
        b.generation = m->cart.generation;
    } else if (m->cart.rom == NULL || m->cart.persistence == COGMASTER_PERSISTENCE_NONE) {
        b.knowledge = COGMASTER_BATTERY_NONE;
    } else if (m->cart.persistence == COGMASTER_PERSISTENCE_BATTERY) {
        b.knowledge = COGMASTER_BATTERY_UNSUPPORTED;
    }
    cogmaster__write_out(out, &b, (uint32_t)sizeof b);
    return COGMASTER_STATUS_OK;
}
