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
 \file indigo_agent_scheduler_sky.c
 */

#include <string.h>
#include <math.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_align.h>
#include <indigo/indigocat/indigocat_solar_system.h>
#include <indigo/indigocat/indigocat_transform.h>

#include "indigo_agent_scheduler_sky.h"

#define SYNODIC_HALF_MONTH	14.765
#define DAY									86400

static double angular_distance(double ra1, double dec1, double ra2, double dec2) {
	// RA in hours, Dec in degrees, result in degrees
	double d1 = dec1 * DEG2RAD, d2 = dec2 * DEG2RAD;
	double dra = (ra1 - ra2) * 15 * DEG2RAD;
	double c = sin(d1) * sin(d2) + cos(d1) * cos(d2) * cos(dra);
	if (c > 1) {
		c = 1;
	} else if (c < -1) {
		c = -1;
	}
	return acos(c) * RAD2DEG;
}

static double normalize_hour_angle(double ha) {
	ha = fmod(ha, 24);
	if (ha < -12) {
		ha += 24;
	} else if (ha >= 12) {
		ha -= 24;
	}
	return ha;
}

// the Moon theory is expensive, so the Sun and the Moon are computed every SOLAR_SYSTEM_STEP grid points
// and interpolated linearly; the Moon moves about 0.5 degrees in an hour, the error is far below the limits
#define SOLAR_SYSTEM_STEP	12

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

static void solar_system(time_t t, double *sun_ra, double *sun_dec, double *moon_ra, double *moon_dec) {
	double jd = UT2JD(t);
	equatorial_coords_s sun, moon;
	indigocat_sun_equatorial_coords(jd, &sun);
	indigocat_moon_equatorial_coords(jd, &moon);
	*sun_ra = sun.ra / 15.0;
	*sun_dec = sun.dec;
	*moon_ra = moon.ra / 15.0;
	*moon_dec = moon.dec;
}

void indigo_scheduler_sky_build(scheduler_sky_grid *sky, time_t start, double latitude, double longitude, double elevation) {
	sky->latitude = latitude;
	sky->longitude = longitude;
	sky->elevation = elevation;
	sky->start = start - start % SCHEDULER_GRID_STEP;
	double sun_ra[2] = { 0, 0 }, sun_dec[2] = { 0, 0 }, moon_ra[2] = { 0, 0 }, moon_dec[2] = { 0, 0 };
	solar_system(sky->start, sun_ra + 1, sun_dec + 1, moon_ra + 1, moon_dec + 1);
	for (int i = 0; i < SCHEDULER_GRID_POINTS; i++) {
		time_t t = sky->start + (time_t)i * SCHEDULER_GRID_STEP;
		int phase = i % SOLAR_SYSTEM_STEP;
		if (phase == 0) {
			sun_ra[0] = sun_ra[1];
			sun_dec[0] = sun_dec[1];
			moon_ra[0] = moon_ra[1];
			moon_dec[0] = moon_dec[1];
			solar_system(t + SOLAR_SYSTEM_STEP * SCHEDULER_GRID_STEP, sun_ra + 1, sun_dec + 1, moon_ra + 1, moon_dec + 1);
		}
		double fraction = (double)phase / SOLAR_SYSTEM_STEP;
		double sun_ra_t = interpolate_ra(sun_ra[0], sun_ra[1], fraction);
		double sun_dec_t = sun_dec[0] + (sun_dec[1] - sun_dec[0]) * fraction;
		sky->moon_ra[i] = interpolate_ra(moon_ra[0], moon_ra[1], fraction);
		sky->moon_dec[i] = moon_dec[0] + (moon_dec[1] - moon_dec[0]) * fraction;
		double az;
		indigo_radec_to_altaz(sun_ra_t, sun_dec_t, &t, latitude, longitude, elevation, sky->sun_altitude + i, &az);
		indigo_radec_to_altaz(sky->moon_ra[i], sky->moon_dec[i], &t, latitude, longitude, elevation, sky->moon_altitude + i, &az);
		double elongation = angular_distance(sun_ra_t, sun_dec_t, sky->moon_ra[i], sky->moon_dec[i]);
		// the elongation is symmetric about full moon, which is all the constraints need
		sky->moon_days_from_full[i] = (180 - elongation) / 180 * SYNODIC_HALF_MONTH;
		sky->moon_illumination[i] = (1 - cos(elongation * DEG2RAD)) / 2 * 100;
		sky->lst[i] = indigo_lst(&t, longitude);
	}
	sky->valid = true;
}

