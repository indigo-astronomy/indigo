# RPi Camera Driver Makefile
# Copyright (c) 2025 Rohan Salodkar
# All rights reserved.
include ../../Makefile.inc

ARCH := $(shell uname -m)
OS := $(shell uname -s)

DRIVER_NAME=indigo_ccd_rpi
DRIVER_METADATA=indigo/indigo_rpi_drivers

ROOT=../..
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT)

DEBUG_BUILD=-g

ifeq ($(OS),Linux)
	ifneq (,$(filter $(ARCH),aarch64 armv7l))
		CCD_RPI := 1
	endif
endif

ifeq ($(CCD_RPI), 1)
	CC=gcc
	CXXFLAGS+=$(DEBUG_BUILD) -fPIC -O3 -Wall -I$(ROOT)/indigo_libs -I$(ROOT)/indigo_drivers $(shell pkg-config --cflags libcamera) -I$(BUILD_INCLUDE) -std=gnu++17 -pthread -DINDIGO_LINUX
	LDFLAGS=-lm -lstdc++ -ludev -L $(BUILD_LIB) -Wl,-rpath=\$$ORIGIN/../lib,-rpath=\$$ORIGIN/../drivers,-rpath=.
	SOEXT=so
	AR=ar
	ARFLAGS=-rv

all:
	@echo "CCD_RPI: Building for aarch64/armv7l"
else
fail:
	@echo "CCD_RPI: Skipping build - ARCH is not 'aarch64' or 'armv7l' (was '$(ARCH)')"
endif

#---------------------------------------------------------------------
#
#	Build the driver
#
#---------------------------------------------------------------------

all: $(BUILD_SHARE)/$(DRIVER_METADATA) $(BUILD_DRIVERS)/$(DRIVER_NAME).a $(BUILD_DRIVERS)/$(DRIVER_NAME) $(DRIVER_SO)

$(BUILD_DRIVERS)/$(DRIVER_NAME).a: $(DRIVER_NAME).o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/$(DRIVER_NAME): $(DRIVER_NAME)_main.o $(BUILD_DRIVERS)/$(DRIVER_NAME).a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo $(shell pkg-config --libs libcamera)

$(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT): $(DRIVER_NAME).o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo $(shell pkg-config --libs libcamera)

$(BUILD_SHARE)/$(DRIVER_METADATA): $(DRIVER_SO)
	$(BUILD_BIN)/indigo_driver_metadata $^ >$@

install: all
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)
	install -m 0644 $(BUILD_DRIVERS)/$(DRIVER_NAME).a $(INSTALL_LIB)
	install $(BUILD_DRIVERS)/$(DRIVER_NAME) $(INSTALL_BIN)
	$(BUILD_BIN)/indigo_driver_metadata $(DRIVER_SO) >$(INSTALL_SHARE)/$(DRIVER_METADATA)

uninstall:
	rm -f $(INSTALL_LIB)/$(DRIVER_NAME).$(SOEXT) $(INSTALL_LIB)/$(DRIVER_NAME).a $(INSTALL_BIN)/$(DRIVER_NAME) $(INSTALL_SHARE)/$(DRIVER_METADATA)

clean:
	rm -f $(BUILD_DRIVERS)/$(DRIVER_NAME).a $(BUILD_DRIVERS)/$(DRIVER_NAME) $(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT) $(DRIVER_NAME)_main.o $(DRIVER_NAME).o
