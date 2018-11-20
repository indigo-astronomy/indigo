#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

INDIGO_VERSION := 2.0
INDIGO_BUILD := 76
INDIGO_ROOT := $(shell pwd)

DEBUG_BUILD=-g

ENABLE_STATIC=yes
ENABLE_SHARED=yes

BUILD_ROOT=build
BUILD_BIN=$(BUILD_ROOT)/bin
BUILD_DRIVERS=$(BUILD_ROOT)/drivers
BUILD_LIB=$(BUILD_ROOT)/lib
BUILD_INCLUDE=$(BUILD_ROOT)/include
BUILD_SHARE=$(BUILD_ROOT)/share
BIN_EXTERNALS=bin_externals

ifeq ($(OS),Windows_NT)
	OS_DETECTED := Windows
else
	OS_DETECTED := $(shell uname -s)
	ARCH_DETECTED=$(shell uname -m)
	ifeq ($(ARCH_DETECTED),armv7l)
		ARCH_DETECTED=arm
		DEBIAN_ARCH=armhf
	endif
	ifeq ($(ARCH_DETECTED),aarch64)
		ARCH_DETECTED=arm64
		DEBIAN_ARCH=arm64
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
	LIBATIKCAMERAS=indigo_drivers/ccd_atik/bin_externals/libatikcameras/lib/macOS/libatikcameras.dylib
	LIBATIK=indigo_drivers/wheel_atik/bin_externals/libatik/lib/macOS/libatik.a
	LIBGX=indigo_drivers/ccd_mi/bin_externals/libgxccd/lib/macOS/libgxccd.a
	LIBFCUSB=indigo_drivers/focuser_fcusb/bin_externals/libfcusb/lib/macOS/libfcusb.a
	LIBDSUSB=indigo_drivers/aux_dsusb/bin_externals/libdsusb/lib/macOS/libdsusb.a
	LIBGPUSB=indigo_drivers/guider_gpusb/bin_externals/libgpusb/lib/macOS/libgpusb.a
	LIBASIEFW=indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/macOS/libEFWFilter.a
	LIBASICAMERA=indigo_drivers/ccd_asi/bin_externals/libasicamera/lib/macOS/libASICamera2.a
	LIBASIST4=indigo_drivers/guider_asi/bin_externals/libusb2st4conv/lib/macOS/libUSB2ST4Conv.a
	LIBTOUPCAM=indigo_drivers/ccd_touptek/bin_externals/libtoupcam/lib/macOS/libtoupcam.dylib
	LIBALTAIRCAM=indigo_drivers/ccd_altair/bin_externals/libaltaircam/lib/macOS/libaltaircam.dylib
	FLISDK=libfli-1.999.1-180223
	PACKAGE_NAME=indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)
	PACKAGE_TYPE=dmg
	UINT=-Duint=unsigned
	LIBUSB_CFLAGS=-I$(INDIGO_ROOT)/$(BUILD_INCLUDE)/libusb-1.0
	LIBUSB_LIBS=-L$(INDIGO_ROOT)/$(BUILD_LIB) -lusb-1.0
endif
ifeq ($(OS_DETECTED),Linux)
	LIBATIKCAMERAS=indigo_drivers/ccd_atik/bin_externals/libatikcameras/lib/Linux/$(ARCH_DETECTED)/libatikcameras.so
	LIBATIK=indigo_drivers/wheel_atik/bin_externals/libatik/lib/Linux/$(ARCH_DETECTED)/libatik.a
	LIBGX=indigo_drivers/ccd_mi/bin_externals/libgxccd/lib/Linux/$(ARCH_DETECTED)/libgxccd.a
	LIBFCUSB=indigo_drivers/focuser_fcusb/bin_externals/libfcusb/lib/Linux/$(ARCH_DETECTED)/libfcusb.a
	LIBDSUSB=indigo_drivers/aux_dsusb/bin_externals/libdsusb/lib/Linux/$(ARCH_DETECTED)/libdsusb.a
	LIBGPUSB=indigo_drivers/guider_gpusb/bin_externals/libgpusb/lib/Linux/$(ARCH_DETECTED)/libgpusb.a
	LIBTOUPCAM=indigo_drivers/ccd_touptek/bin_externals/libtoupcam/lib/Linux/$(ARCH_DETECTED)/libtoupcam.so
	LIBALTAIRCAM=indigo_drivers/ccd_altair/bin_externals/libaltaircam/lib/Linux/$(ARCH_DETECTED)/libaltaircam.so
	LIBASIEFW=indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/Linux/$(ARCH_DETECTED)/libEFWFilter.a
	LIBASICAMERA=indigo_drivers/ccd_asi/bin_externals/libasicamera/lib/Linux/$(ARCH_DETECTED)/libASICamera2.a
	LIBASIST4=indigo_drivers/guider_asi/bin_externals/libusb2st4conv/lib/Linux/$(ARCH_DETECTED)/libUSB2ST4Conv.a
	FLISDK=libfli-1.999.1-180223
	LIBRAW_1394=$(shell pkg-config --libs libraw1394)
	LIBBOOST-REGEX=$(BUILD_LIB)/libboost_regex.a
	PACKAGE_NAME=indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-$(DEBIAN_ARCH)
	INSTALL_PREFIX=/usr/local
endif

#---------------------------------------------------------------------
#
#	Darwin/macOS parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
	CC=clang
	CFLAGS=$(DEBUG_BUILD) -mmacosx-version-min=10.10 -fPIC -O3 -Iindigo_libs -Iindigo_drivers -Iindigo_mac_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS
	MFLAGS=$(DEBUG_BUILD) -mmacosx-version-min=10.10 -fPIC -fno-common -O3 -fobjc-arc -Iindigo_libs -Iindigo_drivers -Iindigo_mac_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS -Wobjc-property-no-attribute
	CXXFLAGS=$(DEBUG_BUILD) -mmacosx-version-min=10.10 -fPIC -O3 -Iindigo_libs -Iindigo_drivers -Iindigo_mac_drivers -I$(BUILD_INCLUDE) -DINDIGO_MACOS
	LDFLAGS=-headerpad_max_install_names -framework Cocoa -mmacosx-version-min=10.10 -framework CoreFoundation -framework IOKit -framework ImageCaptureCore -framework IOBluetooth -lobjc  -L$(BUILD_LIB) -lusb-1.0
	LIBHIDAPI=$(BUILD_LIB)/libhidapi.a
	SOEXT=dylib
	AR=ar
	ARFLAGS=-rv
	EXTERNALS=$(BUILD_LIB)/libusb-1.0.$(SOEXT) $(LIBHIDAPI) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libatikcameras.dylib $(BUILD_LIB)/libatik.a $(BUILD_LIB)/libgxccd.a $(BUILD_LIB)/libqhy.a $(BUILD_LIB)/libfcusb.a $(BUILD_LIB)/libdsusb.a $(BUILD_LIB)/libgpusb.a $(BUILD_LIB)/libnovas.a $(BUILD_LIB)/libEFWFilter.a $(BUILD_LIB)/libASICamera2.a $(BUILD_LIB)/libUSB2ST4Conv.a $(BUILD_LIB)/libdc1394.a $(BUILD_LIB)/libnexstar.a $(BUILD_LIB)/libfli.a $(BUILD_LIB)/libqsiapi.a $(BUILD_LIB)/libftd2xx.a $(BUILD_LIB)/libapogee.a $(BUILD_LIB)/libtoupcam.dylib $(BUILD_LIB)/libaltaircam.dylib
	PLATFORM_DRIVER_LIBS=$(BUILD_DRIVERS)/indigo_ccd_ica.a $(BUILD_DRIVERS)/indigo_guider_eqmac.a $(BUILD_DRIVERS)/indigo_focuser_wemacro_bt.a $(BUILD_DRIVERS)/indigo_focuser_mjkzz_bt.a
	PLATFORM_DRIVER_SOLIBS=$(BUILD_DRIVERS)/indigo_ccd_ica.dylib $(BUILD_DRIVERS)/indigo_guider_eqmac.dylib $(BUILD_DRIVERS)/indigo_focuser_wemacro_bt.dylib $(BUILD_DRIVERS)/indigo_focuser_mjkzz_bt.dylib
endif

#---------------------------------------------------------------------
#
#	Linux parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	ifeq ($(ARCH_DETECTED),arm)
		CFLAGS=$(DEBUG_BUILD) -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -Iindigo_libs -Iindigo_drivers -Iindigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX
		CXXFLAGS=$(DEBUG_BUILD) -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -Iindigo_libs -Iindigo_drivers -Iindigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu++11 -pthread -DINDIGO_LINUX
	else
		CFLAGS=$(DEBUG_BUILD) -fPIC -O3 -Iindigo_libs -Iindigo_drivers -Iindigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX
		CXXFLAGS=$(DEBUG_BUILD) -fPIC -O3 -Iindigo_libs -Iindigo_drivers -Iindigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu++11 -pthread -DINDIGO_LINUX
	endif
	LDFLAGS=-lm -lrt -lusb-1.0 -ldl -ludev -ldns_sd -lgphoto2 -L$(BUILD_LIB) -Wl,-rpath=\$$ORIGIN/../lib,-rpath=\$$ORIGIN/../drivers,-rpath=.
	SOEXT=so
	LIBHIDAPI=$(BUILD_LIB)/libhidapi-hidraw.a
	AR=ar
	ARFLAGS=-rv
	EXTERNALS=$(LIBHIDAPI) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libatikcameras.so $(BUILD_LIB)/libatik.a $(BUILD_LIB)/libgxccd.a $(BUILD_LIB)/libqhy.a $(BUILD_LIB)/libfcusb.a $(BUILD_LIB)/libdsusb.a $(BUILD_LIB)/libgpusb.a $(BUILD_LIB)/libnovas.a $(BUILD_LIB)/libEFWFilter.a $(BUILD_LIB)/libASICamera2.a $(BUILD_LIB)/libUSB2ST4Conv.a $(BUILD_LIB)/libdc1394.a $(BUILD_LIB)/libnexstar.a $(BUILD_LIB)/libfli.a $(BUILD_LIB)/libsbigudrv.a $(BUILD_LIB)/libqsiapi.a $(BUILD_LIB)/libftd2xx.a $(BUILD_LIB)/libapogee.a $(BUILD_LIB)/libraw.a $(BUILD_LIB)/libtoupcam.so $(BUILD_LIB)/libaltaircam.so $(LIBBOOST-REGEX)
	PLATFORM_DRIVER_LIBS=$(BUILD_DRIVERS)/indigo_ccd_gphoto2.a
	PLATFORM_DRIVER_SOLIBS=$(BUILD_DRIVERS)/indigo_ccd_gphoto2.so
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
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/mount_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/gps_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/dome_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/guider_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/system_*))) \
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/aux_*)))

