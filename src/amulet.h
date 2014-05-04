#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#define AMULET_LUA_MODULE_NAME "amulet"

#include "SDL.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "am_util.h"
#include "am_gl.h"
#include "am_metatable_ids.h"
#include "am_lua_util.h"
#include "am_window.h"
#include "am_engine.h"
