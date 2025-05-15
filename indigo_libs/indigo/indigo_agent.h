// Copyright (c) 2017-2025 CloudMakers, s. r. o.
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

/** INDIGO Generic agent base
 \file indigo_agent_driver.h
 */

#ifndef indigo_agent_h
#define indigo_agent_h

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

/** Main agent group name string.
*/
#define AGENT_MAIN_GROUP									"Agent"

/** Device context pointer.
 */
#define AGENT_CONTEXT                					((indigo_agent_context *)device->device_context)

/** Dome device context structure.
 */
typedef struct {
	indigo_device_context device_context;					///< device context base
} indigo_agent_context;

/** Attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_agent_attach(indigo_device *device, const char* driver_name, unsigned version);
/** Enumerate properties callback function.
 */
INDIGO_EXTERN indigo_result indigo_agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
INDIGO_EXTERN indigo_result indigo_agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
INDIGO_EXTERN indigo_result indigo_agent_detach(indigo_device *device);

/** Add snoop rule.
 */
indigo_result indigo_add_snoop_rule(indigo_property *target, const char *source_device, const char *source_property);

/** Remove snoop rule.
 */
indigo_result indigo_remove_snoop_rule(indigo_property *target, const char *source_device, const char *source_property);
	
#ifdef __cplusplus
}
#endif

#endif /* indigo_agent_h */
