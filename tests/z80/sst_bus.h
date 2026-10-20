/* The vector harness's bus: memory and ports served from one test's lists,
   and a log of every bus cycle the Z80 makes. */
#ifndef COGMASTER_SST_BUS_H
#define COGMASTER_SST_BUS_H

#include <stdint.h>

#include "sst_stream.h"

struct sst_logged {
    uint8_t kind;
    uint64_t tick; /* m->ticks when the seam was called */
    uint16_t addr;
    uint8_t data;
    uint16_t refresh;
};

struct sst_bus {
    uint8_t mem[65536];
    uint8_t touched[65536]; /* 1 where mem may be non-zero */
    uint16_t dirty[SST_MAX_RAM + SST_MAX_ACCESS];
    unsigned n_dirty;
    const struct sst_test *test;
    uint8_t port_used[SST_MAX_PORTS];
    struct sst_logged log[SST_MAX_ACCESS];
    unsigned n_log;
    unsigned overflow; /* bus cycles past SST_MAX_ACCESS */
    unsigned ed_undefined;
};

extern struct sst_bus sst_bus;

/* Zeroes the memory the previous test touched, loads t's initial RAM and
   ports and clears the log. */
void sst_bus_load(const struct sst_test *t);

#endif /* COGMASTER_SST_BUS_H */
