#include "amulet.h"

#define MAX_MSG_LEN 102400

#ifdef AM_BACKEND_EMSCRIPTEN
#define LOG_F stdout
#else
#define LOG_F stderr
#endif

void am_report_message(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(LOG_F, "%s\n", msg);
    fflush(LOG_F);
}

void am_report_error(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(LOG_F, "%s\n", msg);
    fflush(LOG_F);
}

void am_debug(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(LOG_F, "%s\n", msg);
    fflush(LOG_F);
}

void am_abort(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(LOG_F, "%s\n", msg);
    fflush(LOG_F);
    assert(false); // so we can catch the abort in gdb if debugging
    exit(EXIT_FAILURE);
}