DRIVER_LIBS=$(PLATFORM_DRIVER_LIBS)\
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/mount_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/gps_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/dome_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/guider_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/system_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/aux_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/agent_*))))

DRIVER_SOLIBS=$(PLATFORM_DRIVER_SOLIBS)\
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/mount_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/gps_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/dome_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/guider_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/system_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/aux_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/agent_*))))

ifeq ($(ARCH_DETECTED),arm64)
	TMP := $(DRIVERS)
	DRIVERS := $(filter-out $(addprefix $(BUILD_DRIVERS)/, indigo_ccd_sbig),$(TMP))
	TMP := $(DRIVER_LIBS)
	DRIVER_LIBS := $(filter-out $(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/, indigo_ccd_sbig)),$(TMP))
	TMP := $(DRIVER_SOLIBS)
	DRIVER_SOLIBS := $(filter-out $(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/, indigo_ccd_sbig)),$(TMP))
endif

SIMULATOR_LIBS=\
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/*_simulator))))

SO_LIBS= $(wildcard $(BUILD_LIB)/*.$(SOEXT))

.PHONY: init submodule-init clean macfixpath

%.o:	%.m
	$(CC) -c -o $@ $< $(MFLAGS)

#---------------------------------------------------------------------
#
#	Default target
#
#---------------------------------------------------------------------

all: init $(EXTERNALS) $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libindigo.$(SOEXT) ctrlpanel drivers $(BUILD_BIN)/indigo_server_standalone $(BUILD_BIN)/indigo_prop_tool $(BUILD_BIN)/test $(BUILD_BIN)/client $(BUILD_BIN)/indigo_server $(BUILD_BIN)/indigo_drivers macfixpath
	cp $(wildcard indigo_drivers/*/indi_go_*.xml) $(BUILD_SHARE)/indi



#---------------------------------------------------------------------
#
#      Create driver list
#
#---------------------------------------------------------------------

$(BUILD_BIN)/indigo_drivers: indigo_tools/indigo_drivers.o
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_tools/indigo_drivers.o $(LDFLAGS) -lindigo
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	#install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif

#---------------------------------------------------------------------
#
#      Fix paths in Mac files
#
#---------------------------------------------------------------------

macfixpath:
ifeq ($(OS_DETECTED),Darwin)
	$(foreach lib, $(DRIVERS) $(DRIVER_SOLIBS) $(SO_LIBS), install_name_tool -id @rpath/../`expr "$(lib)" : '[^/]*/\(.*\)'` $(lib) >/dev/null; install_name_tool -add_rpath @loader_path/../drivers $(lib) >/dev/null 2>&1; install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $(lib) >/dev/null 2>&1; install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib @rpath/../lib/libusb-1.0.0.dylib $(lib) >/dev/null 2>&1;)
endif

#---------------------------------------------------------------------
#
#	Build libusb-1.0 for macOS
#
#---------------------------------------------------------------------

externals/libusb/configure: externals/libusb/configure.ac
	cd externals/libusb; autoreconf -fiv; cd ../..

externals/libusb/Makefile: externals/libusb/configure
	cd externals/libusb; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS)" --with-pic; cd ../..

$(BUILD_LIB)/libusb-1.0.$(SOEXT): externals/libusb/Makefile
	cd externals/libusb; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libhidapi
#
#---------------------------------------------------------------------

externals/hidapi/configure: externals/hidapi/configure.ac
	cd externals/hidapi; autoreconf -fiv; cd ../..

externals/hidapi/Makefile: externals/hidapi/configure
	cd externals/hidapi; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS)" --with-pic; cd ../..

$(LIBHIDAPI): externals/hidapi/Makefile
	cd externals/hidapi; make; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libdc1394
#
#---------------------------------------------------------------------

indigo_drivers/ccd_iidc/externals/libdc1394/configure: indigo_drivers/ccd_iidc/externals/libdc1394/configure.ac
	cd indigo_drivers/ccd_iidc/externals/libdc1394; autoreconf -fiv; cd ../../../..

indigo_drivers/ccd_iidc/externals/libdc1394/Makefile: indigo_drivers/ccd_iidc/externals/libdc1394/configure
ifeq ($(OS_DETECTED),Darwin)
	cd indigo_drivers/ccd_iidc/externals/libdc1394; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --disable-libraw1394 --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS) $(UINT)" LIBUSB_CFLAGS="$(LIBUSB_CFLAGS)" LIBUSB_LIBS="$(LIBUSB_LIBS)"; cd ../../../..
else
	cd indigo_drivers/ccd_iidc/externals/libdc1394; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --disable-libraw1394 --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS) $(UINT)"; cd ../../../..
endif

$(BUILD_LIB)/libdc1394.a: indigo_drivers/ccd_iidc/externals/libdc1394/Makefile
	cd indigo_drivers/ccd_iidc/externals/libdc1394; make install; cd ../../../..

#---------------------------------------------------------------------
#
#	Build libnovas
#
#---------------------------------------------------------------------

$(BUILD_LIB)/libnovas.a: externals/novas/novas.o externals/novas/eph_manager.o externals/novas/novascon.o externals/novas/nutation.o externals/novas/readeph0.o  externals/novas/solsys1.o  externals/novas/solsys3.o
	install externals/novas/novas.h $(BUILD_INCLUDE)
	install externals/novas/novascon.h $(BUILD_INCLUDE)
	install externals/novas/solarsystem.h $(BUILD_INCLUDE)
	install externals/novas/nutation.h $(BUILD_INCLUDE)
	install externals/novas/JPLEPH.421 $(BUILD_LIB)
	$(AR) $(ARFLAGS) $@ $^

#---------------------------------------------------------------------
#
#	Build libjpeg
#
#---------------------------------------------------------------------

externals/libjpeg/Makefile: externals/libjpeg/configure
	cd externals/libjpeg; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS)"; cd ../..

$(BUILD_LIB)/libjpeg.a: externals/libjpeg/Makefile
	cd externals/libjpeg; make install; cd ../..

#---------------------------------------------------------------------
#
#	Build libraw
#
#---------------------------------------------------------------------

indigo_linux_drivers/ccd_gphoto2/externals/libraw/configure: indigo_linux_drivers/ccd_gphoto2/externals/libraw/configure.ac
	cd indigo_linux_drivers/ccd_gphoto2/externals/libraw; autoreconf -fiv; cd ../../../..

indigo_linux_drivers/ccd_gphoto2/externals/libraw/Makefile: indigo_linux_drivers/ccd_gphoto2/externals/libraw/configure
	cd indigo_linux_drivers/ccd_gphoto2/externals/libraw; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) --disable-jasper --disable-lcms --disable-examples --disable-jpeg --disable-openmp CFLAGS="$(CFLAGS)"; cd ../../../..

$(BUILD_LIB)/libraw.a: indigo_linux_drivers/ccd_gphoto2/externals/libraw/Makefile
	cd indigo_linux_drivers/ccd_gphoto2/externals/libraw; make install; cd ../../../..

#---------------------------------------------------------------------
#
#	Install libatikcameras
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/AtikCameras.h: indigo_drivers/ccd_atik/bin_externals/libatikcameras/include/AtikCameras.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_atik/bin_externals/libatikcameras/include/AtikCameras.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libatikcameras.$(SOEXT): $(BUILD_INCLUDE)/AtikCameras.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBATIKCAMERAS) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Install libatik
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libatik.h: indigo_drivers/wheel_atik/bin_externals/libatik/include/libatik.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/wheel_atik/bin_externals/libatik/include/libatik.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libatik.a: $(BUILD_INCLUDE)/libatik.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBATIK) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Install libgxccd
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/gxccd.h: indigo_drivers/ccd_mi/bin_externals/libgxccd/include/gxccd.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_mi/bin_externals/libgxccd/include/gxccd.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libgxccd.a: $(BUILD_INCLUDE)/gxccd.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBGX) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Install libqhy
#
#---------------------------------------------------------------------

