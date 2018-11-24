SDK_DIR = externals/libnexstar
SDK_INCLUDE = -I$(SDK_DIR)/build/include
SDK_LIB = -L$(SDK_DIR)/build/lib -lnexstar
SDK_LIB_A = $(SDK_DIR)/build/lib/libnexstar.a

MAKE_SDK = $(SDK_LIB_A)
SDK_CONFIGURE = $(SDK_DIR)/configure
SDK_CONFIGURE_IN = $(SDK_DIR)/configure.in
