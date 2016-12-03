#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

INDIGO_VERSION := 2.0
INDIGO_BUILD := 9
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
		DEBIAN_ARCH=armhf
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
	LIBQHY=indigo_drivers/ccd_qhy/externals/libqhy/lib/macOS/libqhy.a
	LIBFCUSB=indigo_drivers/focuser_fcusb/bin_externals/libfcusb/lib/macOS/libfcusb.a
	LIBASIEFW=indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/mac/libEFWFilter.a
	LIBASICAMERA=indigo_drivers/ccd_asi/bin_externals/libasicamera/lib/mac/libASICamera2.a
	PACKAGE_NAME=indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)
	PACKAGE_TYPE=dmg
	UINT=-Duint=unsigned
	LIBUSB_CFLAGS=LIBUSB_CFLAGS="-I$(INDIGO_ROOT)/include/libusb-1.0"
	LIBUSB_LIBS=LIBUSB_LIBS="-L$(INDIGO_ROOT)/lib -lusb-1.0"
endif
ifeq ($(OS_DETECTED),Linux)
	LIBATIK=indigo_drivers/ccd_atik/bin_externals/libatik/lib/Linux/$(ARCH_DETECTED)/libatik.a
	LIBQHY=indigo_drivers/ccd_qhy/externals/libqhy/lib/Linux/$(ARCH_DETECTED)/libqhy.a
	LIBFCUSB=indigo_drivers/focuser_fcusb/bin_externals/libfcusb/lib/Linux/$(ARCH_DETECTED)/libfcusb.a
	ifeq ($(ARCH_DETECTED),arm)
		LIBASIEFW=indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/armv6/libEFWFilter.a
		LIBASICAMERA=indigo_drivers/ccd_asi/bin_externals/libasicamera/lib/armv6/libASICamera2.a
	else
		LIBASIEFW=indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/$(ARCH_DETECTED)/libEFWFilter.a
		LIBASICAMERA=indigo_drivers/ccd_asi/bin_externals/libasicamera/lib/$(ARCH_DETECTED)/libASICamera2.a
	endif
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
	CFLAGS=-fPIC -O3 -Iindigo_libs -Iindigo_drivers -Iinclude -std=gnu11 -DINDIGO_MACOS
	LDFLAGS=-framework Cocoa -framework CoreFoundation -framework IOKit -lobjc  -Llib -lusb-1.0
	LIBHIDAPI=lib/libhidapi.a
	SOEXT=dylib
	AR=ar
	ARFLAGS=-rv
	EXTERNALS=lib/libusb-1.0.$(SOEXT) $(LIBHIDAPI) lib/libatik.a lib/libqhy.a lib/libfcusb.a lib/libnovas.a lib/libEFWFilter.a lib/libASICamera2.a lib/libdc1394.a
endif

#---------------------------------------------------------------------
#
#	Linux parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	ifeq ($(ARCH_DETECTED),arm)
		CFLAGS=-g -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -Iindigo_libs -Iindigo_drivers -Iinclude  -std=gnu11 -pthread -DINDIGO_LINUX
	else
		CFLAGS=-g -fPIC -O3 -Iindigo_libs -Iindigo_drivers -Iinclude -std=gnu11 -pthread -DINDIGO_LINUX
	endif
	LDFLAGS=-lm -lrt -lusb-1.0 -ldl -ludev -Llib
	SOEXT=so
	LIBHIDAPI=lib/libhidapi-hidraw.a
	AR=ar
	ARFLAGS=-rv
	EXTERNALS=$(LIBHIDAPI) lib/libatik.a lib/libqhy.a lib/libfcusb.a lib/libnovas.a lib/libEFWFilter.a lib/libASICamera2.a lib/libdc1394.a
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
	$(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*))) \
	$(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*))) \
	$(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*))) \
	$(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/mount_*)))

