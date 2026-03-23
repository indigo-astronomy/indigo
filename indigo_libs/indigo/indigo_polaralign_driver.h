// Copyright (c) 2026 by Rumen G.Bogdanovski
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

/** INDIGO Polar Aligner Driver base
 \file indigo_polaralign_driver.h
 */

#ifndef indigo_polaralign_h
#define indigo_polaralign_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Main polar aligner group name string.
 */
#define POLARALIGN_MAIN_GROUP		"Polar Aligner"

/** Advanced group name string.
 */
#define POLARALIGN_ADVANCED_GROUP	ADVANCED_GROUP

/** Device context pointer.
 */
#define POLARALIGN_CONTEXT			((indigo_polaralign_context *)device->device_context)

//----------------------------------------------------------------------

/** POLARALIGN_OFFSET property pointer.
 *  Mandatory. Change requests must be handled by the driver.
 */
#define POLARALIGN_OFFSET_PROPERTY					(POLARALIGN_CONTEXT->polaralign_offset_property)

/** POLARALIGN_OFFSET.ALTITUDE_OFFSET item pointer.
 */
#define POLARALIGN_OFFSET_ALTITUDE_ITEM			(POLARALIGN_OFFSET_PROPERTY->items+0)

/** POLARALIGN_OFFSET.AZIMUTH_OFFSET item pointer.
 */
#define POLARALIGN_OFFSET_AZIMUTH_ITEM				(POLARALIGN_OFFSET_PROPERTY->items+1)

//----------------------------------------------------------------------

/** POLARALIGN_ABORT_MOTION property pointer.
 *  Mandatory. Change requests must be handled by the driver.
 */
#define POLARALIGN_ABORT_MOTION_PROPERTY			(POLARALIGN_CONTEXT->polaralign_abort_motion_property)

/** POLARALIGN_ABORT_MOTION.ABORT_MOTION item pointer.
 */
#define POLARALIGN_ABORT_MOTION_ITEM				(POLARALIGN_ABORT_MOTION_PROPERTY->items+0)

//----------------------------------------------------------------------

/** POLARALIGN_STEPS_PER_DEGREE property pointer.
 */
#define POLARALIGN_STEPS_PER_DEGREE_PROPERTY			(POLARALIGN_CONTEXT->polaralign_steps_per_degree_property)

/** POLARALIGN_STEPS_PER_DEGREE.ALT item pointer.
 */
#define POLARALIGN_STEPS_PER_DEGREE_ALT_ITEM			(POLARALIGN_STEPS_PER_DEGREE_PROPERTY->items+0)

/** POLARALIGN_STEPS_PER_DEGREE.AZ item pointer.
 */
#define POLARALIGN_STEPS_PER_DEGREE_AZ_ITEM			(POLARALIGN_STEPS_PER_DEGREE_PROPERTY->items+1)

//----------------------------------------------------------------------

/** POLARALIGN_DIRECTION_ALT property pointer.
 */
#define POLARALIGN_DIRECTION_ALT_PROPERTY			(POLARALIGN_CONTEXT->polaralign_direction_alt_property)

/** POLARALIGN_DIRECTION_ALT.NORMAL item pointer.
 */
#define POLARALIGN_DIRECTION_ALT_NORMAL_ITEM			(POLARALIGN_DIRECTION_ALT_PROPERTY->items+0)

/** POLARALIGN_DIRECTION_ALT.REVERSED item pointer.
 */
#define POLARALIGN_DIRECTION_ALT_REVERSED_ITEM			(POLARALIGN_DIRECTION_ALT_PROPERTY->items+1)

//----------------------------------------------------------------------

/** POLARALIGN_DIRECTION_AZ property pointer.
 */
#define POLARALIGN_DIRECTION_AZ_PROPERTY			(POLARALIGN_CONTEXT->polaralign_direction_az_property)

/** POLARALIGN_DIRECTION_AZ.NORMAL item pointer.
 */
