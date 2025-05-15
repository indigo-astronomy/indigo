// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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
 \file indigo_filter.h
 */


#ifndef indigo_filter_h
#define indigo_filter_h
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

#define INDIGO_FILTER_LIST_COUNT							13
#define INDIGO_FILTER_MAX_DEVICES							128
#define INDIGO_FILTER_MAX_CACHED_PROPERTIES		256
	
#define INDIGO_FILTER_CCD_INDEX								0
#define INDIGO_FILTER_WHEEL_INDEX							1
#define INDIGO_FILTER_FOCUSER_INDEX						2
#define INDIGO_FILTER_ROTATOR_INDEX						3
#define INDIGO_FILTER_MOUNT_INDEX							4
#define INDIGO_FILTER_GUIDER_INDEX						5
#define INDIGO_FILTER_DOME_INDEX							6
#define INDIGO_FILTER_GPS_INDEX								7
#define INDIGO_FILTER_JOYSTICK_INDEX					8
#define INDIGO_FILTER_AUX_1_INDEX							9
#define INDIGO_FILTER_AUX_2_INDEX							10
#define INDIGO_FILTER_AUX_3_INDEX							11
#define INDIGO_FILTER_AUX_4_INDEX							12

/** Device context pointer.
 */
#define FILTER_DEVICE_CONTEXT                ((indigo_filter_context *)device->device_context)

/** Client context pointer.
 */
#define FILTER_CLIENT_CONTEXT                ((indigo_filter_context *)client->client_context)

/** CCD list switch property.
 */

#define FILTER_CCD_LIST_PROPERTY					(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_CCD_INDEX])

/** Wheel list switch property.
*/
#define FILTER_WHEEL_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_WHEEL_INDEX])

/** Focuser list switch property.
*/
#define FILTER_FOCUSER_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_FOCUSER_INDEX])

/** Rotator list switch property.
*/
#define FILTER_ROTATOR_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_ROTATOR_INDEX])

/** Mount list switch property.
*/
#define FILTER_MOUNT_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_MOUNT_INDEX])

/** Guider list switch property.
*/
#define FILTER_GUIDER_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_GUIDER_INDEX])

/** Dome list switch property.
*/
#define FILTER_DOME_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_DOME_INDEX])

/** GPS list switch property.
*/
#define FILTER_GPS_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_GPS_INDEX])

/** Joystick list switch property.
 */
#define FILTER_JOYSTICK_LIST_PROPERTY	(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_JOYSTICK_INDEX])

/** AUX #1 list switch property.
*/
#define FILTER_AUX_1_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_AUX_1_INDEX])

/** AUX #2 list switch property.
*/
#define FILTER_AUX_2_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_AUX_2_INDEX])

/** AUX #3 list switch property.
*/
#define FILTER_AUX_3_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_AUX_3_INDEX])

/** AUX #4 list switch property.
*/
#define FILTER_AUX_4_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_device_list_properties[INDIGO_FILTER_AUX_4_INDEX])

/** CCD list switch property.
 */
#define FILTER_RELATED_CCD_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_CCD_INDEX])
	
/** Related wheel list switch property.
*/
#define FILTER_RELATED_WHEEL_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_WHEEL_INDEX])

/** Related focuser list switch property.
*/
#define FILTER_RELATED_FOCUSER_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_FOCUSER_INDEX])

/** Related rotator list switch property.
*/
#define FILTER_RELATED_ROTATOR_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_ROTATOR_INDEX])

/** Related mount list switch property.
*/
#define FILTER_RELATED_MOUNT_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_MOUNT_INDEX])

/** Related guider list switch property.
*/
#define FILTER_RELATED_GUIDER_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_GUIDER_INDEX])

/** Related dome list switch property.
*/
#define FILTER_RELATED_DOME_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_DOME_INDEX])

/** Related GPS list switch property.
*/
#define FILTER_RELATED_GPS_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_GPS_INDEX])

/** Related joystick list switch property.
 */
#define FILTER_RELATED_JOYSTICK_LIST_PROPERTY	(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_JOYSTICK_INDEX])
	
/** Related AUX #1 list switch property.
*/
#define FILTER_RELATED_AUX_1_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_AUX_1_INDEX])

/** Related AUX #2 list switch property.
*/
#define FILTER_RELATED_AUX_2_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_AUX_2_INDEX])

/** Related AUX #3 list switch property.
*/
#define FILTER_RELATED_AUX_3_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_AUX_3_INDEX])

/** Related AUX #4 list switch property.
*/
#define FILTER_RELATED_AUX_4_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_device_list_properties[INDIGO_FILTER_AUX_4_INDEX])

/** Related agent list switch property.
 */
#define FILTER_RELATED_AGENT_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_agent_list_property)

/** FILTER_FORCE_SYMMETRIC_RELATIONS property pointer, property is mandatory, property change request is fully handled by indigo_filter_change_property().
 */
#define FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY	(FILTER_DEVICE_CONTEXT->filter_force_symmetric_relations_property)

/** FILTER_FORCE_SYMMETRIC_RELATIONS.ENABLED property item pointer.
 */
#define FILTER_FORCE_SYMMETRIC_RELATIONS_ENABLED_ITEM	(FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY->items+0)

/** FILTER_FORCE_SYMMETRIC_RELATIONS.DISABLED property item pointer.
 */
