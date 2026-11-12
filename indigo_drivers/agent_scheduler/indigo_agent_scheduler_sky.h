// Copyright (c) 2026 Rumen G. Bogdanovski
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
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO Scheduler agent ephemeris grid and constraints
 \file indigo_agent_scheduler_sky.h
 */

#ifndef agent_scheduler_sky_h
#define agent_scheduler_sky_h

#include <stdbool.h>
#include <time.h>

#include "indigo_agent_scheduler_header.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_GRID_STEP				300
#define SCHEDULER_GRID_POINTS			577
#define SCHEDULER_SETTING_HORIZON	(12 * 3600)

/** Sun, Moon and sidereal time for 48 hours at SCHEDULER_GRID_STEP intervals.
 */
typedef struct {
	bool valid;
	double latitude, longitude, elevation;
	time_t start;
	double sun_altitude[SCHEDULER_GRID_POINTS];
	double moon_altitude[SCHEDULER_GRID_POINTS];
	double moon_ra[SCHEDULER_GRID_POINTS];
	double moon_dec[SCHEDULER_GRID_POINTS];
	double moon_days_from_full[SCHEDULER_GRID_POINTS];
	double moon_illumination[SCHEDULER_GRID_POINTS];
	double lst[SCHEDULER_GRID_POINTS];
} scheduler_sky_grid;

/** Target altitude, hour angle and moon separation on the points of a sky grid.
 */
typedef struct {
	bool valid;
	time_t start;
	double altitude[SCHEDULER_GRID_POINTS];
	double hour_angle[SCHEDULER_GRID_POINTS];
	double moon_separation[SCHEDULER_GRID_POINTS];
} scheduler_target_grid;

/** Where the sky constraints of a task fail on the points of a sky grid: next_bad[i] is the first failing point at or after i, SCHEDULER_GRID_POINTS when none.
 */
typedef struct {
	bool valid;
	time_t start;
	int next_bad[SCHEDULER_GRID_POINTS + 1];
	const char *reason[SCHEDULER_GRID_POINTS];
} scheduler_task_points;

/** Build the sky grid starting at the grid step at or before start.
 */
extern void indigo_scheduler_sky_build(scheduler_sky_grid *sky, time_t start, double latitude, double longitude, double elevation);

/** Build the target grid of a task with @target on a sky grid.
 */
extern void indigo_scheduler_target_build(const scheduler_sky_grid *sky, const scheduler_header *header, scheduler_target_grid *target);

/** Return true if the grid covers the time range.
 */
extern bool indigo_scheduler_sky_covers(const scheduler_sky_grid *sky, time_t from, time_t to);

/** Required moon separation in degrees for days from full moon, Lorentzian when width is not 0.
 */
extern double indigo_scheduler_moon_limit(double separation, double width, double days_from_full);

/** Return true if the sky constraints of the task hold at the grid point index. Otherwise sets reason.
 */
extern bool indigo_scheduler_sky_point_ok(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, int index, const char **reason);

/** Return true if the task window contains the whole interval from - to. Otherwise sets reason.
 */
extern bool indigo_scheduler_window_ok(const scheduler_header *header, time_t from, time_t to, const char **reason);

/** Return true if the absolute window of the task ended before now.
 */
extern bool indigo_scheduler_window_expired(const scheduler_header *header, time_t now);

/** Return true if all constraints of the task hold over the interval from - to. Otherwise sets reason.
 */
extern bool indigo_scheduler_interval_ok(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t from, time_t to, const char **reason);

/** Return the first time at or after from when the task is eligible for its duration, 0 if not within the grid. Sets reason of the first failure.
 */
extern time_t indigo_scheduler_next_eligible(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t from, int duration, const char **reason);

/** Return the number of seconds after from for which the constraints of the task keep holding, at most SCHEDULER_SETTING_HORIZON.
 */
extern int indigo_scheduler_time_to_set(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t from);

/** Evaluate the sky constraints of a task once on every grid point.
 */
extern void indigo_scheduler_points_build(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, scheduler_task_points *points);

/** Same result as indigo_scheduler_interval_ok(), in constant time.
 */
extern bool indigo_scheduler_points_ok(const scheduler_sky_grid *sky, const scheduler_task_points *points, const scheduler_header *header, time_t from, time_t to, const char **reason);

/** Same result as indigo_scheduler_next_eligible(), searching up to limit (0 for the end of the grid).
 */
extern time_t indigo_scheduler_points_next(const scheduler_sky_grid *sky, const scheduler_task_points *points, const scheduler_header *header, time_t from, int duration, time_t limit, const char **reason);

/** Same result as indigo_scheduler_time_to_set().
 */
extern int indigo_scheduler_points_time_to_set(const scheduler_sky_grid *sky, const scheduler_task_points *points, const scheduler_header *header, time_t from);

/** Return the end of the night after now: the next sunrise, or now + 24 hours when the sun does not set within the grid.
 */
extern time_t indigo_scheduler_night_end(const scheduler_sky_grid *sky, time_t now);

#ifdef __cplusplus
}
#endif

#endif /* agent_scheduler_sky_h */
