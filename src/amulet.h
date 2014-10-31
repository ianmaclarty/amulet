#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>
#include <climits>
#include <vector>

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

#include "am_embedded_lua.h"
#include "am_userdata.h"
#include "am_bit_util.h"
#include "am_config.h"
#include "am_reserved_refs.h"
#include "am_logging.h"
#include "am_gl.h"
#include "am_lua_util.h"
#include "am_math.h"
#include "am_window.h"
#include "am_buffer.h"
#include "am_trail.h"
#include "am_renderer.h"
#include "am_action.h"
#include "am_scene.h"
#include "am_program.h"
#include "am_engine.h"
