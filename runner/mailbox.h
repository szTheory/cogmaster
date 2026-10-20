/* --mailbox: the RAM mailbox a diagnostic ROM fills, read with
   cogmaster_peek and printed, never judged (docs/runner.md). */
#ifndef COGMASTER_RUNNER_MAILBOX_H
#define COGMASTER_RUNNER_MAILBOX_H

#include <stddef.h>
#include <stdint.h>

#include "cogmaster.h"

/* CRC-16 CCITT: polynomial $1021, initial value $FFFF, no reflection and no
   final XOR. "123456789" gives $29B1. */
uint16_t cogmaster__crc16_ccitt(const uint8_t *p, size_t n);

/* 1 when the mailbox status byte reads $00: the ROM has finished. */
int cogmaster__mailbox_done(const cogmaster *m);

/* Prints status=, rom=, magic=, crc= and one "NN=VVVV flags=F" line per
   record, all in hex. */
void cogmaster__mailbox_print(const cogmaster *m);

#endif /* COGMASTER_RUNNER_MAILBOX_H */
