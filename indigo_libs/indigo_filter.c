// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO Filter agent base
 \file indigo_filter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include <indigo/indigo_filter.h>

static int interface_mask[INDIGO_FILTER_LIST_COUNT] = { INDIGO_INTERFACE_CCD, INDIGO_INTERFACE_WHEEL, INDIGO_INTERFACE_FOCUSER, INDIGO_INTERFACE_ROTATOR, INDIGO_INTERFACE_MOUNT, INDIGO_INTERFACE_GUIDER, INDIGO_INTERFACE_DOME, INDIGO_INTERFACE_GPS, INDIGO_INTERFACE_AUX_JOYSTICK, INDIGO_INTERFACE_AUX, INDIGO_INTERFACE_AUX, INDIGO_INTERFACE_AUX, INDIGO_INTERFACE_AUX };
static char *property_name_prefix[INDIGO_FILTER_LIST_COUNT] = { "CCD_", "WHEEL_", "FOCUSER_", "ROTATOR_", "MOUNT_", "GUIDER_", "DOME_", "GPS_", "JOYSTICK_", "AUX_1_", "AUX_2_", "AUX_3_", "AUX_4_" };
static int property_name_prefix_len[INDIGO_FILTER_LIST_COUNT] = { 4, 6, 8, 8, 6, 7, 5, 4, 9, 6, 6, 6, 6 };
static char *property_name_label[INDIGO_FILTER_LIST_COUNT] = { "Camera ", "Filter Wheel ", "Focuser ", "Rotator ", "Mount ", "Guider ", "Dome ", "GPS ", "Joystick ", "AUX #1 ", "AUX #2 ", "AUX #3 ", "AUX #4 " };

