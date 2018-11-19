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
endif

ifeq ($(OS_DETECTED),Linux)
	LIBUSB=
	LIBHIDAPI=$(BUILD_LIB)/libhidapi-hidraw.a
endif

.PHONY: all clean

all:	status libindigo

status:
	@printf "\nindigo_libs -------------------------\n\n"
	@printf "libusb-1.0: $(LIBUSB)\n"
	@printf "libhidapi: $(LIBHIDAPI)\n\n"

install: status libindigo
	cp $(BUILD_LIB)/libindigo.$(SOEXT) $(INSTALL_LIB)
ifeq ($(OS_DETECTED),Darwin)
	cp $(LIBUSB) $(INSTALL_LIB)
endif

uninstall: status
	rm -f $(INSTALL_LIB)/libindigo.$(SOEXT)
ifeq ($(OS_DETECTED),Darwin)
	rm -f $(INSTALL_LIB)/libusb-1.0.dylib
endif

clean: status
	rm -f *.o *.orig $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libindigo.$(SOEXT)

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

libindigo: version libusb-1.0 libhidapi libjpeg libnovas $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libindigo.$(SOEXT)

$(BUILD_LIB)/libindigo.a: $(addsuffix .o, $(basename $(wildcard *.c)))
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigo.$(SOEXT): $(addsuffix .o, $(basename $(wildcard *.c)))
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libnovas.a -force_load $(LIBHIDAPI)

#---------------------------------------------------------------------
#
#	Make version dependent files
#
#---------------------------------------------------------------------

version: config.h

config.h:	../Makefile.inc
	cp indigo_config.h indigo_config.h.orig
	sed 's/INDIGO_BUILD.*/INDIGO_BUILD $(INDIGO_BUILD)/' indigo_config.h.orig >indigo_config.h

#---------------------------------------------------------------------
#
#	Build libjpeg
#
#---------------------------------------------------------------------

libjpeg: $(BUILD_LIB)/libjpeg.a

$(BUILD_LIB)/libjpeg.a:
	git clone https://github.com/indigo-astronomy/libjpeg.git
	cd libjpeg; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB)  --enable-shared=no --enable-static=yes CFLAGS="$(CFLAGS)"; make install
	rm -rf libjpeg

#---------------------------------------------------------------------
#
#	Build libnovas
#
#---------------------------------------------------------------------

libnovas: $(BUILD_LIB)/libnovas.a

$(BUILD_LIB)/libnovas.a:
	git clone https://github.com/indigo-astronomy/novas.git
	cd novas; $(CC) -c $(CFLAGS) novas.c eph_manager.c novascon.c nutation.c readeph0.c solsys1.c solsys3.c; $(AR) $(ARFLAGS) $@ *.o; install *.h $(BUILD_INCLUDE); install JPLEPH.421 $(BUILD_LIB)
	rm -rf novas

#---------------------------------------------------------------------
#
#	Build libusb-1.0 for macOS
#
#---------------------------------------------------------------------

libusb-1.0: $(LIBUSB)

$(BUILD_LIB)/libusb-1.0.dylib:
	git clone https://github.com/indigo-astronomy/libusb.git
	cd libusb; autoreconf -fiv; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB) --enable-shared=yes --enable-static=no CFLAGS="$(CFLAGS)" --with-pic; make; make install
	rm -rf libusb

#---------------------------------------------------------------------
#
#	Build libhidapi
#
#---------------------------------------------------------------------

libhidapi: $(LIBHIDAPI)

$(LIBHIDAPI):
	git clone https://github.com/indigo-astronomy/hidapi.git
	cd hidapi; autoreconf -fiv; ./configure --prefix=$(BUILD_ROOT) --libdir=$(BUILD_LIB) --enable-shared=no --enable-static=yes CFLAGS="$(CFLAGS)" --with-pic; make; make install
	rm -rf hidapi
