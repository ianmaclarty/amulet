include ../../Common.mk

all: libglslopt$(ALIB_EXT)

CPP_FILES:=$(shell find src/glsl -name "*.cpp" | grep -v "src/glsl/main\.cpp")
O_CPP_FILES:=$(patsubst %.cpp,%$(OBJ_EXT),$(CPP_FILES))
C_FILES:=$(shell find src -name "*.c")
O_C_FILES:=$(patsubst %.c,%$(OBJ_EXT),$(C_FILES))

CPPFLAGS = $(COMMON_CFLAGS) -Iinclude -Isrc/mesa -Isrc

$(O_CPP_FILES): %$(OBJ_EXT): %.cpp
	$(CPP) $(CPPFLAGS) $(NOLINK_OPT) $< $(OBJ_OUT_OPT)$@

$(O_C_FILES): %$(OBJ_EXT): %.c
	$(CC) $(CPPFLAGS) $(NOLINK_OPT) $< $(OBJ_OUT_OPT)$@

libglslopt$(ALIB_EXT): $(O_C_FILES) $(O_CPP_FILES) 
	$(AR) $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f $(O_CPP_FILES) $(O_C_FILES)
	rm -f *.a
