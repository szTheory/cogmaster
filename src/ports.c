/* The two controller ports at pin level (SMS-CARTRIDGES-AND-PERIPHERALS
   section 3, "Port bit map"). A port's six device-driven pins are b0 Up (D0)
   to b3 Right (D3), b4 TL and b5 TR, 0 = driven low; TH belongs to the
   console, which mixes it, the reset button and any TR it drives itself in
   io.c. The function is pure: it reads the instance and changes nothing, so
   a read of $DC or $DD cannot move a device. */
#include "internal.h"

#define PORT_PINS 0x3Fu

uint8_t cogmaster__ports_pins(const cogmaster *m, uint32_t port, uint64_t tick)
{
    (void)tick;
    uint32_t device = port == 0u ? m->input.port1 : m->input.port2;
    uint32_t held = port == 0u ? m->input.p1 : m->input.p2;
    /* The Game Gear has the one port, wired to its own keys. */
    if (port != 0u && m->machine == COGMASTER_MACHINE_GG) {
        device = COGMASTER_DEVICE_NONE;
    }
    switch (device) {
    case COGMASTER_DEVICE_PAD:
        return (uint8_t)(~held & PORT_PINS);
    case COGMASTER_DEVICE_NONE:
    default:
        return PORT_PINS;
    }
}
