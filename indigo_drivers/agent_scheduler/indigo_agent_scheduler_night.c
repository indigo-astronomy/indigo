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
 \file indigo_agent_scheduler_night.c
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_align.h>
#include <indigo/indigocat/indigocat_solar_system.h>
#include <indigo/indigocat/indigocat_transform.h>

#include "indigo_agent_scheduler_night.h"

#define NIGHT_STEP		300
#define SUN_STEP			12
#define NIGHT_POINTS	(2 * SCHEDULER_NIGHT_SPAN / NIGHT_STEP + 1)

static void sun_coordinates(time_t t, double *ra, double *dec) {
	equatorial_coords_s sun;
	indigocat_sun_equatorial_coords(UT2JD(t), &sun);
	*ra = sun.ra / 15.0;
	*dec = sun.dec;
}

// right ascension in hours, interpolated across 24 h
static double interpolate_ra(double ra1, double ra2, double fraction) {
	double delta = ra2 - ra1;
	if (delta > 12) {
		delta -= 24;
	} else if (delta < -12) {
		delta += 24;
	}
	double ra = ra1 + delta * fraction;
	if (ra < 0) {
		ra += 24;
	} else if (ra >= 24) {
		ra -= 24;
	}
	return ra;
}

double indigo_scheduler_sun_altitude(time_t t, double latitude, double longitude, double elevation) {
	double ra, dec, altitude, azimuth;
	sun_coordinates(t, &ra, &dec);
	indigo_radec_to_altaz(ra, dec, &t, latitude, longitude, elevation, &altitude, &azimuth);
	return altitude;
}

// the time between two samples where the altitude crosses the limit
static time_t crossing(time_t t0, double altitude0, double altitude1, double limit) {
	double fraction = (altitude0 - limit) / (altitude0 - altitude1);
	return t0 + (time_t)lround(fraction * NIGHT_STEP);
}

static void add_night(scheduler_nights *nights, scheduler_night night) {
	if (night.end <= night.start) {
		return;
	}
	if (nights->count > 0) {
		scheduler_night *last = nights->nights + nights->count - 1;
		// the offsets made the nights overlap
		if (last->end >= night.start) {
			if (night.end > last->end) {
				last->end = night.end;
				last->end_known = night.end_known;
			}
			return;
		}
	}
	if (nights->count < SCHEDULER_NIGHT_MAX) {
		nights->nights[nights->count++] = night;
	}
}

void indigo_scheduler_nights_build(scheduler_nights *nights, time_t now, double latitude, double longitude, double elevation, double limit, int start_offset, int end_offset) {
	// allocated per call, the function is called from the agent and from clients of the module
	double *altitude = indigo_safe_malloc(NIGHT_POINTS * sizeof(double));
	start_offset = start_offset < -SCHEDULER_NIGHT_OFFSET_LIMIT ? -SCHEDULER_NIGHT_OFFSET_LIMIT : start_offset > SCHEDULER_NIGHT_OFFSET_LIMIT ? SCHEDULER_NIGHT_OFFSET_LIMIT : start_offset;
	end_offset = end_offset < -SCHEDULER_NIGHT_OFFSET_LIMIT ? -SCHEDULER_NIGHT_OFFSET_LIMIT : end_offset > SCHEDULER_NIGHT_OFFSET_LIMIT ? SCHEDULER_NIGHT_OFFSET_LIMIT : end_offset;
	memset(nights, 0, sizeof(*nights));
	nights->latitude = latitude;
	nights->longitude = longitude;
	nights->elevation = elevation;
	nights->limit = limit;
	nights->start_offset = start_offset;
	nights->end_offset = end_offset;
	nights->built = now;
	nights->from = now - SCHEDULER_NIGHT_SPAN;
	nights->from -= nights->from % NIGHT_STEP;
	nights->to = nights->from + (time_t)(NIGHT_POINTS - 1) * NIGHT_STEP;
	// the Sun moves slowly, its position is computed once per hour and interpolated
	double ra[2] = { 0, 0 }, dec[2] = { 0, 0 };
	sun_coordinates(nights->from, ra + 1, dec + 1);
	for (int i = 0; i < NIGHT_POINTS; i++) {
		time_t t = nights->from + (time_t)i * NIGHT_STEP;
		int phase = i % SUN_STEP;
		if (phase == 0) {
			ra[0] = ra[1];
			dec[0] = dec[1];
			sun_coordinates(t + SUN_STEP * NIGHT_STEP, ra + 1, dec + 1);
		}
		double fraction = (double)phase / SUN_STEP;
		double azimuth;
		indigo_radec_to_altaz(interpolate_ra(ra[0], ra[1], fraction), dec[0] + (dec[1] - dec[0]) * fraction, &t, latitude, longitude, elevation, altitude + i, &azimuth);
	}
	scheduler_night night = { 0 };
	bool below = false;
	for (int i = 0; i < NIGHT_POINTS; i++) {
		time_t t = nights->from + (time_t)i * NIGHT_STEP;
		bool now_below = altitude[i] < limit;
		if (now_below && !below) {
			night.start_known = i > 0;
			night.start = i > 0 ? crossing(t - NIGHT_STEP, altitude[i - 1], altitude[i], limit) + start_offset * 60 : nights->from;
		} else if (!now_below && below) {
			night.end_known = true;
			night.end = crossing(t - NIGHT_STEP, altitude[i - 1], altitude[i], limit) + end_offset * 60;
			add_night(nights, night);
		}
		below = now_below;
	}
	if (below) {
		night.end_known = false;
		night.end = nights->to;
		add_night(nights, night);
	}
	free(altitude);
	nights->valid = true;
}

const scheduler_night *indigo_scheduler_night_at(const scheduler_nights *nights, time_t t) {
	if (nights == NULL || !nights->valid) {
		return NULL;
	}
	for (int i = 0; i < nights->count; i++) {
		if (nights->nights[i].start <= t && t < nights->nights[i].end) {
			return nights->nights + i;
		}
	}
	return NULL;
}

const scheduler_night *indigo_scheduler_night_next(const scheduler_nights *nights, time_t t) {
	if (nights == NULL || !nights->valid) {
		return NULL;
	}
	for (int i = 0; i < nights->count; i++) {
		if (t < nights->nights[i].end) {
			return nights->nights + i;
		}
	}
	return NULL;
}

void indigo_scheduler_night_reset(scheduler_night_state *state) {
	memset(state, 0, sizeof(*state));
}

// the same night computed again, possibly with slightly different ends or after a change of the site
static bool same_night(const scheduler_night *a, const scheduler_night *b) {
	return a->start < b->end && b->start < a->end;
}

scheduler_night_action indigo_scheduler_night_decide(scheduler_night_state *state, const scheduler_nights *nights, time_t now, bool has_work) {
	const scheduler_night *current = indigo_scheduler_night_at(nights, now);
	if (state->handled) {
		if (current != NULL && same_night(current, &state->night)) {
			state->night = *current;
		} else {
			// the handled night is over; its end hook runs only after its start hook
			bool started = state->started;
			state->handled = false;
			state->started = false;
			if (started) {
				return SCHEDULER_NIGHT_END;
			}
		}
	}
	if (!state->handled && current != NULL) {
		state->handled = true;
		state->night = *current;
		state->started = false;
		state->skip_reported = false;
	}
	if (state->handled && !state->started) {
		if (has_work) {
			state->started = true;
			return SCHEDULER_NIGHT_START;
		}
		if (!state->skip_reported) {
			state->skip_reported = true;
			return SCHEDULER_NIGHT_SKIP;
		}
	}
	return SCHEDULER_NIGHT_NONE;
}
