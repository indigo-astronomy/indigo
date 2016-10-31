//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//	You can use this software under the terms of 'INDIGO Astronomy
//  open-source license' (see LICENSE.md).
//
//	THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
//	OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//	ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Atik CCD driver
 \file indigo_ccd_atik.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <libatik/libatik.h>

#include "indigo_driver_xml.h"

#include "indigo_ccd_atik.h"


#undef PRIVATE_DATA
#define PRIVATE_DATA        ((atik_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
	libatik_device_context *device_context;
	libusb_device *dev;
	int device_count;
	indigo_timer *exposure_timer, *temperture_timer, *guider_timer;
	double exposure;
	double cooler_power, target_temperature, current_temperature;
	int target_slot, current_slot;
	unsigned short relay_mask;
	unsigned char *buffer;
	int image_width;
	int image_height;
	bool can_check_temperature;
} atik_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (libatik_read_pixels(PRIVATE_DATA->device_context, 0, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value, (unsigned short *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), &PRIVATE_DATA->image_width, &PRIVATE_DATA->image_height)) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure done");
			indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->image_width, PRIVATE_DATA->image_height, PRIVATE_DATA->exposure);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}

static void pre_exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 2, exposure_timer_callback);
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	if (PRIVATE_DATA->can_check_temperature) {
		bool status;
		if (libatik_check_cooler(PRIVATE_DATA->device_context, &status, &PRIVATE_DATA->cooler_power, &PRIVATE_DATA->current_temperature)) {
			if (CCD_COOLER_ON_ITEM->sw.value != status)
				libatik_set_cooler(PRIVATE_DATA->device_context, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature);
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if (CCD_COOLER_ON_ITEM->sw.value)
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 1 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			else
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10) / 10;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_ITEM->number.value = round(PRIVATE_DATA->cooler_power);
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 5, ccd_temperature_callback);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	atik_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_ccd_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN
		CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 4;
		CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 4;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = libatik_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context);
			}
			if (result) {
				CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = PRIVATE_DATA->device_context->width;
				CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = PRIVATE_DATA->device_context->height;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(PRIVATE_DATA->device_context->pixel_width * 100)/100;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(PRIVATE_DATA->device_context->pixel_height * 100) / 100;
				PRIVATE_DATA->buffer = malloc(2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
				assert(PRIVATE_DATA->buffer != NULL);
				if (PRIVATE_DATA->device_context->has_cooler) {
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_COOLER_POWER_PROPERTY->hidden = false;
					CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;
					bool status;
					libatik_check_cooler(PRIVATE_DATA->device_context, &status, &PRIVATE_DATA->cooler_power, &PRIVATE_DATA->current_temperature);
					PRIVATE_DATA->target_temperature = 0;
					ccd_temperature_callback(device);
				}
				PRIVATE_DATA->can_check_temperature = true;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				if (PRIVATE_DATA->temperture_timer) {
					indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);
					PRIVATE_DATA->temperture_timer = NULL;
				}
				if (PRIVATE_DATA->buffer != NULL)
					free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (PRIVATE_DATA->temperture_timer != NULL)
				indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);
			if (--PRIVATE_DATA->device_count == 0) {
				libatik_close(PRIVATE_DATA->device_context);
				if (PRIVATE_DATA->buffer != NULL)
					free(PRIVATE_DATA->buffer);
				PRIVATE_DATA->buffer = NULL;
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		PRIVATE_DATA->exposure = CCD_EXPOSURE_ITEM->number.value;
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (PRIVATE_DATA->exposure <= 1) {
			PRIVATE_DATA->can_check_temperature = false;
			CCD_EXPOSURE_ITEM->number.value = 0;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			if (libatik_read_pixels(PRIVATE_DATA->device_context, PRIVATE_DATA->exposure, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value, (unsigned short *)(PRIVATE_DATA->buffer + FITS_HEADER_SIZE), &PRIVATE_DATA->image_width, &PRIVATE_DATA->image_height)) {
				CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
				indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->image_width, PRIVATE_DATA->image_height, PRIVATE_DATA->exposure);
			} else {
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			}
			PRIVATE_DATA->can_check_temperature = true;
		} else {
			libatik_start_exposure(PRIVATE_DATA->device_context, CCD_FRAME_TYPE_DARK_ITEM->sw.value);
			if (PRIVATE_DATA->exposure > 2)
				PRIVATE_DATA->exposure_timer = indigo_set_timer(device, PRIVATE_DATA->exposure - 2, pre_exposure_timer_callback);
			else {
				PRIVATE_DATA->can_check_temperature = false;
				PRIVATE_DATA->exposure_timer = indigo_set_timer(device, PRIVATE_DATA->exposure, exposure_timer_callback);
			}
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			libatik_abort_exposure(PRIVATE_DATA->device_context);
			indigo_cancel_timer(device, PRIVATE_DATA->exposure_timer);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10) / 10;
			if (CCD_COOLER_OFF_ITEM->sw.value) {
				indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
				CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			}
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_timer_callback(indigo_device *device) {
	PRIVATE_DATA->guider_timer = NULL;
	libatik_guide_relays(PRIVATE_DATA->device_context, 0);
	if (PRIVATE_DATA->relay_mask & (ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH)) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	if (PRIVATE_DATA->relay_mask & (ATIK_GUIDE_WEST | ATIK_GUIDE_EAST)) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	PRIVATE_DATA->relay_mask = 0;
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	atik_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_guider_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = libatik_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context);
			}
			if (result) {
				assert(PRIVATE_DATA->device_context->has_guider_port);
				libatik_guide_relays(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask = 0);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (--PRIVATE_DATA->device_count == 0) {
				libatik_close(PRIVATE_DATA->device_context);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer);
		PRIVATE_DATA->relay_mask &= ~(ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= ATIK_GUIDE_NORTH;
			PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				PRIVATE_DATA->relay_mask |= ATIK_GUIDE_SOUTH;
				PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
			}
		}
		libatik_guide_relays(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask);
		GUIDER_GUIDE_DEC_PROPERTY->state = PRIVATE_DATA->relay_mask & (ATIK_GUIDE_NORTH | ATIK_GUIDE_SOUTH) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		if (PRIVATE_DATA->guider_timer != NULL)
			indigo_cancel_timer(device, PRIVATE_DATA->guider_timer);
		PRIVATE_DATA->relay_mask &= ~(ATIK_GUIDE_EAST | ATIK_GUIDE_WEST);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			PRIVATE_DATA->relay_mask |= ATIK_GUIDE_EAST;
			PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				PRIVATE_DATA->relay_mask |= ATIK_GUIDE_WEST;
				PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
			}
		}
		libatik_guide_relays(PRIVATE_DATA->device_context, PRIVATE_DATA->relay_mask);
		GUIDER_GUIDE_RA_PROPERTY->state = PRIVATE_DATA->relay_mask & (ATIK_GUIDE_WEST | ATIK_GUIDE_EAST) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

