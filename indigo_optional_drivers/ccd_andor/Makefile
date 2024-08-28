# Andor driver Makefile
# Copyright (c) 2018 Rumen G. Bogdanovski
# All rights reserved.

include ../../Makefile.inc

DRIVER_NAME=indigo_ccd_andor

ROOT=../..
DRIVER_SO = $(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT)

DEBUG_BUILD=-g

OS_DETECTED := $(shell uname -s)
ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	CFLAGS=$(DEBUG_BUILD) -fPIC -O3 -Wall -I$(ROOT)/indigo_libs -I$(ROOT)/indigo_drivers -I $(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX
	LDFLAGS=-lm -L $(BUILD_LIB) -Wl,-rpath=\$$ORIGIN/../lib,-rpath=\$$ORIGIN/../drivers,-rpath=.
	SOEXT=so
	AR=ar
	ARFLAGS=-rv
else
fail:
	@echo "This driver can be built for Linux/Intel only."
endif

#---------------------------------------------------------------------
#
#	Build the driver
#
#---------------------------------------------------------------------

all: $(BUILD_DRIVERS)/$(DRIVER_NAME).a $(BUILD_DRIVERS)/$(DRIVER_NAME) $(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT)

$(BUILD_DRIVERS)/$(DRIVER_NAME).a: $(DRIVER_NAME).o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/$(DRIVER_NAME): $(DRIVER_NAME)_main.o $(BUILD_DRIVERS)/$(DRIVER_NAME).a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo -landor

$(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT): $(DRIVER_NAME).o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo -landor

install:
	install -m 0644 $(DRIVER_SO) $(INSTALL_LIB)

clean:
	rm $(BUILD_DRIVERS)/$(DRIVER_NAME).a $(BUILD_DRIVERS)/$(DRIVER_NAME) $(BUILD_DRIVERS)/$(DRIVER_NAME).$(SOEXT) $(DRIVER_NAME)_main.o $(DRIVER_NAME).o
