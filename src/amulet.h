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

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "metatable_ids.h"
#include "logging.h"
#include "gl.h"
#include "lua_util.h"
#include "window.h"
#include "engine.h"
#include "shader_vars.h"
