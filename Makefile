#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

INDIGO_VERSION := 2.0
INDIGO_BUILD := 0
INDIGO_ROOT := $(shell pwd)

ENABLE_STATIC=yes
ENABLE_SHARED=yes

ifeq ($(OS),Windows_NT)
	OS_DETECTED := Windows
else
	OS_DETECTED := $(shell uname -s)
	ARCH_DETECTED=$(shell uname -m)
	ifeq ($(ARCH_DETECTED),armv7l)
		ARCH_DETECTED=arm
		DEBIAN_ARCH=arm
	endif
	ifeq ($(ARCH_DETECTED),i686)
		ARCH_DETECTED=x86
		DEBIAN_ARCH=i386
	endif
	ifeq ($(ARCH_DETECTED),x86_64)
		ARCH_DETECTED=x64
		DEBIAN_ARCH=amd64
	endif
endif

#---------------------------------------------------------------------
#
#	External helpers
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
	LIBATIK=indigo_drivers/ccd_atik/bin_externals/libatik/lib/macOS/libatik.a
	LIBFCUSB=indigo_drivers/focuser_fcusb/bin_externals/libfcusb/lib/macOS/libfcusb.a
	PACKAGE_NAME=indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)
	PACKAGE_TYPE=dmg
endif
ifeq ($(OS_DETECTED),Linux)
	LIBATIK=indigo_drivers/ccd_atik/bin_externals/libatik/lib/Linux/$(ARCH_DETECTED)/libatik.a
	LIBFCUSB=indigo_drivers/focuser_fcusb/bin_externals/libfcusb/lib/Linux/$(ARCH_DETECTED)/libfcusb.a
	PACKAGE_NAME=indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-$(DEBIAN_ARCH)
	PACKAGE_TYPE=deb
endif

#---------------------------------------------------------------------
#
#	Darwin/macOS parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
	CC=gcc
	CFLAGS=-Iindigo_libs -Iindigo_drivers -Iinclude -std=gnu11 -DINDIGO_MACOS
	LDFLAGS=-framework CoreFoundation -framework IOKit -lobjc
	LIBUSB=lib/libusb-1.0.a
	LIBHIDAPI=lib/libhidapi.a
	AR=ar
	ARFLAGS=-rv
	DEPENDENCIES=$(LIBUSB) $(LIBHIDAPI) lib/libatik.a lib/libfcusb.a
endif

#---------------------------------------------------------------------
#
#	Linux parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	CFLAGS=-Iindigo_libs -Iindigo_drivers -Iinclude  -std=gnu11 -pthread -DINDIGO_LINUX
	LDFLAGS=-lm -lrt -lusb-1.0 -ludev
	LIBUSB=
	LIBHIDAPI=lib/libhidapi-hidraw.a
	AR=ar
	ARFLAGS=-rv
	DEPENDENCIES=$(LIBHIDAPI) lib/libatik.a lib/libfcusb.a
endif

#---------------------------------------------------------------------
#
#	Windows build parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Windows)
#	Windows - TBD
endif


#---------------------------------------------------------------------
#
#	Driver targets
#
#---------------------------------------------------------------------

DRIVERS=\
	$(addprefix bin/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*))) \
	$(addprefix bin/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*))) \
	$(addprefix bin/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))

DRIVER_LIBS=\
	$(addsuffix .a, $(addprefix lib/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .a, $(addprefix lib/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .a, $(addprefix lib/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*))))


.PHONY: init clean

#---------------------------------------------------------------------
#
#	Default target
#
#---------------------------------------------------------------------

all: init $(DEPENDENCIES) lib/libindigo.a drivers bin/test bin/client bin/indigo_server

#---------------------------------------------------------------------
#
#	Build libusb-1.0 for macOS
#
#---------------------------------------------------------------------

externals/libusb/configure: externals/libusb/configure.ac
	cd externals/libusb; autoreconf -i; cd ../..

