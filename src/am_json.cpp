#include "amulet.h"

/*------------------------ parsing ----------------------------*/

struct parse_state {
    const char *start;
    const char *ptr;
};

static void eat_whitespace(parse_state *state);
static int parse_value(lua_State *L, parse_state *state);
static int parse_string(lua_State *L, parse_state *state);
static int parse_object(lua_State *L, parse_state *state);
static int parse_array(lua_State *L, parse_state *state);
static int parse_boolean(lua_State *L, parse_state *state);
static int parse_null(lua_State *L, parse_state *state);
static int parse_number(lua_State *L, parse_state *state);
static int parse_word(lua_State *L, parse_state *state, const char *word, const char *err_msg);
static int push_parse_error(lua_State *L, parse_state *state, const char *msg);

/*
 * Expects a string as its only argument.
 * Returns either the parsed value or nil and an error message.
 */
static int parse_json(lua_State *L) {
    am_check_nargs(L, 1);
    const char *input = lua_tostring(L, 1);
    if (input == NULL) {
        return luaL_error(L, "Argument 1 must be a string");
    }
    int top = lua_gettop(L);
    parse_state state;
    state.start = input;
    state.ptr = input;

    int ok = parse_value(L, &state);
    if (ok) {
        eat_whitespace(&state);
        if (*state.ptr == 0) {
            return 1; // parsed value will be at top + 1.
        } else {
            lua_pop(L, 1); // pop value
            lua_pushnil(L);
            push_parse_error(L, &state, "unexpected trailing characters");
            return 2;
        }
    } else {
        // Error message will be on top of stack.
        // Insert nil before err msg.
        lua_pushnil(L);
        lua_insert(L, top + 1);
        return 2;
    }
}

