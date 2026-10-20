/* Bounded, headerless absolute input records for the runner. */
#ifndef COGMASTER_RUNNER_INPUT_H
#define COGMASTER_RUNNER_INPUT_H
#include <stdio.h>
#include "cogmaster.h"
/* Reads exactly one record. 0 success, 2 malformed/EOF, 1 file error. */
int cogmaster__input_read(FILE *f, cogmaster_input *out);
/* Checks the complete body then rewinds it; keeps memory independent of frames. */
int cogmaster__input_validate(FILE *f, uint32_t frames);
#endif
