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
BENCHMARK_BUILD = $(TEST_BUILD)/benchmark
BENCHMARKS = \
	$(BENCHMARK_BUILD)/bench_timer \
	$(BENCHMARK_BUILD)/bench_queue
UNIT_TESTS = \
	$(UNIT_BUILD)/test_align_math \
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
	$(INTEGRATION_BUILD)/test_dome_baader_simulator \
	$(INTEGRATION_BUILD)/test_dome_beaver_simulator \
	$(INTEGRATION_BUILD)/test_dome_nexdome_simulator \
	$(INTEGRATION_BUILD)/test_dome_nexdome3_simulator \
	$(INTEGRATION_BUILD)/test_dome_skyroof_simulator \
	$(INTEGRATION_BUILD)/test_dome_talon6ror_simulator \
	$(INTEGRATION_BUILD)/test_gps_simulator \
	$(INTEGRATION_BUILD)/test_gps_nmea_simulator \
	$(INTEGRATION_BUILD)/test_mount_ioptron_simulator \
	$(INTEGRATION_BUILD)/test_mount_lx200_simulator \
	$(INTEGRATION_BUILD)/test_mount_nexstar_simulator \
	$(INTEGRATION_BUILD)/test_mount_nexstaraux_simulator \
	$(INTEGRATION_BUILD)/test_mount_pmc8_simulator \
	$(INTEGRATION_BUILD)/test_mount_rainbow_simulator \
	$(INTEGRATION_BUILD)/test_mount_simulator \
	$(INTEGRATION_BUILD)/test_mount_starbook_simulator \
	$(INTEGRATION_BUILD)/test_polaralign_simulator \
	$(INTEGRATION_BUILD)/test_rotator_simulator \
	$(INTEGRATION_BUILD)/test_rotator_optec_simulator \
	$(INTEGRATION_BUILD)/test_rotator_wa_simulator \
	$(INTEGRATION_BUILD)/test_focuser_astromechanics_simulator \
	$(INTEGRATION_BUILD)/test_focuser_askar_simulator \
	$(INTEGRATION_BUILD)/test_rotator_falcon2_simulator \
	$(INTEGRATION_BUILD)/test_wheel_xagyl_simulator \
	$(INTEGRATION_BUILD)/test_wheel_indigo_simulator \
	$(INTEGRATION_BUILD)/test_wheel_quantum_simulator \
	$(INTEGRATION_BUILD)/test_wheel_trutek_simulator \
	$(INTEGRATION_BUILD)/test_wheel_qhy_simulator \
	$(INTEGRATION_BUILD)/test_wheel_optec_simulator \
	$(INTEGRATION_BUILD)/test_focuser_fc3_simulator \
	$(INTEGRATION_BUILD)/test_focuser_efa_simulator \
	$(INTEGRATION_BUILD)/test_focuser_dmfc_simulator \
	$(INTEGRATION_BUILD)/test_focuser_dsd_simulator \
	$(INTEGRATION_BUILD)/test_focuser_focusdreampro_simulator \
	$(INTEGRATION_BUILD)/test_focuser_ioptron_simulator \
	$(INTEGRATION_BUILD)/test_focuser_qhy_simulator \
	$(INTEGRATION_BUILD)/test_focuser_optecfl_simulator \
	$(INTEGRATION_BUILD)/test_focuser_lacerta_simulator \
	$(INTEGRATION_BUILD)/test_focuser_lakeside_simulator \
	$(INTEGRATION_BUILD)/test_focuser_mjkzz_simulator \
	$(INTEGRATION_BUILD)/test_focuser_moonlite_simulator \
	$(INTEGRATION_BUILD)/test_focuser_mypro2_simulator \
	$(INTEGRATION_BUILD)/test_focuser_nfocus_simulator \
	$(INTEGRATION_BUILD)/test_focuser_nstep_simulator \
	$(INTEGRATION_BUILD)/test_focuser_optec_simulator \
	$(INTEGRATION_BUILD)/test_focuser_primaluce_simulator \
	$(INTEGRATION_BUILD)/test_focuser_prodigy_simulator \
	$(INTEGRATION_BUILD)/test_focuser_robofocus_simulator \
	$(INTEGRATION_BUILD)/test_focuser_steeldrive2_simulator \
	$(INTEGRATION_BUILD)/test_focuser_usbv3_simulator \
	$(INTEGRATION_BUILD)/test_focuser_wemacro_simulator \
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
	$(INTEGRATION_BUILD)/test_aux_upb3_simulator \
	$(INTEGRATION_BUILD)/test_aux_ppb_simulator \
	$(INTEGRATION_BUILD)/test_aux_skyalert_simulator \
	$(INTEGRATION_BUILD)/test_aux_sqm_simulator \
	$(INTEGRATION_BUILD)/test_aux_uch_simulator \
	$(INTEGRATION_BUILD)/test_aux_usbdp_simulator

