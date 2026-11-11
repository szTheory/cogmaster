/* The two controller ports at pin level (SMS-CARTRIDGES-AND-PERIPHERALS
   section 3, "Port bit map"). A port's six device-driven pins are b0 Up (D0)
   to b3 Right (D3), b4 TL and b5 TR, 0 = driven low; TH belongs to the
   console, which mixes it, the reset button and any TR it drives itself in
   io.c. The pin function is pure: it reads the instance and changes nothing,
   so a read of $DC or $DD cannot move a device.

   The Sports Pad and the Paddle are written from the protocol in
   SMS-CARTRIDGES-AND-PERIPHERALS section 3.4 and the two after it, and checked against
   MAME's sms_ctrl/sports.cpp and sms_ctrl/paddle.cpp (BSD-3, pin 1d7a454ed79e)
   and ares ms/controller/sports-pad and ms/controller/paddle (ISC, pin
   9408cb43d494); nothing is copied. */
#include <string.h>

#include "internal.h"

#define PORT_PINS 0x3Fu
#define PIN_TL 0x10u
#define PIN_TR 0x20u

/* The Sports Pad reads out X then Y, a high nibble at each TH fall and the low
   nibble at the rise after it (MAME sports.cpp out_w; ares sports-pad.cpp
   write). Step 0 is the pad before its first read and presents $F; steps 1 to 4
   present the high and low nibble of X, then of Y, and step 4 keeps the last
   nibble until the next fall, as MAME holds its output through its timeout (ares
   likewise never returns to $F). The pad answers the negated host motion (MAME
   PORT_REVERSE on both axes, ares negates both), and buttons 1 and 2 are TL
   and TR, active low. */
static uint8_t sports_nibble(const struct cogmaster__port *p)
{
    uint32_t step = p->step;
    if (step == 0u || step > 4u) {
        return 0x0Fu;
    }
    uint8_t byte = (uint8_t)p->val[step > 2u ? 1u : 0u];
    return (uint8_t)((step & 1u) != 0u ? byte >> 4 : byte & 0x0Fu);
}

/* The Paddle sends its position a nibble at a time and says which on TR, a line
   it drives itself at the clock the device carries: TR high is the high nibble
   (MAME paddle.cpp in_r; ares paddle.cpp read). The clock has no state: its
   level is the parity of the tick count in half periods, from tick 0, the
   same on both ports. The button is TL, active low. */
static uint8_t paddle_pins(const struct cogmaster__port *p, uint32_t held, uint64_t tick)
{
    uint32_t phase = (uint32_t)((tick / PADDLE_HALF_TICKS) & 1u);
    uint32_t position = (uint32_t)p->pos ^ 0x80u;
    uint32_t nibble = phase != 0u ? position >> 4 : position & 0x0Fu;
    return (uint8_t)(nibble | (~held & PIN_TL) | (phase << 5));
}

uint8_t cogmaster__ports_pins(const cogmaster *m, uint32_t port, uint64_t tick)
{
    uint32_t device = port == 0u ? m->input.port1 : m->input.port2;
    uint32_t held = port == 0u ? m->input.p1 : m->input.p2;
    /* The Game Gear has the one port, wired to its own keys. */
    if (port != 0u && m->machine == COGMASTER_MACHINE_GG) {
        device = COGMASTER_DEVICE_NONE;
    }
    switch (device) {
    case COGMASTER_DEVICE_PAD:
        return (uint8_t)(~held & PORT_PINS);
    case COGMASTER_DEVICE_PADDLE:
        return paddle_pins(&m->ports[port], held, tick);
    case COGMASTER_DEVICE_SPORTS_PAD:
        return (uint8_t)(sports_nibble(&m->ports[port]) | (~held & (PIN_TL | PIN_TR)));
    case COGMASTER_DEVICE_LIGHT_PHASER:
        /* The Light Phaser's trigger is TL, low while pulled; the other five pins are undriven
           and read high, and the sensor's own line is TH, which belongs to the console and is
           not a port pin here (SMS Power, Light Phaser; the peripherals note,
           SMS-CARTRIDGES-AND-PERIPHERALS section 3.3). Direction bits and every other held bit
           change nothing. */
        return (uint8_t)(0x2Fu | (~held & PIN_TL));
    case COGMASTER_DEVICE_NONE:
    default:
        return PORT_PINS;
    }
}

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* A fall of TH latches both axes and shows the high nibble of X. The pad's byte
   is at most 127 either way, and what it could not send stays in acc for the
   next read (MAME sports.cpp keeps a base per axis and sends the difference;
   ares clamps to a top speed). A second fall that comes before the pad has been
   idle on TH long enough is the Y half of the same read; after that long, or
   from step 0 or 4, the pad starts a new read (MAME sports.cpp timeout). */
