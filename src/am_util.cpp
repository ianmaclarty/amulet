#include "amulet.h"

#include <sys/stat.h>

#define MAX_MSG_LEN 10240

char *am_format(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    int n = strlen(msg);
    char *str = (char*)malloc(n+1);
    strncpy(str, msg, n+1);
    return str;
}

bool am_file_exists(const char *filename) {
    struct stat info;
    return stat(filename, &info) == 0;
}

void am_replchr(char *str, char c0, char c) {
    while (*str) {
        if (*str == c0) *str = c;
        str++;
    }
}

void am_delete_file(const char *file) {
#ifdef AM_WINDOWS
    _unlink(file);
#else
    unlink(file);
#endif
}

void am_make_dir(const char* dir) {
#if defined(AM_WINDOWS)
    _mkdir(dir);
#else
    mkdir(dir, S_IRWXU | S_IRWXG);
#endif
}

void am_delete_empty_dir(const char* dir) {
#if defined(AM_WINDOWS)
    _rmdir(dir);
#else
    rmdir(dir);
#endif
}

void *am_read_file(const char *filename, size_t *len) {
    *len = 0;
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to open file %s\n", filename);
        return NULL;
    }
    size_t l = 0;
    int c;
    do {
        c = fgetc(f);
        if (c == EOF) break;
        else l++;
    } while (1);
    *len = l;
    unsigned char *buf = (unsigned char*)malloc(l);
    fseek(f, 0, SEEK_SET);
    for (size_t i = 0; i < l; i++) {
        buf[i] = fgetc(f);
    }
    return buf;
}
