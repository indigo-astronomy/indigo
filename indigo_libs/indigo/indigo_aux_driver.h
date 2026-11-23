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

/** Weather condition group name string.
 */
#define AUX_WEATHER_GROUP             "Weather"

/** Weather threshold group name string.
 */
#define AUX_THRESHOLDS_GROUP          "Thresholds"

/** Device context pointer.
 */
#define AUX_CONTEXT                   ((indigo_aux_context *)device->device_context)

/** AUX_HUMIDITY property pointer, property is optional (hidden by default), read-only light vector, the item of the current level is OK, the other items IDLE.
 */
#define AUX_HUMIDITY_PROPERTY										(AUX_CONTEXT->aux_humidity_property)

/** AUX_HUMIDITY.HUMID property item pointer.
 */
#define AUX_HUMIDITY_HUMID_ITEM									(AUX_HUMIDITY_PROPERTY->items+0)

/** AUX_HUMIDITY.NORMAL property item pointer.
 */
#define AUX_HUMIDITY_NORMAL_ITEM								(AUX_HUMIDITY_PROPERTY->items+1)

/** AUX_HUMIDITY.DRY property item pointer.
 */
#define AUX_HUMIDITY_DRY_ITEM										(AUX_HUMIDITY_PROPERTY->items+2)

/** AUX_WIND property pointer, property is optional (hidden by default), read-only light vector, the item of the current level is OK, the other items IDLE.
 */
#define AUX_WIND_PROPERTY												(AUX_CONTEXT->aux_wind_property)

/** AUX_WIND.STRONG property item pointer.
 */
#define AUX_WIND_STRONG_ITEM										(AUX_WIND_PROPERTY->items+0)

/** AUX_WIND.MODERATE property item pointer.
 */
#define AUX_WIND_MODERATE_ITEM									(AUX_WIND_PROPERTY->items+1)

/** AUX_WIND.CALM property item pointer.
 */
#define AUX_WIND_CALM_ITEM											(AUX_WIND_PROPERTY->items+2)

/** AUX_RAIN property pointer, property is optional (hidden by default), read-only light vector, the item of the current level is OK, the other items IDLE.
 */
#define AUX_RAIN_PROPERTY												(AUX_CONTEXT->aux_rain_property)

/** AUX_RAIN.RAINING property item pointer.
 */
#define AUX_RAIN_RAINING_ITEM										(AUX_RAIN_PROPERTY->items+0)

/** AUX_RAIN.WET property item pointer.
 */
#define AUX_RAIN_WET_ITEM												(AUX_RAIN_PROPERTY->items+1)

/** AUX_RAIN.DRY property item pointer.
 */
#define AUX_RAIN_DRY_ITEM												(AUX_RAIN_PROPERTY->items+2)

/** AUX_CLOUD property pointer, property is optional (hidden by default), read-only light vector, the item of the current level is OK, the other items IDLE.
 */
#define AUX_CLOUD_PROPERTY											(AUX_CONTEXT->aux_cloud_property)

/** AUX_CLOUD.CLEAR property item pointer.
 */
#define AUX_CLOUD_CLEAR_ITEM										(AUX_CLOUD_PROPERTY->items+0)

/** AUX_CLOUD.CLOUDY property item pointer.
 */
#define AUX_CLOUD_CLOUDY_ITEM										(AUX_CLOUD_PROPERTY->items+1)

/** AUX_CLOUD.OVERCAST property item pointer.
 */
#define AUX_CLOUD_OVERCAST_ITEM									(AUX_CLOUD_PROPERTY->items+2)

/** AUX_SKY property pointer, property is optional (hidden by default), read-only light vector, the item of the current level is OK, the other items IDLE.
 */
#define AUX_SKY_PROPERTY												(AUX_CONTEXT->aux_sky_property)

/** AUX_SKY.DARK property item pointer.
 */
#define AUX_SKY_DARK_ITEM												(AUX_SKY_PROPERTY->items+0)

/** AUX_SKY.LIGHT property item pointer.
 */
#define AUX_SKY_LIGHT_ITEM											(AUX_SKY_PROPERTY->items+1)