static void sports_fall(struct cogmaster__port *p, uint64_t tick)
{
    if (p->step == 2u && tick - p->edge < SPORTS_IDLE_TICKS) {
        p->step = 3u;
        return;
    }
    for (uint32_t i = 0u; i < 2u; i++) {
        int32_t d = clamp32(p->acc[i], -127, 127);
        p->acc[i] = (int16_t)(p->acc[i] - d);
        p->val[i] = (int8_t)-d;
    }
    p->step = 1u;
}

void cogmaster__ports_th(cogmaster *m, uint32_t port, uint32_t wire, uint64_t tick)
{
    uint32_t device = port == 0u ? m->input.port1 : m->input.port2;
    if (device != COGMASTER_DEVICE_SPORTS_PAD) {
        return;
    }
    struct cogmaster__port *p = &m->ports[port];
    if (wire == 0u) {
        sports_fall(p, tick);
        return;
    }
    if (p->step == 1u || p->step == 3u) {
        p->step++;
    }
    p->edge = tick;
}

/* The pad keeps at most eight full bytes of motion it has not been asked for,
   so a pad that is not read does not hoard more (a limit of the model, not of
   the hardware). */
#define SPORTS_ACC_MAX 1023
#define PADDLE_MAX 255

/* A port's motion axes are consumed at frame start, except a Light Phaser's aim, which is a
   position and stays in force until the next call replaces it. The sensor reads the aim through
   the block's snapshot and m->input, never pending_input. */
void cogmaster__ports_frame(cogmaster *m)
{
    for (uint32_t port = 0u; port < 2u; port++) {
        uint32_t device = port == 0u ? m->pending_input.port1 : m->pending_input.port2;
        uint32_t active = port == 0u ? m->input.port1 : m->input.port2;
        int32_t *px = port == 0u ? &m->pending_input.x1 : &m->pending_input.x2;
        int32_t *py = port == 0u ? &m->pending_input.y1 : &m->pending_input.y2;
        int32_t *ax = port == 0u ? &m->input.x1 : &m->input.x2;
        int32_t *ay = port == 0u ? &m->input.y1 : &m->input.y2;
        int32_t axis[2] = {*px, *py};
        struct cogmaster__port *p = &m->ports[port];
        if (device != active) {
            memset(p, 0, sizeof *p);
        }
        if (device == COGMASTER_DEVICE_SPORTS_PAD) {
            for (uint32_t i = 0u; i < 2u; i++) {
                p->acc[i] = (int16_t)clamp32(p->acc[i] + axis[i], -SPORTS_ACC_MAX, SPORTS_ACC_MAX);
            }
        } else if (device == COGMASTER_DEVICE_PADDLE) {
            int32_t at = (int32_t)(p->pos ^ 0x80u) + axis[0];
            p->pos = (uint8_t)((uint32_t)clamp32(at, 0, PADDLE_MAX) ^ 0x80u);
        }
        if (device != COGMASTER_DEVICE_LIGHT_PHASER) {
            *px = *py = *ax = *ay = 0;
        }
    }
}

/* The Sports Pad rides the Master System's ports on any region; the Paddle was made for the
   Japanese console alone (MAME sms_ctrl/paddle.cpp notes: a Japanese release only, and no pin 7
   for the export protocol). The Light Phaser plugs into either port of any Master System region.
   The Game Gear has no controller ports to attach any of them to (it has no TH pins here, and MAME
   models a Game Gear gun only as a to-do). */
int cogmaster__device_ok(const cogmaster *m, uint32_t device)
{
    switch (device) {
    case COGMASTER_DEVICE_PAD:
    case COGMASTER_DEVICE_NONE:
        return 1;
    case COGMASTER_DEVICE_SPORTS_PAD:
    case COGMASTER_DEVICE_LIGHT_PHASER:
        return m->machine != COGMASTER_MACHINE_GG;
    case COGMASTER_DEVICE_PADDLE:
        return m->machine != COGMASTER_MACHINE_GG && m->region == COGMASTER_REGION_JAPAN;
    default:
        return 0;
    }
}
