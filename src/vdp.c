/* The VDP's timing half: the beam events it raises (the frame interrupt
   and the Pause NMI), the V and H counters, the control and data ports with
   VRAM, CRAM and R0-R10, the status byte and /INT. The companion renderer
   consumes these stores.

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

/* The H counter reads the upper 8 bits of the 9-bit pixel counter, which
   runs $00-$93, then $E9-$FF (SMS-VDP section 4.3). Translate our active
   origin by 32 ticks before looking it up: SMS dot 0 = HC08 and GG dot 48
   = HC20. This sub-dot phase is a coordinate inference, not a measurement
   (video.md and the final SMS Display Timing forum posts). */
#define VDP_HCOUNT_JUMP 592u
#define VDP_HCOUNT_SKIP 340u
#define VDP_HCOUNT_ORIGIN 32u

/* Pause: the console latches a press and the VDP asserts /NMI as /HSYNC
   rises at tick 611 of the next-to-last line, which starts the frame's
   last line (Charles MacDonald's logic analyser: "the /HSYNC edge 611 mclk
   into line 260"; SMS-SYSTEM sections 2 and 7). */
#define VDP_NMI_TICK 611u

/* The frame interrupt: the pending flag rises at tick 607 of line 192, the
   first line after the 192-line active display, 4 ticks before /HSYNC rises
   (Charles MacDonald's logic analyser, SMS-SYSTEM section 2; SMS-VDP
   section 4.2). Extended modes select line H with the same named tick;
   the fixed frame origin in bus.c stays where it is. */
#define VDP_INT_TICK 607u
#define VDP_LINE_INT_TICK 608u
#define VDP_LATCH_TICK 600u

/* Status flags: bit 7 frame interrupt pending, 6 sprite overflow, 5 sprite
   collision. Bits 4-0 are undriven and read as 1s (SMS-VDP section 2.6,
   choice 2.A). */
#define VDP_STATUS_FRAME 0x80u
#define VDP_STATUS_LOW 0x1Fu

/* R1 bit 5, IE0: the frame interrupt reaches /INT (SMS-VDP section 2.4). */
#define VDP_R1_IE0 0x20u
#define VDP_R0_IE1 0x10u

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
    v->height = 192u;
    v->cram_latch = 0u;
    if (power) {
        memset(&m->lcd, 0, sizeof m->lcd);
        m->lcd.height = 192u;
        m->lcd.y = 24u;
    }
    v->addr = 0u;
    v->vline = 193u; /* bus origin line 192, after the 604 V step */
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
    v->line_counter = 0xFFu;
    v->line_pending = 0u;
    v->hcounter = 0u;
    v->hheld = 0u;
    v->hscroll = 0u;
    v->vscroll = 0u;
    v->display = 0u;
    memset(v->sprite, 0, sizeof v->sprite);
    v->sprite_line = COGMASTER__FIELD_H;
    v->sprite_count = 0u;
    v->sprite_scanned = 0u;
    v->sprite_zoom = 0u;
    v->sprite_shift = 0u;
    v->sprite_overflow = 0u;
    memset(&m->completed, 0, sizeof m->completed);
    m->completed.height = 192u;
    m->completed.crop =
        cogmaster__crop(cogmaster__native_gg(m) ? COGMASTER_MACHINE_GG : COGMASTER_MACHINE_SMS2);
    m->completed.par_num = cogmaster__native_gg(m) ? 6u : m->tv == COGMASTER_TV_PAL ? 2950000u : 8u;
    m->completed.par_den = cogmaster__native_gg(m) ? 5u : m->tv == COGMASTER_TV_PAL ? 2128137u : 7u;
    m->render_line = 0u;
    m->render_x = 0u;
    if (power) {
        memset(v->cram, 0, sizeof v->cram);
        memset(v->vram, 0, sizeof v->vram);
    }
}

/* /INT is a level: either pending source gated by its IE, held until a status read
   (SMS-VDP section 2.6 and section 4.6). */
