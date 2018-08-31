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

/** INDIGO Mount Driver base
 \file indigo_mount_driver.h
 */

#ifndef indigo_mount_h
#define indigo_mount_h

#include "indigo_bus.h"
#include "indigo_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Main mount group name string.
 */
#define MOUNT_MAIN_GROUP															"Mount"

/** Mount alignment group name string.
 */
#define MOUNT_ALIGNMENT_GROUP													"Alignment"

/** Main site group name string.
 */
#define MOUNT_SITE_GROUP															"Site"

/** Device context pointer.
 */
#define MOUNT_CONTEXT																	((indigo_mount_context *)device->device_context)

//-------------------------------------------
/** MOUNT_GEOGRAPHIC_COORDINATES property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY					(MOUNT_CONTEXT->mount_geographic_coordinates_property)

/** MOUNT_GEOGRAPHIC_COORDINATES.LATITUDE property item pointer.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM		(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->items+0)

/** MOUNT_GEOGRAPHIC_COORDINATES.LONGITUDE property item pointer.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM		(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->items+1)

/** MOUNT_GEOGRAPHIC_COORDINATES.ELEVATION property item pointer.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM		(MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY->items+2)

//---------------------------------------------
/** MOUNT_LST_TIME property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define MOUNT_LST_TIME_PROPERTY												(MOUNT_CONTEXT->mount_lst_time_property)

/** MOUNT_TIME.TIME property item pointer.
 */
#define MOUNT_LST_TIME_ITEM														(MOUNT_LST_TIME_PROPERTY->items+0)

//---------------------------------------------
/** MOUNT_INFO property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define MOUNT_INFO_PROPERTY														(MOUNT_CONTEXT->mount_info_property)

/** MOUNT_INFO.VENDOR property item pointer.
 */
#define MOUNT_INFO_VENDOR_ITEM												(MOUNT_INFO_PROPERTY->items+0)

/** MOUNT_INFO.MODEL property item pointer.
 */
#define MOUNT_INFO_MODEL_ITEM													(MOUNT_INFO_PROPERTY->items+1)

/** MOUNT_INFO.MODEL property item pointer.
 */
#define MOUNT_INFO_FIRMWARE_ITEM											(MOUNT_INFO_PROPERTY->items+2)


//---------------------------------------------
/** MOUNT_UTC_TIME property pointer, property is optional, property change request should be fully handled by the device driver.
 */
#define MOUNT_UTC_TIME_PROPERTY												(MOUNT_CONTEXT->mount_utc_time_property)

/** MOUNT_UTC_TIME.UTC property item pointer.
 */
#define MOUNT_UTC_ITEM																(MOUNT_UTC_TIME_PROPERTY->items+0)

/** MOUNT_UTC_TIME.UTC property item pointer.
 */
#define MOUNT_UTC_OFFEST_ITEM													(MOUNT_UTC_TIME_PROPERTY->items+1)

//----------------------------------------------
/** MOUNT_SET_HOST_TIME property pointer, property is optional, property change request should be fully handled by the device driver.
 */
#define MOUNT_SET_HOST_TIME_PROPERTY									(MOUNT_CONTEXT->mount_set_host_time_property)

/** MOUNT_SET_HOST_TIME.SET property item pointer.
 */
#define MOUNT_SET_HOST_TIME_ITEM											(MOUNT_SET_HOST_TIME_PROPERTY->items+0)

//----------------------------------------------
/** MOUNT_PARK_SET property pointer, property is optional, property change request should be fully handled by indigo_mount_change_property.
 */
#define MOUNT_PARK_SET_PROPERTY												(MOUNT_CONTEXT->mount_set_park_property)

/** MOUNT_PARK_SET.DEFAULT property item pointer.
 */
#define MOUNT_PARK_SET_DEFAULT_ITEM										(MOUNT_PARK_SET_PROPERTY->items+0)

/** MOUNT_PARK_SET.CURRENT property item pointer.
 */
#define MOUNT_PARK_SET_CURRENT_ITEM										(MOUNT_PARK_SET_PROPERTY->items+1)

//----------------------------------------------
/** MOUNT_PARK_POSITION property pointer, property is optional, property change request should be fully handled by indigo_mount_change_property.
*/
#define MOUNT_PARK_POSITION_PROPERTY									(MOUNT_CONTEXT->mount_park_position_property)

