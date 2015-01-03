#define am_debug(fmt, ...) am_log(NULL, 0, false, "DEBUG:%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)
#define am_log0(fmt, ...) am_log(NULL, 0, false, fmt, __VA_ARGS__)
#define am_log1(fmt, ...) am_log(NULL, 0, true, fmt, __VA_ARGS__)

void am_abort(const char *fmt, ...);
void am_reset_log_cache();
void am_log(lua_State *L, int level, bool once, const char *fmt, ...);

void am_open_logging_module(lua_State *L);