/** AUX_SKY.VERY_LIGHT property item pointer.
 */
#define AUX_SKY_VERY_LIGHT_ITEM									(AUX_SKY_PROPERTY->items+2)

/** AUX_HUMIDITY_THRESHOLDS property pointer, property is optional (hidden by default), property change request is fully handled by indigo_aux_change_property, saved by CONFIG.
 */
#define AUX_HUMIDITY_THRESHOLDS_PROPERTY				(AUX_CONTEXT->aux_humidity_thresholds_property)

/** AUX_HUMIDITY_THRESHOLDS.HUMID property item pointer.
 */
#define AUX_HUMIDITY_HUMID_THRESHOLD_ITEM				(AUX_HUMIDITY_THRESHOLDS_PROPERTY->items+0)

/** AUX_HUMIDITY_THRESHOLDS.NORMAL property item pointer.
 */
#define AUX_HUMIDITY_NORMAL_THRESHOLD_ITEM			(AUX_HUMIDITY_THRESHOLDS_PROPERTY->items+1)

/** AUX_WIND_THRESHOLDS property pointer, property is optional (hidden by default), property change request is fully handled by indigo_aux_change_property, saved by CONFIG.
 */
#define AUX_WIND_THRESHOLDS_PROPERTY						(AUX_CONTEXT->aux_wind_thresholds_property)

/** AUX_WIND_THRESHOLDS.STRONG property item pointer.
 */
#define AUX_WIND_STRONG_THRESHOLD_ITEM					(AUX_WIND_THRESHOLDS_PROPERTY->items+0)

/** AUX_WIND_THRESHOLDS.MODERATE property item pointer.
 */
#define AUX_WIND_MODERATE_THRESHOLD_ITEM				(AUX_WIND_THRESHOLDS_PROPERTY->items+1)

/** AUX_RAIN_THRESHOLDS property pointer, property is optional (hidden by default), property change request is fully handled by indigo_aux_change_property, saved by CONFIG.
 */
#define AUX_RAIN_THRESHOLDS_PROPERTY						(AUX_CONTEXT->aux_rain_thresholds_property)

/** AUX_RAIN_THRESHOLDS.RAINING property item pointer.
 */
#define AUX_RAIN_RAINING_THRESHOLD_ITEM					(AUX_RAIN_THRESHOLDS_PROPERTY->items+0)

/** AUX_RAIN_THRESHOLDS.WET property item pointer.
 */
#define AUX_RAIN_WET_THRESHOLD_ITEM							(AUX_RAIN_THRESHOLDS_PROPERTY->items+1)

/** AUX_CLOUD_THRESHOLDS property pointer, property is optional (hidden by default), property change request is fully handled by indigo_aux_change_property, saved by CONFIG.
 */
#define AUX_CLOUD_THRESHOLDS_PROPERTY						(AUX_CONTEXT->aux_cloud_thresholds_property)

/** AUX_CLOUD_THRESHOLDS.CLEAR property item pointer.
 */
#define AUX_CLOUD_CLEAR_THRESHOLD_ITEM					(AUX_CLOUD_THRESHOLDS_PROPERTY->items+0)

/** AUX_CLOUD_THRESHOLDS.CLOUDY property item pointer.
 */
#define AUX_CLOUD_CLOUDY_THRESHOLD_ITEM					(AUX_CLOUD_THRESHOLDS_PROPERTY->items+1)

/** AUX_SKY_THRESHOLDS property pointer, property is optional (hidden by default), property change request is fully handled by indigo_aux_change_property, saved by CONFIG.
 */
#define AUX_SKY_THRESHOLDS_PROPERTY							(AUX_CONTEXT->aux_sky_thresholds_property)

/** AUX_SKY_THRESHOLDS.DARK property item pointer.
 */
#define AUX_SKY_DARK_THRESHOLD_ITEM							(AUX_SKY_THRESHOLDS_PROPERTY->items+0)

/** AUX_SKY_THRESHOLDS.LIGHT property item pointer.
 */
