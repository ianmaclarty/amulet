include ../../Common.mk

MAIN_TARGET = libvorbis$(ALIB_EXT)

all: $(MAIN_TARGET)

CFILES := $(wildcard *.c)
OFILES := $(patsubst %.c,%$(OBJ_EXT),$(CFILES))

#define STB_VORBIS_MAX_CHANNELS AM_MAX_CHANNELS
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#include "../deps/vorbis/stb_vorbis.c"

CFLAGS := $(COMMON_CFLAGS) -I.

$(OFILES): %$(OBJ_EXT): %.c
	$(CC) $(CFLAGS) $(NOLINK_OPT) $<

$(MAIN_TARGET): $(OFILES)
	"$(AR)" $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f *.o *.a *.obj *.lib
