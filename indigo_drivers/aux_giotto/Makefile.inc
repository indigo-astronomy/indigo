# The PrimaLuce Lab protocol library (indigo_drivers/shared/primalucelib) is linked statically into
# the driver library and executable. Its own Makefile decides whether it has to be rebuilt.

# This file is included before the targets of Makefile.drv, so 'all' has to stay the default goal.
.DEFAULT_GOAL := all

PRIMALUCELIB_DIR = ../shared/primalucelib
PRIMALUCELIB = $(BUILD_LIB)/libprimalucelib.a

SDK_LIB = $(PRIMALUCELIB)
MAKE_SDK = $(PRIMALUCELIB)

.PHONY: primalucelib_check

$(PRIMALUCELIB): primalucelib_check
	$(MAKE) -C $(PRIMALUCELIB_DIR) all

$(BUILD_DRIVERS)/indigo_aux_giotto.$(SOEXT) $(BUILD_DRIVERS)/indigo_aux_giotto: $(PRIMALUCELIB)
