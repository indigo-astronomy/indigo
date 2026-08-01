// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

#include <indigo_drivers/ccd_simulator/indigo_ccd_simulator.h>

#include "simulator_test_common.h"

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

int main(void) {
	const indigo_test_case tests[] = {
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
	return indigo_run_tests("CCD simulator integration tests", tests, ARRAY_SIZE(tests));
}