DRIVER_LIBS=\
	$(addsuffix .a, $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .a, $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .a, $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))) \
	$(addsuffix .a, $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/mount_*))))

DRIVER_SOLIBS=\
	$(addsuffix .$(SOEXT), $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/mount_*))))

SIMULATOR_LIBS=\
	$(addsuffix .a, $(addprefix drivers/indigo_, $(notdir $(wildcard indigo_drivers/*_simulator))))

.PHONY: init clean

#---------------------------------------------------------------------
#
#	Default target
#
#---------------------------------------------------------------------

all: init $(EXTERNALS) lib/libindigo.a lib/libindigo.$(SOEXT) drivers bin/indigo_server_standalone bin/test bin/client bin/indigo_server

#---------------------------------------------------------------------
#
#	Build libusb-1.0 for macOS
#
#---------------------------------------------------------------------

externals/libusb/configure: externals/libusb/configure.ac
	cd externals/libusb; autoreconf -i; cd ../..

externals/libusb/Makefile: externals/libusb/configure
	cd externals/libusb; ./configure --prefix=$(INDIGO_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) --with-pic; cd ../..

lib/libusb-1.0.$(SOEXT): externals/libusb/Makefile
	cd externals/libusb; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libhidapi
#
#---------------------------------------------------------------------

externals/hidapi/configure: externals/hidapi/configure.ac
	cd externals/hidapi; ./bootstrap; cd ../..

externals/hidapi/Makefile: externals/hidapi/configure
	cd externals/hidapi; ./configure --prefix=$(INDIGO_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) --with-pic; cd ../..

$(LIBHIDAPI): externals/hidapi/Makefile
	cd externals/hidapi; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libdc1394
#
#---------------------------------------------------------------------

indigo_drivers/ccd_iidc/externals/libdc1394/configure: indigo_drivers/ccd_iidc/externals/libdc1394/configure.ac
	cd indigo_drivers/ccd_iidc/externals/libdc1394; autoreconf -i; cd ../../../..

indigo_drivers/ccd_iidc/externals/libdc1394/Makefile: indigo_drivers/ccd_iidc/externals/libdc1394/configure
	cd indigo_drivers/ccd_iidc/externals/libdc1394; ./configure --prefix=$(INDIGO_ROOT) --disable-libraw1394 --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS) $(UINT)" $(LIBUSB_CFLAGS) $(LIBUSB_LIBS); cd ../../../..

lib/libdc1394.a: indigo_drivers/ccd_iidc/externals/libdc1394/Makefile
	cd indigo_drivers/ccd_iidc/externals/libdc1394; make install; cd ../../../..

#---------------------------------------------------------------------
#
#	Build libnovas
#
#---------------------------------------------------------------------

lib/libnovas.a: externals/novas/novas.o externals/novas/eph_manager.o externals/novas/novascon.o externals/novas/nutation.o externals/novas/readeph0.o  externals/novas/solsys1.o  externals/novas/solsys3.o
	$(AR) $(ARFLAGS) $@ $^
	install externals/novas/JPLEPH.421 lib

#---------------------------------------------------------------------
#
#	Install libatik
#
#---------------------------------------------------------------------

include/libatik/libatik.h: indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik/libatik.h
	install -d include
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik include

lib/libatik.a: include/libatik/libatik.h
	install -d lib
	ln -sf $(INDIGO_ROOT)/$(LIBATIK) lib

#---------------------------------------------------------------------
#
#	Install libqhy
#
#---------------------------------------------------------------------

include/libqhy/libqhy.h: indigo_drivers/ccd_qhy/externals/libqhy/include/libqhy/libqhy.h
	install -d include
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_qhy/externals/libqhy/include/libqhy include

lib/libqhy.a: include/libqhy/libqhy.h
	cd indigo_drivers/ccd_qhy/externals/libqhy; make clean; make; cd ../../../..
	install -d lib
	ln -sf $(INDIGO_ROOT)/$(LIBQHY) lib


#---------------------------------------------------------------------
#
#	Install libEFWFilter
#
#---------------------------------------------------------------------

include/asi_efw/EFW_filter.h: indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include/EFW_filter.h
	install -d include/asi_efw
	cp indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include/EFW_filter.h include/asi_efw

lib/libEFWFilter.a: include/asi_efw/EFW_filter.h
	install -d lib
	cp $(LIBASIEFW) lib


#---------------------------------------------------------------------
#
#       Install libasicamera2
#
#---------------------------------------------------------------------

include/asi_ccd/ASICamera2.h: indigo_drivers/ccd_asi/bin_externals/libasicamera/include/ASICamera2.h
	install -d include/asi_ccd
	cp indigo_drivers/ccd_asi/bin_externals/libasicamera/include/ASICamera2.h include/asi_ccd

lib/libASICamera2.a: include/asi_ccd/ASICamera2.h
	install -d lib
	cp $(LIBASICAMERA) lib


#---------------------------------------------------------------------
#
#	Install libfcusb
#
#---------------------------------------------------------------------

include/libfcusb/libfcusb.h: indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb/libfcusb.h
	install -d include
	ln -sf $(INDIGO_ROOT)/indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb include

lib/libfcusb.a: include/libfcusb/libfcusb.h
	install -d lib
	ln -sf $(INDIGO_ROOT)/$(LIBFCUSB) lib

#---------------------------------------------------------------------
#
#	Initialize
#
#---------------------------------------------------------------------

init:
	$(info -------------------- $(OS_DETECTED) build --------------------)
	$(info drivers: $(notdir $(DRIVERS)))
	git submodule update --init --recursive
#	git submodule update --remote
	install -d bin
	install -d lib
	install -d drivers
	install -d include
	cp indigo_libs/indigo_config.h indigo_libs/indigo_config.h.orig
	sed 's/INDIGO_BUILD.*/INDIGO_BUILD $(INDIGO_BUILD)/' indigo_libs/indigo_config.h.orig >indigo_libs/indigo_config.h
	cp INDIGO\ Server\ for\ macOS/Info.plist INDIGO\ Server\ for\ macOS/Info.plist.orig
	sed '/CFBundleVersion/ { n; s/>.*</>$(INDIGO_BUILD)</; }' INDIGO\ Server\ for\ macOS/Info.plist.orig >INDIGO\ Server\ for\ macOS/Info.plist

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

lib/libindigo.a: $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c)))
	$(AR) $(ARFLAGS) $@ $^

lib/libindigo.$(SOEXT): $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c)))
	$(CC) -shared -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------------------
