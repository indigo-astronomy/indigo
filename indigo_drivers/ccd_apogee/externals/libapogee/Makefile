#
# libapogee - interface library for Apogee cameras
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

LIBAPOGEE=libapogee.a

ifeq ($(OS_DETECTED),Darwin)
	CC=gcc
	CPPFLAGS=-fPIC -O3 -I $(INDIGO_ROOT)/build/include -I. -Ilinux -std=gnu++11 -mmacosx-version-min=10.10
	LDFLAGS=-framework CoreFoundation -framework IOKit -Llib/macOS -lusb-1.0
	AR=ar
	ARFLAGS=-rv
endif

ifeq ($(OS_DETECTED),Linux)
	CC=gcc
	ifeq ($(ARCH_DETECTED),arm)
		CPPFLAGS=-fPIC -O3 -I $(INDIGO_ROOT)/build/include -I. -Ilinux -std=gnu++11 -pthread -march=armv6 -mfpu=vfp -mfloat-abi=hard
	else
		CPPFLAGS=-fPIC -O3 -I $(INDIGO_ROOT)/build/include -I. -Ilinux -std=gnu++11 -pthread
	endif
	LDFLAGS=-lm -lrt -lusb-1.0 -Llib/Linux/$(ARCH_DETECTED) -lhidapi-libusb
	AR=ar
	ARFLAGS=-rv
endif

.PHONY: clean remote

all: $(LIBAPOGEE)

$(LIBAPOGEE):	Alta.o AltaCcdAcqParams.o AltaData.o AltaEthernetIo.o AltaF.o AltaIo.o AltaModeFsm.o AltaUsbIo.o ApgLogger.o ApgTimer.o ApnCamData.o ApogeeCam.o ApogeeFilterWheel.o Ascent.o AscentBasedIo.o AscentBasedUsbIo.o AscentData.o Aspen.o AspenData.o AspenEthernetIo.o AspenIo.o AspenUsbIo.o CamCfgMatrix.o CamGen2Base.o CamGen2CcdAcqParams.o CamGen2ModeFsm.o CamHelpers.o CamRegMirror.o CamUsbIo.o CameraInfo.o CameraIo.o CameraStatusRegs.o CcdAcqParams.o DeviceStrHelpers.o FilterWheelIo.o FindDeviceEthernet.o FindDeviceUsb.o HiC.o IAltaSerialPortIo.o ICamIo.o ILog.o ITimer.o IUsb.o ImgFix.o LoggerException.o ModeFsm.o PlatformData.o PromFx2Io.o Quad.o UdpSocketBase.o apgHelper.o helpers.o libCurlWrap.o parseCfgTabDelim.o linux/GenOneLinuxUSB.o linux/LinuxTimer.o linux/LoggerSyslog.o linux/UdpSocketLinux.o linux/linuxHelpers.o
	$(AR) $(ARFLAGS) $@ $^

install-config:
ifndef CONFIG_PREFIX
	$(error CONFIG_PREFIX is not set)
endif
ifndef RULES_PREFIX
	$(error RULES_PREFIX is not set)
endif
	install -d $(CONFIG_PREFIX)
	install -D -m 0644 conf/* $(CONFIG_PREFIX)
	install -D -m 0644 99-apogee.rules $(RULES_PREFIX)/99-indigo_ccd_apogee.rules

clean:
	rm -f $(LIBAPOGEE) *.o
	rm -f $(LIBAPOGEE) *.a
