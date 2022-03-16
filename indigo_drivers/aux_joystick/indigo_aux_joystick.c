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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO HID Joystick driver
 \file indigo_aux_joystick.c
 */

#define DRIVER_VERSION 0x0006
#define DRIVER_NAME "indigo_joystick"

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

#ifdef INDIGO_MACOS
#import <Cocoa/Cocoa.h>
#import "DDHidJoystick.h"
#endif

#ifdef INDIGO_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <float.h>
#include <errno.h>
#include <linux/joystick.h>

#define MAX_BUTTONS	64
#define MAX_AXES		16

#endif

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_usb_utils.h>

#include "indigo_aux_joystick.h"

#define JOYSTICK_CONFIG_GROUP	"Joystick configuration"
#define MOUNT_MAIN_GROUP			"Mount mapping"

#define PRIVATE_DATA        ((joystick_private_data *)device->private_data)

#define JOYSTICK_BUTTONS_PROPERTY											(PRIVATE_DATA->joystick_buttons_property)

#define JOYSTICK_AXES_PROPERTY												(PRIVATE_DATA->joystick_axes_property)

#define JOYSTICK_MAPPING_PROPERTY											(PRIVATE_DATA->joystick_mapping_property)
#define JOYSTICK_MAPPING_PARKED_ITEM									(JOYSTICK_MAPPING_PROPERTY->items+0)
#define JOYSTICK_MAPPING_UNPARKED_ITEM								(JOYSTICK_MAPPING_PROPERTY->items+1)
#define JOYSTICK_MAPPING_ABORT_ITEM										(JOYSTICK_MAPPING_PROPERTY->items+2)
#define JOYSTICK_MAPPING_TRACKING_ON_ITEM							(JOYSTICK_MAPPING_PROPERTY->items+3)
#define JOYSTICK_MAPPING_TRACKING_OFF_ITEM						(JOYSTICK_MAPPING_PROPERTY->items+4)
#define JOYSTICK_MAPPING_MOTION_RA_ITEM								(JOYSTICK_MAPPING_PROPERTY->items+5)
#define JOYSTICK_MAPPING_MOTION_DEC_ITEM							(JOYSTICK_MAPPING_PROPERTY->items+6)
#define JOYSTICK_MAPPING_RATE_GUIDE_ITEM							(JOYSTICK_MAPPING_PROPERTY->items+7)
#define JOYSTICK_MAPPING_RATE_CENTERING_ITEM					(JOYSTICK_MAPPING_PROPERTY->items+8)
#define JOYSTICK_MAPPING_RATE_FIND_ITEM								(JOYSTICK_MAPPING_PROPERTY->items+9)
#define JOYSTICK_MAPPING_RATE_MAX_ITEM								(JOYSTICK_MAPPING_PROPERTY->items+10)

#define JOYSTICK_OPTIONS_PROPERTY											(PRIVATE_DATA->joystick_options_property)
#define JOYSTICK_OPTIONS_ANALOG_STICK_ITEM						(JOYSTICK_OPTIONS_PROPERTY->items+0)
#define JOYSTICK_OPTIONS_SWAP_RA_ITEM									(JOYSTICK_OPTIONS_PROPERTY->items+1)
#define JOYSTICK_OPTIONS_SWAP_DEC_ITEM								(JOYSTICK_OPTIONS_PROPERTY->items+2)

#define MOUNT_PARK_PROPERTY														(PRIVATE_DATA->mount_park_property)
#define MOUNT_PARK_PARKED_ITEM												(MOUNT_PARK_PROPERTY->items+0)
#define MOUNT_PARK_UNPARKED_ITEM											(MOUNT_PARK_PROPERTY->items+1)

#define MOUNT_SLEW_RATE_PROPERTY											(PRIVATE_DATA->mount_slew_rate_property)
#define MOUNT_SLEW_RATE_GUIDE_ITEM										(MOUNT_SLEW_RATE_PROPERTY->items+0)
#define MOUNT_SLEW_RATE_CENTERING_ITEM								(MOUNT_SLEW_RATE_PROPERTY->items+1)
#define MOUNT_SLEW_RATE_FIND_ITEM											(MOUNT_SLEW_RATE_PROPERTY->items+2)
#define MOUNT_SLEW_RATE_MAX_ITEM											(MOUNT_SLEW_RATE_PROPERTY->items+3)