static int is_whitespace(const char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int is_punct(const char c) {
    return c == ':' || c == '{' || c == '}' || c == '[' || c == ']' || c == ',';
}

static void eat_whitespace(parse_state *state) {
    while (is_whitespace(*state->ptr)) state->ptr++;
}

static int parse_value(lua_State *L, parse_state *state) {
    eat_whitespace(state);
    char c = *state->ptr;
    switch (c) {
        case '"': return parse_string(L, state);
        case '{': return parse_object(L, state);
        case '[': return parse_array(L, state);
        case 't':
        case 'f': return parse_boolean(L, state);
        case 'n': return parse_null(L, state);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': return parse_number(L, state);
        default:  return push_parse_error(L, state, "unexpected character");
    }
}

static int parse_string(lua_State *L, parse_state *state) {
    const char *ptr = state->ptr;
    assert(*ptr == '"');
    ptr++;
    if (*ptr == '"') {
        lua_pushstring(L, "");
        state->ptr = ptr + 1;
        return 1;
    }
    const char *start = ptr;
    while (*ptr != 0 && !(*ptr != '\\' && *(ptr + 1) == '"')) ptr++;
    if (*ptr == 0) return push_parse_error(L, state, "unterminated string");
    int len = ptr - start + 1;
    char *tmp = (char*)malloc(len);
    char *tptr = tmp;
    ptr = start;
    while (*ptr != '"') {
        if (*ptr == '\\') {
            ptr++;
            switch (*ptr) {
                case 'n': *tptr = '\n'; break;
                case 'r': *tptr = '\r'; break;
                case 't': *tptr = '\t'; break;
                case 'b': *tptr = '\b'; break;
                case 'f': *tptr = '\f'; break;
                case '\\': *tptr = '\\'; break;
                case '/': *tptr = '/'; break;
                case '"': *tptr = '"'; break;
                case '\'': *tptr = '\''; break;
                default:
                    // XXX handle unicode escape sequences
                    free(tmp);
                    state->ptr = ptr;
                    return push_parse_error(L, state, "unrecognised escape sequence");
            }
        } else {
            *tptr = *ptr;
        }
        tptr++;
        ptr++;
    }
    state->ptr = ptr + 1;
    lua_pushlstring(L, tmp, (tptr - tmp));
    free(tmp);
    return 1;
}

static int parse_boolean(lua_State *L, parse_state *state) {
    const char *ptr = state->ptr;
    switch (*ptr) {
        case 't':
            if (parse_word(L, state, "true", "expected 'true'")) {
                lua_pushboolean(L, 1);
                return 1;
            } else {
                return 0;
            }
        case 'f': 
            if (parse_word(L, state, "false", "expected 'false'")) {
                lua_pushboolean(L, 0);
                return 1;
            } else {
                return 0;
            }
        default:
            assert(0);
            return 0;
    }
}

static int parse_null(lua_State *L, parse_state *state) {
    if (parse_word(L, state, "null", "expected 'null'")) {
        lua_pushnil(L);
        return 1;
    } else {
        return 0; // parse_word() pushed the error msg
    }
}

static int parse_number(lua_State *L, parse_state *state) {
    char *end;
    double n = strtod(state->ptr, &end);
    if (state->ptr == end || (*end != 0 && !is_whitespace(*end) && !is_punct(*end))) {
        return push_parse_error(L, state, "invalid number");
    } else {
        state->ptr = end;
        if ((double)((int)n) == n) {
            // in case we're using different representation for
            // integers in the lua vm
            lua_pushinteger(L, (int)n);
        } else {
            lua_pushnumber(L, n);
        }
        return 1;
    }
}

static int parse_object(lua_State *L, parse_state *state) {
    assert(*state->ptr == '{');
    state->ptr++;
    eat_whitespace(state);
    lua_newtable(L);
    if (*state->ptr == '}') {
        // empty object
        state->ptr++;
        return 1;
    }
    while (1) {
        // key
        if (!parse_string(L, state)) {
            lua_remove(L, -2); // remove table
            return 0;
        }
        eat_whitespace(state);
        if (*state->ptr != ':') {
            lua_pop(L, 2); // pop table and key
            return push_parse_error(L, state, "colon expected");
        }
        state->ptr++;
        // value
        eat_whitespace(state);
        if (!parse_value(L, state)) {
            lua_remove(L, -2); // remove key
            lua_remove(L, -2); // remove table
            return 0;
        }
        lua_rawset(L, -3);
        eat_whitespace(state);
        if (*state->ptr == ',') {
            state->ptr++;
            eat_whitespace(state);
        } else {
            break;
        }
    }
    if (*state->ptr == 0) {
        lua_pop(L, 1); // pop table.
        return push_parse_error(L, state, "unterminated object");
    }
    if (*state->ptr != '}') {
        lua_pop(L, 1); // pop table
        return push_parse_error(L, state, "unexpected character");
    }
    state->ptr++;
    return 1;
}

static int parse_array(lua_State *L, parse_state *state) {
    assert(*state->ptr == '[');
    state->ptr++;
    eat_whitespace(state);
    lua_newtable(L);
    if (*state->ptr == ']') {
        // empty array
        state->ptr++;
        return 1;
    }
    int i = 1;
    while (1) {
        if (!parse_value(L, state)) {
            lua_remove(L, -2); // remove table
            return 0;
        }
        lua_rawseti(L, -2, i);
        i++;
        eat_whitespace(state);
        if (*state->ptr == ',') {
            state->ptr++;
            eat_whitespace(state);
        } else {
            break;
        }
    }
    if (*state->ptr == 0) {
        lua_pop(L, 1); // pop table.
        return push_parse_error(L, state, "unterminated array");
    }
    if (*state->ptr != ']') {
        lua_pop(L, 1); // pop table
        return push_parse_error(L, state, "unexpected character");
    }
    state->ptr++;
    return 1;
}

static int parse_word(lua_State *L, parse_state *state, const char *word, const char *err_msg) {
    const char *ptr = state->ptr;
    while (*ptr == *word && *ptr != 0 && *word != 0) {
        ptr++;
        word++;
    }
    if (*word == 0 && (is_whitespace(*ptr) || is_punct(*ptr) || *ptr == 0)) {
        state->ptr = ptr;
        return 1;
    } else {
        return push_parse_error(L, state, err_msg);
    }
}

static int push_parse_error(lua_State *L, parse_state *state, const char *msg) {
    int line = 1;
    int column = 1;
    const char *ptr = state->start;
    while (ptr < state->ptr) {
        if (*ptr == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        ptr++;
    }
    lua_pushfstring(L, "%d:%d: %s", line, column, msg);
    return 0;
}

/*------------------------ serialization ----------------------------*/

struct write_state {
    char *buf;
    char *ptr;
    int capacity;
};

static void append_value(lua_State *L, write_state *state);
static void appends(const char *str, write_state *state);
static void appendc(const char c, write_state *state);

#define INIT_CAPACITY 256

static int to_json(lua_State *L) {
    am_check_nargs(L, 1);
    lua_pushvalue(L, 1);
    write_state state;
    state.buf = (char*)malloc(INIT_CAPACITY);
    state.ptr = state.buf;
    state.capacity = INIT_CAPACITY;
    append_value(L, &state);
    lua_pushlstring(L, state.buf, (state.ptr - state.buf));
    free(state.buf);
    return 1;
}

static void appends(const char *str, write_state *state) {
    int len = strlen(str);
    int cap = state->capacity;
    int pos = state->ptr - state->buf;
    int available = cap - pos;
    while (available < len) {
        cap *= 2;
        available = cap - pos;
    }
    if (cap > state->capacity) {
        state->buf = (char*)realloc(state->buf, cap);
        state->ptr = &state->buf[pos];
        state->capacity = cap;
    }
    memcpy(state->ptr, str, len);
    state->ptr += len;
}

static void appendc(const char c, write_state *state) {
    int cap = state->capacity;
    int pos = state->ptr - state->buf;
    int available = cap - pos;
    while (available < 2) {
        cap *= 2;
        available = cap - pos;
    }
    if (cap > state->capacity) {
        state->buf = (char*)realloc(state->buf, cap);
        state->ptr = &state->buf[pos];
        state->capacity = cap;
    }
    *state->ptr = c;
    state->ptr++;
}

#define NUM_BUF_SIZE 32

static void append_value(lua_State *L, write_state *state) {
    char numbuf[NUM_BUF_SIZE];
    int ltype = lua_type(L, -1);
    switch (ltype) {
        case LUA_TNIL: {
            appends("null", state);
            break;
        }
        case LUA_TNUMBER: {
            double n = lua_tonumber(L, -1);
            snprintf(numbuf, NUM_BUF_SIZE, "%.14g", n);
            appends(numbuf, state);
            break;
        }
        case LUA_TBOOLEAN: {
            int b = lua_toboolean(L, -1);
            if (b) appends("true", state);
            else   appends("false", state);
            break;
        }
        case LUA_TSTRING: {
            const char *s = lua_tostring(L, -1);
            appendc('"', state);
            while (*s != 0) {
                switch (*s) {
                    case '\n': appends("\\n", state); break;
                    case '\r': appends("\\r", state); break;
                    case '\t': appends("\\t", state); break;
                    case '\b': appends("\\b", state); break;
                    case '\f': appends("\\f", state); break;
                    case '\\': appends("\\\\", state); break;
                    case '\"': appends("\\\"", state); break;
                    default: appendc(*s, state); break;
                }
                s++;
            }
            appendc('"', state);
            break;
        }
        case LUA_TTABLE: {
            int is_array = 1;
            lua_pushnil(L);
            if (!lua_next(L, -2)) {
                // empty table.  write out as empty array, since
                // that seems like it would be the more common case.
                appends("[]", state);
                break;
            } else {
                if (lua_type(L, -2) == LUA_TSTRING) is_array = 0;
                lua_pop(L, 2); // pop key and value
            }
            if (is_array) {
                appendc('[', state);
                int i = 1;
                lua_rawgeti(L, -1, i++);
                while (1) {
                    append_value(L, state);
                    lua_rawgeti(L, -1, i++);
                    if (lua_isnil(L, -1)) {
                        lua_pop(L, 1); // pop nil
                        break;
                    }
                    appendc(',', state);
                }
                appendc(']', state);
            } else {
                appendc('{', state);
                lua_pushnil(L);
                lua_next(L, -2);
                while (1) {
                    if (lua_type(L, -2) != LUA_TSTRING) {
                        // ignore entries without string fields.
                        lua_pop(L, 1); // pop value, leaving key
                        if (!lua_next(L, -2)) break;
                        continue;
                    }
                    lua_pushvalue(L, -2); // push key
                    append_value(L, state); // write key
                    appendc(':', state);
                    append_value(L, state); // write value, key now on top
                    if (!lua_next(L, -2)) break;
                    appendc(',', state);
                }
                appendc('}', state);
            }
            break;
        }
        default: appends("null", state);
    }
    lua_pop(L, 1); // pop value
}

void am_open_json_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"parse_json", parse_json},
        {"to_json", to_json},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