indigo_result indigo_filter_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface) {
	assert(device != NULL);
	if (FILTER_DEVICE_CONTEXT == NULL) {
		device->device_context = indigo_safe_malloc(sizeof(indigo_filter_context));
	}
	FILTER_DEVICE_CONTEXT->device = device;
	if (FILTER_DEVICE_CONTEXT != NULL) {
		if (indigo_device_attach(device, driver_name, version, INDIGO_INTERFACE_AGENT | device_interface) == INDIGO_OK) {
			CONNECTION_PROPERTY->hidden = true;
			// -------------------------------------------------------------------------------- CCD property
			FILTER_CCD_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_CCD_LIST_PROPERTY_NAME, MAIN_GROUP, "Camera list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_CCD_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_CCD_LIST_PROPERTY->hidden = true;
			FILTER_CCD_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_CCD_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No camera", true);
			// -------------------------------------------------------------------------------- wheel property
			FILTER_WHEEL_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_WHEEL_LIST_PROPERTY_NAME, MAIN_GROUP, "Filter wheel list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_WHEEL_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_WHEEL_LIST_PROPERTY->hidden = true;
			FILTER_WHEEL_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_WHEEL_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No wheel", true);
			// -------------------------------------------------------------------------------- focuser property
			FILTER_FOCUSER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_FOCUSER_LIST_PROPERTY_NAME, MAIN_GROUP, "Focuser list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_FOCUSER_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_FOCUSER_LIST_PROPERTY->hidden = true;
			FILTER_FOCUSER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_FOCUSER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No focuser", true);
			// -------------------------------------------------------------------------------- rotator property
			FILTER_ROTATOR_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_ROTATOR_LIST_PROPERTY_NAME, MAIN_GROUP, "Rotator list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_ROTATOR_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_ROTATOR_LIST_PROPERTY->hidden = true;
			FILTER_ROTATOR_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_ROTATOR_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No rotator", true);
			// -------------------------------------------------------------------------------- mount property
			FILTER_MOUNT_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_MOUNT_LIST_PROPERTY_NAME, MAIN_GROUP, "Mount list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_MOUNT_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_MOUNT_LIST_PROPERTY->hidden = true;
			FILTER_MOUNT_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_MOUNT_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No mount", true);
			// -------------------------------------------------------------------------------- guider property
			FILTER_GUIDER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_GUIDER_LIST_PROPERTY_NAME, MAIN_GROUP, "Guider list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_GUIDER_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_GUIDER_LIST_PROPERTY->hidden = true;
			FILTER_GUIDER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_GUIDER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No guider", true);
			// -------------------------------------------------------------------------------- dome property
			FILTER_DOME_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_DOME_LIST_PROPERTY_NAME, MAIN_GROUP, "Dome list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_DOME_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_DOME_LIST_PROPERTY->hidden = true;
			FILTER_DOME_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_DOME_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No dome", true);
			// -------------------------------------------------------------------------------- GPS property
			FILTER_GPS_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_GPS_LIST_PROPERTY_NAME, MAIN_GROUP, "GPS list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_GPS_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_GPS_LIST_PROPERTY->hidden = true;
			FILTER_GPS_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_GPS_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No GPS", true);
			// -------------------------------------------------------------------------------- Joystick property
			FILTER_JOYSTICK_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_JOYSTICK_LIST_PROPERTY_NAME, MAIN_GROUP, "Joystick list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_JOYSTICK_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_JOYSTICK_LIST_PROPERTY->hidden = true;
			FILTER_JOYSTICK_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_JOYSTICK_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No joystick", true);
			// -------------------------------------------------------------------------------- AUX #1 property
			FILTER_AUX_1_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_1_LIST_PROPERTY_NAME, MAIN_GROUP, "AUX #1 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_1_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_AUX_1_LIST_PROPERTY->hidden = true;
			FILTER_AUX_1_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_1_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #1", true);
			// -------------------------------------------------------------------------------- AUX #2 property
			FILTER_AUX_2_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_2_LIST_PROPERTY_NAME, MAIN_GROUP, "AUX #2 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_2_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_AUX_2_LIST_PROPERTY->hidden = true;
			FILTER_AUX_2_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_2_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #2", true);
			// -------------------------------------------------------------------------------- AUX #3 property
			FILTER_AUX_3_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_3_LIST_PROPERTY_NAME, MAIN_GROUP, "AUX #3 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_3_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_AUX_3_LIST_PROPERTY->hidden = true;
			FILTER_AUX_3_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_3_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #3", true);
			// -------------------------------------------------------------------------------- AUX #4 property
			FILTER_AUX_4_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_AUX_4_LIST_PROPERTY_NAME, MAIN_GROUP, "AUX #4 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_AUX_4_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_AUX_4_LIST_PROPERTY->hidden = true;
			FILTER_AUX_4_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_AUX_4_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #4", true);
			// -------------------------------------------------------------------------------- Related CCD property
			FILTER_RELATED_CCD_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_CCD_LIST_PROPERTY_NAME, MAIN_GROUP, "Related CCD list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_CCD_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_CCD_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_CCD_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_CCD_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No camera", true);
			// -------------------------------------------------------------------------------- Related wheel property
			FILTER_RELATED_WHEEL_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_WHEEL_LIST_PROPERTY_NAME, MAIN_GROUP, "Related wheel list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_WHEEL_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_WHEEL_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_WHEEL_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_WHEEL_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No wheel", true);
			// -------------------------------------------------------------------------------- Related focuser property
			FILTER_RELATED_FOCUSER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_FOCUSER_LIST_PROPERTY_NAME, MAIN_GROUP, "Related focuser list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_FOCUSER_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_FOCUSER_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_FOCUSER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_FOCUSER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No focuser", true);
			// -------------------------------------------------------------------------------- Related rotator property
			FILTER_RELATED_ROTATOR_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_ROTATOR_LIST_PROPERTY_NAME, MAIN_GROUP, "Related rotator list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_ROTATOR_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_ROTATOR_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_ROTATOR_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_ROTATOR_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No rotator", true);
			// -------------------------------------------------------------------------------- Related mount property
			FILTER_RELATED_MOUNT_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_MOUNT_LIST_PROPERTY_NAME, MAIN_GROUP, "Related mount list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_MOUNT_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_MOUNT_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_MOUNT_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_MOUNT_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No mount", true);
			// -------------------------------------------------------------------------------- Related guider property
			FILTER_RELATED_GUIDER_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_GUIDER_LIST_PROPERTY_NAME, MAIN_GROUP, "Related guider list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_GUIDER_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_GUIDER_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_GUIDER_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_GUIDER_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No guider", true);
			// -------------------------------------------------------------------------------- Related dome property
			FILTER_RELATED_DOME_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_DOME_LIST_PROPERTY_NAME, MAIN_GROUP, "Related dome list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_DOME_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_DOME_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_DOME_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_DOME_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No dome", true);
			// -------------------------------------------------------------------------------- Related GPS property
			FILTER_RELATED_GPS_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_GPS_LIST_PROPERTY_NAME, MAIN_GROUP, "Related GPS list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_GPS_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_GPS_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_GPS_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_GPS_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No GPS", true);
			// -------------------------------------------------------------------------------- Related joystick property
			FILTER_RELATED_JOYSTICK_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_JOYSTICK_LIST_PROPERTY_NAME, MAIN_GROUP, "Related joystick", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_JOYSTICK_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_JOYSTICK_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_JOYSTICK_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_JOYSTICK_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No joystick", true);
			// -------------------------------------------------------------------------------- Related AUX #1 property
			FILTER_RELATED_AUX_1_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_1_LIST_PROPERTY_NAME, MAIN_GROUP, "Related AUX #1 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_1_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_AUX_1_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_1_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_1_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #1", true);
			// -------------------------------------------------------------------------------- Related AUX #2 property
			FILTER_RELATED_AUX_2_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_2_LIST_PROPERTY_NAME, MAIN_GROUP, "Related AUX #2 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_2_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_AUX_2_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_2_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_2_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #2", true);
			// -------------------------------------------------------------------------------- Related AUX #3 property
			FILTER_RELATED_AUX_3_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_3_LIST_PROPERTY_NAME, MAIN_GROUP, "Related AUX #3 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_3_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_AUX_3_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_3_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_3_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #3", true);
			// -------------------------------------------------------------------------------- Related AUX #4 property
			FILTER_RELATED_AUX_4_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AUX_4_LIST_PROPERTY_NAME, MAIN_GROUP, "Related AUX #4 list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AUX_4_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_AUX_4_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AUX_4_LIST_PROPERTY->count = 1;
			indigo_init_switch_item(FILTER_RELATED_AUX_4_LIST_PROPERTY->items, FILTER_DEVICE_LIST_NONE_ITEM_NAME, "No AUX device #4", true);
			// -------------------------------------------------------------------------------- Related agents property
			FILTER_RELATED_AGENT_LIST_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, MAIN_GROUP, "Related agent list", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, INDIGO_FILTER_MAX_DEVICES);
			if (FILTER_RELATED_AGENT_LIST_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			FILTER_RELATED_AGENT_LIST_PROPERTY->hidden = true;
			FILTER_RELATED_AGENT_LIST_PROPERTY->count = 0;
			// -------------------------------------------------------------------------------- FILTER_FORCE_SYMMETRIC_RELATIONS
			FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY = indigo_init_switch_property(NULL, device->name, FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY_NAME, MAIN_GROUP, "Force symmetric relations", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			if (FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_switch_item(FILTER_FORCE_SYMMETRIC_RELATIONS_ENABLED_ITEM, FILTER_FORCE_SYMMETRIC_RELATIONS_ENABLED_ITEM_NAME, "Enable", true);
			indigo_init_switch_item(FILTER_FORCE_SYMMETRIC_RELATIONS_DISABLED_ITEM, FILTER_FORCE_SYMMETRIC_RELATIONS_DISABLED_ITEM_NAME, "Disable", false);
			// -------------------------------------------------------------------------------- CCD_LENS_FOV
			CCD_LENS_FOV_PROPERTY = indigo_init_number_property(NULL, device->name, CCD_LENS_FOV_PROPERTY_NAME, "Camera", "FOV and pixel scale", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 4);
			if (CCD_LENS_FOV_PROPERTY == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_number_item(CCD_LENS_FOV_FOV_WIDTH_ITEM, CCD_LENS_FOV_FOV_WIDTH_ITEM_NAME, "FOV width (°)", 0, 180, 0, 0);
			indigo_init_number_item(CCD_LENS_FOV_FOV_HEIGHT_ITEM, CCD_LENS_FOV_FOV_HEIGHT_ITEM_NAME, "FOV height (°)", 0, 180, 0, 0);
			indigo_init_number_item(CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM, CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM_NAME, "Pixel scale width (°/px)", 0, 5, 0, 0);
			indigo_init_number_item(CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM, CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM_NAME, "Pixel scale height (°/px)", 0, 5, 0, 0);
			strcpy(CCD_LENS_FOV_FOV_WIDTH_ITEM->number.format, "%m");
			strcpy(CCD_LENS_FOV_FOV_HEIGHT_ITEM->number.format, "%m");
			strcpy(CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.format, "%.10m");
			strcpy(CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.format, "%.10m");
			CCD_LENS_FOV_PROPERTY->hidden = true;
			// --------------------------------------------------------------------------------
			CONFIG_PROPERTY->hidden = true;
			PROFILE_PROPERTY->hidden = true;
			return INDIGO_OK;
		}
	}
	return INDIGO_FAILED;
}

indigo_result indigo_filter_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_property *device_list = FILTER_DEVICE_CONTEXT->filter_device_list_properties[i];
		indigo_define_matching_property(device_list);
		device_list = FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[i];
		indigo_define_matching_property(device_list);
	}
	if (indigo_property_match(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property, property))
		indigo_define_property(device, FILTER_DEVICE_CONTEXT->filter_related_agent_list_property, NULL);
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		indigo_property *cached_property = FILTER_DEVICE_CONTEXT->agent_property_cache[i];
		if (cached_property && indigo_property_match(cached_property, property))
			indigo_define_property(device, cached_property, NULL);
	}
	if (indigo_property_match(FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY, property)) {
		FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY->hidden = FILTER_RELATED_AGENT_LIST_PROPERTY->hidden;
		indigo_define_property(device, FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY, NULL);
	}
	indigo_define_matching_property(CCD_LENS_FOV_PROPERTY);
	return indigo_device_enumerate_properties(device, client, property);
}