#define MOUNT_MOTION_DEC_PROPERTY											(PRIVATE_DATA->mount_motion_dec_property)
#define MOUNT_MOTION_NORTH_ITEM												(MOUNT_MOTION_DEC_PROPERTY->items+0)
#define MOUNT_MOTION_SOUTH_ITEM						        	  (MOUNT_MOTION_DEC_PROPERTY->items+1)

#define MOUNT_MOTION_RA_PROPERTY											(PRIVATE_DATA->mount_motion_ra_property)
#define MOUNT_MOTION_WEST_ITEM												(MOUNT_MOTION_RA_PROPERTY->items+0)
#define MOUNT_MOTION_EAST_ITEM						            (MOUNT_MOTION_RA_PROPERTY->items+1)

#define MOUNT_ABORT_MOTION_PROPERTY										(PRIVATE_DATA->mount_abort_motion_property)
#define MOUNT_ABORT_MOTION_ITEM												(MOUNT_ABORT_MOTION_PROPERTY->items+0)

#define MOUNT_TRACKING_PROPERTY												(PRIVATE_DATA->mount_tracking_property)
#define MOUNT_TRACKING_ON_ITEM												(MOUNT_TRACKING_PROPERTY->items+0)
#define MOUNT_TRACKING_OFF_ITEM												(MOUNT_TRACKING_PROPERTY->items+1)

typedef struct {
	long index;
	int button_count;
	int axis_count;
	int pov_count;
	int dec_slew_rate;
	int ra_slew_rate;
	indigo_property *joystick_buttons_property;
	indigo_property *joystick_axes_property;
	indigo_property *joystick_mapping_property;
	indigo_property *joystick_options_property;
	indigo_property *mount_park_property;
	indigo_property *mount_slew_rate_property;
	indigo_property *mount_motion_dec_property;
	indigo_property *mount_motion_ra_property;
	indigo_property *mount_abort_motion_property;
	indigo_property *mount_tracking_property;
#ifdef INDIGO_LINUX
	int fd;
	pthread_t thread;
	bool last_button_state[MAX_BUTTONS];
	int last_axis_value[MAX_AXES];
#endif
} joystick_private_data;

