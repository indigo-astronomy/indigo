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

#include "indigo_bus.h"
#include "indigo_driver.h"
#include "indigo_dome_azimuth.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Main dome group name string.
*/
#define DOME_MAIN_GROUP									"Dome"

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

/** DOME_SYNC property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_SYNC_PROPERTY			(DOME_CONTEXT->dome_sync_property)

/** DOME_SYNC.THRESHOLD property item pointer.
 */
#define DOME_SYNC_THRESHOLD_ITEM								(DOME_SYNC_PROPERTY->items+0)

/** DOME_ABORT_MOTION property pointer, property is optional, property change request should be fully handled by dome driver
 */
#define DOME_ABORT_MOTION_PROPERTY						(DOME_CONTEXT->dome_abort_motion_property)

/** DOME_ABORT_MOTION.ABORT_MOTION property item pointer.
 */
#define DOME_ABORT_MOTION_ITEM							(DOME_ABORT_MOTION_PROPERTY->items+0)

/** DOME_SHUTTER property pointer, property is mandatory, property change request should be fully handled by dome driver
 */
#define DOME_SHUTTER_PROPERTY								(DOME_CONTEXT->dome_shutter_property)

/** DOME_SHUTTER.OPENED property item pointer.
 */
#define DOME_SHUTTER_OPENED_ITEM							(DOME_SHUTTER_PROPERTY->items+0)

/** DOME_SHUTTER.CLOSED property item pointer.
 */
#define DOME_SHUTTER_CLOSED_ITEM							(DOME_SHUTTER_PROPERTY->items+1)

/** DOME_PARK property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define DOME_PARK_PROPERTY														(DOME_CONTEXT->dome_park_property)

/** DOME_PARK.PARKED property item pointer.
 */
#define DOME_PARK_PARKED_ITEM												(DOME_PARK_PROPERTY->items+0)

/** DOME_PARK.UNPARKED property item pointer.
 */
#define DOME_PARK_UNPARKED_ITEM											(DOME_PARK_PROPERTY->items+1)

//----------------------------------------------
/** DOME_DIMENSION property pointer, property is optional
 */
#define DOME_DIMENSION_PROPERTY										(DOME_CONTEXT->dome_measurement_property)

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

/** SNOOP_DEVICES property pointer, property is optional.
 */
#define SNOOP_DEVICES_PROPERTY					(DOME_CONTEXT->snoop_devices_property)
	
/** SNOOP_DEVICES_PROPERTY.MOUNT property item pointer.
 */
#define SNOOP_MOUNT_ITEM		(SNOOP_DEVICES_PROPERTY->items+0)


/** Dome device context structure.
 */
typedef struct {
	indigo_device_context device_context;										///< device context base
	indigo_property *dome_speed_property;										///< DOME_SPEED property pointer
	indigo_property *dome_direction_property;								///< DOME_DIRECTION property pointer
	indigo_property *dome_steps_property;										///< DOME_STEPS property pointer
	indigo_property *dome_equatorial_coordinates_property; 	///< DOME_EQUATORIAL_COORDINATES property pointer
	indigo_property *dome_horizontal_coordinates_property;	///< DOME_HORIZONTAL_COORDINATES property pointer
	indigo_property *dome_sync_property;					///< DOME_SYNC property pointer
	indigo_property *dome_abort_motion_property;						///< DOME_ABORT_MOTION property pointer
	indigo_property *dome_shutter_property;									///< DOME_SHUTTER_PROPERTY pointer
	indigo_property *dome_park_property;										///< DOME_PARK property pointer
	indigo_property *dome_measurement_property;							///< DOME_PARK property pointer
	indigo_property *dome_geographic_coordinates_property;	///< DOME_GEOGRAPHIC_COORDINATES property pointer
	indigo_property *snoop_devices_property;								///< SNOOP_DEVICES property pointer
	indigo_timer *sync_timer;
} indigo_dome_context;

/** Attach callback function.
 */
extern indigo_result indigo_dome_attach(indigo_device *device, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_dome_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_dome_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_dome_detach(indigo_device *device);
/** Update dome coordinates.
 */
extern indigo_result indigo_fix_dome_coordinates(indigo_device *device, double ra, double dec, double *alt, double *az);

#ifdef __cplusplus
}
#endif

#endif /* indigo_dome_h */
