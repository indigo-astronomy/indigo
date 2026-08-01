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
	$(INTEGRATION_BUILD)/test_rotator_simulator \
	$(INTEGRATION_BUILD)/test_focuser_askar_simulator \
	$(INTEGRATION_BUILD)/test_rotator_falcon2_simulator \
	$(INTEGRATION_BUILD)/test_wheel_xagyl_simulator \
	$(INTEGRATION_BUILD)/test_wheel_quantum_simulator \
	$(INTEGRATION_BUILD)/test_wheel_trutek_simulator \
	$(INTEGRATION_BUILD)/test_wheel_qhy_simulator \
	$(INTEGRATION_BUILD)/test_wheel_optec_simulator \
	$(INTEGRATION_BUILD)/test_focuser_fc3_simulator \
	$(INTEGRATION_BUILD)/test_focuser_qhy_simulator \
	$(INTEGRATION_BUILD)/test_focuser_optecfl_simulator \
	$(INTEGRATION_BUILD)/test_focuser_lacerta_simulator \
	$(INTEGRATION_BUILD)/test_aux_svbpowerbox_simulator \
	$(INTEGRATION_BUILD)/test_aux_wbplusv3_simulator \
	$(INTEGRATION_BUILD)/test_aux_wbprov3_simulator \
	$(INTEGRATION_BUILD)/test_aux_wcv4ec_simulator \
	$(INTEGRATION_BUILD)/test_aux_arteskyflat_simulator \
	$(INTEGRATION_BUILD)/test_aux_astromechanics_simulator \
	$(INTEGRATION_BUILD)/test_aux_fbc_simulator \
	$(INTEGRATION_BUILD)/test_aux_flatmaster_simulator \
	$(INTEGRATION_BUILD)/test_aux_flipflat_simulator \
	$(INTEGRATION_BUILD)/test_ao_sx_simulator \
	$(INTEGRATION_BUILD)/test_mount_synscan_simulator \
	$(INTEGRATION_BUILD)/test_aux_upb3_simulator

TEST_CFLAGS = $(CFLAGS) -I.. -I../indigo_libs -I../build/include
TEST_LDFLAGS = $(LDFLAGS) -Wl,-rpath,$(BUILD_LIB) -lindigo -lm -lpthread
CCD_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_ccd_simulator.a $(TEST_LDFLAGS) -lindigocat
DOME_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_simulator.a $(TEST_LDFLAGS) -lindigocat
GPS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_gps_simulator.a $(TEST_LDFLAGS) -lindigocat
MOUNT_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_simulator.a $(TEST_LDFLAGS) -lindigocat
POLARALIGN_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_polaralign_simulator.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_simulator.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_ASKAR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_askar.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_FALCON2_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_falcon.a $(TEST_LDFLAGS) -lindigocat
WHEEL_XAGYL_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_xagyl.a $(TEST_LDFLAGS) -lindigocat
WHEEL_QUANTUM_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_quantum.a $(TEST_LDFLAGS) -lindigocat
WHEEL_TRUTEK_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_trutek.a $(TEST_LDFLAGS) -lindigocat
WHEEL_QHY_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_qhy.a $(TEST_LDFLAGS) -lindigocat
WHEEL_OPTEC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_optec.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_FC3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_fc3.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_QHY_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_qhy.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_OPTECFL_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_optecfl.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_LACERTA_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_lacerta.a $(TEST_LDFLAGS) -lindigocat
AUX_SVBPOWERBOX_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_svbpowerbox.a $(TEST_LDFLAGS) -lindigocat
AUX_WBPLUSV3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_wbplusv3.a $(TEST_LDFLAGS) -lindigocat
AUX_WBPROV3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_wbprov3.a $(TEST_LDFLAGS) -lindigocat
AUX_WCV4EC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_wcv4ec.a $(TEST_LDFLAGS) -lindigocat
AUX_ARTESKYFLAT_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_arteskyflat.a $(TEST_LDFLAGS) -lindigocat
AUX_ASTROMECHANICS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_astromechanics.a $(TEST_LDFLAGS) -lindigocat
AUX_FBC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_fbc.a $(TEST_LDFLAGS) -lindigocat
AUX_FLATMASTER_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_flatmaster.a $(TEST_LDFLAGS) -lindigocat
AUX_FLIPFLAT_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_flipflat.a $(TEST_LDFLAGS) -lindigocat
AO_SX_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_ao_sx.a $(TEST_LDFLAGS) -lindigocat
MOUNT_SYNSCAN_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_synscan.a $(TEST_LDFLAGS) -lindigocat
AUX_UPB3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_upb3.a $(TEST_LDFLAGS) -lindigocat

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

$(INTEGRATION_BUILD)/focuser_askar_simulator: ../indigo_drivers/focuser_askar/focuser_askar_simulator/focuser_askar_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/rotator_falcon2_simulator: ../indigo_drivers/rotator_falcon/rotator_falcon2_simulator/rotator_falcon2_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $< -lm