static bool open_joystick(indigo_device *device);
static void close_joystick(indigo_device *device);

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	assert(device != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_JOYSTICK) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- JOYSTICK_BUTTONS
		JOYSTICK_BUTTONS_PROPERTY = indigo_init_switch_property(NULL, device->name, JOYSTICK_BUTTONS_PROPERTY_NAME, JOYSTICK_CONFIG_GROUP, "Joystick buttons", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ANY_OF_MANY_RULE, PRIVATE_DATA->button_count);
		if (JOYSTICK_BUTTONS_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 0; i < PRIVATE_DATA->button_count; i++) {
			char name[INDIGO_NAME_SIZE], label[INDIGO_NAME_SIZE];
			sprintf(name, JOYSTICK_BUTTON_ITEM_NAME, i + 1);
			sprintf(label, "Button %d", i + 1);
			indigo_init_switch_item(JOYSTICK_BUTTONS_PROPERTY->items + i, name, label, false);
		}
		// -------------------------------------------------------------------------------- JOYSTICK_AXES
		int axis_count = PRIVATE_DATA->axis_count + 2 * PRIVATE_DATA->pov_count;
		JOYSTICK_AXES_PROPERTY = indigo_init_number_property(NULL, device->name, JOYSTICK_AXES_PROPERTY_NAME, JOYSTICK_CONFIG_GROUP, "Joystick axes", INDIGO_OK_STATE, INDIGO_RO_PERM, axis_count);
		if (JOYSTICK_AXES_PROPERTY == NULL)
			return INDIGO_FAILED;
		for (int i = 0; i < axis_count; i++) {
			char name[INDIGO_NAME_SIZE], label[INDIGO_NAME_SIZE];
			sprintf(name, JOYSTICK_AXIS_ITEM_NAME, i + 1);
			sprintf(label, "Axis %d", i + 1);
			indigo_init_number_item(JOYSTICK_AXES_PROPERTY->items + i, name, label, -65536, 65536, 0, 0);
		}
		// -------------------------------------------------------------------------------- JOYSTICK_MAPPING
		JOYSTICK_MAPPING_PROPERTY = indigo_init_number_property(NULL, device->name, JOYSTICK_MAPPING_PROPERTY_NAME, JOYSTICK_CONFIG_GROUP, "Buttons and axes mapping", INDIGO_OK_STATE, INDIGO_RW_PERM, 11);
		if (JOYSTICK_MAPPING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(JOYSTICK_MAPPING_PARKED_ITEM, JOYSTICK_MAPPING_PARKED_ITEM_NAME, "Park mount button", 0, PRIVATE_DATA->button_count, 1, 6);
		indigo_init_number_item(JOYSTICK_MAPPING_UNPARKED_ITEM, JOYSTICK_MAPPING_UNPARKED_ITEM_NAME, "Unpark mount button", 0, PRIVATE_DATA->button_count, 1, 8);
		indigo_init_number_item(JOYSTICK_MAPPING_ABORT_ITEM, JOYSTICK_MAPPING_ABORT_ITEM_NAME, "Abort mount movement button", 0, PRIVATE_DATA->button_count, 1, 9);
		indigo_init_number_item(JOYSTICK_MAPPING_TRACKING_ON_ITEM, JOYSTICK_MAPPING_TRACKING_ON_ITEM_NAME, "Tracking on  button", 0, PRIVATE_DATA->button_count, 1, 5);
		indigo_init_number_item(JOYSTICK_MAPPING_TRACKING_OFF_ITEM, JOYSTICK_MAPPING_TRACKING_OFF_ITEM_NAME, "Tracking off  button", 0, PRIVATE_DATA->button_count, 1, 7);
		indigo_init_number_item(JOYSTICK_MAPPING_MOTION_RA_ITEM, JOYSTICK_MAPPING_MOTION_RA_ITEM_NAME, "RA motion axis", 0, PRIVATE_DATA->axis_count + 2 * PRIVATE_DATA->pov_count, 1, 1);
		indigo_init_number_item(JOYSTICK_MAPPING_MOTION_DEC_ITEM, JOYSTICK_MAPPING_MOTION_DEC_ITEM_NAME, "Dec motion axis", 0, PRIVATE_DATA->axis_count + 2 * PRIVATE_DATA->pov_count, 1, 2);
		indigo_init_number_item(JOYSTICK_MAPPING_RATE_GUIDE_ITEM, JOYSTICK_MAPPING_RATE_GUIDE_ITEM_NAME, "Guide rate button", 0, PRIVATE_DATA->button_count, 1, 1);
		indigo_init_number_item(JOYSTICK_MAPPING_RATE_CENTERING_ITEM, JOYSTICK_MAPPING_RATE_CENTERING_ITEM_NAME, "Centering rate button", 0, PRIVATE_DATA->button_count, 1, 2);
		indigo_init_number_item(JOYSTICK_MAPPING_RATE_FIND_ITEM, JOYSTICK_MAPPING_RATE_FIND_ITEM_NAME, "Find rate button", 0, PRIVATE_DATA->button_count, 1, 3);
		indigo_init_number_item(JOYSTICK_MAPPING_RATE_MAX_ITEM, JOYSTICK_MAPPING_RATE_MAX_ITEM_NAME, "Max rate button", 0, PRIVATE_DATA->button_count, 1, 4);
		// -------------------------------------------------------------------------------- MOUNT_PARK
		JOYSTICK_OPTIONS_PROPERTY = indigo_init_switch_property(NULL, device->name, JOYSTICK_OPTIONS_PROPERTY_NAME, JOYSTICK_CONFIG_GROUP, "Options", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 3);
		if (JOYSTICK_OPTIONS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(JOYSTICK_OPTIONS_ANALOG_STICK_ITEM, JOYSTICK_OPTIONS_ANALOG_STICK_ITEM_NAME, "Use stick in analog mode", false);
		indigo_init_switch_item(JOYSTICK_OPTIONS_SWAP_RA_ITEM, JOYSTICK_OPTIONS_SWAP_RA_ITEM_NAME, "Swap RA axis", false);
		indigo_init_switch_item(JOYSTICK_OPTIONS_SWAP_DEC_ITEM, JOYSTICK_OPTIONS_SWAP_DEC_ITEM_NAME, "Swap Dec axis", false);
		// -------------------------------------------------------------------------------- MOUNT_PARK
		MOUNT_PARK_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_PARK_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Park", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_PARK_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_PARK_PARKED_ITEM, MOUNT_PARK_PARKED_ITEM_NAME, "Mount parked", false);
		indigo_init_switch_item(MOUNT_PARK_UNPARKED_ITEM, MOUNT_PARK_UNPARKED_ITEM_NAME, "Mount unparked", false);
		// -------------------------------------------------------------------------------- MOUNT_SLEW_RATE
		MOUNT_SLEW_RATE_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Slew rate", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 4);
		if (MOUNT_SLEW_RATE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_SLEW_RATE_GUIDE_ITEM, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, "Guide rate", false);
		indigo_init_switch_item(MOUNT_SLEW_RATE_CENTERING_ITEM, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, "Centering rate", false);
		indigo_init_switch_item(MOUNT_SLEW_RATE_FIND_ITEM, MOUNT_SLEW_RATE_FIND_ITEM_NAME, "Find rate", false);
		indigo_init_switch_item(MOUNT_SLEW_RATE_MAX_ITEM, MOUNT_SLEW_RATE_MAX_ITEM_NAME, "Max rate", false);
		// -------------------------------------------------------------------------------- MOUNT_MOTION_NS
		MOUNT_MOTION_DEC_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move N/S", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_MOTION_DEC_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_MOTION_NORTH_ITEM, MOUNT_MOTION_NORTH_ITEM_NAME, "North", false);
		indigo_init_switch_item(MOUNT_MOTION_SOUTH_ITEM, MOUNT_MOTION_SOUTH_ITEM_NAME, "South", false);
		// -------------------------------------------------------------------------------- MOUNT_MOTION_WE
		MOUNT_MOTION_RA_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Move W/E", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_MOTION_RA_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_MOTION_WEST_ITEM, MOUNT_MOTION_WEST_ITEM_NAME, "West", false);
		indigo_init_switch_item(MOUNT_MOTION_EAST_ITEM, MOUNT_MOTION_EAST_ITEM_NAME, "East", false);
		// -------------------------------------------------------------------------------- MOUNT_ABORT_MOTION
		MOUNT_ABORT_MOTION_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Abort motion", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (MOUNT_ABORT_MOTION_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_ABORT_MOTION_ITEM, MOUNT_ABORT_MOTION_ITEM_NAME, "Abort motion", false);
		// -------------------------------------------------------------------------------- MOUNT_TRACKING
		MOUNT_TRACKING_PROPERTY = indigo_init_switch_property(NULL, device->name, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_MAIN_GROUP, "Tracking", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 2);
		if (MOUNT_TRACKING_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(MOUNT_TRACKING_ON_ITEM, MOUNT_TRACKING_ON_ITEM_NAME, "Tracking", false);
		indigo_init_switch_item(MOUNT_TRACKING_OFF_ITEM, MOUNT_TRACKING_OFF_ITEM_NAME, "Stopped" , false);

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	indigo_result result = INDIGO_OK;
	if ((result = indigo_aux_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(JOYSTICK_BUTTONS_PROPERTY, property))
				indigo_define_property(device, JOYSTICK_BUTTONS_PROPERTY, NULL);
			if (indigo_property_match(JOYSTICK_AXES_PROPERTY, property))
				indigo_define_property(device, JOYSTICK_AXES_PROPERTY, NULL);
			if (indigo_property_match(JOYSTICK_MAPPING_PROPERTY, property))
				indigo_define_property(device, JOYSTICK_MAPPING_PROPERTY, NULL);
			if (indigo_property_match(JOYSTICK_OPTIONS_PROPERTY, property))
				indigo_define_property(device, JOYSTICK_OPTIONS_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_PARK_PROPERTY, property))
				indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_SLEW_RATE_PROPERTY, property))
				indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_MOTION_DEC_PROPERTY, property))
				indigo_define_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_MOTION_RA_PROPERTY, property))
				indigo_define_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_TRACKING_PROPERTY, property))
				indigo_define_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			if (indigo_property_match(MOUNT_ABORT_MOTION_PROPERTY, property))
				indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
		}
	}
	return result;
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (open_joystick(device)) {
				indigo_define_property(device, JOYSTICK_AXES_PROPERTY, NULL);
				indigo_define_property(device, JOYSTICK_BUTTONS_PROPERTY, NULL);
				indigo_define_property(device, JOYSTICK_MAPPING_PROPERTY, NULL);
				indigo_define_property(device, JOYSTICK_OPTIONS_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_PARK_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_TRACKING_PROPERTY, NULL);
				indigo_define_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			close_joystick(device);
			indigo_delete_property(device, JOYSTICK_AXES_PROPERTY, NULL);
			indigo_delete_property(device, JOYSTICK_BUTTONS_PROPERTY, NULL);
			indigo_delete_property(device, JOYSTICK_MAPPING_PROPERTY, NULL);
			indigo_delete_property(device, JOYSTICK_OPTIONS_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_PARK_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_TRACKING_PROPERTY, NULL);
			indigo_delete_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(JOYSTICK_MAPPING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- JOYSTICK_MAPPING
		indigo_property_copy_values(JOYSTICK_MAPPING_PROPERTY, property, false);
		JOYSTICK_MAPPING_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, JOYSTICK_MAPPING_PROPERTY, NULL);
		}
	} else if (indigo_property_match(JOYSTICK_OPTIONS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- JOYSTICK_OPTIONS
		indigo_property_copy_values(JOYSTICK_OPTIONS_PROPERTY, property, false);
		JOYSTICK_OPTIONS_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, JOYSTICK_OPTIONS_PROPERTY, NULL);
		}
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, JOYSTICK_MAPPING_PROPERTY);
			indigo_save_property(device, NULL, JOYSTICK_OPTIONS_PROPERTY);
		}
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		close_joystick(device);
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	}
	indigo_release_property(JOYSTICK_AXES_PROPERTY);
	indigo_release_property(JOYSTICK_BUTTONS_PROPERTY);
	indigo_release_property(JOYSTICK_MAPPING_PROPERTY);
	indigo_release_property(JOYSTICK_OPTIONS_PROPERTY);
	indigo_release_property(MOUNT_PARK_PROPERTY);
	indigo_release_property(MOUNT_SLEW_RATE_PROPERTY);
	indigo_release_property(MOUNT_MOTION_DEC_PROPERTY);
	indigo_release_property(MOUNT_MOTION_RA_PROPERTY);
	indigo_release_property(MOUNT_TRACKING_PROPERTY);
	indigo_release_property(MOUNT_ABORT_MOTION_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- platform implementation glue

static indigo_device *allocate_device(const char *name, long index, int button_count, int axis_count, int pov_count) {
	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Joystick %s #%08x with %d buttons and %d axes detected", name, index, button_count, axis_count + 2 * pov_count);
	joystick_private_data *private_data = indigo_safe_malloc(sizeof(joystick_private_data));
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
	snprintf(device->name, INDIGO_NAME_SIZE, "%s #%08lx", name, index);
	private_data->index = index;
	private_data->button_count = button_count;
	private_data->axis_count = axis_count;
	private_data->pov_count = pov_count;
	device->private_data = private_data;
	indigo_attach_device(device);
	return device;
}

static void event_axis(indigo_device *device, int axis, int value) {
	//printf("AXIS %d -> %d\n", axis, value);
	JOYSTICK_AXES_PROPERTY->items[axis].number.value = value;
	JOYSTICK_AXES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, JOYSTICK_AXES_PROPERTY, NULL);
	axis++;
	value = value / 10000;
	if (value > 4)
		value = 4;
	else if (value < -4)
		value = -4;
	if (JOYSTICK_MAPPING_MOTION_DEC_ITEM->number.value == axis) {
		if (JOYSTICK_OPTIONS_ANALOG_STICK_ITEM->sw.value) {
			if (value != 0 && abs(value) > PRIVATE_DATA->ra_slew_rate) {
        indigo_item *item = MOUNT_SLEW_RATE_PROPERTY->items + abs(value) - 1;
        if (!item->sw.value) {
          indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, item, true);
          MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
          indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
        }
			}
		}
		PRIVATE_DATA->dec_slew_rate = abs(value);
    if (value < 0) {
			indigo_item *item = JOYSTICK_OPTIONS_SWAP_DEC_ITEM->sw.value ? MOUNT_MOTION_SOUTH_ITEM : MOUNT_MOTION_NORTH_ITEM;
      if (!item->sw.value) {
        item->sw.value = true;
        MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
        indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
      }
    } else  if (value > 0) {
      indigo_item *item = JOYSTICK_OPTIONS_SWAP_DEC_ITEM->sw.value ? MOUNT_MOTION_NORTH_ITEM : MOUNT_MOTION_SOUTH_ITEM;
      if (!item->sw.value) {
        item->sw.value = true;
        MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
        indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
      }
    } else {
      if (MOUNT_MOTION_NORTH_ITEM->sw.value || MOUNT_MOTION_SOUTH_ITEM->sw.value) {
        MOUNT_MOTION_NORTH_ITEM->sw.value = false;
        MOUNT_MOTION_SOUTH_ITEM->sw.value = false;
        MOUNT_MOTION_DEC_PROPERTY->state = INDIGO_OK_STATE;
        indigo_update_property(device, MOUNT_MOTION_DEC_PROPERTY, NULL);
      }
    }
	} else if (JOYSTICK_MAPPING_MOTION_RA_ITEM->number.value == axis) {
		if (JOYSTICK_OPTIONS_ANALOG_STICK_ITEM->sw.value) {
			if (value != 0 && abs(value) > PRIVATE_DATA->dec_slew_rate) {
				indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_PROPERTY->items + abs(value) - 1, true);
				MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
			}
		}
		PRIVATE_DATA->ra_slew_rate = abs(value);
    if (value < 0) {
			indigo_item *item = JOYSTICK_OPTIONS_SWAP_RA_ITEM->sw.value ? MOUNT_MOTION_EAST_ITEM : MOUNT_MOTION_WEST_ITEM;
      if (!item->sw.value) {
        item->sw.value = true;
        MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
        indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
      }
    } else  if (value > 0) {
			indigo_item *item = JOYSTICK_OPTIONS_SWAP_RA_ITEM->sw.value ? MOUNT_MOTION_WEST_ITEM : MOUNT_MOTION_EAST_ITEM;
      if (!item->sw.value) {
        item->sw.value = true;
        MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
        indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
      }
    } else {
      if (MOUNT_MOTION_EAST_ITEM->sw.value || MOUNT_MOTION_WEST_ITEM->sw.value) {
        MOUNT_MOTION_EAST_ITEM->sw.value = false;
        MOUNT_MOTION_WEST_ITEM->sw.value = false;
        MOUNT_MOTION_RA_PROPERTY->state = INDIGO_OK_STATE;
        indigo_update_property(device, MOUNT_MOTION_RA_PROPERTY, NULL);
      }
    }
	}
}