static void update_additional_instances(indigo_device *device) {
	int count = (int)ADDITIONAL_INSTANCES_COUNT_ITEM->number.value;
	for (int i = 0; i < count; i++) {
		if (DEVICE_CONTEXT->additional_device_instances[i] == NULL) {
			indigo_device *additional_device = indigo_safe_malloc_copy(sizeof(indigo_device), device);
			snprintf(additional_device->name, INDIGO_NAME_SIZE, "%s #%d", device->name, i + 2);
			additional_device->lock = NULL;
			additional_device->is_remote = false;
			additional_device->gp_bits = 0;
			additional_device->master_device = NULL;
			additional_device->last_result = 0;
			additional_device->access_token = 0;
			additional_device->device_context = indigo_safe_malloc(MALLOCED_SIZE(device->device_context));
			((indigo_device_context *)additional_device->device_context)->base_device = device;
			additional_device->private_data = indigo_safe_malloc(MALLOCED_SIZE(device->private_data));
			indigo_attach_device(additional_device);
			DEVICE_CONTEXT->additional_device_instances[i] = additional_device;
			indigo_client *agent_client = FILTER_DEVICE_CONTEXT->client;
			indigo_client *additional_client = indigo_safe_malloc_copy(sizeof(indigo_client), agent_client);
			snprintf(additional_client->name, INDIGO_NAME_SIZE, "%s #%d", agent_client->name, i + 2);
			additional_client->is_remote = false;
			additional_client->last_result = 0;
			additional_client->enable_blob_mode_records = NULL;
			additional_client->client_context = additional_device->device_context;
			indigo_attach_client(additional_client);
			FILTER_DEVICE_CONTEXT->additional_client_instances[i] = additional_client;
		}
	}
	for (int i = count; i < MAX_ADDITIONAL_INSTANCES; i++) {
		indigo_device *additional_device = DEVICE_CONTEXT->additional_device_instances[i];
		if (additional_device != NULL) {
			indigo_client *additional_client = FILTER_DEVICE_CONTEXT->additional_client_instances[i];
			indigo_detach_client(additional_client);
			free(additional_client);
			indigo_detach_device(additional_device);
			free(additional_device->private_data);
			free(additional_device);
			DEVICE_CONTEXT->additional_device_instances[i] = NULL;
		}
	}
	ADDITIONAL_INSTANCES_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ADDITIONAL_INSTANCES_PROPERTY, NULL);
}

static void add_cached_connection_property(indigo_device *device, indigo_property *property) {
	int free_index = -1;
	for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
		indigo_property *cached_property = FILTER_DEVICE_CONTEXT->connection_property_cache[i];
		if (cached_property == property) {
			return;
		} else if (cached_property == NULL) {
			free_index = i;
			break;
		}
	}
	if (free_index == -1) {
		indigo_error("[%s:%d] Max cached device count reached", __FUNCTION__, __LINE__);
	} else {
		FILTER_DEVICE_CONTEXT->connection_property_cache[free_index] = property;
	}
}

static void remove_cached_connection_property(indigo_device *device, indigo_property *property) {
	if (*property->name) {
		for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
			if (FILTER_DEVICE_CONTEXT->connection_property_cache[i] == property) {
				FILTER_DEVICE_CONTEXT->connection_property_cache[i] = NULL;
				break;
			}
		}
	} else {
		for (int i = 0; i < INDIGO_FILTER_MAX_DEVICES; i++) {
			if (FILTER_DEVICE_CONTEXT->connection_property_cache[i] && !strcmp(FILTER_DEVICE_CONTEXT->connection_property_cache[i]->device, property->device)) {
				FILTER_DEVICE_CONTEXT->connection_property_cache[i] = NULL;
				break;
			}
		}
	}
}

