// Copyright (c) 2020 by Rumen G.Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>

/** INDIGO Rotator Driver base
 \file indigo_rotator_driver.h
 */

#ifndef indigo_rotator_h
#define indigo_rotator_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Main rotator group name string.
 */
#define ROTATOR_MAIN_GROUP                "Rotator"

/** Device context pointer.
 */
#define ROTATOR_CONTEXT                ((indigo_rotator_context *)device->device_context)

/** ROTATOR_STEPS_PER_REVOLUTION property pointer, property is optional, property change request should be fully handled by indigo_rotator_change_property
 */
#define ROTATOR_STEPS_PER_REVOLUTION_PROPERTY								(ROTATOR_CONTEXT->rotator_steps_per_revolution_property)

/** ROTATOR_STEPS_PER_REVOLUTION.STEPS_PER_REVOLUTION property item pointer.
 */
#define ROTATOR_STEPS_PER_REVOLUTION_ITEM										(ROTATOR_STEPS_PER_REVOLUTION_PROPERTY->items+0)

/** ROTATOR_DIRECTION property pointer, property is optional, property change request should be fully handled by indigo_rotator_change_property.
 */
#define ROTATOR_DIRECTION_PROPERTY							(ROTATOR_CONTEXT->rotator_direction_property)

/** ROTATOR_DIRECTION.NORMAL property item pointer.
 */
#define ROTATOR_DIRECTION_NORMAL_ITEM	(ROTATOR_DIRECTION_PROPERTY->items+0)

/** ROTATOR_DIRECTION.REVERSED property item pointer.
*/
#define ROTATOR_DIRECTION_REVERSED_ITEM				(ROTATOR_DIRECTION_PROPERTY->items+1)

//----------------------------------------------
/** ROTATOR_ON_POSITION_SET property pointer, property is optional, property change request is handled by indigo_rotator_change_property.
*/
#define ROTATOR_ON_POSITION_SET_PROPERTY			(ROTATOR_CONTEXT->rotator_on_position_set_property)

/** ROTATOR_ON_POSITION_SET.GOTO property item pointer.
*/
#define ROTATOR_ON_POSITION_SET_GOTO_ITEM			(ROTATOR_ON_POSITION_SET_PROPERTY->items+0)

/** ROTATOR_ON_POSITION_SET.SYNC property item pointer.
*/
#define ROTATOR_ON_POSITION_SET_SYNC_ITEM			(ROTATOR_ON_POSITION_SET_PROPERTY->items+1)

/** ROTATOR_POSITION property pointer, property is mandatory, property change request should be handled by rotator driver, sync can be handleled by the base class
*/
#define ROTATOR_POSITION_PROPERTY							(ROTATOR_CONTEXT->rotator_position_property)

/** ROTATOR_POSITION.POSITION property item pointer.
*/
#define ROTATOR_POSITION_ITEM									(ROTATOR_POSITION_PROPERTY->items+0)

/** ROTATOR_RELATIVE_MOVE property pointer, property is mandatory, property change request should be fully handled by rotator driver
*/
#define ROTATOR_RELATIVE_MOVE_PROPERTY							(ROTATOR_CONTEXT->rotator_relative_move_property)

/** ROTATOR_RELATIVE_MOVE.RELATIVE_MOVE property item pointer.
*/
#define ROTATOR_RELATIVE_MOVE_ITEM									(ROTATOR_RELATIVE_MOVE_PROPERTY->items+0)

/** ROTATOR_ABORT_MOTION property pointer, property is mandatory, property change request should be fully handled by rotator driver
*/
#define ROTATOR_ABORT_MOTION_PROPERTY					(ROTATOR_CONTEXT->rotator_abort_motion_property)

/** ROTATOR_ABORT_MOTION.ABORT_MOTION property item pointer.
*/
#define ROTATOR_ABORT_MOTION_ITEM							(ROTATOR_ABORT_MOTION_PROPERTY->items+0)