static void event_pov(indigo_device *device, int pov, int value) {
	//printf("POV %d -> %d\n", pov, value);
	switch (value) {
		case 0:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, 0);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, -65536);
			break;
		case 4500:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, 65536);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, -65536);
			break;
		case 9000:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, 65536);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, 0);
			break;
		case 13500:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, 65536);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, 65536);
			break;
		case 18000:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, 0);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, 65536);
			break;
		case 22500:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, -65536);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, 65536);
			break;
		case 27000:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, -65536);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, 0);
			break;
		case 31500:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, -65536);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, -65536);
			break;
		default:
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov, 0);
			event_axis(device, PRIVATE_DATA->axis_count + 2 * pov + 1, 0);
			break;
	}
}

static void event_button(indigo_device *device, int button, bool value) {
	//printf("BUTTON %d -> %d\n", button, value);
	JOYSTICK_BUTTONS_PROPERTY->items[button].sw.value = value;
	JOYSTICK_BUTTONS_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, JOYSTICK_BUTTONS_PROPERTY, NULL);
	button++;
	if (JOYSTICK_MAPPING_PARKED_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_PARKED_ITEM, true);
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_UNPARKED_ITEM->number.value == button && value) {
		MOUNT_PARK_PROPERTY->state = INDIGO_OK_STATE;
		indigo_set_switch(MOUNT_PARK_PROPERTY, MOUNT_PARK_UNPARKED_ITEM, true);
		indigo_update_property(device, MOUNT_PARK_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_ABORT_ITEM->number.value == button) {
		indigo_set_switch(MOUNT_ABORT_MOTION_PROPERTY, MOUNT_ABORT_MOTION_ITEM, value);
		MOUNT_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_ABORT_MOTION_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_TRACKING_ON_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_ON_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_TRACKING_OFF_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_TRACKING_PROPERTY, MOUNT_TRACKING_OFF_ITEM, true);
		MOUNT_TRACKING_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_TRACKING_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_RATE_GUIDE_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_GUIDE_ITEM, true);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_RATE_CENTERING_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_CENTERING_ITEM, true);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_RATE_FIND_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_FIND_ITEM, true);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
	} else  if (JOYSTICK_MAPPING_RATE_MAX_ITEM->number.value == button && value) {
		indigo_set_switch(MOUNT_SLEW_RATE_PROPERTY, MOUNT_SLEW_RATE_MAX_ITEM, true);
		MOUNT_SLEW_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, MOUNT_SLEW_RATE_PROPERTY, NULL);
	}
}

