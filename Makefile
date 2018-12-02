#---------------------------------------------------------------------
#
# Copyright (c) 2018 CloudMakers, s. r. o.
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#---------------------------------------------------------------------

INDIGO_VERSION = 2.0
INDIGO_BUILD = 76

INDIGO_ROOT = $(shell pwd)
BUILD_ROOT = $(INDIGO_ROOT)/build
BUILD_BIN = $(BUILD_ROOT)/bin
BUILD_DRIVERS = $(BUILD_ROOT)/drivers
BUILD_LIB = $(BUILD_ROOT)/lib
BUILD_INCLUDE = $(BUILD_ROOT)/include
BUILD_SHARE = $(BUILD_ROOT)/share

INSTALL_ROOT = /
INSTALL_BIN = $(INSTALL_ROOT)/usr/local/bin
INSTALL_LIB = $(INSTALL_ROOT)/usr/local/lib
INSTALL_ETC = $(INSTALL_ROOT)/etc
INSTALL_SHARE = $(INSTALL_ROOT)/usr/local/share
INSTALL_RULES = $(INSTALL_ROOT)/lib/udev/rules.d
INSTALL_FIRMWARE = $(INSTALL_ROOT)/lib/firmware

STABLE_DRIVERS = agent_lx200_server agent_snoop aux_joystick aux_upb ccd_altair ccd_apogee ccd_asi ccd_atik ccd_dsi ccd_fli ccd_ica ccd_iidc ccd_mi ccd_qhy ccd_qsi ccd_sbig ccd_simulator ccd_ssag ccd_sx ccd_touptek dome_simulator focuser_dmfc focuser_fcusb focuser_fli focuser_mjkzz focuser_mjkzz_bt focuser_moonlite focuser_nfocus focuser_nstep focuser_usbv3 focuser_wemacro focuser_wemacro_bt gps_nmea gps_simulator guider_asi guider_cgusbst4 guider_eqmac mount_ioptron mount_lx200 mount_nexstar mount_simulator mount_temma wheel_asi wheel_atik wheel_fli wheel_sx
UNTESTED_DRIVERS = aux_dsusb aux_rts focuser_lakeside focuser_optec guider_gpusb wheel_optec wheel_quantum wheel_trutek wheel_xagyl
DEVELOPED_DRIVERS = agent_imager mount_synscan system_ascol
OPTIONAL_DRIVERS = ccd_andor

#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

DEBUG_BUILD = -g

ifeq ($(OS),Windows_NT)
	OS_DETECTED = Windows
else
	OS_DETECTED = $(shell uname -s)
	ARCH_DETECTED = $(shell uname -m)
	ifeq ($(OS_DETECTED),Darwin)
		ARCH_DETECTED = x64
		CC = clang
		AR = ar
		CFLAGS = $(DEBUG_BUILD) -mmacosx-version-min=10.10 -fPIC -O3 -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_mac_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS -Duint=unsigned
		CXXFLAGS = $(DEBUG_BUILD) -mmacosx-version-min=10.10 -fPIC -O3 -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_mac_drivers -I$(BUILD_INCLUDE) -DINDIGO_MACOS
		MFLAGS = $(DEBUG_BUILD) -mmacosx-version-min=10.10 -fPIC -fno-common -O3 -fobjc-arc -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_mac_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS -Wobjc-property-no-attribute
		LDFLAGS = -headerpad_max_install_names -framework Cocoa -mmacosx-version-min=10.10 -framework CoreFoundation -framework IOKit -framework ImageCaptureCore -framework IOBluetooth -lobjc  -L$(BUILD_LIB) -lusb-1.0
		ARFLAGS = -rv
		SOEXT = dylib
		INSTALL_ROOT = $(INDIGO_ROOT)/install
	endif
	ifeq ($(OS_DETECTED),Linux)
		ifeq ($(ARCH_DETECTED),armv7l)
			ARCH_DETECTED = arm
			DEBIAN_ARCH = armhf
		endif
		ifeq ($(ARCH_DETECTED),aarch64)
			ARCH_DETECTED = arm64
			DEBIAN_ARCH = arm64
			EXCLUDED_DRIVERS = ccd_sbig
		endif
		ifeq ($(ARCH_DETECTED),i686)
			ARCH_DETECTED = x86
			DEBIAN_ARCH = i386
		endif
		ifeq ($(ARCH_DETECTED),x86_64)
			ARCH_DETECTED = x64
			DEBIAN_ARCH = amd64
		endif
		CC = gcc
		AR = ar
		ifeq ($(ARCH_DETECTED),arm)
			CFLAGS = $(DEBUG_BUILD) -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX
			CXXFLAGS = $(DEBUG_BUILD) -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu++11 -pthread -DINDIGO_LINUX
		else
			CFLAGS = $(DEBUG_BUILD) -fPIC -O3 -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX
			CXXFLAGS = $(DEBUG_BUILD) -fPIC -O3 -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu++11 -pthread -DINDIGO_LINUX
		endif
		LDFLAGS = -lm -lrt -lusb-1.0 -pthread -L$(BUILD_LIB) -Wl,-rpath=\\\$$\$$ORIGIN/../lib,-rpath=\\\$$\$$ORIGIN/../drivers,-rpath=.
		ARFLAGS = -rv
		SOEXT = so
		LIBHIDAPI = $(BUILD_LIB)/libhidapi-hidraw.a
	endif
