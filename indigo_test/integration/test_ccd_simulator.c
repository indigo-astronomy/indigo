// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/ccd_simulator/indigo_ccd_simulator.h>

#include "simulator_test_common.h"
#include "ccd_test_noise.h"
#include <stdatomic.h>
#include <unistd.h>

static const char *ccd_imager_connected_properties[] = {
	CCD_INFO_PROPERTY_NAME,
	CCD_LENS_PROPERTY_NAME,
	CCD_LOCAL_MODE_PROPERTY_NAME,
	CCD_IMAGE_FILE_PROPERTY_NAME,
	CCD_MODE_PROPERTY_NAME,
	CCD_EXPOSURE_PROPERTY_NAME,
	CCD_STREAMING_PROPERTY_NAME,
	CCD_STREAMING_SETTINGS_PROPERTY_NAME,
	CCD_FPS_PROPERTY_NAME,
	CCD_ABORT_EXPOSURE_PROPERTY_NAME,
	CCD_FRAME_PROPERTY_NAME,
	CCD_BIN_PROPERTY_NAME,
	CCD_OFFSET_PROPERTY_NAME,
	CCD_GAIN_PROPERTY_NAME,
	CCD_EGAIN_PROPERTY_NAME,
	CCD_GAMMA_PROPERTY_NAME,
	CCD_FRAME_TYPE_PROPERTY_NAME,
	CCD_IMAGE_FORMAT_PROPERTY_NAME,
	CCD_UPLOAD_MODE_PROPERTY_NAME,
	CCD_PREVIEW_PROPERTY_NAME,
	CCD_IMAGE_PROPERTY_NAME,
	CCD_COOLER_PROPERTY_NAME,
	CCD_COOLER_POWER_PROPERTY_NAME,
	CCD_TEMPERATURE_PROPERTY_NAME,
	CCD_FITS_HEADERS_PROPERTY_NAME,
	CCD_SET_FITS_HEADER_PROPERTY_NAME,
	CCD_REMOVE_FITS_HEADERS_PROPERTY_NAME,
	CCD_JPEG_SETTINGS_PROPERTY_NAME,
	CCD_JPEG_STRETCH_PRESETS_PROPERTY_NAME
};

static const char *ccd_hidden_connected_properties[] = {
	CCD_READ_MODE_PROPERTY_NAME,
	CCD_PREVIEW_IMAGE_PROPERTY_NAME,
	CCD_PREVIEW_HISTOGRAM_PROPERTY_NAME,
	CCD_RBI_FLUSH_ENABLE_PROPERTY_NAME,
	CCD_RBI_FLUSH_PROPERTY_NAME
};

static const simulator_driver_case ccd_imager_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_IMAGER_CAMERA_NAME,
	indigo_ccd_simulator,
	true,
	base_properties_with_instances,
	ARRAY_SIZE(base_properties_with_instances),
	hidden_base_properties,
	ARRAY_SIZE(hidden_base_properties),
	ccd_imager_connected_properties,
	ARRAY_SIZE(ccd_imager_connected_properties),
	ccd_hidden_connected_properties,
	ARRAY_SIZE(ccd_hidden_connected_properties)
};

static const simulator_driver_case ccd_wheel_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_WHEEL_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_focuser_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_FOCUSER_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_guider_camera_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_GUIDER_CAMERA_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_guider_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_GUIDER_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_ao_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_AO_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_bahtinov_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_BAHTINOV_CAMERA_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_dslr_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_DSLR_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static const simulator_driver_case ccd_file_simulator = {
	"Camera Simulator",
	"indigo_ccd_simulator",
	CCD_SIMULATOR_FILE_NAME,
	indigo_ccd_simulator,
	true,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0,
	NULL,
	0
};

static double bounded_number_value(const char *property_name, const char *item_name, double preferred_value) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		return NAN;
	}
	if (preferred_value < item->number.min) {
		return item->number.min;
	}
	if (preferred_value > item->number.max) {
		return item->number.max;
	}
	return preferred_value;
}

static double number_item_max(const char *property_name, const char *item_name) {
	indigo_item *item = find_cached_item(property_name, item_name);
	if (item == NULL) {
		return NAN;
	}
	return item->number.max;
}

static void assert_pulse_resets(const simulator_driver_case *driver_case, const char *property_name, const char *item_name, bool require_busy) {
	double pulse = bounded_number_value(property_name, item_name, 200);
	ASSERT_FALSE(isnan(pulse));
	ASSERT_TRUE(pulse > 0);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, driver_case->device_name, property_name, item_name, pulse));
	if (require_busy) {
		ASSERT_TRUE(wait_for_property_state(property_name, INDIGO_BUSY_STATE));
	}
	ASSERT_TRUE(wait_for_property_state(property_name, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(property_name, item_name, 0, 0.001));
}

