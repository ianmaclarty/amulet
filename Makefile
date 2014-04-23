# Globals 

SRC_DIR    = src
DEPS_DIR   = deps
SDL_DIR    = $(DEPS_DIR)/SDL2-2.0.3
LUA_DIR    = $(DEPS_DIR)/lua-5.2.3
LUAJIT_DIR = $(DEPS_DIR)/LuaJIT-2.0.3
ANGLE_DIR  = $(DEPS_DIR)/angle-chrome_m34
GLM_DIR    = $(DEPS_DIR)/glm-0.9.5.3
GLEW_DIR   = $(DEPS_DIR)/glew-1.10.0

TARGET_PLATFORMS = linux32 linux64 win32 osx ios android html5

DEBUG_TARGETS = $(patsubst %,%.debug,$(TARGET_PLATFORMS))
RELEASE_TARGETS = $(patsubst %,%.release,$(TARGET_PLATFORMS))

TARGETS = $(DEBUG_TARGETS) $(RELEASE_TARGETS)

# Resolve host

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

# Target platform setup

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

EXE_EXT = 
ALIB_EXT = .a
OBJ_EXT = .o
CC=gcc
CPP=g++
DEPS=lua sdl glew
LUA_TARGET=posix
ifeq ($(TARGET_PLATFORM),osx)
  CC=clang
  CPP=clang++
  LUA_TARGET=macosx
endif
ifeq ($(TARGET_PLATFORM),html5)
  CC=emcc
  CPP=em++
  EXE_EXT=.html
  DEPS=lua
  LUA_TARGET=generic
endif
ifeq ($(TARGET_PLATFORM),win32)
  EXE_EXT = .exe
  ALIB_EXT = .lib
  OBJ_EXT = .obj
  CC=CL
  CPP=CL
  LUA_TARGET=generic
endif

# Build settings

BUILD_BASE_DIR = build/$(TARGET_PLATFORM)/$(GRADE)
BUILD_BIN_DIR = $(BUILD_BASE_DIR)/bin
BUILD_LIB_DIR = $(BUILD_BASE_DIR)/lib
BUILD_INCLUDE_DIR = $(BUILD_BASE_DIR)/include
BUILD_STAGING_DIR = $(BUILD_BASE_DIR)/staging
BUILD_DIRS = $(BUILD_BIN_DIR) $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR) $(BUILD_STAGING_DIR)

AMULET = $(BUILD_BIN_DIR)/amulet$(EXE_EXT)

DEP_LIBS = $(patsubst %,$(BUILD_LIB_DIR)/lib%$(ALIB_EXT),$(DEPS))

SDL_ALIB = $(BUILD_LIB_DIR)/libsdl$(ALIB_EXT)
GLEW_ALIB = $(BUILD_LIB_DIR)/libglew$(ALIB_EXT)
LUA_ALIB = $(BUILD_LIB_DIR)/liblua$(ALIB_EXT)

MAIN_TARGET = $(AMULET)

AM_CPP_FILES = $(wildcard $(SRC_DIR)/*.cpp)
AM_H_FILES = $(wildcard $(SRC_DIR)/*.h)
AM_OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_STAGING_DIR)/%$(OBJ_EXT),$(AM_CPP_FILES))

GRADE_CFLAGS=-g
CC_FLAGS = $(GRADE_CFLAGS) -I$(BUILD_INCLUDE_DIR) -pthread
LINK_FLAGS = $(GRADE_CFLAGS) $(SDL_ALIB) $(LUA_ALIB) $(GLEW_ALIB) -lGL -ldl -lm -lrt -pthread

# Rules

default: $(AMULET)

$(AMULET): $(DEP_LIBS) $(AM_OBJ_FILES) | $(BUILD_BIN_DIR)
	$(CPP) $(AM_OBJ_FILES) $(LINK_FLAGS) -o $@
	ln -fs $@ `basename $@`

$(AM_OBJ_FILES): $(BUILD_STAGING_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp $(AM_H_FILES) | $(BUILD_STAGING_DIR)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(SDL_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(SDL_DIR) && ./configure CC=$(CC) CXX=$(CPP) && $(MAKE) clean && $(MAKE)
	cp -r $(SDL_DIR)/include/* $(BUILD_INCLUDE_DIR)/
	cp $(SDL_DIR)/build/.libs/libSDL2.a $@

$(LUA_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(LUA_DIR) && $(MAKE) clean $(LUA_TARGET)
	cp $(LUA_DIR)/src/lua.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/lauxlib.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/luaconf.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/lualib.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/liblua.a $@

$(GLEW_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(GLEW_DIR) && $(MAKE) clean all
	cp -r $(GLEW_DIR)/include/* $(BUILD_INCLUDE_DIR)/
	cp $(GLEW_DIR)/lib/libGLEW.a $@

$(TARGETS): %:
	@$(MAKE) TARGET=$@

$(BUILD_DIRS): %:
	mkdir -p $@

clean:
	rm -f $(BUILD_STAGING_DIR)/*
	rm -f $(BUILD_BIN_DIR)/*

clean-target:
	rm -rf $(BUILD_BASE_DIR)

clean-all:
	rm -rf build

# Banner
ifeq (,$(MAKECMDGOALS))
  $(info ======== Amulet build settings ========)
  $(info Target: $(TARGET_PLATFORM))
  $(info Grade:  $(GRADE))
  $(info Host:   $(HOST_PLATFORM))
  $(info CC:     $(CC))
  $(info CPP:    $(CPP))
  $(info DEPS:   $(DEPS))
  $(info =======================================)
endif

# Tags
.PHONY: tags
tags:
	ctags `find $(SRC_DIR) -name "*.c"` `find $(SRC_DIR) -name "*.cpp"` \
		`find $(SRC_DIR) -name "*.h"` `find $(DEPS_DIR) -name "*.c"` \
		`find $(DEPS_DIR) -name "*.cpp"` `find $(DEPS_DIR) -name "*.h"`
