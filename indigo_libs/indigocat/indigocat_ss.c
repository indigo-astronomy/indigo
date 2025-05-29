// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

/** INDIGO HIP & DSO data
 \file indigo_cat_data.c
 */

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <indigo/indigocat/indigocat_precession.h>
#include <indigo/indigocat/indigocat_solar_system.h>
#include <indigo/indigocat/indigocat_ss.h>

#define DELTA_UT1_UTC (0.0299836 / 86400.0) /* For 2025-05-22 */
#define UT2JD(t) 			((t) / 86400.0 + 2440587.5 + DELTA_UT1_UTC)
#define JDNOW 				UT2JD(time(NULL))

#if defined(INDIGO_WINDOWS)
#pragma warning(disable:4305)
#endif

static indigocat_ss_entry indigo_ss_data[] = {
	{ MERCURY, 0.0, 0.0, -1, "Mercury", 0.0, 0.0 },
	{ VENUS, 0.0, 0.0, -3, "Venus", 0.0, 0.0 },
	{ MARS, 0.0, 0.0, -1, "Mars", 0.0, 0.0 },
	{ JUPITER, 0.0, 0.0, -2, "Jupiter", 0.0, 0.0 },
	{ SATURN, 0.0, 0.0, 0, "Saturn", 0.0, 0.0 },
	{ URANUS, 0.0, 0.0, 5, "Uranus", 0.0, 0.0 },
	{ NEPTUNE, 0.0, 0.0, 7, "Neptune", 0.0, 0.0 },
	{ PLUTO, 0.0, 0.0, 13, "Pluto", 0.0, 0.0 },
	{ SUN, 0.0, 0.0, -26, "Sun", 0.0, 0.0 },
	{ MOON, 0.0, 0.0, -12, "Moon", 0.0, 0.0 },
	{ 0 }
};

indigocat_ss_entry *indigocat_get_ss_data(void) {
	equatorial_coords_s position;

	for (int i = 0; indigo_ss_data[i].id; i++) {
		switch (indigo_ss_data[i].id) {
			case MERCURY:
				indigocat_mercury_equatorial_coords(JDNOW, &position);
				break;
			case VENUS:
				indigocat_venus_equatorial_coords(JDNOW, &position);
				break;
			case MARS:
				indigocat_mars_equatorial_coords(JDNOW, &position);
				break;
			case JUPITER:
				indigocat_jupiter_equatorial_coords(JDNOW, &position);
				break;
			case SATURN:
				indigocat_saturn_equatorial_coords(JDNOW, &position);
				break;
			case URANUS:
				indigocat_uranus_equatorial_coords(JDNOW, &position);
				break;
			case NEPTUNE:
				indigocat_neptune_equatorial_coords(JDNOW, &position);
				break;
			case PLUTO:
				indigocat_pluto_equatorial_coords(JDNOW, &position);
				break;
			case SUN:
				indigocat_sun_equatorial_coords(JDNOW, &position);
				break;
			case MOON:
				indigocat_moon_equatorial_coords(JDNOW, &position);
				break;
			default:
				return indigo_ss_data;
		}
		position.ra /= 15;
		indigo_ss_data[i].ra = position.ra;
		indigo_ss_data[i].dec = position.dec;
		indigocat_j2k_to_jnow_pm(&position.ra, &position.dec, 0, 0);
		indigo_ss_data[i].ra_now = position.ra;
		indigo_ss_data[i].dec_now = position.dec;
	}
	return indigo_ss_data;
}