/** MOUNT_PARK_POSITION.RA property item pointer.
*/
#define MOUNT_PARK_POSITION_HA_ITEM										(MOUNT_PARK_POSITION_PROPERTY->items+0)

/** MOUNT_PARK_POSITION.DEC property item pointer.
*/
#define MOUNT_PARK_POSITION_DEC_ITEM									(MOUNT_PARK_POSITION_PROPERTY->items+1)
	
//----------------------------------------------
/** MOUNT_PARK property pointer, property is optional, property change request should be fully handled by device driver.
*/
#define MOUNT_PARK_PROPERTY														(MOUNT_CONTEXT->mount_park_property)

/** MOUNT_PARK.PARKED property item pointer.
*/
#define MOUNT_PARK_PARKED_ITEM												(MOUNT_PARK_PROPERTY->items+0)

/** MOUNT_PARK.UNPARKED property item pointer.
*/
#define MOUNT_PARK_UNPARKED_ITEM											(MOUNT_PARK_PROPERTY->items+1)
	
//----------------------------------------------
/** MOUNT_HOME_SET property pointer, property is optional, property change request should be fully handled by indigo_mount_change_property.
*/
#define MOUNT_HOME_SET_PROPERTY												(MOUNT_CONTEXT->mount_set_home_property)

/** MOUNT_HOME_SET.DEFAULT property item pointer.
*/
#define MOUNT_HOME_SET_DEFAULT_ITEM										(MOUNT_HOME_SET_PROPERTY->items+0)

/** MOUNT_HOME_SET.CURRENT property item pointer.
*/
#define MOUNT_HOME_SET_CURRENT_ITEM										(MOUNT_HOME_SET_PROPERTY->items+1)

//----------------------------------------------
/** MOUNT_HOME_POSITION property pointer, property is optional, property change request should be fully handled by indigo_mount_change_property.
*/
#define MOUNT_HOME_POSITION_PROPERTY									(MOUNT_CONTEXT->mount_home_position_property)

/** MOUNT_HOME_POSITION.RA property item pointer.
*/
#define MOUNT_HOME_POSITION_HA_ITEM										(MOUNT_HOME_POSITION_PROPERTY->items+0)

/** MOUNT_HOME_POSITION.DEC property item pointer.
*/
#define MOUNT_HOME_POSITION_DEC_ITEM									(MOUNT_HOME_POSITION_PROPERTY->items+1)

//----------------------------------------------
/** MOUNT_HOME property pointer, property is optional, property change request should be fully handled by device driver.
*/
#define MOUNT_HOME_PROPERTY														(MOUNT_CONTEXT->mount_home_property)

/** MOUNT_HOME.HOME property item pointer.
*/
#define MOUNT_HOME_ITEM												        (MOUNT_HOME_PROPERTY->items+0)
	
	
//----------------------------------------------
/** MOUNT_ON_COORDINATES_SET property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_ON_COORDINATES_SET_PROPERTY							(MOUNT_CONTEXT->mount_on_coordinates_set_property)

/** MOUNT_ON_COORDINATES_SET.TRACK property item pointer.
 */
#define MOUNT_ON_COORDINATES_SET_TRACK_ITEM						(MOUNT_ON_COORDINATES_SET_PROPERTY->items+0)

/** MOUNT_ON_COORDINATES_SET.SYNC property item pointer.
 */
#define MOUNT_ON_COORDINATES_SET_SYNC_ITEM						(MOUNT_ON_COORDINATES_SET_PROPERTY->items+1)

/** MOUNT_ON_COORDINATES_SET.SLEW property item pointer.
 */
#define MOUNT_ON_COORDINATES_SET_SLEW_ITEM						(MOUNT_ON_COORDINATES_SET_PROPERTY->items+2)

//---------------------------------------------
/** MOUNT_SLEW_RATE property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_SLEW_RATE_PROPERTY											(MOUNT_CONTEXT->mount_slew_rate_property)

/** MOUNT_SLEW_RATE.GUIDE property item pointer.
 */
