## Setup ##

SDL=SDL2-2.0.3
LUA=lua-5.2.3

-include settings

# Resolve host + target
UNAME := $(shell uname)
ifneq (,$(findstring W32,$(UNAME)))
HOST_PLATFORM = win32
PATH_SEP = ;
EXE_EXT = .exe
ALIB_EXT = .lib
else
PATH_SEP = :
EXE_EXT = 
ALIB_EXT = .a
endif
ifneq (,$(findstring Linux,$(UNAME)))
UNAME_A := $(shell uname -a)
ifneq (,$(findstring x86_64,$(UNAME_A)))
HOST_PLATFORM = linux64
else
HOST_PLATFORM = linux32
endif
endif
ifneq (,$(findstring Darwin,$(UNAME)))
HOST_PLATFORM = osx
endif

ifndef TARGET_PLATFORM
TARGET_PLATFORM=$(HOST_PLATFORM)
endif

ifndef HOST_PLATFORM
$(error Unrecognised host)
endif

ifndef GRADE
GRADE=debug
endif

# Banner
$(info == Building Amulet ==)
$(info Target: $(TARGET_PLATFORM))
$(info Host:   $(HOST_PLATFORM))
$(info =====================)

BUILD_BASE_DIR=build/$(TARGET_PLATFORM)/$(GRADE)
BUILD_BIN_DIR=$(BUILD_BASE_DIR)/bin
BUILD_LIB_DIR=$(BUILD_BASE_DIR)/lib
BUILD_INCLUDE_DIR=$(BUILD_BASE_DIR)/include
BUILD_STAGING_DIR=$(BUILD_BASE_DIR)/staging
AMULET_BINARY=$(BUILD_BIN_DIR)/amulet$(EXE_EXT)

SDL_ALIB=$(BUILD_BASE_DIR)/lib/libsdl$(ALIB_EXT)
LUA_ALIB=$(BUILD_BASE_DIR)/lib/liblua$(ALIB_EXT)

## Rules ##

default: $(AMULET_BINARY)

linux64: sdl.date.$(TARGET_PLATFORM) lua.date.$(TARGET_PLATFORM)

sdl.date.$(TARGET_PLATFORM):
	cd deps/$(SDL) && ./configure && make
	touch $@

lua.date.$(TARGET_PLATFORM):
	cd deps/$(LUA) && make posix
	touch $@

