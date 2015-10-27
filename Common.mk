SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SPACE1=
SPACE=$(SPACE1) $(SPACE1)

TARGET_PLATFORMS = linux32 linux64 msvc32 osx ios32 ios64 iossim android html mingw32 mingw64

DEBUG_TARGETS = $(patsubst %,%.debug,$(TARGET_PLATFORMS))
RELEASE_TARGETS = $(patsubst %,%.release,$(TARGET_PLATFORMS))

# Directories

DEPS_DIR         = deps
SDL_DIR          = $(DEPS_DIR)/SDL2-2.0.3
LUA51_DIR        = $(DEPS_DIR)/lua-5.1.5
LUA52_DIR        = $(DEPS_DIR)/lua-5.2.4
LUA53_DIR        = $(DEPS_DIR)/lua-5.3.1
LUAJIT_DIR       = $(DEPS_DIR)/LuaJIT-2.0.4
ANGLE_DIR        = $(DEPS_DIR)/angle-chrome_m34
GLM_DIR          = $(DEPS_DIR)/glm-0.9.7.1
FT2_DIR          = $(DEPS_DIR)/freetype-2.5.5
STB_DIR		 = $(DEPS_DIR)/stb
KISSFFT_DIR	 = $(DEPS_DIR)/kiss_fft130

SDL_WIN_PREBUILT_DIR = $(SDL_DIR)-VC-prebuilt
ANGLE_WIN_PREBUILT_DIR = $(DEPS_DIR)/angle-win-prebuilt

# Host settings (this is the *build* host, not the host we want to run on)

PATH_SEP = :

UNAME := $(shell uname)
ifneq (,$(findstring W32,$(UNAME)))
  HOST_PLATFORM = msvc32
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
  LUAVM = lua51
endif

SDL_ALIB = $(BUILD_LIB_DIR)/libsdl$(ALIB_EXT)
SDL_WIN_PREBUILT = $(BUILD_LIB_DIR)/sdl-win-prebuilt.date
ANGLE_ALIB = $(BUILD_LIB_DIR)/libangle$(ALIB_EXT)
ANGLE_WIN_PREBUILT = $(BUILD_LIB_DIR)/angle-win-prebuilt.date
LUA51_ALIB = $(BUILD_LIB_DIR)/liblua51$(ALIB_EXT)
LUA52_ALIB = $(BUILD_LIB_DIR)/liblua52$(ALIB_EXT)
LUA53_ALIB = $(BUILD_LIB_DIR)/liblua53$(ALIB_EXT)
LUAJIT_ALIB = $(BUILD_LIB_DIR)/libluajit$(ALIB_EXT)
LUAVM_ALIB = $(BUILD_LIB_DIR)/lib$(LUAVM)$(ALIB_EXT)
FT2_ALIB = $(BUILD_LIB_DIR)/libft2$(ALIB_EXT)
STB_ALIB = $(BUILD_LIB_DIR)/libstb$(ALIB_EXT)
KISSFFT_ALIB = $(BUILD_LIB_DIR)/libkissfft$(ALIB_EXT)

SRC_DIR = src
BUILD_BASE_DIR = builds/$(TARGET_PLATFORM)/$(GRADE)
BUILD_BIN_DIR = $(BUILD_BASE_DIR)/bin
BUILD_OBJ_DIR = $(BUILD_BASE_DIR)/obj
BUILD_LIB_DIR = $(BUILD_BASE_DIR)/lib
BUILD_INC_DIR = $(BUILD_BASE_DIR)/include

BUILD_LUA51_INCLUDE_DIR = $(BUILD_INC_DIR)/lua51
BUILD_LUA52_INCLUDE_DIR = $(BUILD_INC_DIR)/lua52
BUILD_LUA53_INCLUDE_DIR = $(BUILD_INC_DIR)/lua53
BUILD_LUAJIT_INCLUDE_DIR = $(BUILD_INC_DIR)/luajit

