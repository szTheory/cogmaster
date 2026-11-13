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

/* The axis words of a Sports Pad or Paddle port are motion since the last call, in the host's
   units; the sum of a port's calls between two frames is kept, within the same range. On a Light
   Phaser port they are an absolute aim instead and the last call wins. */
#define AXIS_MAX 32767

static int axis_ok(int32_t v)
{
    return v >= -AXIS_MAX && v <= AXIS_MAX;
}

/* pending + d, both inside the range, so the sum fits an int32_t and needs no saturating call. */
static int32_t axis_add(int32_t pending, int32_t d)
{
    int32_t sum = pending + d;
    return sum < -AXIS_MAX ? -AXIS_MAX : (sum > AXIS_MAX ? AXIS_MAX : sum);
}

/* A Light Phaser aim is a field dot, 1 to 256 across and 1 to 240 down, and 0 0 means none. */
#define AIM_MAX_X 256
#define AIM_MAX_Y 240

/* Adds one port's motion to the pending record. A different device than the pending one restarts
   the port's axes; an axis the device does not take is dropped. */
static void add_motion(cogmaster_input *pend, const cogmaster_input *in, uint32_t port)
{
    uint32_t device = port == 0u ? in->port1 : in->port2;
    uint32_t before = port == 0u ? pend->port1 : pend->port2;
    int32_t *px = port == 0u ? &pend->x1 : &pend->x2;
    int32_t *py = port == 0u ? &pend->y1 : &pend->y2;
    int32_t x = port == 0u ? in->x1 : in->x2;
    int32_t y = port == 0u ? in->y1 : in->y2;
    if (device != before) {
        *px = *py = 0;
    }
    if (device == COGMASTER_DEVICE_SPORTS_PAD) {
        *px = axis_add(*px, x);
        *py = axis_add(*py, y);
    } else if (device == COGMASTER_DEVICE_PADDLE) {
        *px = axis_add(*px, x);
    } else if (device == COGMASTER_DEVICE_LIGHT_PHASER) {
        /* The call replaces the aim, and an aim that is not a field dot is the parked state, never
           moved to an edge dot. A stored aim has |dx| and |dy| of at most 256 against any dot, so
           the sensor's squared sum stays below 2 to the 17th and nothing overflows. A later call
           in the same frame without axes parks the aim. */
        int on = x >= 1 && x <= AIM_MAX_X && y >= 1 && y <= AIM_MAX_Y;
        *px = on ? x : 0;
        *py = on ? y : 0;
    }
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
        local.start > 1u || local.port1 > COGMASTER__DEVICE_LAST ||
        local.port2 > COGMASTER__DEVICE_LAST || !axis_ok(local.x1) || !axis_ok(local.y1) ||
        !axis_ok(local.x2) || !axis_ok(local.y2)) {
        return COGMASTER_STATUS_ERR_ARGUMENT;
    }
    /* A device this profile cannot take is refused whole: nothing below has run. */
    if (!cogmaster__device_ok(m, local.port1) || !cogmaster__device_ok(m, local.port2)) {
        return COGMASTER_STATUS_ERR_PROFILE_REJECTED;
    }
    /* Devices and buttons are the latest call's; motion adds up until a frame takes it. */
    cogmaster_input next = m->pending_input;
    add_motion(&next, &local, 0u);
    add_motion(&next, &local, 1u);
    local.size = 0u;
    local.x1 = next.x1;
    local.y1 = next.y1;
    local.x2 = next.x2;
    local.y2 = next.y2;
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
    /* The device blocks start clean, the stored edge included: the tick count restarts at 0, so
       an edge from before would be in its future. The pending motion is not touched. */
    memset(m->ports, 0, sizeof m->ports);
    cogmaster__bus_decode(m);
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
   GG has no TH pins (SMS-SYSTEM section 4, PeripheralPorts). A device that drives TH low
   on an input pin passes its bit in low_mask (b6 port 1, b7 port 2): the Light Phaser's sensor
   does, through sensor_low, and an output TH ignores it. */
/* The TH bits a Light Phaser holds low: b0 of a port's g_sense is the sensor seeing light now, b1 a
   light pulse seen since the last $DD read or the end of the picture. The edge of the H latch
   follows the level alone (bits 1), the $DD readback follows level or pulse (bits 3). A port
   without a gun has a zero block, so the mask is 0 and TH is the pad's. */
static uint8_t sensor_low(const cogmaster *m, uint8_t bits)
{
    uint8_t low = 0u;
    if (m->input.port1 == COGMASTER_DEVICE_LIGHT_PHASER && (m->ports[0].g_sense & bits) != 0u) {
        low |= 0x40u;
    }
    if (m->input.port2 == COGMASTER_DEVICE_LIGHT_PHASER && (m->ports[1].g_sense & bits) != 0u) {
        low |= 0x80u;
    }
    return low;
}

static uint8_t th_pins(const cogmaster *m, uint8_t ctrl, uint8_t low_mask)
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
        if ((ctrl & 2u) != 0u) {
            pins = (uint8_t)(pins & ~(low_mask & 0x40u));
        }
        if ((ctrl & 8u) != 0u) {
            pins = (uint8_t)(pins & ~(low_mask & 0x80u));
        }
    }
    return pins;
}

