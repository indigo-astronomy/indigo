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
#include <novas.h>
#include <eph_manager.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_novas.h>

#define UT2JD(t) ((t) / 86400.0 + 2440587.5 + DELTA_UTC_UT1)
double DELTA_T = 34+32.184+0.477677;
double DELTA_UTC_UT1 = -0.477677/86400.0;

static void init() {
	static int do_init = 1;
	if (do_init) {
		double jd_begin, jd_end;
		short de_number;
		int error = ephem_open("JPLEPH.421", &jd_begin, &jd_end, &de_number);
		if (error != 0) {
			indigo_error("ephem_open() -> %d", error);
		}
		do_init = 0;
	}
}

double indigo_lst(time_t *utc, double longitude) {
	double ut1;
	if (utc)
		ut1 = UT2JD(*utc);
	else
		ut1 = UT2JD(time(NULL));

	double gst;
	int error = sidereal_time(ut1, 0.0, DELTA_T, 0, 0, 0, &gst);
	if (error != 0) {
		indigo_error("sidereal_time() -> %d", error);
		return 0;
	}
	return fmod(gst + longitude/15.0 + 24.0, 24.0);
}

void indigo_eq2hor(time_t *utc, double latitude, double longitude, double elevation, double ra, double dec, double *alt, double *az) {
	double ut1;
	if (utc)
		ut1 = UT2JD(*utc);
	else
		ut1 = UT2JD(time(NULL));

	on_surface position = { latitude, longitude, elevation, 0.0, 0.0 };
	equ2hor(ut1, DELTA_T, 1, 0.0, 0.0, &position, ra, dec, 0, alt, az, &ra, &dec);
	*alt = 90 - *alt;
}

void indigo_app_star(double promora, double promodec, double parallax, double rv, double *ra, double *dec) {
	double ut1_now = time(NULL) / 86400.0 + 2440587.5 + DELTA_UTC_UT1;
	double tt_now = ut1_now + DELTA_T / 86400.0;
	cat_entry star;
	init();
	make_cat_entry("HIP 1", "HP2", 1, *ra, *dec, promora, promodec, parallax, rv, &star);
	int error = app_star(tt_now, &star, 1, ra, dec);
	if (error != 0) {
		indigo_error("app_star() -> %d", error);
	}
}

void indigo_topo_star(double latitude, double longitude, double elevation, double promora, double promodec, double parallax, double rv, double *ra, double *dec) {
	double ut1_now = time(NULL) / 86400.0 + 2440587.5 + DELTA_UTC_UT1;
	double tt_now = ut1_now + DELTA_T / 86400.0;
	cat_entry star;
	init();
	make_cat_entry("HIP1", "HP2", 1, *ra, *dec, promora, promodec, parallax, rv, &star);
	on_surface position = { latitude, longitude, elevation, 0.0, 0.0 };
	int error = topo_star(tt_now, DELTA_T, &star, &position, 1, ra, dec);
	if (error != 0) {
		indigo_error("topo_star() -> %d", error);
	}
}

void indigo_topo_planet(double latitude, double longitude, double elevation, int id, double *ra_j2k, double *dec_j2k, double *ra, double *dec) {
	cat_entry DUMMY_STAR;
	object solarSystem;
	double distance;
	double ut1_now = time(NULL) / 86400.0 + 2440587.5 + DELTA_UTC_UT1;
	double pos[3], j2k_pos[3];
	on_surface position = { latitude, longitude, elevation, 0.0, 0.0 };
	init();
	make_object(0, id, "Dummy", &DUMMY_STAR, &solarSystem);
	int error = topo_planet(ut1_now, &solarSystem, DELTA_T, &position, 1, ra, dec, &distance);
	if (error != 0) {
		indigo_error("topo_planet() -> %d", error);
	}
	radec2vector(*ra, *dec, distance, pos);
	error = precession(ut1_now, pos, 2451545.0, j2k_pos);
	if (error != 0) {
		indigo_error("precession() -> %d", error);
	}
	vector2radec(j2k_pos, ra_j2k, dec_j2k);
}
