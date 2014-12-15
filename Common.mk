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

# Visual C commands

VC_CL=cl.exe
VC_LINK=link.exe
VC_LIB=lib.exe

# Host settings (this is the *build* host, not the host we want to run on)

TARGET_PLATFORMS = linux32 linux64 win32 osx ios android html

DEBUG_TARGETS = $(patsubst %,%.debug,$(TARGET_PLATFORMS))
RELEASE_TARGETS = $(patsubst %,%.release,$(TARGET_PLATFORMS))

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
CPP = g++
LINK = g++
AR = ar
AR_OPTS = rcus
AR_OUT_OPT =
LUA_TARGET = posix
XCFLAGS = -ffast-math -Werror -pthread -fno-strict-aliasing
XLDFLAGS = -lGL -ldl -lm -lrt -pthread

EMSCRIPTEN_EXPORTS_OPT=-s EXPORTED_FUNCTIONS="['_main', '_am_restart_with_script']"

# Adjust flags for target
ifeq ($(TARGET_PLATFORM),osx)
  CC = clang
  CPP = clang++
  LUA_TARGET = macosx
endif
ifeq ($(TARGET_PLATFORM),html)
  CC = emcc
  CPP = em++
  AR = emar
  LINK = em++
  EXE_EXT = .html
  LUAVM = lua
  LUA_TARGET = generic
  XLDFLAGS = -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 $(EMSCRIPTEN_EXPORTS_OPT)
  #XLDFLAGS += -s DEMANGLE_SUPPORT=1
  XCFLAGS = $(EMSCRIPTEN_EXPORTS_OPT)
endif
ifeq ($(TARGET_PLATFORM),win32)
  EXE_EXT = .exe
  ALIB_EXT = .lib
  OBJ_EXT = .obj
  CC = CL
  CPP = CL
  LUA_TARGET = generic
  AR = $(VC_LIB)
  AR_OPTS = -nologo
  AR_OUT_OPT = -OUT:
  XCFLAGS = -fp:fast -Wall -WX
endif

# Adjust flags for grade
ifeq ($(GRADE),debug)
  ifeq ($(TARGET_PLATFORM),html)
    GRADE_CFLAGS = -g -O0
    GRADE_LDFLAGS = -g -g4
    LUA_CFLAGS = -DLUA_USE_APICHECK -g -O0
    LUA_LDFLAGS = -g -g4
  else
    GRADE_CFLAGS = -g -O0
    GRADE_LDFLAGS = -g 
    LUA_CFLAGS = -DLUA_USE_APICHECK -g -O0
    LUA_LDFLAGS = -g
    LUAJIT_CFLAGS = -DLUAJIT_ENABLE_LUA52COMPAT -DLUA_USE_APICHECK -g -O0
    LUAJIT_LDFLAGS = -g
  endif
else
  ifeq ($(TARGET_PLATFORM),html)
    EM_PROFILING =
    #EM_PROFILING = --profiling
    GRADE_CFLAGS = -O3 $(EM_PROFILING) -DNDEBUG
    GRADE_LDFLAGS = -O3 $(EM_PROFILING) 
    LUA_CFLAGS = -O3 $(EM_PROFILING)
    LUA_LDFLAGS = -O3 $(EM_PROFILING)
  else
    GRADE_CFLAGS = -O3 -DNDEBUG
    GRADE_LDFLAGS = -s
    LUA_CFLAGS = -O3
    LUA_LDFLAGS = -O3
    LUAJIT_CFLAGS = -DLUAJIT_ENABLE_LUA52COMPAT
    LUAJIT_LDFLAGS =
  endif
endif

COMMON_CFLAGS = $(GRADE_CFLAGS) $(XCFLAGS) $(CFLAGS)
