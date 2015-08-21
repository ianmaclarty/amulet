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
