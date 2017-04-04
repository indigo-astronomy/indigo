#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

INDIGO_VERSION := 2.0
INDIGO_BUILD := 44
INDIGO_ROOT := $(shell pwd)

ENABLE_STATIC=yes
ENABLE_SHARED=yes

BUILD_ROOT=build
BUILD_BIN=$(BUILD_ROOT)/bin
BUILD_DRIVERS=$(BUILD_ROOT)/drivers
BUILD_LIB=$(BUILD_ROOT)/lib
BUILD_INCLUDE=$(BUILD_ROOT)/include
BUILD_SHARE=$(BUILD_ROOT)/share

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
	LIBUSB_CFLAGS=LIBUSB_CFLAGS="-I$(INDIGO_ROOT)/$(BUILD_INCLUDE)/libusb-1.0"
	LIBUSB_LIBS=LIBUSB_LIBS="-L$(INDIGO_ROOT)/$(BUILD_LIB) -lusb-1.0"
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
	INSTALL_PREFIX=/usr/local
	PACKAGE_TYPE=deb
endif

#---------------------------------------------------------------------
#
#	Darwin/macOS parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Darwin)
	CC=gcc
	CFLAGS=-fPIC -O3 -Iindigo_libs -Iindigo_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS
	LDFLAGS=-framework Cocoa -framework CoreFoundation -framework IOKit -lobjc  -L$(BUILD_LIB) -lusb-1.0
	LIBHIDAPI=$(BUILD_LIB)/libhidapi.a
	SOEXT=dylib
	AR=ar
	ARFLAGS=-rv
	EXTERNALS=$(BUILD_LIB)/libusb-1.0.$(SOEXT) $(LIBHIDAPI) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libatik.a $(BUILD_LIB)/libqhy.a $(BUILD_LIB)/libfcusb.a $(BUILD_LIB)/libnovas.a $(BUILD_LIB)/libEFWFilter.a $(BUILD_LIB)/libASICamera2.a $(BUILD_LIB)/libdc1394.a $(BUILD_LIB)/libnexstar.a $(BUILD_LIB)/libfli.a
endif

#---------------------------------------------------------------------
#
#	Linux parameters
#
#---------------------------------------------------------------------

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	ifeq ($(ARCH_DETECTED),arm)
		CFLAGS=-g -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -Iindigo_libs -Iindigo_drivers -I$(BUILD_INCLUDE)  -std=gnu11 -pthread -DINDIGO_LINUX
	else
		CFLAGS=-g -fPIC -O3 -Iindigo_libs -Iindigo_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX
	endif
	LDFLAGS=-lm -lrt -lusb-1.0 -ldl -ludev -ldns_sd -L$(BUILD_LIB) -Wl,-rpath=\$$ORIGIN/../lib,-rpath=\$$ORIGIN/../drivers,-rpath=.
	SOEXT=so
	LIBHIDAPI=$(BUILD_LIB)/libhidapi-hidraw.a
	AR=ar
	ARFLAGS=-rv
	EXTERNALS=$(LIBHIDAPI) $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libatik.a $(BUILD_LIB)/libqhy.a $(BUILD_LIB)/libfcusb.a $(BUILD_LIB)/libnovas.a $(BUILD_LIB)/libEFWFilter.a $(BUILD_LIB)/libASICamera2.a $(BUILD_LIB)/libdc1394.a $(BUILD_LIB)/libnexstar.a $(BUILD_LIB)/libfli.a
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
	$(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/mount_*)))

DRIVER_LIBS=\
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))) \
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/mount_*))))

DRIVER_SOLIBS=\
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/ccd_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/wheel_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/focuser_*)))) \
	$(addsuffix .$(SOEXT), $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/mount_*))))

