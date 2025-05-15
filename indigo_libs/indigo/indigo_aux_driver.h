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

/** INDIGO Filter aux Driver base
 \file indigo_aux_driver.h
 */

#ifndef indigo_aux_h
#define indigo_aux_h

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

/** Main aux group name string.
 */
#define AUX_MAIN_GROUP                "AUX"

/** Advanced group name string.
 */
#define AUX_ADVANCED_GROUP            ADVANCED_GROUP

/** Attach callback function.
 */
INDIGO_EXTERN indigo_result indigo_aux_attach(indigo_device *device, const char* driver_name, unsigned version, int interface);
/** Enumerate properties callback function.
 */
INDIGO_EXTERN indigo_result indigo_aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
INDIGO_EXTERN indigo_result indigo_aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
INDIGO_EXTERN indigo_result indigo_aux_detach(indigo_device *device);

/** Calculate dewpoint in degC from temperature in degC and relative humidity in % using Magnus formula
*/
INDIGO_EXTERN double indigo_aux_dewpoint(double temperature, double rh);

/** Calculate approximate Bortle dark sky scale from sky brigthness in mag/arcsec^2
 * @param sky_brightness sky brigthness in mag/arcsec^2
 * @return Bortle dark sky scale (1, 2, 3, 4, 4.5, 5, 6, 7, 8 or 9)
 */
INDIGO_EXTERN float indigo_aux_sky_bortle(double sky_brightness);

#ifdef __cplusplus
}
#endif

#endif /* indigo_aux_h */

