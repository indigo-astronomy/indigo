// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO NOVAS wrapper
 \file indigo_novas.c
 */

//#include <time.h>
//#include <novas.h>
//#include <eph_manager.h>

#include <math.h>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_novas.h>
#include <indigo/indigo_align.h>

const double DELTA_T = 34+32.184+0.477677;
const double DELTA_UTC_UT1 = -0.477677/86400.0;

double indigo_mean_gst(time_t *utc) {
	long double gst;
	long double t;
	double jd;

	if (utc)
		jd = UT2JD(*utc);
	else
		jd = UT2JD(time(NULL));

	t = (jd - 2451545.0) / 36525.0;
	gst = 280.46061837 + (360.98564736629 * (jd - 2451545.0)) + (0.000387933 * t * t) - (t * t * t / 38710000.0);
	gst = fmod(gst + 360.0, 360.0);
	gst *= 24.0 / 360.0;
	return gst;
}

double indigo_lst(time_t *utc, double longitude) {
	double gst = indigo_mean_gst(utc);
	return fmod(gst + longitude/15.0 + 24.0, 24.0);
}

void indigo_eq2hor(time_t *utc, double latitude, double longitude, double elevation, double ra, double dec, double *alt, double *az) {
	indigo_spherical_point_t eq_point;
	indigo_spherical_point_t h_point;
	double lst = indigo_lst(utc, longitude);

	indigo_ra_dec_to_point(ra, dec, lst, &eq_point);
	indigo_equatorial_to_hotizontal(&eq_point, latitude * DEG2RAD, &h_point);
	*az = h_point.a * RAD2DEG;
	*alt = h_point.d *RAD2DEG;
}