#
#	Build drivers
#
#---------------------------------------------------------------------

drivers: $(DRIVER_LIBS) $(DRIVER_SOLIBS) $(DRIVERS)

#---------------------------------------------------------------------
#
#	Build CCD simulator driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_simulator.a: indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_simulator: indigo_drivers/ccd_simulator/indigo_ccd_simulator_main.o drivers/indigo_ccd_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_ccd_simulator.$(SOEXT): indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount simulator driver
#
#---------------------------------------------------------------------

drivers/indigo_mount_simulator.a: indigo_drivers/mount_simulator/indigo_mount_simulator.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_mount_simulator: indigo_drivers/mount_simulator/indigo_mount_simulator_main.o drivers/indigo_mount_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_mount_simulator.$(SOEXT): indigo_drivers/mount_simulator/indigo_mount_simulator.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount nexstar driver
#
#---------------------------------------------------------------------

drivers/indigo_mount_nexstar.a: indigo_drivers/mount_nexstar/indigo_mount_nexstar.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_mount_nexstar: indigo_drivers/mount_nexstar/indigo_mount_nexstar_main.o drivers/indigo_mount_nexstar.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_mount_nexstar.$(SOEXT): indigo_drivers/mount_nexstar/indigo_mount_nexstar.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build SX CCD driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_sx.a: indigo_drivers/ccd_sx/indigo_ccd_sx.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_sx: indigo_drivers/ccd_sx/indigo_ccd_sx_main.o drivers/indigo_ccd_sx.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_ccd_sx.$(SOEXT): indigo_drivers/ccd_sx/indigo_ccd_sx.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build SSAG/QHY5 CCD driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_ssag.a: indigo_drivers/ccd_ssag/indigo_ccd_ssag.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_ssag: indigo_drivers/ccd_ssag/indigo_ccd_ssag_main.o drivers/indigo_ccd_ssag.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_ccd_ssag.$(SOEXT): indigo_drivers/ccd_ssag/indigo_ccd_ssag.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build ASI CCD driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_asi.a: indigo_drivers/ccd_asi/indigo_ccd_asi.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_asi: indigo_drivers/ccd_asi/indigo_ccd_asi_main.o drivers/indigo_ccd_asi.a lib/libASICamera2.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