static void exercise_short_exposure(const simulator_driver_case *driver_case) {
	double exposure = bounded_number_value(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0.1);
	ASSERT_FALSE(isnan(exposure));
	ASSERT_TRUE(exposure > 0);
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, driver_case->device_name, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, exposure));
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(CCD_EXPOSURE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, 0, 0.001));
}

static void driver_info_reports_simulator_metadata(void) {
	assert_simulator_driver_info(&ccd_imager_simulator);
}

static void simulator_initializes_enumerates_connects_disconnects_and_shuts_down(void) {
	assert_simulator_properties(&ccd_imager_simulator);
}

static void ccd_wheel_passes_compliance_checks(void) {
	static const char *slot_name_items[] = {
		WHEEL_SLOT_NAME_1_ITEM_NAME,
		WHEEL_SLOT_NAME_2_ITEM_NAME,
		WHEEL_SLOT_NAME_3_ITEM_NAME,
		WHEEL_SLOT_NAME_4_ITEM_NAME,
		WHEEL_SLOT_NAME_5_ITEM_NAME
	};
	static const char *slot_offset_items[] = {
		WHEEL_SLOT_OFFSET_1_ITEM_NAME,
		WHEEL_SLOT_OFFSET_2_ITEM_NAME,
		WHEEL_SLOT_OFFSET_3_ITEM_NAME,
		WHEEL_SLOT_OFFSET_4_ITEM_NAME,
		WHEEL_SLOT_OFFSET_5_ITEM_NAME
	};
	start_connected_simulator(&ccd_wheel_simulator);

	assert_device_interface(INDIGO_INTERFACE_WHEEL);
	assert_property_has_item(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);
	assert_property_has_items(WHEEL_SLOT_NAME_PROPERTY_NAME, slot_name_items, ARRAY_SIZE(slot_name_items));
	assert_property_has_items(WHEEL_SLOT_OFFSET_PROPERTY_NAME, slot_offset_items, ARRAY_SIZE(slot_offset_items));
	assert_number_item_in_range(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);

	double slot_count = number_item_max(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME);
	ASSERT_FALSE(isnan(slot_count));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_wheel_simulator.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3));
	ASSERT_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 3, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_wheel_simulator.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, slot_count + 1));
	ASSERT_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, slot_count, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_wheel_simulator.device_name, WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 0));
	ASSERT_TRUE(wait_for_property_state(WHEEL_SLOT_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(WHEEL_SLOT_PROPERTY_NAME, WHEEL_SLOT_ITEM_NAME, 1, 0.001));

	stop_connected_simulator(&ccd_wheel_simulator);
}

static void ccd_focuser_passes_compliance_checks(void) {
	start_connected_simulator(&ccd_focuser_simulator);

	assert_device_interface(INDIGO_INTERFACE_FOCUSER);
	assert_property_has_item(FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME);
	assert_property_has_item(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME);
	assert_property_has_item(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME);
	assert_number_item_in_range(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);

	double original_position = cached_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME);
	double target_position = bounded_number_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, 200);
	double fast_speed = bounded_number_value(FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 1000);
	ASSERT_FALSE(isnan(original_position));
	ASSERT_FALSE(isnan(target_position));
	ASSERT_FALSE(isnan(fast_speed));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, fast_speed));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, target_position, 0.001));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_DIRECTION_PROPERTY_NAME, FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_DIRECTION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_STEPS_PROPERTY_NAME, FOCUSER_STEPS_ITEM_NAME, 25));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_STEPS_PROPERTY_NAME, INDIGO_OK_STATE));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, 1));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, number_item_max(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME)));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_BUSY_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_ABORT_MOTION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_ALERT_STATE));

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_SPEED_PROPERTY_NAME, FOCUSER_SPEED_ITEM_NAME, fast_speed));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_SPEED_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_ON_POSITION_SET_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_focuser_simulator.device_name, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, original_position));
	ASSERT_TRUE(wait_for_property_state(FOCUSER_POSITION_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, original_position, 0.001));

	stop_connected_simulator(&ccd_focuser_simulator);
}