$(BUILD_LIB)/libqhy.a:
	mkdir libqhy_scratch; cd libqhy_scratch; cmake -DCMAKE_INSTALL_PREFIX=../build/ -DSKIP_FIRMWARE_INSTALL=True ../indigo_drivers/ccd_qhy/bin_externals/qhyccd/; make install; cd ..; rm -rf libqhy_scratch
	cp indigo_drivers/ccd_qhy/bin_externals/qhyccd/include/*.h $(BUILD_INCLUDE)

#---------------------------------------------------------------------
#
#	Install libEFWFilter
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/EFW_filter.h: indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include/EFW_filter.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include/EFW_filter.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libEFWFilter.a: $(LIBASIEFW) $(BUILD_INCLUDE)/EFW_filter.h
	install -d $(BUILD_LIB)
ifeq ($(OS_DETECTED),Darwin)
	lipo $(LIBASIEFW) -thin i386 -output /tmp/32.a
	lipo $(LIBASIEFW) -thin x86_64 -output /tmp/64.a
	ar d /tmp/32.a hid_mac.o
	ar d /tmp/64.a hid_mac.o
	lipo /tmp/32.a /tmp/64.a -create -output $(BUILD_LIB)/libEFWFilter.a
else
	ln -sf $(INDIGO_ROOT)/$(LIBASIEFW) $(BUILD_LIB)/libEFWFilter.a
endif

#---------------------------------------------------------------------
#
# Install libasicamera2
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/ASICamera2.h: indigo_drivers/ccd_asi/bin_externals/libasicamera/include/ASICamera2.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_asi/bin_externals/libasicamera/include/ASICamera2.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libASICamera2.a: $(BUILD_INCLUDE)/ASICamera2.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBASICAMERA) $(BUILD_LIB)


#---------------------------------------------------------------------
#
# Install libUSB2ST4Conv.a
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/USB2ST4_Conv.h: indigo_drivers/guider_asi/bin_externals/libusb2st4conv/include/USB2ST4_Conv.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/guider_asi/bin_externals/libusb2st4conv/include/USB2ST4_Conv.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libUSB2ST4Conv.a: $(BUILD_INCLUDE)/USB2ST4_Conv.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBASIST4) $(BUILD_LIB)


#---------------------------------------------------------------------
#
#	Install libfcusb
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libfcusb.h: indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libfcusb.a: $(LIBFCUSB) $(BUILD_INCLUDE)/libfcusb.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBFCUSB) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Install libdsusb
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libdsusb.h: indigo_drivers/aux_dsusb/bin_externals/libdsusb/include/libdsusb.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/aux_dsusb/bin_externals/libdsusb/include/libdsusb.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libdsusb.a: $(LIBDSUSB) $(BUILD_INCLUDE)/libdsusb.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBDSUSB) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Install libgpusb
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libgpusb.h: indigo_drivers/guider_gpusb/bin_externals/libgpusb/include/libgpusb.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/guider_gpusb/bin_externals/libgpusb/include/libgpusb.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libgpusb.a: $(LIBGPUSB) $(BUILD_INCLUDE)/libgpusb.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBGPUSB) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Build libnexstar
#
#---------------------------------------------------------------------

indigo_drivers/mount_nexstar/externals/libnexstar/configure: indigo_drivers/mount_nexstar/externals/libnexstar/configure.in
	cd indigo_drivers/mount_nexstar/externals/libnexstar; autoreconf -fiv; cd ../../../..

indigo_drivers/mount_nexstar/externals/libnexstar/Makefile: indigo_drivers/mount_nexstar/externals/libnexstar/configure
	cd indigo_drivers/mount_nexstar/externals/libnexstar; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS)"; cd ../../../..

$(BUILD_LIB)/libnexstar.a: indigo_drivers/mount_nexstar/externals/libnexstar/Makefile
	cd indigo_drivers/mount_nexstar/externals/libnexstar; make install; cd ../../../..

#---------------------------------------------------------------------
#
#	Build libapogee
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libapogee/ApogeeCam.h: indigo_drivers/ccd_apogee/externals/libapogee/ApogeeCam.h
	install -d $(BUILD_INCLUDE)
	install -d $(BUILD_INCLUDE)/libapogee
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_apogee/externals/libapogee/*.h $(BUILD_INCLUDE)/libapogee

$(BUILD_LIB)/libapogee.a:	$(BUILD_INCLUDE)/libapogee/ApogeeCam.h
	cd indigo_drivers/ccd_apogee/externals/libapogee; make clean; make; cd ../../../..
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_apogee/externals/libapogee/libapogee.a $(BUILD_LIB)
ifeq ($(OS_DETECTED),Linux)
	cd indigo_drivers/ccd_apogee/externals/boost_regex/build; make clean; make; cd ../../../..
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_apogee/externals/boost_regex/build/gcc/libboost_regex-gcc-1_53.a $(LIBBOOST-REGEX)
endif

#---------------------------------------------------------------------
#
#	Build libfli
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libfli.h: indigo_drivers/ccd_fli/externals/$(FLISDK)/libfli.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_fli/externals/$(FLISDK)/libfli.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libfli.a: $(BUILD_INCLUDE)/libfli.h
	cd indigo_drivers/ccd_fli/externals/$(FLISDK); make clean; make; cd ../../../..
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_fli/externals/$(FLISDK)/libfli.a $(BUILD_LIB)


#---------------------------------------------------------------------
#
#	Build libtoupcam
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/toupcam.h: indigo_drivers/ccd_touptek/bin_externals/libtoupcam/include/toupcam.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_touptek/bin_externals/libtoupcam/include/toupcam.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libtoupcam.$(SOEXT): $(BUILD_INCLUDE)/toupcam.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBTOUPCAM) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Build libaltaircam
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/altaircam.h: indigo_drivers/ccd_altair/bin_externals/libaltaircam/include/altaircam.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_altair/bin_externals/libaltaircam/include/altaircam.h $(BUILD_INCLUDE)

$(BUILD_LIB)/libaltaircam.$(SOEXT): $(BUILD_INCLUDE)/altaircam.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBALTAIRCAM) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	install sbigudrv
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)
$(BUILD_LIB)/libsbigudrv.a:
	mkdir sbig_scratch; cd sbig_scratch; cmake -DCMAKE_INSTALL_PREFIX=../build/ -DSKIP_FIRMWARE_INSTALL=True ../indigo_drivers/ccd_sbig/bin_externals/sbigudrv/; make install; cd ..; rm -rf sbig_scratch
endif

#---------------------------------------------------------------------
#
#	Initialize
#
#---------------------------------------------------------------------
SUBMODULES_FLAG=externals/SUBMODULES-UP-TO-DATE
submodule-init:
ifeq ("$(wildcard $(SUBMODULES_FLAG))","")
ifeq ("$(wildcard externals)","")
	mkdir externals
endif
ifeq ("$(wildcard externals/libusb)","")
	cd externals; git clone https://github.com/indigo-astronomy/libusb.git; cd ../..
else
	cd externals/libusb; git pull; cd ../..
endif
ifeq ("$(wildcard externals/hidapi)","")
	cd externals; git clone https://github.com/indigo-astronomy/hidapi.git; cd ../..
else
	cd externals/hidapi; git pull; cd ../..
endif
ifeq ("$(wildcard externals/novas)","")
	cd externals; git clone https://github.com/indigo-astronomy/novas.git; cd ../..
else
	cd externals/novas; git pull; cd ../..
endif
ifeq ("$(wildcard externals/libjpeg)","")
	cd externals; git clone https://github.com/indigo-astronomy/libjpeg.git; cd ../..
else
	cd externals/libjpeg; git pull; cd ../..
endif
ifeq ("$(wildcard indigo_drivers/ccd_iidc/externals)","")
	mkdir indigo_drivers/ccd_iidc/externals
endif
ifeq ("$(wildcard indigo_drivers/ccd_iidc/externals/libdc1394)","")
	cd indigo_drivers/ccd_iidc/externals; git clone https://github.com/indigo-astronomy/libdc1394.git; cd ../..
else
	cd indigo_drivers/ccd_iidc/externals/libdc1394; git pull; cd ../..
endif
ifeq ("$(wildcard indigo_drivers/mount_nexstar/externals)","")
	mkdir indigo_drivers/mount_nexstar/externals
endif
ifeq ("$(indigo_drivers/mount_nexstar/externals/libnexstar)","")
	cd indigo_drivers/mount_nexstar/externals; git clone https://github.com/indigo-astronomy/libnexstar.git; cd ../..
else
	cd indigo_drivers/mount_nexstar/externals/libnexstar; git pull; cd ../..
endif
	touch $(SUBMODULES_FLAG)
endif

init: submodule-init
	$(info -------------------- $(OS_DETECTED) build --------------------)
	$(info drivers: $(notdir $(DRIVERS)))
	install -d $(BUILD_ROOT)
	install -d $(BUILD_BIN)
	install -d $(BUILD_LIB)
	install -d $(BUILD_DRIVERS)
	install -d $(BUILD_INCLUDE)
	install -d $(BUILD_SHARE)/indigo
	install -d $(BUILD_SHARE)/indi
	cp indigo_libs/indigo_config.h indigo_libs/indigo_config.h.orig
	sed 's/INDIGO_BUILD.*/INDIGO_BUILD $(INDIGO_BUILD)/' indigo_libs/indigo_config.h.orig >indigo_libs/indigo_config.h
	cp INDIGO\ Server\ for\ macOS/Info.plist INDIGO\ Server\ for\ macOS/Info.plist.orig
	sed '/CFBundleVersion/ { n; s/>.*</>$(INDIGO_BUILD)</; }' INDIGO\ Server\ for\ macOS/Info.plist.orig >INDIGO\ Server\ for\ macOS/Info.plist
	cp indigo.pc indigo.pc.orig
	sed 's/Version.*/Version: 2.0-$(INDIGO_BUILD)/' indigo.pc.orig >indigo.pc

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

$(BUILD_LIB)/libindigo.a: $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c)))
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigo.$(SOEXT): $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c))) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libnovas.a
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

