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
    m->bus.line = BUS_ORIGIN_LINE;
    m->bus.t = BUS_ORIGIN_TICK;
    m->bus.deadline = cogmaster__frame_ticks(m);
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

/* The no-mapper layout: ROM at $0000-$BFFF, 8 KiB of work RAM at $C000
   mirrored up to $FFFF (SMS-SYSTEM section 3). ROM past the image reads $FF;
   ROM writes go nowhere. */
static uint8_t mem_read(const cogmaster *m, uint16_t addr)
{
    if (addr >= 0xC000u) {
        return m->ram[addr & (COGMASTER__RAM_SIZE - 1u)];
    }
    return addr < m->cart.size ? m->cart.rom[addr] : 0xFFu;
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

void cogmaster__bus_write(cogmaster *m, uint16_t addr, uint8_t v)
{
    cogmaster__bus_sync(m);
    if (addr >= 0xC000u) {
        m->ram[addr & (COGMASTER__RAM_SIZE - 1u)] = v;
    }
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
    case 0x40u: return COGMASTER__VDP_VCOUNT;
    case 0x41u: return COGMASTER__VDP_HCOUNT;
    case 0x80u: return COGMASTER__VDP_DATA;
    case 0x81u: return COGMASTER__VDP_CONTROL;
    default: return -1;
    }
}

/* Ports the bus does not decode read $FF, the idle data bus. */
uint8_t cogmaster__bus_in(cogmaster *m, uint16_t port)
{
    cogmaster__bus_sync(m);
    int vp = vdp_port(m, port);
    if (vp >= 0) {
        return cogmaster__vdp_read(m, (uint32_t)vp);
    }
    return 0xFFu;
}

/* Writes to $40-$7F go to the sound chip, which is not modelled yet. */
void cogmaster__bus_out(cogmaster *m, uint16_t port, uint8_t v)
{
    cogmaster__bus_sync(m);
    int vp = vdp_port(m, port);
    if (vp == COGMASTER__VDP_DATA || vp == COGMASTER__VDP_CONTROL) {
        cogmaster__vdp_write(m, (uint32_t)vp, v);
    }
}

/* The step has fetched ED and op, so the instruction started two bytes
   back. A dot is two ticks (SMS-SYSTEM section 1). */
void cogmaster__bus_ed_undefined(cogmaster *m, uint8_t op)
{
    cogmaster__bus_sync(m);
    struct cogmaster__fault f;
    memset(&f, 0, sizeof f);
    f.frame = m->frame;
    f.line = m->bus.line;
    f.dot = m->bus.t >> 1;
    f.pc = (uint16_t)(m->z80.pc - 2u);
    f.addr = (uint16_t)(m->z80.pc - 1u);
    f.opcode = op;
    f.width = 8u;
    f.device = COGMASTER__DEVICE_CPU;
    f.kind = COGMASTER__FAULT_ED_UNDEFINED;
    cogmaster__fault_record(m, &f);
}