endif

.PHONY: init all clean clean-all

all:	init
	@$(MAKE)	-C indigo_libs all
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs all
ifeq ($(OS_DETECTED),Darwin)
	@$(MAKE)	-C indigo_mac_drivers  -f ../Makefile.drvs all
endif
ifeq ($(OS_DETECTED),Linux)
	@$(MAKE)	-C indigo_linux_drivers  -f ../Makefile.drvs all
endif
	@$(MAKE)	-C indigo_server all
	@$(MAKE)	-C indigo_tools all

status:
	@$(MAKE)	-C indigo_libs status
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs status
ifeq ($(OS_DETECTED),Darwin)
	@$(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs status
endif
ifeq ($(OS_DETECTED),Linux)
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs status
endif
	@$(MAKE)	-C indigo_server status
	@$(MAKE)	-C indigo_tools status

reconfigure:
	rm -f Makefile.inc
	install -d -m 0755 $(INSTALL_ROOT)
	install -d -m 0755 $(INSTALL_BIN)
	install -d -m 0755 $(INSTALL_LIB)
	install -d -m 0755 $(INSTALL_ETC)
	install -d -m 0755 $(INSTALL_SHARE)
	install -d -m 0755 $(INSTALL_SHARE)/indigo
	install -d -m 0755 $(INSTALL_SHARE)/indi
	install -d -m 0755 $(INSTALL_RULES)
	install -d -m 0755 $(INSTALL_FIRMWARE)

install: reconfigure init all
	@$(MAKE)	-C indigo_libs install
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs install
ifeq ($(OS_DETECTED),Darwin)
	@$(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs install
endif
ifeq ($(OS_DETECTED),Linux)
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs install
endif
	@$(MAKE)	-C indigo_server install
	@$(MAKE)	-C indigo_tools install
ifeq ($(OS_DETECTED),Linux)
	sudo udevadm control --reload-rules
endif

uninstall: reconfigure init
	@$(MAKE)	-C indigo_libs uninstall
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs uninstall
ifeq ($(OS_DETECTED),Darwin)
	@$(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs uninstall
endif
ifeq ($(OS_DETECTED),Linux)
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs uninstall
endif
	@$(MAKE)	-C indigo_server uninstall
	@$(MAKE)	-C indigo_tools uninstall
ifeq ($(OS_DETECTED),Linux)
	sudo udevadm control --reload-rules
endif

ifeq ($(OS_DETECTED),Linux)
package: INSTALL_ROOT = $(INDIGO_ROOT)/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-$(DEBIAN_ARCH)
package: INSTALL_BIN = $(INSTALL_ROOT)/usr/bin
package: INSTALL_LIB = $(INSTALL_ROOT)/usr/lib
package: INSTALL_ETC = $(INSTALL_ROOT)/etc
package: INSTALL_SHARE = $(INSTALL_ROOT)/usr/share
package: INSTALL_RULES = $(INSTALL_ROOT)/lib/udev/rules.d
package: INSTALL_FIRMWARE = $(INSTALL_ROOT)/lib/firmware
package: reconfigure init all
	@$(MAKE)	-C indigo_libs install
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs install
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs install
	@$(MAKE)	-C indigo_server install
	install -d $(INSTALL_ROOT)/DEBIAN
	printf "Package: indigo\n" > $(INSTALL_ROOT)/DEBIAN/control
	printf "Version: $(INDIGO_VERSION)-$(INDIGO_BUILD)\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Installed-Size: $(shell echo `du -s $(INSTALL_ROOT) | cut -f1`)\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Priority: optional\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Architecture: $(DEBIAN_ARCH)\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Replaces: libsbigudrv2,libsbig,libqhy,indi-dsi,indigo-upb\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Maintainer: CloudMakers, s. r. o. <indigo@cloudmakers.eu>\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Homepage: http://www.indigo-astronomy.org\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Depends: fxload, libusb-1.0-0, libgudev-1.0-0, libgphoto2-6, libavahi-compat-libdnssd1\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Description: INDIGO Framework and drivers\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf " INDIGO is a system of standards and frameworks for multiplatform and distributed astronomy software development designed to scale with your needs.\n" >> $(INSTALL_ROOT)/DEBIAN/control
	cat $(INSTALL_ROOT)/DEBIAN/control
	printf "echo Remove pre-2.0-76 build if any\n" > $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/bin/indigo_*\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/indigo_*\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/libindigo*\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/pkgconfig/indigo.pc\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/libtoupcam.$(SOEXT)\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -f /usr/local/lib/libaltaircam.$(SOEXT)\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "sudo rm -rf /usr/local/etc/apogee\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	chmod a+x $(INSTALL_ROOT)/DEBIAN/preinst
	rm -f $(INSTALL_ROOT).deb
	fakeroot dpkg --build $(INSTALL_ROOT)
#	rm -rf $(INSTALL_ROOT)
endif

clean: init
	@$(MAKE)	-C indigo_libs clean
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs clean
ifeq ($(OS_DETECTED),Darwin)
	@$(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs clean
endif
ifeq ($(OS_DETECTED),Linux)
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs clean
endif
	@$(MAKE)	-C indigo_server clean
	@$(MAKE)	-C indigo_tools clean

clean-all:
	@$(MAKE)	-C indigo_libs clean-all
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs clean-all
ifeq ($(OS_DETECTED),Darwin)
	@$(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs clean-all
endif
ifeq ($(OS_DETECTED),Linux)
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs clean-all
	rm -f $(INDIGO_ROOT)/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-$(DEBIAN_ARCH).deb
	rm -rf $(INDIGO_ROOT)/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-$(DEBIAN_ARCH)
endif
	@$(MAKE)	-C indigo_server clean-all
	@$(MAKE)	-C indigo_tools clean-all
	rm -rf $(BUILD_ROOT)

init: Makefile.inc
	install -d -m 0755 $(BUILD_ROOT)
	install -d -m 0755 $(BUILD_BIN)
	install -d -m 0755 $(BUILD_DRIVERS)
	install -d -m 0755 $(BUILD_LIB)
	install -d -m 0755 $(BUILD_INCLUDE)
	install -d -m 0755 $(BUILD_SHARE)

Makefile.inc: $(MAKEFILE_LIST)
	rm -f Makefile.inc
	@printf "# File is created automatically by top level Makefile, don't edit\n\n" > Makefile.inc
	@printf "INDIGO_VERSION = $(INDIGO_VERSION)\n" >> Makefile.inc
	@printf "INDIGO_BUILD = $(INDIGO_BUILD)\n" >> Makefile.inc
	@printf "INDIGO_ROOT = $(INDIGO_ROOT)\n" >> Makefile.inc
	@printf "BUILD_ROOT = $(BUILD_ROOT)\n" >> Makefile.inc
	@printf "BUILD_BIN = $(BUILD_BIN)\n" >> Makefile.inc
	@printf "BUILD_DRIVERS = $(BUILD_DRIVERS)\n" >> Makefile.inc
	@printf "BUILD_LIB = $(BUILD_LIB)\n" >> Makefile.inc
	@printf "BUILD_INCLUDE = $(BUILD_INCLUDE)\n" >> Makefile.inc
	@printf "BUILD_SHARE = $(BUILD_SHARE)\n\n" >> Makefile.inc
	@printf "OS_DETECTED = $(OS_DETECTED)\n" >> Makefile.inc
	@printf "ARCH_DETECTED = $(ARCH_DETECTED)\n" >> Makefile.inc
	@printf "DEBIAN_ARCH = $(DEBIAN_ARCH)\n\n" >> Makefile.inc
	@printf "CC = $(CC)\n" >> Makefile.inc
	@printf "AR = $(AR)\n" >> Makefile.inc
	@printf "CFLAGS = $(CFLAGS)\n" >> Makefile.inc
	@printf "CXXFLAGS = $(CXXFLAGS)\n" >> Makefile.inc
	@printf "MFLAGS = $(MFLAGS)\n" >> Makefile.inc
	@printf "LDFLAGS = $(LDFLAGS)\n" >> Makefile.inc
	@printf "ARFLAGS = $(ARFLAGS)\n" >> Makefile.inc
	@printf "SOEXT = $(SOEXT)\n" >> Makefile.inc
	@printf "LIBHIDAPI = $(LIBHIDAPI)\n\n" >> Makefile.inc
	@printf "INSTALL_ROOT = $(INSTALL_ROOT)\n" >> Makefile.inc
	@printf "INSTALL_BIN = $(INSTALL_BIN)\n" >> Makefile.inc
	@printf "INSTALL_LIB = $(INSTALL_LIB)\n" >> Makefile.inc
	@printf "INSTALL_ETC = $(INSTALL_ETC)\n" >> Makefile.inc
	@printf "INSTALL_SHARE = $(INSTALL_SHARE)\n" >> Makefile.inc
	@printf "INSTALL_RULES = $(INSTALL_RULES)\n" >> Makefile.inc
	@printf "INSTALL_FIRMWARE = $(INSTALL_FIRMWARE)\n\n" >> Makefile.inc
	@printf "STABLE_DRIVERS = $(STABLE_DRIVERS)\n" >> Makefile.inc
	@printf "UNTESTED_DRIVERS = $(UNTESTED_DRIVERS)\n" >> Makefile.inc
	@printf "DEVELOPED_DRIVERS = $(DEVELOPED_DRIVERS)\n" >> Makefile.inc
	@printf "OPTIONAL_DRIVERS = $(OPTIONAL_DRIVERS)\n" >> Makefile.inc
	@printf "EXCLUDED_DRIVERS = $(EXCLUDED_DRIVERS)\n" >> Makefile.inc
	@echo --------------------------------------------------------------------- Makefile.inc
	@cat Makefile.inc
	@echo ---------------------------------------------------------------------
