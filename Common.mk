TARGET_PLATFORMS = linux32 linux64 win32 osx ios android html

DEBUG_TARGETS = $(patsubst %,%.debug,$(TARGET_PLATFORMS))
RELEASE_TARGETS = $(patsubst %,%.release,$(TARGET_PLATFORMS))

# Directories

DEPS_DIR   = deps
SDL_DIR    = $(DEPS_DIR)/SDL2-2.0.3
LUA_DIR    = $(DEPS_DIR)/lua-5.2.3
LUAJIT_DIR = $(DEPS_DIR)/LuaJIT-2.0.3
ANGLE_DIR  = $(DEPS_DIR)/angle-chrome_m34
GLM_DIR    = $(DEPS_DIR)/glm-0.9.5.3
GLEW_DIR   = $(DEPS_DIR)/glew-1.10.0
LIBPNG_DIR = $(DEPS_DIR)/libpng-1.4.3
ZLIB_DIR   = $(DEPS_DIR)/zlib-1.2.5
VORBIS_DIR = $(DEPS_DIR)/vorbis

# Visual C commands

VC_CL=cl.exe
VC_LINK=link.exe
VC_LIB=lib.exe

# Host settings (this is the *build* host, not the host we want to run on)

PATH_SEP = :

UNAME := $(shell uname)
ifneq (,$(findstring W32,$(UNAME)))
  HOST_PLATFORM = win32
  PATH_SEP = ;
else ifneq (,$(findstring Linux,$(UNAME)))
  UNAME_A := $(shell uname -a)
  ifneq (,$(findstring x86_64,$(UNAME_A)))
    HOST_PLATFORM = linux64
  else
    HOST_PLATFORM = linux32
  endif
else ifneq (,$(findstring Darwin,$(UNAME)))
  HOST_PLATFORM = osx
endif

ifndef HOST_PLATFORM
  $(error Unrecognised host)
endif

# Target settings (this is the target platform we want to run on)

-include settings

ifdef TARGET
  ifneq (,$(findstring .debug,$(TARGET)))
    GRADE = debug
    TARGET_PLATFORM = $(subst .debug,,$(TARGET))
  else ifneq (,$(findstring .release,$(TARGET)))
    GRADE = release
    TARGET_PLATFORM = $(subst .release,,$(TARGET))
  else
    $(error Unrecognised TARGET: $(TARGET))
  endif
else
  TARGET_PLATFORM = $(HOST_PLATFORM)
  GRADE = debug
  TARGET = $(TARGET_PLATFORM).$(GRADE)
endif

ifndef LUAVM
  LUAVM=luajit
endif

EXE_EXT = 
ALIB_EXT = .a
OBJ_EXT = .o
DEF_OPT = -D
INCLUDE_OPT = -I
CC = gcc
HOSTCC = gcc
CPP = g++
LINK = g++
AR = ar
AR_OPTS = rcus
AR_OUT_OPT =
LUA_TARGET = posix
XCFLAGS = -DLUA_COMPAT_ALL -Wall -Werror -ffast-math -pthread -fno-strict-aliasing
XLDFLAGS = -lGL -ldl -lm -lrt -pthread
LUA_CFLAGS = -DLUA_COMPAT_ALL
LUA_LDFLAGS = 
LUAJIT_CFLAGS = -DLUAJIT_ENABLE_LUA52COMPAT 
LUAJIT_LDFLAGS = 

EMSCRIPTEN_LIBS = src/library_sdl.js
EMSCRIPTEN_LIBS_OPTS = $(patsubst %,--js-library %,$(EMSCRIPTEN_LIBS))
EMSCRIPTEN_EXPORTS_OPT = -s EXPORTED_FUNCTIONS="['_main', '_am_emscripten_run', '_am_emscripten_resize']"

TARGET_CFLAGS=

# Adjust flags for target
ifeq ($(TARGET_PLATFORM),osx)
  CC = clang
  CPP = clang++
  LUA_TARGET = macosx
  XCFLAGS += -ObjC++
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGL -Wl,-framework,ForceFeedback -lobjc \
  	     -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit \
	     -pagezero_size 10000 -image_base 100000000
endif
ifeq ($(TARGET_PLATFORM),html)
  CC = emcc
  CPP = em++
  AR = emar
  LINK = em++
  LUAVM = lua
  LUA_TARGET = generic
  XLDFLAGS = -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 $(EMSCRIPTEN_EXPORTS_OPT) $(EMSCRIPTEN_LIBS_OPTS)
  #XLDFLAGS += -s DEMANGLE_SUPPORT=1
  XCFLAGS += -Wno-unneeded-internal-declaration $(EMSCRIPTEN_EXPORTS_OPT)
endif
ifeq ($(TARGET_PLATFORM),win32)
  EXE_EXT = .exe
  ALIB_EXT = .lib
  OBJ_EXT = .obj
  CC = $(VC_CL)
  CPP = $(VC_CL)
  LUA_TARGET = generic
  AR = $(VC_LIB)
  AR_OPTS = -nologo
  AR_OUT_OPT = -OUT:
  XCFLAGS = -fp:fast -WX 
  TARGET_CFLAGS = -nologo
  USE_CUSTOM_LUA_MAKEFILE = yes
endif

# Adjust flags for grade
ifeq ($(GRADE),debug)
  ifeq ($(TARGET_PLATFORM),html)
    GRADE_CFLAGS = -g -O0
    GRADE_LDFLAGS = -g -g4
    LUA_CFLAGS += -DLUA_USE_APICHECK -g -O0
    LUA_LDFLAGS += -g -g4
  else ifeq ($(TARGET_PLATFORM),win32)
    GRADE_CFLAGS = -Zi
    GRADE_LDFLAGS =
    LUA_CFLAGS += -DLUA_USE_APICHECK -Zi
    LUA_LDFLAGS +=
  else
    GRADE_CFLAGS = -g -O0
    GRADE_LDFLAGS = -g 
    LUA_CFLAGS += -DLUA_USE_APICHECK -g -O0
    LUA_LDFLAGS += -g
    LUAJIT_CFLAGS += -DLUA_USE_APICHECK -g -O0
    LUAJIT_LDFLAGS += -g
  endif
else
  ifeq ($(TARGET_PLATFORM),html)
    EM_PROFILING =
    #EM_PROFILING = --profiling
    GRADE_CFLAGS = -O3 $(EM_PROFILING) -DNDEBUG
    GRADE_LDFLAGS = -O3 $(EM_PROFILING) 
    LUA_CFLAGS += -O3 $(EM_PROFILING)
    LUA_LDFLAGS += -O3 $(EM_PROFILING)
  else ifeq ($(TARGET_PLATFORM),win32)
    GRADE_CFLAGS = -Ox -DNDEBUG
    GRADE_LDFLAGS =
    LUA_CFLAGS += -Ox
    LUA_LDFLAGS += -Ox
  else
    GRADE_CFLAGS = -O3 -DNDEBUG
    GRADE_LDFLAGS = -s
    LUA_CFLAGS += -O3
    LUA_LDFLAGS += -O3
  endif
endif

COMMON_CFLAGS := $(TARGET_CFLAGS) $(GRADE_CFLAGS) $(CFLAGS)
