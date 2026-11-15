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

/** INDIGO Scheduler agent nights and night hooks
 \file indigo_agent_scheduler_night.h
 */

#ifndef agent_scheduler_night_h
#define agent_scheduler_night_h

#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_NIGHT_SPAN				(36 * 3600)
#define SCHEDULER_NIGHT_MAX					8
#define SCHEDULER_NIGHT_OFFSET_LIMIT	240

/** One night of the scheduler: the Sun below the limit, both ends moved by the offsets. An end that lies outside of the computed span is not known.
 */
typedef struct {
	time_t start, end;
	bool start_known, end_known;
} scheduler_night;

/** The nights from SCHEDULER_NIGHT_SPAN before to SCHEDULER_NIGHT_SPAN after the time they were built for.
 */
typedef struct {
	bool valid;
	double latitude, longitude, elevation;
	double limit;
	int start_offset, end_offset;
	time_t built;
	time_t from, to;
	int count;
	scheduler_night nights[SCHEDULER_NIGHT_MAX];
} scheduler_nights;

/** The state of the night hooks between decisions.
 */
typedef struct {
	bool handled;
	scheduler_night night;
	bool started;
	bool skip_reported;
} scheduler_night_state;

typedef enum {
	SCHEDULER_NIGHT_NONE = 0,
	SCHEDULER_NIGHT_SKIP,
	SCHEDULER_NIGHT_END,
	SCHEDULER_NIGHT_START
} scheduler_night_action;

/** Altitude of the Sun in degrees.
 */
extern double indigo_scheduler_sun_altitude(time_t t, double latitude, double longitude, double elevation);

/** Compute the nights around now; offsets are in minutes and clamped to SCHEDULER_NIGHT_OFFSET_LIMIT. Nights that overlap after the offsets are merged, nights that the offsets leave empty are dropped.
 */
extern void indigo_scheduler_nights_build(scheduler_nights *nights, time_t now, double latitude, double longitude, double elevation, double limit, int start_offset, int end_offset);

/** The night containing t, NULL during the day.
 */
extern const scheduler_night *indigo_scheduler_night_at(const scheduler_nights *nights, time_t t);

/** The night containing t or the first night after it, NULL if there is none in the computed span.
 */
extern const scheduler_night *indigo_scheduler_night_next(const scheduler_nights *nights, time_t t);

/** Reset the state, as on START.
 */
extern void indigo_scheduler_night_reset(scheduler_night_state *state);

/** Decide the next step of the night hooks at a decision at now. has_work tells whether a task can run in the current night. The state is updated as if the returned hook ran; call again after it to get the following step.
 */
extern scheduler_night_action indigo_scheduler_night_decide(scheduler_night_state *state, const scheduler_nights *nights, time_t now, bool has_work);

#ifdef __cplusplus
}
#endif

#endif /* agent_scheduler_night_h */
