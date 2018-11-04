#---------------------------------------------------------------------
#
#	Build INDI-compatible minipackage
#
#---------------------------------------------------------------------

ARCH_DETECTED=$(shell uname -m)
ifeq ($(ARCH_DETECTED),armv7l)
	DEBIAN_ARCH=armhf
endif
ifeq ($(ARCH_DETECTED),aarch64)
	DEBIAN_ARCH=arm64
endif
ifeq ($(ARCH_DETECTED),i686)
	DEBIAN_ARCH=i386
endif
ifeq ($(ARCH_DETECTED),x86_64)
	DEBIAN_ARCH=amd64
endif

CC=gcc
ifeq ($(ARCH_DETECTED),arm)
	CFLAGS=-fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -I../../indigo_libs -std=gnu11 -pthread -DINDIGO_LINUX
	CXXFLAGS=-fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -I../../indigo_libs -std=gnu++11 -pthread -DINDIGO_LINUX
else
	CFLAGS=-fPIC -O3 -I../../indigo_libs -std=gnu11 -pthread -DINDIGO_LINUX
	CXXFLAGS=-fPIC -O3 -I../../indigo_libs -std=gnu++11 -pthread -DINDIGO_LINUX
endif
LDFLAGS=-lm -lrt -lusb-1.0

VERSION=$(shell ../../build/bin/indigo_drivers -v ../../build/drivers/indigo_aux_upb)
PACKAGE_NAME=indigo-upb-$(VERSION)-$(DEBIAN_ARCH)
INSTALL_PREFIX=/usr

all:
	install -d /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin
	install -d /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/share/indi
	install -d /tmp/$(PACKAGE_NAME)/lib/udev/rules.d
	install -d /tmp/$(PACKAGE_NAME)/DEBIAN
	$(CC) $(CFLAGS) -o /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin/indigo_aux_upb indigo_aux_upb_main.o ../../build/drivers/indigo_aux_upb.a ../../build/lib/libindigo.a $(LDFLAGS)
	../../build/bin/indigo_drivers ../../build/drivers/indigo_aux_upb >/tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/share/indi/indi_upb.xml
	cp 99-indigo_aux_upb.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d
	printf "Package: indigo-upb\nVersion: $(VERSION)\nInstalled-Size: $(shell echo $$((`du -s /tmp/$(PACKAGE_NAME) | cut -f1`)))\nPriority: optional\nArchitecture: $(DEBIAN_ARCH)\nMaintainer: CloudMakers, s. r. o.\nDescription: PegasusAstro Ultimate Powerbox INDIGO driver for INDI\n" > /tmp/$(PACKAGE_NAME)/DEBIAN/control
	sudo chown root /tmp/$(PACKAGE_NAME)
	dpkg --build /tmp/$(PACKAGE_NAME)
	mv /tmp/$(PACKAGE_NAME).deb .
	sudo rm -rf /tmp/$(PACKAGE_NAME)
