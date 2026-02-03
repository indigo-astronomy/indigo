ifeq ($(OS_DETECTED),Darwin)
LDFLAGS += -lstdc++
else
LDFLAGS += -lstdc++ -ludev -ldl
endif
FIX_SDK_LIB_SO = \
    if [ -f "$(BUILD_LIB)/libWrapperSdbus.so" ]; then \
        patchelf --replace-needed libsdbus-c++.so.2 libsdbus-zwo.so "$(BUILD_LIB)/libWrapperSdbus.so"; \
        patchelf --set-rpath '$$ORIGIN/../lib:$$ORIGIN/../drivers:.' "$(BUILD_LIB)/libWrapperSdbus.so"; \
    fi; \
    if [ -f "$(BUILD_LIB)/libsdbus-zwo.so" ]; then \
        patchelf --set-soname "$(notdir $(BUILD_LIB)/libsdbus-zwo.so)" "$(BUILD_LIB)/libsdbus-zwo.so"; \
    fi
