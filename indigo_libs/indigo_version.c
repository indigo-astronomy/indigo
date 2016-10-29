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

/** INDIGO Version mapping
 \file indigo_version.c
 */

#include <string.h>

#include "indigo_version.h"

// <getProperties version='1.7' name='CONFIG'/>

struct property_mapping {
	char *legacy;
	char *current;
	struct item_mapping {
		char *legacy;
		char *current;
	} items[10];
};

static struct property_mapping legacy[] = {
	{	"CONNECTION", "CONNECTION", {
			{ "CONNECT", "CONNECTED" },
			{ "DISCONNECT", "DISCONNECTED" },
			NULL
		}
	},
	{	"DEBUG", "DEBUG", {
			{ "ENABLE", "ENABLED" },
			{ "DISABLE", "DISABLED" },
			NULL
		}
	},
	{	"SIMULATION", "SIMULATION", {
			{ "ENABLE", "ENABLED" },
			{ "DISABLE", "DISABLED" },
			NULL
		}
	},
	{	"CONFIG_PROCESS", "CONFIG", {
			{ "CONFIG_LOAD", "LOAD" },
			{ "CONFIG_SAVE", "SAVE" },
			{ "CONFIG_DEFAULT", "DEFAULT" },
			NULL
		}
	},
	{	"DRIVER_INFO", "DEVICE_INFO", {
			{ "DRIVER_NAME", "NAME" },
			{ "DRIVER_VERSION", "VERSION" },
			{ "DRIVER_INTERFACE", "INTERFACE" },
			NULL
		}
	},
	{	"CCD_INFO", "CCD_INFO", {
			{ "CCD_MAX_X", "WIDTH" },
			{ "CCD_MAX_Y", "HEIGHT" },
			{ "CCD_MAX_BIN_X", "MAX_HORIZONAL_BIN" },
			{ "CCD_MAX_BIN_Y", "MAX_VERTICAL_BIN" },
			{ "CCD_PIXEL_SIZE", "PIXEL_SIZE" },
			{ "CCD_PIXEL_SIZE_X", "PIXEL_WIDTH" },
			{ "CCD_PIXEL_SIZE_Y", "PIXEL_HEIGHT" },
			{ "CCD_BITSPERPIXEL", "BITS_PER_PIXEL" },
			NULL
		}
	},
	{	"CCD_EXPOSURE", "CCD_EXPOSURE", {
			{ "CCD_EXPOSURE_VALUE", "EXPOSURE" },
			NULL
		}
	},
	{	"CCD_ABORT_EXPOSURE", "CCD_ABORT_EXPOSURE", {
			{ "ABORT", "ABORT_EXPOSURE" },
			NULL
		}
	},
	{	"CCD_FRAME", "CCD_FRAME", {
			{ "X", "LEFT" },
			{ "Y", "TOP" },
			NULL
		}
	},
	{	"CCD_BINNING", "CCD_BIN", {
			{ "VER_BIN", "HORIZONTAL" },
			{ "HOR_BIN", "VERTICAL" },
			NULL
		}
	},
	{	"CCD_FRAME_TYPE", "CCD_FRAME_TYPE", {
			{ "FRAME_LIGHT", "LIGHT" },
			{ "FRAME_BIAS", "BIAS" },
			{ "FRAME_DARK", "DARK" },
			{ "FRAME_FLAT", "FLAT" },
			NULL
		}
	},
	{	"UPLOAD_MODE", "CCD_UPLOAD_MODE", {
			{ "UPLOAD_CLIENT", "CLIENT" },
			{ "UPLOAD_LOCAL", "LOCAL" },
			{ "UPLOAD_BOTH", "BOTH" },
			NULL
		}
	},
	{	"UPLOAD_SETTINGS", "CCD_LOCAL_MODE", {
			{ "UPLOAD_DIR", "DIR" },
			{ "UPLOAD_PREFIX", "PREFIX" },
			NULL
		}
	},
	{	"CCD_FILE_PATH", "CCD_IMAGE_FILE", {
			{ "FILE_PATH", "FILE" },
			NULL
		}
	},
	{	"CCD1", "CCD_IMAGE", {
			{ "CCD1", "IMAGE" },
			NULL
		}
	},
	{	"CCD_TEMPERATURE", "CCD_TEMPERATURE", {
			{ "CCD_TEMPERATURE_VALUE", "TEMPERATURE" },
			NULL
		}
	},
	{	"CCD_COOLER", "CCD_COOLER", {
			{ "COOLER_ON", "ON" },
			{ "COOLER_OFF", "OFF" },
			NULL
		}
	},
	{	"CCD_COOLER_POWER", "CCD_COOLER_POWER", {
			{ "CCD_COOLER_VALUE", "POWER" },
			NULL
		}
	},
	{	"TELESCOPE_TIMED_GUIDE_NS", "GUIDER_GUIDE_DEC", {
			{ "TIMED_GUIDE_N", "GUIDER_GUIDE_NORTH" },
			{ "TIMED_GUIDE_S", "GUIDER_GUIDE_SOUTH" },
			NULL
		}
	},
	{	"TELESCOPE_TIMED_GUIDE_WE", "GUIDER_GUIDE_RA", {
			{ "TIMED_GUIDE_W", "GUIDER_GUIDE_WEST" },
			{ "TIMED_GUIDE_E", "GUIDER_GUIDE_EAST" },
			NULL
		}
	},
	{	"FILTER_SLOT", "WHEEL_SLOT", {
			{ "FILTER_SLOT_VALUE", "SLOT" },
			NULL
		}
	},
	{	"FILTER_NAME_VALUE", "WHEEL_SLOT", {
			{ "FILTER_SLOT_NAME_1", "SLOT_NAME_1" },
			{ "FILTER_SLOT_NAME_2", "SLOT_NAME_2" },
			{ "FILTER_SLOT_NAME_3", "SLOT_NAME_3" },
			{ "FILTER_SLOT_NAME_4", "SLOT_NAME_4" },
			{ "FILTER_SLOT_NAME_5", "SLOT_NAME_5" },
			{ "FILTER_SLOT_NAME_6", "SLOT_NAME_6" },
			{ "FILTER_SLOT_NAME_7", "SLOT_NAME_7" },
			{ "FILTER_SLOT_NAME_8", "SLOT_NAME_8" },
			{ "FILTER_SLOT_NAME_9", "SLOT_NAME_9" },
			NULL
		}
	},
	{	"FOCUS_SPEED", "FOCUSER_SPEED", {
			{ "FOCUS_SPEED_VALUE", "SPEED" },
			NULL
		}
	},
	{	"FOCUS_MOTION", "FOCUSER_DIRECTION", {
			{ "FOCUS_INWARD", "MOVE_INWARD" },
			{ "FOCUS_OUTWARD", "MOVE_OUTWARD" },
			NULL
		}
	},
	{	"REL_FOCUS_POSITION", "FOCUSER_STEPS", {
			{ "FOCUS_RELATIVE_POSITION", "STEPS" },
			NULL
		}
	},
	{	"ABS_FOCUS_POSITION", "FOCUSER_POSITION", {
			{ "FOCUS_ABSOLUTE_POSITION", "POSITION" },
			NULL
		}
	},
	{	"FOCUS_ABORT_MOTION", "FOCUSER_ABORT_MOTION", {
			{ "ABORT", "ABORT_MOTION" },
			NULL
		}
	},
	{	"GEOGRAPHIC_COORD", "MOUNT_GEOGRAPHIC_COORDINATES", {
			{ "LAT", "LATITUDE" },
			{ "LONG", "LONGITUDE" },
			{ "ELEV", "ELEVATION" },
			NULL
		}
	},
	{	"TIME_LST", "MOUNT_LST_TIME", {
			{ "LST", "TIME" },
			NULL
		}
	},
	{	"TELESCOPE_PARK", "MOUNT_PARK", {
			{ "PARK", "PARKED" },
			{ "UNPARK", "UNPARKED" },
			NULL
		}
	},
	{	"EQUATORIAL_EOD_COORD", "MOUNT_EQUATORIAL_COORDINATES", {
			{ "RA", "RA" },
			{ "DEC", "DEC" },
			NULL
		}
	},
	{	"HORIZONTAL_COORD", "MOUNT_HORIZONTAL_COORDINATES", {
			{ "ALT", "ALT" },
			{ "AZ", "AZ" },
			NULL
		}
	},
	{	"ON_COORD_SET", "MOUNT_ON_COORDINATES_SET", {
			{ "SLEW", "SLEW" },
			{ "TRACK", "TRACK" },
			{ "SYNC", "SYNC" },
			NULL
		}
	},
	{	"TELESCOPE_SLEW_RATE", "MOUNT_SLEW_RATE", {
			{ "SLEW_GUIDE", "GUIDE" },
			{ "SLEW_CENTERING", "CENTERING" },
			{ "SLEW_FIND", "FIND" },
			{ "SLEW_MAX", "MAX" },
			NULL
		}
	},
	NULL
};

