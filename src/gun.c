/* The Light Phaser's sensor: a gun a port's device selects by aiming at the picture. The gun has
   a photodiode behind a lens and holds TH low while the dots in front of it are bright; the
   console reads that on $DD b6 (port 1) or b7 (port 2), and the rise of TH when the light ends
   latches the H counter, which is how a program learns where on the line the gun was pointing.
   A shot is the trigger on TL; the sensor itself responds to light whether or not it is pulled.

   It is written from SMS Power's Light Phaser page, MacDonald's msvdp-20021112.txt section 11 and
   the protocol in SMS-CARTRIDGES-AND-PERIPHERALS section 3, and checked against MAME's
   sms_ctrl/lphaser.cpp and sms_m.cpp (BSD-3, pin 1d7a454ed79e) and ares controller/light-phaser
   (ISC, pin 9408cb43d494); nothing is copied. PITFALLS section 9 is the reason an aim that is not
   a dot of the picture is a state of its own (the gun is parked) and not moved to an edge dot.

   The sensor is lazy. It does not render and does not run per line: a sync brings each gun's
   cursor up to the beam, in time order across the two ports (there is one H latch, so the order
   edges reach it is observable) and a tie goes to port 1. The work is bounded: a gun looks at the
   dots of its disc once per picture, at most 113 dots plus one closing dot on each of 13 rows,
   however often anything polls. A dot is read from the finished picture only once the VDP has
   emitted it, so no poll schedule can change what is seen. To bring a dot into existence the
   sync asks the renderer for dots before the current one only (the bound every VDP write uses),
   never when the picture is over (that would flush the field early) and never while the display is
   on in a mode the renderer rejects (that would record its fault at a position the program did
   not reach). That last case is the one stated exception to schedule independence: the cursor
   then waits for the renderer's own pace.

   Every constant is a model value, not a measurement (see GUN_DELAY_TICKS in internal.h). Where
   this model and MAME agree: the sensor reads the emitted pixel, TH is low while it sees light,
   and the H latch is the rise. Where they differ: MAME releases at the end of the aim's chord,
   this model at the first dot that is not bright, so the two agree on a bright area wider than the
   chord and differ on a smaller one; and MAME gates the TH readback on its latch, which is not
   ported here because the latch never clears in this core. */
#include "internal.h"

/* The half width of the disc's chord on a row dy rows from the aim: the largest h with h squared
   plus dy squared within the radius squared. At most six steps, no table. */
static uint32_t half_width(uint32_t dy)
{
    uint32_t h = GUN_RADIUS;
    while (h > 0u && h * h + dy * dy > GUN_RADIUS * GUN_RADIUS) {
        h--;
    }
    return h;
}

static uint32_t row_dy(uint32_t row, uint32_t aim_row)
{
    return row > aim_row ? row - aim_row : aim_row - row;
}

static uint32_t chord_start(uint32_t aim_dot, uint32_t half)
{
    return aim_dot > half ? aim_dot - half : 0u;
}

static uint32_t chord_end(uint32_t aim_dot, uint32_t half)
{
    uint32_t end = aim_dot + half;
    return end > COGMASTER__FIELD_W - 1u ? COGMASTER__FIELD_W - 1u : end;
}

/* Bright to the sensor: the weighted brightness of the dot's 8-bit expansion of each 4-bit
   channel, red in the low nibble (the finished field's layout), reaches the threshold. */
static uint32_t dot_lit(uint32_t colour)
{
    uint32_t r = (colour & 15u) * 17u;
    uint32_t g = ((colour >> 4) & 15u) * 17u;
    uint32_t b = ((colour >> 8) & 15u) * 17u;
    return ((GUN_WEIGHT_R * r + GUN_WEIGHT_G * g + GUN_WEIGHT_B * b) >> 8) >= GUN_LUMA ? 1u : 0u;
}

/* The order key of a port's next dot: its row, then the tick the sensor sees it at, a dot being
   2 ticks long and seen GUN_DELAY_TICKS after the VDP emits it. The closing dot, 256, is last. */
static uint32_t dot_key(const struct cogmaster__port *p)
{
    return ((uint32_t)p->g_line << 10) | (2u * p->g_dot + GUN_DELAY_TICKS);
}

/* The next row of the disc, or the end of the picture's scan when there is none: past the disc,
   or at or below the bottom of the picture. */
static void next_row(const cogmaster *m, struct cogmaster__port *p)
{
    uint32_t aim_row = (uint32_t)p->g_ay - 1u;
    uint32_t row = (uint32_t)p->g_line + 1u;
    if (row > aim_row + GUN_RADIUS || row >= m->vdp.height) {
        p->g_scan = 0u;
        p->g_line = 0u;
        p->g_dot = 0u;
        return;
    }
    p->g_line = (uint8_t)row;
    p->g_dot = (uint16_t)chord_start((uint32_t)p->g_ax - 1u, half_width(row_dy(row, aim_row)));
}

