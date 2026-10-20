/* Replays inputs through one fuzz entry with a plain main, so every
   pull-request lane runs the seeds and committed regressions without
   libFuzzer.

     cogmaster-fuzz-replay-<target> PATH...

   Each PATH is a file, or a directory whose regular files are all replayed.
   Prints "replayed N" and exits 1 when N is 0 or an input cannot be read.
   An input longer than the nightly lane's -max_len is an error, since
   libFuzzer would never have produced it. Each input is copied into a
   buffer of exactly its length, so a sanitizer sees any read past it. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz.h"

/* 1 MiB, a 512-byte copier header and one byte more: the largest image the
   loader must see to reach its size-reject path. */
#define REPLAY_MAX_LEN 1049089u

static unsigned char buf[REPLAY_MAX_LEN + 1u];
static unsigned long replayed;
static int failures;

static void replay_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "replay: cannot open %s\n", path);
        failures++;
        return;
    }
    size_t n = fread(buf, 1u, sizeof buf, f);
    int bad = ferror(f) != 0;
    fclose(f);
    if (bad || n > REPLAY_MAX_LEN) {
        fprintf(stderr, "replay: %s: %s\n", path, bad ? "read error" : "longer than -max_len");
        failures++;
        return;
    }
    uint8_t *copy = malloc(n > 0u ? n : 1u);
    if (copy == NULL) {
        fprintf(stderr, "replay: out of memory for %s\n", path);
        failures++;
        return;
    }
    memcpy(copy, buf, n);
    (void)LLVMFuzzerTestOneInput(copy, n);
    free(copy);
    replayed++;
}

static void join(char *out, size_t cap, const char *dir, const char *name)
{
    int len = snprintf(out, cap, "%s/%s", dir, name);
    if (len < 0 || (size_t)len >= cap) {
        out[0] = '\0';
    }
}

#ifdef _WIN32
static int is_dir(const char *path)
{
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static void replay_dir(const char *dir)
{
    char pattern[4096];
    join(pattern, sizeof pattern, dir, "*");
    WIN32_FIND_DATAA fd;
    memset(&fd, 0, sizeof fd);
    HANDLE h = pattern[0] != '\0' ? FindFirstFileA(pattern, &fd) : INVALID_HANDLE_VALUE;
    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "replay: cannot list %s\n", dir);
        failures++;
        return;
    }
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            char path[4096];
            join(path, sizeof path, dir, fd.cFileName);
            replay_file(path);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
#else
static int is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void replay_dir(const char *dir)
{
    DIR *d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "replay: cannot list %s\n", dir);
        failures++;
        return;
    }
    for (struct dirent *e = readdir(d); e != NULL; e = readdir(d)) {
        char path[4096];
        struct stat st;
        join(path, sizeof path, dir, e->d_name);
        if (path[0] != '\0' && stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            replay_file(path);
        }
    }
    closedir(d);
}
#endif

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: cogmaster-fuzz-replay-<target> PATH...\n");
        return 2;
    }
    for (int i = 1; i < argc; i++) {
        if (is_dir(argv[i])) {
            replay_dir(argv[i]);
        } else {
            replay_file(argv[i]);
        }
    }
    printf("replayed %lu\n", replayed);
    return failures == 0 && replayed > 0u ? 0 : 1;
}