void indigo_copy_property_name(indigo_version version, indigo_property *property, const char *name) {
	if (version == INDIGO_VERSION_LEGACY) {
		struct property_mapping *property_mapping = legacy;
		while (property_mapping->legacy) {
			if (!strcmp(name, property_mapping->legacy)) {
				INDIGO_DEBUG(indigo_debug("version: %s -> %s (current)", property_mapping->legacy, property_mapping->current));
				strcpy(property->name, property_mapping->current);
				return;
			}
			property_mapping++;
		}
	}
	strncpy(property->name, name, INDIGO_NAME_SIZE);
}

void indigo_copy_item_name(indigo_version version, indigo_property *property, indigo_item *item, const char *name) {
	if (version == INDIGO_VERSION_LEGACY) {
		struct property_mapping *property_mapping = legacy;
		while (property_mapping->legacy) {
			if (!strcmp(property->name, property_mapping->current)) {
				struct item_mapping *item_mapping = property_mapping->items;
				while (item_mapping->legacy) {
					if (!strcmp(name, item_mapping->legacy)) {
						INDIGO_DEBUG(indigo_debug("version: %s.%s -> %s.%s (current)", property_mapping->legacy, item_mapping->legacy, property_mapping->current, item_mapping->current));
						strncpy(item->name, item_mapping->current, INDIGO_NAME_SIZE);
						return;
					}
					item_mapping++;
				}
				return;
			}
			property_mapping++;
		}
	}
	strncpy(item->name, name, INDIGO_NAME_SIZE);
}