drivers/indigo_ccd_asi.$(SOEXT): indigo_drivers/ccd_asi/indigo_ccd_asi.o lib/libASICamera2.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

#---------------------------------------------------------------------
#
#	Build ATIK CCD driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_atik.a: indigo_drivers/ccd_atik/indigo_ccd_atik.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_atik: indigo_drivers/ccd_atik/indigo_ccd_atik_main.o drivers/indigo_ccd_atik.a lib/libatik.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_ccd_atik.$(SOEXT): indigo_drivers/ccd_atik/indigo_ccd_atik.o lib/libatik.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build SX filter wheel driver
#
#---------------------------------------------------------------------

drivers/indigo_wheel_sx.a: indigo_drivers/wheel_sx/indigo_wheel_sx.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_wheel_sx: indigo_drivers/wheel_sx/indigo_wheel_sx_main.o drivers/indigo_wheel_sx.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_wheel_sx.$(SOEXT): indigo_drivers/wheel_sx/indigo_wheel_sx.o $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build ASI filter wheel driver
#
#---------------------------------------------------------------------

drivers/indigo_wheel_asi.a: indigo_drivers/wheel_asi/indigo_wheel_asi.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_wheel_asi: indigo_drivers/wheel_asi/indigo_wheel_asi_main.o drivers/indigo_wheel_asi.a lib/libEFWFilter.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

drivers/indigo_wheel_asi.$(SOEXT): indigo_drivers/wheel_asi/indigo_wheel_asi.o lib/libEFWFilter.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

#---------------------------------------------------------------------
#
#	Build QHY CCD driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_qhy.a: indigo_drivers/ccd_qhy/indigo_ccd_qhy.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_qhy: indigo_drivers/ccd_qhy/indigo_ccd_qhy_main.o drivers/indigo_ccd_qhy.a lib/libqhy.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_ccd_qhy.$(SOEXT): indigo_drivers/ccd_qhy/indigo_ccd_qhy.o lib/libqhy.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Shoestring FCUSB focuser driver
#
#---------------------------------------------------------------------

drivers/indigo_focuser_fcusb.a: indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_focuser_fcusb: indigo_drivers/focuser_fcusb/indigo_focuser_fcusb_main.o drivers/indigo_focuser_fcusb.a lib/libfcusb.a  $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_focuser_fcusb.$(SOEXT): indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.o lib/libfcusb.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build IIDC CCD driver
#
#---------------------------------------------------------------------

drivers/indigo_ccd_iidc.a: indigo_drivers/ccd_iidc/indigo_ccd_iidc.o
	$(AR) $(ARFLAGS) $@ $^

drivers/indigo_ccd_iidc: indigo_drivers/ccd_iidc/indigo_ccd_iidc_main.o drivers/indigo_ccd_iidc.a lib/libdc1394.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