BUILD_DIRS = $(BUILD_BIN_DIR) $(BUILD_LIB_DIR) $(BUILD_INC_DIR) $(BUILD_OBJ_DIR) \
	$(BUILD_LUAJIT_INCLUDE_DIR) $(BUILD_LUA51_INCLUDE_DIR) \
	$(BUILD_LUA52_INCLUDE_DIR) $(BUILD_LUA53_INCLUDE_DIR)

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
XCFLAGS = -Wall -Werror -pthread -fno-strict-aliasing
XLDFLAGS = -ldl -lm -lrt -pthread
LUA_CFLAGS = -DLUA_COMPAT_ALL
LUAJIT_FLAGS = 
OBJ_OUT_OPT = -o
EXE_OUT_OPT = -o
NOLINK_OPT = -c

EMSCRIPTEN_LIBS = src/library_sdl.js
EMSCRIPTEN_LIBS_OPTS = $(patsubst %,--js-library %,$(EMSCRIPTEN_LIBS))
EMSCRIPTEN_EXPORTS_OPT = -s EXPORTED_FUNCTIONS="['_main', '_am_emscripten_run', '_am_emscripten_run_waiting', '_am_emscripten_pause', '_am_emscripten_resume', '_am_emscripten_resize']"

TARGET_CFLAGS=-ffast-math

# Adjust flags for target
ifeq ($(TARGET_PLATFORM),osx)
  CC = clang
  CPP = clang++
  LINK = clang++
  XCFLAGS += -ObjC++
  TARGET_CFLAGS += -m64 -arch x86_64
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGL -Wl,-framework,ForceFeedback -lobjc \
  	     -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit \
	     -Wl,-framework,AVFoundation -Wl,-framework,CoreVideo -Wl,-framework,CoreMedia \
	     -pagezero_size 10000 -image_base 100000000
  LUA_CFLAGS += -DLUA_USE_POSIX
  MACOSX_DEPLOYMENT_TARGET=10.6
  export MACOSX_DEPLOYMENT_TARGET
else ifeq ($(TARGET_PLATFORM),ios32)
  CC = clang
  CPP = clang++
  LINK = $(CPP)
  XCODE_PATH = $(shell xcode-select --print-path)
  SDK_VERSION = $(shell xcodebuild -showsdks | grep iphoneos | sed "s/.*iphoneos//")
  SDK_PATH = $(XCODE_PATH)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$(SDK_VERSION).sdk
  TARGET_CFLAGS += -arch armv7 -isysroot $(SDK_PATH) -miphoneos-version-min=5.0
  XCFLAGS += -ObjC++
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGLES -lobjc \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox \
	     -Wl,-framework,UIKit -Wl,-framework,QuartzCore \
	     -Wl,-framework,CoreMotion -Wl,-framework,Foundation \
	     -Wl,-framework,GLKit
  LUA_CFLAGS += -DLUA_USE_POSIX
  IOS = 1
else ifeq ($(TARGET_PLATFORM),ios64)
  CC = clang
  CPP = clang++
  LINK = $(CPP)
  XCODE_PATH = $(shell xcode-select --print-path)
  SDK_VERSION = $(shell xcodebuild -showsdks | grep iphoneos | sed "s/.*iphoneos//")
  SDK_PATH = $(XCODE_PATH)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$(SDK_VERSION).sdk
  TARGET_CFLAGS += -arch arm64 -isysroot $(SDK_PATH) -miphoneos-version-min=5.0
  XCFLAGS += -ObjC++
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGLES -lobjc \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox \
	     -Wl,-framework,UIKit -Wl,-framework,QuartzCore \
	     -Wl,-framework,CoreMotion -Wl,-framework,Foundation \
	     -Wl,-framework,GLKit
  LUA_CFLAGS += -DLUA_USE_POSIX
  IOS = 1
else ifeq ($(TARGET_PLATFORM),iossim)
  CC = clang
  CPP = clang++
  LINK = $(CPP)
  XCODE_PATH = $(shell xcode-select --print-path)
  SDK_VERSION = $(shell xcodebuild -showsdks | grep iphoneos | sed "s/.*iphoneos//")
  SDK_PATH = $(XCODE_PATH)/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator$(SDK_VERSION).sdk
  TARGET_CFLAGS += -arch x86_64 -isysroot $(SIM_SDK_PATH) -miphoneos-version-min=5.0
  XCFLAGS += -ObjC++
  XLDFLAGS = -lm -liconv -Wl,-framework,OpenGLES -lobjc \
	     -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox \
	     -Wl,-framework,UIKit -Wl,-framework,QuartzCore \
	     -Wl,-framework,CoreMotion -Wl,-framework,Foundation \
	     -Wl,-framework,GLKit
  LUA_CFLAGS += -DLUA_USE_POSIX
  IOS = 1
