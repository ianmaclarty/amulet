#include <stdio.h>

#define AMULET_LUA_MODULE_NAME "amulet"

extern "C" {
#include "SDL.h"
#include "SDL_opengl.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "am_metatable_ids.h"
#include "am_lua_util.h"
#include "am_window.h"
#include "am_gl.h"
#include "am_engine.h"
