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
INTEGRATION_TESTS = $(INTEGRATION_BUILD)/test_guider_asi_sdk \
	$(INTEGRATION_BUILD)/test_guider_cgusbst4_simulator \
	$(INTEGRATION_BUILD)/test_aux_cloudwatcher_simulator \
	$(INTEGRATION_BUILD)/test_aux_mgbox_simulator \
	$(INTEGRATION_BUILD)/test_aux_skyalert_transport \
	$(INTEGRATION_BUILD)/test_aux_sqm_transport \
	$(INTEGRATION_BUILD)/test_wheel_indigo_transport \
	$(INTEGRATION_BUILD)/test_ao_sx_transport \
	$(INTEGRATION_BUILD)/test_gps_nmea_transport  $(INTEGRATION_BUILD)/test_aux_upb_simulator  \
	$(INTEGRATION_BUILD)/test_wheel_atik_usb \
	$(INTEGRATION_BUILD)/test_wheel_sx_usb \
	$(INTEGRATION_BUILD)/test_guider_gpusb_usb \
	$(INTEGRATION_BUILD)/test_focuser_fcusb_usb \
	$(INTEGRATION_BUILD)/test_aux_dsusb_usb \
	$(INTEGRATION_BUILD)/test_focuser_primaluce_motion \
	$(INTEGRATION_BUILD)/test_focuser_usbv3_motion \
	$(INTEGRATION_BUILD)/test_focuser_dmfc_motion \
	$(INTEGRATION_BUILD)/test_focuser_astromechanics_motion \
	$(INTEGRATION_BUILD)/test_wheel_manual \
	$(INTEGRATION_BUILD)/test_guider_cgusbst4_transport \
	$(INTEGRATION_BUILD)/test_aux_geoptikflat_transport \
	$(INTEGRATION_BUILD)/test_aux_rts_transport \
	$(INTEGRATION_BUILD)/test_generator_architecture \
	$(INTEGRATION_BUILD)/test_wheel_playerone_sdk \
	$(INTEGRATION_BUILD)/test_ccd_playerone_sdk \
	$(INTEGRATION_BUILD)/test_ccd_sx_usb \
	$(INTEGRATION_BUILD)/test_wheel_astroasis_sdk \
	$(INTEGRATION_BUILD)/test_ccd_touptek_sdk \
	$(INTEGRATION_BUILD)/test_wheel_asi_sdk \
	$(INTEGRATION_BUILD)/test_focuser_asi_sdk \
	$(INTEGRATION_BUILD)/test_rotator_asi_sdk \
	$(INTEGRATION_BUILD)/test_bus_lifecycle \
	$(INTEGRATION_BUILD)/test_ccd_countdown \
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
MOUNT_SYNSCAN_SIMULATOR_TEST_LDFLAGS = $(INTEGRATION_BUILD)/mount_synscan_test_driver.o $(TEST_LDFLAGS) -lindigocat
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

$(INTEGRATION_BUILD)/test_ccd_countdown: integration/test_ccd_countdown.c test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_bus_lifecycle: integration/test_bus_lifecycle.c test_runner.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/ccd_simulator_test_driver.o: ../indigo_drivers/ccd_simulator/indigo_ccd_simulator.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_set_timer=sim_test_set_timer -Dindigo_reschedule_timer=sim_test_reschedule_timer -c $< -o $@