static bool device_is_available(indigo_device *device, char* name) {
	bool available = true;
	for (int j = 0; j < INDIGO_FILTER_MAX_DEVICES; j++) {
		indigo_property *cached_connection_property = FILTER_DEVICE_CONTEXT->connection_property_cache[j];
		if (cached_connection_property != NULL && !strcmp(cached_connection_property->device, name)) {
			available = cached_connection_property->state == INDIGO_OK_STATE || cached_connection_property->state == INDIGO_ALERT_STATE;
			if (available) {
				available = false;
				for (int k = 0; k < cached_connection_property->count; k++) {
					indigo_item *item = cached_connection_property->items + k;
					if (!strcmp(item->name, CONNECTION_DISCONNECTED_ITEM_NAME) && item->sw.value) {
						available = true;
						break;
					}
				}
			}
			break;
		}
	}
	return available;
}

static indigo_result update_device_list(indigo_device *device, indigo_client *client, indigo_property *device_list, indigo_property *property, char *device_name) {
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value) {
			for (int j = 0; j < device_list->count; j++) {
				if (device_list->items[j].sw.value) {
					if (!strcmp(property->items[i].name, device_list->items[j].name))
						return INDIGO_OK;
					break;
				}
			}
		}
	}
	device_list->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, device_list, NULL);
	*device_name = 0;
	indigo_property *connection_property = indigo_init_switch_property(NULL, "", CONNECTION_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
	for (int i = 1; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			device_list->items[i].sw.value = false;
			strcpy(connection_property->device, device_list->items[i].name);
			indigo_property **device_cache = FILTER_DEVICE_CONTEXT->device_property_cache;
			indigo_property **agent_cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
			for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
				indigo_property *device_property = device_cache[i];
				if (device_property && !strcmp(connection_property->device, device_property->device)) {
					indigo_release_property(device_property);
					device_cache[i] = NULL;
					if (agent_cache[i]) {
						indigo_delete_property(device, agent_cache[i], NULL);
						indigo_release_property(agent_cache[i]);
						agent_cache[i] = NULL;
					}
				}
			}
			if (!CCD_LENS_FOV_PROPERTY->hidden) {
				indigo_delete_property(device, CCD_LENS_FOV_PROPERTY, NULL);
				CCD_LENS_FOV_PROPERTY->hidden = true;
			}
			indigo_init_switch_item(connection_property->items, CONNECTION_DISCONNECTED_ITEM_NAME, NULL, true);
			connection_property->access_token = indigo_get_device_or_master_token(connection_property->device);
			indigo_change_property(client, connection_property);
			break;
		}
	}
	indigo_property_copy_values(device_list, property, false);
	for (int i = 1; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			char *name = device_list->items[i].name;
			if (device_is_available(device, name)) {
				device_list->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, device_list, NULL);
				strcpy(connection_property->device, name);
				indigo_init_switch_item(connection_property->items, CONNECTION_CONNECTED_ITEM_NAME, NULL, true);
				indigo_enumerate_properties(client, connection_property);
				connection_property->access_token = indigo_get_device_or_master_token(connection_property->device);
				indigo_change_property(client, connection_property);
			} else {
				indigo_set_switch(device_list, device_list->items, true);
				device_list->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, device_list, "'%s' is busy or in use and can not be selected.", name);
			}
			indigo_release_property(connection_property);
			return INDIGO_OK;
		}
	}
	device_list->state = INDIGO_OK_STATE;
	indigo_update_property(device, device_list, NULL);
	indigo_release_property(connection_property);
	return INDIGO_OK;
}

static indigo_result update_related_device_list(indigo_device *device, indigo_property *device_list, indigo_property *property) {
	indigo_property_copy_values(device_list, property, false);
	for (int i = 1; i < device_list->count; i++) {
		if (device_list->items[i].sw.value) {
			device_list->state = INDIGO_OK_STATE;
			indigo_property all_properties;
			memset(&all_properties, 0, sizeof(all_properties));
			strcpy(all_properties.device, device_list->items[i].name);
			indigo_enumerate_properties(FILTER_DEVICE_CONTEXT->client, &all_properties);
			indigo_update_property(device, device_list, NULL);
			return INDIGO_OK;
		}
	}
	indigo_update_property(device, device_list, NULL);
	return INDIGO_OK;
}

static void set_reverse_relation(indigo_device *device, void *data) {
	indigo_item *item = (indigo_item *)data;
	if (FILTER_FORCE_SYMMETRIC_RELATIONS_ENABLED_ITEM->sw.value) {
		char reverse_item_name[INDIGO_NAME_SIZE];
		strcpy(reverse_item_name, device->name);
		if (strchr(item->name, '@')) {
			snprintf(reverse_item_name, sizeof(reverse_item_name), "%s @ %s", device->name, indigo_local_service_name);
		} else {
			indigo_copy_name(reverse_item_name, device->name);
		}
		indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, item->name, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, reverse_item_name, item->sw.value);
	}
	indigo_property all_properties;
	memset(&all_properties, 0, sizeof(all_properties));
	strcpy(all_properties.device, item->name);
	indigo_enumerate_properties(FILTER_DEVICE_CONTEXT->client, &all_properties);
}

static indigo_result update_related_agent_list(indigo_device *device, indigo_property *property) {
	indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	bool is_imager_agent = !strncmp(device->name, "Imager Agent", 12);
	for (int i = 0; i < property->count; i++) {
		indigo_item *remote_item = property->items + i;
		for (int j = 0; j < related_agents_property->count; j++) {
			indigo_item *local_item = related_agents_property->items + j;
			if (!strcmp(remote_item->name, local_item->name)) {
				if (remote_item->sw.value == local_item->sw.value) {
					break;
				}
				local_item->sw.value = remote_item->sw.value;
				if (!is_imager_agent || strncmp(local_item->name, "Imager Agent", 12)) {
					indigo_set_timer_with_data(device, 0, set_reverse_relation, NULL, local_item);
				}
				break;
			}
		}
	}
	related_agents_property->state = INDIGO_OK_STATE;
	indigo_update_property(device, related_agents_property, NULL);
	return INDIGO_OK;
}


