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

/** INDIGO Guider Driver base
 \file indigo_guider_driver.h
 */

#ifndef indigo_guider_h
#define indigo_guider_h

#include "indigo_bus.h"
#include "indigo_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Main guider group name string.
 */
#define GUIDER_MAIN_GROUP                "Guider"

/** Device context pointer.
 */
#define GUIDER_CONTEXT                ((indigo_guider_context *)device->device_context)

/** GUIDER_GUIDE_DEC property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define GUIDER_GUIDE_DEC_PROPERTY            (GUIDER_CONTEXT->guider_guide_dec_property)

/** GUIDER_GUIDE.NORTH property item pointer.
 */
#define GUIDER_GUIDE_NORTH_ITEM              (GUIDER_GUIDE_DEC_PROPERTY->items+0)

/** GUIDER_GUIDE.SOUTH property item pointer.
 */
#define GUIDER_GUIDE_SOUTH_ITEM              (GUIDER_GUIDE_DEC_PROPERTY->items+1)

/** GUIDER_GUIDE_RA property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define GUIDER_GUIDE_RA_PROPERTY             (GUIDER_CONTEXT->guider_guide_ra_property)

/** GUIDER_GUIDE.EAST property item pointer.
 */
#define GUIDER_GUIDE_EAST_ITEM               (GUIDER_GUIDE_RA_PROPERTY->items+0)

/** GUIDER_GUIDE.WEST property item pointer.
 */
#define GUIDER_GUIDE_WEST_ITEM               (GUIDER_GUIDE_RA_PROPERTY->items+1)

/** GUIDER_RATE property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define GUIDER_RATE_PROPERTY             			(GUIDER_CONTEXT->guider_rate_property)

/** GUIDER_RATE.RATE property item pointer.
 */
#define GUIDER_RATE_ITEM               				(GUIDER_RATE_PROPERTY->items+0)

/** GUIDER_RATE.DEC_RATE property item pointer.
 */
#define GUIDER_DEC_RATE_ITEM               		(GUIDER_RATE_PROPERTY->items+1)


	
/** Guider device context structure.
 */
typedef struct {
	indigo_device_context device_context;         ///< device context base
	indigo_property *guider_guide_dec_property;   ///< GUIDER_GUIDE_DEC property pointer
	indigo_property *guider_guide_ra_property;    ///< GUIDER_GUIDE_RA property pointer
	indigo_property *guider_rate_property;  			///< GUIDER_RATE property pointer
} indigo_guider_context;

/** Attach callback function.
 */
extern indigo_result indigo_guider_attach(indigo_device *device, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_guider_detach(indigo_device *device);

#ifdef __cplusplus
}
#endif

#endif /* indigo_guider_h */