externals/libusb/Makefile: externals/libusb/configure
	cd externals/libusb; ./configure --prefix=$(INDIGO_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC); cd ../..

$(LIBUSB): externals/libusb/Makefile
	cd externals/libusb; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libhidapi
#
#---------------------------------------------------------------------

externals/hidapi/configure: externals/hidapi/configure.ac
	cd externals/hidapi; ./bootstrap; cd ../..

externals/hidapi/Makefile: externals/hidapi/configure
	cd externals/hidapi; ./configure --prefix=$(INDIGO_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC); cd ../..

$(LIBHIDAPI): externals/hidapi/Makefile
	cd externals/hidapi; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libdc1394
#
#---------------------------------------------------------------------

externals/libdc1394/configure: externals/libdc1394/configure.ac
	cd externals/libdc1394; autoreconf -i; cd ../..

externals/libdc1394/Makefile: externals/libdc1394/configure
	cd externals/libdc1394; ./configure --prefix=$(INDIGO_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="-Duint=unsigned" LIBUSB_CFLAGS="-I$(INDIGO_ROOT)/include/libusb-1.0" LIBUSB_LIBS="-L$(INDIGO_ROOT)/lib -lusb-1.0"; cd ../..

lib/libdc1394.a: externals/libdc1394/Makefile
	cd externals/libdc1394; make install; cd ../..

#---------------------------------------------------------------------
#
#	Install libatik
#
#---------------------------------------------------------------------

include/libatik/libatik.h: indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik/libatik.h
	install -d include/libatik
	cp indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik/libatik.h include/libatik

lib/libatik.a: include/libatik/libatik.h
	install -d lib
	cp $(LIBATIK) lib

#---------------------------------------------------------------------
#
#	Install libfcusb
#
#---------------------------------------------------------------------

include/libfcusb/libfcusb.h: indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb/libfcusb.h
	install -d include/libfcusb
	cp indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb/libfcusb.h include/libfcusb

lib/libfcusb.a: include/libfcusb/libfcusb.h
	install -d lib
	cp $(LIBFCUSB) lib

#---------------------------------------------------------------------
#
#	Initialize
#
#---------------------------------------------------------------------

init:
	$(info -------------------- $(OS_DETECTED) build --------------------)
	$(info drivers: $(notdir $(DRIVERS)))
	git submodule update --init --recursive
	install -d bin
	install -d lib
	install -d include

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

lib/libindigo.a: $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c)))
	$(AR) $(ARFLAGS) $@ $^

#---------------------------------------------------------------------
#
#	Build drivers
#
#---------------------------------------------------------------------

drivers: $(DRIVER_LIBS) $(DRIVERS)

#---------------------------------------------------------------------
#
#	Build CCD simulator driver
#
#---------------------------------------------------------------------

lib/indigo_ccd_simulator.a: indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_ccd_simulator: indigo_drivers/ccd_simulator/indigo_ccd_simulator_main.o lib/indigo_ccd_simulator.a lib/libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build SX CCD driver
#
#---------------------------------------------------------------------

lib/indigo_ccd_sx.a: indigo_drivers/ccd_sx/indigo_ccd_sx.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_ccd_sx: indigo_drivers/ccd_sx/indigo_ccd_sx_main.o lib/indigo_ccd_sx.a lib/libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build SSAG/QHY5 CCD driver
#
#---------------------------------------------------------------------

lib/indigo_ccd_ssag.a: indigo_drivers/ccd_ssag/indigo_ccd_ssag.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_ccd_ssag: indigo_drivers/ccd_ssag/indigo_ccd_ssag_main.o lib/indigo_ccd_ssag.a lib/libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build ASI CCD driver
#
#---------------------------------------------------------------------

lib/indigo_ccd_asi.a: indigo_drivers/ccd_asi/indigo_ccd_asi.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_ccd_asi: indigo_drivers/ccd_asi/indigo_ccd_asi_main.o lib/indigo_ccd_asi.a lib/libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build ATIK CCD driver
#
#---------------------------------------------------------------------

