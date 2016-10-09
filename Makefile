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
CFLAGS=-Iindigo_bus -Iindigo_drivers -std=gnu99 -DINDIGO_DARWIN
AR=ar
ARFLAGS=-rv
endif

ifeq ($(OS_detected),Linux)
CC=gcc
CFLAGS=-Iindigo_bus -Iindigo_drivers -std=gnu99 -pthread -lm -lrt -DINDIGO_LINUX
AR=ar
ARFLAGS=-rv
endif

.PHONY: init clean

all: init indigo_ccd_simulator test client server

init:
	$(info -------------------- $(OS_detected) build --------------------)

libindigo.a:\
	indigo_bus/indigo_bus.o\
	indigo_bus/indigo_xml.o\
	indigo_bus/indigo_version.o\
	indigo_bus/indigo_server_xml.o\
	indigo_bus/indigo_driver_xml.o\
	indigo_bus/indigo_client_xml.o\
	indigo_drivers/indigo_driver.o\
	indigo_drivers/indigo_ccd_driver.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_simulator.a: indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
	$(AR) $(ARFLAGS) $@ $^

indigo_ccd_simulator: indigo_drivers/ccd_simulator/indigo_ccd_simulator_main.o indigo_ccd_simulator.a libindigo.a
	$(CC) $(CFLAGS) -o $@ $^

test: indigo_test/test.o indigo_ccd_simulator.a libindigo.a
	$(CC) $(CFLAGS) -o $@ $^

client: indigo_test/client.o libindigo.a
	$(CC) $(CFLAGS) -o $@ $^

server: indigo_test/server.o indigo_ccd_simulator.a libindigo.a
	$(CC) $(CFLAGS) -o $@ $^

clean: init
	rm -f libindigo.a indigo_ccd_simulator indigo_ccd_simulator.a test client server indigo_test/*.o indigo_bus/*.o indigo_drivers/*.o indigo_drivers/ccd_simulator/*.o