#define MOUNT_SLEW_RATE_GUIDE_ITEM										(MOUNT_SLEW_RATE_PROPERTY->items+0)

/** MOUNT_SLEW_RATE.CENTERING property item pointer.
 */
#define MOUNT_SLEW_RATE_CENTERING_ITEM								(MOUNT_SLEW_RATE_PROPERTY->items+1)

/** MOUNT_SLEW_RATE.FIND property item pointer.
 */
#define MOUNT_SLEW_RATE_FIND_ITEM											(MOUNT_SLEW_RATE_PROPERTY->items+2)

/** MOUNT_SLEW_RATE.MAX property item pointer.
 */
#define MOUNT_SLEW_RATE_MAX_ITEM											(MOUNT_SLEW_RATE_PROPERTY->items+3)

//----------------------------------------------
/** MOUNT_MOTION_NS property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_MOTION_DEC_PROPERTY											(MOUNT_CONTEXT->mount_motion_dec_property)

/** MOUNT_MOTION_NS.NORTH property item pointer.
 */
#define  MOUNT_MOTION_NORTH_ITEM											(MOUNT_MOTION_DEC_PROPERTY->items+0)

/** MOUNT_MOTION_NS.SOUTH property item pointer.
 */
#define  MOUNT_MOTION_SOUTH_ITEM						          (MOUNT_MOTION_DEC_PROPERTY->items+1)

//----------------------------------------------
/** MOUNT_MOTION_WE property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_MOTION_RA_PROPERTY											(MOUNT_CONTEXT->mount_motion_ra_property)

/** MOUNT_MOTION_WE.WEST property item pointer.
 */
#define  MOUNT_MOTION_WEST_ITEM												(MOUNT_MOTION_RA_PROPERTY->items+0)

/** MOUNT_MOTION_WE.EAST property item pointer.
 */
#define  MOUNT_MOTION_EAST_ITEM						            (MOUNT_MOTION_RA_PROPERTY->items+1)

//-----------------------------------------------
/** MOUNT_TRACK_RATE property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_TRACK_RATE_PROPERTY											(MOUNT_CONTEXT->mount_track_rate_property)

/** MOUNT_TRACK_RATE.SIDEREAL property item pointer.
 */
#define MOUNT_TRACK_RATE_SIDEREAL_ITEM								(MOUNT_TRACK_RATE_PROPERTY->items+0)

/** MOUNT_TRACK_RATE.SOLAR property item pointer.
 */
#define MOUNT_TRACK_RATE_SOLAR_ITEM										(MOUNT_TRACK_RATE_PROPERTY->items+1)

/** MOUNT_TRACK_RATE.LINAR property item pointer.
 */
#define MOUNT_TRACK_RATE_LUNAR_ITEM										(MOUNT_TRACK_RATE_PROPERTY->items+2)

//-----------------------------------------------
/** MOUNT_TRACKING property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_TRACKING_PROPERTY												(MOUNT_CONTEXT->mount_tracking_property)

/** MOUNT_TRACKING.ON property item pointer.
 */
#define MOUNT_TRACKING_ON_ITEM												(MOUNT_TRACKING_PROPERTY->items+0)

/** MOUNT_TRACKING.OFF property item pointer.
 */
#define MOUNT_TRACKING_OFF_ITEM												(MOUNT_TRACKING_PROPERTY->items+1)

//-----------------------------------------------
/** MOUNT_GUIDE_RATE property pointer, property is mandatory, property change request is handled by indigo_mount_change_property.
 */
#define MOUNT_GUIDE_RATE_PROPERTY											(MOUNT_CONTEXT->mount_guide_rate_property)

/** MOUNT_GUIDE_RATE.RA property item pointer.
 */
#define MOUNT_GUIDE_RATE_RA_ITEM											(MOUNT_GUIDE_RATE_PROPERTY->items+0)

/** MOUNT_GUIDE_RATE.DEC property item pointer.
 */
#define MOUNT_GUIDE_RATE_DEC_ITEM											(MOUNT_GUIDE_RATE_PROPERTY->items+1)

//-----------------------------------------------
/** MOUNT_EQUATORIAL_COORDINATES property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define MOUNT_EQUATORIAL_COORDINATES_PROPERTY					(MOUNT_CONTEXT->mount_equatorial_coordinates_property)

/** MOUNT_EQUATORIAL_COORDINATES.RA property item pointer.
 */