static void release_device(indigo_device *device) {
	indigo_detach_device(device);
	free(PRIVATE_DATA);
	free(device);
}

#ifdef INDIGO_MACOS

// -------------------------------------------------------------------------------- DDHidLib wrapper

static NSMutableArray *wrappers = nil;

@interface DDHidJoystickWrapper: NSObject
@end

@implementation DDHidJoystickWrapper {
	indigo_device *device;
	DDHidJoystick *joystick;
	dispatch_queue_global_t queue;
}

+(void)rescan {
	if (wrappers == nil) {
		wrappers = [NSMutableArray array];
	}
	NSMutableArray *tmp = [NSMutableArray array];
  __block NSArray *allJoysticks;
  if ([NSThread isMainThread]) {
    allJoysticks = DDHidJoystick.allJoysticks;
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      allJoysticks = DDHidJoystick.allJoysticks;
    });
  }
	for (DDHidJoystick *joystick in allJoysticks) {
		bool found = false;
		for (DDHidJoystickWrapper *wrapper in wrappers) {
			if (joystick.locationId == wrapper->joystick.locationId) {
				[tmp addObject:wrapper];
				[wrappers removeObject:wrapper];
				found = true;
				break;
			}
		}
		if (!found) {
			DDHidJoystickWrapper *wrapper = [[DDHidJoystickWrapper alloc] init];
			
			int axis_count = 0;
			int pov_count = 0;
			if (joystick.countOfSticks > 0) {
				DDHidJoystickStick *stick = [joystick objectInSticksAtIndex:0];
				if (stick.xAxisElement)
					axis_count++;
				if (stick.yAxisElement)
					axis_count++;
				axis_count += stick.countOfStickElements;
				pov_count = stick.countOfPovElements;
			}
			
			wrapper->device = allocate_device([joystick.productName cStringUsingEncoding:NSASCIIStringEncoding], joystick.locationId, joystick.numberOfButtons, axis_count, pov_count);
			wrapper->joystick = joystick;
			wrapper->queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
			[joystick setDelegate:wrapper];
      dispatch_async(dispatch_get_main_queue(), ^{
        [joystick startListening];
      });
			[tmp addObject:wrapper];
		}
	}
	for (DDHidJoystickWrapper *wrapper in wrappers) {
    dispatch_async(wrapper->queue, ^{
      release_device(wrapper->device);
    });
	}
	wrappers = tmp;
}

