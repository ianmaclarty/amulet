enum am_simple_lua_value_type {
    AM_SIMPLE_LUA_STRING,
    AM_SIMPLE_LUA_NUMBER,
    AM_SIMPLE_LUA_BOOLEAN,
    AM_SIMPLE_LUA_NIL,
};

struct am_simple_lua_value {
    am_simple_lua_value_type type;
    union {
        const char *string_value;
        double number_value;
        bool boolean_value;
    } value;
};

typedef am_simple_lua_value (*am_simple_lua_func)(int nargs, am_simple_lua_value *args);

void am_simple_lua_register_function(const char *name, am_simple_lua_func func);

void am_pause_app();
void am_resume_app();
