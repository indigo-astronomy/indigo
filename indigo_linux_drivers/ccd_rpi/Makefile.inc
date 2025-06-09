SDK_DIR ?= $(wildcard bin_externals/*/lib/Linux/$(ARCH_DETECTED))

SDK_INCLUDE ?= $(addprefix -I, $(wildcard bin_externals/*/include))
SDK_LIB = -L$(SDK_DIR)
SDK_LIB_A = $(SDK_DIR)/libcamera.a

CXXFLAGS += -std=gnu++17
LDFLAGS += -Wl,--whole-archive $(SDK_LIB_A) -Wl,--no-whole-archive -lstdc++ -ludev -lpthread -ldl -lm


