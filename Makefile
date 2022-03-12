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
INDIGO_BUILD = 171

# Keep the suffix empty for official releases
INDIGO_BUILD_SUFFIX =

ifneq ($(INDIGO_BUILD_SUFFIX),)
  INDIGO_BUILD := $(INDIGO_BUILD)-$(INDIGO_BUILD_SUFFIX)
endif

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
INSTALL_INCLUDE = $(INSTALL_ROOT)/usr/local/include
INSTALL_ETC = $(INSTALL_ROOT)/etc
INSTALL_SHARE = $(INSTALL_ROOT)/usr/local/share
INSTALL_RULES = $(INSTALL_ROOT)/lib/udev/rules.d
INSTALL_FIRMWARE = $(INSTALL_ROOT)/lib/firmware

STABLE_DRIVERS = agent_alignment agent_auxiliary agent_guider agent_imager agent_lx200_server agent_mount agent_snoop ao_sx aux_cloudwatcher aux_dragonfly aux_dsusb aux_fbc aux_flatmaster aux_flipflat aux_joystick aux_mgbox aux_ppb aux_sqm aux_upb aux_usbdp ccd_altair ccd_apogee ccd_asi ccd_atik ccd_dsi ccd_fli ccd_iidc ccd_mi ccd_ptp ccd_qsi ccd_sbig ccd_simulator ccd_ssag ccd_sx ccd_touptek ccd_uvc dome_dragonfly dome_nexdome3 dome_simulator focuser_asi focuser_dmfc focuser_dsd focuser_efa focuser_fcusb focuser_fli focuser_focusdreampro focuser_lunatico focuser_moonlite focuser_steeldrive2 focuser_usbv3 focuser_wemacro gps_gpsd gps_nmea gps_simulator guider_asi guider_cgusbst4 guider_gpusb mount_ioptron mount_lx200 mount_nexstar mount_nexstaraux mount_pmc8 mount_simulator mount_synscan mount_temma rotator_lunatico rotator_simulator system_ascol wheel_asi wheel_atik wheel_fli wheel_manual wheel_qhy wheel_sx aux_rpio ccd_ica focuser_wemacro_bt guider_eqmac focuser_mypro2 agent_astrometry mount_rainbow agent_scripting focuser_mjkzz focuser_mjkzz_bt dome_talon6ror aux_geoptikflat ccd_svb agent_astap
UNSTABLE_DRIVERS = ccd_qhy ccd_qhy2
UNTESTED_DRIVERS = aux_arteskyflat aux_rts dome_baader dome_nexdome focuser_lakeside focuser_nfocus focuser_nstep focuser_optec focuser_robofocus wheel_optec wheel_quantum wheel_trutek wheel_xagyl dome_skyroof aux_skyalert agent_alpaca dome_beaver focuser_astromechanics aux_astromechanics
DEVELOPED_DRIVERS =
OPTIONAL_DRIVERS = ccd_andor
EXCLUDED_DRIVERS = ccd_gphoto2

#---------------------------------------------------------------------
#
#	Platform detection
#
#---------------------------------------------------------------------

DEBUG_BUILD = -g

ifeq ($(OS),Windows_NT)
	OS_DETECTED = Windows
	INDIGO_CUDA =
