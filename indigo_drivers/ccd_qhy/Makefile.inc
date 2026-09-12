FIRMWARE_TARGET = $(INSTALL_FIRMWARE)/qhy
LDFLAGS += -lstdc++

# Both SDK variants compile the same generated C++ implementation.
.DEFAULT_GOAL := all
QHY_BUILD_DRIVER := indigo_$(notdir $(CURDIR))
QHY_GENERATED_CPP := ../ccd_qhy/indigo_ccd_qhy.cpp
override DRIVER_O = $(QHY_BUILD_DRIVER).o
override SOURCES = $(QHY_BUILD_DRIVER).cpp $(QHY_BUILD_DRIVER)_main.c
# Generation is handled below, not by Makefile.drv's per-source rule.
override DEFINITION =

$(QHY_GENERATED_CPP): ../ccd_qhy/indigo_ccd_qhy.driver ../../indigo_tools/indigo_generator.c
	@set -e; qhy_tmp=$$(mktemp -d ../ccd_qhy/.generated.XXXXXX); trap 'rm -rf "$$qhy_tmp"' EXIT HUP INT TERM; cp ../ccd_qhy/indigo_ccd_qhy.driver "$$qhy_tmp/"; $(BUILD_BIN)/indigo_generator "$$qhy_tmp/indigo_ccd_qhy.driver"; mv "$$qhy_tmp/indigo_ccd_qhy.h" ../ccd_qhy/; mv "$$qhy_tmp/indigo_ccd_qhy_main.c" ../ccd_qhy/; mv "$$qhy_tmp/indigo_ccd_qhy.cpp" $@

$(QHY_BUILD_DRIVER).o: $(QHY_BUILD_DRIVER).cpp $(QHY_GENERATED_CPP)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(QHY_BUILD_DRIVER)_main.o: $(QHY_BUILD_DRIVER)_main.c $(QHY_GENERATED_CPP)
