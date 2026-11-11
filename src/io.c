/* The I/O chip's ports: every port the VDP does not answer (SMS-SYSTEM
   section 4). The export Master System decodes A7, A6 and A0 only, so
   $00-$3F latch $3E (even) and $3F (odd) and $C0-$FF answer as $DC (even)
   and $DD (odd). The Japanese console and the Game Gear decode the full
   address (section 4, "Full decode exceptions"). The Game Gear's own ports,
   $00-$06, exist only in its native mode; in Master System mode the console
   keeps its full decode and its pad, and decodes $3E and $3F as a full-decode
   Master System does. With the YM2413 unit attached, $F0 to $F2 are the unit's
   ports and answer ahead of the I/O chip. */
#include <string.h>

#include "internal.h"

/* The input struct's first released size: every API version up to 11 had these
   eight words and no more, and a caller built against one of them owns exactly
   that many bytes. */
#define FIRST_SIZE_INPUT 32u
_Static_assert(FIRST_SIZE_INPUT == offsetof(cogmaster_input, x1), "first input size");

/* The axis words are validated and, until the devices that read them exist,
   dropped. */
#define AXIS_MAX 32767

static int axis_ok(int32_t v)
{
    return v >= -AXIS_MAX && v <= AXIS_MAX;
}

cogmaster_status cogmaster_set_input(cogmaster *m, const cogmaster_input *in)
{
    if (m == NULL || in == NULL) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    cogmaster_status st = cogmaster__check_size(in, FIRST_SIZE_INPUT, (uint32_t)sizeof *in);
    if (st != COGMASTER_STATUS_OK) {
        return st;
    }
    if (!m->loaded) {
        return COGMASTER_STATUS_ERR_NO_CONTENT;
    }
    /* Read only the whole fields the caller's tag covers: a caller built
       against a smaller struct owns no more bytes than that, and the fields
       it does not have stay 0. The assembled copy is checked, then stored. */
    uint32_t tag = in->size;
    uint32_t n = (tag < (uint32_t)sizeof *in ? tag : (uint32_t)sizeof *in) & ~3u;
    cogmaster_input local;
    memset(&local, 0, sizeof local);
    const uint8_t *src = (const uint8_t *)in;
    uint8_t *dst = (uint8_t *)&local;
    for (uint32_t i = 0u; i < n; i++) {
        dst[i] = src[i];
    }
    if (local.p1 > 63u || local.p2 > 63u || local.pause > 1u || local.reset > 1u ||
        local.start > 1u || local.port1 > COGMASTER_DEVICE_NONE ||
        local.port2 > COGMASTER_DEVICE_NONE || !axis_ok(local.x1) || !axis_ok(local.y1) ||
        !axis_ok(local.x2) || !axis_ok(local.y2)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    local.size = 0u;
    local.x1 = local.y1 = local.x2 = local.y2 = 0;
    m->pending_input = local;
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

/* Whether $00-$06 exist and $3E does not: the Game Gear in native mode. In
   Master System mode the console keeps its full decode and its pad, but those
   ports are not decoded and $3E and $3F answer as on a full-decode Master
   System. */
static int has_gg_ports(const cogmaster *m)
{
    return cogmaster__native_gg(m);
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
    io->mem_ctrl = has_gg_ports(m) ? IO_MEM_GG : IO_MEM_SMS;
    io->io_ctrl = IO_CTRL_RESET;
    for (uint32_t i = 0; i < 6u; i++) {
        io->gg[i] = gg_reset[i];
    }
    io->glasses = COGMASTER_GLASSES_NONE;
    io->glasses_frame = COGMASTER_GLASSES_NONE;
}

/* A write at $FFF8-$FFFF that reached work RAM's mirror. At $FFFC and up a cartridge on the bus
   sees it as a mapper register; below, only the 3-D glasses adapter does (SMS Power, 3-D Glasses),
   which snoops the bus whatever $3E says, so the eye is noted on every Master System profile and
   never on a Game Gear. The register is write-only, so noting it changes nothing the guest can read
   and no hashed byte. The window-relative tick is unsigned and never below zero: the window starts
   at the previous deadline and the CPU is past it. A write with work RAM off reaches nothing and
   faults as the bus does for any other address. */
void cogmaster__io_glasses_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    int ram_on = (m->io.mem_ctrl & 0x10u) == 0u;
    if (addr >= 0xFFFCu) {
        if ((m->io.mem_ctrl & 0x40u) == 0u) {
            cogmaster__mapper_write(m, addr, v);
        } else if (!ram_on) {
            cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_RAM, COGMASTER__FAULT_UNMAPPED_WRITE);
        }
        return;
    }
    if (m->machine != COGMASTER_MACHINE_GG) {
        uint8_t eye = (v & 1u) != 0u ? COGMASTER_GLASSES_LEFT : COGMASTER_GLASSES_RIGHT;
        uint64_t window = m->ticks - (m->bus.deadline - cogmaster__frame_ticks(m));
        m->io.glasses = eye;
        if (window < COGMASTER__GLASSES_CUT_TICKS(cogmaster__frame_lines(m))) {
            m->io.glasses_frame = eye;
        }
    }
    if (!ram_on) {
        cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_RAM, COGMASTER__FAULT_UNMAPPED_WRITE);
    }
}