TEST_CFLAGS = $(CFLAGS) -I.. -I../indigo_libs -I../build/include
TEST_LDFLAGS = $(LDFLAGS) -Wl,-rpath,$(BUILD_LIB) -lindigo -lm -lpthread
CCD_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_ccd_simulator.a $(TEST_LDFLAGS) -lindigocat
DOME_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_simulator.a $(TEST_LDFLAGS) -lindigocat
DOME_BAADER_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_baader.a $(TEST_LDFLAGS) -lindigocat
DOME_BEAVER_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_beaver.a $(TEST_LDFLAGS) -lindigocat
DOME_NEXDOME_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_nexdome.a $(TEST_LDFLAGS) -lindigocat
DOME_NEXDOME3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_nexdome3.a $(TEST_LDFLAGS) -lindigocat
DOME_SKYROOF_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_skyroof.a $(TEST_LDFLAGS) -lindigocat
DOME_TALON6ROR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_dome_talon6ror.a $(TEST_LDFLAGS) -lindigocat
GPS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_gps_simulator.a $(TEST_LDFLAGS) -lindigocat
GPS_NMEA_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_gps_nmea.a $(TEST_LDFLAGS) -lindigocat
MOUNT_IOPTRON_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_ioptron.a $(TEST_LDFLAGS) -lindigocat
MOUNT_LX200_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_lx200.a $(TEST_LDFLAGS) -lindigocat
MOUNT_NEXSTAR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_nexstar.a $(TEST_LDFLAGS) ../indigo_drivers/mount_nexstar/externals/libnexstar/build/lib/libnexstar.a -lindigocat
MOUNT_NEXSTARAUX_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_nexstaraux.a $(TEST_LDFLAGS) -lindigocat
MOUNT_PMC8_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_pmc8.a $(TEST_LDFLAGS) -lindigocat
MOUNT_RAINBOW_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_rainbow.a $(TEST_LDFLAGS) -lindigocat
MOUNT_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_simulator.a $(TEST_LDFLAGS) -lindigocat
MOUNT_STARBOOK_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_starbook.a $(TEST_LDFLAGS) -lcurl -lindigocat
MOUNT_TEMMA_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_mount_temma.a $(TEST_LDFLAGS) -lindigocat
POLARALIGN_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_polaralign_simulator.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_simulator.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_OPTEC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_optec.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_WA_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_wa.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_ASTROMECHANICS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_astromechanics.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_ASKAR_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_askar.a $(TEST_LDFLAGS) -lindigocat
ROTATOR_FALCON2_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_falcon.a $(TEST_LDFLAGS) -lindigocat
WHEEL_XAGYL_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_xagyl.a $(TEST_LDFLAGS) -lindigocat
WHEEL_INDIGO_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_indigo.a $(TEST_LDFLAGS) -lindigocat
WHEEL_QUANTUM_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_quantum.a $(TEST_LDFLAGS) -lindigocat
WHEEL_TRUTEK_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_trutek.a $(TEST_LDFLAGS) -lindigocat
WHEEL_QHY_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_qhy.a $(TEST_LDFLAGS) -lindigocat
WHEEL_OPTEC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_wheel_optec.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_FC3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_fc3.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_EFA_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_efa.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_DMFC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_dmfc.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_DSD_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_dsd.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_FOCUSDREAMPRO_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_focusdreampro.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_IOPTRON_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_ioptron.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_QHY_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_qhy.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_OPTECFL_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_optecfl.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_LACERTA_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_lacerta.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_LAKESIDE_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_lakeside.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_MJKZZ_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_mjkzz.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_MOONLITE_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_moonlite.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_MYPRO2_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_mypro2.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_NFOCUS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_nfocus.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_NSTEP_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_nstep.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_OPTEC_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_optec.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_PRIMALUCE_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_primaluce.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_PRODIGY_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_prodigy.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_ROBOFOCUS_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_robofocus.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_STEELDRIVE2_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_steeldrive2.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_USBV3_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_usbv3.a $(TEST_LDFLAGS) -lindigocat
FOCUSER_WEMACRO_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_focuser_wemacro.a $(TEST_LDFLAGS) -lindigocat
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
AUX_PPB_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_ppb.a $(TEST_LDFLAGS) -lindigocat
AUX_SKYALERT_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_skyalert.a $(TEST_LDFLAGS) -lindigocat
AUX_SQM_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_sqm.a $(TEST_LDFLAGS) -lindigocat
AUX_UCH_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_uch.a $(TEST_LDFLAGS) -lindigocat
AUX_USBDP_SIMULATOR_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_aux_usbdp.a $(TEST_LDFLAGS) -lindigocat

