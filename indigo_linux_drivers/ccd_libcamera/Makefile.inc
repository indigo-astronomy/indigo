# libcamera is optional at build time: without its development files the bridge is built as a stub
# that reports no cameras, so indigo_linux_drivers still builds on every Linux host.
LIBCAMERA_CFLAGS := $(shell pkg-config --cflags libcamera 2>/dev/null)
LIBCAMERA_LIBS := $(shell pkg-config --libs libcamera 2>/dev/null)

ifneq ($(strip $(LIBCAMERA_LIBS)),)
CXXFLAGS += -DHAVE_LIBCAMERA $(LIBCAMERA_CFLAGS)
LDFLAGS += $(LIBCAMERA_LIBS)
endif

# libcamera 0.7 headers use std::span, so they need C++20.
CXXFLAGS += -std=gnu++20
ifeq ($(OS_DETECTED),Darwin)
LDFLAGS += -lc++
else
LDFLAGS += -lstdc++
endif
