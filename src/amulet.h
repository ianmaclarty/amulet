#if defined(AM_LINUX32) || defined(AM_LINUX64)
    #define AM_LINUX
#endif
#if defined(AM_WIN32) || defined(AM_OSX) || defined(AM_LINUX) || defined(AM_ANDROID)
    #define AM_BACKEND_SDL
#elif defined(AM_IOS)
    #define AM_BACKEND_IOS
#elif defined(AM_HTML)
    #define AM_BACKEND_EMSCRIPTEN
    #include <emscripten.h>
#else
    #error unsupported target
#endif
#if defined(AM_OSX) || defined(AM_LINUX)
    #define AM_GLPROFILE_DESKTOP 1
#elif defined(AM_WIN32) || defined(AM_ANDROID) || defined(AM_IOS) || defined(AM_HTML)
    #define AM_GLPROFILE_ES 1
#else
    #error unsupported target
#endif

#if defined(AM_GLPROFILE_ES) && defined(AM_WIN32)
#define AM_NEED_GLES2_FUNC_PTRS
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
#include <unistd.h>

#include <new>
#include <climits>
#include <vector>

#include "c99.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

extern "C" {
#include "ft2build.h"
#include FT_FREETYPE_H
}

extern "C" {
#define STB_VORBIS_NO_STDIO 1
#include "stb_image.h"
#include "stb_vorbis.h"
#include "miniz.h"
}

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "glm/gtc/quaternion.hpp"

#include "am_util.h"
#include "am_utf8.h"
#include "am_package.h"
#include "am_gl.h"
#include "am_backend.h"
#include "am_time.h"
#include "am_input.h"
#include "am_embedded.h"
#include "am_userdata.h"
#include "am_config.h"
#include "am_options.h"
#include "am_reserved_refs.h"
#include "am_logging.h"
#include "am_lua_util.h"
#include "am_math.h"
#include "am_buffer.h"
#include "am_texture2d.h"
#include "am_framebuffer.h"
#include "am_audio.h"
#include "am_trail.h"
#include "am_action.h"
#include "am_scene.h"
#include "am_window.h"
#include "am_renderer.h"
#include "am_program.h"
#include "am_transforms.h"
#include "am_depthbuffer.h"
#include "am_culling.h"
#include "am_blending.h"
#include "am_image.h"
#include "am_text.h"
#include "am_model.h"
#include "am_engine.h"

#undef near
#undef far