indigo_result indigo_filter_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_property *device_list = FILTER_DEVICE_CONTEXT->filter_device_list_properties[i];
		if (indigo_property_match(device_list, property)) {
			if (FILTER_DEVICE_CONTEXT->running_process || device_list->state == INDIGO_BUSY_STATE) {
				indigo_update_property(device, device_list, "You can't change selection now!");
				return INDIGO_OK;
			}
			return update_device_list(device, FILTER_DEVICE_CONTEXT->client, device_list, property, FILTER_DEVICE_CONTEXT->device_name[i]);
		}
		device_list = FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[i];
		if (indigo_property_match(device_list, property))
			return update_related_device_list(device, device_list, property);
	}
	if (indigo_property_match(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property, property)) {
		return update_related_agent_list(device, property);
	}
	indigo_property **agent_cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		if (agent_cache[i] && indigo_property_match_defined(agent_cache[i], property)) {
			indigo_property *copy = indigo_copy_property(NULL, property);
			strcpy(copy->device, FILTER_DEVICE_CONTEXT->device_property_cache[i]->device);
			strcpy(copy->name, FILTER_DEVICE_CONTEXT->device_property_cache[i]->name);
			copy->access_token = indigo_get_device_or_master_token(copy->device);
			indigo_change_property(client, copy);
			indigo_release_property(copy);
			return INDIGO_OK;
		}
	}
	if (indigo_property_match(FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FILTER_FORCE_SYMMETRIC_RELATIONS
		indigo_property_copy_values(FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY, property, false);
		indigo_update_property(device, FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(ADDITIONAL_INSTANCES_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- ADDITIONAL_INSTANCES
		assert(DEVICE_CONTEXT->base_device == NULL);
		indigo_property_copy_values(ADDITIONAL_INSTANCES_PROPERTY, property, false);
		if (FILTER_DEVICE_CONTEXT->client != NULL) {
			update_additional_instances(device);
		}
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_filter_device_detach(indigo_device *device) {
	assert(device != NULL);
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_release_property(FILTER_DEVICE_CONTEXT->filter_device_list_properties[i]);
		indigo_release_property(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[i]);
	}
	indigo_release_property(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property);
	for (int i = 0; i < MAX_ADDITIONAL_INSTANCES; i++) {
		indigo_client *additional_client = FILTER_DEVICE_CONTEXT->additional_client_instances[i];
		if (additional_client != NULL) {
			indigo_detach_client(additional_client);
			free(additional_client);
			FILTER_DEVICE_CONTEXT->additional_client_instances[i] = NULL;
		}
	}
	indigo_release_property(CCD_LENS_FOV_PROPERTY);
	indigo_release_property(FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY);
	return indigo_device_detach(device);
}

indigo_result indigo_filter_client_attach(indigo_client *client) {
	assert(client != NULL);
	assert (FILTER_CLIENT_CONTEXT != NULL);
	FILTER_CLIENT_CONTEXT->client = client;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		device_cache[i] = NULL;
		agent_cache[i] = NULL;
	}
	indigo_property all_properties;
	memset(&all_properties, 0, sizeof(all_properties));
	indigo_enumerate_properties(client, &all_properties);
	update_additional_instances(FILTER_CLIENT_CONTEXT->device);
	return INDIGO_OK;
}

static bool device_in_list(indigo_property *device_list, char *name) {
	int count = device_list->count;
	for (int i = 0; i < count; i++) {
		if (!strcmp(name, device_list->items[i].name)) {
			return true;
		}
	}
	return false;
}

static void add_to_list(indigo_device *device, indigo_property *device_list, char *name) {
	int count = device_list->count;
	if (count < INDIGO_FILTER_MAX_DEVICES) {
		indigo_delete_property(device, device_list, NULL);
		indigo_init_switch_item(device_list->items + count, name, name, false);
		device_list->count++;
		indigo_define_property(device, device_list, NULL);
	} else {
		indigo_error("[%s:%d] Max device count reached", __FUNCTION__, __LINE__);
	}
}

static void remove_from_list(indigo_device *device, indigo_property *device_list, int start, char *name, char *selected_name) {
	for (int i = start; i < device_list->count; i++) {
		if (!strcmp(name, device_list->items[i].name)) {
			if (device_list->items[i].sw.value && selected_name) {
				device_list->items[0].sw.value = true;
				if (selected_name) {
					*selected_name = 0;
				}
				device_list->state = INDIGO_ALERT_STATE;
			}
			int size = (device_list->count - i - 1) * sizeof(indigo_item);
			if (size > 0) {
				char* buffer = indigo_safe_malloc(size);
				memcpy(buffer, device_list->items + i + 1, size);
				memcpy(device_list->items + i, buffer, size);
				indigo_safe_free(buffer);
			}
			indigo_delete_property(device, device_list, NULL);
			device_list->count--;
			indigo_define_property(device, device_list, NULL);
			break;
		}
	}
}

static void update_ccd_lens_info(indigo_device *device, indigo_property *property) {
	bool update = false;
	if (!strcmp(property->name, CCD_LENS_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_LENS_FOCAL_LENGTH_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->focal_length = item->number.value;
				CCD_LENS_FOV_PROPERTY->state = item->number.value > 0 ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
				break;
			}
		}
		update = true;
	}
	if (!strcmp(property->name, CCD_INFO_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_INFO_PIXEL_WIDTH_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->pixel_width = item->number.value;
			}
			if (!strcmp(item->name, CCD_INFO_PIXEL_HEIGHT_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->pixel_height = item->number.value;
			}
		}
		update = true;
	}
	if (!strcmp(property->name, CCD_FRAME_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_FRAME_WIDTH_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->frame_width = (int)item->number.value;
			}
			if (!strcmp(item->name, CCD_FRAME_HEIGHT_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->frame_height = (int)item->number.value;
			}
		}
		update = true;
	}
	if (!strcmp(property->name, CCD_BIN_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			indigo_item *item = property->items + i;
			if (!strcmp(item->name, CCD_BIN_HORIZONTAL_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->bin_horizontal = (int)item->number.value;
			}
			if (!strcmp(item->name, CCD_BIN_VERTICAL_ITEM_NAME)) {
				FILTER_DEVICE_CONTEXT->bin_vertical = (int)item->number.value;
			}
		}
		update = true;
	}
	if (update) {
		CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.value = indigo_pixel_scale(FILTER_DEVICE_CONTEXT->focal_length, FILTER_DEVICE_CONTEXT->pixel_width) / 3600.0;
		CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.value = indigo_pixel_scale(FILTER_DEVICE_CONTEXT->focal_length, FILTER_DEVICE_CONTEXT->pixel_height) / 3600.0;
		CCD_LENS_FOV_FOV_WIDTH_ITEM->number.value = CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.value * FILTER_DEVICE_CONTEXT->frame_width;
		CCD_LENS_FOV_FOV_HEIGHT_ITEM->number.value = CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.value * FILTER_DEVICE_CONTEXT->frame_height;
		CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.value = CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.value * FILTER_DEVICE_CONTEXT->bin_horizontal;
		CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.value = CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.value * FILTER_DEVICE_CONTEXT->bin_vertical;
		indigo_update_property(device, CCD_LENS_FOV_PROPERTY, NULL);
	}
}

indigo_result indigo_filter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		return INDIGO_OK;
	}
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (property->type == INDIGO_BLOB_VECTOR) {
		indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB_URL);
	}
	if (!strcmp(property->name, CONNECTION_PROPERTY_NAME)) {
		add_cached_connection_property(device, property);
		for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
			indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
			if (!device_list->hidden) {
				continue;
			}
			if (property->state != INDIGO_BUSY_STATE) {
				indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
				for (int j = 1; j < device_list->count; j++) {
					if (!strcmp(property->device, device_list->items[j].name) && device_list->items[j].sw.value) {
						if (device_list->state == INDIGO_BUSY_STATE) {
							if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
								indigo_property *configuration_property = indigo_init_switch_property(NULL, property->device, CONFIG_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
								indigo_init_switch_item(configuration_property->items, CONFIG_LOAD_ITEM_NAME, NULL, true);
								configuration_property->access_token = indigo_get_device_or_master_token(configuration_property->device);
								indigo_change_property(client, configuration_property);
								indigo_release_property(configuration_property);
								strcpy(FILTER_CLIENT_CONTEXT->device_name[i], property->device);
								device_list->state = INDIGO_OK_STATE;
								indigo_property all_properties;
								memset(&all_properties, 0, sizeof(all_properties));
								strcpy(all_properties.device, property->device);
								indigo_enumerate_properties(client, &all_properties);
							}
							indigo_update_property(device, device_list, NULL);
							return INDIGO_OK;
						} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
							indigo_set_switch(device_list, device_list->items, true);
							device_list->state = INDIGO_ALERT_STATE;
							strcpy(FILTER_CLIENT_CONTEXT->device_name[i], "");
							indigo_update_property(device, device_list, NULL);
							return INDIGO_OK;
						}
					}
				}
			}
		}
	} else {
		if (!strcmp(property->name, INFO_PROPERTY_NAME)) {
			indigo_item *interface = indigo_get_item(property, INFO_DEVICE_INTERFACE_ITEM_NAME);
			if (interface) {
				int mask = atoi(interface->text.value);
				indigo_property *tmp;
				if ((mask & INDIGO_INTERFACE_AGENT) == INDIGO_INTERFACE_AGENT) {
					tmp = FILTER_CLIENT_CONTEXT->filter_related_agent_list_property;
					if (!tmp->hidden && !device_in_list(tmp, property->device) && (FILTER_CLIENT_CONTEXT->validate_related_agent == NULL || FILTER_CLIENT_CONTEXT->validate_related_agent(FILTER_CLIENT_CONTEXT->device, property, mask)))
						add_to_list(device, tmp, property->device);
				} else {
					for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
						if ((mask & interface_mask[i]) == interface_mask[i]) {
							tmp = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
							if (!tmp->hidden && !device_in_list(tmp, property->device) && (FILTER_CLIENT_CONTEXT->validate_device == NULL || FILTER_CLIENT_CONTEXT->validate_device(FILTER_CLIENT_CONTEXT->device, i, property, mask)))
								add_to_list(device, tmp, property->device);
							tmp = FILTER_CLIENT_CONTEXT->filter_related_device_list_properties[i];
							if (!tmp->hidden && !device_in_list(tmp, property->device) && (FILTER_CLIENT_CONTEXT->validate_related_device == NULL || FILTER_CLIENT_CONTEXT->validate_related_device(FILTER_CLIENT_CONTEXT->device, i, property, mask)))
								add_to_list(device, tmp, property->device);
						}
					}
				}
			}
		}
		for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
			char *name_prefix = property_name_prefix[i];
			int name_prefix_length = property_name_prefix_len[i];
			if (strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[i]))
				continue;
			if (i == INDIGO_FILTER_CCD_INDEX) {
				update_ccd_lens_info(device, property);
			}
			bool found = false;
			for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
				if (indigo_property_match(device_cache[j], property)) {
					found = true;
					break;
				}
			}
			if (!found) {
				int free_index;
				for (free_index = 0; free_index < INDIGO_FILTER_MAX_CACHED_PROPERTIES; free_index++) {
					if (device_cache[free_index] == NULL) {
						device_cache[free_index] = indigo_copy_property(NULL, property);
						indigo_property *agent_property = indigo_copy_property(NULL, property);
						strcpy(agent_property->device, device->name);
						bool translate = strncmp(name_prefix, agent_property->name, name_prefix_length);
						if (translate && !strcmp(name_prefix, "CCD_") && !strncmp(agent_property->name, "DSLR_", 5))
							translate = false;
						if (translate) {
							strcpy(agent_property->name, name_prefix);
							strcat(agent_property->name, property->name);
//							if (!strcmp(property->group, MAIN_GROUP) || !strcmp(property->group, ADVANCED_GROUP) || !strcmp(property->group, SITE_GROUP)) {
//								strcpy(agent_property->group, property_name_label[i]);
//								strcat(agent_property->group, property->group);
//							} else {
//								strcpy(agent_property->label, property_name_label[i]);
//								strcat(agent_property->label, property->label);
//							}
						}
						if (strncmp(property_name_label[i], property->group, strlen(property->group))) {
							strcpy(agent_property->group, property_name_label[i]);
							strcat(agent_property->group, property->group);
						}
						agent_cache[free_index] = agent_property;
						indigo_define_property(device, agent_property, message);
						break;
					}
				}
				if (free_index == INDIGO_FILTER_MAX_CACHED_PROPERTIES) {
					indigo_error("[%s:%d] Max cached properties count reached", __FUNCTION__, __LINE__);
				}
			}
			return INDIGO_OK;
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		return INDIGO_OK;
	}
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME) && property->state != INDIGO_BUSY_STATE) {
			indigo_item *connected_device = indigo_get_item(property, CONNECTION_CONNECTED_ITEM_NAME);
			indigo_property *device_list = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
			for (int j = 1; j < device_list->count; j++) {
				if (!strcmp(property->device, device_list->items[j].name) && device_list->items[j].sw.value) {
					if (device_list->state == INDIGO_BUSY_STATE) {
						if (property->state == INDIGO_ALERT_STATE) {
							indigo_set_switch(device_list, device_list->items, true);
							device_list->state = INDIGO_ALERT_STATE;
							strcpy(FILTER_CLIENT_CONTEXT->device_name[i], "");
						} else if (connected_device->sw.value && property->state == INDIGO_OK_STATE) {
							indigo_property *configuration_property = indigo_init_switch_property(NULL, property->device, CONFIG_PROPERTY_NAME, NULL, NULL, INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
							indigo_init_switch_item(configuration_property->items, CONFIG_LOAD_ITEM_NAME, NULL, true);
							configuration_property->access_token = indigo_get_device_or_master_token(configuration_property->device);
							indigo_change_property(client, configuration_property);
							indigo_release_property(configuration_property);
							strcpy(FILTER_CLIENT_CONTEXT->device_name[i], property->device);
							if (i == INDIGO_FILTER_CCD_INDEX) {
								CCD_LENS_FOV_PROPERTY->hidden = false;
								CCD_LENS_FOV_FOV_WIDTH_ITEM->number.value =
								CCD_LENS_FOV_FOV_HEIGHT_ITEM->number.value =
								CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM->number.value =
								CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM->number.value = 0;
								CCD_LENS_FOV_PROPERTY->state = INDIGO_IDLE_STATE;
								FILTER_CLIENT_CONTEXT->frame_width =
								FILTER_CLIENT_CONTEXT->frame_height =
								FILTER_CLIENT_CONTEXT->bin_horizontal =
								FILTER_CLIENT_CONTEXT->bin_vertical = 0;
								FILTER_CLIENT_CONTEXT->pixel_width =
								FILTER_CLIENT_CONTEXT->pixel_height =
								FILTER_CLIENT_CONTEXT->focal_length = 0.0;
								indigo_define_property(device, CCD_LENS_FOV_PROPERTY, NULL);
							}
							device_list->state = INDIGO_OK_STATE;
							indigo_property all_properties;
							memset(&all_properties, 0, sizeof(all_properties));
							strcpy(all_properties.device, property->device);
							indigo_enumerate_properties(client, &all_properties);
						}
						indigo_update_property(device, device_list, NULL);
						return INDIGO_OK;
					} else if (device_list->state == INDIGO_OK_STATE && !connected_device->sw.value) {
						indigo_set_switch(device_list, device_list->items, true);
						device_list->state = INDIGO_ALERT_STATE;
						strcpy(FILTER_CLIENT_CONTEXT->device_name[i], "");
						indigo_update_property(device, device_list, NULL);
						for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
							if (device_cache[i] && !strcmp(property->device, device_cache[i]->device)) {
								indigo_release_property(device_cache[i]);
								device_cache[i] = NULL;
								if (agent_cache[i]) {
									indigo_delete_property(device, agent_cache[i], message);
									indigo_release_property(agent_cache[i]);
									agent_cache[i] = NULL;
								}
							}
						}
						return INDIGO_OK;
					}
				}
			}
		} else {
			if (strcmp(property->device, FILTER_CLIENT_CONTEXT->device_name[i]))
				continue;
			if (i == INDIGO_FILTER_CCD_INDEX) {
				update_ccd_lens_info(device, property);
			}
			for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
				indigo_property *agent_property = agent_cache[i];
				indigo_property *device_property = device_cache[i];
				if (indigo_property_match(device_property, property)) {
					device_cache[i] = indigo_copy_property(device_property, property);
					if (agent_property) {
						if (agent_property->type == INDIGO_TEXT_VECTOR) {
							for (int k = 0; k < agent_property->count; k++) {
								indigo_set_text_item_value(agent_property->items + k, indigo_get_text_item_value(property->items + k));
							}
						} else {
							memcpy(agent_property->items, property->items, property->count * sizeof(indigo_item));
						}
						agent_property->state = property->state;
						indigo_update_property(device, agent_property, message);
					}
					return INDIGO_OK;
				}
			}
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (device == FILTER_CLIENT_CONTEXT->device) {
		return INDIGO_OK;
	}
	device = FILTER_CLIENT_CONTEXT->device;
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	if (*property->name) {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (indigo_property_match(device_cache[i], property)) {
				indigo_release_property(device_cache[i]);
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					indigo_delete_property(device, agent_cache[i], NULL);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
				break;
			}
		}
		if (!strcmp(property->name, CONNECTION_PROPERTY_NAME))
			remove_cached_connection_property(device, property);
	} else {
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] && !strcmp(device_cache[i]->device, property->device)) {
				indigo_release_property(device_cache[i]);
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					indigo_delete_property(device, agent_cache[i], message);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
			}
		}
		remove_cached_connection_property(device, property);
	}
	if (*property->name == 0 || !strcmp(property->name, INFO_PROPERTY_NAME)) {
		for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
			remove_from_list(device, FILTER_CLIENT_CONTEXT->filter_device_list_properties[i], 1, property->device, FILTER_CLIENT_CONTEXT->device_name[i]);
			remove_from_list(device, FILTER_CLIENT_CONTEXT->filter_related_device_list_properties[i], 1, property->device, NULL);
		}
		remove_from_list(device, FILTER_CLIENT_CONTEXT->filter_related_agent_list_property, 0, property->device, NULL);
		for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
			if (device_cache[i] && !strcmp(property->device, device_cache[i]->device)) {
				indigo_release_property(device_cache[i]);
				device_cache[i] = NULL;
				if (agent_cache[i]) {
					indigo_delete_property(device, agent_cache[i], message);
					indigo_release_property(agent_cache[i]);
					agent_cache[i] = NULL;
				}
			}
		}
	}
	return INDIGO_OK;
}

