// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO CCD Simulator driver
 \file indigo_ccd_simulator.c
 */

#define DRIVER_VERSION 0x0001

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "indigo_driver_xml.h"

#include "indigo_ccd_simulator.h"

#define WIDTH               1600
#define HEIGHT              1200
#define TEMP_UPDATE         5.0
#define STARS               100

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((simulator_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
	int star_x[STARS], star_y[STARS], star_a[STARS];
	char image[FITS_HEADER_SIZE + 2 * WIDTH * HEIGHT];
	double target_temperature, current_temperature;
	int target_slot, current_slot;
	int target_position, current_position;
	indigo_timer *exposure_timer, *temperature_timer, *guider_timer;
} simulator_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		simulator_private_data *private_data = PRIVATE_DATA;
		unsigned short *raw = (unsigned short *)(private_data->image+FITS_HEADER_SIZE);
		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		int frame_width = (int)CCD_FRAME_WIDTH_ITEM->number.value / horizontal_bin;
		int frame_height = (int)CCD_FRAME_HEIGHT_ITEM->number.value / vertical_bin;
		int size = frame_width * frame_height;
		for (int i = 0; i < size; i++)
			raw[i] = (rand() & 0xFF); // noise
		for (int i = 0; i < STARS; i++) {
			double centerX = (private_data->star_x[i]+rand()/(double)RAND_MAX/5-0.5)/horizontal_bin;
			double centerY = (private_data->star_y[i]+rand()/(double)RAND_MAX/5-0.5)/vertical_bin;
			int a = private_data->star_a[i];
			int xMax = (int)round(centerX)+4/horizontal_bin;
			int yMax = (int)round(centerY)+4/vertical_bin;
			for (int y = yMax-8/vertical_bin; y <= yMax; y++) {
				int yw = y*frame_width;
				for (int x = xMax-8/horizontal_bin; x <= xMax; x++) {
					double xx = centerX-x;
					double yy = centerY-y;
					double v = a*exp(-(xx*xx/2.0+yy*yy/2.0));
					raw[yw+x] += (unsigned short)v;
				}
			}
		}
		indigo_process_image(device, private_data->image, frame_width, frame_height, true, NULL);
	}
}

static void ccd_temperature_callback(indigo_device *device) {
	double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
	if (diff > 0) {
		if (diff > 10) {
			if (CCD_COOLER_ON_ITEM->sw.value && CCD_COOLER_POWER_ITEM->number.value != 100) {
				CCD_COOLER_POWER_ITEM->number.value = 100;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		} else if (diff > 5) {
			if (CCD_COOLER_ON_ITEM->sw.value && CCD_COOLER_POWER_ITEM->number.value != 50) {
				CCD_COOLER_POWER_ITEM->number.value = 50;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
		}
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
		CCD_TEMPERATURE_ITEM->number.value = --(PRIVATE_DATA->current_temperature);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else if (diff < 0) {
		if (CCD_COOLER_POWER_ITEM->number.value > 0) {
			CCD_COOLER_POWER_ITEM->number.value = 0;
			indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		}
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
		CCD_TEMPERATURE_ITEM->number.value = ++(PRIVATE_DATA->current_temperature);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	} else {
		CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, TEMP_UPDATE, &PRIVATE_DATA->temperature_timer);
}

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);

	simulator_private_data *private_data = device->device_context;
	device->device_context = NULL;

	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = false;
		SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
		SIMULATION_ENABLED_ITEM->sw.value = true;
		SIMULATION_DISABLED_ITEM->sw.value = false;
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_MODE, CCD_FRAME
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = WIDTH;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = HEIGHT;
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.max = 4;
		CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.max = 4;
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 3;
		char name[32];
		sprintf(name, "RAW %dx%d", WIDTH, HEIGHT);
		indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
		sprintf(name, "RAW %dx%d", WIDTH/2, HEIGHT/2);
		indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
		sprintf(name, "RAW %dx%d", WIDTH/4, HEIGHT/4);
		indigo_init_switch_item(CCD_MODE_ITEM+2, "BIN_4x4", name, false);
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = 5.2;
		CCD_INFO_PIXEL_WIDTH_ITEM->number.value = 5.2;
		CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		// -------------------------------------------------------------------------------- CCD_IMAGE
		for (int i = 0; i < STARS; i++) {
			private_data->star_x[i] = rand() % (WIDTH - 20) + 10; // generate some star positions
			private_data->star_y[i] = rand() % (HEIGHT - 20) + 10;
			private_data->star_a[i] = 1000 * (rand() % 60);       // and brightness
		}
		// -------------------------------------------------------------------------------- CCD_COOLER, CCD_TEMPERATURE, CCD_COOLER_POWER
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		CCD_COOLER_POWER_PROPERTY->hidden = false;
		indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
		private_data->target_temperature = private_data->current_temperature = CCD_TEMPERATURE_ITEM->number.value = 25;
		CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_COOLER_POWER_ITEM->number.value = 0;
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
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value)
			PRIVATE_DATA->temperature_timer = indigo_set_timer(device, TEMP_UPDATE, ccd_temperature_callback);
		else {
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value, exposure_timer_callback);
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
		}
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		int h = CCD_BIN_HORIZONTAL_ITEM->number.value;
		int v = CCD_BIN_VERTICAL_ITEM->number.value;
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		if (!(CCD_BIN_HORIZONTAL_ITEM->number.value == 1 || CCD_BIN_HORIZONTAL_ITEM->number.value == 2 || CCD_BIN_HORIZONTAL_ITEM->number.value == 4) || CCD_BIN_HORIZONTAL_ITEM->number.value != CCD_BIN_VERTICAL_ITEM->number.value) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = h;
			CCD_BIN_VERTICAL_ITEM->number.value = v;
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			return INDIGO_OK;
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CCD_COOLER_ON_ITEM->sw.value) {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
		} else {
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
			CCD_COOLER_POWER_ITEM->number.value = 0;
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value = 25;
		}
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		indigo_delete_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
		CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target temperature %g", PRIVATE_DATA->target_temperature);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_timer_callback(indigo_device *device) {
	PRIVATE_DATA->guider_timer = NULL;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value != 0 || GUIDER_GUIDE_SOUTH_ITEM->number.value != 0) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	if (GUIDER_GUIDE_EAST_ITEM->number.value != 0 || GUIDER_GUIDE_WEST_ITEM->number.value != 0) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	simulator_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
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
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);		
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				PRIVATE_DATA->guider_timer = indigo_set_timer(device, duration/1000.0, guider_timer_callback);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO wheel device implementation

