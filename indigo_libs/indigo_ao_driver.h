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

/** INDIGO AO Driver base
 \file indigo_ao_driver.h
 */

#ifndef indigo_ao_h
#define indigo_ao_h

#include "indigo_bus.h"
#include "indigo_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Main ao group name string.
 */
#define AO_MAIN_GROUP                	"Guider"

/** Device context pointer.
 */
#define AO_CONTEXT                		((indigo_ao_context *)device->device_context)

/** AO_GUIDE_DEC property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define AO_GUIDE_DEC_PROPERTY         (AO_CONTEXT->ao_guide_dec_property)

/** AO_GUIDE_DEC.NORTH property item pointer.
 */
#define AO_GUIDE_NORTH_ITEM           (AO_GUIDE_DEC_PROPERTY->items+0)

/** AO_GUIDE_DEC.SOUTH property item pointer.
 */
#define AO_GUIDE_SOUTH_ITEM           (AO_GUIDE_DEC_PROPERTY->items+1)
	
/** AO_GUIDE_RA property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define AO_GUIDE_RA_PROPERTY          (AO_CONTEXT->ao_guide_ra_property)
	
/** AO_GUIDE_RA.EAST property item pointer.
 */
#define AO_GUIDE_EAST_ITEM            (AO_GUIDE_RA_PROPERTY->items+0)

/** AO_GUIDE_RA.WEST property item pointer.
 */
#define AO_GUIDE_WEST_ITEM            (AO_GUIDE_RA_PROPERTY->items+1)

/** AO_RESET property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define AO_RESET_PROPERTY             (AO_CONTEXT->ao_reset_property)

/** AO_RESET.CENTER property item pointer.
 */
#define AO_CENTER_ITEM             		(AO_RESET_PROPERTY->items+0)

/** AO_RESET.UNJAM property item pointer.
 */
#define AO_UNJAM_ITEM              		(AO_RESET_PROPERTY->items+1)

	
/** Guider device context structure.
 */
typedef struct {
	indigo_device_context device_context;         ///< device context base
	indigo_property *ao_guide_dec_property;				///< AO_GUIDE_DEC property pointer
	indigo_property *ao_guide_ra_property;				///< AO_GUIDE_RA property pointer
	indigo_property *ao_reset_property;						///< AO_RESET property pointer
} indigo_ao_context;

/** Attach callback function.
 */
extern indigo_result indigo_ao_attach(indigo_device *device, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_ao_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_ao_detach(indigo_device *device);

#ifdef __cplusplus
}
#endif

#endif /* indigo_ao_h */