void indigo_scheduler_target_build(const scheduler_sky_grid *sky, const scheduler_header *header, scheduler_target_grid *target) {
	target->valid = false;
	if (!sky->valid || !header->has_target) {
		return;
	}
	double ra = header->ra, dec = header->dec;
	indigo_j2k_to_jnow(&ra, &dec);
	for (int i = 0; i < SCHEDULER_GRID_POINTS; i++) {
		time_t t = sky->start + (time_t)i * SCHEDULER_GRID_STEP;
		double az;
		indigo_radec_to_altaz(ra, dec, &t, sky->latitude, sky->longitude, sky->elevation, target->altitude + i, &az);
		target->hour_angle[i] = normalize_hour_angle(sky->lst[i] - ra);
		target->moon_separation[i] = angular_distance(ra, dec, sky->moon_ra[i], sky->moon_dec[i]);
	}
	target->start = sky->start;
	target->valid = true;
}

bool indigo_scheduler_sky_covers(const scheduler_sky_grid *sky, time_t from, time_t to) {
	return sky->valid && from >= sky->start && to <= sky->start + (time_t)(SCHEDULER_GRID_POINTS - 1) * SCHEDULER_GRID_STEP;
}

double indigo_scheduler_moon_limit(double separation, double width, double days_from_full) {
	if (width <= 0) {
		return separation;
	}
	double x = days_from_full / width;
	return separation / (1 + x * x);
}

static bool needs_sky(const scheduler_header *header) {
	return header->has_target || header->twilight != SCHEDULER_TWILIGHT_NONE;
}

bool indigo_scheduler_sky_point_ok(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, int index, const char **reason) {
	static const double twilight_limit[] = { 90, -6, -12, -18 };
	if (header->twilight != SCHEDULER_TWILIGHT_NONE && sky->sun_altitude[index] >= twilight_limit[header->twilight]) {
		*reason = "too bright (twilight)";
		return false;
	}
	if (!header->has_target) {
		return true;
	}
	double altitude = target->altitude[index];
	if (altitude <= 0) {
		*reason = "target below the horizon";
		return false;
	}
	if (header->has_min_altitude && altitude < header->min_altitude) {
		*reason = "target below the altitude limit";
		return false;
	}
	if (header->has_max_airmass && indigo_airmass(altitude) > header->max_airmass) {
		*reason = "airmass too high";
		return false;
	}
	if (header->has_ha) {
		double ha = target->hour_angle[index];
		bool inside = header->ha_from <= header->ha_to ? (ha >= header->ha_from && ha <= header->ha_to) : (ha >= header->ha_from || ha <= header->ha_to);
		if (!inside) {
			*reason = "hour angle out of range";
			return false;
		}
	}
	// the moon brightens the sky only above the horizon
	if (sky->moon_altitude[index] > 0) {
		if (header->has_moon && target->moon_separation[index] < indigo_scheduler_moon_limit(header->moon_separation, header->moon_width, sky->moon_days_from_full[index])) {
			*reason = "too close to the moon";
			return false;
		}
		if (header->has_illumination && sky->moon_illumination[index] > header->max_illumination) {
			*reason = "moon too bright";
			return false;
		}
	}
	return true;
}

// the occurrence of a nightly window containing t, false if t is outside of the window
static bool nightly_occurrence(const scheduler_header *header, time_t t, time_t *occurrence_end) {
	int start = header->has_window_start ? header->window_start_minute : 0;
	int end = header->has_window_end ? header->window_end_minute : 1440;
	time_t day = t - (t % DAY);
	double minute = (double)(t % DAY) / 60;
	if (start < end) {
		if (minute >= start && minute < end) {
			*occurrence_end = day + end * 60;
			return true;
		}
		return false;
	}
	if (minute >= start) {
		*occurrence_end = day + DAY + end * 60;
		return true;
	}
	if (minute < end) {
		*occurrence_end = day + end * 60;
		return true;
	}
	return false;
}

