LDFLAGS += -lstdc++
FIX_SDK_LIB_DYLIB = install_name_tool -change `otool -L $(SDK_LIB_SO) | grep libusb -m1 | cut -f2 | cut -f1 -d " "` $(BUILD_LIB)/libusb-1.0.dylib $(BUILD_LIB)/$(notdir $(SDK_LIB_SO))
