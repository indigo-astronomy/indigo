#---------------------------------------------------------------------
#
# Copyright (c) 2026 CloudMakers, s. r. o.
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
#---------------------------------------------------------------------

include ../Makefile.inc

TEST_BUILD = build
UNIT_BUILD = $(TEST_BUILD)/unit
INTEGRATION_BUILD = $(TEST_BUILD)/integration
UNIT_TESTS = \
	$(UNIT_BUILD)/test_aux_math \
	$(UNIT_BUILD)/test_base64 \
	$(UNIT_BUILD)/test_bus_helpers \
	$(UNIT_BUILD)/test_bus_property \
	$(UNIT_BUILD)/test_dome_azimuth \
	$(UNIT_BUILD)/test_md5 \
	$(UNIT_BUILD)/test_polynomial_fit \
	$(UNIT_BUILD)/test_protocol_json \
	$(UNIT_BUILD)/test_protocol_xml \
	$(UNIT_BUILD)/test_timer \
	$(UNIT_BUILD)/test_token \
	$(UNIT_BUILD)/test_raw_image
INTEGRATION_TESTS = \
	$(INTEGRATION_BUILD)/test_bus_lifecycle \
	$(INTEGRATION_BUILD)/test_ccd_simulator \
	$(INTEGRATION_BUILD)/test_dome_simulator \
	$(INTEGRATION_BUILD)/test_gps_simulator \
	$(INTEGRATION_BUILD)/test_mount_simulator \
	$(INTEGRATION_BUILD)/test_polaralign_simulator \
	$(INTEGRATION_BUILD)/test_rotator_simulator

TEST_CFLAGS = $(CFLAGS) -I.. -I../indigo_libs -I../build/include
TEST_LDFLAGS = $(LDFLAGS) -Wl,-rpath,$(BUILD_LIB) -lindigo -lm -lpthread
CCD_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_ccd_simulator.a $(TEST_LDFLAGS) -lindigocat
DOME_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_simulator.a $(TEST_LDFLAGS) -lindigocat
GPS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_gps_simulator.a $(TEST_LDFLAGS) -lindigocat
MOUNT_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_simulator.a $(TEST_LDFLAGS) -lindigocat
POLARALIGN_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_polaralign_simulator.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_simulator.a $(TEST_LDFLAGS) -lindigocat

.PHONY: test test-unit test-integration test-clean check-lib

test: test-unit test-integration

test-unit: check-lib $(UNIT_TESTS)
	@for test in $(UNIT_TESTS); do \
		printf "Running %s\n" "$$test"; \
		"$$test" || exit $$?; \
	done

test-integration: check-lib $(INTEGRATION_TESTS)
	@for test in $(INTEGRATION_TESTS); do \
		printf "Running %s\n" "$$test"; \
		"$$test" || exit $$?; \
	done

check-lib:
	@test -f $(BUILD_LIB)/libindigo.a -o -f $(BUILD_LIB)/libindigo.$(SOEXT) || { \
		printf "Missing INDIGO library in $(BUILD_LIB). Run 'make all' from the repository root first.\n"; \
		exit 1; \
	}

$(UNIT_BUILD):
	mkdir -p $@

$(INTEGRATION_BUILD):
	mkdir -p $@

$(UNIT_BUILD)/test_bus_property: unit/test_bus_property.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_bus_helpers: unit/test_bus_helpers.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_token: unit/test_token.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_raw_image: unit/test_raw_image.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_base64: unit/test_base64.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_md5: unit/test_md5.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_dome_azimuth: unit/test_dome_azimuth.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_aux_math: unit/test_aux_math.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_polynomial_fit: unit/test_polynomial_fit.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_protocol_json: unit/test_protocol_json.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_protocol_xml: unit/test_protocol_xml.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(UNIT_BUILD)/test_timer: unit/test_timer.c test_runner.h | $(UNIT_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_bus_lifecycle: integration/test_bus_lifecycle.c test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_ccd_simulator: integration/test_ccd_simulator.c integration/simulator_test_common.h test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(CCD_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_simulator: integration/test_dome_simulator.c integration/simulator_test_common.h test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_gps_simulator: integration/test_gps_simulator.c integration/simulator_test_common.h test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(GPS_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_mount_simulator: integration/test_mount_simulator.c integration/simulator_test_common.h test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_polaralign_simulator: integration/test_polaralign_simulator.c integration/simulator_test_common.h test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(POLARALIGN_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_rotator_simulator: integration/test_rotator_simulator.c integration/simulator_test_common.h test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(ROTATOR_SIMULATOR_TEST_LDFLAGS)

test-clean:
	rm -rf $(TEST_BUILD)
