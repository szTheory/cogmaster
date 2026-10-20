/* The I/O ports per machine and region: a small image is loaded through the
   public API, and the ports are read and written through the bus in
   src/internal.h: cogmaster-test-io <case>. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cogmaster.h"
#include "internal.h"
#include "../check.h"

#define IMAGE_SIZE 1024u
#define STAMP_AT 0x0100u
#define STAMP 0x5Au

static uint8_t image[IMAGE_SIZE];

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* A new instance of the profile with a 1 KiB image of HALTs stamped at
   $0100. */
static cogmaster *load(uint32_t machine, uint32_t region, uint32_t tv)
{
    cogmaster_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    cogmaster *m = NULL;
    CHECK_EQ_U64(cogmaster_create(&cfg, &m), COGMASTER_STATUS_OK);
    if (m == NULL) {
        exit(1);
    }
    memset(image, 0x76, sizeof image);
    image[STAMP_AT] = STAMP;
    cogmaster_media media;
    memset(&media, 0, sizeof media);
    media.size = (uint32_t)sizeof media;
    media.image = image;
    media.image_size = sizeof image;
    media.machine = machine;
    media.region = region;
    media.tv = tv;
    CHECK_EQ_U64(cogmaster_load(m, &media), COGMASTER_STATUS_OK);
    return m;
}

static cogmaster *export_sms(void)
{
    return load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_NTSC);
}

static cogmaster *japan_sms(void)
{
    return load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, COGMASTER_TV_NTSC);
}

static cogmaster *gg(uint32_t region)
{
    return load(COGMASTER_MACHINE_GG, region, COGMASTER_TV_NTSC);
}

static cogmaster_fault fault_of(const cogmaster *m)
{
    cogmaster_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(cogmaster_get_fault(m, &f), COGMASTER_STATUS_OK);
    return f;
}

/* $3E b6 disables the cartridge and b4 work RAM: their reads return open
   bus, counted, and record unmapped_read on the cartridge or on RAM;
   re-enabling brings the bytes back. Toggling the bits alone is no fault. */
static void test_port3e(void)
{
    cogmaster *m = export_sms();
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xAB);
    cogmaster__bus_write(m, 0xC010u, 0x11u);
    for (uint32_t i = 0; i < 4u; i++) {
        cogmaster__bus_out(m, 0x003Eu, 0xEBu);
        cogmaster__bus_out(m, 0x003Eu, 0xBBu);
        cogmaster__bus_out(m, 0x003Eu, 0xABu);
    }
    CHECK_EQ_U64(fault_of(m).present, 0);
    CHECK_EQ_HEX(cogmaster__bus_read(m, STAMP_AT), STAMP);
    cogmaster__bus_out(m, 0x003Eu, 0xEBu);
    uint32_t before = m->counters.open_bus_reads;
    CHECK_EQ_HEX(cogmaster__bus_read(m, STAMP_AT), 0xFF);
    CHECK_EQ_U64(m->counters.open_bus_reads, before + 1u);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xC010u), 0x11);
    cogmaster_fault f = fault_of(m);
    CHECK_EQ_U64(f.present, 1);
    CHECK_EQ_U64(f.kind, COGMASTER__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(f.device, COGMASTER__DEVICE_CART);
    CHECK_EQ_HEX(f.addr, STAMP_AT);
    cogmaster__bus_out(m, 0x003Eu, 0xABu);
    CHECK_EQ_HEX(cogmaster__bus_read(m, STAMP_AT), STAMP);
    cogmaster_destroy(m);

    m = export_sms();
    cogmaster__bus_write(m, 0xC010u, 0x11u);
    cogmaster__bus_out(m, 0x003Eu, 0xBBu);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xC010u), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xE010u), 0xFF);
    CHECK_EQ_HEX(cogmaster__bus_read(m, STAMP_AT), STAMP);
    f = fault_of(m);
    CHECK_EQ_U64(f.kind, COGMASTER__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(f.device, COGMASTER__DEVICE_RAM);
    CHECK_EQ_HEX(f.addr, 0xC010);
    cogmaster__bus_out(m, 0x003Eu, 0xABu);
    CHECK_EQ_HEX(cogmaster__bus_read(m, 0xC010u), 0x11);
    cogmaster_destroy(m);
}

