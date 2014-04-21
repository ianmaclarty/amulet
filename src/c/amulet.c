#include "amulet.h"

static int report_status(lua_State *L, int status);

int main (int argc, char **argv) {
    SDL_Init(SDL_INIT_VIDEO);

    if (argc < 2) {
        fprintf(stderr, "expecting 1 argument\n"); 
        return 1;
    }
    lua_State *L = am_engine_init();
    int status = luaL_dofile(L, argv[1]);
    report_status(L, status);
    am_engine_destroy(L);

    SDL_Quit();
    return status;
}

static int report_status(lua_State *L, int status) {
    if (status && !lua_isnil(L, -1)) {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL) msg = "(error object is not a string)";
        fprintf(stderr, "%s\n", msg);
        lua_pop(L, 1);
    }
    return status;
}