SIMULATOR_LIBS=\
	$(addsuffix .a, $(addprefix $(BUILD_DRIVERS)/indigo_, $(notdir $(wildcard indigo_drivers/*_simulator))))

SO_LIBS= $(wildcard $(BUILD_LIB)/*.$(SOEXT))

.PHONY: init clean macfixpath

#---------------------------------------------------------------------
#
#	Default target
#
#---------------------------------------------------------------------

all: init $(EXTERNALS) $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libindigo.$(SOEXT) indigo_server/ctrl.data drivers $(BUILD_BIN)/indigo_server_standalone $(BUILD_BIN)/indigo_prop_tool $(BUILD_BIN)/test $(BUILD_BIN)/client $(BUILD_BIN)/indigo_server macfixpath

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
	cd externals/libusb; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) --with-pic; cd ../..

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
	cd externals/hidapi; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) --with-pic; cd ../..

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
	cd indigo_drivers/ccd_iidc/externals/libdc1394; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --disable-libraw1394 --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS) $(UINT)" $(LIBUSB_CFLAGS) $(LIBUSB_LIBS); cd ../../../..

$(BUILD_LIB)/libdc1394.a: indigo_drivers/ccd_iidc/externals/libdc1394/Makefile
	cd indigo_drivers/ccd_iidc/externals/libdc1394; make install; cd ../../../..

#---------------------------------------------------------------------
#
#	Build libnovas
#
#---------------------------------------------------------------------

$(BUILD_LIB)/libnovas.a: externals/novas/novas.o externals/novas/eph_manager.o externals/novas/novascon.o externals/novas/nutation.o externals/novas/readeph0.o  externals/novas/solsys1.o  externals/novas/solsys3.o
	$(AR) $(ARFLAGS) $@ $^
	install externals/novas/JPLEPH.421 $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Build libjpeg
#
#---------------------------------------------------------------------

externals/libjpeg/Makefile: indigo_drivers/ccd_iidc/externals/libdc1394/configure
	cd externals/libjpeg; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS)"; cd ../..

$(BUILD_LIB)/libjpeg.a: externals/libjpeg/Makefile
	cd externals/libjpeg; make install; cd ../..

#---------------------------------------------------------------------
#
#	Install libatik
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libatik/libatik.h: indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik/libatik.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_atik/bin_externals/libatik/include/libatik $(BUILD_INCLUDE)

$(BUILD_LIB)/libatik.a: $(BUILD_INCLUDE)/libatik/libatik.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBATIK) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Install libqhy
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libqhy/libqhy.h: indigo_drivers/ccd_qhy/externals/libqhy/include/libqhy/libqhy.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/ccd_qhy/externals/libqhy/include/libqhy $(BUILD_INCLUDE)

$(BUILD_LIB)/libqhy.a: $(BUILD_INCLUDE)/libqhy/libqhy.h
	cd indigo_drivers/ccd_qhy/externals/libqhy; make clean; make; cd ../../../..
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBQHY) $(BUILD_LIB)


#---------------------------------------------------------------------
#
#	Install libEFWFilter
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/asi_efw/EFW_filter.h: indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include/EFW_filter.h
	install -d $(BUILD_INCLUDE)/asi_efw
	cp indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include/EFW_filter.h $(BUILD_INCLUDE)/asi_efw

$(BUILD_LIB)/libEFWFilter.a: $(BUILD_INCLUDE)/asi_efw/EFW_filter.h
	install -d $(BUILD_LIB)
	cp $(LIBASIEFW) $(BUILD_LIB)


#---------------------------------------------------------------------
#
# Install libasicamera2
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/asi_ccd/ASICamera2.h: indigo_drivers/ccd_asi/bin_externals/libasicamera/include/ASICamera2.h
	install -d $(BUILD_INCLUDE)/asi_ccd
	cp indigo_drivers/ccd_asi/bin_externals/libasicamera/include/ASICamera2.h $(BUILD_INCLUDE)/asi_ccd

$(BUILD_LIB)/libASICamera2.a: $(BUILD_INCLUDE)/asi_ccd/ASICamera2.h
	install -d $(BUILD_LIB)
	cp $(LIBASICAMERA) $(BUILD_LIB)


#---------------------------------------------------------------------
#
#	Install libfcusb
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libfcusb/libfcusb.h: indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb/libfcusb.h
	install -d $(BUILD_INCLUDE)
	ln -sf $(INDIGO_ROOT)/indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include/libfcusb $(BUILD_INCLUDE)

$(BUILD_LIB)/libfcusb.a: $(BUILD_INCLUDE)/libfcusb/libfcusb.h
	install -d $(BUILD_LIB)
	ln -sf $(INDIGO_ROOT)/$(LIBFCUSB) $(BUILD_LIB)

#---------------------------------------------------------------------
#
#	Build libnexstar
#
#---------------------------------------------------------------------

indigo_drivers/mount_nexstar/externals/libnexstar/configure: indigo_drivers/mount_nexstar/externals/libnexstar/configure.in
	cd indigo_drivers/mount_nexstar/externals/libnexstar; autoreconf -fiv; cd ../../../..

indigo_drivers/mount_nexstar/externals/libnexstar/Makefile: indigo_drivers/mount_nexstar/externals/libnexstar/configure
	cd indigo_drivers/mount_nexstar/externals/libnexstar; ./configure --prefix=$(INDIGO_ROOT)/$(BUILD_ROOT) --enable-shared=$(ENABLE_SHARED) --enable-static=$(ENABLE_STATIC) CFLAGS="$(CFLAGS)"; cd ../../../..

$(BUILD_LIB)/libnexstar.a: indigo_drivers/mount_nexstar/externals/libnexstar/Makefile
	cd indigo_drivers/mount_nexstar/externals/libnexstar; make install; cd ../../../..

#---------------------------------------------------------------------
#
#	Build libfli
#
#---------------------------------------------------------------------

$(BUILD_INCLUDE)/libfli/libfli.h: indigo_drivers/ccd_fli/externals/libfli-1.104/libfli.h
	install -d $(BUILD_INCLUDE)
	install -d $(BUILD_INCLUDE)/libfli
	cp indigo_drivers/ccd_fli/externals/libfli-1.104/libfli.h $(BUILD_INCLUDE)/libfli

$(BUILD_LIB)/libfli.a: $(BUILD_INCLUDE)/libfli/libfli.h
	cd indigo_drivers/ccd_fli/externals/libfli-1.104; make clean; make; cd ../../../..
	install -d $(BUILD_LIB)
	cp indigo_drivers/ccd_fli/externals/libfli-1.104/libfli.a $(BUILD_LIB)


#---------------------------------------------------------------------
#
#	indtall sbigudrv
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

init:
	$(info -------------------- $(OS_DETECTED) build --------------------)
	$(info drivers: $(notdir $(DRIVERS)))
	git submodule update --init --recursive
	install -d $(BUILD_ROOT)
	install -d $(BUILD_BIN)
	install -d $(BUILD_LIB)
	install -d $(BUILD_DRIVERS)
	install -d $(BUILD_INCLUDE)
	install -d $(BUILD_SHARE)/indigo
	cp indigo_libs/indigo_config.h indigo_libs/indigo_config.h.orig
	sed 's/INDIGO_BUILD.*/INDIGO_BUILD $(INDIGO_BUILD)/' indigo_libs/indigo_config.h.orig >indigo_libs/indigo_config.h
	cp INDIGO\ Server\ for\ macOS/Info.plist INDIGO\ Server\ for\ macOS/Info.plist.orig
	sed '/CFBundleVersion/ { n; s/>.*</>$(INDIGO_BUILD)</; }' INDIGO\ Server\ for\ macOS/Info.plist.orig >INDIGO\ Server\ for\ macOS/Info.plist

#---------------------------------------------------------------------
#
#	Build libindigo
#
#---------------------------------------------------------------------

$(BUILD_LIB)/libindigo.a: $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c)))
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_LIB)/libindigo.$(SOEXT): $(addsuffix .o, $(basename $(wildcard indigo_libs/*.c))) $(BUILD_LIB)/libjpeg.a
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

$(BUILD_DRIVERS)/indigo_ccd_simulator.a: indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_simulator: indigo_drivers/ccd_simulator/indigo_ccd_simulator_main.o $(BUILD_DRIVERS)/indigo_ccd_simulator.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_simulator.$(SOEXT): indigo_drivers/ccd_simulator/indigo_ccd_simulator.o
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

$(BUILD_DRIVERS)/indigo_ccd_atik: indigo_drivers/ccd_atik/indigo_ccd_atik_main.o $(BUILD_DRIVERS)/indigo_ccd_atik.a $(BUILD_LIB)/libatik.a $(LIBHIDAPI)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_atik.$(SOEXT): indigo_drivers/ccd_atik/indigo_ccd_atik.o $(BUILD_LIB)/libatik.a $(LIBHIDAPI)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

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
#	Build QHY CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_qhy.a: indigo_drivers/ccd_qhy/indigo_ccd_qhy.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_qhy: indigo_drivers/ccd_qhy/indigo_ccd_qhy_main.o $(BUILD_DRIVERS)/indigo_ccd_qhy.a $(BUILD_LIB)/libqhy.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_qhy.$(SOEXT): indigo_drivers/ccd_qhy/indigo_ccd_qhy.o $(BUILD_LIB)/libqhy.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

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
#	Build IIDC CCD driver
#
#---------------------------------------------------------------------

$(BUILD_DRIVERS)/indigo_ccd_iidc.a: indigo_drivers/ccd_iidc/indigo_ccd_iidc.o
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DRIVERS)/indigo_ccd_iidc: indigo_drivers/ccd_iidc/indigo_ccd_iidc_main.o $(BUILD_DRIVERS)/indigo_ccd_iidc.a $(BUILD_LIB)/libdc1394.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lindigo

$(BUILD_DRIVERS)/indigo_ccd_iidc.$(SOEXT): indigo_drivers/ccd_iidc/indigo_ccd_iidc.o $(BUILD_LIB)/libdc1394.a
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lindigo

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
#       Build SBIG CCD driver
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
$(BUILD_INCLUDE)/libsbig/sbigudrv.h:
	install -d $(BUILD_INCLUDE)/libsbig
	cp indigo_drivers/ccd_sbig/bin_externals/sbigudrv/include/sbigudrv.h $(BUILD_INCLUDE)/libsbig/

indigo_drivers/ccd_sbig/indigo_ccd_sbig.o: $(BUILD_INCLUDE)/libsbig/sbigudrv.h

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

$(BUILD_BIN)/indigo_server: indigo_server/indigo_server.o $(SIMULATOR_LIBS) indigo_server/ctrl.data
	$(CC) $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server/indigo_server.o $(SIMULATOR_LIBS) $(LDFLAGS) -lstdc++ -lindigo
ifeq ($(OS_DETECTED),Darwin)
	install_name_tool -add_rpath @loader_path/../drivers $@
	install_name_tool -change $(BUILD_LIB)/libindigo.dylib  @rpath/../lib/libindigo.dylib $@
	install_name_tool -change $(INDIGO_ROOT)/$(BUILD_LIB)/libusb-1.0.0.dylib  @rpath/../lib/libusb-1.0.0.dylib $@
endif

$(BUILD_BIN)/indigo_server_standalone: indigo_server/indigo_server.c $(DRIVER_LIBS)  $(BUILD_LIB)/libindigo.a $(EXTERNALS) indigo_server/ctrl.data
	$(CC) -DSTATIC_DRIVERS $(CFLAGS) $(AVAHI_CFLAGS) -o $@ indigo_server/indigo_server.c $(DRIVER_LIBS)  $(BUILD_LIB)/libindigo.a $(EXTERNALS) $(LDFLAGS) -lstdc++
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

indigo_server/ctrl.data:	indigo_server/ctrl.html
#	python tools/rjsmin.py <indigo_server/ctrl.html | gzip | hexdump -v -e '1/1 "0x%02x, "' >indigo_server/ctrl.data
	cat indigo_server/ctrl.html | gzip | hexdump -v -e '1/1 "0x%02x, "' >indigo_server/ctrl.data

#---------------------------------------------------------------------
#
#	Install
#
#---------------------------------------------------------------------

install:
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_server $(INSTALL_PREFIX)/bin
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_server_standalone $(INSTALL_PREFIX)/bin
	sudo install -D -m 0755 $(BUILD_BIN)/indigo_prop_tool $(INSTALL_PREFIX)/bin
	sudo install -D -m 0644 $(DRIVERS) $(INSTALL_PREFIX)/bin
	sudo install -D -m 0644 $(BUILD_LIB)/libindigo.so $(INSTALL_PREFIX)/lib
	sudo install -D -m 0644 $(DRIVER_SOLIBS) $(INSTALL_PREFIX)/lib
	sudo install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /lib/udev/rules.d/99-indigo_ccd_sx.rules
	sudo install -D -m 0644 indigo_drivers/ccd_atik/indigo_ccd_atik.rules /lib/udev/rules.d/99-indigo_ccd_atik.rules
	sudo install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /lib/udev/rules.d/99-indigo_ccd_ssag.rules
	sudo install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules /lib/udev/rules.d/99-indigo_ccd_asi.rules
	sudo install -D -m 0644 indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/99-efw.rules /lib/udev/rules.d/99-indigo_wheel_asi.rules
	sudo install -D -m 0644 indigo_drivers/focuser_usbv3/indigo_focuser_usbv3.rules /lib/udev/rules.d/99-indigo_focuser_usbv3.rules
	sudo udevadm control --reload-rules

#---------------------------------------------------------------------
#
#	Build packages
#
#---------------------------------------------------------------------

package: $(PACKAGE_NAME).$(PACKAGE_TYPE)

$(PACKAGE_NAME).deb: all
	install -d /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin
	install $(BUILD_BIN)/indigo_server /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin
	install $(BUILD_BIN)/indigo_server_standalone /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin
	install $(BUILD_BIN)/indigo_prop_tool /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin
	install $(DRIVERS) /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/bin
	install -d /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/lib
	install $(BUILD_LIB)/libindigo.so /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/lib
	install $(BUILD_LIB)/libindigo.a /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/lib
	install $(DRIVER_LIBS) /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/lib
	install $(DRIVER_SOLIBS) /tmp/$(PACKAGE_NAME)/$(INSTALL_PREFIX)/lib
	install -D -m 0644 indigo_drivers/ccd_sx/indigo_ccd_sx.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_sx.rules
	install -D -m 0644 indigo_drivers/ccd_atik/indigo_ccd_atik.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_atik.rules
	install -D -m 0644 indigo_drivers/ccd_ssag/indigo_ccd_ssag.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_ssag.rules
	install -D -m 0644 indigo_drivers/ccd_asi/indigo_ccd_asi.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_ccd_asi.rules
	install -D -m 0644 indigo_drivers/wheel_asi/bin_externals/libEFWFilter/lib/99-efw.rules /tmp/$(PACKAGE_NAME)/lib/udev/rules.d/99-indigo_wheel_asi.rules
	cp -r $(BUILD_SHARE) /tmp/$(PACKAGE_NAME)
	install -d /tmp/$(PACKAGE_NAME)/DEBIAN
	printf "Package: indigo\nVersion: $(INDIGO_VERSION)-$(INDIGO_BUILD)\nPriority: optional\nArchitecture: $(DEBIAN_ARCH)\nMaintainer: CloudMakers, s. r. o.\nDepends: libusb-1.0-0, libgudev-1.0-0, libavahi-compat-libdnssd1\nDescription: INDIGO Server\n" > /tmp/$(PACKAGE_NAME)/DEBIAN/control
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
	rm -f $(BUILD_ROOT)/bin/indigo_server*
	rm -f $(BUILD_ROOT)/lib/libindigo*
	rm -rf $(BUILD_ROOT)/drivers
	rm -f indigo_libs/*.o
	rm -f indigo_server/*.o
	rm -f $(wildcard indigo_drivers/*/*.o)
	rm -f $(wildcard indigo_test/*.o)

#---------------------------------------------------------------------
#
#	Clean indigo & externals build
#
#---------------------------------------------------------------------

clean-all: clean
	rm -rf $(BUILD_ROOT)
	cd externals/hidapi; make maintainer-clean; rm configure; cd ../..
	cd externals/libusb; make maintainer-clean; rm configure; cd ../..
	cd externals/libjpeg; make distclean; cd ../..
	cd indigo_drivers/ccd_iidc/externals/libdc1394; make maintainer-clean; rm configure; cd ../../../..
	cd indigo_drivers/mount_nexstar/externals/libnexstar; make maintainer-clean; rm configure; cd ../../../..
	cd indigo_drivers/ccd_fli/externals/libfli-1.104; make clean; cd ../../../..