/** ROTATOR_BACKLASH property pointer, property is optional, property change request should be fully handled by rotator driver
*/
#define ROTATOR_BACKLASH_PROPERTY							(ROTATOR_CONTEXT->rotator_backlash_property)

/** ROTATOR_BACKLASH.BACKLASH property item pointer.
*/
#define ROTATOR_BACKLASH_ITEM									(ROTATOR_BACKLASH_PROPERTY->items+0)

/** ROTATOR_LIMITS property pointer, property is optional, property change request should be fully handled by rotator driver
 */
#define ROTATOR_LIMITS_PROPERTY								(ROTATOR_CONTEXT->rotator_limits_property)

/** ROTATOR_LIMITS.MIN_POSITION property item pointer.
 */
#define ROTATOR_LIMITS_MIN_POSITION_ITEM							(ROTATOR_LIMITS_PROPERTY->items+0)

/** ROTATOR_LIMITS.MAX_POSITION property item pointer.
 */
#define ROTATOR_LIMITS_MAX_POSITION_ITEM							(ROTATOR_LIMITS_PROPERTY->items+1)

/** ROTATOR_RAW_POSITION property pointer, property is optional and read only
 */
#define ROTATOR_RAW_POSITION_PROPERTY		(ROTATOR_CONTEXT->rotator_raw_position_property)

/** ROTATOR_RAW_POSITION.POSITION proeprty item pointer.
 */
#define ROTATOR_RAW_POSITION_ITEM 			(ROTATOR_RAW_POSITION_PROPERTY->items + 0)

/** ROTATOR_POSITION_OFFSET property pointer, property is optional, handled by the base class
 */
#define ROTATOR_POSITION_OFFSET_PROPERTY		(ROTATOR_CONTEXT->rotator_position_offset_property)

/** ROTATOR_POSITION_OFFSET.OFFSET proeprty item pointer.
 */
#define ROTATOR_POSITION_OFFSET_ITEM 			(ROTATOR_POSITION_OFFSET_PROPERTY->items + 0)


/** Focuser device context structure.
 */
typedef struct {
	indigo_device_context device_context;							      ///< device context base
	indigo_property *rotator_steps_per_revolution_property; ///< ROTATOR_STEPS_PER_REVOLUTION property pointer
	indigo_property *rotator_direction_property;            ///< ROTATOR_DIRECTION property pointer
	indigo_property *rotator_on_position_set_property;      ///< ROTATOR_ON_POSITION_SET property pointer
	indigo_property *rotator_position_property;             ///< ROTATOR_POSITION property pointer
	indigo_property *rotator_relative_move_property;        ///< ROTATOR_RELATIVE_MOVE property pointer
	indigo_property *rotator_abort_motion_property;         ///< ROTATOR_ABORT_MOTION property pointer
	indigo_property *rotator_backlash_property;             ///< ROTATOR_BACKLASH property pointer
	indigo_property *rotator_limits_property;               ///< ROTATOR_LIMITS property pointer
	indigo_property *rotator_raw_position_property;         ///< ROTATOR_RAW_POSITION property pointer
	indigo_property *rotator_position_offset_property;      ///< ROTATOR_POSITION_OFFSET property pointer
} indigo_rotator_context;

/** Attach callback function.
 */
extern indigo_result indigo_rotator_attach(indigo_device *device, const char* driver_name, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_rotator_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_rotator_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_rotator_detach(indigo_device *device);


/**
 * @brief Normalize an angle to the range [0, 360).
 *
 * This function takes an angle as input and normalizes it to the range [0, 360) degrees. 
 * That is, it adds or subtracts multiples of 360 until the angle is within the range [0, 360).
 *
 * @param angle The angle to be normalized, in degrees.
 * @return The angle normalized to the range [0, 360) degrees.
 */
double indigo_range360(double angle);

#ifdef __cplusplus
}
#endif

#endif /* indigo_rotator_h */