/* The level on a port's TH wire: pulled high when the pin is an input, else the level the
   console drives. Unlike th_pins it has no region echo, so a device sees the same wire on every
   region (port 0: direction $3F b1, level b5; port 1: direction b3, level b7). */
static uint32_t th_wire(uint8_t ctrl, uint32_t port)
{
    uint8_t dir = port == 0u ? 2u : 8u;
    uint8_t level = port == 0u ? 0x20u : 0x80u;
    return (ctrl & dir) != 0u || (ctrl & level) != 0u ? 1u : 0u;
}

/* The single latch site: a rising effective TH on either port latches the H counter at the
   in-line tick given. Every edge of the console's own $3F write comes through here with the
   current tick, and a sensor edge will with the tick it happened at; ports_th is never called
   for a sensor edge (SMS-SYSTEM section 4). */
static void th_edge(cogmaster *m, uint8_t old_pins, uint8_t new_pins, uint32_t tick)
{
    if (((uint8_t)~old_pins & new_pins & 0xC0u) != 0u) {
        cogmaster__vdp_latch_h_at(m, tick);
    }
}

/* Every $3F write comes here with the level before and after it, and the
   tick it lands on. Low-to-high on either effective TH captures, including
   release to the input pull-up (SMS-SYSTEM section 4). A device that watches
   TH is told of each change of its port's wire, so its edges alternate. */
void cogmaster__io_th_changed(cogmaster *m, uint8_t old_ctrl, uint8_t new_ctrl, uint64_t tick)
{
    uint8_t lit = sensor_low(m, 1u);
    th_edge(m, th_pins(m, old_ctrl, lit), th_pins(m, new_ctrl, lit), m->bus.t);
    for (uint32_t port = 0u; port < 2u; port++) {
        uint32_t wire = th_wire(new_ctrl, port);
        if (th_wire(old_ctrl, port) != wire) {
            cogmaster__ports_th(m, port, wire, tick);
        }
    }
}

/* The sensor of a port changed state at an in-line tick (SMS-CARTRIDGES-AND-PERIPHERALS section 3,
   Light Phaser). TH falls when it sees light and rises when it stops, and the H counter is
   latched by the rise like any other: the pins before and after go through the same function
   and the same edge rule as a $3F write. A fall on an input TH also sets the pulse bit. The
   console's own wire did not change, so no device is told (ports_th is not called). */
void cogmaster__io_sensor_edge(cogmaster *m, uint32_t port, uint32_t lit, uint32_t tick)
{
    struct cogmaster__port *p = &m->ports[port];
    uint8_t before = th_pins(m, m->io.io_ctrl, sensor_low(m, 1u));
    p->g_sense = (uint8_t)(lit != 0u ? (p->g_sense | 1u) : (p->g_sense & ~1u));
    uint8_t after = th_pins(m, m->io.io_ctrl, sensor_low(m, 1u));
    if (((uint8_t)(before & ~after) & (0x40u << port)) != 0u) {
        p->g_sense |= 2u;
    }
    th_edge(m, before, after, tick);
    m->gun_stats.edges++;
}

/* A write to $3F first brings the sensor up to its tick, so the edges before it saw the old
   direction bits, and an edge and the write at one tick go sensor first. */
static void io_control(cogmaster *m, uint8_t ctrl)
{
    uint8_t old = m->io.io_ctrl;
    if (cogmaster__gun_attached(m)) {
        cogmaster__gun_sync(m);
    }
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
    return (uint8_t)((v & 0x3Fu) | th_pins(m, m->io.io_ctrl, sensor_low(m, 3u)));
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
    if ((p & 1u) == 0u) {
        return port_dc(m);
    }
    if (!cogmaster__gun_attached(m)) {
        return port_dd(m);
    }
    /* A $DD read brings the sensor up to the beam and then takes the pulse bit of each port whose
       TH is an input: the console's own latch of the TH line, like hheld, so it lives here and
       port_dd stays a pure function of the instance. */
    cogmaster__gun_sync(m);
    uint8_t v = port_dd(m);
    if ((m->io.io_ctrl & 2u) != 0u) {
        m->ports[0].g_sense &= 1u;
    }
    if ((m->io.io_ctrl & 8u) != 0u) {
        m->ports[1].g_sense &= 1u;
    }
    return v;
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
    /* A read of $00-$3F nothing drives (the memory and I/O control ports are write-only) returns
       the open bus and is no fault: shipped games read such ports (SMS-SYSTEM C5 and section 4). */
    return cogmaster__io_open_bus(m);
}

/* The bus sends every port but the VDP's and the sound chip's here. Writes
   to the I/O chip ($C0-$FF) and to ports nothing decodes latch nothing and are
   no fault: no document describes an effect, let alone a fault, for such a
   write (SMS-SYSTEM section 4). */
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
            cogmaster__bus_decode(m);
        }
    }
}