+(void)shutdown {
	for (DDHidJoystickWrapper *wrapper in wrappers) {
    dispatch_async(wrapper->queue, ^{
      release_device(wrapper->device);
    });
	}
}

+(void)startDevice:(indigo_device *)device {
	for (DDHidJoystickWrapper *wrapper in wrappers) {
		if (wrapper->device == device) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [wrapper->joystick startListening];
      });
		}
	}
}

+(void)stopDevice:(indigo_device *)device {
	for (DDHidJoystickWrapper *wrapper in wrappers) {
		if (wrapper->device == device) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [wrapper->joystick stopListening];
      });
		}
	}
}

-(id)initWidth:(void *)device {
	self = [super init];
	if (self) {
		self->device = device;
		return self;
	}
	return nil;
}

-(void)ddhidJoystick: (DDHidJoystick *) joystick stick: (unsigned) stick xChanged: (int) value {
	indigo_device *device = self->device;
	if (IS_CONNECTED)
    dispatch_async(queue, ^{
      event_axis(device, 0, value);
    });
}

-(void)ddhidJoystick: (DDHidJoystick *) joystick stick: (unsigned) stick yChanged: (int) value {
	indigo_device *device = self->device;
	if (IS_CONNECTED)
    dispatch_async(queue, ^{
      event_axis(device, 1, value);
    });
}

