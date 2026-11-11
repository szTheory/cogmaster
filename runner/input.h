/* Bounded, headerless absolute input records for the runner. */
#ifndef COGMASTER_RUNNER_INPUT_H
#define COGMASTER_RUNNER_INPUT_H
#include <stdio.h>
#include "cogmaster.h"
/* Reads exactly one record of 35 bytes (axes 0) or 75 bytes (with x1 y1 x2 y2). 0 success,
   2 malformed/EOF, 1 file error. */
int cogmaster__input_read(FILE *f, cogmaster_input *out);
/* Checks the complete body then rewinds it; keeps memory independent of frames. port1 and port2
   are the resolved devices: motion for a port whose device takes none returns 2 with *bad_port set
   to 1 or 2; any other refusal leaves *bad_port 0. */
int cogmaster__input_validate(FILE *f, uint32_t frames, uint32_t port1, uint32_t port2,
                              int *bad_port);
#endif