static void assert_ccd_camera_compliance(const simulator_driver_case *driver_case, bool has_bin_property, const char * const *extra_properties, int extra_property_count) {
	static const char *ccd_info_items[] = {
		CCD_INFO_WIDTH_ITEM_NAME,
		CCD_INFO_HEIGHT_ITEM_NAME,
		CCD_INFO_MAX_HORIZONTAL_BIN_ITEM_NAME,
		CCD_INFO_MAX_VERTICAL_BIN_ITEM_NAME,
		CCD_INFO_PIXEL_SIZE_ITEM_NAME,
		CCD_INFO_PIXEL_WIDTH_ITEM_NAME,
		CCD_INFO_PIXEL_HEIGHT_ITEM_NAME,
		CCD_INFO_BITS_PER_PIXEL_ITEM_NAME
	};
	static const char *frame_type_items[] = {
		CCD_FRAME_TYPE_LIGHT_ITEM_NAME,
		CCD_FRAME_TYPE_BIAS_ITEM_NAME,
		CCD_FRAME_TYPE_DARK_ITEM_NAME,
		CCD_FRAME_TYPE_FLAT_ITEM_NAME
	};
	static const char *upload_mode_items[] = {
		CCD_UPLOAD_MODE_CLIENT_ITEM_NAME,
		CCD_UPLOAD_MODE_LOCAL_ITEM_NAME,
		CCD_UPLOAD_MODE_BOTH_ITEM_NAME
	};
	static const char *preview_items[] = {
		CCD_PREVIEW_ENABLED_ITEM_NAME,
		CCD_PREVIEW_DISABLED_ITEM_NAME
	};
	start_connected_simulator(driver_case);

	assert_device_interface(INDIGO_INTERFACE_CCD);
	assert_property_has_items(CCD_INFO_PROPERTY_NAME, ccd_info_items, ARRAY_SIZE(ccd_info_items));
	assert_property_has_item(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME);
	assert_property_has_item(CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME);
	assert_property_has_item(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_WIDTH_ITEM_NAME);
	assert_property_has_item(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_HEIGHT_ITEM_NAME);
	if (has_bin_property) {
		assert_property_has_item(CCD_BIN_PROPERTY_NAME, CCD_BIN_HORIZONTAL_ITEM_NAME);
		assert_property_has_item(CCD_BIN_PROPERTY_NAME, CCD_BIN_VERTICAL_ITEM_NAME);
	}
	assert_property_has_items(CCD_FRAME_TYPE_PROPERTY_NAME, frame_type_items, ARRAY_SIZE(frame_type_items));
	assert_property_has_item(CCD_IMAGE_FORMAT_PROPERTY_NAME, CCD_IMAGE_FORMAT_RAW_ITEM_NAME);
	assert_property_has_items(CCD_UPLOAD_MODE_PROPERTY_NAME, upload_mode_items, ARRAY_SIZE(upload_mode_items));
	assert_property_has_items(CCD_PREVIEW_PROPERTY_NAME, preview_items, ARRAY_SIZE(preview_items));
	assert_defined_property(CCD_IMAGE_PROPERTY_NAME);
	assert_defined_properties(extra_properties, extra_property_count);
	assert_number_item_in_range(CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME);
	assert_number_item_in_range(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_WIDTH_ITEM_NAME);
	assert_number_item_in_range(CCD_FRAME_PROPERTY_NAME, CCD_FRAME_HEIGHT_ITEM_NAME);
	exercise_short_exposure(driver_case);

	stop_connected_simulator(driver_case);
}

static void ccd_imager_passes_compliance_checks(void) {
	assert_ccd_camera_compliance(&ccd_imager_simulator, true, NULL, 0);
}

static void ccd_guider_camera_passes_compliance_checks(void) {
	static const char *extra_properties[] = {
		"GUIDER_MODE",
		"SIMULATION_SETUP"
	};
	assert_ccd_camera_compliance(&ccd_guider_camera_simulator, true, extra_properties, ARRAY_SIZE(extra_properties));
}

static void ccd_bahtinov_camera_passes_compliance_checks(void) {
	static const char *extra_properties[] = {
		"BAHTINOV_SETTINGS"
	};
	assert_ccd_camera_compliance(&ccd_bahtinov_simulator, false, extra_properties, ARRAY_SIZE(extra_properties));
}

static void ccd_dslr_passes_compliance_checks(void) {
	static const char *extra_properties[] = {
		DSLR_PROGRAM_PROPERTY_NAME,
		DSLR_SHUTTER_PROPERTY_NAME,
		DSLR_APERTURE_PROPERTY_NAME,
		DSLR_ISO_PROPERTY_NAME
	};
	assert_ccd_camera_compliance(&ccd_dslr_simulator, false, extra_properties, ARRAY_SIZE(extra_properties));
}

static void ccd_file_camera_passes_compliance_checks(void) {
	reset_simulator_context(&ccd_file_simulator);

	ASSERT_EQ_INT(INDIGO_OK, indigo_start());
	ASSERT_EQ_INT(INDIGO_OK, indigo_attach_client(&simulator_test_client));
	ASSERT_EQ_INT(INDIGO_OK, ccd_file_simulator.entry(INDIGO_DRIVER_INIT, NULL));
	enumerate_simulator_device();

	assert_device_interface(INDIGO_INTERFACE_CCD);
	assert_defined_property(CONNECTION_PROPERTY_NAME);
	assert_property_has_item("FILE_NAME", "PATH");

	ASSERT_EQ_INT(INDIGO_OK, ccd_file_simulator.entry(INDIGO_DRIVER_SHUTDOWN, NULL));
	ASSERT_EQ_INT(INDIGO_OK, indigo_detach_client(&simulator_test_client));
	ASSERT_EQ_INT(INDIGO_OK, indigo_stop());
	release_cached_properties();
}