indigo_result indigo_filter_client_detach(indigo_client *client) {
	for (int i = 0; i < INDIGO_FILTER_LIST_COUNT; i++) {
		indigo_property *list = FILTER_CLIENT_CONTEXT->filter_device_list_properties[i];
		for (int j = 1; j < list->count; j++) {
			indigo_item *item = list->items + j;
			if (item->sw.value) {
				indigo_change_switch_property_1(client, item->name, CONNECTION_PROPERTY_NAME, CONNECTION_DISCONNECTED_ITEM_NAME, true);
				break;
			}
		}
	}
	indigo_property **device_cache = FILTER_CLIENT_CONTEXT->device_property_cache;
	indigo_property **agent_cache = FILTER_CLIENT_CONTEXT->agent_property_cache;
	for (int i = 0; i < INDIGO_FILTER_MAX_CACHED_PROPERTIES; i++) {
		if (device_cache[i]) {
			indigo_release_property(device_cache[i]);
		}
		if (agent_cache[i]) {
			indigo_release_property(agent_cache[i]);
		}
	}
	return INDIGO_OK;
}

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
__attribute__((deprecated))
#endif
bool indigo_filter_cached_property(indigo_device *device, int index, char *name, indigo_property **device_property, indigo_property **agent_property) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->device_property_cache;
	char *device_name = FILTER_DEVICE_CONTEXT->device_name[index];
	indigo_property *property;
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if ((property = cache[j])) {
			if (!strcmp(property->device, device_name) && !strcmp(property->name, name)) {
				if (device_property) {
					*device_property = cache[j];
				}
				if (agent_property) {
					*agent_property = FILTER_DEVICE_CONTEXT->agent_property_cache[j];
				}
				return true;
			}
		}
	}
	return false;
}

