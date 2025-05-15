// Copyright (c) 2017-2025 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO GPS Driver base
 \file indigo_gps_driver.h
 */

#ifndef indigo_gps_h
#define indigo_gps_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Main site group name string.
 */
#define GPS_SITE_GROUP															SITE_GROUP

/** Advanced group name string.
 */
#define GPS_ADVANCED_GROUP													ADVANCED_GROUP

/** Device context pointer.
 */
#define GPS_CONTEXT																((indigo_gps_context *)device->device_context)

//-------------------------------------------
/** GPS_GEOGRAPHIC_COORDINATES property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define GPS_GEOGRAPHIC_COORDINATES_PROPERTY					(GPS_CONTEXT->gps_geographic_coordinates_property)

/** GPS_GEOGRAPHIC_COORDINATES.LATITUDE property item pointer.
 */
#define GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM		(GPS_GEOGRAPHIC_COORDINATES_PROPERTY->items+0)

/** GPS_GEOGRAPHIC_COORDINATES.LONGITUDE property item pointer.
 */
#define GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM		(GPS_GEOGRAPHIC_COORDINATES_PROPERTY->items+1)

/** GPS_GEOGRAPHIC_COORDINATES.ELEVATION property item pointer.
 */
#define GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM		(GPS_GEOGRAPHIC_COORDINATES_PROPERTY->items+2)

/** GPS_GEOGRAPHIC_COORDINATES.ACCURACY property item pointer.
 */
#define GPS_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM		(GPS_GEOGRAPHIC_COORDINATES_PROPERTY->items+3)

//---------------------------------------------
/** GPS_UTC_TIME property pointer, property is optional, property change request should be fully handled by the device driver.
 */
#define GPS_UTC_TIME_PROPERTY												(GPS_CONTEXT->gps_utc_time_property)

/** GPS_UTC_TIME.UTC property item pointer.
 */
#define GPS_UTC_ITEM																(GPS_UTC_TIME_PROPERTY->items+0)

/** GPS_UTC_TIME.OFFSET property item pointer.
 */
#define GPS_UTC_OFFEST_ITEM													(GPS_UTC_TIME_PROPERTY->items+1)


//---------------------------------------------
/** GPS_STAUS property pointer, property is optional.
 */
#define GPS_STATUS_PROPERTY													(GPS_CONTEXT->gps_status_property)

/** GPS_NO_FIX_ITEM property item pointer.
 */
#define GPS_STATUS_NO_FIX_ITEM												(GPS_STATUS_PROPERTY->items+0)

/** GPS_2D_FIX_ITEM property item pointer.
 */
#define GPS_STATUS_2D_FIX_ITEM												(GPS_STATUS_PROPERTY->items+1)

/** GPS_3D_FIX_ITEM property item pointer.
 */
#define GPS_STATUS_3D_FIX_ITEM												(GPS_STATUS_PROPERTY->items+2)


//-------------------------------------------
/** GPS_ADVANCED property pointer.
 */
#define GPS_ADVANCED_PROPERTY                    (GPS_CONTEXT->gps_advanced_property)

/** GPS_ADVANCED.ENABLED property item pointer.
 */
#define GPS_ADVANCED_ENABLED_ITEM                (GPS_ADVANCED_PROPERTY->items+0)

/** GPS_ADVANCED_DISABLED property item pointer.
 */
#define GPS_ADVANCED_DISABLED_ITEM               (GPS_ADVANCED_PROPERTY->items+1)


//-------------------------------------------
/** GPS_ADVANCED_STATUS property pointer.
 */
#define GPS_ADVANCED_STATUS_PROPERTY                    (GPS_CONTEXT->gps_advanced_status_property)

/** GPS_ADVANCED_STATUS.SVS_IN_USE property item pointer.
 */
#define GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM             (GPS_ADVANCED_STATUS_PROPERTY->items+0)

/** GPS_ADVANCED_STATUS.SVS_IN_VIEW property item pointer.
 */
#define GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM            (GPS_ADVANCED_STATUS_PROPERTY->items+1)

/** GPS_ADVANCED_STATUS.PDOP property item pointer.
 */
#define GPS_ADVANCED_STATUS_PDOP_ITEM                   (GPS_ADVANCED_STATUS_PROPERTY->items+2)

/** GPS_ADVANCED_STATUS.HDOP property item pointer.
 */
#define GPS_ADVANCED_STATUS_HDOP_ITEM                   (GPS_ADVANCED_STATUS_PROPERTY->items+3)

/** GPS_ADVANCED_STATUS.VDOP property item pointer.
 */
#define GPS_ADVANCED_STATUS_VDOP_ITEM                   (GPS_ADVANCED_STATUS_PROPERTY->items+4)


//------------------------------------------------
/** Mount device context structure.
 */
typedef struct {
	indigo_device_context device_context;                   ///< device context base
	indigo_property *gps_geographic_coordinates_property;	///< GPS_GEOGRAPHIC_COORDINATES property pointer
	indigo_property *gps_utc_time_property;                 ///< GPS_UTC_TIME property_pointe
	indigo_property *gps_status_property;                   ///< GPS_STAUS property_pointe
	indigo_property *gps_advanced_property;                 ///< GPS_ADVANCED property_pointe
	indigo_property *gps_advanced_status_property;          ///< GPS_ADVANCED_STAUS property_pointe
} indigo_gps_context;

/** Attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_gps_attach(indigo_device *device, const char* driver_name, unsigned version);
/** Enumerate properties callback function.
 */
INDIGO_EXTERN indigo_result indigo_gps_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
INDIGO_EXTERN indigo_result indigo_gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
INDIGO_EXTERN indigo_result indigo_gps_detach(indigo_device *device);

#ifdef __cplusplus
}
#endif

#endif /* indigo_gps_h */
