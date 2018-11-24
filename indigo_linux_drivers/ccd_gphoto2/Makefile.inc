SDK_DIR = externals/libraw

SDK_INCLUDE = -I$(SDK_DIR)/build/include
SDK_LIB = -L$(SDK_DIR)/build/lib -lraw
SDK_LIB_A = $(SDK_DIR)/build/lib/libraw.a

MAKE_SDK = $(SDK_LIB_A)
SDK_CONFIGURE = $(SDK_DIR)/configure
SDK_CONFIGURE_IN = $(SDK_DIR)/configure.ac

CPPFLAFS += -I
LDFLAGS += -lstdc++ -lgphoto2
CONFIGURE_FLAGS = --disable-jasper --disable-lcms --disable-examples --disable-jpeg --disable-openmp
