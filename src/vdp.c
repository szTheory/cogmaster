/* The VDP's timing half: the beam events it raises (the frame interrupt
   and the Pause NMI), the V and H counters, the control and data ports with
   VRAM, CRAM and R0-R10, the status byte and /INT. There is no renderer yet; the mode bits in R0
   and R1 are stored and the mode is treated as 192 lines.

   Time is in ticks, one per VDP clock; t is the tick in a 684-tick line,
   with t 0 at the first active dot (SMS-VDP sections 2 and 4, SMS-SYSTEM
   section 2). */
#include <string.h>

#include "internal.h"

#define VDP_LINE_TICKS 684u

/* The CPU sees the V counter step to the next line's value at tick 604
   (SMS-VDP section 4.2 and section 4.9; SMS-SYSTEM section 2 measures "at or
   after 599"). */
#define VDP_VCOUNT_TICK 604u

/* The 192-line V counter reads lines 0-218 as $00-$DA and the rest from $D5
   on NTSC, lines 0-242 as $00-$F2 and the rest from $BA on PAL, so both
   end at $FF (SMS-VDP section 4.4). */
#define VDP_VCOUNT_JUMP_NTSC 0xDAu
#define VDP_VCOUNT_BACK_NTSC 6u
#define VDP_VCOUNT_JUMP_PAL 0xF2u
#define VDP_VCOUNT_BACK_PAL 57u

/* The H counter reads the upper 8 bits of the 9-bit pixel counter, which
   jumps from pixel 295 to 466 at tick 592: $00-$93, then $E9-$FF (SMS-VDP
   section 4.3, the formula of ares). */
#define VDP_HCOUNT_JUMP 592u
#define VDP_HCOUNT_SKIP 340u

/* Pause: the console latches a press and the VDP asserts /NMI as /HSYNC
   rises at tick 611 of the next-to-last line, which starts the frame's
   last line (Charles MacDonald's logic analyser: "the /HSYNC edge 611 mclk
   into line 260"; SMS-SYSTEM sections 2 and 7). */
#define VDP_NMI_TICK 611u

/* The frame interrupt: the pending flag rises at tick 607 of line 192, the
   first line after the 192-line active display, 4 ticks before /HSYNC rises
   (Charles MacDonald's logic analyser, SMS-SYSTEM section 2; SMS-VDP
   section 4.2). The 224- and 240-line modes and the unsettled later
   estimate may move it; the frame origin in bus.c stays where it is. */
#define VDP_INT_TICK 607u
#define VDP_INT_LINE 192u

/* Status flags: bit 7 frame interrupt pending, 6 sprite overflow, 5 sprite
   collision. Bits 4-0 are undriven and read as 1s (SMS-VDP section 2.6,
   choice 2.A). */
#define VDP_STATUS_FRAME 0x80u
#define VDP_STATUS_LOW 0x1Fu

/* R1 bit 5, IE0: the frame interrupt reaches /INT (SMS-VDP section 2.4). */
#define VDP_R1_IE0 0x20u

/* The 14-bit VRAM address (SMS-VDP section 2.2). */
#define VDP_ADDR_MASK 0x3FFFu

/* The control codes in the second control byte's bits 7-6 (SMS-VDP
   section 2.2). */
#define VDP_CODE_VRAM_READ 0u
#define VDP_CODE_REGISTER 2u
#define VDP_CODE_CRAM_WRITE 3u

void cogmaster__vdp_reset(cogmaster *m, int power)
{
    struct cogmaster__vdp *v = &m->vdp;
    v->addr = 0u;
    memset(v->reg, 0, sizeof v->reg);
    /* The registers as the BIOS leaves them (ARCHITECTURE section 9). */
    v->reg[2] = 0x0Eu;
    v->reg[10] = 0xFFu;
    v->status = 0u;
    v->latch = 0u;
    v->first = 0u;
    v->code = 0u;
    v->buffer = 0u;
    v->pause = 0u;
    m->render_line = 0u;
    if (power) {
        memset(v->cram, 0, sizeof v->cram);
        memset(v->vram, 0, sizeof v->vram);
    }
}

/* /INT is a level: the frame flag AND IE0, held until a status read
   (SMS-VDP section 2.6 and section 4.6). */
static void vdp_irq(cogmaster *m)
{
    const struct cogmaster__vdp *v = &m->vdp;
    m->z80.int_line =
        (uint8_t)((v->status & VDP_STATUS_FRAME) != 0u && (v->reg[1] & VDP_R1_IE0) != 0u);
}

/* The line /NMI is asserted on. The Game Gear has no Pause button: in its
   own mode Start is a port bit (SMS-SYSTEM section 7), so it has none. */
static uint32_t vdp_nmi_line(const cogmaster *m)
{
    if (m->machine == COGMASTER_MACHINE_GG) {
        return UINT32_MAX;
    }
    return cogmaster__frame_lines(m) - 2u;
}

uint32_t cogmaster__vdp_next_event(const cogmaster *m, uint32_t line, uint32_t t)
{
    if (line < COGMASTER__FIELD_H && t < 512u) {
        return 512u;
    }
    if (line == VDP_INT_LINE && t < VDP_INT_TICK) {
        return VDP_INT_TICK;
    }
    if (line == vdp_nmi_line(m) && t < VDP_NMI_TICK) {
        return VDP_NMI_TICK;
    }
    return VDP_LINE_TICKS;
}

