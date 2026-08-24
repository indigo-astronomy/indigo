// Copyright (c) 2017-2025 Rumen G. Bogdanovski.
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO dome azimuth solver
 \file indigo_dome_azimuth.c
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <indigo/indigo_dome_azimuth.h>

double indigo_map24(double hour) {
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


double indigo_dome_solve_azimuth(double ha, double dec, double site_latitude, double dome_radius, double mount_dec_height, double mount_dec_length, double mount_dec_offset_NS, double mount_dec_offset_EW, int side_of_pier) {
	ha = indigo_map24(ha);

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

	/*
	side_of_pier is MOUNT_SIDE_OF_PIER (-1 EAST, +1 WEST, 0 unknown) - the side
	of the pier the OTA is on.

	The two branches below are the two ends of the declination axis. The axis
	turns with the mount, so the ends swap sides once the hour angle passes 6
	hours: cos(ha0) tells which end is the east one right now.

	At 6 hours the axis runs north to south, neither end is east or west and the
	reported side says nothing. There, and when nothing is reported, fall back to
	counterweight down - the end holding the OTA above the pivot.
	*/
	double ota_east_component = cos(ha0 * M_PI / 12.0);
	double ota_up_component = sin(ha0 * M_PI / 12.0) * cos(theta);
	bool east_branch;

	if (side_of_pier == 0 || fabs(ota_east_component) < 1e-6) {
		/* counterweight down, on the meridian the axis is level so settle it east */
		east_branch = ota_up_component >= -1e-9;
	} else {
		bool ota_east_of_pier = side_of_pier < 0;
		east_branch = (ota_east_component > 0) == ota_east_of_pier;
	}

	if (site_latitude >= 0) {
		if (east_branch) {
			phi = (6.0 - ha0) * M_PI / 12.0;
		} else {
			phi = -(6.0 + ha0) * M_PI / 12.0;
		}
	} else {
		if (east_branch) {
			phi = -(6.0 - ha0) * M_PI / 12.0;
		} else {
			phi = (6.0 + ha0) * M_PI / 12.0;
		}
	}

	/*
	The pivot offsets are given by the user in absolute compass terms (+N/-S and
	+E/-W) but x and y are hemisphere dependent (x is +E and y is +N for +lat,
	x is +W and y is +S for -lat) so they have to be mirrored for -lat.
	*/
	double offset_NS = mount_dec_offset_NS;
	double offset_EW = mount_dec_offset_EW;
	if (site_latitude < 0) {
		offset_NS = -offset_NS;
		offset_EW = -offset_EW;
	}

	/* Find the dome coordinates of the OTA reference point for a German equatorial */
	x0 = mount_dec_length * sin(phi) + offset_EW;
	y0 = -mount_dec_length * cos(phi) * sin(theta) + offset_NS;
	z0 = mount_dec_length * cos(phi) * cos(theta) + mount_dec_height;

	/*
	Follow the optical axis from that point to where it leaves the dome, which is
	where the ray meets the sphere of the dome radius centred on the dome centre.
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

	double ux = cos(telalt2) * sin(telaz2);
	double uy = cos(telalt2) * cos(telaz2);
	double uz = sin(telalt2);

	double r0 = sqrt(x0 * x0 + y0 * y0 + z0 * z0);
	if (r0 >= dome_radius) {
		/* the mount does not fit in the dome, nothing sensible to trace */
		return telaz;
	}
	double b = x0 * ux + y0 * uy + z0 * uz;
	double discriminant = b * b - (r0 * r0 - dome_radius * dome_radius);
	if (discriminant < 0) {
		return telaz;
	}
	double rp = -b + sqrt(discriminant);
	double x = x0 + rp * ux;
	double y = y0 + rp * uy;

	/*
	Use (x,y,0) from the exit point to find the azimuth of the dome
	Azimuth is N (0), E (90), S (180), W (270) in both hemispheres
	However x and y are different in the hemispheres so we fix that here
	*/
	double zeta = atan2(x, y) * 180.0 / M_PI;
	if (site_latitude < 0) {
		zeta += 180.0;
	}
	return map360(zeta);
}

double indigo_azimuth_distance(double az1, double az2) {
	double distance = fabs(az1 - az2);
	distance = (distance > 180) ? fabs(distance - 360) : distance;
	//INDIGO_DEBUG(indigo_debug("%s: az distance %f - %f = %f",__FUNCTION__, az1, az2, distance));
	return distance;
}

#ifdef _TEST_

int main(int argc, char *argv[]) {
	if (argc != 3) return 1;
	double ha = atof(argv[1]);
	double dec = atof(argv[2]);
	/* indigo_dome_solve_azimuth(ha, dec, site_latitude, dome_radius, mount_dec_height, mount_dec_length, mount_dec_offset_NS, mount_dec_offset_EW, side_of_pier) */
	double daz = indigo_dome_solve_azimuth(ha, dec, -38.3334, 1.75, 0, 0.6, 0.24, 0, 0);
	printf("southern daz=%.2f\n", daz);
	/* the mirrored northern site should give 180 - daz */
	daz = indigo_dome_solve_azimuth(ha, -dec, 38.3334, 1.75, 0, 0.6, 0.24, 0, 0);
	printf("northern daz=%.2f\n", daz);
	return 0;
}

#endif /* _TEST_ */
