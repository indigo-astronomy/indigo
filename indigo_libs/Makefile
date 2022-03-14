#---------------------------------------------------------------------
#
# Copyright (c) 2018 CloudMakers, s. r. o.
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#---------------------------------------------------------------------

include ../Makefile.inc

ifeq ($(OS_DETECTED),Darwin)
	LIBUSB=$(BUILD_LIB)/libusb-1.0.dylib
	LIBHIDAPI=$(BUILD_LIB)/libhidapi.a
	FORCE_ALL_ON=-Wl,-force_load
	FORCE_ALL_OFF=
endif

ifeq ($(OS_DETECTED),Linux)
	LIBUSB=
	LIBHIDAPI=$(BUILD_LIB)/libhidapi-libusb.a
	FORCE_ALL_ON=-Wl,--whole-archive
	FORCE_ALL_OFF=-Wl,--no-whole-archive
endif

BIN_EXTERNALS = $(INDIGO_ROOT)/bin_externals

ifeq ($(INDIGO_CUDA),)
	LIBINDIGO_FILES = $(addsuffix .o, $(basename $(wildcard *.c)))
else
	LIBINDIGO_FILES = $(addsuffix .o, $(basename $(wildcard *.c))) $(addsuffix .o, $(basename $(wildcard *.cu)))
endif

%.o: %.cu
	nvcc $(NVCCFLAGS) -c $< -o $@

.PHONY: all clean

all:	status libindigo

status:
	@printf "\nindigo_libs -------------------------\n\n"
	@printf "libusb-1.0: $(LIBUSB)\n"
	@printf "libhidapi: $(LIBHIDAPI)\n\n"

install: status libindigo
	cp $(BUILD_LIB)/libindigo.$(SOEXT) $(INSTALL_LIB)
	cp $(BUILD_LIB)/libindigo.a $(INSTALL_LIB)
ifeq ($(OS_DETECTED),Darwin)
	cp $(LIBUSB) $(INSTALL_LIB)
endif
	install -d $(INSTALL_INCLUDE)/indigo
	cp indigo/*.h $(INSTALL_INCLUDE)/indigo

uninstall: status
	rm -f $(INSTALL_LIB)/libindigo.$(SOEXT)
	rm -f $(INSTALL_LIB)/libindigo.a
ifeq ($(OS_DETECTED),Darwin)
	rm -f $(INSTALL_LIB)/libusb-1.0.dylib
endif
	rm -rf $(INSTALL_INCLUDE)/indigo

clean: status
	rm -f *.o *.orig $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libindigo.$(SOEXT) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libtiff.a $(BUILD_LIB)/libtiffxx.a $(BUILD_LIB)/libnovas.a $(BUILD_LIB)/libusb-1.0.dylib $(LIBHIDAPI) $(BUILD_LIB)/libftd2xx.a

clean-all: status
	git clean -dfx

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

libindigo: version libusb libhidapi libjpeg libtiff libnovas libftd2xx $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libindigo.$(SOEXT)

$(BUILD_LIB)/libindigo.a: $(LIBINDIGO_FILES)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigo.$(SOEXT): $(LIBINDIGO_FILES) $(BUILD_LIB)/libnovas.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(FORCE_ALL_ON) $(BUILD_LIB)/libjpeg.a $(FORCE_ALL_OFF) $(BUILD_LIB)/libtiff.a $(BUILD_LIB)/libtiffxx.a $(FORCE_ALL_ON) $(LIBHIDAPI) $(FORCE_ALL_OFF) -ldl -lusb-1.0 -lz

#---------------------------------------------------------------------
#
#	Make version dependent files
#
#---------------------------------------------------------------------

version: indigo_config.h

indigo_config.h:	../Makefile.inc
	cp indigo/indigo_config.h indigo/indigo_config.h.orig
	sed 's/INDIGO_BUILD.*/INDIGO_BUILD "$(INDIGO_BUILD)"/' indigo/indigo_config.h.orig >indigo/indigo_config.h

#---------------------------------------------------------------------
#
#	Build libjpeg
#
#---------------------------------------------------------------------

libjpeg: $(BUILD_LIB)/libjpeg.a

$(BUILD_LIB)/libjpeg.a:
	export CC=$(CC); export CXX=$(CC); export CFLAGS="$(CFLAGS)"; export CXXFLAGS="$(CXXFLAGS)"; export LDFLAGS="$(LDFLAGS)"; cd externals/libjpeg; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB)  --enable-shared=no --enable-static=yes CFLAGS="$(CFLAGS)"; make install