drivers/indigo_ccd_iidc.$(SOEXT): indigo_drivers/ccd_iidc/indigo_ccd_iidc.o lib/libdc1394.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build tests
#
#---------------------------------------------------------------------

bin/test: indigo_test/test.o drivers/indigo_ccd_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

bin/client: indigo_test/client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build indigo_server
#
#---------------------------------------------------------------------

bin/indigo_server: indigo_server/indigo_server.o $(SIMULATOR_LIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

bin/indigo_server_standalone: indigo_server/indigo_server.c $(DRIVER_LIBS) $(EXTERNALS) lib/libindigo.a
	$(CC) -DSTATIC_DRIVERS $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++


#---------------------------------------------------------------------
#
#	Install
#
#---------------------------------------------------------------------

install:
	sudo install -D -m 0755 bin/indigo_server /tmp/$(PACKAGE_NAME)/usr/local/bin
	sudo install -D -m 0755 bin/indigo_server_standalone /tmp/$(PACKAGE_NAME)/usr/local/bin
	sudo install -D -m 0644 $(DRIVERS) /tmp/$(PACKAGE_NAME)/usr/local/bin
	sudo install -D -m 0644 $(DRIVER_LIBS) /tmp/$(PACKAGE_NAME)/usr/local/lib
	sudo install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /lib/udev/rules.d/99-indigo_ccd_sx.rules
	sudo install -D -m 0644 indigo_drivers/ccd_atik/indigo_ccd_atik.rules /lib/udev/rules.d/99-indigo_ccd_atik.rules
	sudo install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /lib/udev/rules.d/99-indigo_ccd_ssag.rules
	sudo install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules /lib/udev/rules.d/99-indigo_ccd_asi.rules
	sudo install -D -m 0644 indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/99-efw.rules /lib/udev/rules.d/99-indigo_wheel_asi.rules
	sudo udevadm control --reload-rules

#---------------------------------------------------------------------
#
#	Build packages
#
#---------------------------------------------------------------------

package: $(PACKAGE_NAME).$(PACKAGE_TYPE)

$(PACKAGE_NAME).deb: clean all
	install -d /tmp/$(PACKAGE_NAME)/usr/local/bin
	install bin/indigo_server /tmp/$(PACKAGE_NAME)/usr/local/bin
	install bin/indigo_server_standalone /tmp/$(PACKAGE_NAME)/usr/local/bin
	install $(DRIVERS) /tmp/$(PACKAGE_NAME)/usr/local/bin
	install -d /tmp/$(PACKAGE_NAME)/usr/local/lib
	install $(DRIVER_LIBS) /tmp/$(PACKAGE_NAME)/usr/local/lib
	install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_sx.rules
	install -D -m 0644 indigo_drivers/ccd_atik/indigo_ccd_atik.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_atik.rules
	install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_ssag.rules
	install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_asi.rules
	install -D -m 0644 indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/99-efw.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_wheel_asi.rules
	cp -r share /tmp/$(PACKAGE_NAME)
	install -d /tmp/$(PACKAGE_NAME)/DEBIAN
	printf "Package: indigo\nVersion: $(INDIGO_VERSION)-$(INDIGO_BUILD)\nPriority: optional\nArchitecture: $(DEBIAN_ARCH)\nMaintainer: CloudMakers, s. r. o.\nDepends: libusb-1.0-0, libgudev-1.0-0\nDescription: INDIGO Server\n" > /tmp/$(PACKAGE_NAME)/DEBIAN/control
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
	rm -rf drivers/*
	rm -rf include/*
	rm -f indigo_libs/*.o
	rm -f indigo_server/*.o
	rm -f $(wildcard indigo_drivers/*/*.o)
	rm -f $(wildcard indigo_test/*.o)
	cd externals/hidapi; make maintainer-clean; cd ../..
	cd externals/libusb; make maintainer-clean; cd ../..
	cd indigo_drivers/ccd_iidc/externals/libdc1394; make maintainer-clean; cd ../../../..