else
	OS_DETECTED = $(shell uname -s)
	ARCH_DETECTED = $(shell uname -m)
	ifeq ($(OS_DETECTED),Darwin)
		CC = /usr/bin/clang
		AR = /usr/bin/libtool
		INDIGO_CUDA =
		ifeq ($(findstring arm64e,$(shell file $(CC) | head -1)),arm64e)
			MAC_ARCH = -arch x86_64 -arch arm64
		else
			MAC_ARCH = -arch x86_64
		endif
		CFLAGS = $(DEBUG_BUILD) $(MAC_ARCH) -mmacosx-version-min=10.10 -fPIC -O3 -isystem$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_mac_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS -Duint=unsigned
		CXXFLAGS = $(DEBUG_BUILD) $(MAC_ARCH) -mmacosx-version-min=10.10 -fPIC -O3 -isystem$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_mac_drivers -I$(BUILD_INCLUDE) -DINDIGO_MACOS
		MFLAGS = $(DEBUG_BUILD) $(MAC_ARCH) -mmacosx-version-min=10.10 -fPIC -fno-common -O3 -fobjc-arc -isystem$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_mac_drivers -I$(BUILD_INCLUDE) -std=gnu11 -DINDIGO_MACOS -Wobjc-property-no-attribute
		LDFLAGS = $(DEBUG_BUILD) $(MAC_ARCH) -headerpad_max_install_names -framework Cocoa -mmacosx-version-min=10.10 -framework CoreFoundation -framework IOKit -framework ImageCaptureCore -framework IOBluetooth -lobjc  -L$(BUILD_LIB)
		ARFLAGS = -static -o
		SOEXT = dylib
		INSTALL_ROOT = $(INDIGO_ROOT)/install
	endif
	ifeq ($(OS_DETECTED),Linux)
		ifeq ($(ARCH_DETECTED),armv6l)
			ARCH_DETECTED = arm
			DEBIAN_ARCH = armhf
			DRPI_MANAGEMENT = -DRPI_MANAGEMENT
		endif
		ifeq ($(ARCH_DETECTED),armv7l)
			ARCH_DETECTED = arm
			DEBIAN_ARCH = armhf
			DRPI_MANAGEMENT = -DRPI_MANAGEMENT
		endif
		ifeq ($(ARCH_DETECTED),aarch64)
			ARCH_DETECTED = arm64
			DEBIAN_ARCH = arm64
			DRPI_MANAGEMENT = -DRPI_MANAGEMENT
			#EXCLUDED_DRIVERS += ccd_sbig
		endif
		ifeq ($(ARCH_DETECTED),i686)
			ARCH_DETECTED = x86
			DEBIAN_ARCH = i386
		endif
		ifeq ($(ARCH_DETECTED),x86_64)
			ifneq ($(wildcard /lib/x86_64-linux-gnu/),)
				ARCH_DETECTED = x64
				DEBIAN_ARCH = amd64
			else ifneq ($(wildcard /lib64/),)
				ARCH_DETECTED = x64
				DEBIAN_ARCH = amd64
			else
				ARCH_DETECTED = x86
				DEBIAN_ARCH = i386
			endif
		endif
		CC = gcc
		AR = ar
		INDIGO_CUDA = $(wildcard /usr/local/cuda)
		ifeq ($(INDIGO_CUDA),)
			CUDA_BUILD = ""
			LDFLAGS = -lm -lrt -lusb-1.0 -pthread -L$(BUILD_LIB) -Wl,-rpath=\\\$$\$$ORIGIN/../lib,-rpath=\\\$$\$$ORIGIN/../drivers,-rpath=.
		else
			NVCC = $(INDIGO_CUDA)/bin/nvcc
			CUDA_LIBS = $(addprefix -L,$(wildcard $(INDIGO_CUDA)/lib*))
			CUDA_BUILD = "-DINDIGO_CUDA"
			NVCCFLAGS = $(DEBUG_BUILD) -Xcompiler -fPIC -O3 -isystem $(INDIGO_CUDA)/include -isystem $(INDIGO_ROOT)/indigo_libs -I $(INDIGO_ROOT)/indigo_drivers -I $(INDIGO_ROOT)/indigo_linux_drivers -I $(BUILD_INCLUDE) -std=c++11 -DINDIGO_LINUX
			LDFLAGS = -lm -lrt -lusb-1.0 -pthread -lcudart $(CUDA_LIBS) -L$(BUILD_LIB) -Wl,-rpath=\\\$$\$$ORIGIN/../lib,-rpath=\\\$$\$$ORIGIN/../drivers,-rpath=.
		endif
		ifeq ($(ARCH_DETECTED),arm)
			CFLAGS = $(DEBUG_BUILD) $(CUDA_BUILD) -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm -mthumb-interwork -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX $(DRPI_MANAGEMENT) -D_FILE_OFFSET_BITS=64
			CXXFLAGS = $(DEBUG_BUILD) $(CUDA_BUILD) -fPIC -O3 -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm -mthumb-interwork -I$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu++11 -pthread -DINDIGO_LINUX
		else
			CFLAGS = $(DEBUG_BUILD) $(CUDA_BUILD) -fPIC -O3 -isystem$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu11 -pthread -DINDIGO_LINUX $(DRPI_MANAGEMENT)
			CXXFLAGS = $(DEBUG_BUILD) $(CUDA_BUILD) -fPIC -O3 -isystem$(INDIGO_ROOT)/indigo_libs -I$(INDIGO_ROOT)/indigo_drivers -I$(INDIGO_ROOT)/indigo_linux_drivers -I$(BUILD_INCLUDE) -std=gnu++11 -pthread -DINDIGO_LINUX
		endif
		ARFLAGS = -rv
		SOEXT = so
		LIBHIDAPI = $(BUILD_LIB)/libhidapi-libusb.a
	endif
endif

.PHONY: init all clean clean-all

all:	init $(BUILD_LIB)/libindigo.$(SOEXT)
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