const char *indigo_property_name(indigo_version version, indigo_property *property) {
	if (version == INDIGO_VERSION_LEGACY) {
		struct property_mapping *property_mapping = legacy;
		while (property_mapping->legacy) {
			if (!strcmp(property->name, property_mapping->current)) {
				INDIGO_DEBUG(indigo_debug("version: %s -> %s (legacy)", property_mapping->current, property_mapping->legacy));
				return property_mapping->legacy;
			}
			property_mapping++;
		}
	}
	return property->name;
}

const char *indigo_item_name(indigo_version version, indigo_property *property, indigo_item *item) {
	if (version == INDIGO_VERSION_LEGACY) {
		struct property_mapping *property_mapping = legacy;
		while (property_mapping->legacy) {
			if (!strcmp(property->name, property_mapping->current)) {
				struct item_mapping *item_mapping = property_mapping->items;
				while (item_mapping->legacy) {
					if (!strcmp(item->name, item_mapping->current)) {
						INDIGO_DEBUG(indigo_debug("version: %s.%s -> %s.%s (legacy)", property_mapping->current, item_mapping->current, property_mapping->legacy, item_mapping->legacy));
						return item_mapping->legacy;
					}
					item_mapping++;
				}
				return item->name;
			}
			property_mapping++;
		}
	}
	return item->name;
}


