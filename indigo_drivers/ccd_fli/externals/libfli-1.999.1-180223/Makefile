##########
##	Makefile for libfli
##	Usage:
##		make - create libfli.a
##		make clean - remove all *.o files and libfli.a
##	This makefile builds with the libusb-1.0 library
##		by default. To build with the old linux files,
##		change the VPATH lines below from unix/libusb
##		to unix/linux. Also, remove the "-D__LIBUSB__"
##		flag below to remove LibUSB functionality.
##########

UNAME	= $(shell uname -s)
ifeq ($(findstring BSD, $(UNAME)), BSD)
  UNAME	= BSD
endif

VPATH	= unix
ifeq ($(UNAME), Linux)
  VPATH += unix/libusb
#  VPATH	+= unix/linux
####
##	To build with LibUSB keep above
##	To build without LibUSB, swap comment sign above
##		to change VPATH from unix/libusb to unix/linux
####
endif
ifeq ($(UNAME), BSD)
  VPATH	+= unix/bsd
endif
ifeq ($(UNAME), Darwin)
  VPATH += unix/libusb
endif

DIR	= $(shell pwd)
CC	= gcc
INC	= $(DIR) $(DIR)/unix
CFLAGS	= -D__LIBUSB__ -Wall -O3 -fPIC -g $(patsubst %, -I%, $(INC))
####
##	To build with LibUSB keep "-D__LIBUSB__" flag above
##	To build without LibUSB remove this flag
####

ifeq ($(UNAME), Linux)
	CFLAGS += -Wno-unused-but-set-variable
endif

ifeq ($(UNAME), Darwin)
	CFLAGS += -Wno-absolute-value -arch x86_64
	INC += $(DIR)/unix/osx /Developer/SDKs/MacOSX10.12.sdk/usr/include /usr/local/include
	LIBS =IOkit
	LIBPATH = /usr/local/lib /sw/lib /usr/lib/ /System/Library/Frameworks/CoreFoundation.framework
	LDLIBS = $(patsubst %, -l%, $(LIBS))
	LOADLIBES = $(patsubst %, -L%, $(LIBPATH))
endif

AR	= ar
ARFLAGS	= -rus

SYS	= libfli-sys.o
DEBUG	= libfli-debug.o
MEM	= libfli-mem.o
USB_IO	= libfli-usb.o libfli-usb-sys.o
SERIAL_IO	= libfli-serial.o
IO	= $(USB_IO) $(SERIAL_IO)
CAM	= libfli-camera.o libfli-camera-parport.o libfli-camera-usb.o
FILT	= libfli-filter-focuser.o

ALLOBJ	= $(SYS) $(DEBUG) $(MEM) $(IO) $(CAM) $(FILT)

libfli.a: libfli.o $(ALLOBJ)
	$(AR) $(ARFLAGS) $@ $^

doc: doc-html doc-pdf

.PHONY: doc-html
doc-html: libfli.dxx libfli.h libfli.c
	doc++ -d doc $<

.PHONY: doc-pdf
doc-pdf: libfli.dxx libfli.h libfli.c
	(test -d pdf || mkdir pdf) && cp docxx.sty pdf
	doc++ -t $< | sed 's///g' > pdf/libfli.tex
	cd pdf && latex libfli.tex && latex libfli.tex
	cd pdf && dvips -o libfli.ps libfli.dvi && ps2pdf libfli.ps
	mv pdf/libfli.pdf . && rm -rf pdf

.PHONY: clean
clean:
	rm -f $(ALLOBJ) libfli.o libfli.a
