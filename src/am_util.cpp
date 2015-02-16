#include "amulet.h"

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
