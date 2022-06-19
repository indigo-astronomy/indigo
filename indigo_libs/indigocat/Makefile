include ../../Makefile.inc

LIBINDIGOCAT_FILES = $(addsuffix .o, $(basename $(wildcard *.c)))

.PHONY: all clean

all:	libindigocat

libindigocat: $(BUILD_LIB)/libindigocat.a $(BUILD_LIB)/libindigocat.$(SOEXT)

$(BUILD_LIB)/libindigocat.a: $(LIBINDIGOCAT_FILES)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigocat.$(SOEXT): $(LIBINDIGOCAT_FILES)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -ldl -lusb-1.0 -lz -lstdc++

clean:
	rm -f *.o *.orig $(BUILD_LIB)/libindigocat.*
