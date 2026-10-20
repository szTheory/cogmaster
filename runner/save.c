#include "save.h"
#include "exit_status.h"
#include "path.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <process.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef COGMASTER_SAVE_TEST
static int save_fault(const cogmaster__save *save, int point)
{
#ifdef COGMASTER_SAVE_FAIL
    if (COGMASTER_SAVE_FAIL == point)
        return 1;
#endif
    return save->fault == point;
}
#define SAVE_FAULT(s, point) save_fault(s, point)
#else
#define SAVE_FAULT(s, point) 0
#endif

static int save_error(const char *why)
{
    fprintf(stderr, "cogmaster-run: save: %s\n", why);
    return COGMASTER__EXIT_FAILURE;
}

static int save_directory(const char *dir)
{
#ifdef _WIN32
    wchar_t *path = cogmaster__path_wide(dir);
    DWORD attr = path != NULL ? GetFileAttributesW(path) : INVALID_FILE_ATTRIBUTES;
    free(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static char *save_path(const char *dir, const char *rom)
{
    const char *base = rom;
    for (const char *p = rom; *p; p++) {
        if (*p == '/'
#ifdef _WIN32
            || *p == '\\'
#endif
        )
            base = p + 1;
    }
    const char *dot = strrchr(base, '.');
    size_t stem = dot != NULL && dot != base ? (size_t)(dot - base) : strlen(base);
    size_t n = strlen(dir);
    if (stem == 0 || n > SIZE_MAX - stem - 6u)
        return NULL;
    char *path = (char *)malloc(n + stem + 6u);
    if (path != NULL) {
        memcpy(path, dir, n);
        path[n++] = '/';
        memcpy(path + n, base, stem);
        memcpy(path + n + stem, ".srm", 5u);
    }
    return path;
}

int cogmaster__save_load(cogmaster__save *save, cogmaster *m, const char *dir, const char *rom)
{
    if (!save_directory(dir))
        return save_error("--save-dir must name an existing directory");
    cogmaster_battery b = {0};
    b.size = sizeof b;
    if (cogmaster_get_battery(m, &b) != COGMASTER_STATUS_OK)
        return save_error("cannot query battery");
    if (b.knowledge == COGMASTER_BATTERY_UNKNOWN || b.knowledge == COGMASTER_BATTERY_UNSUPPORTED) {
        fprintf(stderr, "cogmaster-run: save: %s\n",
                b.knowledge == COGMASTER_BATTERY_UNKNOWN ? "unknown persistence"
                                                         : "unsupported persistence geometry");
        return COGMASTER__EXIT_REFUSED;
    }
    if (b.knowledge == COGMASTER_BATTERY_NONE)
        return COGMASTER__EXIT_OK;
    save->path = save_path(dir, rom);
    if (save->path == NULL)
        return save_error("cannot construct path");
    uint8_t *data = (uint8_t *)malloc(b.length + 1u);
    if (data == NULL)
        return save_error("out of memory");
    errno = 0;
    FILE *f = cogmaster__path_open(save->path, "rb");
    int status = COGMASTER__EXIT_OK;
    size_t n = 0;
    if (f == NULL) {
        if (errno != ENOENT)
            status = save_error("cannot read file");
    } else {
        n = fread(data, 1u, b.length + 1u, f);
        int bad = ferror(f);
        if (fclose(f) != 0)
            bad = 1;
        if (bad)
            status = save_error("cannot read or close file");
    }
    if (status == COGMASTER__EXIT_OK) {
        cogmaster_status st = cogmaster_import_battery(m, data, n);
        if (st == COGMASTER_STATUS_ERR_SAVE_SIZE) {
            fprintf(stderr, "cogmaster-run: save: oversized file\n");
            status = COGMASTER__EXIT_SAVE_SIZE;
        } else if (st != COGMASTER_STATUS_OK) {
            status = save_error("cannot import battery");
        }
    }
    free(data);
    if (status == COGMASTER__EXIT_OK)
        save->acknowledged = b.generation;
    return status;
}

/* Exclusive creation never truncates a pre-existing name. The temporary is
   next to the target, so replacement cannot cross filesystem boundaries. */
static FILE *save_temporary(const char *path, char **temporary)
{
    size_t n = strlen(path);
    char *tmp = (char *)malloc(n + 48u);
    if (tmp == NULL)
        return NULL;
#ifdef _WIN32
    for (uint32_t i = 0; i < 1024u; i++) {
        (void)snprintf(tmp, n + 48u, "%s.tmp-%lu-%lu", path, (unsigned long)_getpid(),
                       (unsigned long)i);
        wchar_t *wide = cogmaster__path_wide(tmp);
        HANDLE h = wide != NULL ? CreateFileW(wide, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                                              FILE_ATTRIBUTE_NORMAL, NULL)
                                : INVALID_HANDLE_VALUE;
        DWORD error = GetLastError();
        free(wide);
        if (h == INVALID_HANDLE_VALUE) {
            if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
                continue;
            break;
        }
        int fd = _open_osfhandle((intptr_t)h, _O_BINARY | _O_WRONLY);
        FILE *f = fd >= 0 ? _fdopen(fd, "wb") : NULL;
        if (f != NULL) {
            *temporary = tmp;
            return f;
        }
        if (fd >= 0)
            (void)_close(fd);
        else
            (void)CloseHandle(h);
        wide = cogmaster__path_wide(tmp);
        if (wide != NULL)
            (void)DeleteFileW(wide);
        free(wide);
        break;
    }
#else
    /* POSIX open(O_CREAT|O_EXCL) also works in WASI, whose libc omits
       mkstemp. Mode 0600 retains private native save-file permissions. */
    for (uint32_t i = 0u; i < 1024u; i++) {
        (void)snprintf(tmp, n + 48u, "%s.tmp-%lu", path, (unsigned long)i);
        int fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd < 0) {
            if (errno == EEXIST)
                continue;
            break;
        }
        FILE *f = fdopen(fd, "wb");
        if (f != NULL) {
            *temporary = tmp;
            return f;
        }
        (void)close(fd);
        (void)unlink(tmp);
        break;
    }
#endif
    free(tmp);
    return NULL;
}

static void save_remove(const char *path)
{
#ifdef _WIN32
    wchar_t *wide = cogmaster__path_wide(path);
    if (wide != NULL)
        (void)DeleteFileW(wide);
    free(wide);
#else
    (void)unlink(path);
#endif
}

static int save_replace(const char *tmp, const char *path)
{
#ifdef _WIN32
    wchar_t *from = cogmaster__path_wide(tmp), *to = cogmaster__path_wide(path);
    /* MoveFileExW handles both first create and overwrite. Do not use
       ReplaceFileW: its documented merge errors can remove the old name.
       Microsoft Learn, MoveFileExW and ReplaceFileW return values. */
    int ok = from != NULL && to != NULL &&
             MoveFileExW(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    free(from);
    free(to);
    return ok;
#else
    /* POSIX rename: existing target remains visible as old or complete new
       bytes. POSIX.1 rename(), atomic replacement requirement. */
    return rename(tmp, path) == 0;
#endif
}

int cogmaster__save_flush(cogmaster__save *save, cogmaster *m)
{
    if (save->path == NULL)
        return COGMASTER__EXIT_OK;
    cogmaster_battery b = {0};
    b.size = sizeof b;
    if (cogmaster_get_battery(m, &b) != COGMASTER_STATUS_OK)
        return save_error("cannot query battery");
    if (b.generation == save->acknowledged)
        return COGMASTER__EXIT_OK;
    char *tmp = NULL;
    if (SAVE_FAULT(save, COGMASTER_SAVE_CREATE))
        return save_error("cannot create temporary");
    FILE *f = save_temporary(save->path, &tmp);
    if (f == NULL)
        return save_error("cannot create temporary");
    size_t n = b.length - (SAVE_FAULT(save, COGMASTER_SAVE_WRITE) ? 1u : 0u);
    int ok = fwrite(b.data, 1u, n, f) == b.length;
    if (fflush(f) != 0)
        ok = 0;
    if (SAVE_FAULT(save, COGMASTER_SAVE_FLUSH))
        ok = 0;
#ifdef _WIN32
    if (_commit(_fileno(f)) != 0)
        ok = 0;
#elif !defined(__wasi__)
    if (fsync(fileno(f)) != 0)
        ok = 0;
#endif
    if (SAVE_FAULT(save, COGMASTER_SAVE_SYNC))
        ok = 0;
    if (fclose(f) != 0)
        ok = 0;
    if (SAVE_FAULT(save, COGMASTER_SAVE_CLOSE))
        ok = 0;
#ifdef COGMASTER_SAVE_TEST
    if (ok && SAVE_FAULT(save, COGMASTER_SAVE_INTERRUPT))
        _Exit(93);
#endif
    if (SAVE_FAULT(save, COGMASTER_SAVE_REPLACE))
        ok = 0;
    if (ok)
        ok = save_replace(tmp, save->path);
    if (!ok)
        save_remove(tmp);
    free(tmp);
    if (!ok)
        return save_error("cannot write, synchronize, close or replace file");
    save->acknowledged = b.generation;
    return COGMASTER__EXIT_OK;
}

void cogmaster__save_dispose(cogmaster__save *save)
{
    free(save->path);
    save->path = NULL;
}
