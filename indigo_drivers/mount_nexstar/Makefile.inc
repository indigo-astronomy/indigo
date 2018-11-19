SDK_DIR = externals/libnexstar

SDK_INCLUDE = -I$(SDK_DIR)/src
SDK_LIB = -L$(SDK_DIR)/lib/.libs -lnexstar
SDK_LIB_A = $(SDK_DIR)/lib/.libs/libnexstar.a

MAKE_SDK = $(SDK_LIB_A)
CLEAN_SDK = clean_sdk

CFLAGS += -Wno-constant-conversion
