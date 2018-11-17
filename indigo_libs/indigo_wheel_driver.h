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

/** INDIGO Filter wheel Driver base
 \file indigo_wheel_driver.h
 */

#ifndef indigo_wheel_h
#define indigo_wheel_h

#include "indigo_bus.h"
#include "indigo_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Main wheel group name string.
 */
#define WHEEL_MAIN_GROUP                "Filter Wheel"

/** Device context pointer.
 */
#define WHEEL_CONTEXT                ((indigo_wheel_context *)device->device_context)

/** WHEEL_SLOT property pointer, property is mandatory, property change request should be fully handled by device driver.
 */
#define WHEEL_SLOT_PROPERTY									(WHEEL_CONTEXT->wheel_slot_property)

/** WHEEL_SLOT.SLOT property item pointer.
 */
#define WHEEL_SLOT_ITEM											(WHEEL_SLOT_PROPERTY->items+0)

/** WHEEL_SLOT_NAME property pointer, property is mandatory, property change request should be fully handled by indigo_wheel_change_property.
 */
#define WHEEL_SLOT_NAME_PROPERTY             (WHEEL_CONTEXT->wheel_slot_name_property)

/** WHEEL_SLOT_NAME.NAME_1 property item pointer.
 */
#define WHEEL_SLOT_NAME_1_ITEM               (WHEEL_SLOT_NAME_PROPERTY->items+0)

/** WHEEL_SLOT_OFFSET property pointer, property is mandatory, property change request should be fully handled by indigo_wheel_change_property.
*/
#define WHEEL_SLOT_OFFSET_PROPERTY             (WHEEL_CONTEXT->wheel_slot_offset_property)

/** WHEEL_SLOT_NAME.NAME_1 property item pointer.
*/
#define WHEEL_SLOT_OFFSET_1_ITEM               (WHEEL_SLOT_OFFSET_PROPERTY->items+0)

/** Wheel device context structure.
 */
typedef struct {
	indigo_device_context device_context;       ///< device context base
	indigo_property *wheel_slot_property;				///< WHEEL_SLOT property pointer
	indigo_property *wheel_slot_name_property;  ///< WHEEL_SLOT_NAME property pointer
	indigo_property *wheel_slot_offset_property;///< WHEEL_SLOT_OFFSET property pointer
} indigo_wheel_context;

/** Attach callback function.
 */
extern indigo_result indigo_wheel_attach(indigo_device *device, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_wheel_detach(indigo_device *device);

#ifdef __cplusplus
}
#endif

#endif /* indigo_wheel_h */

