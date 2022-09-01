LDFLAGS += -lstdc++
FIX_SDK_LIB_SO = patchelf --set-soname `basename $(SDK_LIB_SO)` $(BUILD_LIB)/$(notdir $(SDK_LIB_SO))