#define MOUNT_EQUATORIAL_COORDINATES_RA_ITEM					(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items+0)

/** MOUNT_EQUATORIAL_COORDINATES.DEC property item pointer.
 */
#define MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM					(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items+1)

//-----------------------------------------------
/** MOUNT_HORIZONTAL_COORDINATES property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define MOUNT_HORIZONTAL_COORDINATES_PROPERTY					(MOUNT_CONTEXT->mount_horizontal_coordinates_property)

/** MOUNT_HORIZONTAL_COORDINATES.AZ property item pointer.
 */
#define MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM					(MOUNT_HORIZONTAL_COORDINATES_PROPERTY->items+0)
	
/** MOUNT_HORIZONTAL_COORDINATES.ALT property item pointer.
 */
#define MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM					(MOUNT_HORIZONTAL_COORDINATES_PROPERTY->items+1)

//------------------------------------------------
/** MOUNT_ABORT_MOTION property pointer, property is mandatory, property change request should be fully handled by focuser driver
 */
#define MOUNT_ABORT_MOTION_PROPERTY										(MOUNT_CONTEXT->mount_abort_motion_property)

/** FOCUSER_ABORT_MOTION.ABORT_MOTION property item pointer.
 */
#define MOUNT_ABORT_MOTION_ITEM												(MOUNT_ABORT_MOTION_PROPERTY->items+0)

//------------------------------------------------
/** MOUNT_ALIGNMENT_MODE property pointer, property is mandatory, property change request is fully handled by indigo_mount_change_property
 */
#define MOUNT_ALIGNMENT_MODE_PROPERTY									(MOUNT_CONTEXT->mount_alignment_mode_property)

/** MOUNT_ALIGNMENT_MODE.SINGLE_POINT property item pointer.
 */
#define MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM				(MOUNT_ALIGNMENT_MODE_PROPERTY->items+0)

/** MOUNT_ALIGNMENT_MODE.MULTI_POINT property item pointer.
 */
#define MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM					(MOUNT_ALIGNMENT_MODE_PROPERTY->items+1)

/** MOUNT_ALIGNMENT_MODE.CONTROLLER property item pointer.
 */
#define MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM					(MOUNT_ALIGNMENT_MODE_PROPERTY->items+2)
	
//-----------------------------------------------
/** MOUNT_MAPED_COORDINATES property pointer, property is mandatory, read-only and should be fully controlled by device driver.
 */
#define MOUNT_RAW_COORDINATES_PROPERTY							(MOUNT_CONTEXT->mount_raw_coordinates_property)

/** MOUNT_RAW_COORDINATES.RA property item pointer.
 */
#define MOUNT_RAW_COORDINATES_RA_ITEM							(MOUNT_RAW_COORDINATES_PROPERTY->items+0)

/** MOUNT_EQUATORIAL_COORDINATES.DEC property item pointer.
 */
#define MOUNT_RAW_COORDINATES_DEC_ITEM							(MOUNT_RAW_COORDINATES_PROPERTY->items+1)

//------------------------------------------------
/** MOUNT_ALIGNMENT_SELECT_POINTS property pointer, property is mandatory, property change request is fully handled by indigo_mount_change_property
 */
#define MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY				(MOUNT_CONTEXT->mount_alignment_select_points_property)

//------------------------------------------------
/** MOUNT_ALIGNMENT_DELETE_POINTS property pointer, property is mandatory, property change request is fully handled by indigo_mount_change_property
 */
#define MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY				(MOUNT_CONTEXT->mount_alignment_delete_points_property)

/** MOUNT_SNOOP_DEVICES property pointer, property is optional.
*/
#define MOUNT_SNOOP_DEVICES_PROPERTY									(MOUNT_CONTEXT->mount_snoop_devices_property)

/** MOUNT_SNOOP_DEVICES_PROPERTY.JOYSTICK property item pointer.
*/
#define MOUNT_SNOOP_JOYSTICK_ITEM														(MOUNT_SNOOP_DEVICES_PROPERTY->items+0)

/** MOUNT_SNOOP_DEVICES_PROPERTY.GPS property item pointer.
 */
