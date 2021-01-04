#
# libgps - interface library for GPSD
#

INDIGO_ROOT=../../../..

ifeq ($(OS),Windows_NT)
  OS_DETECTED=Windows
else
	OS_DETECTED=$(shell uname -s)
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

LIBGPS=libgps.a

ifeq ($(OS_DETECTED),Darwin)
	CC = /usr/bin/clang
	ifeq ($(findstring arm64e,$(shell file $(CC) | head -1)),arm64e)
		MAC_ARCH = -arch x86_64 -arch arm64
	else
		MAC_ARCH = -arch x86_64
	endif
	CPPFLAGS = $(MAC_ARCH) -fPIC -O3 -I $(INDIGO_ROOT)/build/include -I. -mmacosx-version-min=10.10
	LDFLAGS = $(MAC_ARCH) -framework CoreFoundation -framework IOKit -Llib/macOS
	AR = ar
	ARFLAGS = -rus
endif

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	ifeq ($(ARCH_DETECTED),arm)
		CPPFLAGS=-fPIC -O3 -I $(INDIGO_ROOT)/build/include -I. -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm -mthumb-interwork
	else
		CPPFLAGS=-fPIC -O3 -I $(INDIGO_ROOT)/build/include -I.
	endif
	LDFLAGS=-lm -Llib/Linux/$(ARCH_DETECTED)
	AR=ar
	ARFLAGS=-rv
endif

.PHONY: clean remote

all: $(LIBGPS)

$(LIBGPS): ais_json.o gps_maskdump.o json.o libgpsd_core.o libgps_sock.o rtcm2_json.o timespec_str.o bits.o gpsutils.o libgps_core.o libgps_json.o netlib.o rtcm3_json.o gpsdclient.o hex.o libgps_dbus.o libgps_shm.o os_compat.o shared_json.o
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -f $(LIBGPS) *.o
	rm -f $(LIBGPS) *.a