lib/indigo_ccd_atik.a: indigo_drivers/ccd_atik/indigo_ccd_atik.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_ccd_atik: indigo_drivers/ccd_atik/indigo_ccd_atik_main.o lib/indigo_ccd_atik.a lib/libindigo.a  lib/libatik.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build SX filter wheel driver
#
#---------------------------------------------------------------------

lib/indigo_wheel_sx.a: indigo_drivers/wheel_sx/indigo_wheel_sx.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_wheel_sx: indigo_drivers/wheel_sx/indigo_wheel_sx_main.o lib/indigo_wheel_sx.a lib/libindigo.a $(LIBUSB) $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build Shoestring FCUSB focuser driver
#
#---------------------------------------------------------------------

lib/indigo_focuser_fcusb.a: indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.o
	$(AR) $(ARFLAGS) $@ $^

bin/indigo_focuser_fcusb: indigo_drivers/focuser_fcusb/indigo_focuser_fcusb_main.o lib/indigo_focuser_fcusb.a lib/libindigo.a lib/libfcusb.a $(LIBUSB) $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build tests
#
#---------------------------------------------------------------------

bin/test: indigo_test/test.o lib/indigo_ccd_simulator.a lib/libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/client: indigo_test/client.o lib/libindigo.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/indigo_server: indigo_libs/indigo_server.o $(DRIVER_LIBS) lib/libindigo.a $(DEPENDENCIES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Install rules
#
#---------------------------------------------------------------------

rules:
	sudo cp indigo_drivers/ccd_sx/indigo_ccd_sx.rules /lib/udev/rules.d/99-sx.rules
	sudo cp indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /lib/udev/rules.d/99-ssag.rules
	sudo cp indigo_drivers/ccd_asi/indigo_ccd_asi.rules /lib/udev/rules.d/99-asi.rules
	sudo udevadm control --reload-rules

#---------------------------------------------------------------------
#
#	Build packages
#
#---------------------------------------------------------------------


package: $(PACKAGE_NAME).$(PACKAGE_TYPE)

$(PACKAGE_NAME).deb: all
	install -d /tmp/$(PACKAGE_NAME)/usr/local/bin
	install bin/indigo_server /tmp/$(PACKAGE_NAME)/usr/local/bin
	install $(DRIVERS) /tmp/$(PACKAGE_NAME)/usr/local/bin
	install -d /tmp/$(PACKAGE_NAME)/usr/local/lib
	install $(DRIVER_LIBS) /tmp/$(PACKAGE_NAME)/usr/local/lib
	install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_sx.rules
	install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_ssag.rules
	install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules debian/lib/udev/rules.d/99-indigo_ccd_asi.rules
	cp -r share /tmp/$(PACKAGE_NAME)
	install -d /tmp/$(PACKAGE_NAME)/DEBIAN
	printf "Package: indigo\nVersion: $(INDIGO_VERSION)-$(INDIGO_BUILD)\nPriority: optional\nArchitecture: $(DEBIAN_ARCH)\nMaintainer: CloudMakers, s. r. o.\nDepends: libusb-1.0-0, libgudev-1.0-0\nDescription: INDIGO Server\n" > /tmp/$(PACKAGE_NAME)/DEBIAN/control
	cat /tmp/indigo-2.0-0-i386/DEBIAN/control
	sudo chown root /tmp/$(PACKAGE_NAME)
	dpkg --build /tmp/$(PACKAGE_NAME)
	mv /tmp/$(PACKAGE_NAME).deb .
	sudo rm -rf /tmp/$(PACKAGE_NAME)

#---------------------------------------------------------------------
#
#	Clean
#
#---------------------------------------------------------------------

clean: init
	rm -f bin/*
	rm -rf lib/*
	rm -f indigo_libs/*.o
	rm -f $(wildcard indigo_drivers/*/*.o)
	rm -f $(wildcard indigo_test/*.o)