#define MOUNT_SNOOP_GPS_ITEM																(MOUNT_SNOOP_DEVICES_PROPERTY->items+1)
	

//------------------------------------------------
/** Max number of alignment points.
 */
	
#define MOUNT_MAX_ALIGNMENT_POINTS										10

/** Aligment point structure.
 */

typedef struct {
	bool used;
	double ra, dec;
	double raw_ra, raw_dec;
} indigo_alignment_point;

//------------------------------------------------
/** Mount device context structure.
 */
typedef struct {
	indigo_device_context device_context;										///< device context base
	int alignment_point_count;															///< number of defined alignment points
	indigo_alignment_point alignment_points[MOUNT_MAX_ALIGNMENT_POINTS]; ///< alignment points
	indigo_property *mount_geographic_coordinates_property;	///< MOUNT_GEOGRAPHIC_COORDINATES property pointer
	indigo_property *mount_info_property;                   ///< MOUNT_INFO property pointer
	indigo_property *mount_lst_time_property;								///< MOUNT_LST_TIME property pointer
	indigo_property *mount_utc_time_property;               ///< MOUNT_UTC_TIME property_pointer
	indigo_property *mount_set_host_time_property;          ///< MOUNT_UTC_FROM_HOST property_pointer
	indigo_property *mount_park_property;										///< MOUNT_PARK property pointer
	indigo_property *mount_set_park_property;								///< MOUNT_PARK_SET property pointer
	indigo_property *mount_park_position_property;					///< MOUNT_PARK_POSITION property pointer
	indigo_property *mount_home_property;										///< MOUNT_HOME property pointer
	indigo_property *mount_set_home_property;								///< MOUNT_HOME_SET property pointer
	indigo_property *mount_home_position_property;					///< MOUNT_HOME_POSITION property pointer
	indigo_property *mount_on_coordinates_set_property;			///< MOUNT_ON_COORDINATES_SET property pointer
	indigo_property *mount_slew_rate_property;							///< MOUNT_SLEW_RATE property pointer
	indigo_property *mount_track_rate_property;							///< MOUNT_TRACK_RATE property pointer
	indigo_property *mount_tracking_property;								///< MOUNT_TRACKING property pointer
	indigo_property *mount_guide_rate_property;							///< MOUNT_GUIDE_RATE property pointer
	indigo_property *mount_equatorial_coordinates_property;	///< MOUNT_EQUATORIAL_COORDINATES property pointer
	indigo_property *mount_horizontal_coordinates_property;	///< MOUNT_HORIZONTAL_COORDINATES property pointer
	indigo_property *mount_abort_motion_property;						///< MOUNT_ABORT_MOTION property pointer
	indigo_property *mount_motion_dec_property;							///< MOUNT_MOTION_NS property pointer
	indigo_property *mount_motion_ra_property;							///< MOUNT_MOTION_WE property pointer
	indigo_property *mount_alignment_mode_property;					///< MOUNT_ALIGNMENT_MODE property pointer
	indigo_property *mount_raw_coordinates_property;				///< MOUNT_RAW_COORDINATES property pointer
	indigo_property *mount_alignment_select_points_property;///< MOUNT_ALIGNMENT_SELECT_POINTS property pointer
	indigo_property *mount_alignment_delete_points_property;///< MOUNT_ALIGNMENT_DELETE_POINTS property pointer
	indigo_property *mount_snoop_devices_property;					///< MOUNT_SNOOP_DEVICES property pointer
} indigo_mount_context;

/** Attach callback function.
 */
extern indigo_result indigo_mount_attach(indigo_device *device, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_mount_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_mount_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_mount_detach(indigo_device *device);

/** Translate coordinates to native.
 */

extern indigo_result indigo_translated_to_raw(indigo_device *device, double ra, double dec, double *raw_ra, double *raw_dec);

/** Translate coordinates from native.
 */

extern indigo_result indigo_raw_to_translated(indigo_device *device, double raw_ra, double raw_dec, double *ra, double *dec);

/** Translate coordinates from native.
 */
	
extern void indigo_update_coordinates(indigo_device *device, const char *message);
	
#ifdef __cplusplus
}
#endif

#endif /* indigo_mount_h */