$(BUILD_DRIVERS)/indigo_ccd_simulator.a: indigo_drivers/ccd_simulator/indigo_ccd_simulator.o indigo_drivers/ccd_simulator/indigo_ccd_simulator_data.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_simulator: indigo_drivers/ccd_simulator/indigo_ccd_simulator_main.o $(BUILD_DRIVERS)/indigo_ccd_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_simulator.$(SOEXT): indigo_drivers/ccd_simulator/indigo_ccd_simulator.o indigo_drivers/ccd_simulator/indigo_ccd_simulator_data.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount simulator driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_mount_simulator.a: indigo_drivers/mount_simulator/indigo_mount_simulator.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_mount_simulator: indigo_drivers/mount_simulator/indigo_mount_simulator_main.o $(BUILD_DRIVERS)/indigo_mount_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_mount_simulator.$(SOEXT): indigo_drivers/mount_simulator/indigo_mount_simulator.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build GPS simulator driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_gps_simulator.a: indigo_drivers/gps_simulator/indigo_gps_simulator.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_gps_simulator: indigo_drivers/gps_simulator/indigo_gps_simulator_main.o $(BUILD_DRIVERS)/indigo_gps_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_gps_simulator.$(SOEXT): indigo_drivers/gps_simulator/indigo_gps_simulator.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Dome simulator driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_dome_simulator.a: indigo_drivers/dome_simulator/indigo_dome_simulator.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_dome_simulator: indigo_drivers/dome_simulator/indigo_dome_simulator_main.o $(BUILD_DRIVERS)/indigo_dome_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_dome_simulator.$(SOEXT): indigo_drivers/dome_simulator/indigo_dome_simulator.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build GPS NMEA driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_gps_nmea.a: indigo_drivers/gps_nmea/indigo_gps_nmea.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_gps_nmea: indigo_drivers/gps_nmea/indigo_gps_nmea_main.o $(BUILD_DRIVERS)/indigo_gps_nmea.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_gps_nmea.$(SOEXT): indigo_drivers/gps_nmea/indigo_gps_nmea.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount nexstar driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_mount_nexstar.a: indigo_drivers/mount_nexstar/indigo_mount_nexstar.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_mount_nexstar: indigo_drivers/mount_nexstar/indigo_mount_nexstar_main.o $(BUILD_DRIVERS)/indigo_mount_nexstar.a $(BUILD_LIB)/libnexstar.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_mount_nexstar.$(SOEXT): indigo_drivers/mount_nexstar/indigo_mount_nexstar.o $(BUILD_LIB)/libnexstar.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount lx200 driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_mount_lx200.a: indigo_drivers/mount_lx200/indigo_mount_lx200.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_mount_lx200: indigo_drivers/mount_lx200/indigo_mount_lx200_main.o $(BUILD_DRIVERS)/indigo_mount_lx200.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_mount_lx200.$(SOEXT): indigo_drivers/mount_lx200/indigo_mount_lx200.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount ioptron driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_mount_ioptron.a: indigo_drivers/mount_ioptron/indigo_mount_ioptron.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_mount_ioptron: indigo_drivers/mount_ioptron/indigo_mount_ioptron_main.o $(BUILD_DRIVERS)/indigo_mount_ioptron.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_mount_ioptron.$(SOEXT): indigo_drivers/mount_ioptron/indigo_mount_ioptron.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build CG-USB-ST4 guider driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_guider_cgusbst4.a: indigo_drivers/guider_cgusbst4/indigo_guider_cgusbst4.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_guider_cgusbst4: indigo_drivers/guider_cgusbst4/indigo_guider_cgusbst4_main.o $(BUILD_DRIVERS)/indigo_guider_cgusbst4.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_guider_cgusbst4.$(SOEXT): indigo_drivers/guider_cgusbst4/indigo_guider_cgusbst4.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount synscan driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_mount_synscan.a: indigo_drivers/mount_synscan/indigo_mount_synscan.o indigo_drivers/mount_synscan/indigo_mount_synscan_protocol.o indigo_drivers/mount_synscan/indigo_mount_synscan_driver.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_mount_synscan: indigo_drivers/mount_synscan/indigo_mount_synscan_main.o $(BUILD_DRIVERS)/indigo_mount_synscan.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_mount_synscan.$(SOEXT): indigo_drivers/mount_synscan/indigo_mount_synscan.o indigo_drivers/mount_synscan/indigo_mount_synscan_protocol.o indigo_drivers/mount_synscan/indigo_mount_synscan_driver.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build mount temma driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_mount_temma.a: indigo_drivers/mount_temma/indigo_mount_temma.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_mount_temma: indigo_drivers/mount_temma/indigo_mount_temma_main.o $(BUILD_DRIVERS)/indigo_mount_temma.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_mount_temma.$(SOEXT): indigo_drivers/mount_temma/indigo_mount_temma.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Apogee CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_apogee.a: indigo_drivers/ccd_apogee/indigo_ccd_apogee.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_apogee: indigo_drivers/ccd_apogee/indigo_ccd_apogee_main.o $(BUILD_DRIVERS)/indigo_ccd_apogee.a $(BUILD_LIB)/libapogee.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo -lcurl $(BUILD_LIB)/libapogee.a $(LIBBOOST-REGEX)

$(BUILD_DRIVERS)/indigo_ccd_apogee.$(SOEXT): indigo_drivers/ccd_apogee/indigo_ccd_apogee.o $(BUILD_LIB)/libapogee.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo -lcurl $(BUILD_LIB)/libapogee.a $(LIBBOOST-REGEX)

#---------------------------------------------------------------------
#
#	Build SX CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_sx.a: indigo_drivers/ccd_sx/indigo_ccd_sx.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_sx: indigo_drivers/ccd_sx/indigo_ccd_sx_main.o $(BUILD_DRIVERS)/indigo_ccd_sx.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_sx.$(SOEXT): indigo_drivers/ccd_sx/indigo_ccd_sx.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build SX filter wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_sx.a: indigo_drivers/wheel_sx/indigo_wheel_sx.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_sx: indigo_drivers/wheel_sx/indigo_wheel_sx_main.o $(BUILD_DRIVERS)/indigo_wheel_sx.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_sx.$(SOEXT): indigo_drivers/wheel_sx/indigo_wheel_sx.o $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build SSAG/QHY5 CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_ssag.a: indigo_drivers/ccd_ssag/indigo_ccd_ssag.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_ssag: indigo_drivers/ccd_ssag/indigo_ccd_ssag_main.o $(BUILD_DRIVERS)/indigo_ccd_ssag.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_ssag.$(SOEXT): indigo_drivers/ccd_ssag/indigo_ccd_ssag.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Meade DSI CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_dsi.a: indigo_drivers/ccd_dsi/indigo_ccd_dsi.o indigo_drivers/ccd_dsi/libdsi.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_dsi: indigo_drivers/ccd_dsi/indigo_ccd_dsi_main.o $(BUILD_DRIVERS)/indigo_ccd_dsi.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_dsi.$(SOEXT): indigo_drivers/ccd_dsi/indigo_ccd_dsi.o indigo_drivers/ccd_dsi/libdsi.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build ASI CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_asi.a: indigo_drivers/ccd_asi/indigo_ccd_asi.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_asi: indigo_drivers/ccd_asi/indigo_ccd_asi_main.o $(BUILD_DRIVERS)/indigo_ccd_asi.a $(BUILD_LIB)/libASICamera2.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

$(BUILD_DRIVERS)/indigo_ccd_asi.$(SOEXT): indigo_drivers/ccd_asi/indigo_ccd_asi.o $(BUILD_LIB)/libASICamera2.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

#---------------------------------------------------------------------
#
#	Build ATIK CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_atik.a: indigo_drivers/ccd_atik/indigo_ccd_atik.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_atik: indigo_drivers/ccd_atik/indigo_ccd_atik_main.o $(BUILD_DRIVERS)/indigo_ccd_atik.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo -latikcameras

$(BUILD_DRIVERS)/indigo_ccd_atik.$(SOEXT): indigo_drivers/ccd_atik/indigo_ccd_atik.o $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo -latikcameras

#---------------------------------------------------------------------
#
#	Build ATIK wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_atik.a: indigo_drivers/wheel_atik/indigo_wheel_atik.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_atik: indigo_drivers/wheel_atik/indigo_wheel_atik_main.o $(BUILD_DRIVERS)/indigo_wheel_atik.a $(BUILD_LIB)/libatik.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_atik.$(SOEXT): indigo_drivers/wheel_atik/indigo_wheel_atik.o $(BUILD_LIB)/libatik.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build MI CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_mi.a: indigo_drivers/ccd_mi/indigo_ccd_mi.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_mi: indigo_drivers/ccd_mi/indigo_ccd_mi_main.o $(BUILD_DRIVERS)/indigo_ccd_mi.a $(BUILD_LIB)/libgxccd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_mi.$(SOEXT): indigo_drivers/ccd_mi/indigo_ccd_mi.o $(BUILD_LIB)/libgxccd.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo


#---------------------------------------------------------------------
#
#	Build ASI filter wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_asi.a: indigo_drivers/wheel_asi/indigo_wheel_asi.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_asi: indigo_drivers/wheel_asi/indigo_wheel_asi_main.o $(BUILD_DRIVERS)/indigo_wheel_asi.a $(BUILD_LIB)/libEFWFilter.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

$(BUILD_DRIVERS)/indigo_wheel_asi.$(SOEXT): indigo_drivers/wheel_asi/indigo_wheel_asi.o $(BUILD_LIB)/libEFWFilter.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo


#---------------------------------------------------------------------
#
#	Build ASI USB2ST4 guider driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_guider_asi.a: indigo_drivers/guider_asi/indigo_guider_asi.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_guider_asi: indigo_drivers/guider_asi/indigo_guider_asi_main.o $(BUILD_DRIVERS)/indigo_guider_asi.a $(BUILD_LIB)/libUSB2ST4Conv.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

$(BUILD_DRIVERS)/indigo_guider_asi.$(SOEXT): indigo_drivers/guider_asi/indigo_guider_asi.o $(BUILD_LIB)/libUSB2ST4Conv.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo


#---------------------------------------------------------------------
#
#	Build QHY CCD driver (based on the official SDK)
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_qhy.a: indigo_drivers/ccd_qhy/indigo_ccd_qhy.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_qhy: indigo_drivers/ccd_qhy/indigo_ccd_qhy_main.o $(BUILD_DRIVERS)/indigo_ccd_qhy.a $(BUILD_LIB)/libqhy.a
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

$(BUILD_DRIVERS)/indigo_ccd_qhy.$(SOEXT): indigo_drivers/ccd_qhy/indigo_ccd_qhy.o $(BUILD_LIB)/libqhy.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

