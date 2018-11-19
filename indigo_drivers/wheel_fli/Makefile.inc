SDK_DIR = externals/libfli-1.999.1-180223

SDK_INCLUDE = -I$(SDK_DIR)
SDK_LIB = -L$(SDK_DIR) -lfli
SDK_LIB_A = $(SDK_DIR)/libfli.a

MAKE_SDK = $(SDK_LIB_A)
CLEAN_SDK = clean_sdk