static void wheel_timer_callback(indigo_device *device) {
	libatik_check_filter_wheel(PRIVATE_DATA->device_context, &PRIVATE_DATA->current_slot);
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == PRIVATE_DATA->target_slot) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback);
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	atik_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_wheel_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			bool result = true;
			if (PRIVATE_DATA->device_count++ == 0) {
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, CONNECTION_PROPERTY, NULL);
				result = libatik_open(PRIVATE_DATA->dev, &PRIVATE_DATA->device_context);
			}
			if (result) {
				assert(PRIVATE_DATA->device_context->has_filter_wheel);
				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = PRIVATE_DATA->device_context->filter_count;
				libatik_check_filter_wheel(PRIVATE_DATA->device_context, &PRIVATE_DATA->current_slot);
				WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (--PRIVATE_DATA->device_count == 0) {
				libatik_close(PRIVATE_DATA->device_context);
			}
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_slot = WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
			libatik_set_filter_wheel(PRIVATE_DATA->device_context, PRIVATE_DATA->target_slot);
			indigo_set_timer(device, 0.5, wheel_timer_callback);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	indigo_device_disconnect(device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_wheel_detach(device);
}


// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES                   10

static indigo_device *devices[MAX_DEVICES];

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static indigo_device ccd_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device guider_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};
	static indigo_device wheel_template = {
		"", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		wheel_detach
	};
	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			libatik_camera_type type;
			const char *name;
			bool is_guider, has_fw;
			libatik_camera(dev, &type, &name, &is_guider, &has_fw);
			atik_private_data *private_data = malloc(sizeof(atik_private_data));
			assert(private_data != NULL);
			memset(private_data, 0, sizeof(atik_private_data));
			private_data->dev = dev;
			libusb_ref_device(dev);
			indigo_device *device = malloc(sizeof(indigo_device));
			assert(device != NULL);
			memcpy(device, &ccd_template, sizeof(indigo_device));
			strcpy(device->name, name);
			device->device_context = private_data;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
					break;
				}
			}
			if (is_guider) {
				device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &guider_template, sizeof(indigo_device));
				strcpy(device->name, name);
				strcat(device->name, " (guider)");
				device->device_context = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
						break;
					}
				}
			}
			if (has_fw) {
				device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &wheel_template, sizeof(indigo_device));
				strcpy(device->name, name);
				strcat(device->name, " (wheel)");
				device->device_context = private_data;
				for (int j = 0; j < MAX_DEVICES; j++) {
					if (devices[j] == NULL) {
						indigo_async((void *)(void *)indigo_attach_device, devices[j] = device);
						break;
					}
				}
			}
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			atik_private_data *private_data = NULL;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] != NULL) {
					indigo_device *device = devices[j];
					if (PRIVATE_DATA->dev == dev) {
						private_data = PRIVATE_DATA;
						indigo_detach_device(device);
						free(device);
						devices[j] = NULL;
					}
				}
			}
			if (private_data != NULL) {
				libusb_unref_device(dev);
				free(private_data);
			}
			break;
		}
	}
	return 0;
}

static libusb_hotplug_callback_handle callback_handle1, callback_handle2;

indigo_result indigo_ccd_atik(bool state) {
	libatik_use_syslog = indigo_use_syslog;
	static bool current_state = false;
	if (state == current_state)
		return INDIGO_OK;
	if ((current_state = state)) {
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		libusb_init(NULL);
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ATIK_VID1, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle1);
		if (rc >= 0)
			rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, ATIK_VID2, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle2);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_atik: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		indigo_start_usb_even_handler();
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
	} else {
		libusb_hotplug_deregister_callback(NULL, callback_handle1);
		libusb_hotplug_deregister_callback(NULL, callback_handle2);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_atik: libusb_hotplug_deregister_callback [%d]", __LINE__));
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		return INDIGO_OK;
	}
}


