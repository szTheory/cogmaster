/* The I/O chip's ports: every port the VDP does not answer (SMS-SYSTEM
   section 4). The export Master System decodes A7, A6 and A0 only, so
   $00-$3F latch $3E (even) and $3F (odd) and $C0-$FF answer as $DC (even)
   and $DD (odd). The Japanese console and the Game Gear decode the full
   address (section 4, "Full decode exceptions"). */
#include "internal.h"

cogmaster_status cogmaster_set_input(cogmaster *m, const cogmaster_input *in)
{
    if (m == NULL || in == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_size(in, 32u, (uint32_t)sizeof *in);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    if (in->p1 > 63u || in->p2 > 63u || in->pause > 1u || in->reset > 1u || in->start > 1u ||
        in->port1 > COGMASTER_DEVICE_NONE || in->port2 > COGMASTER_DEVICE_NONE) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    m->pending_input = *in;
    m->pending_input.size = 0u;
    return COGMASTER_STATUS_OK;
}

/* $3E is active low (section 4). */
#define IO_CART_OFF 0x40u
#define IO_RAM_OFF 0x10u
#define IO_CHIP_OFF 0x04u

/* The hand-off values: the BIOS leaves $3E at $AB on the Master System and
   $A8 on the Game Gear, and $3F at $FF, every pin an input (SMS-SYSTEM C7).
   The Game Gear's $01-$06 power on as $7F $FF $00 $FF $00 $FF (section 8). */
#define IO_MEM_SMS 0xABu
#define IO_MEM_GG 0xA8u
#define IO_CTRL_RESET 0xFFu
static const uint8_t gg_reset[6] = {0x7Fu, 0xFFu, 0x00u, 0xFFu, 0x00u, 0xFFu};

/* Game Gear $00 (section 4): b7 Start, 0 = pressed; b6 region, 0 = Japan;
   b5 0 for NTSC, the only standard the Game Gear was made for, so the port
   powers on as $C0 (section 8). */
#define GG_START_UP 0x80u
#define GG_EXPORT 0x40u
/* $05: b2-0 are the link's status flags, which nothing here sets. */
#define GG_STATUS_FLAGS 0x07u

static int is_gg(const cogmaster *m)
{
    return m->machine == COGMASTER_MACHINE_GG;
}

static int full_decode(const cogmaster *m)
{
    return is_gg(m) || m->region == COGMASTER_REGION_JAPAN;
}

/* An undriven read: SMS1 retains the last driven byte, whereas the SMS2
   and the Game Gear pull the data bus up to $FF
   (SMS-SYSTEM C5). Shipped software makes such reads, so they are counted,
   not faults. */
uint8_t cogmaster__io_open_bus(cogmaster *m)
{
    m->counters.open_bus_reads++;
    return (uint8_t)(m->bus.data | cogmaster__profile(m).bus_pullup);
}

void cogmaster__io_reset(cogmaster *m)
{
    struct cogmaster__io *io = &m->io;
    io->mem_ctrl = is_gg(m) ? IO_MEM_GG : IO_MEM_SMS;
    io->io_ctrl = IO_CTRL_RESET;
    for (uint32_t i = 0; i < 6u; i++) {
        io->gg[i] = gg_reset[i];
    }
}

/* $DC: port A up, down, left, right, TL, TR, then port B up and down, 0 =
   pressed. No input reaches the core yet, so every button is up. */
static uint8_t port_dc(const cogmaster *m)
{
    uint32_t p1 = m->input.port1 == COGMASTER_DEVICE_NONE ? 0u : m->input.p1;
    uint32_t p2 = m->input.port2 == COGMASTER_DEVICE_NONE || is_gg(m) ? 0u : m->input.p2;
    return (uint8_t)~(p1 | ((p2 & 3u) << 6));
}

/* $DD: b3-0 port B left, right, TL, TR; b4 the reset button, 1 where there
   is none; b5 1 on the SMS and the Game Gear; b7-6 port B and port A TH
   (section 4). A TH set as an output ($3F b3 or b1 clear) reads back its
   level ($3F b7 or b5) on the export console only; the Japanese console
   reads 0 there (section 4, "$3F I/O control"), which is how software tells
   the regions apart (section 5). The Game Gear has no TH lines. */
static uint8_t port_dd(const cogmaster *m)
{
    uint32_t p2 = m->input.port2 == COGMASTER_DEVICE_NONE || is_gg(m) ? 0u : m->input.p2;
    uint8_t v = (uint8_t)~((p2 >> 2) | (is_gg(m) ? 0u : m->input.reset << 4));
    uint8_t c = m->io.io_ctrl;
    if (is_gg(m)) {
        return v;
    }
    uint8_t echo = m->region == COGMASTER_REGION_JAPAN ? 0x00u : c;
    if ((c & 0x02u) == 0u) {
        v = (uint8_t)((v & ~0x40u) | ((echo & 0x20u) << 1));
    }
    if ((c & 0x08u) == 0u) {
        v = (uint8_t)((v & ~0x80u) | (echo & 0x80u));
    }
    return v;
}

/* $C0-$FF: the I/O chip, unless $3E b2 has turned it off; then the bus is
   undriven. Software turns it off to probe the FM chip, so that read is no
   fault (section 4). */
static uint8_t chip_in(cogmaster *m, uint32_t p)
{
    if ((m->io.mem_ctrl & IO_CHIP_OFF) != 0u) {
        return cogmaster__io_open_bus(m);
    }
    if (full_decode(m) && p != 0xDCu && p != 0xDDu) {
        return cogmaster__io_open_bus(m);
    }
    m->input_read = 1u;
    return (p & 1u) != 0u ? port_dd(m) : port_dc(m);
}

uint8_t cogmaster__io_in(cogmaster *m, uint16_t port)
{
    uint32_t p = port & 0xFFu;
    if (p >= 0xC0u) {
        return chip_in(m, p);
    }
    if (is_gg(m) && p <= 6u) {
        if (p == 0u) {
            m->input_read = 1u;
            return (uint8_t)((m->input.start ? 0u : GG_START_UP) |
                             (m->region == COGMASTER_REGION_JAPAN ? 0u : GG_EXPORT));
        }
        return m->io.gg[p - 1u];
    }
    if (p < 0x40u) {
        /* $3E and $3F are write-only; nothing drives a read there. */
        cogmaster__bus_fault(m, port, COGMASTER__DEVICE_CTRL, COGMASTER__FAULT_UNMAPPED_READ);
    }
    return cogmaster__io_open_bus(m);
}

/* The bus sends every port but the VDP's and the sound chip's here. Writes
   to the I/O chip ($C0-$FF) latch nothing and are no fault; on a
   full-decode console a write nothing else latches is unmapped_write. */
void cogmaster__io_out(cogmaster *m, uint16_t port, uint8_t v)
{
    struct cogmaster__io *io = &m->io;
    uint32_t p = port & 0xFFu;
    if (is_gg(m)) {
        /* Game Gear mode has no $3E; $04 is the link's receive register,
           which a write leaves alone. */
        if (p >= 1u && p <= 6u) {
            if (p != 4u) {
                io->gg[p - 1u] = p == 5u ? (uint8_t)(v & ~GG_STATUS_FLAGS) : v;
            }
            return;
        }
        if (p == 0x3Fu) {
            io->io_ctrl = v;
            return;
        }
    } else if (p < 0x40u && (!full_decode(m) || p >= 0x3Eu)) {
        if ((p & 1u) != 0u) {
            io->io_ctrl = v;
        } else {
            io->mem_ctrl = v;
        }
        return;
    }
    if (p < 0xC0u) {
        cogmaster__bus_fault(m, port, COGMASTER__DEVICE_NONE, COGMASTER__FAULT_UNMAPPED_WRITE);
    }
}