#define AUX_SKY_LIGHT_THRESHOLD_ITEM						(AUX_SKY_THRESHOLDS_PROPERTY->items+1)

/** AUX_WEATHER_SAFETY property pointer, property is optional (hidden by default), read-only light vector set with indigo_aux_evaluate_weather_safety() and indigo_aux_update_weather_safety().
 */
#define AUX_WEATHER_SAFETY_PROPERTY							(AUX_CONTEXT->aux_weather_safety_property)

/** AUX_WEATHER_SAFETY.SAFE property item pointer.
 */
#define AUX_WEATHER_SAFETY_SAFE_ITEM						(AUX_WEATHER_SAFETY_PROPERTY->items+0)

/** AUX_WEATHER_SAFETY.CLOUDS property item pointer.
 */
#define AUX_WEATHER_SAFETY_CLOUDS_ITEM					(AUX_WEATHER_SAFETY_PROPERTY->items+1)

/** AUX_WEATHER_SAFETY.RAIN property item pointer.
 */
#define AUX_WEATHER_SAFETY_RAIN_ITEM						(AUX_WEATHER_SAFETY_PROPERTY->items+2)

/** AUX_WEATHER_SAFETY.WIND property item pointer.
 */
#define AUX_WEATHER_SAFETY_WIND_ITEM						(AUX_WEATHER_SAFETY_PROPERTY->items+3)

/** AUX_WEATHER_SAFETY.HUMIDITY property item pointer.
 */
#define AUX_WEATHER_SAFETY_HUMIDITY_ITEM				(AUX_WEATHER_SAFETY_PROPERTY->items+4)

/** AUX device context structure.
 *
 * The weather condition, threshold and safety properties are hidden by default. A weather driver unhides those
 * it supports in its attach callback and sets the threshold labels, ranges and defaults in its own units.
 * The thresholds are defined while the device is attached and saved by CONFIG, the condition and safety
 * properties are defined while the device is connected.
 */
typedef struct {
	indigo_device_context device_context;										///< device context base
	indigo_property *aux_humidity_property;									///< AUX_HUMIDITY property pointer
	indigo_property *aux_wind_property;											///< AUX_WIND property pointer
	indigo_property *aux_rain_property;											///< AUX_RAIN property pointer
	indigo_property *aux_cloud_property;										///< AUX_CLOUD property pointer
	indigo_property *aux_sky_property;											///< AUX_SKY property pointer
	indigo_property *aux_humidity_thresholds_property;			///< AUX_HUMIDITY_THRESHOLDS property pointer
	indigo_property *aux_wind_thresholds_property;					///< AUX_WIND_THRESHOLDS property pointer
	indigo_property *aux_rain_thresholds_property;					///< AUX_RAIN_THRESHOLDS property pointer
	indigo_property *aux_cloud_thresholds_property;					///< AUX_CLOUD_THRESHOLDS property pointer
	indigo_property *aux_sky_thresholds_property;						///< AUX_SKY_THRESHOLDS property pointer
	indigo_property *aux_weather_safety_property;						///< AUX_WEATHER_SAFETY property pointer
} indigo_aux_context;

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

/** Set the lights and the state of a weather condition property (AUX_HUMIDITY, AUX_WIND, AUX_RAIN, AUX_CLOUD or AUX_SKY) without publishing it.
 *
 * With INDIGO_OK_STATE (the sensor operates normally) the level item is OK and all other items are IDLE.
 * With INDIGO_ALERT_STATE (the sensor gives no valid reading), INDIGO_IDLE_STATE (the sensor is not present) or
 * any other state all items are IDLE. The property state is set to state.
 * @param property condition property
 * @param state sensor state
 * @param level item of the current level, used only with INDIGO_OK_STATE
 */
INDIGO_EXTERN void indigo_aux_set_condition(indigo_property *property, indigo_property_state state, indigo_item *level);

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
 * (the OVERCAST, RAINING, STRONG or HUMID light is OK), so the condition light always agrees with AUX_CLOUD,
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
	bool omit_values;		///< the reason message names the condition without value and limit, for readings that mean nothing to the user
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

