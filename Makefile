include Common.mk

MAIN_TARGET = $(AMULET)

# Build settings

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

AM_DEFS = AM_$(shell echo $(TARGET_PLATFORM) | tr a-z A-Z) AM_$(shell echo $(GRADE) | tr a-z A-Z)

ifeq ($(LUAVM),luajit)
  BUILD_LUAVM_INCLUDE_DIR=$(BUILD_LUAJIT_INCLUDE_DIR)
  AM_DEFS += AM_LUAJIT
else
  BUILD_LUAVM_INCLUDE_DIR=$(BUILD_LUA_INCLUDE_DIR)
endif

AMULET = $(BUILD_BIN_DIR)/amulet$(EXE_EXT)

ifeq ($(TARGET_PLATFORM),html)
  AM_DEPS = lua
else
  AM_DEPS = $(LUAVM) sdl glew png z
endif

DEP_ALIBS = $(patsubst %,$(BUILD_LIB_DIR)/lib%$(ALIB_EXT),$(AM_DEPS))

SDL_ALIB = $(BUILD_LIB_DIR)/libsdl$(ALIB_EXT)
GLEW_ALIB = $(BUILD_LIB_DIR)/libglew$(ALIB_EXT)
LUA_ALIB = $(BUILD_LIB_DIR)/liblua$(ALIB_EXT)
LUAJIT_ALIB = $(BUILD_LIB_DIR)/libluajit$(ALIB_EXT)
LUAVM_ALIB = $(BUILD_LIB_DIR)/lib$(LUAVM)$(ALIB_EXT)
LIBPNG_ALIB = $(BUILD_LIB_DIR)/libpng$(ALIB_EXT)
ZLIB_ALIB = $(BUILD_LIB_DIR)/libz$(ALIB_EXT)

AM_CPP_FILES = $(sort $(wildcard $(SRC_DIR)/*.cpp) src/am_embedded_lua.cpp)
AM_H_FILES = $(wildcard $(SRC_DIR)/*.h)
AM_OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_OBJ_DIR)/%$(OBJ_EXT),$(AM_CPP_FILES))

AM_INCLUDE_FLAGS = $(INCLUDE_OPT)$(BUILD_INC_DIR) $(INCLUDE_OPT)$(BUILD_LUAVM_INCLUDE_DIR) \
	$(INCLUDE_OPT)$(GLM_DIR)

AM_DEF_FLAGS=$(patsubst %,$(DEF_OPT)%,$(AM_DEFS))

AM_CFLAGS = -Wall $(AM_DEF_FLAGS) $(COMMON_CFLAGS) $(AM_INCLUDE_FLAGS)
AM_LDFLAGS = $(GRADE_LDFLAGS) $(DEP_ALIBS) $(XLDFLAGS) $(LDFLAGS)

# Rules

default: $(AMULET)

ifeq ($(TARGET_PLATFORM),html)
$(AMULET): $(DEP_ALIBS) $(AM_OBJ_FILES) | $(BUILD_BIN_DIR)
	cp samples/dancing_triangles.lua main.lua
	$(LINK) --embed-file main.lua $(AM_OBJ_FILES) $(AM_LDFLAGS) -o $@
	rm main.lua
	@$(PRINT_BUILD_DONE_MSG)
else
$(AMULET): $(DEP_ALIBS) $(AM_OBJ_FILES) | $(BUILD_BIN_DIR)
	$(LINK) $(AM_OBJ_FILES) $(AM_LDFLAGS) -o $@
	ln -fs $@ `basename $@`
	@$(PRINT_BUILD_DONE_MSG)
endif

$(AM_OBJ_FILES): $(BUILD_OBJ_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp $(AM_H_FILES) | $(BUILD_OBJ_DIR)
	$(CPP) $(AM_CFLAGS) -c $< -o $@

$(SDL_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INC_DIR)
	cd $(SDL_DIR) && ./configure --disable-render --disable-loadso CC=$(CC) CXX=$(CPP) && $(MAKE) clean && $(MAKE)
	cp -r $(SDL_DIR)/include/* $(BUILD_INC_DIR)/
	cp $(SDL_DIR)/build/.libs/libSDL2.a $@

$(LUA_ALIB): | $(BUILD_LIB_DIR) $(BUILD_LUA_INCLUDE_DIR)
	cd $(LUA_DIR) && $(MAKE) clean $(LUA_TARGET) CC=$(CC) MYCFLAGS="$(LUA_CFLAGS)" MYLDFLAGS="$(LUA_LDFLAGS)"
	cp $(LUA_DIR)/src/*.h $(BUILD_LUA_INCLUDE_DIR)/
	cp $(LUA_DIR)/src/liblua.a $@

$(LUAJIT_ALIB): | $(BUILD_LIB_DIR) $(BUILD_LUAJIT_INCLUDE_DIR)
	cd $(LUAJIT_DIR) && $(MAKE) clean all CFLAGS="$(LUAJIT_CFLAGS)" LDFLAGS="$(LUAJIT_LDFLAGS)"
	cp $(LUAJIT_DIR)/src/*.h $(BUILD_LUAJIT_INCLUDE_DIR)/
	cp $(LUAJIT_DIR)/src/libluajit.a $@

$(GLEW_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INC_DIR)
	cd $(GLEW_DIR) && $(MAKE) clean all
	cp -r $(GLEW_DIR)/include/* $(BUILD_INC_DIR)/
	cp $(GLEW_DIR)/lib/libGLEW.a $@

$(LIBPNG_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INC_DIR)
	cd $(LIBPNG_DIR) && $(MAKE) clean all
	cp $(LIBPNG_DIR)/libpng$(ALIB_EXT) $@
	cp $(LIBPNG_DIR)/png.h $(BUILD_INC_DIR)/
	cp $(LIBPNG_DIR)/pngconf.h $(BUILD_INC_DIR)/

$(ZLIB_ALIB): | $(BUILD_LIB_DIR) $(BUILD_INC_DIR)
	cd $(ZLIB_DIR) && $(MAKE) clean all
	cp $(ZLIB_DIR)/libz$(ALIB_EXT) $@
	cp $(ZLIB_DIR)/zlib.h $(BUILD_INC_DIR)/
	cp $(ZLIB_DIR)/zconf.h $(BUILD_INC_DIR)/

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
	rm -f $(BUILD_OBJ_DIR)/*
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
PRINT_BUILD_DONE_MSG = \
  echo ======== Amulet build successful ======; \
  echo TARGET_PLATFORM:    $(TARGET_PLATFORM); \
  echo HOST_PLATFORM:      $(HOST_PLATFORM); \
  echo GRADE:              $(GRADE); \
  echo CC:                 $(CC); \
  echo CPP:                $(CPP); \
  echo DEPS:               $(AM_DEPS); \
  echo =======================================;

# Tags
.PHONY: tags
tags:
	ctags `find $(SRC_DIR) -name "*.c"` `find $(SRC_DIR) -name "*.cpp"` \
		`find $(SRC_DIR) -name "*.h"` `find $(DEPS_DIR) -name "*.c" | grep -v iOS` \
		`find $(DEPS_DIR) -name "*.cpp"` `find $(DEPS_DIR) -name "*.h"`
