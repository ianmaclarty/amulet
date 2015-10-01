#include "amulet.h"

#define MAX_MSG_LEN (1024 * 100)

#ifdef AM_BACKEND_EMSCRIPTEN
#define LOG_F stdout
#else
#define LOG_F stderr
#endif

static char* log_cache = NULL;
static int log_cache_size = 0;
static int log_cache_end = 0;

static void init_logcache() {
    if (log_cache == NULL) {
        log_cache_size = (1024 * 100);
        log_cache = (char*)malloc(log_cache_size); // never freed
        memset(log_cache, 0, log_cache_size);
    }
}

// Returns true if the msg was not found and adds it to the cache.
// XXX not thread safe
static bool check_log_cache(char *msg) {
    init_logcache();
    int i = 0;
    int msglen = strlen(msg);
    while (i < log_cache_end) {
        bool found = true;
        int j = 0;
        while (i < log_cache_end && j < msglen) {
            if (log_cache[i] != msg[j]) {
                found = false;
                break;
            }
            i++;
            j++;
        }
        if (found) return true;
        while (log_cache[i] != '\0') i++;
        i++;
    }
    // not found, add to cache if space
    if (log_cache_size - log_cache_end > msglen + 1) {
        memcpy(&log_cache[log_cache_end], msg, msglen);
        log_cache_end += msglen + 1;
        assert(log_cache_end <= log_cache_size);
    }
    return false;
}

void am_reset_log_cache() {
    log_cache_end = 0;
}

void am_log(lua_State *L, int level, bool once, const char *fmt, ...) {
    char *msg = (char*)malloc(MAX_MSG_LEN);
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    if (L != NULL && level > 0) {
        lua_Debug ar;
        int r = lua_getstack(L, level, &ar);
        if (r) {
            lua_getinfo(L, "Sl", &ar);
            char *tmp = (char*)malloc(strlen(msg)+1);
            strcpy(tmp, msg);
            const char *fmt;
            if (am_conf_log_gl_calls) {
                fmt = "// %s:%d: %s";
            } else {
                fmt = "%s:%d: %s";
            }
            snprintf(msg, MAX_MSG_LEN, fmt, ar.short_src, ar.currentline, tmp);
            free((void*)tmp);
        }
    }
    if (!once || !check_log_cache(msg)) {
        fprintf(LOG_F, "%s\n", msg);
        fflush(LOG_F);
    }
    free(msg);
}

void am_abort(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    fprintf(LOG_F, "%s\n", msg);
    fprintf(LOG_F, "*** ABORTING ***\n");
    fflush(LOG_F);
    assert(false); // so we can catch the abort in gdb if debugging
    exit(EXIT_FAILURE);
}

static int log(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    const char *str = luaL_checkstring(L, 1);
    bool once = false;
    int level = 1;
    if (nargs > 1) {
        once = lua_toboolean(L, 2);
    }
    if (nargs > 2) {
        level = lua_tointeger(L, 3);
    }
    am_log(L, level, once, "%s", str);
    return 0;
}

void am_open_logging_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"log", log},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
