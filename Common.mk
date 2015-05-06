SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

TARGET_PLATFORMS = linux32 linux64 win32 osx ios android html

DEBUG_TARGETS = $(patsubst %,%.debug,$(TARGET_PLATFORMS))
RELEASE_TARGETS = $(patsubst %,%.release,$(TARGET_PLATFORMS))

# Directories

DEPS_DIR         = deps
SDL_DIR          = $(DEPS_DIR)/SDL2-2.0.3
LUA_DIR          = $(DEPS_DIR)/lua-5.2.3
LUAJIT_DIR       = $(DEPS_DIR)/LuaJIT-2.0.3
ANGLE_DIR        = $(DEPS_DIR)/angle-chrome_m34
GLM_DIR          = $(DEPS_DIR)/glm-0.9.5.3
FT2_DIR          = $(DEPS_DIR)/freetype-2.5.5

SDL_WIN_PREBUILT_DIR = $(SDL_DIR)-VC-prebuilt
ANGLE_WIN_PREBUILT_DIR = $(DEPS_DIR)/angle-win-prebuilt
LIBTURBOJPEG_WIN_PREBUILT_DIR = $(DEPS_DIR)/libjpeg-turbo-1.4.0-VC-prebuilt

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

-include $(SELF_DIR)settings

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
  GRADE = release
  TARGET = $(TARGET_PLATFORM).$(GRADE)
endif

ifndef LUAVM
  LUAVM=luajit
endif

SDL_ALIB = $(BUILD_LIB_DIR)/libsdl$(ALIB_EXT)
SDL_WIN_PREBUILT = $(BUILD_LIB_DIR)/sdl-win-prebuilt.date
ANGLE_ALIB = $(BUILD_LIB_DIR)/libangle$(ALIB_EXT)
ANGLE_WIN_PREBUILT = $(BUILD_LIB_DIR)/angle-win-prebuilt.date
LUA_ALIB = $(BUILD_LIB_DIR)/liblua$(ALIB_EXT)
LUAJIT_ALIB = $(BUILD_LIB_DIR)/libluajit$(ALIB_EXT)
LUAVM_ALIB = $(BUILD_LIB_DIR)/lib$(LUAVM)$(ALIB_EXT)
FT2_ALIB = $(BUILD_LIB_DIR)/libft2$(ALIB_EXT)

SRC_DIR = src
BUILD_BASE_DIR = builds/$(TARGET_PLATFORM)/$(GRADE)
BUILD_BIN_DIR = $(BUILD_BASE_DIR)/bin
BUILD_OBJ_DIR = $(BUILD_BASE_DIR)/obj
BUILD_LIB_DIR = $(BUILD_BASE_DIR)/lib
BUILD_INC_DIR = $(BUILD_BASE_DIR)/include

BUILD_LUA_INCLUDE_DIR = $(BUILD_INC_DIR)/lua
BUILD_LUAJIT_INCLUDE_DIR = $(BUILD_INC_DIR)/luajit

BUILD_DIRS = $(BUILD_BIN_DIR) $(BUILD_LIB_DIR) $(BUILD_INC_DIR) $(BUILD_OBJ_DIR) \
	$(BUILD_LUAJIT_INCLUDE_DIR) $(BUILD_LUA_INCLUDE_DIR)

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
OBJ_OUT_OPT = -o
EXE_OUT_OPT = -o
NOLINK_OPT = -c

EMSCRIPTEN_LIBS = src/library_sdl.js
EMSCRIPTEN_LIBS_OPTS = $(patsubst %,--js-library %,$(EMSCRIPTEN_LIBS))
EMSCRIPTEN_EXPORTS_OPT = -s EXPORTED_FUNCTIONS="['_main', '_am_emscripten_run', '_am_emscripten_resize']"

TARGET_CFLAGS=

# Adjust flags for target
ifeq ($(TARGET_PLATFORM),osx)
  CC = clang
  CPP = clang++
  LINK = clang++
  LUA_TARGET = macosx
  XCFLAGS += -ObjC++
  TARGET_CFLAGS += -m64 -arch x86_64
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGL -Wl,-framework,ForceFeedback -lobjc \
  	     -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit \
	     -pagezero_size 10000 -image_base 100000000
  LUA_CFLAGS += -DLUA_USE_POSIX
  MACOSX_DEPLOYMENT_TARGET=10.6
  export MACOSX_DEPLOYMENT_TARGET