#---------------------------------------------------------------------
#
#	Build Shoestring FCUSB focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_fcusb.a: indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_fcusb: indigo_drivers/focuser_fcusb/indigo_focuser_fcusb_main.o $(BUILD_DRIVERS)/indigo_focuser_fcusb.a $(BUILD_LIB)/libfcusb.a  $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_fcusb.$(SOEXT): indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.o $(BUILD_LIB)/libfcusb.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Shoestring DSUSB shutter driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_aux_dsusb.a: indigo_drivers/aux_dsusb/indigo_aux_dsusb.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_aux_dsusb: indigo_drivers/aux_dsusb/indigo_aux_dsusb_main.o $(BUILD_DRIVERS)/indigo_aux_dsusb.a $(BUILD_LIB)/libdsusb.a  $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_aux_dsusb.$(SOEXT): indigo_drivers/aux_dsusb/indigo_aux_dsusb.o $(BUILD_LIB)/libdsusb.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Shoestring GPUSB guider driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_guider_gpusb.a: indigo_drivers/guider_gpusb/indigo_guider_gpusb.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_guider_gpusb: indigo_drivers/guider_gpusb/indigo_guider_gpusb_main.o $(BUILD_DRIVERS)/indigo_guider_gpusb.a $(BUILD_LIB)/libgpusb.a  $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_guider_gpusb.$(SOEXT): indigo_drivers/guider_gpusb/indigo_guider_gpusb.o $(BUILD_LIB)/libgpusb.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build IIDC CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_iidc.a: indigo_drivers/ccd_iidc/indigo_ccd_iidc.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_iidc: indigo_drivers/ccd_iidc/indigo_ccd_iidc_main.o $(BUILD_DRIVERS)/indigo_ccd_iidc.a $(BUILD_LIB)/libdc1394.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBRAW_1394) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_iidc.$(SOEXT): indigo_drivers/ccd_iidc/indigo_ccd_iidc.o $(BUILD_LIB)/libdc1394.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(LIBRAW_1394) -lindigo

#---------------------------------------------------------------------
#
#       Build gphoto2 CCD driver
#
#---------------------------------------------------------------------
# Note: indigo_ccd_gphoto2.c is temporally compilied with g++ due to linking problems
# of libraw on Ubuntu 18 with gcc/g++ 7.3.0

$(BUILD_DRIVERS)/indigo_ccd_gphoto2.a: indigo_linux_drivers/ccd_gphoto2/indigo_ccd_gphoto2.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_gphoto2: indigo_linux_drivers/ccd_gphoto2/indigo_ccd_gphoto2_main.o $(BUILD_DRIVERS)/indigo_ccd_gphoto2.a
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo -lgphoto2

$(BUILD_DRIVERS)/indigo_ccd_gphoto2.$(SOEXT): indigo_linux_drivers/ccd_gphoto2/indigo_ccd_gphoto2.o
	g++ -shared -o $@ $^ $(LDFLAGS) -lindigo -lgphoto2 $(BUILD_LIB)/libraw.a

#---------------------------------------------------------------------
#
#	Build FLI CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_fli.a: indigo_drivers/ccd_fli/indigo_ccd_fli.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_fli: indigo_drivers/ccd_fli/indigo_ccd_fli_main.o $(BUILD_DRIVERS)/indigo_ccd_fli.a $(BUILD_LIB)/libfli.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_fli.$(SOEXT): indigo_drivers/ccd_fli/indigo_ccd_fli.o $(BUILD_LIB)/libfli.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build FLI wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_fli.a: indigo_drivers/wheel_fli/indigo_wheel_fli.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_fli: indigo_drivers/wheel_fli/indigo_wheel_fli_main.o $(BUILD_DRIVERS)/indigo_wheel_fli.a $(BUILD_LIB)/libfli.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_fli.$(SOEXT): indigo_drivers/wheel_fli/indigo_wheel_fli.o $(BUILD_LIB)/libfli.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build FLI focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_fli.a: indigo_drivers/focuser_fli/indigo_focuser_fli.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_fli: indigo_drivers/focuser_fli/indigo_focuser_fli_main.o $(BUILD_DRIVERS)/indigo_focuser_fli.a $(BUILD_LIB)/libfli.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_fli.$(SOEXT): indigo_drivers/focuser_fli/indigo_focuser_fli.o $(BUILD_LIB)/libfli.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
# Build SBIG CCD driver
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)

indigo_drivers/ccd_sbig/indigo_ccd_sbig.o: $(BUILD_LIB)/libsbigudrv.a

$(BUILD_DRIVERS)/indigo_ccd_sbig.a: indigo_drivers/ccd_sbig/indigo_ccd_sbig.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_sbig: indigo_drivers/ccd_sbig/indigo_ccd_sbig_main.o $(BUILD_DRIVERS)/indigo_ccd_sbig.a $(BUILD_LIB)/libsbigudrv.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_sbig.$(SOEXT): indigo_drivers/ccd_sbig/indigo_ccd_sbig.o $(BUILD_LIB)/libsbigudrv.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

else ifeq ($(OS_DETECTED),Darwin)

$(BUILD_INCLUDE)/sbigudrv.h:
	install -d $(BUILD_INCLUDE)
	cp indigo_drivers/ccd_sbig/bin_externals/sbigudrv/include/sbigudrv.h $(BUILD_INCLUDE)

indigo_drivers/ccd_sbig/indigo_ccd_sbig.o: $(BUILD_INCLUDE)/sbigudrv.h

$(BUILD_DRIVERS)/indigo_ccd_sbig.a: indigo_drivers/ccd_sbig/indigo_ccd_sbig.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_sbig: indigo_drivers/ccd_sbig/indigo_ccd_sbig_main.o $(BUILD_DRIVERS)/indigo_ccd_sbig.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_sbig.$(SOEXT): indigo_drivers/ccd_sbig/indigo_ccd_sbig.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

endif

#---------------------------------------------------------------------
#
#	Build USB_Focus v3 focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_usbv3.a: indigo_drivers/focuser_usbv3/indigo_focuser_usbv3.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_usbv3: indigo_drivers/focuser_usbv3/indigo_focuser_usbv3_main.o $(BUILD_DRIVERS)/indigo_focuser_usbv3.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_usbv3.$(SOEXT): indigo_drivers/focuser_usbv3/indigo_focuser_usbv3.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build PegasusAstro DMFC focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_dmfc.a: indigo_drivers/focuser_dmfc/indigo_focuser_dmfc.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_dmfc: indigo_drivers/focuser_dmfc/indigo_focuser_dmfc_main.o $(BUILD_DRIVERS)/indigo_focuser_dmfc.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_dmfc.$(SOEXT): indigo_drivers/focuser_dmfc/indigo_focuser_dmfc.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build PegasusAstro UPB aux driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_aux_upb.a: indigo_drivers/aux_upb/indigo_aux_upb.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_aux_upb: indigo_drivers/aux_upb/indigo_aux_upb_main.o $(BUILD_DRIVERS)/indigo_aux_upb.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_aux_upb.$(SOEXT): indigo_drivers/aux_upb/indigo_aux_upb.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Rigel Systems nSTEP focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_nstep.a: indigo_drivers/focuser_nstep/indigo_focuser_nstep.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_nstep: indigo_drivers/focuser_nstep/indigo_focuser_nstep_main.o $(BUILD_DRIVERS)/indigo_focuser_nstep.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_nstep.$(SOEXT): indigo_drivers/focuser_nstep/indigo_focuser_nstep.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Rigel Systems nFOCUS focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_nfocus.a: indigo_drivers/focuser_nfocus/indigo_focuser_nfocus.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_nfocus: indigo_drivers/focuser_nfocus/indigo_focuser_nfocus_main.o $(BUILD_DRIVERS)/indigo_focuser_nfocus.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_nfocus.$(SOEXT): indigo_drivers/focuser_nfocus/indigo_focuser_nfocus.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	MoonLite focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_moonlite.a: indigo_drivers/focuser_moonlite/indigo_focuser_moonlite.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_moonlite: indigo_drivers/focuser_moonlite/indigo_focuser_moonlite_main.o $(BUILD_DRIVERS)/indigo_focuser_moonlite.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_moonlite.$(SOEXT): indigo_drivers/focuser_moonlite/indigo_focuser_moonlite.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	LakesideAstro focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_lakeside.a: indigo_drivers/focuser_lakeside/indigo_focuser_lakeside.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_lakeside: indigo_drivers/focuser_lakeside/indigo_focuser_lakeside_main.o $(BUILD_DRIVERS)/indigo_focuser_lakeside.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_lakeside.$(SOEXT): indigo_drivers/focuser_lakeside/indigo_focuser_lakeside.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	MJKZZ focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_mjkzz.a: indigo_drivers/focuser_mjkzz/indigo_focuser_mjkzz.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_mjkzz: indigo_drivers/focuser_mjkzz/indigo_focuser_mjkzz_main.o $(BUILD_DRIVERS)/indigo_focuser_mjkzz.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_mjkzz.$(SOEXT): indigo_drivers/focuser_mjkzz/indigo_focuser_mjkzz.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_mjkzz_bt.a: indigo_mac_drivers/focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_mjkzz_bt.dylib: indigo_mac_drivers/focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build RTS-on-COM aux driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_aux_rts.a: indigo_drivers/aux_rts/indigo_aux_rts.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_aux_rts: indigo_drivers/aux_rts/indigo_aux_rts_main.o $(BUILD_DRIVERS)/indigo_aux_rts.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_aux_rts.$(SOEXT): indigo_drivers/aux_rts/indigo_aux_rts.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build ToupTek CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_touptek.a: indigo_drivers/ccd_touptek/indigo_ccd_touptek.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_touptek: indigo_drivers/ccd_touptek/indigo_ccd_touptek_main.o $(BUILD_DRIVERS)/indigo_ccd_touptek.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo -ltoupcam