bool indigo_scheduler_window_ok(const scheduler_header *header, time_t from, time_t to, const char **reason) {
	switch (header->window_kind) {
		case SCHEDULER_WINDOW_NONE:
			return true;
		case SCHEDULER_WINDOW_ABSOLUTE:
			if (header->has_window_start && from < header->window_start) {
				*reason = "window not open yet";
				return false;
			}
			if (header->has_window_end && to > header->window_end) {
				*reason = "window ends before the task can finish";
				return false;
			}
			return true;
		case SCHEDULER_WINDOW_NIGHTLY: {
			time_t occurrence_end;
			if (!nightly_occurrence(header, from, &occurrence_end)) {
				*reason = "outside of the nightly window";
				return false;
			}
			if (to > occurrence_end) {
				*reason = "nightly window ends before the task can finish";
				return false;
			}
			return true;
		}
	}
	return true;
}

bool indigo_scheduler_window_expired(const scheduler_header *header, time_t now) {
	return header->window_kind == SCHEDULER_WINDOW_ABSOLUTE && header->has_window_end && now >= header->window_end;
}

bool indigo_scheduler_interval_ok(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t from, time_t to, const char **reason) {
	if (!indigo_scheduler_window_ok(header, from, to, reason)) {
		return false;
	}
	if (!needs_sky(header)) {
		return true;
	}
	if (!sky->valid || (header->has_target && (!target->valid || target->start != sky->start))) {
		*reason = "site coordinates unknown";
		return false;
	}
	if (!indigo_scheduler_sky_covers(sky, from, to)) {
		*reason = "beyond the forecast";
		return false;
	}
	int first = (int)((from - sky->start) / SCHEDULER_GRID_STEP);
	int last = (int)((to - sky->start + SCHEDULER_GRID_STEP - 1) / SCHEDULER_GRID_STEP);
	if (last >= SCHEDULER_GRID_POINTS) {
		last = SCHEDULER_GRID_POINTS - 1;
	}
	for (int i = first; i <= last; i++) {
		if (!indigo_scheduler_sky_point_ok(sky, target, header, i, reason)) {
			return false;
		}
	}
	return true;
}

time_t indigo_scheduler_next_eligible(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t from, int duration, const char **reason) {
	const char *first_reason = NULL;
	time_t limit = sky->valid ? sky->start + (time_t)(SCHEDULER_GRID_POINTS - 1) * SCHEDULER_GRID_STEP : from + 2 * DAY;
	time_t t = from;
	while (t + duration <= limit) {
		const char *candidate_reason = NULL;
		if (indigo_scheduler_interval_ok(sky, target, header, t, t + duration, &candidate_reason)) {
			if (reason) {
				*reason = first_reason;
			}
			return t;
		}
		if (first_reason == NULL) {
			first_reason = candidate_reason;
		}
		time_t next = t - t % SCHEDULER_GRID_STEP + SCHEDULER_GRID_STEP;
		// an absolute window opening between grid points is a candidate of its own
		if (header->window_kind == SCHEDULER_WINDOW_ABSOLUTE && header->has_window_start && header->window_start > t && header->window_start < next) {
			next = header->window_start;
		}
		t = next;
	}
	if (reason) {
		*reason = first_reason;
	}
	return 0;
}

int indigo_scheduler_time_to_set(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t from) {
	const char *reason;
	time_t t = from;
	while (t - from < SCHEDULER_SETTING_HORIZON) {
		time_t next = t - t % SCHEDULER_GRID_STEP + SCHEDULER_GRID_STEP;
		if (!indigo_scheduler_interval_ok(sky, target, header, t, next, &reason)) {
			return (int)(t - from);
		}
		t = next;
	}
	return SCHEDULER_SETTING_HORIZON;
}

void indigo_scheduler_points_build(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, scheduler_task_points *points) {
	points->valid = false;
	if (!sky->valid || (header->has_target && (target == NULL || !target->valid || target->start != sky->start))) {
		return;
	}
	points->next_bad[SCHEDULER_GRID_POINTS] = SCHEDULER_GRID_POINTS;
	for (int i = SCHEDULER_GRID_POINTS - 1; i >= 0; i--) {
		const char *reason = NULL;
		if (needs_sky(header) && !indigo_scheduler_sky_point_ok(sky, target, header, i, &reason)) {
			points->next_bad[i] = i;
			points->reason[i] = reason;
		} else {
			points->next_bad[i] = points->next_bad[i + 1];
			points->reason[i] = NULL;
		}
	}
	points->start = sky->start;
	points->valid = true;
}