$(INTEGRATION_BUILD)/wheel_xagyl_simulator: ../indigo_drivers/wheel_xagyl/wheel_xagyl_simulator/wheel_xagyl_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/wheel_quantum_simulator: ../indigo_drivers/wheel_quantum/wheel_quantum_simulator/wheel_quantum_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/wheel_trutek_simulator: ../indigo_drivers/wheel_trutek/wheel_trutek_simulator/wheel_trutek_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/wheel_qhy_simulator: ../indigo_drivers/wheel_qhy/wheel_qhy_simulator/wheel_qhy_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/wheel_optec_simulator: ../indigo_drivers/wheel_optec/wheel_optec_simulator/wheel_optec_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_fc3_simulator: ../indigo_drivers/focuser_fc3/focuser_fc3_simulator/focuser_fc3_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_qhy_simulator: ../indigo_drivers/focuser_qhy/focuser_qhy_simulator/focuser_qhy_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_optecfl_simulator: ../indigo_drivers/focuser_optecfl/focuser_optecfl_simulator/focuser_optecfl_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_lacerta_simulator: ../indigo_drivers/focuser_lacerta/focuser_lacerta_simulator/focuser_lacerta_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/aux_svbpowerbox_simulator: ../indigo_drivers/aux_svbpowerbox/aux_svbpowerbox_simulator/aux_svbpowerbox_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/aux_wbplusv3_simulator: ../indigo_drivers/aux_wbplusv3/aux_wbplusv3_simulator/aux_wbplusv3_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/aux_wbprov3_simulator: ../indigo_drivers/aux_wbprov3/aux_wbprov3_simulator/aux_wbprov3_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/aux_wcv4ec_simulator: ../indigo_drivers/aux_wcv4ec/aux_wcv4ec_simulator/aux_wcv4ec_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/aux_arteskyflat_simulator: ../indigo_drivers/aux_arteskyflat/aux_arteskyflat_simulator/aux_arteskyflat_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_astromechanics_simulator: ../indigo_drivers/aux_astromechanics/aux_astromechanics_simulator/aux_astromechanics_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_fbc_simulator: ../indigo_drivers/aux_fbc/aux_fbc_simulator/aux_fbc_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_flatmaster_simulator: ../indigo_drivers/aux_flatmaster/aux_flatmaster_simulator/aux_flatmaster_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_flipflat_simulator: ../indigo_drivers/aux_flipflat/aux_flipflat_simulator/aux_flipflat_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/ao_sx_simulator: ../indigo_drivers/ao_sx/ao_sx_simulator/ao_sx_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/mount_synscan_simulator: ../indigo_drivers/mount_synscan/mount_synscan_simulator/mount_synscan_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/aux_upb3_simulator: ../indigo_drivers/aux_upb3/aux_upb3_simulator/aux_upb3_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/test_focuser_askar_simulator: integration/test_focuser_askar_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_askar_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_ASKAR_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_rotator_falcon2_simulator: integration/test_rotator_falcon2_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_falcon2_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(ROTATOR_FALCON2_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_xagyl_simulator: integration/test_wheel_xagyl_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_xagyl_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_XAGYL_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_quantum_simulator: integration/test_wheel_quantum_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_quantum_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_QUANTUM_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_trutek_simulator: integration/test_wheel_trutek_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_trutek_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_TRUTEK_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_qhy_simulator: integration/test_wheel_qhy_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_qhy_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_QHY_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_optec_simulator: integration/test_wheel_optec_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_optec_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_OPTEC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_fc3_simulator: integration/test_focuser_fc3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_fc3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_FC3_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_qhy_simulator: integration/test_focuser_qhy_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_qhy_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_QHY_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_optecfl_simulator: integration/test_focuser_optecfl_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_optecfl_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_OPTECFL_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_lacerta_simulator: integration/test_focuser_lacerta_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_lacerta_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_LACERTA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_svbpowerbox_simulator: integration/test_aux_svbpowerbox_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_svbpowerbox_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_SVBPOWERBOX_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_wbplusv3_simulator: integration/test_aux_wbplusv3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_wbplusv3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_WBPLUSV3_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_wbprov3_simulator: integration/test_aux_wbprov3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_wbprov3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_WBPROV3_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_wcv4ec_simulator: integration/test_aux_wcv4ec_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_wcv4ec_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_WCV4EC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_arteskyflat_simulator: integration/test_aux_arteskyflat_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_arteskyflat_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_ARTESKYFLAT_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_astromechanics_simulator: integration/test_aux_astromechanics_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_astromechanics_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_ASTROMECHANICS_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_fbc_simulator: integration/test_aux_fbc_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_fbc_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_FBC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_flatmaster_simulator: integration/test_aux_flatmaster_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_flatmaster_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_FLATMASTER_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_flipflat_simulator: integration/test_aux_flipflat_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_flipflat_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_FLIPFLAT_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_ao_sx_simulator: integration/test_ao_sx_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/ao_sx_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AO_SX_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_mount_synscan_simulator: integration/test_mount_synscan_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_synscan_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_SYNSCAN_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_upb3_simulator: integration/test_aux_upb3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_upb3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_UPB3_SIMULATOR_TEST_LDFLAGS)

test-clean:
	rm -rf $(TEST_BUILD)
