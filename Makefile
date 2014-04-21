# Globals 

SDL_DIR = deps/SDL2-2.0.3
LUA_DIR = deps/lua-5.2.3
LUAJIT_DIR = deps/LuaJIT-2.0.3
ANGLE_DIR = deps/angle-chrome_m34
GLM_DIR = deps/glm-0.9.5.3

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
DEPS=lua sdl
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
LUA_ALIB = $(BUILD_LIB_DIR)/liblua$(ALIB_EXT)

MAIN_TARGET = $(AMULET)

AM_C_FILES = $(wildcard src/c/*.c)
AM_H_FILES = $(wildcard src/c/*.h)
AM_OBJ_FILES = $(patsubst src/c/%.c,$(BUILD_STAGING_DIR)/%$(OBJ_EXT),$(AM_C_FILES))

GRADE_CFLAGS=-g
CC_FLAGS = $(GRADE_CFLAGS) -I$(BUILD_INCLUDE_DIR) -pthread
LINK_FLAGS = $(GRADE_CFLAGS) $(BUILD_LIB_DIR)/libsdl.a $(BUILD_LIB_DIR)/liblua.a -lGL -ldl -lm -lrt -pthread

# Rules

default: $(AMULET)

$(AMULET): $(AM_OBJ_FILES) $(DEP_LIBS) | $(BUILD_BIN_DIR)
	$(CC) $(AM_OBJ_FILES) $(LINK_FLAGS) -o $@
	ln -fs $@ `basename $@`

$(AM_OBJ_FILES): $(BUILD_STAGING_DIR)/%$(OBJ_EXT): src/c/%.c $(AM_H_FILES) | $(BUILD_STAGING_DIR)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(SDL_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(SDL_DIR) && ./configure CC=$(CC) CXX=$(CPP) && $(MAKE) clean && $(MAKE)
	cp $(SDL_DIR)/build/.libs/libSDL2.a $@
	cp -r $(SDL_DIR)/include/* $(BUILD_INCLUDE_DIR)/

$(LUA_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(LUA_DIR) && $(MAKE) clean $(LUA_TARGET)
	cp $(LUA_DIR)/src/liblua.a $@
	cp $(LUA_DIR)/src/lua.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/lauxlib.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/luaconf.h $(BUILD_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/lualib.h $(BUILD_INCLUDE_DIR)/

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
	ctags `find src -name "*.c"` `find src -name "*.cpp"` `find src -name "*.h"` `find deps -name "*.c"` `find deps -name "*.cpp"` `find deps -name "*.h"`
