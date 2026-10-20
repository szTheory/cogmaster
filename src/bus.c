/* The Z80's bus in the library: the beam counters, the frame deadline, the
   memory decode and the seam functions z80.h declares. The vector harness
   defines its own seam and never links this file.

   Time is one tick per VDP clock: a line is 684 ticks, an NTSC frame 262
   lines and a PAL frame 313 (SMS-SYSTEM sections 1 and C1). The beam is kept as a line and a tick
   in the line, advanced by the ticks since the last access, so nothing here
   divides a 64-bit count. */
#include <string.h>

#include "internal.h"

#define BUS_LINE_TICKS 684u

/* The frame origin: the beam position at tick 0 after load and reset. A
   frame runs from frame-interrupt tick to frame-interrupt tick (line 192,
   tick 607 in the 192-line mode on NTSC and PAL alike, SMS-SYSTEM section 2),
   so starting there makes every frame, the first included, exactly one frame
   long. That tick's frame event counts
   as already passed. The interrupt tick itself is the VDP's to define; these
   two constants only place the start. */
#define BUS_ORIGIN_LINE 192u
#define BUS_ORIGIN_TICK 607u

uint32_t cogmaster__frame_ticks(const cogmaster *m)
{
    return m->tv == COGMASTER_TV_PAL ? COGMASTER__TICKS_PER_FRAME_PAL
                                     : COGMASTER__TICKS_PER_FRAME_NTSC;
}

uint32_t cogmaster__frame_lines(const cogmaster *m)
{
    return m->tv == COGMASTER_TV_PAL ? COGMASTER__LINES_PAL : COGMASTER__LINES_NTSC;
}

void cogmaster__bus_origin(cogmaster *m)
{
    m->ticks = 0u;
    m->frame = 0u;
    m->bus.at = 0u;
    m->bus.vdp_at = 0u;
    m->bus.line = BUS_ORIGIN_LINE;
    m->bus.t = BUS_ORIGIN_TICK;
    m->bus.op_pc = 0u;
    m->bus.op = 0u;
    m->bus.deadline = cogmaster__frame_ticks(m);
}

/* The beam only ever moves by the ticks it is synced over, and the test card
   adds a whole frame to both, so it is the origin advanced by at, modulo the
   frame. The offset into the frame is reduced first, so nothing here divides
   a 64-bit count. */
uint32_t cogmaster__bus_beam_at(const cogmaster *m, uint64_t frame, uint64_t at)
{
    uint32_t ft = cogmaster__frame_ticks(m);
    uint64_t start = frame * ft;
    uint32_t into = at >= start ? (uint32_t)(at - start) : ft - (uint32_t)(start - at);
    return (BUS_ORIGIN_LINE * BUS_LINE_TICKS + BUS_ORIGIN_TICK + into) % ft;
}

/* Brings the beam up to m->ticks, firing each VDP event the beam reaches
   on the way, so an access sees every event at or before its stamp. A Z80
   step is at most 23 T-states (69 ticks), so the loop runs a few times per
   access at most. */
void cogmaster__bus_sync(cogmaster *m)
{
    struct cogmaster__bus *b = &m->bus;
    uint32_t lines = cogmaster__frame_lines(m);
    uint32_t left = (uint32_t)(m->ticks - b->at);
    b->at = m->ticks;
    while (left > 0u) {
        uint32_t next = cogmaster__vdp_next_event(m, b->line, b->t);
        uint32_t gap = next - b->t;
        if (left < gap) {
            b->t += left;
            break;
        }
        left -= gap;
        if (next == BUS_LINE_TICKS) {
            b->t = 0u;
            b->line = b->line + 1u == lines ? 0u : b->line + 1u;
        } else {
            b->t = next;
            cogmaster__vdp_event(m, b->line, next);
        }
    }
}

/* No step loops inside itself (prefix byte, block-op iteration, halted M1
   and interrupt acceptance are each one step of at most 69 ticks), so the
   frame stops at most 68 ticks past its deadline, and the overshoot is
   charged to the next frame (ARCHITECTURE section 2). */
uint32_t cogmaster__bus_run_frame(cogmaster *m)
{
    uint64_t deadline = m->bus.deadline;
    while (m->ticks < deadline) {
        cogmaster__z80_step(m);
    }
    cogmaster__bus_sync(m);
    m->bus.deadline = deadline + cogmaster__frame_ticks(m);
    m->frame++;
    return (uint32_t)(m->ticks - deadline);
}

/* $3E b6 and b4, active low, take the cartridge and work RAM off the bus
   (SMS-SYSTEM section 4). */
#define BUS_CART_OFF 0x40u
#define BUS_RAM_OFF 0x10u

/* A read nobody drives: the slot area with the cartridge off (the SMS2 and
   the Game Gear have no card, expansion or BIOS to answer instead) or work
   RAM off. */
static int mem_undriven(const cogmaster *m, uint16_t addr)
{
    uint8_t off = addr >= 0xC000u ? BUS_RAM_OFF : BUS_CART_OFF;
    return (m->io.mem_ctrl & off) != 0u;
}

/* Cartridge slots at $0000-$BFFF through the board's mapper, 8 KiB of work
   RAM at $C000 mirrored up to $FFFF (SMS-SYSTEM section 3). Changes
   nothing, so peek shares it. */
static uint8_t mem_driven(const cogmaster *m, uint16_t addr)
{
    if (addr >= 0xC000u) {
        return m->ram[addr & (COGMASTER__RAM_SIZE - 1u)];
    }
    return cogmaster__mapper_read(m, addr);
}

