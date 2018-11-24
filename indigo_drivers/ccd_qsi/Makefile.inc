SDK_DIR = externals/qsiapi-7.6.0

SDK_INCLUDE = -I$(SDK_DIR)/build/include
SDK_LIB = -L$(SDK_DIR)/build/lib -lqsiapi
SDK_LIB_A = $(SDK_DIR)/build/lib/libqsiapi.a

MAKE_SDK = $(SDK_LIB_A)
SDK_CONFIGURE = $(SDK_DIR)/configure
SDK_CONFIGURE_IN = $(SDK_DIR)/configure.ac

LDFLAGS += -lstdc++ -lftd2xx
