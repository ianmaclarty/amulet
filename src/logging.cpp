#include "amulet.h"

#define MAX_MSG_LEN 102400

void am_report_message(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}

void am_report_error(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}

void am_abort(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
    exit(EXIT_FAILURE);
}
