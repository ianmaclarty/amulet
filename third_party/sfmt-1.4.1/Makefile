include ../../Common.mk

all: libsfmt$(ALIB_EXT)

C_FILES:=$(wildcard *.c)
O_FILES:=$(patsubst %.c,%$(OBJ_EXT),$(C_FILES))

CFLAGS = $(COMMON_CFLAGS) -I. -DSFMT_MEXP=19937

$(O_FILES): %$(OBJ_EXT): %.c
	$(CC) $(CFLAGS) $(NOLINK_OPT) $< $(OBJ_OUT_OPT)$@

libsfmt$(ALIB_EXT): $(O_FILES)
	$(AR) $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f *.o *.a *.lib *.obj *.o-arm* *.o-sim*