static void vdp_irq(cogmaster *m)
{
    const struct cogmaster__vdp *v = &m->vdp;
    m->z80.int_line =
        (uint8_t)(((v->status & VDP_STATUS_FRAME) != 0u && (v->reg[1] & VDP_R1_IE0) != 0u) ||
                  (v->line_pending != 0u && (v->reg[0] & VDP_R0_IE1) != 0u));
}

/* Mode 4 extended heights on SMS2/GG; SMS1 invalid text combinations
   retain the 192-line timing fallback (SMS-VDP section 2.5). */
static uint32_t vdp_pending_height(const cogmaster *m)
{
    const uint8_t *r = m->vdp.reg;
    if (!cogmaster__profile(m).vdp_masks && (r[0] & 6u) == 6u) {
        if ((r[1] & 0x18u) == 0x10u) {
            return 224u;
        }
        if ((r[1] & 0x18u) == 0x08u) {
            return 240u;
        }
    }
    return 192u;
}

static int vdp_rolling(const cogmaster *m)
{
    return m->tv == COGMASTER_TV_NTSC && m->vdp.height == 240u;
}

/* The line /NMI is asserted on. The Game Gear has no Pause button: in its
   own mode Start is a port bit (SMS-SYSTEM section 7), so it has none. */
static uint32_t vdp_nmi_line(const cogmaster *m)
{
    if (cogmaster__native_gg(m)) {
        return UINT32_MAX;
    }
    return cogmaster__frame_lines(m) - 2u;
}

uint32_t cogmaster__vdp_next_event(const cogmaster *m, uint32_t line, uint32_t t)
{
    if (line < m->vdp.height && t < 512u) {
        return 512u;
    }
    if (t < VDP_LATCH_TICK) {
        return VDP_LATCH_TICK;
    }
    if (vdp_rolling(m) && t < VDP_VCOUNT_TICK) {
        return VDP_VCOUNT_TICK;
    }
    if (line == m->vdp.height && t < VDP_INT_TICK) {
        return VDP_INT_TICK;
    }
    if (t < VDP_LINE_INT_TICK) {
        return VDP_LINE_INT_TICK;
    }
    if (line == vdp_nmi_line(m) && t < VDP_NMI_TICK) {
        return VDP_NMI_TICK;
    }
    return VDP_LINE_TICKS;
}

void cogmaster__vdp_event(cogmaster *m, uint32_t line, uint32_t t)
{
    /* The documented NTSC-240 counter rolls through 263 values while the
       host keeps a 262-line frame. This state never changes bus deadlines
       (SMS-VDP section 4.4; selected separation in video.md). */
    if (t == VDP_VCOUNT_TICK && vdp_rolling(m)) {
        m->vdp.vline = m->vdp.vline == 262u ? 0u : (uint16_t)(m->vdp.vline + 1u);
    }
    if (line < m->vdp.height && t == 512u) {
        cogmaster__vdp_render_to(m, line + 1u);
    }
    /* Selected tick-600 next-line latch; R9 latches before line zero
       once per field (docs/hardware/video.md). */
    if (t == VDP_LATCH_TICK) {
        m->vdp.hscroll = m->vdp.reg[8];
        m->vdp.display = (uint8_t)((m->vdp.reg[1] >> 6) & 1u);
        if (line + 1u == cogmaster__frame_lines(m)) {
            m->vdp.vscroll = m->vdp.reg[9];
            int rolling = vdp_rolling(m);
            m->vdp.height = (uint8_t)vdp_pending_height(m);
            if (!rolling && vdp_rolling(m)) {
                m->vdp.vline = (uint16_t)line;
            }
            m->render_line = 0u;
            m->render_x = 0u;
            m->vdp.sprite_line = m->vdp.height;
        }
        uint32_t next = line + 1u == cogmaster__frame_lines(m) ? 0u : line + 1u;
        if (next < m->vdp.height) {
            cogmaster__vdp_sprite_scan(m, next);
        }
    }
    if (line == m->vdp.height && t == VDP_INT_TICK) {
        cogmaster__vdp_finish_field(m);
        m->vdp.status = (uint8_t)(m->vdp.status | VDP_STATUS_FRAME);
        vdp_irq(m);
    }
    /* Active lines plus the first postactive line decrement/test; all
       other lines reload without raising pending (SMS-VDP section 4.5).
       R10 writes only affect the next reload. */
    if (t == VDP_LINE_INT_TICK) {
        struct cogmaster__vdp *v = &m->vdp;
        if (line > m->vdp.height || v->line_counter == 0u) {
            if (line <= m->vdp.height) {
                v->line_pending = 1u;
            }
            v->line_counter = v->reg[10];
        } else {
            v->line_counter--;
        }
        vdp_irq(m);
    }
    if (line == vdp_nmi_line(m) && t == VDP_NMI_TICK && m->vdp.pause) {
        m->vdp.pause = 0u;
        m->z80.nmi_pending = 1u;
    }
}

