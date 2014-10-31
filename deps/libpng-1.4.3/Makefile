include ../../Common.mk

MAIN_TARGET = libpng$(ALIB_EXT)

all: $(MAIN_TARGET)

CFILES := $(wildcard *.c)
OFILES := $(patsubst %.c,%$(OBJ_EXT),$(CFILES))

CFLAGS := $(AM_CFLAGS) -I. -I../$(ZLIB)

$(OFILES): %$(OBJ_EXT): %.c
	$(CC) $(CFLAGS) -c $<

$(MAIN_TARGET): $(OFILES)
	$(AR) $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f *.o *.a *.obj *.lib
