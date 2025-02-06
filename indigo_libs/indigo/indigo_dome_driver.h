// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO Dome Driver base
 \file indigo_dome_driver.h
 */

#ifndef indigo_dome_h
#define indigo_dome_h

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_dome_azimuth.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Main dome group name string.
*/
#define DOME_MAIN_GROUP									"Dome"

/** Site group name string.
 */
#define DOME_SITE_GROUP									SITE_GROUP

/** Advanced group name string.
 */
#define DOME_ADVANCED_GROUP            	ADVANCED_GROUP


/** Device context pointer.
 */
#define DOME_CONTEXT                					((indigo_dome_context *)device->device_context)

/** DOME_SPEED property pointer, property is optional, property change request should be fully handled by indigo_dome_change_property
 */
#define DOME_SPEED_PROPERTY								(DOME_CONTEXT->dome_speed_property)

/** DOME_SPEED.SPEED property item pointer.
 */
#define DOME_SPEED_ITEM									(DOME_SPEED_PROPERTY->items+0)

/** DOME_DIRECTION property pointer, property is optional, property change request should be fully handled by indigo_dome_change_property.
 */
#define DOME_DIRECTION_PROPERTY							(DOME_CONTEXT->dome_direction_property)

/** DOME_DIRECTION.MOVE_CLOCKWISE property item pointer.
 */
#define DOME_DIRECTION_MOVE_CLOCKWISE_ITEM				(DOME_DIRECTION_PROPERTY->items+0)

/** DOME_DIRECTION.MOVE_COUNTERCLOCKWISE property item pointer.
 */
#define DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM		(DOME_DIRECTION_PROPERTY->items+1)


//----------------------------------------------
/** DOME_ON_HORIZONTAL_COORDINATES_SET property pointer, property is optional.
 */
#define DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY							(DOME_CONTEXT->dome_on_horiz_coordinates_set_property)

/**  DOME_ON_HORIZONTAL_COORDINATES_SET.GOTO property item pointer.
 */
#define DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM						(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->items+0)

/**  DOME_ON_HORIZONTAL_COORDINATES_SET.SYNC property item pointer.
 */
#define DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM						(DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY->items+1)


/** DOME_STEPS property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_STEPS_PROPERTY								(DOME_CONTEXT->dome_steps_property)

/** DOME_STEPS.STEPS property item pointer.
 */
#define DOME_STEPS_ITEM									(DOME_STEPS_PROPERTY->items+0)

/** DOME_EQUATORIAL_COORDINATES property pointer, property is optional, property change request should be fully handled by device driver.
*/
#define DOME_EQUATORIAL_COORDINATES_PROPERTY					(DOME_CONTEXT->dome_equatorial_coordinates_property)

/** DOME_EQUATORIAL_COORDINATES.RA property item pointer.
 */
#define DOME_EQUATORIAL_COORDINATES_RA_ITEM					(DOME_EQUATORIAL_COORDINATES_PROPERTY->items+0)

/** DOME_EQUATORIAL_COORDINATES.DEC property item pointer.
 */
#define DOME_EQUATORIAL_COORDINATES_DEC_ITEM					(DOME_EQUATORIAL_COORDINATES_PROPERTY->items+1)

/** DOME_HORIZONTAL_COORDINATES property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_HORIZONTAL_COORDINATES_PROPERTY			(DOME_CONTEXT->dome_horizontal_coordinates_property)

/** DOME_HORIZONTAL_COORDINATES.AZ property item pointer.
 */
#define DOME_HORIZONTAL_COORDINATES_AZ_ITEM								(DOME_HORIZONTAL_COORDINATES_PROPERTY->items+0)

/** DOME_HORIZONTAL_COORDINATES.ALT property item pointer.
 */
#define DOME_HORIZONTAL_COORDINATES_ALT_ITEM								(DOME_HORIZONTAL_COORDINATES_PROPERTY->items+1)

/** DOME_SLAVING property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_SLAVING_PROPERTY								(DOME_CONTEXT->dome_slaving_property)

/** DOME_SLAVING.ENABLE property item pointer.
 */
#define DOME_SLAVING_ENABLE_ITEM							(DOME_SLAVING_PROPERTY->items+0)

/** DOME_SLAVING.DISABLE property item pointer.
 */
#define DOME_SLAVING_DISABLE_ITEM							(DOME_SLAVING_PROPERTY->items+1)

/** DOME_SLAVING_PARAMETERS property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_SLAVING_PARAMETERS_PROPERTY						(DOME_CONTEXT->dome_slaving_parameters_property)

/** DOME_SLAVING_PARAMETERS.THRESHOLD property item pointer.
 */
#define DOME_SLAVING_THRESHOLD_ITEM							(DOME_SLAVING_PARAMETERS_PROPERTY->items+0)


