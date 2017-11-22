// Copyright (c) 2017 Rumen G.Bogdanovski.
// All rights reserved.
//
// Based on PyDome code
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
// 2.0 by Rumen G.Bogdanovski <rumen@skyarchive.org>

/** INDIGO dome azimuth solver
 \file indigo_dome_azimuth.c
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>


static double map24(double hour) {
	double hour24;
	if (hour < 0.0) {
		int n = (int)(hour / 24.0) - 1;
		hour24 = hour - (double)n * 24.0;
		return hour24;
	} else if (hour >= 24.0) {
		int n = (int)(hour / 24.0);
		hour24 = hour - (double)n * 24.0;
		return hour24;
	} else {
		return hour;
	}
}


static double map360(double angle) {
	if (angle < 0.0) {
		int n = (int)(angle / 360.0) - 1;
		return (angle - (double)n * 360.0);
	} else if (angle >= 360.0) {
		int n = (int)(angle / 360.0);
		return (angle - (double)n * 360.0);
	} else {
		return angle;
	}
}


static void equatorial_to_horizontal(double ha, double dec, double site_latitude, double *azimuth, double *altitude) {
	double phi = site_latitude*M_PI/180.0;

	ha = ha * M_PI / 12.0;
	dec = dec * M_PI / 180.0;
	*altitude = asin(sin(phi)*sin(dec) + cos(phi)*cos(dec)*cos(ha));
	*altitude = *altitude * 180.0 / M_PI;
	*azimuth = atan2(-cos(dec)*sin(ha), sin(dec)*cos(phi) - sin(phi)*cos(dec)*cos(ha));
	*azimuth = *azimuth*180.0/M_PI;
	*azimuth = map360(*azimuth);
}


double indigo_dome_solve_azimuth(double ha, double dec, double site_latitude, double dome_radius, double mount_dec_height, double mount_dec_length, double mount_dec_offset_NS, double mount_dec_offset_EW) {
	ha = map24(ha);

	/* Map an hourangle in hours to  -12 <= ha0 < +12 */
	double ha0 = (ha >= 12.0) ? (ha - 24.0) : ha;

	double telaz, telalt;
	equatorial_to_horizontal(ha, dec, site_latitude, &telaz, &telalt);

	/*
	Find the reference point on the optical axis in dome coordinates
	z: vertical
	y: north - south
	x: east - west
	theta: altitude of the polar axis from the horizontal plane
	phi: rotation of dec axis about polar axis

	Meaning of x and y compared to sky depends on the hemisphere!
	z: always + up
	y: always + toward N for +lat or S for -lat
	x: maintains right-handed coordinate system with z and y
	 thus x is + to E for +lat and to W for -lat
	theta: always positive from the horizontal plane to the pole
	phi: rotation about polar axis
	 and is +90 for a horizontal axis with OTA toward +x
	 and is   0 for OTA over mount with dec axis counterweight down
	 and is -90 for a horizontal axis with OTA toward -x
	*/
	double x0, y0, z0, phi = 0, theta = 0;

	if (site_latitude >= 0.) {
		theta = site_latitude * M_PI / 180.0;
	} else {
		theta = -1. * site_latitude * M_PI / 180.0;
	}

	/* if German equatorial the origin changes with HA */
	if (site_latitude >= 0) {
		if (ha0 > 0) {
			/* Looking west with OTA east of pier */
			phi = (6.0 - ha0) * M_PI / 12.0;
		} else {
			/* Looking east with OTA west of pier */
			phi = -(6.0 + ha0) * M_PI / 12.0;
		}
	} else {
		if (ha0 > 0) {
			/* Looking west with OTA east of pier */
			phi = -(6.0 - ha0) * M_PI / 12.0;
		} else {
			/* Looking east with OTA west of pier */
			phi = (6.0 + ha0) * M_PI / 12.0;
		}
	}

	/* Find the dome coordinates of the OTA reference point for a German equatorial */
	x0 = mount_dec_length * sin(phi) + mount_dec_offset_EW;
	y0 = -mount_dec_length * cos(phi) * sin(theta) + mount_dec_offset_NS;
	z0 = mount_dec_length * cos(phi) * cos(theta) + mount_dec_height;

	/*
	(x,y,z) is on the optical axis
	Iterate to make this point also lie on the dome surface
	Begin iteration assuming the zero point is at the center of the dome
	Telescope azimuth is measured from the direction to the pole
	*/
	double telaz2, telalt2;

	if (site_latitude >= 0) {
		telaz2 = telaz * M_PI / 180.0;
		telalt2 = telalt * M_PI / 180.0;
	} else {
		telaz2 = telaz - 180.0;
		telaz2 = map360(telaz2);
		telaz2 = telaz2 * M_PI / 180.0;
		telalt2 = telalt * M_PI / 180.0;
	}

	double d = 0;
	double r = dome_radius;
	int n = 0;
	double x, y, z;
	/* converges fast => 5 should be ok */
	while (n < 5) {
		d = d - (r - dome_radius);
		double rp = dome_radius + d;
		x = x0 + rp * cos(telalt2) * sin(telaz2);
		y = y0 + rp * cos(telalt2) * cos(telaz2);
		z = z0 + rp * sin(telalt2);
		r = sqrt(x*x + y*y + z*z);
		//printf("n, r, rp phi: %d %f %f %f\n", n, r, rp, phi);
		n++;
	}

	/*
	Use (x,y,0) from the interation to find the azimuth of the dome
	Azimuth is N (0), E (90), S (180), W (270) in both hemispheres
	However x and y are different in the hemispheres so we fix that here
	*/
	double zeta = atan2(x, y);
	if ((zeta > -2 * M_PI) && (zeta < 2 * M_PI)) {
		if (site_latitude > 0) {
			zeta = (180.0 / M_PI) * zeta;
			zeta = map360(zeta);
		} else {
			zeta = (180.0 / M_PI) * zeta;
			zeta = zeta + 180;
			zeta = map360(zeta);
		}
	} else {
		zeta = telaz;
	}
	return zeta;
}

#ifdef _TEST_

int main(int argc, char *argv[]) {
	if(argc != 3) return 1;
	double ha = atof(argv[1]);
	double dec = atof(argv[2]);
	//solve_dome_azimuth(ha, dec, site_latitude, dome_radius, mount_dec_height, mount_dec_length, mount_dec_offset, mount_is_gem)
	double daz = indigo_dome_solve_azimuth(ha, dec, -38.3334, 1.75, 0, 0.6, 0.24);
	printf("daz=%.2f\n", daz);
	daz = indigo_dome_solve_azimuth(ha, dec, -38.3334, 1.75, 0, 0.6, 0.24);
	printf("daz=%.2f\n", daz);
}

#endif /* _TEST_ */
