// Copyright (c) 2021 CloudMakers, s. r. o.
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

/** INDIGO Generic platesolver base
 \file indigo_platesolver_driver.h
 */

#ifndef indigo_platesolver_h
#define indigo_platesolver_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLATESOLVER_MAIN_GROUP		"Plate solver"

#define INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA										((platesolver_private_data *)device->private_data)
#define INDIGO_PLATESOLVER_CLIENT_PRIVATE_DATA										((platesolver_private_data *)FILTER_CLIENT_CONTEXT->device->private_data)

#define AGENT_PLATESOLVER_USE_INDEX_PROPERTY	(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->use_index_property)

#define AGENT_PLATESOLVER_HINTS_PROPERTY			(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->hints_property)
#define AGENT_PLATESOLVER_HINTS_RADIUS_ITEM 	(AGENT_PLATESOLVER_HINTS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_HINTS_RA_ITEM    		(AGENT_PLATESOLVER_HINTS_PROPERTY->items+1)
#define AGENT_PLATESOLVER_HINTS_DEC_ITEM    	(AGENT_PLATESOLVER_HINTS_PROPERTY->items+2)
#define AGENT_PLATESOLVER_HINTS_PARITY_ITEM   (AGENT_PLATESOLVER_HINTS_PROPERTY->items+3)
#define AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM (AGENT_PLATESOLVER_HINTS_PROPERTY->items+4)
#define AGENT_PLATESOLVER_HINTS_DEPTH_ITEM    (AGENT_PLATESOLVER_HINTS_PROPERTY->items+5)
#define AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM (AGENT_PLATESOLVER_HINTS_PROPERTY->items+6)

#define AGENT_PLATESOLVER_WCS_PROPERTY				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->wcs_property)
#define AGENT_PLATESOLVER_WCS_RA_ITEM    			(AGENT_PLATESOLVER_WCS_PROPERTY->items+0)
#define AGENT_PLATESOLVER_WCS_DEC_ITEM    		(AGENT_PLATESOLVER_WCS_PROPERTY->items+1)
#define AGENT_PLATESOLVER_WCS_ANGLE_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+2)
#define AGENT_PLATESOLVER_WCS_WIDTH_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+3)
#define AGENT_PLATESOLVER_WCS_HEIGHT_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+4)
#define AGENT_PLATESOLVER_WCS_SCALE_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+5)
#define AGENT_PLATESOLVER_WCS_PARITY_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+6)
#define AGENT_PLATESOLVER_WCS_INDEX_ITEM    	(AGENT_PLATESOLVER_WCS_PROPERTY->items+7)

#define AGENT_PLATESOLVER_SYNC_PROPERTY				(INDIGO_PLATESOLVER_DEVICE_PRIVATE_DATA->sync_mode_property)
#define AGENT_PLATESOLVER_SYNC_DISABLED_ITEM	(AGENT_PLATESOLVER_SYNC_PROPERTY->items+0)
#define AGENT_PLATESOLVER_SYNC_SYNC_ITEM			(AGENT_PLATESOLVER_SYNC_PROPERTY->items+1)
#define AGENT_PLATESOLVER_SYNC_CENTER_ITEM		(AGENT_PLATESOLVER_SYNC_PROPERTY->items+2)

/** Plate solver  structure.
 */
typedef struct {
	indigo_device *device;
	void *image;
	unsigned long size;
} indigo_platesolver_task;

/** Platesolver private data structure.
 */
typedef struct {
	indigo_property *use_index_property;
	indigo_property *hints_property;
	indigo_property *wcs_property;
	indigo_property *sync_mode_property;
	void (*save_config)(indigo_device *);
	void *((*solve)(indigo_platesolver_task *));
	pthread_mutex_t mutex;
	bool failed;
} platesolver_private_data;

extern void indigo_platesolver_save_config(indigo_device *device);
extern void indigo_platesolver_sync(indigo_device *device);

/** Device attach callback function.
 */
extern indigo_result indigo_platesolver_device_attach(indigo_device *device, const char* driver_name, unsigned version, indigo_device_interface device_interface);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_platesolver_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_platesolver_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_platesolver_device_detach(indigo_device *device);

#define indigo_platesolver_client_attach indigo_filter_client_attach
#define indigo_platesolver_define_property indigo_filter_define_property
#define indigo_platesolver_delete_property indigo_filter_delete_property
#define indigo_platesolver_client_detach indigo_filter_client_detach

/** Client update property callback function.
 */
extern indigo_result indigo_platesolver_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* indigo_platesolver_h */