$(BUILD_DRIVERS)/indigo_ccd_touptek.$(SOEXT): indigo_drivers/ccd_touptek/indigo_ccd_touptek.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo -ltoupcam

#---------------------------------------------------------------------
#
#	Build AltairAstro CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_altair.a: indigo_drivers/ccd_altair/indigo_ccd_altair.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_altair: indigo_drivers/ccd_altair/indigo_ccd_altair_main.o $(BUILD_DRIVERS)/indigo_ccd_altair.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo -laltaircam

$(BUILD_DRIVERS)/indigo_ccd_altair.$(SOEXT): indigo_drivers/ccd_altair/indigo_ccd_altair.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo -laltaircam

#---------------------------------------------------------------------
#
#	Build WeMacro Rail driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_wemacro.a: indigo_drivers/focuser_wemacro/indigo_focuser_wemacro.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_wemacro: indigo_drivers/focuser_wemacro/indigo_focuser_wemacro_main.o $(BUILD_DRIVERS)/indigo_focuser_wemacro.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_wemacro.$(SOEXT): indigo_drivers/focuser_wemacro/indigo_focuser_wemacro.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_wemacro_bt.a: indigo_mac_drivers/focuser_wemacro_bt/indigo_focuser_wemacro_bt.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_wemacro_bt.dylib: indigo_mac_drivers/focuser_wemacro_bt/indigo_focuser_wemacro_bt.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo


#---------------------------------------------------------------------
#
#	Build ICA CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_ica.a: indigo_mac_drivers/ccd_ica/indigo_ccd_ica.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp_nikon.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp_canon.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp_sony.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_ica.dylib: indigo_mac_drivers/ccd_ica/indigo_ccd_ica.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp_nikon.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp_canon.o indigo_mac_drivers/ccd_ica/indigo_ica_ptp_sony.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build EQMac driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_guider_eqmac.a: indigo_mac_drivers/guider_eqmac/indigo_guider_eqmac.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_guider_eqmac.dylib: indigo_mac_drivers/guider_eqmac/indigo_guider_eqmac.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build HID Joystick driver
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
indigo_drivers/aux_joystick/indigo_aux_joystick.o: indigo_drivers/aux_joystick/indigo_aux_joystick.m
	$(CC) $(MFLAGS) -c -o $@ $< -Iindigo_drivers/aux_joystick/external/DDHidLib

