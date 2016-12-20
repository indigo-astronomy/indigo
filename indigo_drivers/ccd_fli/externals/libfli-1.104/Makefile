UNAME	= $(shell uname -s)
ifeq ($(findstring BSD, $(UNAME)), BSD)
  UNAME	= BSD
endif

VPATH	= unix
ifeq ($(UNAME), Linux)
  VPATH	+= unix/linux
endif
ifeq ($(UNAME), BSD)
  VPATH	+= unix/bsd
endif
ifeq ($(UNAME), Darwin)
  VPATH	+= unix/osx
endif

DIR	= $(shell pwd)
CC	= gcc
INC	= $(DIR) $(DIR)/unix
CFLAGS	= -Wall -O2 -g $(patsubst %, -I%, $(INC))

ifeq ($(UNAME), Darwin)
	CFLAGS += -arch i386 -arch x86_64
	INC += $(DIR)/unix/osx /Developer/SDKs/MacOSX10.7.sdk/usr/include
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
ifeq ($(UNAME), Linux)
  PPORT_IO	= libfli-parport.o
endif
SERIAL_IO	= libfli-serial.o
IO	= $(USB_IO) $(PPORT_IO) $(SERIAL_IO)
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
	doc++ -t $< | sed 's///g' > pdf/libfli.tex
	cd pdf && latex libfli.tex && latex libfli.tex
	cd pdf && dvips -o libfli.ps libfli.dvi && ps2pdf libfli.ps
	mv pdf/libfli.pdf . && rm -rf pdf

.PHONY: clean
clean:
	rm -f $(ALLOBJ) libfli.o libfli.a
