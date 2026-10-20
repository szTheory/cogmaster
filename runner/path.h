/* UTF-8 host paths. Windows argv is converted at the entry point. */
#ifndef COGMASTER_RUNNER_PATH_H
#define COGMASTER_RUNNER_PATH_H
#include <stdio.h>
#ifdef _WIN32
#include <stdlib.h>
#include <windows.h>
static wchar_t *cogmaster__path_wide(const char *path)
{
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    wchar_t *out = n > 0 ? (wchar_t *)malloc((size_t)n * sizeof *out) : NULL;
    if (out != NULL && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, out, n)) {
        free(out);
        out = NULL;
    }
    return out;
}
static FILE *cogmaster__path_open(const char *path, const char *mode)
{
    wchar_t *wide = cogmaster__path_wide(path);
    FILE *f = wide != NULL ? _wfopen(wide, mode[0] == 'r' ? L"rb" : L"wb") : NULL;
    free(wide);
    return f;
}
static inline int cogmaster__path_remove(const char *path)
{
    wchar_t *wide = cogmaster__path_wide(path);
    int result = wide != NULL ? _wremove(wide) : -1;
    free(wide);
    return result;
}
#else
#define cogmaster__path_open fopen
#define cogmaster__path_remove remove
#endif
#endif
