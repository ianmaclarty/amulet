include ../../Common.mk

all: libstb$(ALIB_EXT)

C_FILES:=$(wildcard *.c)
O_FILES:=$(patsubst %.c,%$(OBJ_EXT),$(C_FILES))

CFLAGS = $(COMMON_CFLAGS) -I. -DSTB_VORBIS_NO_STDIO -DSTBI_NO_STDIO

$(O_FILES): %$(OBJ_EXT): %.c
	$(CC) $(CFLAGS) $(NOLINK_OPT) $< $(OBJ_OUT_OPT)$@

libstb$(ALIB_EXT): $(O_FILES)
	$(AR) $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f *.o *.a *.lib *.obj *.o-arm* *.o-sim*