/* Region detection: $3F = $F5 makes both TH lines outputs at 1, $55 at 0.
   The export console reads them back on $DD b7-6 (%11, then %00); the
   Japanese console does not, so the first answer differs. $FF makes them
   inputs again, pulled up. */
static void test_port3f_region(void)
{
    const uint32_t regions[] = {COGMASTER_REGION_EXPORT, COGMASTER_REGION_JAPAN};
    for (size_t k = 0; k < 2u; k++) {
        cogmaster *m = load(COGMASTER_MACHINE_SMS2, regions[k], COGMASTER_TV_NTSC);
        CHECK_EQ_HEX(m->io.io_ctrl, 0xFF);
        cogmaster__bus_out(m, 0x003Fu, 0xF5u);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x00DDu) & 0xC0u, k == 0u ? 0xC0 : 0x00);
        cogmaster__bus_out(m, 0x003Fu, 0x55u);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x00DDu) & 0xC0u, 0x00);
        cogmaster__bus_out(m, 0x003Fu, 0xFFu);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x00DDu) & 0xC0u, 0xC0);
        CHECK_EQ_U64(fault_of(m).present, 0);
        cogmaster_destroy(m);
    }
}

/* With nothing pressed $DC reads $FF; $DD reads $FF: port B released, b4
   the absent reset button (1), b5 1 on the SMS and the Game Gear, TH
   inputs pulled up. */
static void test_port_dc_dd(void)
{
    cogmaster *ms[] = {
        export_sms(),
        japan_sms(),
        load(COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, COGMASTER_TV_PAL),
        gg(COGMASTER_REGION_EXPORT),
    };
    for (size_t k = 0; k < sizeof ms / sizeof ms[0]; k++) {
        CHECK_EQ_HEX(cogmaster__bus_in(ms[k], 0x00DCu), 0xFF);
        uint8_t dd = cogmaster__bus_in(ms[k], 0x00DDu);
        CHECK_EQ_HEX(dd, 0xFF);
        CHECK_EQ_HEX(dd & 0x30u, 0x30);
        CHECK_EQ_U64(fault_of(ms[k]).present, 0);
        cogmaster_destroy(ms[k]);
    }
}

/* Game Gear $00: b7 Start released, b6 0 on a Japanese unit, b5 0 for
   NTSC, the only standard it was made for ($C0 at power-on, SMS-SYSTEM
   section 8). $01-$06 power on as $7F $FF $00 $FF $00 $FF; the link and
   stereo registers keep what is written, RX does not. $3E does not exist
   in Game Gear mode, so a write there changes no enable. */
static void test_gg_ports(void)
{
    cogmaster *m = gg(COGMASTER_REGION_EXPORT);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x0000u), 0xC0);
    static const uint8_t reset[6] = {0x7Fu, 0xFFu, 0x00u, 0xFFu, 0x00u, 0xFFu};
    for (uint32_t p = 1; p <= 6u; p++) {
        CHECK_EQ_HEX(cogmaster__bus_in(m, (uint16_t)p), reset[p - 1u]);
    }
    CHECK_EQ_U64(fault_of(m).present, 0);
    cogmaster__bus_out(m, 0x0002u, 0x12u);
    cogmaster__bus_out(m, 0x0004u, 0x34u);
    cogmaster__bus_out(m, 0x0006u, 0x56u);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x0002u), 0x12);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x0004u), 0x00);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x0006u), 0x56);
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xA8);
    cogmaster__bus_out(m, 0x003Eu, 0xFFu);
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xA8);
    CHECK_EQ_HEX(cogmaster__bus_read(m, STAMP_AT), STAMP);
    cogmaster_destroy(m);

    m = gg(COGMASTER_REGION_JAPAN);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x0000u), 0x80);
    cogmaster_destroy(m);
}

/* An undriven port reads $FF on the SMS2 and the Game Gear and counts one
   open-bus read each time. */
