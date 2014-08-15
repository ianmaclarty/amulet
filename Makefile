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
DEPS = $(LUAVM) sdl glew
LUA_TARGET = posix
XCFLAGS = -Werror -Wall -pthread -fno-strict-aliasing
XLDFLAGS = -lGL -ldl -lm -lrt -pthread
AM_DEFS = AM_$(shell echo $(TARGET_PLATFORM) | tr a-z A-Z) AM_$(shell echo $(GRADE) | tr a-z A-Z)

ifeq ($(TARGET_PLATFORM),osx)
  CC = clang
  CPP = clang++
  LUA_TARGET = macosx
endif
ifeq ($(TARGET_PLATFORM),html5)
  CC = emcc
  CPP = em++
  EXE_EXT = .html
  LUAVM = lua
  DEPS = lua
  LUA_TARGET = generic
endif
ifeq ($(TARGET_PLATFORM),win32)
  EXE_EXT = .exe
  ALIB_EXT = .lib
  OBJ_EXT = .obj
  CC = CL
  CPP = CL
  LUA_TARGET = generic
endif

# Build settings

BUILD_BASE_DIR = builds/$(TARGET_PLATFORM)/$(GRADE)
BUILD_BIN_DIR = $(BUILD_BASE_DIR)/bin
BUILD_LIB_DIR = $(BUILD_BASE_DIR)/lib
BUILD_INCLUDE_DIR = $(BUILD_BASE_DIR)/include
BUILD_STAGING_DIR = $(BUILD_BASE_DIR)/staging

BUILD_LUA_INCLUDE_DIR = $(BUILD_INCLUDE_DIR)/lua
BUILD_LUAJIT_INCLUDE_DIR = $(BUILD_INCLUDE_DIR)/luajit

BUILD_DIRS = $(BUILD_BIN_DIR) $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR) $(BUILD_STAGING_DIR) \
	$(BUILD_LUAJIT_INCLUDE_DIR) $(BUILD_LUA_INCLUDE_DIR)

ifeq ($(LUAVM),luajit)
  BUILD_LUAVM_INCLUDE_DIR=$(BUILD_LUAJIT_INCLUDE_DIR)
  AM_DEFS += AM_LUAJIT
else
  BUILD_LUAVM_INCLUDE_DIR=$(BUILD_LUA_INCLUDE_DIR)
endif

AMULET = $(BUILD_BIN_DIR)/amulet$(EXE_EXT)

DEP_ALIBS = $(patsubst %,$(BUILD_LIB_DIR)/lib%$(ALIB_EXT),$(DEPS))

SDL_ALIB = $(BUILD_LIB_DIR)/libsdl$(ALIB_EXT)
GLEW_ALIB = $(BUILD_LIB_DIR)/libglew$(ALIB_EXT)
LUA_ALIB = $(BUILD_LIB_DIR)/liblua$(ALIB_EXT)
LUAJIT_ALIB = $(BUILD_LIB_DIR)/libluajit$(ALIB_EXT)
LUAVM_ALIB = $(BUILD_LIB_DIR)/lib$(LUAVM)$(ALIB_EXT)

MAIN_TARGET = $(AMULET)

