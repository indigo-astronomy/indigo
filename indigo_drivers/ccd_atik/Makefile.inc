FIX_SDK_LIB_SO = install_name_tool -change `otool -L $(SDK_LIB_SO) | grep libusb | cut -f2 | cut -f1 -d " "` $(BUILD_LIB)/libusb-1.0.0.dylib $(BUILD_LIB)/$(notdir $(SDK_LIB_SO))