-(void)ddhidJoystick: (DDHidJoystick *) joystick stick: (unsigned) stick otherAxis: (unsigned) otherAxis valueChanged: (int) value {
	indigo_device *device = self->device;
	if (IS_CONNECTED)
    dispatch_async(queue, ^{
      event_axis(device, otherAxis + 2, value);
    });
}

-(void)ddhidJoystick: (DDHidJoystick *) joystick stick: (unsigned) stick povNumber: (unsigned) povNumber valueChanged: (int) value {
	indigo_device *device = self->device;
	if (IS_CONNECTED)
    dispatch_async(queue, ^{
      event_pov(device, povNumber, value);
    });
}
-(void)ddhidJoystick: (DDHidJoystick *) joystick buttonDown: (unsigned) buttonNumber {
	indigo_device *device = self->device;
	if (IS_CONNECTED)
    dispatch_async(queue, ^{
      event_button(device, buttonNumber, 1);
    });
}
-(void)ddhidJoystick: (DDHidJoystick *) joystick buttonUp: (unsigned) buttonNumber {
	indigo_device *device = self->device;
	if (IS_CONNECTED)
    dispatch_async(queue, ^{
      event_button(device, buttonNumber, 0);
    });
}

@end

static bool open_joystick(indigo_device *device) {
	[DDHidJoystickWrapper startDevice:device];
	return true;
}

static void close_joystick(indigo_device *device) {
	[DDHidJoystickWrapper stopDevice:device];
}

#endif