$(BUILD_LIB)/libindigo.$(SOEXT): $(filter-out $(INDIGO_ROOT)/indigo_libs/indigo/indigo_config.h, $(wildcard $(INDIGO_ROOT)/indigo_libs/indigo/*.h))
	@echo --------------------------------------------------------------------- Forced clean - framework headers are changed
	@$(MAKE) clean

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
	install -d -m 0755 $(INSTALL_INCLUDE)
	install -d -m 0755 $(INSTALL_ETC)
	install -d -m 0755 $(INSTALL_SHARE)
	install -d -m 0755 $(INSTALL_SHARE)/indigo
	install -d -m 0755 $(INSTALL_SHARE)/indi
	install -d -m 0755 $(INSTALL_RULES)
	install -d -m 0755 $(INSTALL_FIRMWARE)

install: reconfigure init all
	@sudo $(MAKE)	-C indigo_libs install
	@sudo $(MAKE)	-C indigo_drivers -f ../Makefile.drvs install
ifeq ($(OS_DETECTED),Darwin)
	@sudo $(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs install
endif
ifeq ($(OS_DETECTED),Linux)
	@sudo $(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs install
endif
	@sudo $(MAKE)	-C indigo_server install
	@sudo $(MAKE)	-C indigo_tools install
ifeq ($(OS_DETECTED),Linux)
	sudo udevadm control --reload-rules
	@$(MAKE)	    -C tools/fxload -f Makefile
	@sudo install -d /sbin
	@sudo install -d /usr/sbin
	@sudo install -m 0755 tools/fxload/fxload /sbin
	@sudo install -m 0755 tools/fxload/fxload /usr/sbin
endif

indigo-environment-install:
	install -m 0755 systemd/indigo-environment /usr/bin
	install -m 0644 systemd/indigo-environment.service /lib/systemd/system
	systemctl enable indigo-environment
	systemctl start indigo-environment

uninstall: reconfigure init
	@sudo $(MAKE)	-C indigo_libs uninstall
	@sudo $(MAKE)	-C indigo_drivers -f ../Makefile.drvs uninstall
ifeq ($(OS_DETECTED),Darwin)
	@sudo $(MAKE)	-C indigo_mac_drivers -f ../Makefile.drvs uninstall
endif
ifeq ($(OS_DETECTED),Linux)
	@sudo $(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs uninstall
endif
	@sudo $(MAKE)	-C indigo_server uninstall
	@sudo $(MAKE)	-C indigo_tools uninstall
ifeq ($(OS_DETECTED),Linux)
	sudo udevadm control --reload-rules
endif

ifeq ($(OS_DETECTED),Linux)
package: INSTALL_ROOT = $(INDIGO_ROOT)/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-$(DEBIAN_ARCH)
package: INSTALL_BIN = $(INSTALL_ROOT)/usr/bin
package: INSTALL_LIB = $(INSTALL_ROOT)/usr/lib
package: INSTALL_INCLUDE = $(INSTALL_ROOT)/usr/include
package: INSTALL_ETC = $(INSTALL_ROOT)/etc
package: INSTALL_SHARE = $(INSTALL_ROOT)/usr/share
package: INSTALL_RULES = $(INSTALL_ROOT)/lib/udev/rules.d
package: INSTALL_FIRMWARE = $(INSTALL_ROOT)/lib/firmware
package: reconfigure init all
	@$(MAKE)	-C indigo_libs install
	@$(MAKE)	-C indigo_drivers -f ../Makefile.drvs install
	@$(MAKE)	-C indigo_linux_drivers -f ../Makefile.drvs install
	@$(MAKE)	-C indigo_server install
	@$(MAKE)	-C indigo_tools install
	@$(MAKE)	-C tools/fxload -f Makefile
ifeq ($(ARCH_DETECTED),$(filter $(ARCH_DETECTED),arm arm64))
	install -d $(INSTALL_ROOT)/usr/bin
	install -m 0755 tools/rpi_ctrl.sh $(INSTALL_ROOT)/usr/bin
	install -m 0755 tools/wifi_channel_selector.pl $(INSTALL_ROOT)/usr/bin
endif
	install -d $(INSTALL_ROOT)/usr/sbin
	install -m 0755 tools/fxload/fxload $(INSTALL_ROOT)/usr/sbin
	install -m 0755 systemd/indigo-environment $(INSTALL_ROOT)/usr/bin
	install -d $(INSTALL_ROOT)/lib/systemd/system
	install -m 0644 systemd/indigo-environment.service $(INSTALL_ROOT)/lib/systemd/system
	install -d $(INSTALL_ROOT)/DEBIAN
	printf "Package: indigo\n" > $(INSTALL_ROOT)/DEBIAN/control
	printf "Version: $(INDIGO_VERSION)-$(INDIGO_BUILD)\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Installed-Size: $(shell echo `du -s $$(INSTALL_ROOT) | cut -f1`)\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Priority: optional\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Architecture: $(DEBIAN_ARCH)\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Replaces: fxload,libsbigudrv2,libsbig,libqhy,indi-dsi,indigo-upb\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Maintainer: CloudMakers, s. r. o. <indigo@cloudmakers.eu>\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Homepage: http://www.indigo-astronomy.org\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Depends: indigo-astrometry | astrometry.net, libusb-1.0-0, libgudev-1.0-0, libgphoto2-6, libavahi-compat-libdnssd1\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf "Description: INDIGO Framework and drivers\n" >> $(INSTALL_ROOT)/DEBIAN/control
	printf " INDIGO is a system of standards and frameworks for multiplatform and distributed astronomy software development designed to scale with your needs.\n" >> $(INSTALL_ROOT)/DEBIAN/control
	cat $(INSTALL_ROOT)/DEBIAN/control
	printf "echo Remove pre-2.0-76 build if any\n" > $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -f /usr/local/bin/indigo_*\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -f /usr/local/lib/indigo_*\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -f /usr/local/lib/libindigo*\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -f /usr/local/lib/pkgconfig/indigo.pc\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -f /usr/local/lib/libtoupcam.$(SOEXT)\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -f /usr/local/lib/libaltaircam.$(SOEXT)\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	printf "rm -rf /usr/local/etc/apogee\n" >> $(INSTALL_ROOT)/DEBIAN/preinst
	chmod a+x $(INSTALL_ROOT)/DEBIAN/preinst
	echo "#!/bin/bash" >$(INSTALL_ROOT)/DEBIAN/postinst
	echo >>$(INSTALL_ROOT)/DEBIAN/postinst
	echo "if [ -z \`which systemctl\` ]; then echo \"No systemctl, will not configure!\"; exit 0; fi" >>$(INSTALL_ROOT)/DEBIAN/postinst
	echo "# Configure INDIGO environment setvice" >>$(INSTALL_ROOT)/DEBIAN/postinst
	echo "systemctl enable indigo-environment" >>$(INSTALL_ROOT)/DEBIAN/postinst
	echo "systemctl start indigo-environment" >>$(INSTALL_ROOT)/DEBIAN/postinst
ifeq ($(ARCH_DETECTED),$(filter $(ARCH_DETECTED),arm arm64))
	tail -n +2 tools/rpi_ctrl_fix.sh >> $(INSTALL_ROOT)/DEBIAN/postinst
else
	echo "exit 0" >>$(INSTALL_ROOT)/DEBIAN/postinst
endif
	chmod a+x $(INSTALL_ROOT)/DEBIAN/postinst
	echo "#!/bin/bash" >$(INSTALL_ROOT)/DEBIAN/prerm
	echo >>$(INSTALL_ROOT)/DEBIAN/prerm
	echo "if [ -z \`which systemctl\` ]; then echo \"No systemctl, will not disable indigo-environment service!\"; exit 0; fi" >>$(INSTALL_ROOT)/DEBIAN/prerm
	echo "# Disable INDIGO environment setvice" >>$(INSTALL_ROOT)/DEBIAN/prerm
	echo "systemctl stop indigo-environment" >>$(INSTALL_ROOT)/DEBIAN/prerm
	echo "systemctl disable indigo-environment" >>$(INSTALL_ROOT)/DEBIAN/prerm
	chmod a+x $(INSTALL_ROOT)/DEBIAN/prerm

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

clean-all: Makefile.inc
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
	rm -rf Makefile.inc

init: Makefile.inc
	install -d -m 0755 $(BUILD_ROOT)
	install -d -m 0755 $(BUILD_BIN)
	install -d -m 0755 $(BUILD_DRIVERS)
	install -d -m 0755 $(BUILD_LIB)
	install -d -m 0755 $(BUILD_INCLUDE)
	install -d -m 0755 $(BUILD_SHARE)/indigo

Makefile.inc: Makefile
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
	@printf "INDIGO_CUDA = $(INDIGO_CUDA)\n" >> Makefile.inc
	@printf "CC = $(CC)\n" >> Makefile.inc
	@printf "AR = $(AR)\n" >> Makefile.inc
	@printf "NVCC = $(NVCC)\n" >> Makefile.inc
	@printf "CFLAGS = $(CFLAGS)\n" >> Makefile.inc
	@printf "CXXFLAGS = $(CXXFLAGS)\n" >> Makefile.inc
	@printf "NVCCFLAGS = $(NVCCFLAGS)\n" >> Makefile.inc
	@printf "MFLAGS = $(MFLAGS)\n" >> Makefile.inc
	@printf "LDFLAGS = $(LDFLAGS)\n" >> Makefile.inc
	@printf "ARFLAGS = $(ARFLAGS)\n" >> Makefile.inc
	@printf "SOEXT = $(SOEXT)\n" >> Makefile.inc
	@printf "LIBHIDAPI = $(LIBHIDAPI)\n\n" >> Makefile.inc
	@printf "INSTALL_ROOT = $(INSTALL_ROOT)\n" >> Makefile.inc
	@printf "INSTALL_BIN = $(INSTALL_BIN)\n" >> Makefile.inc
	@printf "INSTALL_LIB = $(INSTALL_LIB)\n" >> Makefile.inc
	@printf "INSTALL_INCLUDE = $(INSTALL_INCLUDE)\n" >> Makefile.inc
	@printf "INSTALL_ETC = $(INSTALL_ETC)\n" >> Makefile.inc
	@printf "INSTALL_SHARE = $(INSTALL_SHARE)\n" >> Makefile.inc
	@printf "INSTALL_RULES = $(INSTALL_RULES)\n" >> Makefile.inc
	@printf "INSTALL_FIRMWARE = $(INSTALL_FIRMWARE)\n\n" >> Makefile.inc
	@printf "STABLE_DRIVERS = $(STABLE_DRIVERS)\n" >> Makefile.inc
	@printf "UNSTABLE_DRIVERS = $(UNSTABLE_DRIVERS)\n" >> Makefile.inc
	@printf "UNTESTED_DRIVERS = $(UNTESTED_DRIVERS)\n" >> Makefile.inc
	@printf "DEVELOPED_DRIVERS = $(DEVELOPED_DRIVERS)\n" >> Makefile.inc
	@printf "OPTIONAL_DRIVERS = $(OPTIONAL_DRIVERS)\n" >> Makefile.inc
	@printf "EXCLUDED_DRIVERS = $(EXCLUDED_DRIVERS)\n" >> Makefile.inc
	@echo --------------------------------------------------------------------- Makefile.inc
	@cat Makefile.inc
	@echo ---------------------------------------------------------------------

debs-remote:
	ssh ubuntu32.local "cd indigo; git reset --hard; git pull; make clean-all; make; sudo make package"
	scp ubuntu32.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-i386.deb .
	ssh ubuntu64.local "cd indigo; git reset --hard; git pull; make clean-all; make; sudo make package"
	scp ubuntu64.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-amd64.deb .
	ssh raspi32.local "cd indigo; git reset --hard; git pull; make clean-all; make; sudo make package"
	scp raspi32.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-armhf.deb .
	ssh raspi64.local "cd indigo; git reset --hard; git pull; make clean-all; make; sudo make package"
	scp raspi64.local:indigo/indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-arm64.deb .

debs-docker:
	sh tools/make_source_tarball.sh $(INDIGO_VERSION)-$(INDIGO_BUILD)
	sh tools/build_debs.sh "i386/debian:stretch-slim" "indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-i386.deb" $(INDIGO_VERSION)-$(INDIGO_BUILD)
	sh tools/build_debs.sh "amd64/debian:stretch-slim" "indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-amd64.deb" $(INDIGO_VERSION)-$(INDIGO_BUILD)
	sh tools/build_debs.sh "arm32v7/debian:buster-slim" "indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-armhf.deb" $(INDIGO_VERSION)-$(INDIGO_BUILD)
	sh tools/build_debs.sh "arm64v8/debian:buster-slim" "indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-arm64.deb" $(INDIGO_VERSION)-$(INDIGO_BUILD)
	rm indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD).tar.gz

init-repo:
	aptly repo create -distribution=indigo -component=main indigo-release

publish:
	rm -f ~/Desktop/public
	aptly repo remove indigo-release indigo_$(INDIGO_VERSION)-$(INDIGO_BUILD)_i386 indigo_$(INDIGO_VERSION)-$(INDIGO_BUILD)_amd64 indigo_$(INDIGO_VERSION)-$(INDIGO_BUILD)_armhf indigo_$(INDIGO_VERSION)-$(INDIGO_BUILD)_arm64
	aptly repo add indigo-release indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-i386.deb indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-amd64.deb indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-armhf.deb indigo-$(INDIGO_VERSION)-$(INDIGO_BUILD)-arm64.deb
	aptly repo show -with-packages indigo-release
	aptly publish -force-drop drop indigo
	aptly publish repo indigo-release
	ln -s ~/.aptly/public ~/Desktop
