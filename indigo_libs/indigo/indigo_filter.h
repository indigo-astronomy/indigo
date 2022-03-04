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
 \file indigo_filter.h
 */

#ifndef indigo_filter_h
#define indigo_filter_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INDIGO_FILTER_LIST_COUNT							12
#define INDIGO_FILTER_MAX_DEVICES							32
#define INDIGO_FILTER_MAX_CACHED_PROPERTIES		256
	
#define INDIGO_FILTER_CCD_INDEX								0
#define INDIGO_FILTER_WHEEL_INDEX							1
#define INDIGO_FILTER_FOCUSER_INDEX						2
#define INDIGO_FILTER_MOUNT_INDEX							3
#define INDIGO_FILTER_GUIDER_INDEX						4
#define INDIGO_FILTER_DOME_INDEX							5
#define INDIGO_FILTER_GPS_INDEX								6
#define INDIGO_FILTER_JOYSTICK_INDEX					7
#define INDIGO_FILTER_AUX_1_INDEX							8
#define INDIGO_FILTER_AUX_2_INDEX							9
#define INDIGO_FILTER_AUX_3_INDEX							10
#define INDIGO_FILTER_AUX_4_INDEX							11

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
	indigo_property *device_property_cache[INDIGO_FILTER_MAX_CACHED_PROPERTIES];
	indigo_property *agent_property_cache[INDIGO_FILTER_MAX_CACHED_PROPERTIES];
	indigo_property *connection_property_cache[INDIGO_FILTER_MAX_DEVICES];
	char *connection_property_device_cache[INDIGO_FILTER_MAX_DEVICES];
	bool running_process;
	bool property_removed;
	bool (*validate_related_agent)(indigo_device *device, indigo_property *info_property, int mask);
	bool (*validate_device)(indigo_device *device, int index, indigo_property *info_property, int mask);
	bool (*validate_related_device)(indigo_device *device, int index, indigo_property *info_property, int mask);
	indigo_client *additional_client_instances[MAX_ADDITIONAL_INSTANCES];
} indigo_filter_context;

/** Device attach callback function.
 */
extern indigo_result indigo_filter_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_filter_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_filter_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_filter_device_detach(indigo_device *device);

	
/** Client attach callback function.
 */
extern indigo_result indigo_filter_client_attach(indigo_client *client);
/** Client define property callback function.
 */
extern indigo_result indigo_filter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
/** Client update property callback function.
 */
extern indigo_result indigo_filter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
/** Client delete property callback function.
 */
extern indigo_result indigo_filter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);
/** Client attach callback function.
 */
extern indigo_result indigo_filter_client_detach(indigo_client *client);
/** Find remote cached properties.
 */
extern bool indigo_filter_cached_property(indigo_device *device, int index, char *name, indigo_property **device_property, indigo_property **agent_property);
/** Forward property change to a different device.
 */
extern indigo_result indigo_filter_forward_change_property(indigo_client *client, indigo_property *property, char *device_name);
#ifdef __cplusplus
}
#endif

#endif /* indigo_wheel_h */

