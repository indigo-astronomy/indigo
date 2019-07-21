# set the include path found by configure

lib_LTLIBRARIES = libqsiapi.la 

if ENABLE_LIBFTD2XX
FTD2XXSTACK = -Wc,-DUSELIBFTD2XX
else 
FTD2XXSTACK =
endif

if ENABLE_LIBFTDI
FTDISTACK = -Wc,-DUSELIBFTDIZERO
else
FTDISTACK =
endif

if ENABLE_LIBFTDI1
FTDI1STACK = -Wc,-DUSELIBFTDIONE
else
FTDI1STACK =
endif

libqsiapi_la_CPPFLAGS = $(all_includes) $(FTD2XX_CFLAGS) $(FTDI_CFLAGS) $(FTDISTACK) $(FTDI1STACK) $(FTD2XXSTACK) $(CY_CPPFLAGS)

libqsiapi_la_CXXFLAGS = -fPIC -g -fpermissive -Wall

include_HEADERS = qsiapi.h QSIError.h

noinst_HEADERS = CCDCamera.h CameraID.h ConvertUTF.h Filter.h FilterWheel.h HotPixelMap.h QSILog.h \
	QSI_Global.h QSI_Interface.h QSI_PacketWrapper.h QSI_Registry.h \
	QSI_States.h QSI_USBWrapper.h SimpleIni.h WinTypes.h ftd2xx.h wincompat.h QSICriticalSection.h QSIFeatures.h \
    HotPixelMap.h VidPid.h  QSIModelInfo.h ICameraEeprom.h HostIO_USB.h IHostIO.h HostConnection.h HostIO_TCP.h HostIO_CyUSB.h

libqsiapi_la_SOURCES = CCDCamera.cpp CameraID.cpp ConvertUTF.c Filter.cpp \
	FilterWheel.cpp HotPixelMap.cpp QSI_PacketWrapper.cpp QSI_USBWrapper.cpp qsiapi.cpp qsicopyright.txt QSIFeatures.cpp \
    VidPid.cpp QSIModelInfo.cpp ICameraEeprom.cpp QSI_Interface.cpp QSILog.cpp HostIO_TCP.cpp  HostIO_USB.cpp IHostIO.cpp HostConnection.cpp \
	QSIError.cpp HostIO_CyUSB.cpp

# version-info a:b:c yields a library name (a-c).c.b
# example (3:2:1) yields 2.1.2
#libqsiapi_la_LDFLAGS = -version-info 0:0:0 $(all_libraries) -fPIC --shared

libqsiapi_la_LIBADD = -L/usr/local/lib $(FTD2XX_LIBS) $(FTDI_LIBS) $(CY_LIBS) -lc

libqsiapi_la_METASOURCES = AUTO

libqsiapi_la_DEPENDENCIES = 
