/* Exercise production PNG cleanup after a failed close, including a CRT
   code page that interprets narrow UTF-8 paths as a different filename. */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

static int png_close_failure(FILE *f)
{
    (void)fclose(f);
#ifdef _WIN32
    if (setlocale(LC_ALL, ".1252") == NULL) {
        abort();
    }
#endif
    return EOF;
}

#define fclose png_close_failure
#include "../../runner/main.c"
