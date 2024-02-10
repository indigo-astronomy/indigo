FIRMWARE_TARGET = $(INSTALL_FIRMWARE)/qhy
CFLAGS += -DQHY2
CXXFLAGS += -DQHY2
LDFLAGS += -lstdc++

FIX_SDK_LIB_DYLIB = install_name_tool -change `otool -L $(SDK_LIB_SO) | grep libusb -m1 | cut -f2 | cut -f1 -d " "` $(BUILD_LIB)/libusb-1.0.dylib $(BUILD_LIB)/$(notdir $(SDK_LIB_SO))

FIX_SDK_LIB_SO = patchelf --set-soname `basename $(SDK_LIB_SO)` $(BUILD_LIB)/$(notdir $(SDK_LIB_SO))