/* Looks at the port's next dot: a dot of the chord is sensed when its pixel is bright, the dot
   after the chord (256 at the end of the field) never is, so the light ends there. */
static void look(cogmaster *m, uint32_t port)
{
    struct cogmaster__port *p = &m->ports[port];
    uint32_t aim_dot = (uint32_t)p->g_ax - 1u;
    uint32_t half = half_width(row_dy(p->g_line, (uint32_t)p->g_ay - 1u));
    uint32_t dot = p->g_dot;
    uint32_t inside = dot <= chord_end(aim_dot, half) ? 1u : 0u;
    uint32_t seen = inside != 0u ? dot_lit(m->field[p->g_line][dot]) : 0u;
    if (seen != (uint32_t)(p->g_sense & 1u)) {
        cogmaster__io_sensor_edge(m, port, seen, 2u * dot + GUN_DELAY_TICKS);
    }
    m->gun_stats.samples++;
    if (inside != 0u) {
        p->g_dot = (uint16_t)(dot + 1u);
    } else {
        next_row(m, p);
    }
}

/* Whether the VDP has emitted the port's next dot: the picture is over, or its render cursor has
   passed the dot. */
static int emitted(const cogmaster *m, const struct cogmaster__port *p, int over)
{
    return over || p->g_line < m->render_line ||
           (p->g_line == m->render_line && p->g_dot < m->render_x);
}

/* Looks at every dot due at or before the beam (line, t), in time order across the ports. A line
   at or past the picture's height means the picture is over and everything is due. */
static void run(cogmaster *m, uint32_t line, uint32_t t)
{
    int over = line >= m->vdp.height;
    uint32_t beam = over ? UINT32_MAX : (line << 10) | t;
    int rendered = 0;
    for (;;) {
        int pick = -1;
        uint32_t best = 0u;
        for (uint32_t i = 0u; i < 2u; i++) {
            const struct cogmaster__port *p = &m->ports[i];
            if (p->g_scan == 0u || dot_key(p) > beam) {
                continue;
            }
            if (!emitted(m, p, over) && !rendered && !over && !cogmaster__vdp_render_blocked(m)) {
                /* Before the beam's own dot, as a VRAM write does, so the pixels and every status
                   flag come out as they would without the gun. */
                cogmaster__vdp_render_until(m, line, t / 2u);
                rendered = 1;
            }
            if (emitted(m, p, over) && (pick < 0 || dot_key(p) < best)) {
                pick = (int)i;
                best = dot_key(p);
            }
        }
        if (pick < 0) {
            return;
        }
        look(m, (uint32_t)pick);
    }
}

void cogmaster__gun_sync(cogmaster *m)
{
    m->gun_stats.syncs++;
    /* From the wrap to the end of the last line the next picture is armed and has not begun. */
    if (m->bus.line + 1u == cogmaster__frame_lines(m) && m->bus.t >= COGMASTER__VDP_WRAP_TICK) {
        return;
    }
    run(m, m->bus.line, m->bus.t);
}

/* Arms a port for the picture that begins: the aim in force is snapshotted, so a picture that
   reaches into the next frame's window keeps the aim it began with, and the scan starts at the
   disc's first row inside the picture. No aim, or none of the disc in the picture, is parked. */
static void arm(cogmaster *m, uint32_t port)
{
    struct cogmaster__port *p = &m->ports[port];
    uint32_t ax = (uint32_t)(port == 0u ? m->input.x1 : m->input.x2);
    uint32_t ay = (uint32_t)(port == 0u ? m->input.y1 : m->input.y2);
    uint32_t first = ay > GUN_RADIUS + 1u ? ay - 1u - GUN_RADIUS : 0u;
    p->g_ax = (uint16_t)ax;
    p->g_ay = (uint8_t)ay;
    p->g_scan = 0u;
    p->g_line = 0u;
    p->g_dot = 0u;
    if (ax != 0u && first < m->vdp.height) {
        p->g_scan = 1u;
        p->g_line = (uint8_t)first;
        p->g_dot = (uint16_t)chord_start(ax - 1u, half_width(row_dy(first, ay - 1u)));
    }
}

/* The wrap to the next picture, at the tick the VDP latches its registers. The old picture is
   over, so what the sensor has left of it is finished first, then the light pulse bit of every
   port is dropped (a pulse never outlives its picture) and each gun is armed. */
void cogmaster__gun_wrap(cogmaster *m)
{
    run(m, UINT32_MAX, 0u);
    for (uint32_t port = 0u; port < 2u; port++) {
        uint32_t device = port == 0u ? m->input.port1 : m->input.port2;
        if (device == COGMASTER_DEVICE_LIGHT_PHASER) {
            m->ports[port].g_sense &= (uint8_t)~2u;
            arm(m, port);
        }
    }
}