/** DOME_ABORT_MOTION property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_ABORT_MOTION_PROPERTY						(DOME_CONTEXT->dome_abort_motion_property)

/** DOME_ABORT_MOTION.ABORT_MOTION property item pointer.
 */
#define DOME_ABORT_MOTION_ITEM							(DOME_ABORT_MOTION_PROPERTY->items+0)

/** DOME_SHUTTER property pointer, property is mandatory, property change request should be fully handled by dome driver
 */
#define DOME_SHUTTER_PROPERTY								(DOME_CONTEXT->dome_shutter_property)

/** DOME_SHUTTER.CLOSED property item pointer.
 */
#define DOME_SHUTTER_CLOSED_ITEM							(DOME_SHUTTER_PROPERTY->items+0)

/** DOME_SHUTTER.OPENED property item pointer.
 */
#define DOME_SHUTTER_OPENED_ITEM							(DOME_SHUTTER_PROPERTY->items+1)

/** DOME_FLAP property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_FLAP_PROPERTY									(DOME_CONTEXT->dome_flap_property)

/** DOME_FLAP.CLOSED property item pointer.
 */
#define DOME_FLAP_CLOSED_ITEM								(DOME_FLAP_PROPERTY->items+0)

/** DOME_FLAP.OPENED property item pointer.
 */
#define DOME_FLAP_OPENED_ITEM								(DOME_FLAP_PROPERTY->items+1)

/** DOME_PARK property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define DOME_PARK_PROPERTY														(DOME_CONTEXT->dome_park_property)

/** DOME_PARK.PARKED property item pointer.
 */
#define DOME_PARK_PARKED_ITEM												(DOME_PARK_PROPERTY->items+0)

/** DOME_PARK.UNPARKED property item pointer.
 */
#define DOME_PARK_UNPARKED_ITEM											(DOME_PARK_PROPERTY->items+1)


/** DOME_PARK_POSITION property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define DOME_PARK_POSITION_PROPERTY										(DOME_CONTEXT->dome_park_position_property)

/** DOME_PARK_POSITION.AZ property item pointer.
 */
#define DOME_PARK_POSITION_AZ_ITEM									(DOME_PARK_POSITION_PROPERTY->items+0)

/** DOME_PARK_POSITION.ALT property item pointer.
 */
#define DOME_PARK_POSITION_ALT_ITEM								(DOME_PARK_POSITION_PROPERTY->items+1)

/** DOME_HOME property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define DOME_HOME_PROPERTY												(DOME_CONTEXT->dome_home_property)

/** DOME_HOME.HOME property item pointer.
 */
#define DOME_HOME_ITEM													(DOME_HOME_PROPERTY->items+0)

//----------------------------------------------
/** DOME_DIMENSION property pointer, property is optional
 */
#define DOME_DIMENSION_PROPERTY										(DOME_CONTEXT->dome_dimension_property)

/** DOME_DIMENSION.RADIUS property item pointer.
 */
#define DOME_RADIUS_ITEM														(DOME_DIMENSION_PROPERTY->items+0)

/** DOME_DIMENSION.SHUTTER_WIDTH property item pointer.
 */
#define DOME_SHUTTER_WIDTH_ITEM											(DOME_DIMENSION_PROPERTY->items+1)

/** DOME_DIMENSION.MOUNT_PIVOT_OFFSET_NS property item pointer.
 */
#define DOME_MOUNT_PIVOT_OFFSET_NS_ITEM								(DOME_DIMENSION_PROPERTY->items+2)

/** DOME_DIMENSION.MOUNT_PIVOT_OFFSET_EW property item pointer.
 */
#define DOME_MOUNT_PIVOT_OFFSET_EW_ITEM									(DOME_DIMENSION_PROPERTY->items+3)

/** DOME_DIMENSION.MOUNT_PIVOT_VERTICAL_OFFSET property item pointer.
 */
#define DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM										(DOME_DIMENSION_PROPERTY->items+4)

/** DOME_DIMENSION.MOUNT_PIVOT_OTA_OFFSET property item pointer.
 */
#define DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM												(DOME_DIMENSION_PROPERTY->items+5)

/** DOME_GEOGRAPHIC_COORDINATES property pointer, property is optional.
 */
#define DOME_GEOGRAPHIC_COORDINATES_PROPERTY					(DOME_CONTEXT->dome_geographic_coordinates_property)

/** DOME_GEOGRAPHIC_COORDINATES.LATITUDE property item pointer.
 */
#define DOME_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM		(DOME_GEOGRAPHIC_COORDINATES_PROPERTY->items+0)

/** DOME_GEOGRAPHIC_COORDINATES.LONGITUDE property item pointer.
 */
