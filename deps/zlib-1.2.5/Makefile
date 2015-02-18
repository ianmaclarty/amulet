include ../../Common.mk

MAIN_TARGET = libz$(ALIB_EXT)

all: $(MAIN_TARGET)

CFILES := $(wildcard *.c)
OFILES := $(patsubst %.c,%$(OBJ_EXT),$(CFILES))

CFLAGS := $(COMMON_CFLAGS) -I. -I../$(ZLIB)

$(OFILES): %$(OBJ_EXT): %.c
	$(CC) $(CFLAGS) $(NOLINK_OPT) $<

$(MAIN_TARGET): $(OFILES)
	$(AR) $(AR_OPTS) $(AR_OUT_OPT)$@ $^

.PHONY: clean
clean:
	rm -f *.o *.a *.obj *.lib