AM_CPP_FILES = $(wildcard $(SRC_DIR)/*.cpp)
AM_H_FILES = $(wildcard $(SRC_DIR)/*.h)
AM_OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_STAGING_DIR)/%$(OBJ_EXT),$(AM_CPP_FILES))

ifeq ($(GRADE),debug)
  GRADE_CFLAGS = -g -O0
  GRADE_LDFLAGS = -g
  LUA_CFLAGS = -DLUA_USE_APICHECK -g -O0
  LUA_LDFLAGS = -g
  LUAJIT_CFLAGS = -DLUAJIT_ENABLE_LUA52COMPAT -DLUA_USE_APICHECK -g -O0
  LUAJIT_LDFLAGS = -g
else
  GRADE_CFLAGS = -O3 -DNDEBUG
  GRADE_LDFLAGS = -s
  LUA_CFLAGS = -O3
  LUA_LDFLAGS =
  LUAJIT_CFLAGS = -DLUAJIT_ENABLE_LUA52COMPAT
  LUAJIT_LDFLAGS =
endif

AM_INCLUDE_FLAGS = $(INCLUDE_OPT)$(BUILD_INCLUDE_DIR) $(INCLUDE_OPT)$(BUILD_LUAVM_INCLUDE_DIR) \
	$(INCLUDE_OPT)$(GLM_DIR)

AM_DEF_FLAGS=$(patsubst %,$(DEF_OPT)%,$(AM_DEFS))

AM_CFLAGS = $(AM_DEF_FLAGS) $(GRADE_CFLAGS) $(AM_INCLUDE_FLAGS) $(XCFLAGS) $(CFLAGS)
AM_LDFLAGS = $(GRADE_LDFLAGS) $(DEP_ALIBS) $(XLDFLAGS) $(LDFLAGS)

# Rules

default: $(AMULET)

$(AMULET): $(DEP_ALIBS) $(AM_OBJ_FILES) | $(BUILD_BIN_DIR)
	$(LINK) $(AM_OBJ_FILES) $(AM_LDFLAGS) -o $@
	ln -fs $@ `basename $@`
	@echo DONE

$(AM_OBJ_FILES): $(BUILD_STAGING_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp $(AM_H_FILES) | $(BUILD_STAGING_DIR)
	$(CPP) $(AM_CFLAGS) -c $< -o $@

$(SDL_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(SDL_DIR) && ./configure --disable-render --disable-loadso CC=$(CC) CXX=$(CPP) && $(MAKE) clean && $(MAKE)
	cp -r $(SDL_DIR)/include/* $(BUILD_INCLUDE_DIR)/
	cp $(SDL_DIR)/build/.libs/libSDL2.a $@

$(LUA_ALIB): | $(BUILD_LIB_DIR) $(BUILD_LUA_INCLUDE_DIR)
	cd $(LUA_DIR) && $(MAKE) clean $(LUA_TARGET) MYCFLAGS="$(LUA_CFLAGS)" MYLDFLAGS="$(LUA_LDFLAGS)"
	cp $(LUA_DIR)/src/*.h $(BUILD_LUA_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/liblua.a $@

$(LUAJIT_ALIB): | $(BUILD_LIB_DIR) $(BUILD_LUAJIT_INCLUDE_DIR)
	cd $(LUAJIT_DIR) && $(MAKE) clean all CFLAGS="$(LUAJIT_CFLAGS)" LDFLAGS="$(LUAJIT_LDFLAGS)"
	cp $(LUAJIT_DIR)/src/*.h $(BUILD_LUAJIT_INCLUDE_DIR)/
	cp $(LUAJIT_DIR)/src/libluajit.a $@

$(GLEW_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INCLUDE_DIR)
	cd $(GLEW_DIR) && $(MAKE) clean all
	cp -r $(GLEW_DIR)/include/* $(BUILD_INCLUDE_DIR)/
	cp $(GLEW_DIR)/lib/libGLEW.a $@

$(TARGETS): %:
	@$(MAKE) TARGET=$@

$(BUILD_DIRS): %:
	mkdir -p $@

# Embedded Lua code

EMBEDDED_LUA_FILES = $(wildcard src/*.lua)

src/am_embedded_lua.cpp: $(EMBEDDED_LUA_FILES)
	echo "#include \"amulet.h\"" > $@; \
	echo "am_embedded_lua_script am_embedded_lua_scripts[] = {" >> $@; \
	for f in `ls src/*.lua | sort`; do \
		name=`basename $$f`; \
		echo "{\""$$name"\", " >> $@; \
		cat $$f | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | sed 's/^/    "/' | sed 's/$$/\\n"/' >> $@; \
		echo "    }," >> $@; \
		echo >> $@; \
	done; \
	echo "{NULL, NULL}};" >> $@; \
	echo "" >> $@

# Cleanup

clean:
	rm -f $(BUILD_STAGING_DIR)/*
	rm -f $(BUILD_BIN_DIR)/*

clean-target:
	rm -rf $(BUILD_BASE_DIR)

clean-all: clean-tests
	rm -rf builds

# Tests

TIMEPROG = /usr/bin/time
TIMEFORMAT = "[%es %Mk]"

CPP_TESTS = $(patsubst tests/test_%.cpp,test_%,$(wildcard tests/*.cpp))
LUA_TESTS = $(patsubst tests/test_%.lua,test_%,$(wildcard tests/*.lua))

.PHONY: test
test: run_cpp_tests run_lua_tests

.PHONY: run_cpp_tests
run_cpp_tests:
	@echo Running C++ tests...
	@for f in $(CPP_TESTS); do \
	    fexe=tests/$$f$(EXE_EXT); \
	    fexp=tests/$$f.exp; \
	    fexp2=tests/$$f.exp2; \
	    fout=tests/$$f.out; \
	    fres=tests/$$f.res; \
	    ftime=tests/$$f.time; \
	    g++ -O0 -g tests/$$f.cpp -o $$fexe; \
	    $(TIMEPROG) -f $"$(TIMEFORMAT)$" -o $$ftime $$fexe > $$fout 2>&1 ; \
	    tdata=`cat $$ftime`; \
	    if ( diff -u $$fexp $$fout > $$fres ) || ( [ -e $$fexp2 ] && ( diff -u $$fexp2 $$fout > $$fres ) ); then \
		res=" passed "; \
	    else \
		res="*FAILED*"; \
	    fi; \
	    printf "%-30s%s       %s\n" "$$f" "$$res" "$$tdata"; \
	done
	@echo DONE

.PHONY: run_lua_tests
run_lua_tests: $(AMULET)
	@echo Running Lua tests...
	@for t in $(LUA_TESTS); do \
	    flua=tests/$$t.lua; \
	    fexp=tests/$$t.exp; \
	    fexp2=tests/$$t.exp2; \
	    fout=tests/$$t.out; \
	    fres=tests/$$t.res; \
	    ftime=tests/$$t.time; \
	    $(TIMEPROG) -f $"$(TIMEFORMAT)$" -o $$ftime $(AMULET) $$flua > $$fout 2>&1 ; \
	    tdata=`cat $$ftime`; \
	    if ( diff -u $$fexp $$fout > $$fres ) || ( [ -e $$fexp2 ] && ( diff -u $$fexp2 $$fout > $$fres ) ); then \
		res="  pass  "; \
	    else \
		res="**FAIL**"; \
	    fi; \
	    printf "%-30s%s       %s\n" "$$t" "$$res" "$$tdata"; \
	done
	@echo DONE

clean-tests:
	rm -f tests/*.out
	rm -f tests/*.res
	rm -f tests/*.err
	rm -f tests/*.time
	rm -f $(patsubst %,tests/%$(EXE_EXT),$(CPP_TESTS))

# Avoid setting options or variables in submakes,
# because setting TARGET messes up the SDL build.
MAKEOVERRIDES =
unexport

# Banner
ifeq (,$(findstring .,$(MAKECMDGOALS)))
  $(info ======== Amulet build settings ========)
  $(info TARGET_PLATFORM:    $(TARGET_PLATFORM))
  $(info HOST_PLATFORM:      $(HOST_PLATFORM))
  $(info GRADE:              $(GRADE))
  $(info CC:                 $(CC))
  $(info CPP:                $(CPP))
  $(info DEPS:               $(DEPS))
  $(info =======================================)
endif

# Tags
.PHONY: tags
tags:
	ctags `find $(SRC_DIR) -name "*.c"` `find $(SRC_DIR) -name "*.cpp"` \
		`find $(SRC_DIR) -name "*.h"` `find $(DEPS_DIR) -name "*.c" | grep -v iOS` \
		`find $(DEPS_DIR) -name "*.cpp"` `find $(DEPS_DIR) -name "*.h"`
