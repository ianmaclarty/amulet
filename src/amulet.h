#if defined(AM_LINUX32) || defined(AM_LINUX64)
    #define AM_LINUX
#endif
#if defined(AM_MINGW32) || defined(AM_MINGW64)
    #define AM_MINGW
#endif
#if defined(AM_MSVC64) || defined(AM_MSVC32)
    #define AM_MSVC
#endif
#if defined(AM_MINGW) || defined(AM_MSVC)
    #define AM_WINDOWS
#endif
#if defined(AM_IOS32) || defined(AM_IOS64) || defined(AM_IOSSIM)
    #define AM_IOS
#endif
#if defined(AM_ANDROID_ARM32) || defined(AM_ANDROID_ARM64) || defined(AM_ANDROID_X86) || defined(AM_ANDROID_X86_64)
    #define AM_ANDROID
#endif
#if defined(AM_WINDOWS) || defined(AM_OSX) || defined(AM_LINUX)
    #define AM_BACKEND_SDL
#elif defined(AM_IOS)
    #define AM_BACKEND_IOS
#elif defined(AM_ANDROID)
    #define AM_BACKEND_ANDROID
#elif defined(AM_HTML)
    #define AM_BACKEND_EMSCRIPTEN
    #include <emscripten.h>
#else
    #error unsupported target
#endif
#if defined(AM_OSX) || defined(AM_LINUX) || defined(AM_WINDOWS)
    #define AM_GLPROFILE_DESKTOP 1
#elif defined(AM_ANDROID) || defined(AM_IOS) || defined(AM_HTML)
    #define AM_GLPROFILE_ES 1
#else
    #error unsupported target
#endif

#if defined(AM_WINDOWS) || defined(AM_LINUX) || defined(AM_OSX)
    #define AM_HAVE_GLOB
    #define AM_EXPORT
    #define AM_SPRITEPACK
#endif

#if defined(AM_WINDOWS) || defined(AM_LINUX)
#define AM_NEED_GL_FUNC_PTRS
#endif

#if (UINTPTR_MAX == UINT64_MAX)
    #define AM_64BIT 1
#elif (UINTPTR_MAX == UINT32_MAX)
    #define AM_32BIT 1
#else
    #error unsupported word size
#endif


#ifdef AM_WINDOWS
#define UNICODE 1
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <inttypes.h>
#undef __STDC_LIMIT_MACROS
#include <math.h>
#include <time.h>

// networking
#if defined(AM_IOS) || defined(AM_OSX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#endif

#if defined(AM_ANDROID)
#include <android/log.h>
#endif

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
#define STB_VORBIS_NO_STDIO 1
#include "stb_image.h"
#include "stb_vorbis.h"
#include "miniz.h"
}

#include "kiss_fftr.h"

#include "tinymt32.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "glm/gtc/quaternion.hpp"

#include "am_version.h"
#include "am_alloc.h"
#include "am_util.h"
#include "am_utf8.h"
#include "am_package.h"
#include "am_gl.h"
#include "am_time.h"
#include "am_input.h"
#include "am_embedded.h"
#include "am_userdata.h"
#include "am_backend.h"
#include "am_config.h"
#include "am_options.h"
#include "am_reserved_refs.h"
#include "am_logging.h"
#include "am_lua_util.h"
#include "am_math.h"
#include "am_buffer.h"
#include "am_mathv.h"
#include "am_view.h"
#include "am_image.h"
#include "am_texture2d.h"
#include "am_vbo.h"
#include "am_audio.h"
#include "am_action.h"
#include "am_scene.h"
#include "am_framebuffer.h"
#include "am_window.h"
#include "am_renderer.h"
#include "am_program.h"
#include "am_transforms.h"
#include "am_depthbuffer.h"
#include "am_stencilbuffer.h"
#include "am_culling.h"
#include "am_blending.h"
#include "am_model.h"
#include "am_engine.h"
#include "am_json.h"
#include "am_http.h"
#include "am_browser.h"
#include "am_spritepack.h"
#include "am_export.h"
#include "am_rand.h"
#include "am_sfxr.h"
#include "am_glob.h"
#include "am_i18n.h"
#include "am_net.h"
#include "am_steamworks.h"
#include "am_native.h"

#undef near
#undef far