#ifdef INDIGO_LINUX

// -------------------------------------------------------------------------------- Linux wrapper

#define MAX_DEVICES		5

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *poll(indigo_device *device) {
	int index = PRIVATE_DATA->index;
	int joy_fd;
	char path[128];
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Joystick #%ld poll thread started", index);
	sprintf(path, "/dev/input/js%ld", PRIVATE_DATA->index);
	if ((joy_fd = open(path, O_RDONLY)) == -1) {
		PRIVATE_DATA->fd = 0;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't access %s (%s)", path, strerror(errno));
	} else {
		struct js_event js;
		PRIVATE_DATA->fd = joy_fd;
		fcntl(joy_fd, F_SETFL, O_NONBLOCK);
		while (joy_fd) {
			if (read(joy_fd, &js, sizeof(struct js_event)) > 0) {
				pthread_mutex_lock(&mutex);
				if (devices[index]) {
					switch (js.type & ~JS_EVENT_INIT) {
						case JS_EVENT_AXIS: {
							if (js.number < MAX_AXES && PRIVATE_DATA->last_axis_value[js.number] != js.value) {
								event_axis(device, js.number, 2 * js.value);
								PRIVATE_DATA->last_axis_value[js.number] = js.value;
							}
							break;
						}
						case JS_EVENT_BUTTON: {
							if (js.number < MAX_BUTTONS && PRIVATE_DATA->last_button_state[js.number] != js.value) {
								event_button(device, js.number, js.value);
								PRIVATE_DATA->last_button_state[js.number] = js.value;
							}
							break;
						}
					}
				} else {
					joy_fd = 0;
				}
				pthread_mutex_unlock(&mutex);
			} else {
				if (errno == EBADF) {
					joy_fd = 0;
				} else {
					indigo_usleep(100000);
				}
			}
		}
	}
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Joystick #%ld poll thread finished", index);
}

static void rescan() {
	DIR *dev_input = opendir("/dev/input");
	if (dev_input == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No folder /dev/input");
		return;
	}
	struct dirent * dir;
	bool found[MAX_DEVICES];
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < MAX_DEVICES; i++)
		found[i] = false;
	while ((dir = readdir(dev_input)) != NULL) {
		int index = 0;
		if (sscanf(dir->d_name, "js%d", &index) == 1) {
			found[index] = true;
			if (devices[index])
				continue;
			int joy_fd, axis_count=0, button_count=0;
			char name[512];
			memset(name, 0, sizeof(name));
			snprintf(name, sizeof(name), "/dev/input/%s", dir->d_name);
			if ((joy_fd = open(name, O_RDONLY)) == -1) {
				pthread_mutex_unlock(&mutex);
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't access %s (%s)", name, strerror(errno));
				return;
			}
			ioctl(joy_fd, JSIOCGAXES, &axis_count);
			ioctl(joy_fd, JSIOCGBUTTONS, &button_count);
			ioctl(joy_fd, JSIOCGNAME(80), &name);
			close(joy_fd);
			devices[index] = allocate_device(name, index, button_count, axis_count, 0);
		}
	}
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] && !found[i]) {
			release_device(devices[i]);
			devices[i] = NULL;
		}
	}
	pthread_mutex_unlock(&mutex);
}

static void shutdown() {
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < MAX_DEVICES; i++) {
		if (devices[i]) {
			release_device(devices[i]);
			devices[i] = NULL;
		}
	}
	pthread_mutex_unlock(&mutex);
}

static bool open_joystick(indigo_device *device) {
	indigo_async((void *(*)(void *))poll, device);
	return true;
}

static void close_joystick(indigo_device *device) {
	if (PRIVATE_DATA->fd) {
		close(PRIVATE_DATA->fd);
		PRIVATE_DATA->fd = 0;
	}
}

#endif

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	indigo_usleep(500000);
#ifdef INDIGO_MACOS
  [DDHidJoystickWrapper rescan];
#endif
#ifdef INDIGO_LINUX
	rescan();
#endif
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_aux_joystick(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "HID Joystick", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
#ifdef INDIGO_MACOS
		[DDHidJoystickWrapper rescan];
#endif
#ifdef INDIGO_LINUX
			rescan();
#endif
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_NO_FLAGS, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
#ifdef INDIGO_LINUX
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
#endif
#ifdef INDIGO_MACOS
		//TBD
#endif
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
#ifdef INDIGO_MACOS
		[DDHidJoystickWrapper shutdown];
#endif
#ifdef INDIGO_LINUX
			shutdown();
#endif
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
