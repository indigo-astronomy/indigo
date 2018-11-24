SDK_DIR = externals/libdc1394

SDK_INCLUDE = -I$(SDK_DIR)/build/include
SDK_LIB = -L$(SDK_DIR)/build/lib -ldc1394
SDK_LIB_SO = $(SDK_DIR)/build/lib/libdc1394.$(SOEXT)

MAKE_SDK = $(SDK_LIB_SO)
SDK_CONFIGURE = $(SDK_DIR)/configure
SDK_CONFIGURE_IN = $(SDK_DIR)/configure.ac
