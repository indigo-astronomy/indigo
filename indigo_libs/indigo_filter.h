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

#include "indigo_bus.h"
#include "indigo_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INDIGO_FILTER_MAX_DEVICES							32
#define INDIGO_FILTER_MAX_CACHED_PROPERTIES		128
	
/** Device context pointer.
 */
#define FILTER_DEVICE_CONTEXT                ((indigo_filter_context *)device->device_context)

/** Client context pointer.
 */
#define FILTER_CLIENT_CONTEXT                ((indigo_filter_context *)client->client_context)

/** Device list switch property.
 */

#define FILTER_DEVICE_LIST_PROPERTY					(FILTER_DEVICE_CONTEXT->filter_device_list_property)

/** Related ccd list switch property.
*/
#define FILTER_RELATED_CCD_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_ccd_list_property)

/** Related wheel list switch property.
*/
#define FILTER_RELATED_WHEEL_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_wheel_list_property)

/** Related focuser list switch property.
*/
#define FILTER_RELATED_FOCUSER_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_focuser_list_property)

/** Related mount list switch property.
*/
#define FILTER_RELATED_MOUNT_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_mount_list_property)

/** Related guider list switch property.
*/
#define FILTER_RELATED_GUIDER_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_guider_list_property)

/** Related dome list switch property.
*/
#define FILTER_RELATED_DOME_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_dome_list_property)

/** Related gps list switch property.
*/
#define FILTER_RELATED_GPS_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_gps_list_property)

/** Related aux #1 list switch property.
*/
#define FILTER_RELATED_AUX_1_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_aux_1_list_property)

/** Related aux #2 list switch property.
*/
#define FILTER_RELATED_AUX_2_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_aux_2_list_property)

/** Related aux #3 list switch property.
*/
#define FILTER_RELATED_AUX_3_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_aux_3_list_property)

/** Related aux #4 list switch property.
*/
#define FILTER_RELATED_AUX_4_LIST_PROPERTY		(FILTER_DEVICE_CONTEXT->filter_related_aux_4_list_property)

/** Filter device context structure.
 */
typedef struct {
	indigo_device_context device_context;       ///< device context base
	indigo_device *device;
	indigo_client *client;
	indigo_device_interface device_interface;
	char device_name[INDIGO_NAME_SIZE];
	char related_ccd_name[INDIGO_NAME_SIZE];
	char related_wheel_name[INDIGO_NAME_SIZE];
	char related_focuser_name[INDIGO_NAME_SIZE];
	char related_mount_name[INDIGO_NAME_SIZE];
	char related_guider_name[INDIGO_NAME_SIZE];
	char related_dome_name[INDIGO_NAME_SIZE];
	char related_gps_name[INDIGO_NAME_SIZE];
	char related_aux_1_name[INDIGO_NAME_SIZE];
	char related_aux_2_name[INDIGO_NAME_SIZE];
	char related_aux_3_name[INDIGO_NAME_SIZE];
	char related_aux_4_name[INDIGO_NAME_SIZE];
	indigo_property *filter_device_list_property;
	indigo_property *filter_related_ccd_list_property;
	indigo_property *filter_related_wheel_list_property;
	indigo_property *filter_related_focuser_list_property;
	indigo_property *filter_related_mount_list_property;
	indigo_property *filter_related_guider_list_property;
	indigo_property *filter_related_dome_list_property;
	indigo_property *filter_related_gps_list_property;
	indigo_property *filter_related_aux_1_list_property;
	indigo_property *filter_related_aux_2_list_property;
	indigo_property *filter_related_aux_3_list_property;
	indigo_property *filter_related_aux_4_list_property;
	indigo_property *device_property_cache[INDIGO_FILTER_MAX_CACHED_PROPERTIES];
	indigo_property *agent_property_cache[INDIGO_FILTER_MAX_CACHED_PROPERTIES];
} indigo_filter_context;

/** Device attach callback function.
 */
extern indigo_result indigo_filter_device_attach(indigo_device *device, unsigned version, indigo_device_interface device_interface);
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
extern indigo_result indigo_filter_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message);
/** Client update property callback function.
 */
extern indigo_result indigo_filter_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message);
/** Client delete property callback function.
 */
extern indigo_result indigo_filter_delete_property(indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message);
/** Client attach callback function.
 */
extern indigo_result indigo_filter_client_detach(indigo_client *client);

#ifdef __cplusplus
}
#endif

#endif /* indigo_wheel_h */