static uint8_t mem_read(cogmaster *m, uint16_t addr)
{
    if ((m->io.mem_ctrl & (BUS_CART_OFF | BUS_RAM_OFF)) != 0u && mem_undriven(m, addr)) {
        cogmaster__bus_fault(m, addr,
                             addr >= 0xC000u ? COGMASTER__DEVICE_RAM : COGMASTER__DEVICE_CART,
                             COGMASTER__FAULT_UNMAPPED_READ);
        return cogmaster__io_open_bus(m);
    }
    return mem_driven(m, addr);
}

/* What the CPU would read at addr, without the read: no beam sync, no
   fault and no open-bus count. An undriven address reads $FF, the value
   cogmaster__io_open_bus returns. */
#define BUS_PEEK_NONE 0x100u

uint32_t cogmaster_peek(const cogmaster *m, uint32_t space, uint32_t addr)
{
    if (m == NULL || !m->loaded || space != COGMASTER_SPACE_BUS || addr > 0xFFFFu) {
        return BUS_PEEK_NONE;
    }
    if (mem_undriven(m, (uint16_t)addr)) {
        return 0xFFu;
    }
    return mem_driven(m, (uint16_t)addr);
}

uint8_t cogmaster__bus_fetch(cogmaster *m, uint16_t addr, uint16_t refresh)
{
    (void)refresh;
    cogmaster__bus_sync(m);
    return mem_read(m, addr);
}

uint8_t cogmaster__bus_read(cogmaster *m, uint16_t addr)
{
    cogmaster__bus_sync(m);
    return mem_read(m, addr);
}

/* A write to the Sega registers at $FFFC-$FFFF also lands in work RAM, the
   only place the CPU can read them back (SMS-CARTRIDGES section 2.1). With
   work RAM off only the registers see it. */
void cogmaster__bus_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    cogmaster__bus_sync(m);
    if (addr >= 0xC000u) {
        if ((m->io.mem_ctrl & BUS_RAM_OFF) == 0u) {
            m->ram[addr & (COGMASTER__RAM_SIZE - 1u)] = v;
        }
        if (addr < 0xFFFCu) {
            return;
        }
    }
    cogmaster__mapper_write(m, addr, v);
}

/* The VDP's port for an I/O address, or -1. The export Master System
   decodes A7, A6 and A0 only, so $40-$7F and $80-$BF mirror $7E/$7F and
   $BE/$BF; the Japanese console and the Game Gear decode the full address
   (SMS-SYSTEM section 4, SMS-VDP section 2.1). */
static int vdp_port(const cogmaster *m, uint16_t port)
{
    uint32_t p = port & 0xFFu;
    if (m->machine == COGMASTER_MACHINE_GG || m->region == COGMASTER_REGION_JAPAN) {
        if (p != 0x7Eu && p != 0x7Fu && p != 0xBEu && p != 0xBFu) {
            return -1;
        }
    }
    switch (p & 0xC1u) {
    case 0x40u:
        return COGMASTER__VDP_VCOUNT;
    case 0x41u:
        return COGMASTER__VDP_HCOUNT;
    case 0x80u:
        return COGMASTER__VDP_DATA;
    case 0x81u:
        return COGMASTER__VDP_CONTROL;
    default:
        return -1;
    }
}

/* Every port the VDP does not answer belongs to the I/O chip. */
uint8_t cogmaster__bus_in(cogmaster *m, uint16_t port)
{
    cogmaster__bus_sync(m);
    int vp = vdp_port(m, port);
    if (vp >= 0) {
        m->bus.vdp_at = m->ticks;
        return cogmaster__vdp_read(m, (uint32_t)vp);
    }
    return cogmaster__io_in(m, port);
}

/* Writes to $40-$7F go to the sound chip, which is not modelled yet. */
void cogmaster__bus_out(cogmaster *m, uint16_t port, uint8_t v)
{
    cogmaster__bus_sync(m);
    int vp = vdp_port(m, port);
    if (vp == COGMASTER__VDP_DATA || vp == COGMASTER__VDP_CONTROL) {
        m->bus.vdp_at = m->ticks;
        cogmaster__vdp_write(m, (uint32_t)vp, v);
    } else if ((port & 0xC0u) != 0x40u) {
        cogmaster__io_out(m, port, v);
    }
}

/* A dot is two ticks (SMS-SYSTEM section 1). */
static void fault_at(cogmaster *m, uint16_t pc, uint16_t addr, uint8_t op, uint8_t device,
                     uint8_t kind)
{
    struct cogmaster__fault f;
    memset(&f, 0, sizeof f);
    f.frame = m->frame;
    f.line = m->bus.line;
    f.dot = m->bus.t >> 1;
    f.pc = pc;
    f.addr = addr;
    f.opcode = op;
    f.width = 8u;
    f.device = device;
    f.kind = kind;
    cogmaster__fault_record(m, &f);
}

void cogmaster__bus_fault(cogmaster *m, uint16_t addr, uint8_t device, uint8_t kind)
{
    fault_at(m, m->bus.op_pc, addr, m->bus.op, device, kind);
}

/* The step has fetched ED and op, so the instruction started two bytes
   back. */
void cogmaster__bus_ed_undefined(cogmaster *m, uint8_t op)
{
    cogmaster__bus_sync(m);
    fault_at(m, (uint16_t)(m->z80.pc - 2u), (uint16_t)(m->z80.pc - 1u), op, COGMASTER__DEVICE_CPU,
             COGMASTER__FAULT_ED_UNDEFINED);
}