static void test_open_bus(void)
{
    static const struct {
        uint32_t machine, region;
        uint16_t port;
    } reads[] = {
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, 0x0000u},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_EXPORT, 0x003Fu},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, 0x00C0u},
        {COGMASTER_MACHINE_SMS2, COGMASTER_REGION_JAPAN, 0x0010u},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, 0x0007u},
        {COGMASTER_MACHINE_GG, COGMASTER_REGION_EXPORT, 0x00DEu},
    };
    for (size_t k = 0; k < sizeof reads / sizeof reads[0]; k++) {
        cogmaster *m = load(reads[k].machine, reads[k].region, COGMASTER_TV_NTSC);
        for (uint32_t i = 1; i <= 3u; i++) {
            CHECK_EQ_HEX(cogmaster__bus_in(m, reads[k].port), 0xFF);
            CHECK_EQ_U64(m->counters.open_bus_reads, i);
        }
        cogmaster_destroy(m);
    }
}

/* The export console decodes A7, A6 and A0: $C0-$FF answer as $DC (even)
   and $DD (odd), and $00-$3F latch as $3E (even) and $3F (odd). With $3E
   b2 set the I/O chip is off and $C0-$FF read open bus, which is no fault.
   The Japanese console and the Game Gear answer only at $DC, $DD, $3E and
   $3F (SMS-SYSTEM section 4). $3F = $55 makes $DD read $3F. */
static void test_decode_mirrors(void)
{
    cogmaster *m = export_sms();
    cogmaster__bus_out(m, 0x0001u, 0x55u);
    CHECK_EQ_HEX(m->io.io_ctrl, 0x55);
    static const uint16_t odd[] = {0x00C1u, 0x00DDu, 0x00DFu, 0x00F1u, 0x00FFu, 0x12DDu};
    static const uint16_t even[] = {0x00C0u, 0x00DCu, 0x00DEu, 0x00F0u, 0x00F2u, 0x00FEu};
    for (size_t i = 0; i < 6u; i++) {
        CHECK_EQ_HEX(cogmaster__bus_in(m, odd[i]), 0x3F);
        CHECK_EQ_HEX(cogmaster__bus_in(m, even[i]), 0xFF);
    }
    CHECK_EQ_U64(m->counters.open_bus_reads, 0);
    cogmaster__bus_out(m, 0x0002u, 0xAFu);
    CHECK_EQ_HEX(m->io.mem_ctrl, 0xAF);
    for (size_t i = 0; i < 6u; i++) {
        CHECK_EQ_HEX(cogmaster__bus_in(m, odd[i]), 0xFF);
    }
    CHECK_EQ_U64(m->counters.open_bus_reads, 6);
    cogmaster__bus_out(m, 0x0002u, 0xABu);
    CHECK_EQ_HEX(cogmaster__bus_in(m, 0x00C1u), 0x3F);
    CHECK_EQ_U64(fault_of(m).present, 0);
    cogmaster_destroy(m);

    cogmaster *full[] = {japan_sms(), gg(COGMASTER_REGION_EXPORT)};
    for (size_t k = 0; k < 2u; k++) {
        m = full[k];
        cogmaster__bus_out(m, 0x003Fu, 0x55u);
        CHECK_EQ_HEX(m->io.io_ctrl, 0x55);
        uint8_t dd = k == 0u ? 0x3Fu : 0xFFu; /* the Game Gear has no TH lines */
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x00DDu), dd);
        CHECK_EQ_HEX(cogmaster__bus_in(m, 0x00DCu), 0xFF);
        static const uint16_t others[] = {0x00C0u, 0x00C1u, 0x00DEu, 0x00DFu, 0x00F1u, 0x00FFu};
        for (size_t i = 0; i < 6u; i++) {
            CHECK_EQ_HEX(cogmaster__bus_in(m, others[i]), 0xFF);
        }
        CHECK_EQ_U64(m->counters.open_bus_reads, 6);
        cogmaster__bus_out(m, 0x0001u, 0xFFu);
        CHECK_EQ_HEX(m->io.io_ctrl, 0x55);
        cogmaster_destroy(m);
    }
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"decode_mirrors", test_decode_mirrors}, {"gg_ports", test_gg_ports},
    {"open_bus", test_open_bus},             {"port3e", test_port3e},
    {"port3f_region", test_port3f_region},   {"port_dc_dd", test_port_dc_dd},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: cogmaster-test-io <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "cogmaster-test-io: unknown case %s\n", argv[1]);
    return 2;
}
