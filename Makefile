ifeq ($(OS),Windows_NT)
OS_detected := Windows
else
OS_detected := $(shell uname -s)
endif

ifeq ($(OS_detected),Windows)
#	Windows - TBD
endif

ifeq ($(OS_detected),Darwin)
CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iexternals/libusb/libusb -Iexternals/hidapi -std=gnu11 -DINDIGO_DARWIN
LDFLAGS=-framework CoreFoundation -framework IOKit
LIBUSB=libusb.a
HIDAPI=externals/hidapi/mac/hid.o
AR=ar
ARFLAGS=-rv
endif

ifeq ($(OS_detected),Linux)
CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iexternals/hidapi -std=gnu11 -pthread -DINDIGO_LINUX
LDFLAGS=-lm -lrt -lusb-1.0
LIBUSB=
HIDAPI=externals/hidapi/linux/hid.o
AR=ar
ARFLAGS=-rv
endif

ifeq ($(OS_detected),FreeBSD)
CC=cc
CFLAGS=-Iindigo_bus -Iindigo_drivers -Iexternals/hidapi -std=gnu11 -pthread -DINDIGO_FREEBSD
LDFLAGS=-lm -lrt -lusb
LIBUSB=
HIDAPI=
AR=ar
ARFLAGS=-rv
endif

.PHONY: init clean

all: init test client server drivers

drivers:\
	indigo_ccd_simulator\
	indigo_ccd_sx\
	indigo_ccd_ssag\
	indigo_ccd_asi

init:
	$(info -------------------- $(OS_detected) build --------------------)
ifeq ($(OS_detected),Darwin)
	printf "#define DEFAULT_VISIBILITY\n#define ENABLE_LOGGING 1\n#define HAVE_GETTIMEOFDAY 1\n#define HAVE_POLL_H 1\n#define HAVE_SYS_TIME_H 1\n#define OS_DARWIN 1\n#define POLL_NFDS_TYPE nfds_t\n#define THREADS_POSIX 1\n#define _GNU_SOURCE 1\n" >externals/libusb/libusb/config.h
endif

libusb.a:\
	externals/libusb/libusb/core.o\
	externals/libusb/libusb/descriptor.o\
	externals/libusb/libusb/hotplug.o\
	externals/libusb/libusb/io.o\
	externals/libusb/libusb/strerror.o\
	externals/libusb/libusb/sync.o\
	externals/libusb/libusb/os/darwin_usb.o\
	externals/libusb/libusb/os/poll_posix.o\
	externals/libusb/libusb/os/threads_posix.o
	$(AR) $(ARFLAGS) $@ $^


libindigo.a:\
	indigo_bus/indigo_bus.o\
	indigo_bus/indigo_base64.o\
	indigo_bus/indigo_xml.o\
	indigo_bus/indigo_version.o\
	indigo_bus/indigo_server_xml.o\
	indigo_bus/indigo_driver_xml.o\
	indigo_bus/indigo_client_xml.o\
	indigo_drivers/indigo_driver.o\
	indigo_drivers/indigo_ccd_driver.o\
	indigo_drivers/indigo_guider_driver.o\
	indigo_drivers/indigo_wheel_driver.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_simulator.a: indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_simulator: indigo_drivers/ccd_simulator/indigo_ccd_simulator_main.o indigo_ccd_simulator.a libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

indigo_ccd_sx.a: indigo_drivers/ccd_sx/indigo_ccd_sx.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_sx: indigo_drivers/ccd_sx/indigo_ccd_sx_main.o indigo_ccd_sx.a libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

indigo_ccd_ssag.a: indigo_drivers/ccd_ssag/indigo_ccd_ssag.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_ssag: indigo_drivers/ccd_ssag/indigo_ccd_ssag_main.o indigo_ccd_ssag.a libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

indigo_ccd_asi.a: indigo_drivers/ccd_asi/indigo_ccd_asi.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_asi: indigo_drivers/ccd_asi/indigo_ccd_asi_main.o indigo_ccd_asi.a libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: indigo_test/test.o indigo_ccd_simulator.a libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: indigo_test/client.o libindigo.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: indigo_test/server.o indigo_ccd_simulator.a indigo_ccd_sx.a indigo_ccd_ssag.a indigo_ccd_asi.a libindigo.a $(LIBUSB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean: init
	rm -f $(LIBUSB) externals/libusb/libusb/config.h externals/libusb/libusb/*.o externals/libusb/libusb/os/*.o
	rm -f $(HIDAPI)
	rm -f libindigo.a indigo_bus/*.o indigo_drivers/*.o
	rm -f indigo_ccd_simulator indigo_ccd_simulator.a indigo_drivers/ccd_simulator/*.o
	rm -f indigo_ccd_sx indigo_ccd_sx.a indigo_drivers/ccd_sx/*.o
	rm -f indigo_ccd_ssag indigo_ccd_ssag.a indigo_drivers/ccd_ssag/*.o
	rm -f indigo_ccd_asi indigo_ccd_asi.a indigo_drivers/ccd_asi/*.o
	rm -f test client server indigo_test/*.o