indigo_result indigo_filter_forward_change_property(indigo_client *client, indigo_property *property, char *device_name, char *property_name) {
	indigo_property *copy = indigo_copy_property(NULL, property);
	if (device_name) {
		strcpy(copy->device, device_name);
	}
	if (property_name) {
		strcpy(copy->name, property_name);
	}
	copy->access_token = indigo_get_device_or_master_token(copy->device);
	indigo_result result = indigo_change_property(client, copy);
	indigo_release_property(copy);
	return result;
}

char *indigo_filter_first_related_agent(indigo_device *device, char *base_name_1) {
	indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	int base_name_len_1 = (int)strlen(base_name_1);
	for (int i = 0; i < related_agents_property->count; i++) {
		indigo_item *item = related_agents_property->items + i;
		if (item->sw.value && !strncmp(base_name_1, item->name, base_name_len_1)) {
			return item->name;
		}
	}
	return NULL;
}

char *indigo_filter_first_related_agent_2(indigo_device *device, char *base_name_1, char *base_name_2) {
	indigo_property *related_agents_property = FILTER_DEVICE_CONTEXT->filter_related_agent_list_property;
	int base_name_len_1 = (int)strlen(base_name_1);
	int base_name_len_2 = (int)strlen(base_name_2);
	for (int i = 0; i < related_agents_property->count; i++) {
		indigo_item *item = related_agents_property->items + i;
		if (item->sw.value && (!strncmp(base_name_1, item->name, base_name_len_1) || !strncmp(base_name_2, item->name, base_name_len_2))) {
			return item->name;
		}
	}
	return NULL;
}

