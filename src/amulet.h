#if defined(AM_LINUX32) || defined(AM_LINUX64)
    #define AM_LINUX
#endif
#if defined(AM_WIN32) || defined(AM_OSX) || defined(AM_LINUX) || defined(AM_ANDROID) || defined(AM_IOS)
    #define AM_BACKEND_SDL
#elif defined(AM_HTML)
    #define AM_BACKEND_EMSCRIPTEN
    #include <emscripten.h>
#else
    #error unsupported target
#endif

#ifdef AM_WIN32
#include <windows.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <math.h>

#include <new>
#include <climits>
#include <vector>

#include "c99.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "stb_vorbis.h"

extern "C" {
#include "png.h"
#include "pngconf.h"
}

#include "am_util.h"
#include "am_backend.h"
#include "am_time.h"
#include "am_input.h"
#include "am_embedded.h"
#include "am_userdata.h"
#include "am_config.h"
#include "am_reserved_refs.h"
#include "am_logging.h"
#include "am_gl.h"
#include "am_lua_util.h"
#include "am_math.h"
#include "am_window.h"
#include "am_buffer.h"
#include "am_texture2d.h"
#include "am_framebuffer.h"
#include "am_audio.h"
#include "am_trail.h"
#include "am_action.h"
#include "am_scene.h"
#include "am_renderer.h"
#include "am_program.h"
#include "am_transforms.h"
#include "am_depthbuffer.h"
#include "am_culling.h"
#include "am_image.h"
#include "am_model.h"
#include "am_engine.h"

#undef near
#undef far