#define FILTER_COUNT	5

static void wheel_timer_callback(indigo_device *device) {
	PRIVATE_DATA->current_slot = (PRIVATE_DATA->current_slot) % (int)WHEEL_SLOT_ITEM->number.max + 1;
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
	simulator_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_wheel_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- WHEEL_SLOT, WHEEL_SLOT_NAME
		WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = FILTER_COUNT;
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot = PRIVATE_DATA->target_slot = 1;
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
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
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
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
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_ALERT_STATE) {
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	} else {
		if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value && PRIVATE_DATA->current_position < PRIVATE_DATA->target_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_ITEM->number.value = (PRIVATE_DATA->current_position += FOCUSER_SPEED_ITEM->number.value);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_set_timer(device, 0.1, focuser_timer_callback);
		} else if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value && PRIVATE_DATA->current_position > PRIVATE_DATA->target_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			FOCUSER_POSITION_ITEM->number.value = (PRIVATE_DATA->current_position -= FOCUSER_SPEED_ITEM->number.value);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			indigo_set_timer(device, 0.1, focuser_timer_callback);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
	}
}

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	simulator_private_data *private_data = device->device_context;
	device->device_context = NULL;
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_CONTEXT->private_data = private_data;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = 1;
		// --------------------------------------------------------------------------------
		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(device->device_context != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - FOCUSER_STEPS_ITEM->number.value;
		} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + FOCUSER_STEPS_ITEM->number.value;
		}
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0.5, focuser_timer_callback);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.value;
		FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		indigo_set_timer(device, 0.5, focuser_timer_callback);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device);
	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;

static indigo_device *imager_ccd = NULL;
static indigo_device *imager_wheel = NULL;
static indigo_device *imager_focuser = NULL;

static indigo_device *guider_ccd = NULL;
static indigo_device *guider_guider = NULL;

indigo_result indigo_ccd_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device imager_camera_template = {
		CCD_SIMULATOR_IMAGER_CAMERA_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device imager_wheel_template = {
		CCD_SIMULATOR_WHEEL_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		wheel_detach
	};
	static indigo_device imager_focuser_template = {
		CCD_SIMULATOR_FOCUSER_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		focuser_attach,
		indigo_focuser_enumerate_properties,
		focuser_change_property,
		focuser_detach
	};
	static indigo_device guider_camera_template = {
		CCD_SIMULATOR_GUIDER_CAMERA_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		indigo_ccd_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};
	static indigo_device guider_template = {
		CCD_SIMULATOR_GUIDER_NAME, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "CCD Simulator", __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		private_data = malloc(sizeof(simulator_private_data));
		assert(private_data != NULL);
		imager_ccd = malloc(sizeof(indigo_device));
		assert(imager_ccd != NULL);
		memcpy(imager_ccd, &imager_camera_template, sizeof(indigo_device));
		imager_ccd->device_context = private_data;
		indigo_attach_device(imager_ccd);
		imager_wheel = malloc(sizeof(indigo_device));
		assert(imager_wheel != NULL);
		memcpy(imager_wheel, &imager_wheel_template, sizeof(indigo_device));
		imager_wheel->device_context = private_data;
		indigo_attach_device(imager_wheel);
		imager_focuser = malloc(sizeof(indigo_device));
		assert(imager_focuser != NULL);
		memcpy(imager_focuser, &imager_focuser_template, sizeof(indigo_device));
		imager_focuser->device_context = private_data;
		indigo_attach_device(imager_focuser);
		guider_ccd = malloc(sizeof(indigo_device));
		assert(guider_ccd != NULL);
		memcpy(guider_ccd, &guider_camera_template, sizeof(indigo_device));
		guider_ccd->device_context = private_data;
		indigo_attach_device(guider_ccd);		
		guider_guider = malloc(sizeof(indigo_device));
		assert(guider_guider != NULL);
		memcpy(guider_guider, &guider_template, sizeof(indigo_device));
		guider_guider->device_context = private_data;
		indigo_attach_device(guider_guider);
		break;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		if (imager_ccd != NULL) {
			indigo_detach_device(imager_ccd);
			free(imager_ccd);
			imager_ccd = NULL;
		}
		if (imager_wheel != NULL) {
			indigo_detach_device(imager_wheel);
			free(imager_wheel);
			imager_wheel = NULL;
		}
		if (imager_focuser != NULL) {
			indigo_detach_device(imager_focuser);
			free(imager_focuser);
			imager_focuser = NULL;
		}
		if (guider_ccd != NULL) {
			indigo_detach_device(guider_ccd);
			free(guider_ccd);
			guider_ccd = NULL;
		}
		if (guider_guider != NULL) {
			indigo_detach_device(guider_guider);
			free(guider_guider);
			guider_guider = NULL;
		}
		if (private_data != NULL) {
			free(private_data);
			private_data = NULL;
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}
	return INDIGO_OK;
}