/* Literal interval boundaries from SMS-VDP section 4.4; uint8 conversion
   supplies the initial 00 wrap in the extended PAL modes. */
static uint8_t vdp_vcount(const cogmaster *m)
{
    uint32_t height = m->vdp.height;
    if (vdp_rolling(m)) {
        return (uint8_t)m->vdp.vline;
    }
    uint32_t line = m->bus.line;
    if (m->bus.t >= VDP_VCOUNT_TICK) {
        line = line + 1u == cogmaster__frame_lines(m) ? 0u : line + 1u;
    }
    if (m->tv == COGMASTER_TV_PAL) {
        uint32_t jump = height == 224u ? 258u : height == 240u ? 266u : 242u;
        return (uint8_t)(line > jump ? line - 57u : line);
    }
    return (uint8_t)(line > (height == 224u ? 234u : 218u) ? line - 6u : line);
}

static uint8_t vdp_hcount(const cogmaster *m)
{
    uint32_t t = (m->bus.t + VDP_HCOUNT_ORIGIN) % VDP_LINE_TICKS;
    return (uint8_t)((t >= VDP_HCOUNT_JUMP ? t + VDP_HCOUNT_SKIP : t) >> 2);
}

void cogmaster__vdp_latch_h(cogmaster *m)
{
    m->vdp.hcounter = vdp_hcount(m);
    m->vdp.hheld = 1u;
}

/* A status read returns the flags with the low bits set, then clears the
   flags and the control latch and releases /INT (SMS-VDP section 2.6). */
static uint8_t vdp_status_read(cogmaster *m)
{
    /* Events at this tick precede the read: include its current active dot,
       then clear. The monotonic cursor prevents reasserting that same dot
       on a second read (SMS-VDP section 3; selected model in video.md). */
    if (m->bus.line < m->vdp.height) {
        cogmaster__vdp_render_until(m, m->bus.line, m->bus.t / 2u + 1u);
    }
    struct cogmaster__vdp *v = &m->vdp;
    uint8_t s = (uint8_t)(v->status | VDP_STATUS_LOW);
    v->status = 0u;
    v->line_pending = 0u;
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
        /* Native GG commits both bytes only on an odd address; an even
           write changes the shared latch (SMS-VDP section 6.2). */
        if (cogmaster__native_gg(m)) {
            if ((v->addr & 1u) != 0u) {
                v->cram[(v->addr & 62u)] = v->cram_latch;
                v->cram[(v->addr & 63u)] = b & 15u;
            } else {
                v->cram_latch = b;
            }
        } else {
            v->cram[v->addr & 31u] = b;
        }
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
        return m->vdp.hheld ? m->vdp.hcounter : vdp_hcount(m);
    case COGMASTER__VDP_DATA:
        return vdp_data_read(m);
    default:
        return vdp_status_read(m);
    }
}

void cogmaster__vdp_write(cogmaster *m, uint32_t port, uint8_t v)
{
    /* Catch up to this dot before mutating a source (video.md). R8/R9
       and display writes affect their latches, not earlier intervals. */
    if (m->bus.line < m->vdp.height) {
        cogmaster__vdp_render_until(m, m->bus.line, m->bus.t / 2u);
    }
    if (port == COGMASTER__VDP_CONTROL) {
        vdp_control_write(m, v);
    } else if (port == COGMASTER__VDP_DATA) {
        vdp_data_write(m, v);
    }
}