else ifeq ($(TARGET_PLATFORM),ios)
  CC = $(SELF_DIR)tools/ioscc
  CPP = $(SELF_DIR)tools/iosc++
  LINK = $(CPP)
  XCODE_PATH=$(shell xcode-select --print-path)
  SDK_VERSION=$(shell xcodebuild -showsdks | grep iphoneos | sed "s/.*iphoneos//")
  SDK_PATH=$(XCODE_PATH)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$(SDK_VERSION).sdk
  AM_ARMV7_FLAGS=-arch armv7 -isysroot $(SDK_PATH)
  export AM_ARMV7_FLAGS
  AM_ARMV7S_FLAGS=-arch armv7s -isysroot $(SDK_PATH)
  export AM_ARMV7S_FLAGS
  AM_ARM64_FLAGS=-arch arm64 -isysroot $(SDK_PATH)
  export AM_ARM64_FLAGS
  LUA_TARGET = generic
  XCFLAGS += -ObjC++
  TARGET_CFLAGS += -miphoneos-version-min=4.3 
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGLES -lobjc \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox \
	     -Wl,-framework,UIKit -Wl,-framework,QuartzCore \
	     -Wl,-framework,CoreMotion -Wl,-framework,Foundation \
	     $(TARGET_CFLAGS)
  LUA_CFLAGS += -DLUA_USE_POSIX
  IPHONEOS_DEPLOYMENT_TARGET=4.3
  export IPHONEOS_DEPLOYMENT_TARGET
else ifeq ($(TARGET_PLATFORM),html)
  CC = emcc
  CPP = em++
  AR = emar
  LINK = em++
  LUAVM = lua
  LUA_TARGET = generic
  XLDFLAGS = -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 $(EMSCRIPTEN_EXPORTS_OPT) $(EMSCRIPTEN_LIBS_OPTS)
  #XLDFLAGS += -s DEMANGLE_SUPPORT=1
  XCFLAGS += -Wno-unneeded-internal-declaration $(EMSCRIPTEN_EXPORTS_OPT)
else ifeq ($(TARGET_PLATFORM),win32)
  VC_CL = cl.exe
  VC_CL_PATH = $(shell which $(VC_CL))
  VC_CL_DIR = $(shell dirname "$(VC_CL_PATH)")
  VC_LINK = $(VC_CL_DIR)/link.exe
  VC_LIB = $(VC_CL_DIR)/lib.exe
  EXE_EXT = .exe
  ALIB_EXT = .lib
  OBJ_EXT = .obj
  OBJ_OUT_OPT = -Fo
  EXE_OUT_OPT = /OUT:
  CC = $(VC_CL)
  CPP = $(VC_CL)
  LINK = $(VC_LINK)
  LUA_TARGET = generic
  AR = $(VC_LIB)
  AR_OPTS = -nologo
  AR_OUT_OPT = -OUT:
  XCFLAGS = -DLUA_COMPAT_ALL -fp:fast -WX 
  XLDFLAGS = -SUBSYSTEM:CONSOLE \
	-NODEFAULTLIB:msvcrt.lib \
	$(BUILD_LIB_DIR)/libturbojpeg.lib \
	$(BUILD_LIB_DIR)/SDL2.lib
  TARGET_CFLAGS = -nologo -EHsc
else
  LUA_CFLAGS += -DLUA_USE_POSIX
endif

# Adjust flags for grade
ifeq ($(GRADE),debug)
  ifeq ($(TARGET_PLATFORM),html)
    GRADE_CFLAGS = -O1
    GRADE_LDFLAGS =
    LUA_CFLAGS += -DLUA_USE_APICHECK -O1
    LUA_LDFLAGS +=
  else ifeq ($(TARGET_PLATFORM),win32)
    GRADE_CFLAGS = -MTd -Zi
    GRADE_LDFLAGS = -DEBUG
    LUA_CFLAGS += -DLUA_USE_APICHECK
    LUA_LDFLAGS +=
  else
    GRADE_CFLAGS = -g -O1
    GRADE_LDFLAGS = -g 
    LUA_CFLAGS += -DLUA_USE_APICHECK -g -O1
    LUA_LDFLAGS += -g
    LUAJIT_CFLAGS += -DLUA_USE_APICHECK -g -O1
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
  else ifeq ($(TARGET_PLATFORM),osx)
    GRADE_CFLAGS = -O3 -DNDEBUG
    GRADE_LDFLAGS =
    LUA_CFLAGS += -O3
    LUA_LDFLAGS += -O2
  else
    GRADE_CFLAGS = -O3 -DNDEBUG
    GRADE_LDFLAGS = -s
    LUA_CFLAGS += -O3
    LUA_LDFLAGS += -O3
  endif
endif

COMMON_CFLAGS := $(TARGET_CFLAGS) $(GRADE_CFLAGS) $(CFLAGS)