$(INTEGRATION_BUILD)/test_ccd_simulator: integration/test_ccd_simulator.c integration/simulator_test_common.h integration/ccd_test_noise.h test_runner.h $(INTEGRATION_BUILD)/ccd_simulator_test_driver.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(INTEGRATION_BUILD)/ccd_simulator_test_driver.o $(CCD_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_simulator: integration/test_dome_simulator.c integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h integration/serial_simulator_test_common.h $(BUILD_DRIVERS)/indigo_dome_simulator.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_gps_simulator: integration/test_gps_simulator.c integration/simulator_test_common.h test_runner.h integration/serial_simulator_test_common.h $(BUILD_DRIVERS)/indigo_gps_simulator.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(GPS_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_mount_simulator: integration/test_mount_simulator.c integration/simulator_test_common.h test_runner.h integration/serial_simulator_test_common.h $(BUILD_DRIVERS)/indigo_mount_simulator.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_polaralign_simulator: integration/test_polaralign_simulator.c integration/simulator_test_common.h test_runner.h integration/serial_simulator_test_common.h $(BUILD_DRIVERS)/indigo_polaralign_simulator.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(POLARALIGN_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_rotator_simulator: integration/test_rotator_simulator.c integration/simulator_test_common.h test_runner.h integration/serial_simulator_test_common.h $(BUILD_DRIVERS)/indigo_rotator_simulator.a | $(INTEGRATION_BUILD)
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

$(INTEGRATION_BUILD)/focuser_efa_simulator: ../indigo_drivers/focuser_efa/focuser_efa_simulator/focuser_efa_simulator.c simulator_common/serial_simulator_common.h simulator_common/serial_motion.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< -lm

$(INTEGRATION_BUILD)/focuser_dmfc_simulator: ../indigo_drivers/focuser_dmfc/focuser_dmfc_simulator/focuser_dmfc_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_dsd_simulator: ../indigo_drivers/focuser_dsd/focuser_dsd_simulator/focuser_dsd_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_focusdreampro_simulator: ../indigo_drivers/focuser_focusdreampro/focuser_focusdreampro_simulator/focuser_focusdreampro_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/focuser_ioptron_simulator: ../indigo_drivers/focuser_ioptron/focuser_ioptron_simulator/focuser_ioptron_simulator.c simulator_common/serial_simulator_common.h simulator_common/serial_motion.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< -lm

$(INTEGRATION_BUILD)/focuser_qhy_simulator: ../indigo_drivers/focuser_qhy/focuser_qhy_simulator/focuser_qhy_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_optecfl_simulator: ../indigo_drivers/focuser_optecfl/focuser_optecfl_simulator/focuser_optecfl_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -pthread -o $@ $<

$(INTEGRATION_BUILD)/focuser_lacerta_simulator: ../indigo_drivers/focuser_lacerta/focuser_lacerta_simulator/focuser_lacerta_simulator.c simulator_common/serial_simulator_common.h simulator_common/serial_motion.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< -lm

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

$(INTEGRATION_BUILD)/focuser_prodigy_simulator: ../indigo_drivers/focuser_prodigy/focuser_prodigy_simulator/focuser_prodigy_simulator.c simulator_common/serial_simulator_common.h simulator_common/serial_motion.h | $(INTEGRATION_BUILD)
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

$(INTEGRATION_BUILD)/test_dome_skyroof_simulator: integration/test_dome_skyroof_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_skyroof_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_SKYROOF_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_dome_talon6ror_simulator: integration/test_dome_talon6ror_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/dome_talon6ror_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(DOME_TALON6ROR_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_gps_nmea_simulator: integration/test_gps_nmea_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/gps_nmea_simulator $(BUILD_DRIVERS)/indigo_gps_nmea.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(GPS_NMEA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_mount_ioptron_simulator: integration/test_mount_ioptron_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_ioptron_simulator | $(INTEGRATION_BUILD)
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

$(INTEGRATION_BUILD)/test_mount_nexstaraux_simulator: integration/test_mount_nexstaraux_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_nexstaraux_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_NEXSTARAUX_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/mount_pmc8_simulator: ../indigo_drivers/mount_pmc8/mount_pmc8_simulator/mount_pmc8_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_mount_pmc8_simulator: integration/test_mount_pmc8_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_pmc8_simulator | $(INTEGRATION_BUILD)
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

$(INTEGRATION_BUILD)/wheel_indigo_simulator: ../indigo_drivers/wheel_indigo/wheel_indigo_simulator/wheel_indigo_simulator.c simulator_common/serial_simulator_common.h simulator_common/serial_motion.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_wheel_indigo_simulator: integration/test_wheel_indigo_simulator.c $(BUILD_DRIVERS)/indigo_wheel_indigo.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_indigo_simulator | $(INTEGRATION_BUILD)
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

$(INTEGRATION_BUILD)/test_focuser_efa_simulator: integration/test_focuser_efa_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_efa_simulator $(BUILD_DRIVERS)/indigo_focuser_efa.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_EFA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_efa_simulator_asan: integration/test_focuser_efa_simulator.c ../indigo_drivers/focuser_efa/indigo_focuser_efa.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_efa_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/focuser_efa/indigo_focuser_efa.c $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_dmfc_simulator: integration/test_focuser_dmfc_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_dmfc_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_DMFC_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_dsd_simulator: integration/test_focuser_dsd_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_dsd_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_DSD_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_focusdreampro_simulator: integration/test_focuser_focusdreampro_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_focusdreampro_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_FOCUSDREAMPRO_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_ioptron_simulator: integration/test_focuser_ioptron_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_ioptron_simulator $(BUILD_DRIVERS)/indigo_focuser_ioptron.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_IOPTRON_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_ioptron_simulator_asan: integration/test_focuser_ioptron_simulator.c ../indigo_drivers/focuser_ioptron/indigo_focuser_ioptron.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_ioptron_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/focuser_ioptron/indigo_focuser_ioptron.c $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_qhy_simulator: integration/test_focuser_qhy_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_qhy_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_QHY_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_optecfl_simulator: integration/test_focuser_optecfl_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_optecfl_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_OPTECFL_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_lacerta_simulator: integration/test_focuser_lacerta_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_lacerta_simulator $(BUILD_DRIVERS)/indigo_focuser_lacerta.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(FOCUSER_LACERTA_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_lacerta_simulator_asan: integration/test_focuser_lacerta_simulator.c ../indigo_drivers/focuser_lacerta/indigo_focuser_lacerta.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_lacerta_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/focuser_lacerta/indigo_focuser_lacerta.c $(TEST_LDFLAGS)

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

$(INTEGRATION_BUILD)/test_focuser_prodigy_simulator_asan: integration/test_focuser_prodigy_simulator.c ../indigo_drivers/focuser_prodigy/indigo_focuser_prodigy.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_prodigy_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/focuser_prodigy/indigo_focuser_prodigy.c $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_prodigy_simulator: integration/test_focuser_prodigy_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_prodigy_simulator $(BUILD_DRIVERS)/indigo_focuser_prodigy.a | $(INTEGRATION_BUILD)
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

$(INTEGRATION_BUILD)/test_ao_sx_simulator: integration/test_ao_sx_simulator.c $(BUILD_DRIVERS)/indigo_ao_sx.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/ao_sx_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AO_SX_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_mount_synscan_simulator: integration/test_mount_synscan_simulator.c $(INTEGRATION_BUILD)/mount_synscan_test_driver.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h $(INTEGRATION_BUILD)/mount_synscan_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(MOUNT_SYNSCAN_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_upb3_simulator: integration/test_aux_upb3_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_upb3_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_UPB3_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_ppb_simulator: integration/test_aux_ppb_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_ppb_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_PPB_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_skyalert_simulator: integration/test_aux_skyalert_simulator.c $(BUILD_DRIVERS)/indigo_aux_skyalert.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_skyalert_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_SKYALERT_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_sqm_simulator: integration/test_aux_sqm_simulator.c $(BUILD_DRIVERS)/indigo_aux_sqm.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_sqm_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_SQM_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_uch_simulator: integration/test_aux_uch_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_uch_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_UCH_SIMULATOR_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_aux_usbdp_simulator: integration/test_aux_usbdp_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_usbdp_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(AUX_USBDP_SIMULATOR_TEST_LDFLAGS)

test-clean:
	rm -rf $(TEST_BUILD)

EAF_SDK_TEST_INCLUDE = -I../indigo_drivers/focuser_asi/bin_externals/libEAFFocuser/include
EAF_SDK_TEST_REPLACEMENTS = -Dlibusb_hotplug_register_callback_sim=eaf_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=eaf_test_usb_deregister_poll -Dindigo_start_usb_event_handler=eaf_test_usb_start -Dindigo_try_global_lock=eaf_test_lock -Dindigo_global_unlock=eaf_test_unlock -Dindigo_attach_device=eaf_test_attach -Dindigo_detach_device=eaf_test_detach -Dlibusb_hotplug_register_callback=eaf_test_usb_register -Dlibusb_hotplug_deregister_callback=eaf_test_usb_deregister -Dlibusb_ref_device=eaf_test_usb_ref -Dlibusb_unref_device=eaf_test_usb_unref -Dlibusb_get_device_descriptor=eaf_test_usb_descriptor

$(INTEGRATION_BUILD)/focuser_asi_sdk_driver.o: ../indigo_drivers/focuser_asi/indigo_focuser_asi.c ../indigo_drivers/focuser_asi/indigo_focuser_asi.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(EAF_SDK_TEST_INCLUDE) $(EAF_SDK_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_focuser_asi_sdk: integration/test_focuser_asi_sdk.c integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/focuser_asi_sdk_driver.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(EAF_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/focuser_asi_sdk_driver.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

EFW_SDK_TEST_INCLUDE = -I../indigo_drivers/wheel_asi/bin_externals/libEFWFilter/include
EFW_SDK_TEST_REPLACEMENTS = -Dlibusb_hotplug_register_callback_sim=efw_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=efw_test_usb_deregister_poll -Dindigo_start_usb_event_handler=efw_test_usb_start -Dindigo_try_global_lock=efw_test_lock -Dindigo_global_unlock=efw_test_unlock -Dindigo_attach_device=efw_test_attach -Dindigo_detach_device=efw_test_detach -Dlibusb_hotplug_register_callback=efw_test_usb_register -Dlibusb_hotplug_deregister_callback=efw_test_usb_deregister -Dlibusb_ref_device=efw_test_usb_ref -Dlibusb_unref_device=efw_test_usb_unref -Dlibusb_get_device_descriptor=efw_test_usb_descriptor

$(INTEGRATION_BUILD)/wheel_asi_sdk_driver.o: ../indigo_drivers/wheel_asi/indigo_wheel_asi.c ../indigo_drivers/wheel_asi/indigo_wheel_asi.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(EFW_SDK_TEST_INCLUDE) $(EFW_SDK_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_wheel_asi_sdk: integration/test_wheel_asi_sdk.c integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_asi_sdk_driver.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(EFW_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/wheel_asi_sdk_driver.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

PW_SDK_TEST_INCLUDE = -I../indigo_drivers/wheel_playerone/bin_externals/libPlayerOnePW/include
PW_SDK_TEST_REPLACEMENTS = -Dindigo_execute_handler_in=pw_test_poll -Dlibusb_hotplug_register_callback_sim=pw_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=pw_test_usb_deregister_poll -Dindigo_start_usb_event_handler=pw_test_usb_start -Dindigo_try_global_lock=pw_test_lock -Dindigo_global_unlock=pw_test_unlock -Dindigo_attach_device=pw_test_attach -Dindigo_detach_device=pw_test_detach -Dlibusb_hotplug_register_callback=pw_test_usb_register -Dlibusb_hotplug_deregister_callback=pw_test_usb_deregister -Dlibusb_ref_device=pw_test_usb_ref -Dlibusb_unref_device=pw_test_usb_unref -Dlibusb_get_device_descriptor=pw_test_usb_descriptor

$(INTEGRATION_BUILD)/wheel_playerone_sdk_driver.o: ../indigo_drivers/wheel_playerone/indigo_wheel_playerone.c ../indigo_drivers/wheel_playerone/indigo_wheel_playerone.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(PW_SDK_TEST_INCLUDE) $(PW_SDK_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/wheel_playerone_sdk_framework.o: ../indigo_libs/indigo_driver.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_config_folder=pw_test_config_folder -c $< -o $@

$(INTEGRATION_BUILD)/wheel_playerone_sdk_base.o: ../indigo_libs/indigo_wheel_driver.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(INTEGRATION_BUILD)/test_wheel_playerone_sdk: integration/test_wheel_playerone_sdk.c integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_playerone_sdk_driver.o $(INTEGRATION_BUILD)/wheel_playerone_sdk_framework.o $(INTEGRATION_BUILD)/wheel_playerone_sdk_base.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(PW_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/wheel_playerone_sdk_driver.o $(INTEGRATION_BUILD)/wheel_playerone_sdk_framework.o $(INTEGRATION_BUILD)/wheel_playerone_sdk_base.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

OFW_SDK_TEST_INCLUDE = -I../indigo_drivers/wheel_astroasis/bin_externals/OasisFilterWheel/include
OFW_SDK_TEST_REPLACEMENTS = -Dindigo_execute_handler_in=ofw_test_poll -Dlibusb_hotplug_register_callback_sim=ofw_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=ofw_test_usb_deregister_poll -Dindigo_start_usb_event_handler=ofw_test_usb_start -Dindigo_try_global_lock=ofw_test_lock -Dindigo_global_unlock=ofw_test_unlock -Dindigo_attach_device=ofw_test_attach -Dindigo_detach_device=ofw_test_detach -Dlibusb_hotplug_register_callback=ofw_test_usb_register -Dlibusb_hotplug_deregister_callback=ofw_test_usb_deregister -Dlibusb_ref_device=ofw_test_usb_ref -Dlibusb_unref_device=ofw_test_usb_unref -Dlibusb_get_device_descriptor=ofw_test_usb_descriptor

$(INTEGRATION_BUILD)/wheel_astroasis_sdk_driver.o: ../indigo_drivers/wheel_astroasis/indigo_wheel_astroasis.c ../indigo_drivers/wheel_astroasis/indigo_wheel_astroasis.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(OFW_SDK_TEST_INCLUDE) $(OFW_SDK_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/wheel_astroasis_sdk_framework.o: ../indigo_libs/indigo_driver.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_config_folder=ofw_test_config_folder -c $< -o $@

$(INTEGRATION_BUILD)/wheel_astroasis_sdk_base.o: ../indigo_libs/indigo_wheel_driver.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(INTEGRATION_BUILD)/test_wheel_astroasis_sdk: integration/test_wheel_astroasis_sdk.c integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_astroasis_sdk_driver.o $(INTEGRATION_BUILD)/wheel_astroasis_sdk_framework.o $(INTEGRATION_BUILD)/wheel_astroasis_sdk_base.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(OFW_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/wheel_astroasis_sdk_driver.o $(INTEGRATION_BUILD)/wheel_astroasis_sdk_framework.o $(INTEGRATION_BUILD)/wheel_astroasis_sdk_base.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

CAA_SDK_TEST_INCLUDE = -I../indigo_drivers/rotator_asi/bin_externals/libCAA/include
CAA_SDK_TEST_REPLACEMENTS = -Dindigo_rotator_change_property=caa_test_base_change -Dlibusb_hotplug_register_callback_sim=caa_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=caa_test_usb_deregister_poll -Dindigo_save_property=caa_test_save -Dindigo_start_usb_event_handler=caa_test_usb_start -Dindigo_try_global_lock=caa_test_lock -Dindigo_global_unlock=caa_test_unlock -Dindigo_attach_device=caa_test_attach -Dindigo_detach_device=caa_test_detach -Dlibusb_hotplug_register_callback=caa_test_usb_register -Dlibusb_hotplug_deregister_callback=caa_test_usb_deregister -Dlibusb_ref_device=caa_test_usb_ref -Dlibusb_unref_device=caa_test_usb_unref -Dlibusb_get_device_descriptor=caa_test_usb_descriptor

$(INTEGRATION_BUILD)/rotator_asi_sdk_driver.o: ../indigo_drivers/rotator_asi/indigo_rotator_asi.c ../indigo_drivers/rotator_asi/indigo_rotator_asi.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(CAA_SDK_TEST_INCLUDE) $(CAA_SDK_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_rotator_asi_sdk: integration/test_rotator_asi_sdk.c integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_asi_sdk_driver.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(CAA_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/rotator_asi_sdk_driver.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

# ToupTek SDK regression: real driver with SDK/USB lifecycle replacements, no hardware.
TOUPTEK_SDK_TEST_INCLUDE = -I../indigo_drivers/ccd_touptek/bin_externals/libtoupcam/include
TOUPTEK_SDK_TEST_REPLACEMENTS = -Dindigo_execute_handler_with_data=touptek_test_execute_handler_with_data -Dindigo_execute_handler_in=touptek_test_execute_handler_in -Dindigo_queue_create=touptek_test_queue_create -Dlibusb_hotplug_register_callback_sim=touptek_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=touptek_test_usb_deregister_poll -Dindigo_start_usb_event_handler=touptek_test_usb_start -Dindigo_try_global_lock=touptek_test_lock -Dindigo_global_unlock=touptek_test_unlock -Dindigo_attach_device=touptek_test_attach -Dindigo_detach_device=touptek_test_detach -Dlibusb_hotplug_register_callback=touptek_test_usb_register -Dlibusb_hotplug_deregister_callback=touptek_test_usb_deregister -Dlibusb_get_device_list=touptek_test_usb_list -Dlibusb_free_device_list=touptek_test_usb_free

$(INTEGRATION_BUILD)/ccd_touptek_sdk_driver.o: ../indigo_drivers/ccd_touptek/indigo_ccd_touptek.c ../indigo_drivers/ccd_touptek/indigo_ccd_touptek.h ../indigo_drivers/ccd_touptek/indigo_ccd_touptek_vendors.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(TOUPTEK_SDK_TEST_INCLUDE) $(TOUPTEK_SDK_TEST_REPLACEMENTS) -c $< -o $@


# Compile the framework property dispatcher separately so only its config-path lookup is replaced.
# All CONFIG serialization/parsing and base driver behavior remain real; no user HOME override.
TOUPTEK_SDK_TEST_LDFLAGS = $(filter-out -lindigo,$(TEST_LDFLAGS)) $(BUILD_LIB)/libindigo.a $(BUILD_LIB)/libjpeg.a $(BUILD_LIB)/libtiff.a $(BUILD_LIB)/libtiffxx.a $(BUILD_LIB)/libraw.a -lhidapi -lusb-1.0 -lz -lstdc++ -ldl
ifneq ($(OS_DETECTED),Darwin)
TOUPTEK_SDK_TEST_LDFLAGS += -ldns_sd -lavahi-client -lavahi-common
endif

$(INTEGRATION_BUILD)/ccd_touptek_sdk_framework.o: ../indigo_libs/indigo_driver.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_config_folder=touptek_test_config_folder -c $< -o $@

$(INTEGRATION_BUILD)/test_ccd_touptek_sdk: integration/test_ccd_touptek_sdk.c integration/ccd_test_noise.h test_runner.h $(INTEGRATION_BUILD)/ccd_touptek_sdk_driver.o $(INTEGRATION_BUILD)/ccd_touptek_sdk_framework.o
	$(CC) $(TEST_CFLAGS) $(TOUPTEK_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/ccd_touptek_sdk_driver.o $(INTEGRATION_BUILD)/ccd_touptek_sdk_framework.o -o $@ $(TOUPTEK_SDK_TEST_LDFLAGS)

# Physical hardware is opt-in, deliberately excluded from test/test-integration.
HW_DRIVER ?= touptek
HW_HOTPLUG ?= 0
.PHONY: test-ccd-touptek-hw
test-ccd-touptek-hw: check-lib $(TEST_BUILD)/hardware/test_ccd_touptek_hw
	INDIGO_TEST_DRIVER=$(HW_DRIVER) $(TEST_BUILD)/hardware/test_ccd_touptek_hw --run $(if $(filter 1,$(HW_HOTPLUG)),--hotplug)

$(TEST_BUILD)/hardware:
	mkdir -p $@

$(TEST_BUILD)/hardware/ccd_touptek_hw_driver.o: ../indigo_drivers/ccd_touptek/indigo_ccd_touptek.c ../indigo_drivers/ccd_touptek/indigo_ccd_touptek.h ../indigo_drivers/ccd_touptek/indigo_ccd_touptek_vendors.h Makefile | $(TEST_BUILD)/hardware
	$(CC) $(TEST_CFLAGS) $(TOUPTEK_SDK_TEST_INCLUDE) -c $< -o $@

$(TEST_BUILD)/hardware/test_ccd_touptek_hw: hardware/test_ccd_touptek_hw.c test_runner.h $(TEST_BUILD)/hardware/ccd_touptek_hw_driver.o $(TEST_BUILD)/hardware/ccd_altair_hw_driver.o
	$(CC) $(TEST_CFLAGS) $< $(TEST_BUILD)/hardware/ccd_touptek_hw_driver.o $(TEST_BUILD)/hardware/ccd_altair_hw_driver.o -o $@ $(TEST_LDFLAGS) -ltoupcam -laltaircam -lusb-1.0

$(TEST_BUILD)/hardware/ccd_altair_hw_driver.o: ../indigo_drivers/ccd_altair/indigo_ccd_altair.c ../indigo_drivers/ccd_altair/indigo_ccd_altair.h ../indigo_drivers/ccd_touptek/indigo_ccd_touptek.h ../indigo_drivers/ccd_touptek/indigo_ccd_touptek.c ../indigo_drivers/ccd_touptek/indigo_ccd_touptek_vendors.h Makefile | $(TEST_BUILD)/hardware
	$(CC) $(TEST_CFLAGS) -I../indigo_drivers/ccd_altair/bin_externals/libaltaircam/include -c $< -o $@

.PHONY: test-generator-architecture
test-generator-architecture: $(INTEGRATION_BUILD)/test_generator_architecture
	$(INTEGRATION_BUILD)/test_generator_architecture

$(INTEGRATION_BUILD)/test_generator_architecture: integration/test_generator_architecture.c test_runner.h ../build/bin/indigo_generator Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_GENERATOR='"$(abspath ../build/bin/indigo_generator)"' -DTEST_INCLUDE='"$(abspath ../indigo_libs)"' -DTEST_CC='"$(CC)"' $< -o $@

POA_SDK_TEST_INCLUDE = -I../indigo_drivers/ccd_playerone/bin_externals/libplayeronecamera/include
POA_SDK_TEST_REPLACEMENTS = -Dindigo_queue_create=poa_test_queue_create -Dindigo_execute_handler_in=poa_test_execute_in -Dlibusb_hotplug_register_callback_sim=poa_test_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=poa_test_usb_deregister_poll -Dindigo_start_usb_event_handler=poa_test_usb_start -Dindigo_try_global_lock=poa_test_lock -Dindigo_global_unlock=poa_test_unlock -Dindigo_attach_device=poa_test_attach -Dindigo_detach_device=poa_test_detach -Dlibusb_hotplug_register_callback=poa_test_usb_register -Dlibusb_hotplug_deregister_callback=poa_test_usb_deregister -Dlibusb_ref_device=poa_test_usb_ref -Dlibusb_unref_device=poa_test_usb_unref -Dlibusb_get_device_descriptor=poa_test_usb_descriptor

$(INTEGRATION_BUILD)/ccd_playerone_sdk_driver.o: ../indigo_drivers/ccd_playerone/indigo_ccd_playerone.c ../indigo_drivers/ccd_playerone/indigo_ccd_playerone.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(if $(findstring clang,$(CC)),-Wunreachable-code -Werror=unreachable-code) $(POA_SDK_TEST_INCLUDE) $(POA_SDK_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/ccd_playerone_sdk_framework.o: ../indigo_libs/indigo_driver.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_config_folder=poa_test_config_folder -c $< -o $@

$(INTEGRATION_BUILD)/test_ccd_playerone_sdk: integration/test_ccd_playerone_sdk.c integration/ccd_test_noise.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/ccd_playerone_sdk_driver.o $(INTEGRATION_BUILD)/ccd_playerone_sdk_framework.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(POA_SDK_TEST_INCLUDE) $< $(INTEGRATION_BUILD)/ccd_playerone_sdk_driver.o $(INTEGRATION_BUILD)/ccd_playerone_sdk_framework.o -o $@ $(TOUPTEK_SDK_TEST_LDFLAGS)


# Physical Player One hardware is opt-in and excluded from normal integration tests.
.PHONY: test-ccd-playerone-hw
test-ccd-playerone-hw: check-lib $(TEST_BUILD)/hardware/test_ccd_playerone_hw
	$(TEST_BUILD)/hardware/test_ccd_playerone_hw --run $(if $(filter 1,$(HW_HOTPLUG)),--hotplug)

$(TEST_BUILD)/hardware/test_ccd_playerone_hw: hardware/test_ccd_playerone_hw.c ../build/drivers/indigo_ccd_playerone.a $(INTEGRATION_BUILD)/ccd_playerone_sdk_framework.o | $(TEST_BUILD)/hardware
	$(CC) $(TEST_CFLAGS) $< ../build/drivers/indigo_ccd_playerone.a $(INTEGRATION_BUILD)/ccd_playerone_sdk_framework.o -o $@ $(TOUPTEK_SDK_TEST_LDFLAGS) -lPlayerOneCamera

# Dynamic-loader acceptance uses the shared bus and never saves user configuration.
.PHONY: test-ccd-playerone-reload-hw
test-ccd-playerone-reload-hw: check-lib $(TEST_BUILD)/hardware/test_ccd_playerone_reload_hw
	$(TEST_BUILD)/hardware/test_ccd_playerone_reload_hw --run

$(TEST_BUILD)/hardware/test_ccd_playerone_reload_hw: hardware/test_ccd_playerone_hw.c ../build/drivers/indigo_ccd_playerone.$(SOEXT) | $(TEST_BUILD)/hardware
	$(CC) $(TEST_CFLAGS) -DPOA_DYNAMIC_DRIVER -DPOA_DRIVER_PATH='"$(BUILD_DRIVERS)/indigo_ccd_playerone.$(SOEXT)"' $< -o $@ $(TEST_LDFLAGS) -ldl

# Native SX command/transfer emulation; production driver is compiled separately.
SX_USB_TEST_REPLACEMENTS = -Dlibusb_open=sx_test_open -Dlibusb_close=sx_test_close -Dlibusb_kernel_driver_active=sx_test_kernel -Dlibusb_claim_interface=sx_test_claim -Dlibusb_get_config_descriptor=sx_test_config -Dlibusb_free_config_descriptor=sx_test_free_config -Dlibusb_bulk_transfer=sx_test_bulk -Dlibusb_ref_device=sx_test_ref -Dlibusb_unref_device=sx_test_unref -Dlibusb_get_device_descriptor=sx_test_descriptor -Dindigo_get_usb_path=sx_test_path -Dindigo_start_usb_event_handler=sx_test_usb_start -Dlibusb_hotplug_register_callback=sx_test_register -Dlibusb_hotplug_register_callback_sim=sx_test_register_sim -Dlibusb_hotplug_deregister_callback=sx_test_deregister -Dlibusb_hotplug_deregister_callback_poll=sx_test_deregister_poll -Dindigo_try_global_lock=sx_test_lock -Dindigo_global_unlock=sx_test_unlock -Dindigo_execute_handler_in=sx_test_execute_in -Dindigo_attach_device=sx_test_attach -Dindigo_detach_device=sx_test_detach -Dindigo_queue_create=sx_test_queue_create -Dindigo_execute_priority_handler_in=sx_test_execute_priority

$(INTEGRATION_BUILD)/ccd_sx_usb_driver.o: ../indigo_drivers/ccd_sx/indigo_ccd_sx.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(SX_USB_TEST_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_ccd_sx_usb: integration/test_ccd_sx_usb.c integration/ccd_test_noise.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/ccd_sx_usb_driver.o | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(INTEGRATION_BUILD)/ccd_sx_usb_driver.o -o $@ $(TOUPTEK_SDK_TEST_LDFLAGS)

OUTPUT_TRANSPORT_REPLACEMENTS = -Dindigo_uni_open_serial=output_open -Dindigo_uni_close=output_close -Dindigo_uni_discard=output_discard -Dindigo_uni_vprintf=output_write -Dindigo_uni_vtprintf=output_write_line -Dindigo_uni_read_section=output_read -Dindigo_uni_set_rts=output_rts

$(INTEGRATION_BUILD)/guider_cgusbst4_transport.o: ../indigo_drivers/guider_cgusbst4/indigo_guider_cgusbst4.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(OUTPUT_TRANSPORT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_guider_cgusbst4_transport: integration/test_serial_outputs.c $(INTEGRATION_BUILD)/guider_cgusbst4_transport.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_KIND=1 -DTEST_ENTRY=indigo_guider_cgusbst4 -DTEST_NAME='"CG-USB-ST4 Adapter"' $< $(INTEGRATION_BUILD)/guider_cgusbst4_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_geoptikflat_transport.o: ../indigo_drivers/aux_geoptikflat/indigo_aux_geoptikflat.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(OUTPUT_TRANSPORT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_aux_geoptikflat_transport: integration/test_serial_outputs.c $(INTEGRATION_BUILD)/aux_geoptikflat_transport.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_KIND=2 -DTEST_ENTRY=indigo_aux_geoptikflat -DTEST_NAME='"Geoptik Flat Generator"' $< $(INTEGRATION_BUILD)/aux_geoptikflat_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_rts_transport.o: ../indigo_drivers/aux_rts/indigo_aux_rts.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(OUTPUT_TRANSPORT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_aux_rts_transport: integration/test_serial_outputs.c $(INTEGRATION_BUILD)/aux_rts_transport.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_KIND=3 -DTEST_ENTRY=indigo_aux_rts -DTEST_NAME='"RTS-on-COM shutter"' $< $(INTEGRATION_BUILD)/aux_rts_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_wheel_manual: integration/test_wheel_manual.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(BUILD_DRIVERS)/indigo_wheel_manual.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(BUILD_DRIVERS)/indigo_wheel_manual.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_astromechanics_motion: integration/test_focuser_motion.c $(INTEGRATION_BUILD)/focuser_astromechanics_simulator $(BUILD_DRIVERS)/indigo_focuser_astromechanics.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_ENTRY=indigo_focuser_astromechanics -DTEST_NAME='"ASTROMECHANICS Focuser"' -DTEST_SIMULATOR='"build/integration/focuser_astromechanics_simulator"' $< -o $@ $(BUILD_DRIVERS)/indigo_focuser_astromechanics.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_dmfc_motion: integration/test_focuser_motion.c $(INTEGRATION_BUILD)/focuser_dmfc_simulator $(BUILD_DRIVERS)/indigo_focuser_dmfc.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_ENTRY=indigo_focuser_dmfc -DTEST_NAME='"Pegasus DMFC"' -DTEST_SIMULATOR='"build/integration/focuser_dmfc_simulator"' $< -o $@ $(BUILD_DRIVERS)/indigo_focuser_dmfc.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_usbv3_motion: integration/test_focuser_motion.c $(INTEGRATION_BUILD)/focuser_usbv3_simulator $(BUILD_DRIVERS)/indigo_focuser_usbv3.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_ENTRY=indigo_focuser_usbv3 -DTEST_NAME='"USB_Focus v3"' -DTEST_SIMULATOR='"build/integration/focuser_usbv3_simulator"' $< -o $@ $(BUILD_DRIVERS)/indigo_focuser_usbv3.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_focuser_primaluce_motion: integration/test_focuser_motion.c $(INTEGRATION_BUILD)/focuser_primaluce_simulator $(BUILD_DRIVERS)/indigo_focuser_primaluce.a integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DTEST_ENTRY=indigo_focuser_primaluce -DTEST_NAME='"PrimaluceLab Focuser"' -DTEST_SIMULATOR='"build/integration/focuser_primaluce_simulator"' $< -o $@ $(BUILD_DRIVERS)/indigo_focuser_primaluce.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/focuser_astromechanics_simulator: simulator_common/serial_motion.h

$(INTEGRATION_BUILD)/focuser_dmfc_simulator: simulator_common/serial_motion.h

$(INTEGRATION_BUILD)/focuser_usbv3_simulator: simulator_common/serial_motion.h

$(INTEGRATION_BUILD)/focuser_primaluce_simulator: simulator_common/serial_motion.h

$(INTEGRATION_BUILD)/mount_synscan_test_driver.o: ../indigo_drivers/mount_synscan/indigo_mount_synscan.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_config_folder=synscan_test_config_folder -c $< -o $@

USB_OUTPUT_INCLUDES = -I../indigo_drivers/aux_dsusb/bin_externals/libdsusb/include -I../indigo_drivers/focuser_fcusb/bin_externals/libfcusb/include -I../indigo_drivers/guider_gpusb/bin_externals/libgpusb/include -I../indigo_drivers/wheel_atik/bin_externals/libatik/include
USB_OUTPUT_REPLACEMENTS = -Dindigo_queue_create=output_queue_create -Dindigo_attach_device=output_attach -Dindigo_detach_device=output_detach -Dindigo_start_usb_event_handler=output_usb_start -Dlibusb_hotplug_register_callback=output_usb_register -Dlibusb_hotplug_deregister_callback=output_usb_deregister -Dlibusb_hotplug_register_callback_sim=output_usb_register_sim -Dlibusb_hotplug_deregister_callback_poll=output_usb_deregister_poll -Dlibusb_ref_device=output_usb_ref -Dlibusb_unref_device=output_usb_unref -Dindigo_get_usb_path=output_usb_path -Dindigo_uni_open_hid=output_hid_open -Dindigo_uni_close=output_hid_close -Dindigo_uni_read=output_hid_read -Dindigo_uni_write=output_hid_write

$(INTEGRATION_BUILD)/aux_dsusb_usb_output.o: ../indigo_drivers/aux_dsusb/indigo_aux_dsusb.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) $(USB_OUTPUT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_aux_dsusb_usb: integration/test_usb_outputs.c $(INTEGRATION_BUILD)/aux_dsusb_usb_output.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) -DTEST_KIND=1 -DTEST_ENTRY=indigo_aux_dsusb -DTEST_NAME='"DSUSB test"' $< $(INTEGRATION_BUILD)/aux_dsusb_usb_output.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/focuser_fcusb_usb_output.o: ../indigo_drivers/focuser_fcusb/indigo_focuser_fcusb.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) $(USB_OUTPUT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_focuser_fcusb_usb: integration/test_usb_outputs.c $(INTEGRATION_BUILD)/focuser_fcusb_usb_output.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) -DTEST_KIND=2 -DTEST_ENTRY=indigo_focuser_fcusb -DTEST_NAME='"FCUSB test"' $< $(INTEGRATION_BUILD)/focuser_fcusb_usb_output.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/guider_gpusb_usb_output.o: ../indigo_drivers/guider_gpusb/indigo_guider_gpusb.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) $(USB_OUTPUT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_guider_gpusb_usb: integration/test_usb_outputs.c $(INTEGRATION_BUILD)/guider_gpusb_usb_output.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) -DTEST_KIND=3 -DTEST_ENTRY=indigo_guider_gpusb -DTEST_NAME='"GPUSB test"' $< $(INTEGRATION_BUILD)/guider_gpusb_usb_output.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/wheel_sx_usb_output.o: ../indigo_drivers/wheel_sx/indigo_wheel_sx.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) $(USB_OUTPUT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_wheel_sx_usb: integration/test_usb_outputs.c $(INTEGRATION_BUILD)/wheel_sx_usb_output.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) -DTEST_KIND=4 -DTEST_ENTRY=indigo_wheel_sx -DTEST_NAME='"SX Filter Wheel #1"' $< $(INTEGRATION_BUILD)/wheel_sx_usb_output.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/wheel_atik_usb_output.o: ../indigo_drivers/wheel_atik/indigo_wheel_atik.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) $(USB_OUTPUT_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_wheel_atik_usb: integration/test_usb_outputs.c $(INTEGRATION_BUILD)/wheel_atik_usb_output.o integration/serial_simulator_test_common.h integration/simulator_test_common.h integration/abort_queue_test_common.h test_runner.h Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $(USB_OUTPUT_INCLUDES) -DTEST_KIND=5 -DTEST_ENTRY=indigo_wheel_atik -DTEST_NAME='"Atik Filter Wheel #1"' $< $(INTEGRATION_BUILD)/wheel_atik_usb_output.o -o $@ $(TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/aux_upb_simulator: ../indigo_drivers/aux_upb/aux_upb_simulator/aux_upb_simulator.c simulator_common/serial_simulator_common.h simulator_common/serial_motion.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_aux_upb_simulator: integration/test_aux_upb_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_upb_simulator $(BUILD_DRIVERS)/indigo_aux_upb.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(BUILD_DRIVERS)/indigo_aux_upb.a $(TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/gps_nmea_transport.o: ../indigo_drivers/gps_nmea/indigo_gps_nmea.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_open_serial_with_config=nmea_test_open -Dindigo_uni_read_section=nmea_test_read -Dindigo_uni_close=nmea_test_close -c $< -o $@

$(INTEGRATION_BUILD)/test_gps_nmea_transport: integration/test_gps_nmea_transport.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/gps_nmea_transport.o Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(INTEGRATION_BUILD)/gps_nmea_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/ao_sx_transport.o: ../indigo_drivers/ao_sx/indigo_ao_sx.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_open_serial=ao_test_open -Dindigo_uni_close=ao_test_close -Dindigo_uni_discard=ao_test_discard -Dindigo_uni_vprintf=ao_test_write -Dindigo_uni_read_section=ao_test_read -c $< -o $@

$(INTEGRATION_BUILD)/test_ao_sx_transport: integration/test_ao_sx_transport.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/ao_sx_transport.o Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(INTEGRATION_BUILD)/ao_sx_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/wheel_indigo_transport.o: ../indigo_drivers/wheel_indigo/indigo_wheel_indigo.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_open_serial_with_speed=wheel_test_open -Dindigo_uni_close=wheel_test_close -Dindigo_uni_discard=wheel_test_discard -Dindigo_uni_vtprintf=wheel_test_write -Dindigo_uni_read_section=wheel_test_read -c $< -o $@

$(INTEGRATION_BUILD)/test_wheel_indigo_transport: integration/test_wheel_indigo_transport.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/wheel_indigo_transport.o Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(INTEGRATION_BUILD)/wheel_indigo_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_sqm_transport.o: ../indigo_drivers/aux_sqm/indigo_aux_sqm.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_open_serial_with_speed=sqm_test_open -Dindigo_uni_close=sqm_test_close -Dindigo_uni_discard=sqm_test_discard -Dindigo_uni_write=sqm_test_write -Dindigo_uni_read_section=sqm_test_read -c $< -o $@

$(INTEGRATION_BUILD)/test_aux_sqm_transport: integration/test_aux_sqm_transport.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_sqm_transport.o Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(INTEGRATION_BUILD)/aux_sqm_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_skyalert_transport.o: ../indigo_drivers/aux_skyalert/indigo_aux_skyalert.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -Dindigo_uni_open_serial_with_speed=skyalert_test_open -Dindigo_uni_close=skyalert_test_close -Dindigo_uni_discard=skyalert_test_discard -Dindigo_uni_printf=skyalert_test_printf -Dindigo_uni_read_section=skyalert_test_read -c $< -o $@

$(INTEGRATION_BUILD)/test_aux_skyalert_transport: integration/test_aux_skyalert_transport.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_skyalert_transport.o Makefile | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(INTEGRATION_BUILD)/aux_skyalert_transport.o -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_cloudwatcher_simulator: ../indigo_drivers/aux_cloudwatcher/aux_cloudwatcher_simulator/aux_cloudwatcher_simulator.c simulator_common/aux_simulator_common.h simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_aux_cloudwatcher_simulator: integration/test_aux_cloudwatcher_simulator.c integration/aux_test_isolation.h integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_cloudwatcher_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(BUILD_DRIVERS)/indigo_aux_cloudwatcher.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_mgbox_simulator: ../indigo_drivers/aux_mgbox/aux_mgbox_simulator/aux_mgbox_simulator.c simulator_common/aux_simulator_common.h simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_aux_mgbox_simulator: integration/test_aux_mgbox_simulator.c integration/aux_test_isolation.h integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_mgbox_simulator $(BUILD_DRIVERS)/indigo_aux_mgbox.a | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(BUILD_DRIVERS)/indigo_aux_mgbox.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/aux_dragonfly_simulator: ../indigo_drivers/aux_dragonfly/aux_dragonfly_simulator/aux_dragonfly_simulator.c simulator_common/aux_simulator_common.h simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_aux_dragonfly_simulator: integration/test_aux_dragonfly_simulator.c integration/aux_test_isolation.h integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/aux_dragonfly_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(BUILD_DRIVERS)/indigo_aux_dragonfly.a $(TEST_LDFLAGS)

# Opt-in: the Dragonfly protocol uses a loopback UDP socket.
.PHONY: test-aux-dragonfly-simulator
test-aux-dragonfly-simulator: $(INTEGRATION_BUILD)/test_aux_dragonfly_simulator
	./$(INTEGRATION_BUILD)/test_aux_dragonfly_simulator

# Instrument the unchanged driver itself, not only the test executable.
$(INTEGRATION_BUILD)/test_aux_cloudwatcher_simulator_asan: integration/test_aux_cloudwatcher_simulator.c ../indigo_drivers/aux_cloudwatcher/indigo_aux_cloudwatcher.c integration/aux_test_isolation.h $(INTEGRATION_BUILD)/aux_cloudwatcher_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/aux_cloudwatcher/indigo_aux_cloudwatcher.c $(TEST_LDFLAGS)

# Instrument the unchanged driver itself, not only the test executable.
$(INTEGRATION_BUILD)/test_aux_mgbox_simulator_asan: integration/test_aux_mgbox_simulator.c ../indigo_drivers/aux_mgbox/indigo_aux_mgbox.c integration/aux_test_isolation.h $(INTEGRATION_BUILD)/aux_mgbox_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/aux_mgbox/indigo_aux_mgbox.c $(TEST_LDFLAGS)

# Instrument the unchanged driver itself, not only the test executable.
$(INTEGRATION_BUILD)/test_aux_dragonfly_simulator_asan: integration/test_aux_dragonfly_simulator.c ../indigo_drivers/aux_dragonfly/indigo_aux_dragonfly.c integration/aux_test_isolation.h $(INTEGRATION_BUILD)/aux_dragonfly_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $< ../indigo_drivers/aux_dragonfly/indigo_aux_dragonfly.c $(TEST_LDFLAGS)

# The production ASI USB-ST4 implementation excludes macOS arm64. Test its
# supported x86_64 branch unchanged (Rosetta is required on Apple Silicon).
ASI_ST4_TEST_CFLAGS = $(TEST_CFLAGS)
ASI_ST4_TEST_LDFLAGS = $(TEST_LDFLAGS)
ifeq ($(OS_DETECTED),Darwin)
ASI_ST4_TEST_CFLAGS = $(filter-out -arch arm64 x86_64,$(TEST_CFLAGS)) -arch x86_64
ASI_ST4_TEST_LDFLAGS = $(filter-out -arch arm64 x86_64,$(TEST_LDFLAGS)) -arch x86_64
endif
ASI_ST4_INCLUDE = -I../indigo_drivers/guider_asi/bin_externals/libusb2st4conv/include
ASI_ST4_REPLACEMENTS = -Dindigo_start_usb_event_handler=asi_test_usb_start -Dlibusb_hotplug_register_callback=asi_test_usb_register -Dlibusb_hotplug_deregister_callback=asi_test_usb_deregister -Dlibusb_get_device_descriptor=asi_test_usb_descriptor -Dindigo_attach_device=asi_test_attach -Dindigo_detach_device=asi_test_detach

$(INTEGRATION_BUILD)/guider_asi_sdk_driver.o: ../indigo_drivers/guider_asi/indigo_guider_asi.c Makefile | $(INTEGRATION_BUILD)
	$(CC) $(ASI_ST4_TEST_CFLAGS) $(ASI_ST4_INCLUDE) $(ASI_ST4_REPLACEMENTS) -c $< -o $@

$(INTEGRATION_BUILD)/test_guider_asi_sdk: integration/test_guider_asi_sdk.c integration/guider_asi_fake_sdk.c integration/guider_asi_fake_sdk.h integration/simulator_test_common.h integration/serial_simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/guider_asi_sdk_driver.o | $(INTEGRATION_BUILD)
	$(CC) $(ASI_ST4_TEST_CFLAGS) $(ASI_ST4_INCLUDE) $< integration/guider_asi_fake_sdk.c $(INTEGRATION_BUILD)/guider_asi_sdk_driver.o -o $@ $(ASI_ST4_TEST_LDFLAGS) -lusb-1.0

$(INTEGRATION_BUILD)/guider_cgusbst4_simulator: ../indigo_drivers/guider_cgusbst4/guider_cgusbst4_simulator/guider_cgusbst4_simulator.c simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $<

$(INTEGRATION_BUILD)/test_guider_cgusbst4_simulator: integration/test_guider_cgusbst4_simulator.c integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/guider_cgusbst4_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< $(BUILD_DRIVERS)/indigo_guider_cgusbst4.a -o $@ $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/rotator_lunatico_simulator: ../indigo_drivers/rotator_lunatico/rotator_lunatico_simulator/rotator_lunatico_simulator.c simulator_common/aux_simulator_common.h simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< -lm

$(INTEGRATION_BUILD)/rotator_lunatico_udp_simulator: ../indigo_drivers/rotator_lunatico/rotator_lunatico_simulator/rotator_lunatico_simulator.c simulator_common/aux_simulator_common.h simulator_common/serial_simulator_common.h | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DSIM_UDP=1 -o $@ $< -lm

ROTATOR_LUNATICO_TEST_LDFLAGS = $(BUILD_DRIVERS)/indigo_rotator_lunatico.a $(TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_rotator_lunatico_simulator: integration/test_rotator_lunatico_simulator.c integration/aux_test_isolation.h integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_lunatico_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -o $@ $< $(ROTATOR_LUNATICO_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_rotator_lunatico_udp_simulator: integration/test_rotator_lunatico_simulator.c integration/aux_test_isolation.h integration/serial_simulator_test_common.h integration/simulator_test_common.h test_runner.h $(INTEGRATION_BUILD)/rotator_lunatico_udp_simulator | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) -DLUNATICO_SIMULATOR='"build/integration/rotator_lunatico_udp_simulator"' -o $@ $< $(ROTATOR_LUNATICO_TEST_LDFLAGS)

.PHONY: test-rotator-lunatico-udp
test-rotator-lunatico-udp: $(INTEGRATION_BUILD)/test_rotator_lunatico_udp_simulator
	./$(INTEGRATION_BUILD)/test_rotator_lunatico_udp_simulator