#define FILTER_FORCE_SYMMETRIC_RELATIONS_DISABLED_ITEM	(FILTER_FORCE_SYMMETRIC_RELATIONS_PROPERTY->items+1)


/** CCD_LENS_FOV property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_LENS_FOV_PROPERTY                (FILTER_DEVICE_CONTEXT->ccd_lens_info_property)

/** CCD_LENS_FOV.FOV_WIDTH property item pointer.
 */
#define CCD_LENS_FOV_FOV_WIDTH_ITEM          (CCD_LENS_FOV_PROPERTY->items+0)

/** CCD_LENS_FOV.FOV_HEIGHT property item pointer.
 */
#define CCD_LENS_FOV_FOV_HEIGHT_ITEM         (CCD_LENS_FOV_PROPERTY->items+1)

/** CCD_LENS_FOV.PIXEL_SCALE_WIDTH property item pointer.
 */
#define CCD_LENS_FOV_PIXEL_SCALE_WIDTH_ITEM  (CCD_LENS_FOV_PROPERTY->items+2)

/** CCD_LENS_FOV.PIXEL_SCALE_HEIGHT property item pointer.
 */
#define CCD_LENS_FOV_PIXEL_SCALE_HEIGHT_ITEM (CCD_LENS_FOV_PROPERTY->items+3)

/** Filter device context structure.
 */
typedef struct {
	indigo_device_context device_context;       ///< device context base
	indigo_device *device;
	indigo_client *client;
	char device_name[INDIGO_FILTER_LIST_COUNT][INDIGO_NAME_SIZE];
	indigo_property *filter_device_list_properties[INDIGO_FILTER_LIST_COUNT];
	indigo_property *filter_related_device_list_properties[INDIGO_FILTER_LIST_COUNT];
	indigo_property *filter_related_agent_list_property;
	indigo_property *filter_force_symmetric_relations_property;
	indigo_property *device_property_cache[INDIGO_FILTER_MAX_CACHED_PROPERTIES];
	indigo_property *agent_property_cache[INDIGO_FILTER_MAX_CACHED_PROPERTIES];
	indigo_property *connection_property_cache[INDIGO_FILTER_MAX_DEVICES];
	bool running_process;
	bool (*validate_related_agent)(indigo_device *device, indigo_property *info_property, int mask);
	bool (*validate_device)(indigo_device *device, int index, indigo_property *info_property, int mask);
	bool (*validate_related_device)(indigo_device *device, int index, indigo_property *info_property, int mask);
	indigo_client *additional_client_instances[MAX_ADDITIONAL_INSTANCES];
	indigo_property *ccd_lens_info_property;
	int frame_width, frame_height;
	int bin_horizontal, bin_vertical;
	double pixel_width, pixel_height;
	double focal_length;
	double fov_width, fov_height;
} indigo_filter_context;

#define INDIGO_FILTER_CCD_SELECTED 				(!FILTER_CCD_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_WHEEL_SELECTED 			(!FILTER_WHEEL_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_FOCUSER_SELECTED 		(!FILTER_FOCUSER_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_ROTATOR_SELECTED 		(!FILTER_ROTATOR_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_MOUNT_SELECTED 			(!FILTER_MOUNT_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_GUIDER_SELECTED 		(!FILTER_GUIDER_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_DOME_SELECTED 			(!FILTER_DOME_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_GPS_SELECTED 				(!FILTER_GPS_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_JOYSTICK_SELECTED 	(!FILTER_JOYSTICK_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_AUX_1_SELECTED 			(!FILTER_AUX_1_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_AUX_2_SELECTED 			(!FILTER_AUX_2_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_AUX_3_SELECTED 			(!FILTER_AUX_3_LIST_PROPERTY->items->sw.value)
#define INDIGO_FILTER_AUX_4_SELECTED 			(!FILTER_AUX_4_LIST_PROPERTY->items->sw.value)

/** Device attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface);
/** Enumerate properties callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_device_detach(indigo_device *device);

/** Client attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_client_attach(indigo_client *client);
/** Client define property callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
/** Client update property callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
/** Client delete property callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
/** Client attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_filter_client_detach(indigo_client *client);
/** Find remote cached properties.
 */
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
__attribute__((deprecated))
#endif
INDIGO_EXTERN bool indigo_filter_cached_property(indigo_device *device, int index, char *name, indigo_property **device_property, indigo_property **agent_property);
/** Forward property change to a different device.
 */
INDIGO_EXTERN indigo_result indigo_filter_forward_change_property(indigo_client *client, indigo_property *property, char *device_name, char *property_name);
/** Find the full name of the first related agent starting with a given base name.
 */
INDIGO_EXTERN char *indigo_filter_first_related_agent(indigo_device *device, char *base_name_1);
/** Find the full name of the first related agent starting with any of given base names.
 */
INDIGO_EXTERN char *indigo_filter_first_related_agent_2(indigo_device *device, char *base_name_1, char *base_name_2);
/** Return index of selected item on given switch and select the new one (or none for NULL).
 */
INDIGO_EXTERN int indigo_save_switch_state(indigo_device *device, char *name, char *new_state);
/** Restore selected item on given switch to index.
 */
INDIGO_EXTERN void indigo_restore_switch_state(indigo_device *device, char *name, int index);

#ifdef __cplusplus
}
#endif

#endif /* indigo_wheel_h */