bool indigo_scheduler_points_ok(const scheduler_sky_grid *sky, const scheduler_task_points *points, const scheduler_header *header, time_t from, time_t to, const char **reason) {
	if (!indigo_scheduler_window_ok(header, from, to, reason)) {
		return false;
	}
	if (!needs_sky(header)) {
		return true;
	}
	if (!sky->valid || !points->valid || points->start != sky->start) {
		*reason = "site coordinates unknown";
		return false;
	}
	if (!indigo_scheduler_sky_covers(sky, from, to)) {
		*reason = "beyond the forecast";
		return false;
	}
	int first = (int)((from - sky->start) / SCHEDULER_GRID_STEP);
	int last = (int)((to - sky->start + SCHEDULER_GRID_STEP - 1) / SCHEDULER_GRID_STEP);
	if (last >= SCHEDULER_GRID_POINTS) {
		last = SCHEDULER_GRID_POINTS - 1;
	}
	int bad = points->next_bad[first];
	if (bad <= last) {
		*reason = points->reason[bad];
		return false;
	}
	return true;
}

time_t indigo_scheduler_points_next(const scheduler_sky_grid *sky, const scheduler_task_points *points, const scheduler_header *header, time_t from, int duration, time_t limit, const char **reason) {
	const char *first_reason = NULL;
	time_t grid_end = sky->valid ? sky->start + (time_t)(SCHEDULER_GRID_POINTS - 1) * SCHEDULER_GRID_STEP : from + 2 * DAY;
	if (limit == 0 || limit > grid_end) {
		limit = grid_end;
	}
	time_t t = from;
	while (t + duration <= limit) {
		const char *candidate_reason = NULL;
		if (indigo_scheduler_points_ok(sky, points, header, t, t + duration, &candidate_reason)) {
			if (reason) {
				*reason = first_reason;
			}
			return t;
		}
		if (first_reason == NULL) {
			first_reason = candidate_reason;
		}
		time_t next = t - t % SCHEDULER_GRID_STEP + SCHEDULER_GRID_STEP;
		// a failing sky point is skipped at once, the interval cannot contain it
		if (needs_sky(header) && sky->valid && points->valid && points->start == sky->start && t >= sky->start) {
			int first = (int)((t - sky->start) / SCHEDULER_GRID_STEP);
			int last = (int)((t + duration - sky->start + SCHEDULER_GRID_STEP - 1) / SCHEDULER_GRID_STEP);
			int bad = first < SCHEDULER_GRID_POINTS ? points->next_bad[first] : SCHEDULER_GRID_POINTS;
			if (bad <= last && bad < SCHEDULER_GRID_POINTS) {
				time_t after = sky->start + (time_t)(bad + 1) * SCHEDULER_GRID_STEP;
				if (after > next) {
					next = after;
				}
			}
		}
		if (header->window_kind == SCHEDULER_WINDOW_ABSOLUTE && header->has_window_start && header->window_start > t && header->window_start < next) {
			next = header->window_start;
		}
		t = next;
	}
	if (reason) {
		*reason = first_reason;
	}
	return 0;
}

int indigo_scheduler_points_time_to_set(const scheduler_sky_grid *sky, const scheduler_task_points *points, const scheduler_header *header, time_t from) {
	const char *reason;
	time_t t = from;
	while (t - from < SCHEDULER_SETTING_HORIZON) {
		time_t next = t - t % SCHEDULER_GRID_STEP + SCHEDULER_GRID_STEP;
		if (!indigo_scheduler_points_ok(sky, points, header, t, next, &reason)) {
			return (int)(t - from);
		}
		t = next;
	}
	return SCHEDULER_SETTING_HORIZON;
}

time_t indigo_scheduler_night_end(const scheduler_sky_grid *sky, time_t now) {
	if (sky->valid && indigo_scheduler_sky_covers(sky, now, now)) {
		int i = (int)((now - sky->start) / SCHEDULER_GRID_STEP);
		// during the day, the night ends at the sunrise after the next sunset
		while (i < SCHEDULER_GRID_POINTS && sky->sun_altitude[i] >= 0) {
			i++;
		}
		while (i < SCHEDULER_GRID_POINTS && sky->sun_altitude[i] < 0) {
			i++;
		}
		if (i < SCHEDULER_GRID_POINTS) {
			return sky->start + (time_t)i * SCHEDULER_GRID_STEP;
		}
	}
	return now + DAY;
}
