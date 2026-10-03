include ../../Makefile.inc

LIBINDIGOCAT_FILES = $(addsuffix .o, $(basename $(wildcard *.c)))
LIBINDIGOCAT_DEPS = $(LIBINDIGOCAT_FILES:.o=.d)

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

# -MD records the included headers in a .d file next to each object, so a header change rebuilds
# the objects too (-MMD would skip them, the indigo headers come via -isystem). A missing .d (an
# object built before this rule) forces one rebuild. The touch keeps the object newer than its .d.
%.o: %.c %.d
	$(CC) $(CFLAGS) -MD -MP -c $< -o $@
	@touch $@

$(LIBINDIGOCAT_DEPS):

-include $(LIBINDIGOCAT_DEPS)

clean:
	rm -f *.o *.d *.orig $(BUILD_LIB)/libindigocat.*
