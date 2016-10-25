#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

INDIGO_VERSION := 2.0
INDIGO_ROOT := $(shell pwd)

ifeq ($(OS),Windows_NT)
	OS_DETECTED := Windows
else
	OS_DETECTED := $(shell uname -s)
	ARCH_DETECTED=$(shell uname -m)
	ifeq ($(ARCH_DETECTED),armv7l)
		ARCH_DETECTED=arm
	endif
	ifeq ($(ARCH_DETECTED),i686)
		ARCH_DETECTED=x86
	endif
	ifeq ($(ARCH_DETECTED),x86_64)
		ARCH_DETECTED=x64
	endif
endif

#---------------------------------------------------------------------
#
#	External helpers
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
	LIBATIK=indigo_drivers/ccd_atik/bin_externals/libatik/lib/macOS/libatik.a
endif
ifeq ($(OS_DETECTED),Linux)
	LIBATIK=indigo_drivers/ccd_atik/bin_externals/libatik/lib/Linux/$(ARCH_DETECTED)/libatik.a
endif

#---------------------------------------------------------------------
#
#	Darwin/macOS parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
	CC=gcc
	CFLAGS=-Iindigo -Iindigo_drivers -Iinclude -std=gnu11 -DINDIGO_MACOS
	LDFLAGS=-framework CoreFoundation -framework IOKit -lobjc
	LIBUSB=lib/libusb-1.0.a
	LIBHIDAPI=lib/libhidapi.a
	AR=ar
	ARFLAGS=-rv
	DEPENDENCIES=$(LIBUSB) $(LIBHIDAPI) lib/libatik.a
endif

#---------------------------------------------------------------------
#
#	Linux parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	CFLAGS=-Iindigo -Iindigo_drivers -Iinclude  -std=gnu11 -pthread -DINDIGO_LINUX
	LDFLAGS=-lm -lrt -lusb-1.0 -ludev
	LIBUSB=
	LIBHIDAPI=lib/libhidapi-hidraw.a
	AR=ar
	ARFLAGS=-rv
	DEPENDENCIES=$(LIBHIDAPI) lib/libatik.a
endif

#---------------------------------------------------------------------
#
#	Windows build parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Windows)
#	Windows - TBD
endif

.PHONY: init clean

all: init $(DEPENDENCIES) lib/libindigo.a drivers bin/test bin/client bin/server

#---------------------------------------------------------------------
#
#	Build libusb-1.0 for macOS
#
#---------------------------------------------------------------------

externals/libusb/configure: externals/libusb/configure.ac
	cd externals/libusb; autoreconf -i; cd ../..

externals/libusb/Makefile: externals/libusb/configure
	cd externals/libusb; ./configure --prefix=$(INDIGO_ROOT); cd ../..

lib/libusb-1.0.a: externals/libusb/Makefile
	cd externals/libusb; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libhidapi
#
#---------------------------------------------------------------------

externals/hidapi/configure: externals/hidapi/configure.ac
	cd externals/hidapi; ./bootstrap; cd ../..

externals/hidapi/Makefile: externals/hidapi/configure
	cd externals/hidapi; ./configure --prefix=$(INDIGO_ROOT); cd ../..

lib/libhidapi.a: externals/hidapi/Makefile
	cd externals/hidapi; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libdc1394
#
#---------------------------------------------------------------------

externals/libdc1394/configure: externals/libdc1394/configure.ac
	cd externals/libdc1394; autoreconf -i; cd ../..

externals/libdc1394/Makefile: externals/libdc1394/configure
	cd externals/libdc1394; ./configure --prefix=$(INDIGO_ROOT) CFLAGS="-Duint=unsigned" LIBUSB_CFLAGS="-I$(INDIGO_ROOT)/include/libusb-1.0" LIBUSB_LIBS="-L$(INDIGO_ROOT)/lib -lusb-1.0"; cd ../..

lib/libdc1394.a: externals/libdc1394/Makefile
	cd externals/libdc1394; make install; cd ../..

#---------------------------------------------------------------------
#
#	Install libatik
#
#---------------------------------------------------------------------

include/libatik/libatik.h:
	mkdir -p include/libatik
	cp indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik/libatik.h include/libatik

lib/libatik.a: include/libatik/libatik.h
	cp $(LIBATIK) lib

#---------------------------------------------------------------------
#
#	Initialize
#
#---------------------------------------------------------------------

init:
	$(info -------------------- $(OS_DETECTED) build --------------------)
	git submodule update --init --recursive
	mkdir -p bin
	mkdir -p lib
	mkdir -p include

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

lib/libindigo.a: $(addsuffix .o, $(basename $(wildcard indigo/*.c)))
	$(AR) $(ARFLAGS) $@ $^

#---------------------------------------------------------------------
#
#	Build drivers
#
#---------------------------------------------------------------------

drivers:\
	$(addprefix bin/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*))) \
	$(addsufix .a, $(addprefix lib/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addprefix bin/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*))) \
	$(addsufix .a, $(addprefix lib/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*))))

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
#	Build tests
#
#---------------------------------------------------------------------

bin/test: indigo_test/test.o lib/indigo_ccd_simulator.a lib/libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/client: indigo_test/client.o lib/libindigo.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/server: indigo_test/server.o $(wildcard lib/indigo_ccd_*.a) $(wildcard lib/indigo_wheel_*.a) lib/libindigo.a $(DEPENDENCIES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

rules:
ifeq ($(OS_DETECTED),Linux)
	sudo cp indigo_drivers/ccd_sx/indigo_ccd_sx.rules /lib/udev/rules.d/99-sx.rules
	sudo cp indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /lib/udev/rules.d/99-ssag.rules
	sudo cp indigo_drivers/ccd_asi/indigo_ccd_asi.rules /lib/udev/rules.d/99-asi.rules
	sudo udevadm control --reload-rules
endif

clean: init
	rm bin/*
	rm lib/libindigo.a indigo/*.o
	rm $(wildcard indigo_drivers/*/*.o)
	rm $(wildcard lib/indigo_ccd_*.a)
	rm $(wildcard lib/indigo_wheel_*.a)
