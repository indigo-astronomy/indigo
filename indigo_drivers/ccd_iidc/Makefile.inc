SDK_DIR = externals/libdc1394

SDK_INCLUDE = -I$(SDK_DIR)/build/include
SDK_LIB = -L$(SDK_DIR)/build/lib -ldc1394
SDK_LIB_SO = $(SDK_DIR)/build/lib/libdc1394.$(SOEXT)

MAKE_SDK = $(SDK_LIB_SO)
SDK_CONFIGURE = $(SDK_DIR)/configure
SDK_CONFIGURE_IN = $(SDK_DIR)/configure.ac

FIX_SDK_LIB_SO = install_name_tool -change `otool -L $(SDK_LIB_SO) | grep libusb | cut -f2 | cut -f1 -d " "` $(BUILD_LIB)/libusb-1.0.0.dylib $(BUILD_LIB)/$(notdir $(SDK_LIB_SO))
