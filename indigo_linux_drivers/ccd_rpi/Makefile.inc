ARCH := $(shell uname -m)
OS := $(shell uname -s)

ifeq ($(OS),Linux)
  ifneq (,$(filter $(ARCH),aarch64 armv7l))
    CCD_RPI := 1
  endif
endif

ifeq ($(CCD_RPI), 1)

SDK_DIR ?= $(wildcard bin_externals/*/lib/Linux/$(ARCH_DETECTED))

SDK_INCLUDE ?= $(addprefix -I, $(wildcard bin_externals/*/include))
SDK_LIB = -L$(SDK_DIR)
SDK_LIB_A = $(SDK_DIR)/libcamera.a

CXXFLAGS += -std=gnu++17
LDFLAGS += -Wl,--whole-archive $(SDK_LIB_A) -Wl,--no-whole-archive -lstdc++ -ludev -lpthread -ldl -lm

all:
	@echo "CCD_RPI: Building for aarch64/armv7l"

else

all:
	@echo "CCD_RPI: Skipping build - ARCH is not 'aarch64' or 'armv7l' (was '$(ARCH)')"

endif