#---------------------------------------------------------------------
#
#	Build libtiff
#
#---------------------------------------------------------------------

libtiff: $(BUILD_LIB)/libtiff.a

$(BUILD_LIB)/libtiff.a:
	export CC=$(CC); export CXX=$(CC); export CFLAGS="$(CFLAGS)"; export CXXFLAGS="$(CXXFLAGS)"; export LDFLAGS="$(LDFLAGS)"; cd externals/libtiff; ./configure --disable-ccitt --disable-packbits --disable-thunder --disable-next --disable-logluv --disable-mdi --disable-zlib --disable-pixarlog --disable-jpeg --disable-old-jpeg --disable-jbig --disable-lzma --disable-zstd --disable-webp --enable-shared=no --enable-static=yes --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB) CFLAGS="$(CFLAGS)"; make install

#---------------------------------------------------------------------
#
#	Build libnovas
#
#---------------------------------------------------------------------

libnovas: $(BUILD_LIB)/libnovas.a

$(BUILD_LIB)/libnovas.a: externals/novas/novas.c externals/novas/eph_manager.c externals/novas/novascon.c externals/novas/nutation.c externals/novas/readeph0.c externals/novas/solsys1.c
	cd externals/novas; rm -f *.o; $(CC) -c $(CFLAGS) novas.c eph_manager.c novascon.c nutation.c readeph0.c solsys1.c; $(AR) $(ARFLAGS) $@ *.o; install *.h $(BUILD_INCLUDE);

#---------------------------------------------------------------------
#
#	Build libusb-1.0 for macOS
#
#---------------------------------------------------------------------

libusb: $(LIBUSB)

$(BUILD_LIB)/libusb-1.0.dylib:
	export CC=$(CC); export CXX=$(CC); export CFLAGS="$(CFLAGS)"; export CXXFLAGS="$(CXXFLAGS)"; export LDFLAGS="$(LDFLAGS)"; cd externals/libusb; autoreconf -fiv; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB) --enable-shared=yes --enable-static=no CFLAGS="$(CFLAGS)" --with-pic; make; make install

#---------------------------------------------------------------------
#
#	Build libhidapi
#
#---------------------------------------------------------------------

libhidapi: $(LIBHIDAPI)

$(LIBHIDAPI):
	export CC=$(CC); export CXX=$(CC); export CFLAGS="$(CFLAGS)"; export CXXFLAGS="$(CXXFLAGS)"; export LDFLAGS="$(LDFLAGS) -lusb-1.0"; cd externals/hidapi; autoreconf -fiv; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB) --enable-shared=no --enable-static=yes CFLAGS="$(CFLAGS)" --with-pic; make; make install

#---------------------------------------------------------------------
#
#	Install libftd2xx
#
#---------------------------------------------------------------------

libftd2xx: $(BUILD_LIB)/libftd2xx.a

$(BUILD_LIB)/libftd2xx.a:
	install -d $(BUILD_LIB)
ifeq ($(OS_DETECTED),Darwin)
	hdiutil attach -noverify -noautoopen $(BIN_EXTERNALS)/D2XX1.4.4.dmg
	cp /Volumes/release/D2XX/ftd2xx.h /Volumes/release/D2XX/WinTypes.h $(BUILD_INCLUDE)
	cp /Volumes/release/D2XX/libftd2xx.a $(BUILD_LIB)
	hdiutil detach /Volumes/release
	rm -rf /tmp/D2XX1.4.4.dmg
endif
ifeq ($(OS_DETECTED),Linux)
ifeq ($(ARCH_DETECTED),arm)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-arm-v6-hf-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-arm-v6-hf-1.4.6.tgz /tmp/release
endif
ifeq ($(ARCH_DETECTED),arm64)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-arm-v8-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-arm-v8-1.4.6.tgz /tmp/release
endif
ifeq ($(ARCH_DETECTED),x86)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-i386-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-i386-1.4.6.tgz /tmp/release
endif
ifeq ($(ARCH_DETECTED),x64)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-x86_64-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-x86_64-1.4.6.tgz /tmp/release
endif
endif
	ar -d $(BUILD_LIB)/libftd2xx.a core.o descriptor.o io.o hotplug.o sync.o darwin_usb.o poll_posix.o threads_posix.o 
