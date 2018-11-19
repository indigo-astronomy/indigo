SDK_DIR = externals/libnexstar

SDK_INCLUDE = -I$(SDK_DIR)/src
SDK_LIB = -L$(SDK_DIR)/lib/.libs -lnexstar
SDK_LIB_A = $(SDK_DIR)/lib/.libs/libnexstar.a

MAKE_SDK = $(SDK_LIB_A)
SDK_CONFIGURE = $(SDK_DIR)/configure
SDK_CONFIGURE_IN = $(SDK_DIR)/configure.in
CLEAN_SDK = clean_sdk

CFLAGS += -Wno-constant-conversion