#define POLARALIGN_DIRECTION_AZ_NORMAL_ITEM			(POLARALIGN_DIRECTION_AZ_PROPERTY->items+0)

/** POLARALIGN_DIRECTION_AZ.REVERSED item pointer.
 */
#define POLARALIGN_DIRECTION_AZ_REVERSED_ITEM			(POLARALIGN_DIRECTION_AZ_PROPERTY->items+1)

//----------------------------------------------------------------------

/** POLARALIGN_RESET_POSITION_ALT property pointer.
 */
#define POLARALIGN_RESET_POSITION_ALT_PROPERTY			(POLARALIGN_CONTEXT->polaralign_reset_position_alt_property)

/** POLARALIGN_RESET_POSITION_ALT.RESET item pointer.
 */
#define POLARALIGN_RESET_POSITION_ALT_ITEM			(POLARALIGN_RESET_POSITION_ALT_PROPERTY->items+0)

//----------------------------------------------------------------------

/** POLARALIGN_RESET_POSITION_AZ property pointer.
 */
#define POLARALIGN_RESET_POSITION_AZ_PROPERTY			(POLARALIGN_CONTEXT->polaralign_reset_position_az_property)

/** POLARALIGN_RESET_POSITION_AZ.RESET item pointer.
 */
#define POLARALIGN_RESET_POSITION_AZ_ITEM			(POLARALIGN_RESET_POSITION_AZ_PROPERTY->items+0)

//----------------------------------------------------------------------

/** POLARALIGN_LIMITS property pointer.
 */
#define POLARALIGN_LIMITS_PROPERTY				(POLARALIGN_CONTEXT->polaralign_limits_property)

/** POLARALIGN_LIMITS.MIN_POSITION_ALT item pointer.
 */
#define POLARALIGN_LIMITS_MIN_POSITION_ALT_ITEM			(POLARALIGN_LIMITS_PROPERTY->items+0)

/** POLARALIGN_LIMITS.MAX_POSITION_ALT item pointer.
 */
#define POLARALIGN_LIMITS_MAX_POSITION_ALT_ITEM			(POLARALIGN_LIMITS_PROPERTY->items+1)

/** POLARALIGN_LIMITS.MIN_POSITION_AZ item pointer.
 */
#define POLARALIGN_LIMITS_MIN_POSITION_AZ_ITEM			(POLARALIGN_LIMITS_PROPERTY->items+2)

/** POLARALIGN_LIMITS.MAX_POSITION_AZ item pointer.
 */
#define POLARALIGN_LIMITS_MAX_POSITION_AZ_ITEM			(POLARALIGN_LIMITS_PROPERTY->items+3)

//----------------------------------------------------------------------

/** Polar aligner device context structure.
 */
typedef struct {
	indigo_device_context device_context;					///< device context base
	indigo_property *polaralign_offset_property;			///< POLARALIGN_OFFSET property pointer
	indigo_property *polaralign_abort_motion_property;		///< POLARALIGN_ABORT_MOTION property pointer
	indigo_property *polaralign_steps_per_degree_property;	///< POLARALIGN_STEPS_PER_DEGREE property pointer
	indigo_property *polaralign_direction_alt_property;		///< POLARALIGN_DIRECTION_ALT property pointer
	indigo_property *polaralign_direction_az_property;		///< POLARALIGN_DIRECTION_AZ property pointer
	indigo_property *polaralign_reset_position_alt_property;	///< POLARALIGN_RESET_POSITION_ALT property pointer
	indigo_property *polaralign_reset_position_az_property;		///< POLARALIGN_RESET_POSITION_AZ property pointer
	indigo_property *polaralign_limits_property;				///< POLARALIGN_LIMITS property pointer
} indigo_polaralign_context;

/** Attach callback function.
 */
extern indigo_result indigo_polaralign_attach(indigo_device *device, const char *driver_name, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_polaralign_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_polaralign_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_polaralign_detach(indigo_device *device);

#ifdef __cplusplus
}
#endif

#endif /* indigo_polaralign_h */
