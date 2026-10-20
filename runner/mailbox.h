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

/* The magic and the version and status bytes after it. */
#define COGMASTER__MAILBOX_HEAD 6u

/* After a frame, given the mailbox's first bytes: 1 when the status reads
   $00 (done) with the magic in place, and a frame since the magic last
   appeared ended with the status reading $80 (running). Power-on clears
   work RAM and the ROM writes the magic before $80, so a $00 that no
   running status preceded is not yet done. *running carries that between
   frames; start it at 0. */
int cogmaster__mailbox_stop(int *running, const uint8_t head[COGMASTER__MAILBOX_HEAD]);

/* cogmaster__mailbox_stop over the mailbox m holds. */
int cogmaster__mailbox_done(const cogmaster *m, int *running);

/* Prints status=, rom=, magic=, crc= and one "NN=VVVV flags=F" line per
   record, all in hex. */
void cogmaster__mailbox_print(const cogmaster *m);

#endif /* COGMASTER_RUNNER_MAILBOX_H */