.PHONY: test test-unit test-integration benchmark test-clean check-lib

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

# Benchmarks measure timing, they do not pass or fail, so they are not part of
# the test target. Results are comparable only between runs on the same machine.
benchmark: check-lib $(BENCHMARKS)
	@for benchmark in $(BENCHMARKS); do \
		printf "Running %s\n" "$$benchmark"; \
		"$$benchmark" || exit $$?; \
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

$(BENCHMARK_BUILD):
	mkdir -p $@

$(BENCHMARK_BUILD)/bench_timer: benchmark/bench_timer.c | $(BENCHMARK_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(BENCHMARK_BUILD)/bench_queue: benchmark/bench_queue.c | $(BENCHMARK_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

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

$(UNIT_BUILD)/test_align_math: unit/test_align_math.c test_runner.h | $(UNIT_BUILD)
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

$(INTEGRATION_BUILD)/dome_baader_simulator: ../indigo_drivers/dome_baader/dome_baader_simulator/dome_baader_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/dome_beaver_simulator: ../indigo_drivers/dome_beaver/dome_beaver_simulator/dome_beaver_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/dome_nexdome_simulator: ../indigo_drivers/dome_nexdome/dome_nexdome_simulator/dome_nexdome_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/dome_nexdome3_simulator: ../indigo_drivers/dome_nexdome3/dome_nexdome3_simulator/dome_nexdome3_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/dome_skyroof_simulator: ../indigo_drivers/dome_skyroof/dome_skyroof_simulator/dome_skyroof_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/dome_talon6ror_simulator: ../indigo_drivers/dome_talon6ror/dome_talon6ror_simulator/dome_talon6ror_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/gps_nmea_simulator: ../indigo_drivers/gps_nmea/gps_nmea_simulator/gps_nmea_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/mount_ioptron_simulator: ../indigo_drivers/mount_ioptron/mount_ioptron_simulator/mount_ioptron_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_astromechanics_simulator: ../indigo_drivers/focuser_astromechanics/focuser_astromechanics_simulator/focuser_astromechanics_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

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

$(INTEGRATION_BUILD)/focuser_efa_simulator: ../indigo_drivers/focuser_efa/focuser_efa_simulator/focuser_efa_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_dmfc_simulator: ../indigo_drivers/focuser_dmfc/focuser_dmfc_simulator/focuser_dmfc_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_dsd_simulator: ../indigo_drivers/focuser_dsd/focuser_dsd_simulator/focuser_dsd_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_focusdreampro_simulator: ../indigo_drivers/focuser_focusdreampro/focuser_focusdreampro_simulator/focuser_focusdreampro_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_ioptron_simulator: ../indigo_drivers/focuser_ioptron/focuser_ioptron_simulator/focuser_ioptron_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_qhy_simulator: ../indigo_drivers/focuser_qhy/focuser_qhy_simulator/focuser_qhy_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_optecfl_simulator: ../indigo_drivers/focuser_optecfl/focuser_optecfl_simulator/focuser_optecfl_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_lacerta_simulator: ../indigo_drivers/focuser_lacerta/focuser_lacerta_simulator/focuser_lacerta_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_lakeside_simulator: ../indigo_drivers/focuser_lakeside/focuser_lakeside_simulator/focuser_lakeside_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_mjkzz_simulator: ../indigo_drivers/focuser_mjkzz/focuser_mjkzz_simulator/focuser_mjkzz_simulator.c simulator_common/serial_simulator_common.h ../indigo_drivers/focuser_mjkzz/mjkzz_def.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_moonlite_simulator: ../indigo_drivers/focuser_moonlite/focuser_moonlite_simulator/focuser_moonlite_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_mypro2_simulator: ../indigo_drivers/focuser_mypro2/focuser_mypro2_simulator/focuser_mypro2_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_nfocus_simulator: ../indigo_drivers/focuser_nfocus/focuser_nfocus_simulator/focuser_nfocus_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_nstep_simulator: ../indigo_drivers/focuser_nstep/focuser_nstep_simulator/focuser_nstep_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_optec_simulator: ../indigo_drivers/focuser_optec/focuser_optec_simulator/focuser_optec_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_primaluce_simulator: ../indigo_drivers/focuser_primaluce/focuser_primaluce_simulator/focuser_primaluce_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_prodigy_simulator: ../indigo_drivers/focuser_prodigy/focuser_prodigy_simulator/focuser_prodigy_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_robofocus_simulator: ../indigo_drivers/focuser_robofocus/focuser_robofocus_simulator/focuser_robofocus_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_steeldrive2_simulator: ../indigo_drivers/focuser_steeldrive2/focuser_steeldrive2_simulator/focuser_steeldrive2_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_usbv3_simulator: ../indigo_drivers/focuser_usbv3/focuser_usbv3_simulator/focuser_usbv3_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_wemacro_simulator: ../indigo_drivers/focuser_wemacro/focuser_wemacro_simulator/focuser_wemacro_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

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

$(INTEGRATION_BUILD)/aux_ppb_simulator: ../indigo_drivers/aux_ppb/aux_ppb_simulator/aux_ppb_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_skyalert_simulator: ../indigo_drivers/aux_skyalert/aux_skyalert_simulator/aux_skyalert_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_sqm_simulator: ../indigo_drivers/aux_sqm/aux_sqm_simulator/aux_sqm_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_uch_simulator: ../indigo_drivers/aux_uch/aux_uch_simulator/aux_uch_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/aux_usbdp_simulator: ../indigo_drivers/aux_usbdp/aux_usbdp_simulator/aux_usbdp_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_focuser_askar_simulator: integration/test_focuser_askar_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_askar_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_ASKAR_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_baader_simulator: integration/test_dome_baader_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_baader_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_BAADER_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_beaver_simulator: integration/test_dome_beaver_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_beaver_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_BEAVER_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_nexdome_simulator: integration/test_dome_nexdome_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_nexdome_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_NEXDOME_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_nexdome3_simulator: integration/test_dome_nexdome3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_nexdome3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_NEXDOME3_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_skyroof_simulator: integration/test_dome_skyroof_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_skyroof_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_SKYROOF_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_talon6ror_simulator: integration/test_dome_talon6ror_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_talon6ror_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_TALON6ROR_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_gps_nmea_simulator: integration/test_gps_nmea_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/gps_nmea_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(GPS_NMEA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_mount_ioptron_simulator: integration/test_mount_ioptron_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_ioptron_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_IOPTRON_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_lx200_simulator: ../indigo_drivers/mount_lx200/mount_lx200_simulator/mount_lx200_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_lx200_simulator: integration/test_mount_lx200_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_lx200_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_LX200_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_nexstar_simulator: ../indigo_drivers/mount_nexstar/mount_nexstar_simulator/mount_nexstar_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_nexstar_simulator: integration/test_mount_nexstar_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_nexstar_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_NEXSTAR_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_nexstaraux_simulator: ../indigo_drivers/mount_nexstaraux/mount_nexstaraux_simulator/mount_nexstaraux_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_nexstaraux_simulator: integration/test_mount_nexstaraux_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_nexstaraux_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_NEXSTARAUX_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_pmc8_simulator: ../indigo_drivers/mount_pmc8/mount_pmc8_simulator/mount_pmc8_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_pmc8_simulator: integration/test_mount_pmc8_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_pmc8_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_PMC8_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_rainbow_simulator: ../indigo_drivers/mount_rainbow/mount_rainbow_simulator/mount_rainbow_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_rainbow_simulator: integration/test_mount_rainbow_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_rainbow_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_RAINBOW_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_starbook_simulator: ../indigo_drivers/mount_starbook/mount_starbook_simulator/mount_starbook_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_starbook_simulator: integration/test_mount_starbook_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_starbook_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_STARBOOK_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_temma_simulator: ../indigo_drivers/mount_temma/mount_temma_simulator/mount_temma_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_temma_simulator: integration/test_mount_temma_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_temma_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_TEMMA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_astromechanics_simulator: integration/test_focuser_astromechanics_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_astromechanics_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_ASTROMECHANICS_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_rotator_falcon2_simulator: integration/test_rotator_falcon2_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_falcon2_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(ROTATOR_FALCON2_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/rotator_optec_simulator: ../indigo_drivers/rotator_optec/rotator_optec_simulator/rotator_optec_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_rotator_optec_simulator: integration/test_rotator_optec_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_optec_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(ROTATOR_OPTEC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/rotator_wa_simulator: ../indigo_drivers/rotator_wa/rotator_wa_simulator/rotator_wa_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_rotator_wa_simulator: integration/test_rotator_wa_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_wa_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(ROTATOR_WA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_xagyl_simulator: integration/test_wheel_xagyl_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_xagyl_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_XAGYL_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/wheel_indigo_simulator: ../indigo_drivers/wheel_indigo/wheel_indigo_simulator/wheel_indigo_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_wheel_indigo_simulator: integration/test_wheel_indigo_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_indigo_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(WHEEL_INDIGO_SIMULATOR_TEST_LDFLAGS)

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

$(INTEGRATION_BUILD)/test_focuser_efa_simulator: integration/test_focuser_efa_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_efa_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_EFA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_dmfc_simulator: integration/test_focuser_dmfc_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_dmfc_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_DMFC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_dsd_simulator: integration/test_focuser_dsd_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_dsd_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_DSD_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_focusdreampro_simulator: integration/test_focuser_focusdreampro_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_focusdreampro_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_FOCUSDREAMPRO_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_ioptron_simulator: integration/test_focuser_ioptron_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_ioptron_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_IOPTRON_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_qhy_simulator: integration/test_focuser_qhy_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_qhy_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_QHY_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_optecfl_simulator: integration/test_focuser_optecfl_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_optecfl_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_OPTECFL_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_lacerta_simulator: integration/test_focuser_lacerta_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_lacerta_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_LACERTA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_lakeside_simulator: integration/test_focuser_lakeside_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_lakeside_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_LAKESIDE_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_mjkzz_simulator: integration/test_focuser_mjkzz_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_mjkzz_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_MJKZZ_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_moonlite_simulator: integration/test_focuser_moonlite_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_moonlite_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_MOONLITE_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_mypro2_simulator: integration/test_focuser_mypro2_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_mypro2_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_MYPRO2_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_nfocus_simulator: integration/test_focuser_nfocus_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_nfocus_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_NFOCUS_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_nstep_simulator: integration/test_focuser_nstep_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_nstep_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_NSTEP_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_optec_simulator: integration/test_focuser_optec_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_optec_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_OPTEC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_primaluce_simulator: integration/test_focuser_primaluce_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_primaluce_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_PRIMALUCE_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_prodigy_simulator: integration/test_focuser_prodigy_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_prodigy_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_PRODIGY_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_robofocus_simulator: integration/test_focuser_robofocus_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_robofocus_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_ROBOFOCUS_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_steeldrive2_simulator: integration/test_focuser_steeldrive2_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_steeldrive2_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_STEELDRIVE2_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_usbv3_simulator: integration/test_focuser_usbv3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_usbv3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_USBV3_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_wemacro_simulator: integration/test_focuser_wemacro_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_wemacro_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_WEMACRO_SIMULATOR_TEST_LDFLAGS)

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

$(INTEGRATION_BUILD)/test_aux_ppb_simulator: integration/test_aux_ppb_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_ppb_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_PPB_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_skyalert_simulator: integration/test_aux_skyalert_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_skyalert_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_SKYALERT_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_sqm_simulator: integration/test_aux_sqm_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_sqm_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_SQM_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_uch_simulator: integration/test_aux_uch_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_uch_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_UCH_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_usbdp_simulator: integration/test_aux_usbdp_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_usbdp_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_USBDP_SIMULATOR_TEST_LDFLAGS)

test-clean:
	rm -rf $(TEST_BUILD)