static void ccd_guider_passes_compliance_checks(void) {
	static const char *guide_ra_items[] = {
		GUIDER_GUIDE_EAST_ITEM_NAME,
		GUIDER_GUIDE_WEST_ITEM_NAME
	};
	static const char *guide_dec_items[] = {
		GUIDER_GUIDE_NORTH_ITEM_NAME,
		GUIDER_GUIDE_SOUTH_ITEM_NAME
	};
	start_connected_simulator(&ccd_guider_simulator);

	assert_device_interface(INDIGO_INTERFACE_GUIDER);
	assert_property_has_items(GUIDER_GUIDE_RA_PROPERTY_NAME, guide_ra_items, ARRAY_SIZE(guide_ra_items));
	assert_property_has_items(GUIDER_GUIDE_DEC_PROPERTY_NAME, guide_dec_items, ARRAY_SIZE(guide_dec_items));
	assert_property_has_item(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);

	assert_pulse_resets(&ccd_guider_simulator, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_EAST_ITEM_NAME, true);
	assert_pulse_resets(&ccd_guider_simulator, GUIDER_GUIDE_RA_PROPERTY_NAME, GUIDER_GUIDE_WEST_ITEM_NAME, true);
	assert_pulse_resets(&ccd_guider_simulator, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_NORTH_ITEM_NAME, true);
	assert_pulse_resets(&ccd_guider_simulator, GUIDER_GUIDE_DEC_PROPERTY_NAME, GUIDER_GUIDE_SOUTH_ITEM_NAME, true);

	double original_rate = cached_number_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME);
	double test_rate = bounded_number_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, number_item_max(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME) / 2);
	ASSERT_FALSE(isnan(original_rate));
	ASSERT_FALSE(isnan(test_rate));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_guider_simulator.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, test_rate));
	ASSERT_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));
	ASSERT_TRUE(wait_for_number_item_value(GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, test_rate, 0.001));
	ASSERT_EQ_INT(INDIGO_OK, indigo_change_number_property_1(&simulator_test_client, ccd_guider_simulator.device_name, GUIDER_RATE_PROPERTY_NAME, GUIDER_RATE_ITEM_NAME, original_rate));
	ASSERT_TRUE(wait_for_property_state(GUIDER_RATE_PROPERTY_NAME, INDIGO_OK_STATE));

	stop_connected_simulator(&ccd_guider_simulator);
}

static void ccd_ao_passes_compliance_checks(void) {
	static const char *ao_ra_items[] = {
		AO_GUIDE_EAST_ITEM_NAME,
		AO_GUIDE_WEST_ITEM_NAME
	};
	static const char *ao_dec_items[] = {
		AO_GUIDE_NORTH_ITEM_NAME,
		AO_GUIDE_SOUTH_ITEM_NAME
	};
	start_connected_simulator(&ccd_ao_simulator);

	assert_device_interface(INDIGO_INTERFACE_AO);
	assert_property_has_items(AO_GUIDE_RA_PROPERTY_NAME, ao_ra_items, ARRAY_SIZE(ao_ra_items));
	assert_property_has_items(AO_GUIDE_DEC_PROPERTY_NAME, ao_dec_items, ARRAY_SIZE(ao_dec_items));
	assert_property_has_item(AO_RESET_PROPERTY_NAME, AO_CENTER_ITEM_NAME);

	assert_pulse_resets(&ccd_ao_simulator, AO_GUIDE_RA_PROPERTY_NAME, AO_GUIDE_EAST_ITEM_NAME, false);
	assert_pulse_resets(&ccd_ao_simulator, AO_GUIDE_RA_PROPERTY_NAME, AO_GUIDE_WEST_ITEM_NAME, false);
	assert_pulse_resets(&ccd_ao_simulator, AO_GUIDE_DEC_PROPERTY_NAME, AO_GUIDE_NORTH_ITEM_NAME, false);
	assert_pulse_resets(&ccd_ao_simulator, AO_GUIDE_DEC_PROPERTY_NAME, AO_GUIDE_SOUTH_ITEM_NAME, false);

	ASSERT_EQ_INT(INDIGO_OK, indigo_change_switch_property_1(&simulator_test_client, ccd_ao_simulator.device_name, AO_RESET_PROPERTY_NAME, AO_CENTER_ITEM_NAME, true));
	ASSERT_TRUE(wait_for_property_state(AO_RESET_PROPERTY_NAME, INDIGO_OK_STATE));

	stop_connected_simulator(&ccd_ao_simulator);
}

static bool sim_fast_temperature;
static indigo_timer **sim_temperature_timer;

bool sim_test_set_timer(indigo_device *device, double delay, indigo_timer_callback callback, indigo_timer **timer) {
	if (sim_fast_temperature && !strcmp(device->name, CCD_SIMULATOR_IMAGER_CAMERA_NAME) && delay == 5) {
		sim_temperature_timer = timer;
		delay = 0.05;
	}
	return indigo_set_timer(device, delay, callback, timer);
}