/* An output TR replaces the pad level, with the Japanese region echo low
   (SMS-CARTRIDGES-AND-PERIPHERALS section 3.1). GG has no external TR. */
static uint8_t tr_pin(const cogmaster *m, uint8_t v, uint8_t pin, uint8_t dir, uint8_t level)
{
    if (!is_gg(m) && (m->io.io_ctrl & dir) == 0u) {
        v = (uint8_t)(v & ~pin);
        if (m->region != COGMASTER_REGION_JAPAN && (m->io.io_ctrl & level) != 0u)
            v |= pin;
    }
    return v;
}

/* $DC: port A up, down, left, right, TL, TR, then port B up and down, 0 =
   pressed. Independent direction bits pass through, including SOCD. */
static uint8_t port_dc(const cogmaster *m)
{
    uint8_t a = cogmaster__ports_pins(m, 0u, m->ticks);
    uint8_t b = cogmaster__ports_pins(m, 1u, m->ticks);
    return tr_pin(m, (uint8_t)(a | ((b & 3u) << 6)), 0x20u, 1u, 0x10u);
}

/* Effective TH pins, shared by readback and the rising-edge H latch.
   Unconnected pad inputs pull high; Japanese output TH reads low; native
   GG has no TH pins (SMS-SYSTEM section 4, PeripheralPorts). */
static uint8_t th_pins(const cogmaster *m, uint8_t ctrl)
{
    uint8_t pins = 0xC0u;
    uint8_t echo = m->region == COGMASTER_REGION_JAPAN ? 0u : ctrl;
    if (!is_gg(m)) {
        if ((ctrl & 2u) == 0u) {
            pins = (uint8_t)((pins & ~0x40u) | ((echo & 0x20u) << 1));
        }
        if ((ctrl & 8u) == 0u) {
            pins = (uint8_t)((pins & ~0x80u) | (echo & 0x80u));
        }
    }
    return pins;
}

/* Every $3F write comes here with the level before and after it, and the
   tick it lands on (the tick is for the devices that watch TH). Low-to-high
   on either effective TH captures, including release to the input pull-up
   (SMS-SYSTEM section 4). */
void cogmaster__io_th_changed(cogmaster *m, uint8_t old_ctrl, uint8_t new_ctrl, uint64_t tick)
{
    (void)tick;
    uint8_t old = th_pins(m, old_ctrl);
    if (((uint8_t)~old & th_pins(m, new_ctrl) & 0xC0u) != 0u) {
        cogmaster__vdp_latch_h(m);
    }
}

static void io_control(cogmaster *m, uint8_t ctrl)
{
    uint8_t old = m->io.io_ctrl;
    m->io.io_ctrl = ctrl;
    cogmaster__io_th_changed(m, old, ctrl, m->ticks);
}

/* $DD: b3-0 port B left, right, TL, TR; b4 the reset button, 1 where there
   is none; b5 1 on the SMS and the Game Gear; b7-6 port B and port A TH
   (section 4). A TH set as an output ($3F b3 or b1 clear) reads back its
   level ($3F b7 or b5) on the export console only; the Japanese console
   reads 0 there (section 4, "$3F I/O control"), which is how software tells
   the regions apart (section 5). The Game Gear has no TH lines. */
static uint8_t port_dd(const cogmaster *m)
{
    /* Only export SMS1 has the physical switch (SMS-SYSTEM section 7). */
    uint32_t reset = m->machine == COGMASTER_MACHINE_SMS1 && m->region == COGMASTER_REGION_EXPORT &&
                             m->input.port1 != COGMASTER_DEVICE_NONE
                         ? m->input.reset
                         : 0u;
    uint8_t b = cogmaster__ports_pins(m, 1u, m->ticks);
    uint8_t v = tr_pin(m, (uint8_t)(~(reset << 4) & ((b >> 2) | 0xF0u)), 8u, 4u, 0x40u);
    return (uint8_t)((v & 0x3Fu) | th_pins(m, m->io.io_ctrl));
}

/* The YM2413 unit's ports: $F0 latches a register, $F1 writes it, $F2 is the audio control
   (SMS-AUDIO section 4.1). The unit answers whether or not $3E b2 has turned the I/O chip
   off; software turns the chip off to reach it (section 4). */
#define FM_PORT_ADDRESS 0xF0u
#define FM_PORT_DATA 0xF1u
#define FM_PORT_CTRL 0xF2u

