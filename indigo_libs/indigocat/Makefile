include ../../Makefile.inc

LIBINDIGOCAT_FILES = $(addsuffix .o, $(basename $(wildcard *.c)))

.PHONY: all clean

all:	libindigocat

install: libindigocat
	cp $(BUILD_LIB)/libindigocat.$(SOEXT) $(INSTALL_LIB)
	cp $(BUILD_LIB)/libindigocat.a $(INSTALL_LIB)
	install -d $(INSTALL_INCLUDE)/indigo/indigocat
	cp ../indigo/indigocat/*.h $(INSTALL_INCLUDE)/indigo/indigocat

uninstall:
	rm -f $(INSTALL_LIB)/libindigocat.$(SOEXT)
	rm -f $(INSTALL_LIB)/libindigocat.a
	rm -rf $(INSTALL_INCLUDE)/indigo/indigocat

libindigocat: $(BUILD_LIB)/libindigocat.a $(BUILD_LIB)/libindigocat.$(SOEXT)

$(BUILD_LIB)/libindigocat.a: $(LIBINDIGOCAT_FILES)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigocat.$(SOEXT): $(LIBINDIGOCAT_FILES)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -ldl -lusb-1.0 -lz -lstdc++

clean:
	rm -f *.o *.orig $(BUILD_LIB)/libindigocat.*
