include ../../Common.mk

MAIN_TARGET = libangle$(ALIB_EXT)

all: $(MAIN_TARGET)

ifdef WINDOWS
OSSOURCE_SRC_FILE=src/compiler/translator/ossource_win.cpp
else
OSSOURCE_SRC_FILE=src/compiler/translator/ossource_posix.cpp
endif

CPPFILES := \
	$(shell find src/common -name "*.cpp") \
	$(shell find src/third_party -name "*.cpp") \
	$(shell find src/compiler -name "*.cpp" | grep -v ossource) \
	$(OSSOURCE_SRC_FILE)

OFILES := $(patsubst %.cpp,%$(OBJ_EXT),$(CPPFILES))

CFLAGS := $(COMMON_CFLAGS) -Iinclude -Isrc

$(OFILES): %$(OBJ_EXT): %.cpp
	$(CPP) $(CFLAGS) $(NOLINK_OPT) $< $(OBJ_OUT_OPT)$@

$(MAIN_TARGET): $(OFILES)
	$(AR) $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f $(MAIN_TARGET) $(OFILES)