$(BUILD_DRIVERS)/indigo_aux_joystick.a: indigo_drivers/aux_joystick/indigo_aux_joystick.o $(addsuffix .o, $(basename $(wildcard indigo_drivers/aux_joystick/external/DDHidLib/*.m)))
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_aux_joystick: indigo_drivers/aux_joystick/indigo_aux_joystick_main.o $(BUILD_DRIVERS)/indigo_aux_joystick.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_aux_joystick.$(SOEXT): indigo_drivers/aux_joystick/indigo_aux_joystick.o $(addsuffix .o, $(basename $(wildcard indigo_drivers/aux_joystick/external/DDHidLib/*.m)))
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo
endif
ifeq ($(OS_DETECTED),Linux)
indigo_drivers/aux_joystick/indigo_aux_joystick.o: indigo_drivers/aux_joystick/indigo_aux_joystick.c
	$(CC)  $(CFLAGS) -c -o $@ $<

$(BUILD_DRIVERS)/indigo_aux_joystick.a: indigo_drivers/aux_joystick/indigo_aux_joystick.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_aux_joystick: indigo_drivers/aux_joystick/indigo_aux_joystick_main.o $(BUILD_DRIVERS)/indigo_aux_joystick.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_aux_joystick.$(SOEXT): indigo_drivers/aux_joystick/indigo_aux_joystick.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo
endif

#---------------------------------------------------------------------
#
#	Install libftd2xx
#
#---------------------------------------------------------------------

$(BUILD_LIB)/libftd2xx.a:
	install -d $(BUILD_LIB)
ifeq ($(OS_DETECTED),Darwin)
	hdiutil attach -noverify -noautoopen $(BIN_EXTERNALS)/D2XX1.4.4.dmg
	cp /Volumes/release/D2XX/ftd2xx.h /Volumes/release/D2XX/WinTypes.h $(BUILD_INCLUDE)
	cp /Volumes/release/D2XX/libftd2xx.a $(BUILD_LIB)
	hdiutil detach /Volumes/release
	rm -rf /tmp/D2XX1.4.4.dmg
endif
ifeq ($(OS_DETECTED),Linux)
ifeq ($(ARCH_DETECTED),arm)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-arm-v6-hf-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-arm-v6-hf-1.4.6.tgz /tmp/release
endif
ifeq ($(ARCH_DETECTED),arm64)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-arm-v8-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-arm-v8-1.4.6.tgz /tmp/release
endif
ifeq ($(ARCH_DETECTED),x86)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-i386-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-i386-1.4.6.tgz /tmp/release
endif
ifeq ($(ARCH_DETECTED),x64)
	tar xvfz $(BIN_EXTERNALS)/libftd2xx-x86_64-1.4.6.tgz -C /tmp
	cp /tmp/release/ftd2xx.h $(BUILD_INCLUDE)
	cp /tmp/release/build/libftd2xx.a $(BUILD_LIB)
	rm -rf /tmp/libftd2xx-x86_64-1.4.6.tgz /tmp/release
endif
endif

#---------------------------------------------------------------------
#
#	Build libqsi
#
#---------------------------------------------------------------------

indigo_drivers/ccd_qsi/externals/qsiapi-7.6.0/Makefile:
	cd indigo_drivers/ccd_qsi/externals/qsiapi-7.6.0; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --libdir=$(INDIGO_ROOT)/$(BUILD_LIB) --enable-shared=no --enable-static=yes CFLAGS="$(CFLAGS)" --with-pic; cd ../../../..

$(BUILD_LIB)/libqsiapi.a: indigo_drivers/ccd_qsi/externals/qsiapi-7.6.0/Makefile
	cd indigo_drivers/ccd_qsi/externals/qsiapi-7.6.0/lib; make; make install; cd ../../../../..

#---------------------------------------------------------------------
#
#	Build QSI CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_qsi.a: $(BUILD_LIB)/libqsiapi.a $(BUILD_LIB)/libftd2xx.a indigo_drivers/ccd_qsi/indigo_ccd_qsi.o
	$(AR) $(ARFLAGS) $@ indigo_drivers/ccd_qsi/indigo_ccd_qsi.o

$(BUILD_DRIVERS)/indigo_ccd_qsi: indigo_drivers/ccd_qsi/indigo_ccd_qsi_main.o $(BUILD_DRIVERS)/indigo_ccd_qsi.a  $(BUILD_LIB)/libqsiapi.a $(BUILD_LIB)/libftd2xx.a
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

$(BUILD_DRIVERS)/indigo_ccd_qsi.$(SOEXT): indigo_drivers/ccd_qsi/indigo_ccd_qsi.o $(BUILD_LIB)/libqsiapi.a $(BUILD_LIB)/libftd2xx.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lstdc++ -lindigo

#---------------------------------------------------------------------
#
#	Build Snoop agent
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_agent_snoop.a: indigo_drivers/agent_snoop/indigo_agent_snoop.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_agent_snoop.$(SOEXT): indigo_drivers/agent_snoop/indigo_agent_snoop.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Imager agent
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_agent_imager.a: indigo_drivers/agent_imager/indigo_agent_imager.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_agent_imager.$(SOEXT): indigo_drivers/agent_imager/indigo_agent_imager.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build LX200 server agent
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_agent_lx200_server.a: indigo_drivers/agent_lx200_server/indigo_agent_lx200_server.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_agent_lx200_server.$(SOEXT): indigo_drivers/agent_lx200_server/indigo_agent_lx200_server.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Brightstar Quantum filter wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_quantum.a: indigo_drivers/wheel_quantum/indigo_wheel_quantum.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_quantum: indigo_drivers/wheel_quantum/indigo_wheel_quantum_main.o $(BUILD_DRIVERS)/indigo_wheel_quantum.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_quantum.$(SOEXT): indigo_drivers/wheel_quantum/indigo_wheel_quantum.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Trutek filter wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_trutek.a: indigo_drivers/wheel_trutek/indigo_wheel_trutek.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_trutek: indigo_drivers/wheel_trutek/indigo_wheel_trutek_main.o $(BUILD_DRIVERS)/indigo_wheel_trutek.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_trutek.$(SOEXT): indigo_drivers/wheel_trutek/indigo_wheel_trutek.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Xagyl filter wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_xagyl.a: indigo_drivers/wheel_xagyl/indigo_wheel_xagyl.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_xagyl: indigo_drivers/wheel_xagyl/indigo_wheel_xagyl_main.o $(BUILD_DRIVERS)/indigo_wheel_xagyl.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_xagyl.$(SOEXT): indigo_drivers/wheel_xagyl/indigo_wheel_xagyl.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Optec filter wheel driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_wheel_optec.a: indigo_drivers/wheel_optec/indigo_wheel_optec.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_wheel_optec: indigo_drivers/wheel_optec/indigo_wheel_optec_main.o $(BUILD_DRIVERS)/indigo_wheel_optec.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_wheel_optec.$(SOEXT): indigo_drivers/wheel_optec/indigo_wheel_optec.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build Optec focuser driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_focuser_optec.a: indigo_drivers/focuser_optec/indigo_focuser_optec.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_focuser_optec: indigo_drivers/focuser_optec/indigo_focuser_optec_main.o $(BUILD_DRIVERS)/indigo_focuser_optec.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_focuser_optec.$(SOEXT): indigo_drivers/focuser_optec/indigo_focuser_optec.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#-------------------------------------------------------------------
#
#	Build ASCOL system driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_system_ascol.a: indigo_drivers/system_ascol/indigo_system_ascol.o indigo_drivers/system_ascol/libascol/libascol.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_system_ascol: indigo_drivers/system_ascol/indigo_system_ascol_main.o $(BUILD_DRIVERS)/indigo_system_ascol.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_system_ascol.$(SOEXT): indigo_drivers/system_ascol/indigo_system_ascol.o indigo_drivers/system_ascol/libascol/libascol.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build tests
#
#---------------------------------------------------------------------

$(BUILD_BIN)/test: indigo_test/test.o $(BUILD_DRIVERS)/indigo_ccd_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_BIN)/client: indigo_test/client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

#---------------------------------------------------------------------
#
#	Build indigo_server
#
#---------------------------------------------------------------------

$(BUILD_BIN)/indigo_server: indigo_server/indigo_server.o $(SIMULATOR_LIBS) ctrlpanel
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server/indigo_server.o $(SIMULATOR_LIBS) $(LDFLAGS) -lstdc++ -lindigo
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif

$(BUILD_BIN)/indigo_server_standalone: indigo_server/indigo_server.c $(DRIVER_LIBS) $(BUILD_LIB)/libindigo.a $(EXTERNALS) ctrlpanel
	$(CC) -DSTATIC_DRIVERS $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server/indigo_server.c $(DRIVER_LIBS) $(se) $(BUILD_LIB)/libindigo.a $(EXTERNALS) $(LDFLAGS) $(LIBRAW_1394) -lstdc++  -lcurl -ltoupcam  -laltaircam
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif

#---------------------------------------------------------------------
#
#       Build indigo_prop_tool
#
#---------------------------------------------------------------------

$(BUILD_BIN)/indigo_prop_tool: indigo_tools/indigo_prop_tool.o
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_tools/indigo_prop_tool.o $(LDFLAGS) -lindigo
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	#install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif


#---------------------------------------------------------------------
#
#	Control panel
#
#---------------------------------------------------------------------
ctrlpanel: indigo_server/ctrl.data indigo_server/resource/angular.min.js.data indigo_server/resource/bootstrap.min.js.data indigo_server/resource/bootstrap.css.data indigo_server/resource/jquery.min.js.data indigo_server/resource/glyphicons-halflings-regular.ttf.data indigo_server/resource/logo.png.data


indigo_server/ctrl.data:	indigo_server/ctrl.html
#	python tools/rjsmin.py <indigo_server/ctrl.html | gzip | hexdump -v -e '1/1 "0x%02x, "' >indigo_server/ctrl.data
	cat indigo_server/ctrl.html | gzip | hexdump -v -e '1/1 "0x%02x, "' >indigo_server/ctrl.data

indigo_server/resource/angular.min.js.data:	indigo_server/resource/angular.min.js
	cat indigo_server/resource/angular.min.js | gzip | hexdump -v -e '1/1 "0x%02x, "' > indigo_server/resource/angular.min.js.data

indigo_server/resource/bootstrap.min.js.data:	indigo_server/resource/bootstrap.min.js
	cat indigo_server/resource/bootstrap.min.js | gzip | hexdump -v -e '1/1 "0x%02x, "' > indigo_server/resource/bootstrap.min.js.data

indigo_server/resource/bootstrap.css.data:	indigo_server/resource/bootstrap.css
	cat indigo_server/resource/bootstrap.css | gzip | hexdump -v -e '1/1 "0x%02x, "' > indigo_server/resource/bootstrap.css.data

indigo_server/resource/jquery.min.js.data:	indigo_server/resource/jquery.min.js
	cat indigo_server/resource/jquery.min.js | gzip | hexdump -v -e '1/1 "0x%02x, "' > indigo_server/resource/jquery.min.js.data

indigo_server/resource/glyphicons-halflings-regular.ttf.data:	indigo_server/resource/glyphicons-halflings-regular.ttf
	cat indigo_server/resource/glyphicons-halflings-regular.ttf | gzip | hexdump -v -e '1/1 "0x%02x, "' > indigo_server/resource/glyphicons-halflings-regular.ttf.data

indigo_server/resource/logo.png.data:	indigo_server/resource/logo.png
	cat indigo_server/resource/logo.png | gzip | hexdump -v -e '1/1 "0x%02x, "' > indigo_server/resource/logo.png.data


#---------------------------------------------------------------------
#
#	Install
#
#---------------------------------------------------------------------

install:
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_server $(INSTALL_PREFIX)/bin
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_server_standalone $(INSTALL_PREFIX)/bin
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_prop_tool $(INSTALL_PREFIX)/bin
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_drivers $(INSTALL_PREFIX)/bin
	sudo install -D -m 0755 $(DRIVERS) $(INSTALL_PREFIX)/bin
	sudo install -D -m 0644 $(BUILD_LIB)/libindigo.$(SOEXT) $(INSTALL_PREFIX)/lib
	sudo install -D -m 0644 $(BUILD_LIB)/libtoupcam.$(SOEXT) $(INSTALL_PREFIX)/lib
	sudo install -D -m 0644 $(BUILD_LIB)/libaltaircam.$(SOEXT) $(INSTALL_PREFIX)/lib
	sudo install -D -m 0644 $(DRIVER_SOLIBS) $(INSTALL_PREFIX)/lib
	mkdir sbig_scratch; cd sbig_scratch; cmake -DCMAKE_INSTALL_PREFIX=/ -DSKIP_LIBS_INSTALL="True" ../indigo_drivers/ccd_sbig/bin_externals/sbigudrv/; make install; cd ..; rm -rf sbig_scratch
	mkdir qhy_scratch; cd qhy_scratch; cmake -DCMAKE_INSTALL_PREFIX=/ -DSKIP_LIBS_INSTALL="True" ../indigo_drivers/ccd_qhy/bin_externals/qhyccd/; make install; cd ..; rm -rf qhy_scratch
	cd indigo_drivers/ccd_apogee/externals/libapogee; make install-config CONFIG_PREFIX=/etc/apogee RULES_PREFIX=/lib/udev/rules.d; cd ../../../../
	sudo install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /lib/udev/rules.d/99-indigo_ccd_sx.rules
	sudo install -D -m 0644 indigo_drivers/ccd_fli/indigo-fli.rules /lib/udev/rules.d/99-indigo_fli.rules
	sudo install -D -m 0644 indigo_drivers/ccd_atik/indigo_ccd_atik.rules /lib/udev/rules.d/99-indigo_ccd_atik.rules
	sudo install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /lib/udev/rules.d/99-indigo_ccd_ssag.rules
	sudo install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules /lib/udev/rules.d/99-indigo_ccd_asi.rules
	sudo install -D -m 0644 indigo_drivers/ccd_dsi/99-meadedsi.rules /lib/udev/rules.d/99-meadedsi.rules
	sudo install -D -m 0644 indigo_drivers/ccd_dsi/meade-deepskyimager.hex /lib/firmware/meade-deepskyimager.hex
	sudo install -D -m 0644 indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/99-efw.rules /lib/udev/rules.d/99-indigo_wheel_asi.rules
	sudo install -D -m 0644 indigo_drivers/guider_asi/bin_externals/libusb2st4conv/lib/USB2ST4.rules /lib/udev/rules.d/99-indigo_guider_asi.rules
	sudo install -D -m 0644 indigo_drivers/focuser_usbv3/indigo_focuser_usbv3.rules /lib/udev/rules.d/99-indigo_focuser_usbv3.rules
	sudo install -D -m 0644 indigo_drivers/focuser_wemacro/indigo_focuser_wemacro.rules /lib/udev/rules.d/99-indigo_focuser_wemacro.rules
	sudo install -D -m 0644 indigo_drivers/ccd_mi/indigo_ccd_mi.rules /lib/udev/rules.d/99-indigo_ccd_mi.rules
	sudo install -D -m 0644 indigo_drivers/ccd_touptek/bin_externals/libtoupcam/99-toupcam.rules /lib/udev/rules.d/99-indigo_ccd_toupcam.rules
	sudo install -D -m 0644 indigo_drivers/ccd_altair/bin_externals/libaltaircam/99-altaircam.rules /lib/udev/rules.d/99-indigo_ccd_altaircam.rules
	sudo install -D -m 0644 indigo_drivers/aux_upb/99-indigo_aux_upb.rules /lib/udev/rules.d/99-indigo_aux_upb.rules
	sudo udevadm control --reload-rules
	sudo install -D /usr/share/indi
	sudo install -m 0644 $(BUILD_SHARE)/indi/indi_go_*.xml /usr/share/indi

#---------------------------------------------------------------------
#
#	Uninstall
#
#---------------------------------------------------------------------

uninstall:
	sudo rm -f $(INSTALL_PREFIX)/bin/indigo_*
	sudo rm -f $(INSTALL_PREFIX)/lib/indigo_*
	sudo rm -f $(INSTALL_PREFIX)/lib/libindigo*
	sudo rm -f $(INSTALL_PREFIX)/lib/pkgconfig/indigo.pc
	sudo rm -f $(INSTALL_PREFIX)/lib/libtoupcam.$(SOEXT)
	sudo rm -f $(INSTALL_PREFIX)/lib/libaltaircam.$(SOEXT)
	sudo rm -f /lib/udev/rules.d/99-indigo_*.rules
	sudo rm -f /lib/udev/rules.d/51-sbig-debian.rules
	sudo rm -f /lib/udev/rules.d/85-qhyccd.rules
	sudo rm -f /lib/udev/rules.d/99-meadedsi.rules
	sudo rm -f /lib/firmware/sbigfcam.hex
	sudo rm -f /lib/firmware/sbiglcam.hex
	sudo rm -f /lib/firmware/sbigpcam.hex
	sudo rm -f /lib/firmware/sbigucam.hex
	sudo rm -f /lib/firmware/stfga.bin
	sudo rm -rf /lib/firmware/qhy
	sudo rm -rf /usr/local/etc/apogee /etc/apogee
	sudo rm -f /lib/firmware/meade-deepskyimager.hex
	sudo udevadm control --reload-rules
	sudo rm -rf /usr/share/indi/indi_indigo.xml /usr/share/indi/indi_go_*.xml


#---------------------------------------------------------------------
#
#	Build packages
#
#---------------------------------------------------------------------

REWRITE_DEBS="libsbigudrv2,libsbig,libqhy,indi-dsi,indigo-upb"
package: $(PACKAGE_NAME).deb

package-prepare: all
	install -d /tmp/$(PACKAGE_NAME)/usr/bin
	install -d /tmp/$(PACKAGE_NAME)/usr/lib
	install -d /tmp/$(PACKAGE_NAME)/usr/include/indigo
	install -d /tmp/$(PACKAGE_NAME)/usr/share
	install $(BUILD_BIN)/indigo_server /tmp/$(PACKAGE_NAME)/usr/bin
	install $(BUILD_BIN)/indigo_server_standalone /tmp/$(PACKAGE_NAME)/usr/bin
	install $(BUILD_BIN)/indigo_prop_tool /tmp/$(PACKAGE_NAME)/usr/bin
	install $(DRIVERS) /tmp/$(PACKAGE_NAME)/usr/bin
	install $(BUILD_LIB)/libindigo.so /tmp/$(PACKAGE_NAME)/usr/lib
	install $(BUILD_LIB)/libindigo.a /tmp/$(PACKAGE_NAME)/usr/lib
	install $(DRIVER_LIBS) /tmp/$(PACKAGE_NAME)/usr/lib
	install $(DRIVER_SOLIBS) /tmp/$(PACKAGE_NAME)/usr/lib
	install $(BUILD_LIB)/libtoupcam.$(SOEXT) /tmp/$(PACKAGE_NAME)/usr/lib
	install $(BUILD_LIB)/libaltaircam.$(SOEXT) /tmp/$(PACKAGE_NAME)/usr/lib
	mkdir sbig_scratch; cd sbig_scratch; cmake -DCMAKE_INSTALL_PREFIX=/tmp/$(PACKAGE_NAME) -DSKIP_LIBS_INSTALL="True" ../indigo_drivers/ccd_sbig/bin_externals/sbigudrv/; make install; cd ..; rm -rf sbig_scratch
	mkdir qhy_scratch; cd qhy_scratch; cmake -DCMAKE_INSTALL_PREFIX=/tmp/$(PACKAGE_NAME) -DSKIP_LIBS_INSTALL="True" ../indigo_drivers/ccd_qhy/bin_externals/qhyccd/; make install; cd ..; rm -rf qhy_scratch
	cd indigo_drivers/ccd_apogee/externals/libapogee; make install-config CONFIG_PREFIX=/tmp/$(PACKAGE_NAME)/etc/apogee RULES_PREFIX=/tmp/$(PACKAGE_NAME)/lib/udev/rules.d; cd ../../../../
	install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_sx.rules
	install -D -m 0644 indigo_drivers/ccd_fli/indigo-fli.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_fli.rules
	install -D -m 0644 indigo_drivers/ccd_atik/indigo_ccd_atik.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_atik.rules
	install -D -m 0644 indigo_drivers/ccd_dsi/99-meadedsi.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-meadedsi.rules
	install -D -m 0644 indigo_drivers/ccd_dsi/meade-deepskyimager.hex /tmp/$(PACKAGE_NAME)/lib/firmware/meade-deepskyimager.hex
	install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_ssag.rules
	install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_asi.rules
	install -D -m 0644 indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/99-efw.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_wheel_asi.rules
	install -D -m 0644 indigo_drivers/guider_asi/bin_externals/libusb2st4conv/lib/USB2ST4.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_guider_asi.rules
	install -D -m 0644 indigo_drivers/focuser_usbv3/indigo_focuser_usbv3.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_focuser_usbv3.rules
	install -D -m 0644 indigo_drivers/focuser_wemacro/indigo_focuser_wemacro.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_focuser_wemacro.rules
	sudo install -D -m 0644 indigo_drivers/ccd_mi/indigo_ccd_mi.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_mi.rules
	sudo install -D -m 0644 indigo_drivers/ccd_touptek/bin_externals/libtoupcam/99-toupcam.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_toupcam.rules
	sudo install -D -m 0644 indigo_drivers/ccd_altair/bin_externals/libaltaircam/99-altaircam.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_altaircam.rules
	sudo install -D -m 0644 indigo_drivers/aux_upb/99-indigo_aux_upb.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_aux_upb.rules
	install -m 0644 indigo_libs/indigo_*.h /tmp/$(PACKAGE_NAME)/usr/include/indigo
	install -D -m 0644 indigo.pc /tmp/$(PACKAGE_NAME)/usr/lib/pkgconfig/indigo.pc
	cp -r $(BUILD_SHARE)/* /tmp/$(PACKAGE_NAME)/usr/share

$(PACKAGE_NAME).deb: package-prepare
	rm -f $(PACKAGE_NAME).deb
	install -d /tmp/$(PACKAGE_NAME)/DEBIAN
	printf "Package: indigo\n" > /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Version: $(INDIGO_VERSION)-$(INDIGO_BUILD)\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Installed-Size: $(shell echo $$((`du -s /tmp/$(PACKAGE_NAME) | cut -f1`)))\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Priority: optional\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Architecture: $(DEBIAN_ARCH)\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Replaces: $(REWRITE_DEBS)\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Maintainer: CloudMakers, s. r. o. <indigo@cloudmakers.eu>\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Homepage: http://www.indigo-astronomy.org\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Depends: fxload, libusb-1.0-0, libgudev-1.0-0, libgphoto2-6, libavahi-compat-libdnssd1\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "Description: INDIGO Framework and drivers\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf " INDIGO is a system of standards and frameworks for multiplatform and distributed astronomy software development designed to scale with your needs.\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/control
	cat /tmp/$(PACKAGE_NAME)/DEBIAN/control
	printf "echo Remove pre-2.0-76 build if any\n" > /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/bin/indigo_*\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/indigo_*\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/libindigo*\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/pkgconfig/indigo.pc\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/libtoupcam.$(SOEXT)\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/libaltaircam.$(SOEXT)\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	printf "sudo rm -rf /usr/local/etc/apogee\n" >> /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	cat /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	chmod a+x /tmp/$(PACKAGE_NAME)/DEBIAN/preinst
	sudo chown root /tmp/$(PACKAGE_NAME)
	dpkg --build /tmp/$(PACKAGE_NAME)
	mv /tmp/$(PACKAGE_NAME).deb .
	sudo rm -rf /tmp/$(PACKAGE_NAME)

packages: package fliusb-package

fliusb-package:
	cd indigo_drivers/ccd_fli/externals/fliusb-1.3 && make package && cd ../../../..
	cp indigo_drivers/ccd_fli/externals/fliusb-1.3/*.deb .
	rm indigo_drivers/ccd_fli/externals/fliusb-1.3/*.deb

#---------------------------------------------------------------------
#
#	Clean indigo build
#
#---------------------------------------------------------------------

clean: init
	rm -rf $(BUILD_ROOT)/bin/indigo*
	rm -rf $(BUILD_ROOT)/bin/test
	rm -rf $(BUILD_ROOT)/bin/client
	rm -rf $(BUILD_LIB)/libindigo*
	rm -rf $(BUILD_ROOT)/drivers
	rm -rf $(BUILD_ROOT)/share
	rm -f indigo_libs/*.o
	rm -f indigo_server/*.o
	rm -f indigo_server/*.data
	rm -f indigo_server/resource/*.data
	rm -f $(wildcard indigo_drivers/*/*.o)
	rm -f $(wildcard indigo_mac_drivers/*/*.o)
	rm -f $(wildcard indigo_test/*.o)

#---------------------------------------------------------------------
#
#	Clean indigo & externals build
#
#---------------------------------------------------------------------

clean-all: clean
	rm -rf $(BUILD_ROOT)
	rm -rf externals
	cd indigo_drivers/ccd_iidc/externals/libdc1394; make maintainer-clean; rm configure; cd ../../../..
	cd indigo_drivers/mount_nexstar/externals/libnexstar; make maintainer-clean; rm configure; cd ../../../..
	cd indigo_drivers/ccd_fli/externals/libfli-1.999.1-180223; make clean; cd ../../../..
	cd indigo_drivers/ccd_apogee/externals/libapogee; make clean; cd ../../../..
ifeq ($(OS_DETECTED),Linux)
	cd indigo_linux_drivers/ccd_gphoto2/externals/libraw; make maintainer-clean; rm configure; cd ../../../..
endif

#---------------------------------------------------------------------

remote:
	ssh ubuntu32.local "cd indigo; git reset --hard; git pull; make clean; make; sudo make package"
	scp ubuntu32.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-i386.deb .
	ssh ubuntu64.local "cd indigo; git reset --hard; git pull; make clean; make; sudo make package"
	scp ubuntu64.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-amd64.deb .
	ssh raspi32.local "cd indigo; git reset --hard; git pull; make clean; make; sudo make package"
	scp raspi32.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-armhf.deb .
	ssh raspi64.local "cd indigo; git reset --hard; git pull; make clean; make; sudo make package"
	scp raspi64.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-arm64.deb .

#---------------------------------------------------------------------
#
#	Remote build on ubuntu32.local, ubuntu64.local, raspi32.local and raspi64.local
#
# https://www.aptly.info
#
#---------------------------------------------------------------------

init-repo:
	aptly repo create -distribution=indigo -component=main indigo-release

publish:
	rm -f ~/Desktop/public
	aptly repo remove indigo-release indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-i386 indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-amd64 indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-armhf indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-arm64
	aptly repo add indigo-release indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-i386.deb indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-amd64.deb indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-armhf.deb indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-arm64.deb
	aptly repo show -with-packages indigo-release
	aptly publish -force-drop drop indigo
	aptly publish repo indigo-release
	ln -s ~/.aptly/public ~/Desktop