bool sim_test_reschedule_timer(indigo_device *device, double delay, indigo_timer **timer) {
	return indigo_reschedule_timer(device, sim_fast_temperature && timer == sim_temperature_timer ? 0.05 : delay, timer);
}

// Extra scenarios observe the production simulator, never its built-in image arrays.
static atomic_int sim_frames, sim_bad_frames;
static atomic_int sim_countdown_ticks;
static const char *sim_properties[] = { "BAHTINOV_SETTINGS", "CCD_ABORT_EXPOSURE", "CCD_BIN", "CCD_COOLER", "CCD_EXPOSURE", "CCD_FRAME", "CCD_IMAGE_FORMAT", "CCD_STREAMING", "CCD_TEMPERATURE", "CCD_UPLOAD_MODE", "CONNECTION", "DSLR_APERTURE", "DSLR_ISO", "DSLR_PROGRAM", "DSLR_SHUTTER", "GUIDER_MODE", "SIMULATION_SETUP" };
static atomic_int sim_revisions[ARRAY_SIZE(sim_properties)];

static int sim_property_index(const char *name) {
	for (int i = 0; i < ARRAY_SIZE(sim_properties); i++) {
		if (!strcmp(name, sim_properties[i])) { return i; }
	}
	return -1;
}
static atomic_uint sim_width, sim_height, sim_signature;
static atomic_bool sim_check_noise;
static char sim_raw_path[] = "/tmp/indigo_ccd_noise_XXXXXX";

static indigo_result sim_update(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (!strcmp(property->device, context.driver_case->device_name)) {
		if (!strcmp(property->name, "CCD_EXPOSURE") && property->state == INDIGO_BUSY_STATE && property->items[0].number.value > 0 && property->items[0].number.value < property->items[0].number.target) {
			atomic_fetch_add(&sim_countdown_ticks, 1);
		}
		if (!strcmp(property->name, "CCD_IMAGE") && property->state == INDIGO_OK_STATE && property->count && property->items[0].blob.size) {
			indigo_item *item = property->items;
			indigo_raw_header header = { 0 };
			bool valid = item->blob.value && !strcmp(item->blob.format, ".raw") && item->blob.size >= sizeof(header);
			if (valid) {
				memcpy(&header, item->blob.value, sizeof(header));
				int bytes = header.signature == INDIGO_RAW_MONO8 ? 1 : header.signature == INDIGO_RAW_MONO16 ? 2 : header.signature == INDIGO_RAW_RGB24 ? 3 : header.signature == INDIGO_RAW_RGB48 ? 6 : 0;
				size_t length = (size_t)header.width * header.height * bytes;
				valid = bytes && header.width && header.height && item->blob.size >= sizeof(header) + length;
				if (valid && atomic_load(&sim_check_noise)) {
					const unsigned char *pixels = (unsigned char *)item->blob.value + sizeof(header);
					for (size_t i = 0; i < length; i++) {
						if (pixels[i] != (ccd_test_noise(i, 0) >> 8)) { valid = false; break; }
					}
				}
			}
			atomic_store(&sim_width, header.width);
			atomic_store(&sim_height, header.height);
			atomic_store(&sim_signature, header.signature);
			if (!valid) { atomic_fetch_add(&sim_bad_frames, 1); }
			atomic_fetch_add(&sim_frames, 1);
		}
	}
	indigo_result result = simulator_client_update_property(client, device, property, message);
	int index = sim_property_index(property->name);
	if (index >= 0 && !strcmp(property->device, context.driver_case->device_name)) { atomic_fetch_add(&sim_revisions[index], 1); }
	return result;
}

#define SIM_CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); indigo_test_failures++; goto cleanup; } } while (0)

static bool sim_wait_revision(const char *property, indigo_property_state state, int before) {
	for (int i = 0; i < 800; i++) {
		indigo_property *p = find_cached_property(property);
		if (atomic_load(&sim_revisions[sim_property_index(property)]) > before && p && p->state == state) { return true; }
		indigo_usleep(10000);
	}
	fprintf(stderr, "Simulator timeout: %s state %d\n", property, state);
	return false;
}

static bool sim_number(const char *property, int count, const char **items, const double *values, indigo_property_state state) {
	int before = atomic_load(&sim_revisions[sim_property_index(property)]);
	return indigo_change_number_property(&simulator_test_client, context.driver_case->device_name, property, count, items, values) == INDIGO_OK && sim_wait_revision(property, state, before);
}

static bool sim_switch(const char *property, const char *item, indigo_property_state state) {
	int before = atomic_load(&sim_revisions[sim_property_index(property)]);
	return indigo_change_switch_property_1(&simulator_test_client, context.driver_case->device_name, property, item, true) == INDIGO_OK && sim_wait_revision(property, state, before);
}