#define DOME_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM		(DOME_GEOGRAPHIC_COORDINATES_PROPERTY->items+1)

/** DOME_GEOGRAPHIC_COORDINATES.ELEVATION property item pointer.
 */
#define DOME_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM		(DOME_GEOGRAPHIC_COORDINATES_PROPERTY->items+2)

//---------------------------------------------
/** DOME_UTC_TIME property pointer, property is optional, property change request should be fully handled by the device driver.
*/
#define DOME_UTC_TIME_PROPERTY												(DOME_CONTEXT->dome_utc_time_property)

/** DOME_UTC_TIME.UTC property item pointer.
*/
#define DOME_UTC_ITEM															(DOME_UTC_TIME_PROPERTY->items+0)

/** DOME_UTC_TIME.OFFSET property item pointer.
*/
#define DOME_UTC_OFFSET_ITEM													(DOME_UTC_TIME_PROPERTY->items+1)

//----------------------------------------------
/** DOME_SET_HOST_TIME property pointer, property is optional, property change request should be fully handled by the device driver.
*/
#define DOME_SET_HOST_TIME_PROPERTY									(DOME_CONTEXT->dome_set_host_time_property)

/** DOME_SET_HOST_TIME.SET property item pointer.
*/
#define DOME_SET_HOST_TIME_ITEM											(DOME_SET_HOST_TIME_PROPERTY->items+0)

/** DOME_SNOOP_DEVICES property pointer, property is optional.
 */
#define DOME_SNOOP_DEVICES_PROPERTY					(DOME_CONTEXT->dome_snoop_devices_property)

/** DOME_SNOOP_DEVICES_PROPERTY.MOUNT property item pointer.
 */
#define DOME_SNOOP_MOUNT_ITEM					(DOME_SNOOP_DEVICES_PROPERTY->items+0)

/** DOME_SNOOP_DEVICES_PROPERTY.GPS property item pointer.
 */
#define DOME_SNOOP_GPS_ITEM					(DOME_SNOOP_DEVICES_PROPERTY->items+1)


/** Dome device context structure.
 */
typedef struct {
	indigo_device_context device_context;										///< device context base
	indigo_property *dome_speed_property;										///< DOME_SPEED property pointer
	indigo_property *dome_direction_property;								///< DOME_DIRECTION property pointer
	indigo_property *dome_on_horiz_coordinates_set_property;				///< DOME_ON_HORIZONTAL_COORDINATES_SET property pointer
	indigo_property *dome_steps_property;										///< DOME_STEPS property pointer
	indigo_property *dome_equatorial_coordinates_property; 	///< DOME_EQUATORIAL_COORDINATES property pointer
	indigo_property *dome_horizontal_coordinates_property;	///< DOME_HORIZONTAL_COORDINATES property pointer
	indigo_property *dome_slaving_property;								///< DOME_SLAVING property pointer
	indigo_property *dome_slaving_parameters_property;					///< DOME_SLAVING_PARAMETERS property pointer
	indigo_property *dome_abort_motion_property;						///< DOME_ABORT_MOTION property pointer
	indigo_property *dome_shutter_property;									///< DOME_SHUTTER_PROPERTY pointer
	indigo_property *dome_flap_property;									///< DOME_FLAP_PROPERTY pointer
	indigo_property *dome_park_property;										///< DOME_PARK property pointer
	indigo_property *dome_park_position_property;							///< DOME_PARK_POSITION property pointer
	indigo_property *dome_home_property;							///< DOME_HOME property pointer
	indigo_property *dome_dimension_property;								///< DOME_DIMENSION property pointer
	indigo_property *dome_geographic_coordinates_property;	///< DOME_GEOGRAPHIC_COORDINATES property pointer
	indigo_property *dome_utc_time_property;               	///< DOME_UTC_TIME property_pointer
	indigo_property *dome_set_host_time_property;          	///< DOME_UTC_FROM_HOST property_pointer
	indigo_property *dome_snoop_devices_property;						///< DOME_SNOOP_DEVICES property pointer
	indigo_timer *sync_timer;
} indigo_dome_context;

/** Attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_dome_attach(indigo_device *device, const char* driver_name, unsigned version);
/** Enumerate properties callback function.
 */
INDIGO_EXTERN indigo_result indigo_dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
INDIGO_EXTERN indigo_result indigo_dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
INDIGO_EXTERN indigo_result indigo_dome_detach(indigo_device *device);
/** Update dome azimuth according to mount and OTA dimensions.
 */
INDIGO_EXTERN bool indigo_fix_dome_azimuth(indigo_device *device, double ra, double dec, double az_prev, double *az);

#ifdef __cplusplus
}
#endif

#endif /* indigo_dome_h */