void cogmaster__vdp_event(cogmaster *m, uint32_t line, uint32_t t)
{
    if (line < COGMASTER__FIELD_H && t == 512u) {
        cogmaster__vdp_render_to(m, line + 1u);
    }
    if (line == VDP_INT_LINE && t == VDP_INT_TICK) {
        cogmaster__vdp_finish_field(m);
        m->vdp.status = (uint8_t)(m->vdp.status | VDP_STATUS_FRAME);
        vdp_irq(m);
    } else if (line == vdp_nmi_line(m) && t == VDP_NMI_TICK && m->vdp.pause) {
        m->vdp.pause = 0u;
        m->z80.nmi_pending = 1u;
    }
}

/* The V counter at the beam: the line's value until tick 604, then the next
   line's. The mode is treated as 192 lines. */
static uint8_t vdp_vcount(const cogmaster *m)
{
    uint32_t line = m->bus.line;
    if (m->bus.t >= VDP_VCOUNT_TICK) {
        line = line + 1u == cogmaster__frame_lines(m) ? 0u : line + 1u;
    }
    if (m->tv == COGMASTER_TV_PAL) {
        return (uint8_t)(line > VDP_VCOUNT_JUMP_PAL ? line - VDP_VCOUNT_BACK_PAL : line);
    }
    return (uint8_t)(line > VDP_VCOUNT_JUMP_NTSC ? line - VDP_VCOUNT_BACK_NTSC : line);
}

static uint8_t vdp_hcount(const cogmaster *m)
{
    uint32_t t = m->bus.t;
    return (uint8_t)((t >= VDP_HCOUNT_JUMP ? t + VDP_HCOUNT_SKIP : t) >> 2);
}

/* A status read returns the flags with the low bits set, then clears the
   flags and the control latch and releases /INT (SMS-VDP section 2.6). */
static uint8_t vdp_status_read(cogmaster *m)
{
    struct cogmaster__vdp *v = &m->vdp;
    uint8_t s = (uint8_t)(v->status | VDP_STATUS_LOW);
    v->status = 0u;
    v->latch = 0u;
    vdp_irq(m);
    return s;
}

/* The control port takes two bytes: the address's low 8 bits, then its
   high 6 bits with the code in bits 7-6. Code 0 loads the read-ahead byte
   and steps the address; code 2 writes the first byte to the register the
   second byte's low 4 bits name, and R11-R15 do not exist
   (SMS-VDP section 2.2). */
static void vdp_control_write(cogmaster *m, uint8_t b)
{
    struct cogmaster__vdp *v = &m->vdp;
    if (!v->latch) {
        v->first = b;
        v->addr = (uint16_t)((v->addr & 0x3F00u) | b);
        v->latch = 1u;
        return;
    }
    v->latch = 0u;
    v->code = (uint8_t)(b >> 6);
    v->addr = (uint16_t)(((uint32_t)(b & 0x3Fu) << 8) | v->first);
    if (v->code == VDP_CODE_VRAM_READ) {
        v->buffer = v->vram[v->addr];
        v->addr = (uint16_t)((v->addr + 1u) & VDP_ADDR_MASK);
    } else if (v->code == VDP_CODE_REGISTER) {
        uint32_t r = b & 0x0Fu;
        if (r < COGMASTER__VDP_REGS) {
            v->reg[r] = v->first;
            vdp_irq(m);
        }
    }
}

/* Every data-port access clears the control latch and steps the address.
   A read returns the read-ahead byte and refills it from VRAM at the
   address the last access stepped to, so reads stream VRAM one byte
   behind the address; a write stores the byte in CRAM under code 3, in VRAM under the
   other codes, and in the read-ahead byte (SMS-VDP section 2.2 and section 2.3). */
static uint8_t vdp_data_read(cogmaster *m)
{
    struct cogmaster__vdp *v = &m->vdp;
    uint8_t b = v->buffer;
    v->latch = 0u;
    v->buffer = v->vram[v->addr];
    v->addr = (uint16_t)((v->addr + 1u) & VDP_ADDR_MASK);
    return b;
}

static void vdp_data_write(cogmaster *m, uint8_t b)
{
    struct cogmaster__vdp *v = &m->vdp;
    v->latch = 0u;
    if (v->code == VDP_CODE_CRAM_WRITE) {
        v->cram[v->addr & (COGMASTER__CRAM_SIZE - 1u)] = b;
    } else {
        v->vram[v->addr] = b;
    }
    v->buffer = b;
    v->addr = (uint16_t)((v->addr + 1u) & VDP_ADDR_MASK);
}

uint8_t cogmaster__vdp_read(cogmaster *m, uint32_t port)
{
    switch (port) {
    case COGMASTER__VDP_VCOUNT:
        return vdp_vcount(m);
    case COGMASTER__VDP_HCOUNT:
        return vdp_hcount(m);
    case COGMASTER__VDP_DATA:
        return vdp_data_read(m);
    default:
        return vdp_status_read(m);
    }
}

void cogmaster__vdp_write(cogmaster *m, uint32_t port, uint8_t v)
{
    /* Catch up before a write changes a line's source (SMS-VDP section 6).
       This line renderer samples a whole line at its first visible write. */
    if (m->bus.line < COGMASTER__FIELD_H) {
        cogmaster__vdp_render_to(m, m->bus.line + (m->bus.t != 0u ? 1u : 0u));
    }
    if (port == COGMASTER__VDP_CONTROL) {
        vdp_control_write(m, v);
    } else if (port == COGMASTER__VDP_DATA) {
        vdp_data_write(m, v);
    }
}
