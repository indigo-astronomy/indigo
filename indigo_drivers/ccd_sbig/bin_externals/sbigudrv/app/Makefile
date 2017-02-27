# To link against -lsbigudrv in ../libs:
#   make
# To link against installed -lsbigudrv:
#   make LIBS_SBIG=-lsbigudrv

# Comment next two lines to build without FITS file support
CPPFLAGS_FITS= -DINCLUDE_FITSIO 
LIBS_FITS  = -lcfitsio

# libusb-1.0 is required
CPPFLAGS_USB = -I/usr/include/libusb-1.0
LIBS_USB = -lusb-1.0

ARCHDIR = $(PWD)/../libs/$(shell arch | sed -e 's/armv6l/arm_v6/' \
                                            -e 's/armv7l/arm_v7/' \
                                            -e 's/armv8l/arm_v8/')
LIBS_SBIG ?= -Wl,-rpath,$(ARCHDIR) -L$(ARCHDIR) -lsbigudrv

CPPFLAGS = $(CPPFLAGS_USB) $(CPPFLAGS_FITS)
CXXFLAGS = -Wall -O2
LIBS = $(LIBS_SBIG) $(LIBS_USB) $(LIBS_FITS) -lm

SRC = testmain.cpp csbigcam.cpp csbigimg.cpp
OBJ = $(SRC:.cpp=.o)

BUILD = testapp


all: $(BUILD)

testapp: $(OBJ)
	$(CXX) -o $@ $^ $(LIBS)

$(OBJ): csbigcam.h csbigimg.h

clean:
	rm -f $(OBJ) $(BUILD)