int indigo_save_switch_state(indigo_device *device, char *name, char *new_state) {
	indigo_property **cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
	indigo_property *property;
	for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
		if ((property = cache[j])) {
			if (!strcmp(property->device, device->name) && !strcmp(property->name, name)) {
				property = FILTER_DEVICE_CONTEXT->agent_property_cache[j];
				for (int i = 0; i < property->count; i++) {
					if (property->items[i].sw.value) {
						if (new_state) {
							indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, property->name, new_state, true);
						}
						return i;
					}
				}
			}
		}
	}
	return -1;
}

void indigo_restore_switch_state(indigo_device *device, char *name, int index) {
	if (index >= 0) {
		indigo_property **cache = FILTER_DEVICE_CONTEXT->agent_property_cache;
		indigo_property *property;
		for (int j = 0; j < INDIGO_FILTER_MAX_CACHED_PROPERTIES; j++) {
			if ((property = cache[j])) {
				if (!strcmp(property->device, device->name) && !strcmp(property->name, name)) {
					property = FILTER_DEVICE_CONTEXT->agent_property_cache[j];
					if (index < property->count) {
						indigo_change_switch_property_1(FILTER_DEVICE_CONTEXT->client, device->name, property->name, property->items[index].name, true);
						indigo_update_property(device, property, NULL);
						break;
					}
				}
			}
		}
	}
}