static int fm_port(const cogmaster *m, uint32_t p)
{
    return m->fm_unit != 0u && p >= FM_PORT_ADDRESS && p <= FM_PORT_CTRL;
}

/* $F2 reads back the control bits written, with bits 7, 6 and 5 the bits 11, 7 and 3 of a
   12-bit counter that counts lines (the 315-5297's, SMS-AUDIO section 4.1); bits 4 to 2 are
   0. The counter is the line index of the tick count since the last load or reset, so it
   needs no state. $F0 and $F1 drive nothing. */
static uint8_t fm_in(cogmaster *m, uint32_t p)
{
    if (p != FM_PORT_CTRL) {
        return cogmaster__io_open_bus(m);
    }
    uint32_t line = (uint32_t)(m->ticks / 684u);
    return (uint8_t)(m->io.audio_ctrl | (((line >> 3) & 1u) << 5) | (((line >> 7) & 1u) << 6) |
                     (((line >> 11) & 1u) << 7));
}

/* A write to $F1 first completes every sample due at or before its tick, then the register
   changes, so it takes effect from the next sample. A write to $F2 brings both chips up to
   date, so what changed before it is gated as it was, and stores the control bits the
   profile keeps. */
static void fm_out(cogmaster *m, uint32_t p, uint8_t v)
{
    if (p == FM_PORT_ADDRESS) {
        m->opll.latch = v;
    } else if (p == FM_PORT_DATA) {
        cogmaster__fm_catch_up(m, m->ticks);
        cogmaster__opll_write(&m->opll, m->opll.latch, v);
    } else {
        cogmaster__psg_catch_up(m, m->ticks);
        cogmaster__fm_catch_up(m, m->ticks);
        m->io.audio_ctrl = (uint8_t)(v & m->fm_mask);
        cogmaster__synth_level(m, m->ticks, 6u, m->io.audio_ctrl);
    }
}

/* $C0-$FF: the I/O chip, unless $3E b2 has turned it off; then the bus is
   undriven. Software turns it off to probe the FM chip, so that read is no
   fault (section 4). The ports $F0 to $F2 mirror $DC and $DD on an export
   console without the unit, and no read of them counts as input. */
static uint8_t chip_in(cogmaster *m, uint32_t p)
{
    if ((m->io.mem_ctrl & IO_CHIP_OFF) != 0u) {
        return cogmaster__io_open_bus(m);
    }
    if (full_decode(m) && p != 0xDCu && p != 0xDDu && !(is_gg(m) && (p == 0xC0u || p == 0xC1u))) {
        return cogmaster__io_open_bus(m);
    }
    if (p < FM_PORT_ADDRESS || p > FM_PORT_CTRL) {
        m->input_read = 1u;
    }
    return (p & 1u) != 0u ? port_dd(m) : port_dc(m);
}

uint8_t cogmaster__io_in(cogmaster *m, uint16_t port)
{
    uint32_t p = port & 0xFFu;
    if (fm_port(m, p)) {
        return fm_in(m, p);
    }
    if (p >= 0xC0u) {
        return chip_in(m, p);
    }
    if (has_gg_ports(m) && p <= 6u) {
        if (p == 0u) {
            m->input_read = 1u;
            return (uint8_t)((m->input.port1 != COGMASTER_DEVICE_NONE && m->input.start
                                  ? 0u
                                  : GG_START_UP) |
                             (m->region == COGMASTER_REGION_JAPAN ? 0u : GG_EXPORT));
        }
        return p == 0u ? cogmaster__io_open_bus(m) : m->io.gg[p - 1u];
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
    if (fm_port(m, p)) {
        fm_out(m, p, v);
        return;
    }
    if (has_gg_ports(m)) {
        /* Game Gear mode has no $3E; $04 is the link's receive register,
           which a write leaves alone. $06 is the stereo mask: the chip is
           caught up to the write and the mask is stamped at its tick, one
           copy of it living here (SMS-AUDIO section 3). */
        if (p >= 1u && p <= 6u) {
            if (p == 6u) {
                cogmaster__psg_catch_up(m, m->ticks);
                io->gg[5] = v;
                cogmaster__synth_level(m, m->ticks, 4u, v);
            } else if (p != 4u) {
                io->gg[p - 1u] = p == 5u ? (uint8_t)(v & ~GG_STATUS_FLAGS) : v;
            }
            return;
        }
        if (p == 0x3Fu) {
            io_control(m, v);
            return;
        }
    } else if (p < 0x40u && (!full_decode(m) || p >= 0x3Eu)) {
        if ((p & 1u) != 0u) {
            io_control(m, v);
        } else {
            io->mem_ctrl = v;
        }
        return;
    }
    if (p < 0xC0u) {
        cogmaster__bus_fault(m, port, COGMASTER__DEVICE_NONE, COGMASTER__FAULT_UNMAPPED_WRITE);
    }
}
