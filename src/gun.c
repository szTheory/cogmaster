/* The Light Phaser sensor will live here: the gun a port-1 device selects by
   aiming at the picture, seen by the console as TL low while the beam crosses
   the aimed spot and as a rising TH that latches the H counter.

   It is written from SMS Power's Light Phaser page, MacDonald's
   msvdp-20021112.txt section 11 and the protocol in
   SMS-CARTRIDGES-AND-PERIPHERALS section 3.3, and checked against MAME's
   sms_ctrl/lphaser.cpp and sms_m.cpp (BSD-3, pin 1d7a454ed79e) and ares
   controller/light-phaser (ISC, pin 9408cb43d494); nothing is copied. */
#include "internal.h"

void cogmaster__gun_sync(cogmaster *m)
{
    (void)m;
}

void cogmaster__gun_wrap(cogmaster *m)
{
    (void)m;
}
