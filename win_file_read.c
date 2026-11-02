/*
 * main.c - read a whole file into memory and report its size and line count.
 *
 * Build:
 *   Linux/macOS:  cc -std=c99 -Wall -Wextra -O2 main.c -o readfile
 *   MSVC:         cl /W4 /O2 main.c
 *   MinGW:        gcc -std=c99 -Wall -Wextra -O2 main.c -o readfile.exe
 *
 * Usage:
 *   readfile <file> [file ...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>   /* CommandLineToArgvW */
#include <stdint.h>
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif
#endif

/* Prints "what: <strerror(errno)>" and exits. */
static void die_w_perror(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

#ifdef _WIN32
/* Like die_w_perror(), but reports GetLastError() instead of errno. */
static void die_w_lasterror(const char *what)
{
    DWORD err = GetLastError();
    char msg[512];
    DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, err, 0, msg, sizeof msg, NULL);
    while (n > 0 && (msg[n - 1] == '\r' || msg[n - 1] == '\n'))
        msg[--n] = '\0';
    if (n == 0)
        snprintf(msg, sizeof msg, "error %lu", (unsigned long)err);
    fprintf(stderr, "%s: %s\n", what, msg);
    exit(EXIT_FAILURE);
}

/* Converts a UTF-8 string to a heap-allocated UTF-16 string. Exits on failure. */
static wchar_t *utf8_to_wide(const char *s)
{
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    if (n == 0)
        die_w_lasterror(s);
    wchar_t *w = malloc((size_t)n * sizeof *w);
    if (!w)
        die_w_perror("malloc");
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, w, n))
        die_w_lasterror(s);
    return w;
}

/* Converts a UTF-16 string to a heap-allocated UTF-8 string. Exits on failure. */
static char *wide_to_utf8(const wchar_t *w)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (n == 0)
        die_w_lasterror("WideCharToMultiByte");
    char *s = malloc((size_t)n);
    if (!s)
        die_w_perror("malloc");
    if (!WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL))
        die_w_lasterror("WideCharToMultiByte");
    return s;
}

/* Builds a UTF-8 argv from the real (UTF-16) command line, so paths with
 * non-ASCII characters survive regardless of the ANSI code page. */
static char **get_utf8_argv(int *argc_out)
{
    int argc;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!wargv)
        die_w_lasterror("CommandLineToArgvW");

    char **argv = malloc(((size_t)argc + 1) * sizeof *argv);
    if (!argv)
        die_w_perror("malloc");
    for (int i = 0; i < argc; i++)
        argv[i] = wide_to_utf8(wargv[i]);
    argv[argc] = NULL;

    LocalFree(wargv);
    *argc_out = argc;
    return argv;
}

static void free_utf8_argv(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
#endif /* _WIN32 */

/* Reads a whole file into a heap buffer sized to it, NUL-terminated so text
 * files can be used as strings. Stores the byte count in *len if len isn't
 * NULL. Exits on failure. */
static char *read_file_sized(const char *path, size_t *len)
{
#ifdef _WIN32
    // WINDOWS IMPLEMENTATION
    wchar_t *wpath = utf8_to_wide(path);
    HANDLE h = CreateFileW(wpath, GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
    if (h == INVALID_HANDLE_VALUE)
        die_w_lasterror(path);
    free(wpath);

    LARGE_INTEGER size;
    if (!GetFileSizeEx(h, &size))
        die_w_lasterror(path);
    if ((unsigned long long)size.QuadPart >= (unsigned long long)SIZE_MAX) {
        fprintf(stderr, "%s: file too large\n", path);
        exit(EXIT_FAILURE);
    }
    size_t total = (size_t)size.QuadPart;

    char *buf = malloc(total + 1);
    if (!buf)
        die_w_perror("malloc");

    /* ReadFile takes a DWORD count, so read in chunks of at most 1 GiB. */
    size_t n = 0;
    while (n < total) {
        size_t left = total - n;
        DWORD chunk = left > 0x40000000u ? 0x40000000u : (DWORD)left;
        DWORD got;
        if (!ReadFile(h, buf + n, chunk, &got, NULL))
            die_w_lasterror(path);
        if (got == 0)
            break; /* EOF early: file shrank since we sized it */
        n += got;
    }
    buf[n] = '\0';
    CloseHandle(h);

    if (len)
        *len = n;
    return buf;

#else
    // POSIX IMPLEMENTATION
    FILE *f = fopen(path, "rb");
    if (!f)
        die_w_perror(path);

    if (fseek(f, 0, SEEK_END) != 0)
        die_w_perror(path);
    long size = ftell(f);
    if (size < 0)
        die_w_perror(path);
    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (!buf)
        die_w_perror("malloc");

    size_t n = fread(buf, 1, (size_t)size, f);
    if (ferror(f))
        die_w_perror(path);
    buf[n] = '\0';
    fclose(f);

    if (len)
        *len = n;
    return buf;
#endif /* _WIN32 */
}

/* Counts lines the way `wc -l` does, plus one for a final unterminated line.
 * Uses the length rather than strlen so embedded NULs don't cut it short. */
static size_t count_lines(const char *buf, size_t len)
{
    size_t lines = 0;
    const char *p = buf, *end = buf + len;
    const char *nl;
    while (p < end && (nl = memchr(p, '\n', (size_t)(end - p))) != NULL) {
        lines++;
        p = nl + 1;
    }
    if (p < end)
        lines++;
    return lines;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Replace the ANSI-code-page argv with a UTF-8 one. */
    (void)argv;
    argv = get_utf8_argv(&argc);
    SetConsoleOutputCP(CP_UTF8); /* so UTF-8 file names print correctly */
#endif

    if (argc < 2) {
        fprintf(stderr, "usage: %s <file> [file ...]\n",
                argc > 0 ? argv[0] : "readfile");
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        size_t len;
        char *data = read_file_sized(argv[i], &len);

        printf("%s: %zu bytes, %zu lines\n",
               argv[i], len, count_lines(data, len));

        free(data);
    }

#ifdef _WIN32
    free_utf8_argv(argc, argv);
#endif
    return EXIT_SUCCESS;
}