else ifeq ($(TARGET_PLATFORM),html)
  CC = emcc
  CPP = em++
  AR = emar
  LINK = em++
  XLDFLAGS = --memory-init-file 0 -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 $(EMSCRIPTEN_EXPORTS_OPT) $(EMSCRIPTEN_LIBS_OPTS)
  #XLDFLAGS += -s DEMANGLE_SUPPORT=1
  XCFLAGS += -Wno-unneeded-internal-declaration $(EMSCRIPTEN_EXPORTS_OPT)
  EXE_OUT_OPT = -o$(SPACE)
  OBJ_OUT_OPT = -o$(SPACE)
else ifeq ($(TARGET_PLATFORM),msvc32)
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
  AR = $(VC_LIB)
  AR_OPTS = -nologo
  AR_OUT_OPT = -OUT:
  XCFLAGS = -DLUA_COMPAT_ALL -WX 
  XLDFLAGS = -SUBSYSTEM:CONSOLE \
	-NODEFAULTLIB:msvcrt.lib \
	$(BUILD_LIB_DIR)/SDL2.lib
  TARGET_CFLAGS = -nologo -EHsc -fp:fast
else ifeq ($(TARGET_PLATFORM),mingw32)
  EXE_EXT = .exe
  CC = i686-w64-mingw32-gcc
  CPP = i686-w64-mingw32-g++
  LINK = $(CPP)
  AR = i686-w64-mingw32-ar
  XLDFLAGS = -static $(BUILD_LIB_DIR)/SDL2.lib
  XCFLAGS = -Wall -Werror -fno-strict-aliasing
  LUAJIT_FLAGS += HOST_CC="gcc -m32" CROSS=i686-w64-mingw32- TARGET_SYS=Windows
else ifeq ($(TARGET_PLATFORM),linux32)
  TARGET_CFLAGS += -m32
  LDFLAGS += -m32
  LUA_CFLAGS += -DLUA_USE_POSIX
  LUAJIT_FLAGS += CC="gcc -m32"
else ifeq ($(TARGET_PLATFORM),linux64)
  LUA_CFLAGS += -DLUA_USE_POSIX
endif

# Adjust flags for grade
ifeq ($(GRADE),debug)
  ifeq ($(TARGET_PLATFORM),html)
    GRADE_CFLAGS = -O1 -profiling
    GRADE_LDFLAGS = -profiling
    LUA_CFLAGS += -DLUA_USE_APICHECK
  else ifeq ($(TARGET_PLATFORM),msvc32)
    GRADE_CFLAGS = -MTd -Zi
    GRADE_LDFLAGS = -DEBUG
    LUA_CFLAGS += -DLUA_USE_APICHECK
  else
    GRADE_CFLAGS = -g -O1
    GRADE_LDFLAGS = -g 
    LUA_CFLAGS += -DLUA_USE_APICHECK
    LUAJIT_FLAGS += CFLAGS="-DLUA_USE_APICHECK -g" LDFLAGS=-g
  endif
else
  ifeq ($(TARGET_PLATFORM),html)
    EM_PROFILING =
    #EM_PROFILING = --profiling
    GRADE_CFLAGS = -O3 $(EM_PROFILING) -DNDEBUG
    GRADE_LDFLAGS = -O3 $(EM_PROFILING) 
  else ifeq ($(TARGET_PLATFORM),msvc32)
    GRADE_CFLAGS = -Ox -DNDEBUG
    GRADE_LDFLAGS =
  else ifeq ($(TARGET_PLATFORM),osx)
    GRADE_CFLAGS = -O3 -DNDEBUG
    GRADE_LDFLAGS =
  else
    GRADE_CFLAGS = -O3 -DNDEBUG
    GRADE_LDFLAGS = -s
  endif
endif

COMMON_CFLAGS := $(TARGET_CFLAGS) $(GRADE_CFLAGS) $(CFLAGS)
