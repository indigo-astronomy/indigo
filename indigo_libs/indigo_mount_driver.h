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

/** INDIGO Filter mount Driver base
 \file indigo_mount_driver.h
 */

#ifndef indigo_mount_device_h
#define indigo_mount_device_h

#include "indigo_bus.h"
#include "indigo_driver.h"

/** Main mount group name string.
 */
#define MOUNT_MAIN_GROUP												"Mount main"

/** Device context pointer.
 */
#define MOUNT_DEVICE_CONTEXT										((indigo_mount_device_context *)device->device_context)

/** MOUNT_PARK property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define MOUNT_PARK_PROPERTY											(MOUNT_DEVICE_CONTEXT->mount_park_property)

/** MOUNT_PARK.PARKED property item pointer.
 */
#define MOUNT_PARK_PARKED_ITEM									(MOUNT_PARK_PROPERTY->items+0)

/** MOUNT_PARK.UNPARKED property item pointer.
 */
#define MOUNT_PARK_UNPARKED_ITEM								(MOUNT_PARK_PROPERTY->items+1)

/** MOUNT_EQUATORIAL_COORDINATES property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define MOUNT_EQUATORIAL_COORDINATES_PROPERTY		(MOUNT_DEVICE_CONTEXT->mount_equatorial_coordinates_property)

/** MOUNT_EQUATORIAL_COORDINATES.RA property item pointer.
 */
#define MOUNT_EQUATORIAL_COORDINATES_RA_ITEM		(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items+0)

/** MOUNT_EQUATORIAL_COORDINATES.DEC property item pointer.
 */
#define MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM		(MOUNT_EQUATORIAL_COORDINATES_PROPERTY->items+1)

/** MOUNT_HORIZONTAL_COORDINATES property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define MOUNT_HORIZONTAL_COORDINATES_PROPERTY		(MOUNT_DEVICE_CONTEXT->mount_horizontal_coordinates_property)

/** MOUNT_HORIZONTAL_COORDINATES.ALT property item pointer.
 */
#define MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM		(MOUNT_HORIZONTAL_COORDINATES_PROPERTY->items+0)

/** MOUNT_HORIZONTAL_COORDINATES.AZ property item pointer.
 */
#define MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM		(MOUNT_HORIZONTAL_COORDINATES_PROPERTY->items+1)

/** MOUNT_ON_COORDINATES_SET property pointer, property is mandatory, property change request is handled by indigo_mount_device_change_property.
 */
#define MOUNT_ON_COORDINATES_SET_PROPERTY				(MOUNT_DEVICE_CONTEXT->mount_on_coordinates_set_property)

/** MOUNT_ON_COORDINATES_SET.TRACK property item pointer.
 */
#define MOUNT_ON_COORDINATES_SET_TRACK_ITEM			(MOUNT_ON_COORDINATES_SET_PROPERTY->items+0)

/** MOUNT_ON_COORDINATES_SET.SLEW property item pointer.
 */
#define MOUNT_ON_COORDINATES_SET_SLEW_ITEM			(MOUNT_ON_COORDINATES_SET_PROPERTY->items+1)

/** MOUNT_ON_COORDINATES_SET.SYNC property item pointer.
 */
#define MOUNT_ON_COORDINATES_SET_SYNC_ITEM			(MOUNT_ON_COORDINATES_SET_PROPERTY->items+2)

/** Wheel device context structure.
 */
typedef struct {
	indigo_device_context device_context;										///< device context base
	indigo_property *mount_park_property;										///< MOUNT_PARK property pointer
	indigo_property *mount_equatorial_coordinates_property;	///< MOUNT_EQUATORIAL_COORDINATES property pointer
	indigo_property *mount_horizontal_coordinates_property;	///< MOUNT_HORIZONTAL_COORDINATES property pointer
	indigo_property *mount_on_coordinates_set_property;			///< MOUNT_ON_COORDINATES_SET property pointer
} indigo_mount_device_context;

/** Attach callback function.
 */
extern indigo_result indigo_mount_device_attach(indigo_device *device, indigo_version version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_mount_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_mount_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_mount_device_detach(indigo_device *device);

#endif /* indigo_mount_device_h */

