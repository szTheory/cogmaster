/* The VDP's timing half: the beam events it raises, the control port, the
   status byte and /INT. There is no renderer yet; the mode bits in R0 and
   R1 are stored and the mode is treated as 192 lines.

   Time is in ticks, one per VDP clock; t is the tick in a 684-tick line,
   with t 0 at the first active dot (SMS-VDP sections 2 and 4, SMS-SYSTEM
   section 2). */
#include <string.h>

#include "internal.h"

#define VDP_LINE_TICKS 684u

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
    if (power) {
        memset(v->cram, 0, sizeof v->cram);
        memset(v->vram, 0, sizeof v->vram);
    }
}

/* /INT is a level: the frame flag AND IE0, held until a status read
   (SMS-VDP sections 2.6 and 4.6). */
static void vdp_irq(cogmaster *m)
{
    const struct cogmaster__vdp *v = &m->vdp;
    m->z80.int_line = (uint8_t)((v->status & VDP_STATUS_FRAME) != 0u &&
                                (v->reg[1] & VDP_R1_IE0) != 0u);
}

uint32_t cogmaster__vdp_next_event(const cogmaster *m, uint32_t line, uint32_t t)
{
    (void)m;
    if (line == VDP_INT_LINE && t < VDP_INT_TICK) {
        return VDP_INT_TICK;
    }
    return VDP_LINE_TICKS;
}

void cogmaster__vdp_event(cogmaster *m, uint32_t line, uint32_t t)
{
    if (line == VDP_INT_LINE && t == VDP_INT_TICK) {
        m->vdp.status = (uint8_t)(m->vdp.status | VDP_STATUS_FRAME);
        vdp_irq(m);
    }
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
   second byte's low 4 bits name, and R11-R15 do not exist (SMS-VDP section
   2.2). */
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

uint8_t cogmaster__vdp_read(cogmaster *m, uint32_t port)
{
    if (port == COGMASTER__VDP_CONTROL) {
        return vdp_status_read(m);
    }
    return 0xFFu;
}

void cogmaster__vdp_write(cogmaster *m, uint32_t port, uint8_t v)
{
    if (port == COGMASTER__VDP_CONTROL) {
        vdp_control_write(m, v);
    }
}
