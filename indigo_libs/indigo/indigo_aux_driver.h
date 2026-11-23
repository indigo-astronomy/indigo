// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

/** Weather safety conditions, one per condition item of AUX_WEATHER_SAFETY.
 */
typedef enum {
	INDIGO_AUX_SAFETY_CLOUDS = 0,
	INDIGO_AUX_SAFETY_RAIN,
	INDIGO_AUX_SAFETY_WIND,
	INDIGO_AUX_SAFETY_HUMIDITY,
	INDIGO_AUX_SAFETY_CONDITION_COUNT
} indigo_aux_safety_condition;

/** State of one weather safety condition as measured by the driver.
 *
 * The driver decides whether the reading reached its unsafe level from its own classification
 * (OVERCAST, RAINING, STRONG or HUMID), so the condition light always agrees with AUX_CLOUD,
 * AUX_RAIN, AUX_WIND or AUX_HUMIDITY. A zero-initialized entry is a condition without a sensor.
 */
typedef struct {
	bool fitted;				///< the unit has this sensor (may be known only after connect)
	bool valid;					///< the sensor gave a valid reading
	bool unsafe;				///< the reading is at or past its unsafe level
	bool reversed;			///< a lower reading is worse, e.g. the resistance of a rain sensor
	double value;				///< the reading, used in the reason message
	double limit;				///< the unsafe level, used in the reason message
	const char *unit;		///< unit of value and limit used in the reason message, e.g. "m/s", or NULL
} indigo_aux_safety_input;

/** Evaluate the weather safety conditions into the lights of AUX_WEATHER_SAFETY.
 *
 * A condition item is IDLE without a fitted sensor, OK below its unsafe level and ALERT at or past it or
 * without a valid reading. SAFE is ALERT if any condition is ALERT, OK if every fitted condition is OK and
 * IDLE if no condition is fitted. The property state is OK, or ALERT if a fitted sensor has no valid reading.
 * Items are looked up by name, missing items are ignored. The previous lights and property state serve as the previous
 * result, so call it with NULL message to set the initial lights before the property is defined.
 * @param property AUX_WEATHER_SAFETY property
 * @param conditions INDIGO_AUX_SAFETY_CONDITION_COUNT conditions indexed by indigo_aux_safety_condition
 * @param message buffer for the reason message, may be NULL
 * @param message_size size of the message buffer
 * @return true if the SAFE verdict changed, or while it is ALERT, if the set of ALERT conditions or the validity of the readings changed; the reason is then written to message
 */
INDIGO_EXTERN bool indigo_aux_evaluate_weather_safety(indigo_property *property, const indigo_aux_safety_input *conditions, char *message, size_t message_size);

/** Evaluate the weather safety conditions and publish AUX_WEATHER_SAFETY, with the reason message when indigo_aux_evaluate_weather_safety() reports a change.
 */
INDIGO_EXTERN void indigo_aux_update_weather_safety(indigo_device *device, indigo_property *property, const indigo_aux_safety_input *conditions);

#ifdef __cplusplus
}
#endif

#endif /* indigo_aux_h */