static bool sim_expose(void) {
	int before = atomic_load(&sim_frames);
	return sim_number("CCD_EXPOSURE", 1, (const char *[]){ "EXPOSURE" }, (double []){ 0.03 }, INDIGO_OK_STATE) && atomic_load(&sim_frames) == before + 1 && !atomic_load(&sim_bad_frames);
}

static void sim_begin(const simulator_driver_case *driver) {
	atomic_store(&sim_frames, 0);
	atomic_store(&sim_bad_frames, 0);
	atomic_store(&sim_check_noise, false);
	simulator_test_client.update_property = sim_update;
	start_connected_simulator(driver);
	sim_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE);
	sim_switch("CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE);
}

static void sim_end(const simulator_driver_case *driver) {
	if (context.connected && ((find_cached_property("CCD_EXPOSURE") && find_cached_property("CCD_EXPOSURE")->state == INDIGO_BUSY_STATE) || (find_cached_property("CCD_STREAMING") && find_cached_property("CCD_STREAMING")->state == INDIGO_BUSY_STATE))) { sim_switch("CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE); }
	stop_connected_simulator(driver);
	sim_fast_temperature = false;
	simulator_test_client.update_property = simulator_client_update_property;
}

static void simulator_raw_geometry_and_bins(void) {
	sim_begin(&ccd_imager_simulator);
	SIM_CHECK(sim_expose());
	SIM_CHECK(atomic_load(&sim_width) == IMAGER_WIDTH && atomic_load(&sim_height) == IMAGER_HEIGHT);
	for (int bin = 1; bin <= 4; bin *= 2) {
		SIM_CHECK(sim_number("CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, (double []){ bin, bin }, INDIGO_OK_STATE));
		SIM_CHECK(sim_number("CCD_FRAME", 4, (const char *[]){ "LEFT", "TOP", "WIDTH", "HEIGHT" }, (double []){ 16, 24, 128, 96 }, INDIGO_OK_STATE));
		SIM_CHECK(sim_expose());
		SIM_CHECK(atomic_load(&sim_width) == 128 / bin && atomic_load(&sim_height) == 96 / bin);
	}
	SIM_CHECK(sim_number("CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, (double []){ 3, 3 }, INDIGO_ALERT_STATE));
	SIM_CHECK(cached_number_value("CCD_BIN", "HORIZONTAL") == 4);
	SIM_CHECK(sim_number("CCD_BIN", 2, (const char *[]){ "HORIZONTAL", "VERTICAL" }, (double []){ 1, 2 }, INDIGO_ALERT_STATE));
	SIM_CHECK(sim_expose());
cleanup:
	sim_end(&ccd_imager_simulator);
}

static void simulator_stream_abort_and_reconnect(void) {
	sim_begin(&ccd_imager_simulator);
	SIM_CHECK(sim_number("CCD_FRAME", 2, (const char *[]){ "WIDTH", "HEIGHT" }, (double []){ 64, 64 }, INDIGO_OK_STATE));
	SIM_CHECK(sim_number("CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.02, 3 }, INDIGO_OK_STATE));
	SIM_CHECK(atomic_load(&sim_frames) == 3 && !atomic_load(&sim_bad_frames));
	SIM_CHECK(sim_number("CCD_EXPOSURE", 1, (const char *[]){ "EXPOSURE" }, (double []){ 2 }, INDIGO_BUSY_STATE));
	SIM_CHECK(indigo_change_number_property_1(&simulator_test_client, context.driver_case->device_name, "CCD_EXPOSURE", "EXPOSURE", 0.01) == INDIGO_OK);
	SIM_CHECK(cached_number_value("CCD_EXPOSURE", "EXPOSURE") > 0.01);
	SIM_CHECK(sim_switch("CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	SIM_CHECK(sim_expose());
	SIM_CHECK(sim_number("CCD_STREAMING", 2, (const char *[]){ "EXPOSURE", "COUNT" }, (double []){ 0.02, -1 }, INDIGO_BUSY_STATE));
	int before = atomic_load(&sim_frames);
	for (int i = 0; i < 200 && atomic_load(&sim_frames) == before; i++) { indigo_usleep(10000); }
	SIM_CHECK(atomic_load(&sim_frames) > before);
	SIM_CHECK(sim_switch("CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	SIM_CHECK(sim_expose());
	SIM_CHECK(sim_number("CCD_EXPOSURE", 1, (const char *[]){ "EXPOSURE" }, (double []){ 1 }, INDIGO_BUSY_STATE));
	SIM_CHECK(sim_switch("CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	SIM_CHECK(sim_switch("CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	SIM_CHECK(sim_expose());
cleanup:
	sim_end(&ccd_imager_simulator);
}

static void simulator_countdown_progress_abort_and_restart(void) {
	sim_begin(&ccd_imager_simulator);
	atomic_store(&sim_countdown_ticks, 0);
	SIM_CHECK(sim_number("CCD_EXPOSURE", 1, (const char *[]){ "EXPOSURE" }, (double []){ 3 }, INDIGO_BUSY_STATE));
	for (int i = 0; i < 250 && atomic_load(&sim_countdown_ticks) == 0; i++) {
		indigo_usleep(10000);
	}
	SIM_CHECK(atomic_load(&sim_countdown_ticks) > 0);
	SIM_CHECK(wait_for_property_state("CCD_EXPOSURE", INDIGO_OK_STATE));
	SIM_CHECK(cached_number_value("CCD_EXPOSURE", "EXPOSURE") == 0);
	atomic_store(&sim_countdown_ticks, 0);
	SIM_CHECK(sim_number("CCD_EXPOSURE", 1, (const char *[]){ "EXPOSURE" }, (double []){ 3 }, INDIGO_BUSY_STATE));
	SIM_CHECK(sim_switch("CCD_ABORT_EXPOSURE", "ABORT_EXPOSURE", INDIGO_OK_STATE));
	SIM_CHECK(sim_expose());
	SIM_CHECK(sim_switch("CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	SIM_CHECK(sim_switch("CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	SIM_CHECK(sim_expose());
cleanup:
	sim_end(&ccd_imager_simulator);
}

static void simulator_cooling_target_and_polling(void) {
	sim_fast_temperature = true;
	sim_begin(&ccd_imager_simulator);
	SIM_CHECK(sim_switch("CCD_COOLER", "ON", INDIGO_OK_STATE));
	SIM_CHECK(sim_number("CCD_TEMPERATURE", 1, (const char *[]){ "TEMPERATURE" }, (double []){ 24 }, INDIGO_BUSY_STATE));
	SIM_CHECK(wait_for_number_item_value("CCD_TEMPERATURE", "TEMPERATURE", 24, 0.01));
	SIM_CHECK(wait_for_property_state("CCD_TEMPERATURE", INDIGO_OK_STATE));
	SIM_CHECK(cached_number_value("CCD_COOLER_POWER", "POWER") == 20);
	SIM_CHECK(sim_number("CCD_TEMPERATURE", 1, (const char *[]){ "TEMPERATURE" }, (double []){ 25 }, INDIGO_BUSY_STATE));
	SIM_CHECK(wait_for_number_item_value("CCD_TEMPERATURE", "TEMPERATURE", 25, 0.01));
	SIM_CHECK(sim_switch("CCD_COOLER", "OFF", INDIGO_OK_STATE));
	SIM_CHECK(sim_switch("CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	SIM_CHECK(sim_switch("CONNECTION", "CONNECTED", INDIGO_OK_STATE));
	SIM_CHECK(sim_expose());
cleanup:
	sim_end(&ccd_imager_simulator);
}

static void simulator_camera_modes_and_settings(void) {
	const simulator_driver_case *drivers[] = { &ccd_guider_camera_simulator, &ccd_bahtinov_simulator, &ccd_dslr_simulator };
	for (int i = 0; i < 3; i++) {
		sim_begin(drivers[i]);
		if (i == 0) {
			SIM_CHECK(sim_number("SIMULATION_SETUP", 1, (const char *[]){ "J2000" }, (double []){ 1950 }, INDIGO_OK_STATE));
			SIM_CHECK(cached_number_value("SIMULATION_SETUP", "J2000") == 2000);
			const char *modes[] = { "STARS", "FLIPPED_STARS", "SUN", "ECLIPSE" };
			for (int mode = 0; mode < 4; mode++) {
				SIM_CHECK(sim_switch("GUIDER_MODE", modes[mode], INDIGO_OK_STATE));
				SIM_CHECK(sim_expose());
			}
		} else if (i == 1) {
			SIM_CHECK(sim_number("BAHTINOV_SETTINGS", 1, (const char *[]){ "ROTATION" }, (double []){ 90 }, INDIGO_OK_STATE));
			SIM_CHECK(sim_expose());
			SIM_CHECK(atomic_load(&sim_signature) == INDIGO_RAW_MONO8);
		} else {
			SIM_CHECK(sim_switch("DSLR_PROGRAM", "M", INDIGO_OK_STATE));
			SIM_CHECK(sim_switch("DSLR_SHUTTER", "0.1", INDIGO_OK_STATE));
			SIM_CHECK(sim_switch("DSLR_APERTURE", "28", INDIGO_OK_STATE));
			SIM_CHECK(sim_switch("DSLR_ISO", "400", INDIGO_OK_STATE));
			SIM_CHECK(sim_expose());
			SIM_CHECK(atomic_load(&sim_signature) == INDIGO_RAW_RGB24);
		}
		sim_end(drivers[i]);
	}
	return;
cleanup:
	sim_end(context.driver_case);
}

static void simulator_file_noise_formats_and_failure(void) {
	const unsigned signatures[] = { INDIGO_RAW_MONO8, INDIGO_RAW_MONO16, INDIGO_RAW_RGB24, INDIGO_RAW_RGB48 };
	const int bytes[] = { 1, 2, 3, 6 };
	reset_simulator_context(&ccd_file_simulator);
	simulator_test_client.update_property = sim_update;
	indigo_start();
	indigo_attach_client(&simulator_test_client);
	indigo_ccd_simulator(INDIGO_DRIVER_INIT, NULL);
	enumerate_simulator_device();
	int fd = mkstemp(sim_raw_path);
	SIM_CHECK(fd >= 0);
	close(fd);
	SIM_CHECK(indigo_change_text_property_1(&simulator_test_client, ccd_file_simulator.device_name, "FILE_NAME", "PATH", "/nonexistent/indigo-noise.raw") == INDIGO_OK);
	SIM_CHECK(sim_switch("CONNECTION", "CONNECTED", INDIGO_ALERT_STATE));
	SIM_CHECK(indigo_change_text_property_1(&simulator_test_client, ccd_file_simulator.device_name, "FILE_NAME", "PATH", sim_raw_path) == INDIGO_OK);
	for (int format = 0; format < 4; format++) {
		FILE *file = fopen(sim_raw_path, "wb");
		SIM_CHECK(file != NULL);
		indigo_raw_header header = { signatures[format], 64, 48 };
		bool written = fwrite(&header, sizeof(header), 1, file) == 1;
		for (int i = 0; i < 64 * 48 * bytes[format]; i++) { written = (fputc(ccd_test_noise(i, 0) >> 8, file) != EOF) && written; }
		written = fclose(file) == 0 && written;
		SIM_CHECK(written);
		SIM_CHECK(sim_switch("CONNECTION", "CONNECTED", INDIGO_OK_STATE));
		SIM_CHECK(sim_switch("CCD_IMAGE_FORMAT", "RAW", INDIGO_OK_STATE));
		SIM_CHECK(sim_switch("CCD_UPLOAD_MODE", "CLIENT", INDIGO_OK_STATE));
		atomic_store(&sim_check_noise, true);
		atomic_store(&sim_bad_frames, 0);
		SIM_CHECK(sim_expose());
		SIM_CHECK(atomic_load(&sim_signature) == signatures[format] && atomic_load(&sim_width) == 64 && atomic_load(&sim_height) == 48);
		SIM_CHECK(sim_switch("CONNECTION", "DISCONNECTED", INDIGO_OK_STATE));
	}
cleanup:
	atomic_store(&sim_check_noise, false);
	if (context.connected) { sim_switch("CONNECTION", "DISCONNECTED", INDIGO_OK_STATE); }
	indigo_ccd_simulator(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_detach_client(&simulator_test_client);
	indigo_stop();
	release_cached_properties();
	unlink(sim_raw_path);
	simulator_test_client.update_property = simulator_client_update_property;
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	const indigo_test_case tests[] = {
		{ "simulator_countdown_progress_abort_and_restart", simulator_countdown_progress_abort_and_restart },
		{ "simulator_raw_geometry_and_bins", simulator_raw_geometry_and_bins },
		{ "simulator_stream_abort_and_reconnect", simulator_stream_abort_and_reconnect },
		{ "simulator_cooling_target_and_polling", simulator_cooling_target_and_polling },
		{ "simulator_camera_modes_and_settings", simulator_camera_modes_and_settings },
		{ "simulator_file_noise_formats_and_failure", simulator_file_noise_formats_and_failure },
		{ "driver_info_reports_simulator_metadata", driver_info_reports_simulator_metadata },
		{ "simulator_initializes_enumerates_connects_disconnects_and_shuts_down", simulator_initializes_enumerates_connects_disconnects_and_shuts_down },
		{ "ccd_imager_passes_compliance_checks", ccd_imager_passes_compliance_checks },
		{ "ccd_wheel_passes_compliance_checks", ccd_wheel_passes_compliance_checks },
		{ "ccd_focuser_passes_compliance_checks", ccd_focuser_passes_compliance_checks },
		{ "ccd_guider_camera_passes_compliance_checks", ccd_guider_camera_passes_compliance_checks },
		{ "ccd_guider_passes_compliance_checks", ccd_guider_passes_compliance_checks },
		{ "ccd_ao_passes_compliance_checks", ccd_ao_passes_compliance_checks },
		{ "ccd_bahtinov_camera_passes_compliance_checks", ccd_bahtinov_camera_passes_compliance_checks },
		{ "ccd_dslr_passes_compliance_checks", ccd_dslr_passes_compliance_checks },
		{ "ccd_file_camera_passes_compliance_checks", ccd_file_camera_passes_compliance_checks }
	};
	int result = 0, matched = 0;
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		if (argc < 2 || strstr(tests[i].name, argv[1])) {
			matched++;
			result |= indigo_run_tests("CCD simulator integration tests", tests + i, 1);
		}
	}
	return matched ? result : 1;
}
