/* The harness's definitions of the Z80 bus seam. The library's bus is not
   linked, so these are the only definitions. */
#include "sst_bus.h"

#include "internal.h"

struct sst_bus sst_bus;

static void touch(uint16_t addr)
{
    if (!sst_bus.touched[addr] && sst_bus.n_dirty < SST_MAX_RAM + SST_MAX_ACCESS) {
        sst_bus.touched[addr] = 1;
        sst_bus.dirty[sst_bus.n_dirty++] = addr;
    }
}

void sst_bus_load(const struct sst_test *t)
{
    for (unsigned k = 0; k < sst_bus.n_dirty; k++) {
        sst_bus.mem[sst_bus.dirty[k]] = 0;
        sst_bus.touched[sst_bus.dirty[k]] = 0;
    }
    sst_bus.n_dirty = 0;
    for (unsigned k = 0; k < t->n_ram_in; k++) {
        touch(t->ram_in[k].addr);
        sst_bus.mem[t->ram_in[k].addr] = t->ram_in[k].value;
    }
    sst_bus.test = t;
    for (unsigned k = 0; k < SST_MAX_PORTS; k++) {
        sst_bus.port_used[k] = 0;
    }
    sst_bus.n_log = 0;
    sst_bus.overflow = 0;
    sst_bus.ed_undefined = 0;
}

static void log_cycle(cogmaster *m, uint8_t kind, uint16_t addr, uint8_t data, uint16_t refresh)
{
    if (sst_bus.n_log == SST_MAX_ACCESS) {
        sst_bus.overflow++;
        return;
    }
    sst_bus.log[sst_bus.n_log++] = (struct sst_logged){kind, m->ticks, addr, data, refresh};
}

uint8_t cogmaster__bus_fetch(cogmaster *m, uint16_t addr, uint16_t refresh)
{
    uint8_t v = sst_bus.mem[addr];
    log_cycle(m, SST_M1, addr, v, refresh);
    return v;
}

uint8_t cogmaster__bus_read(cogmaster *m, uint16_t addr)
{
    uint8_t v = sst_bus.mem[addr];
    log_cycle(m, SST_MR, addr, v, 0);
    return v;
}

void cogmaster__bus_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    touch(addr);
    sst_bus.mem[addr] = v;
    log_cycle(m, SST_MW, addr, v, 0);
}

/* The test's next unused read on this port, in list order; 0xFF when the
   test lists none. */
uint8_t cogmaster__bus_in(cogmaster *m, uint16_t port)
{
    uint8_t v = 0xFF;
    const struct sst_test *t = sst_bus.test;
    for (unsigned k = 0; k < t->n_ports; k++) {
        if (!sst_bus.port_used[k] && t->ports[k].dir == 'r' && t->ports[k].addr == port) {
            sst_bus.port_used[k] = 1;
            v = t->ports[k].value;
            break;
        }
    }
    log_cycle(m, SST_IOR, port, v, 0);
    return v;
}

void cogmaster__bus_out(cogmaster *m, uint16_t port, uint8_t v)
{
    log_cycle(m, SST_IOW, port, v, 0);
}

void cogmaster__bus_ed_undefined(cogmaster *m, uint8_t op)
{
    (void)m;
    (void)op;
    sst_bus.ed_undefined++;
}
