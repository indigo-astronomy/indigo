// Created by Rumen Bogdanovski, 2022
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

#include <math.h>
#include <time.h>

#include <indigo/indigocat/indigocat_precession.h>
#include <indigo/indigocat/indigocat_dynamical_time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

equatorial_coords_s indigocat_precess(const equatorial_coords_s *c0, const double eq0, const double eq1) {
	double rot[3][3];
	equatorial_coords_s c1 = {0, 0};

	double cosd = cos(c0->dec);

	double x0 = cosd * cos(c0->ra);
	double y0 = cosd * sin(c0->ra);
	double z0 = sin(c0->dec);

	double ST = (eq0 - 2000.0) * 0.001;
	double T = (eq1 - eq0) * 0.001;

	const double sec2rad = DEG2RAD/3600.0;
	double A = sec2rad * T * (23062.181 + ST * (139.656 + 0.0139 * ST) + T * (30.188 - 0.344 * ST + 17.998 * T));
	double B = sec2rad * T * T * (79.280 + 0.410 * ST + 0.205 * T) + A;
	double C = sec2rad * T * (20043.109 - ST * (85.33 + 0.217 * ST) + T * (-42.665 - 0.217 * ST - 41.833 * T));

	double sinA = sin(A);
	double sinB = sin(B);
	double sinC = sin(C);
	double cosA = cos(A);
	double cosB = cos(B);
	double cosC = cos(C);

	rot[0][0] = cosA * cosB * cosC - sinA * sinB;
	rot[0][1] = (-1) * sinA * cosB * cosC - cosA * sinB;
	rot[0][2] = (-1) * sinC * cosB;

	rot[1][0] = cosA * cosC * sinB + sinA * cosB;
	rot[1][1] = (-1) * sinA * cosC * sinB + cosA * cosB;
	rot[1][2] = (-1) * sinB * sinC;

	rot[2][0] = cosA * sinC;
	rot[2][1] = (-1) * sinA * sinC;
	rot[2][2] = cosC;

	double x1 = rot[0][0] * x0 + rot[0][1] * y0 + rot[0][2] * z0;
	double y1 = rot[1][0] * x0 + rot[1][1] * y0 + rot[1][2] * z0;
	double z1 = rot[2][0] * x0 + rot[2][1] * y0 + rot[2][2] * z0;

	if (x1 == 0) {
		if (y1 > 0) {
			c1.ra = 90.0 * DEG2RAD;
		} else {
			c1.ra = 270.0 * DEG2RAD;
		}
	} else {
		c1.ra = atan2(y1, x1);
	}
	if (c1.ra < 0) {
		c1.ra += 360 * DEG2RAD;
	}

	c1.dec = atan2(z1 , sqrt(1 - z1 * z1));

	return c1;
}

equatorial_coords_s indigocat_apply_proper_motion(const equatorial_coords_s *c0, double pmra, double pmdec, double eq0, double eq1) {
	equatorial_coords_s c1 = { 0, 0 };
	double t = eq1 - eq0;
	c1.ra = c0->ra + t * pmra * DEG2RAD / 3600000;
	c1.dec = c0->dec + t * pmdec * DEG2RAD / 3600000;
	c1.ra = fmod(c1.ra + 2 * M_PI, 2 * M_PI);
	return c1;
}

void indigocat_j2k_to_jnow_pm(double *ra, double *dec, double pmra, double pmdec) {
	equatorial_coords_s coordinates = { *ra * 15 * DEG2RAD, *dec * DEG2RAD };
	double now = 2000 + ((time(NULL) / 86400.0 + 2440587.5 - 0.477677 / 86400.0) - 2451545.0) / 365.25;
	if (pmra != 0 && pmdec != 0)
		coordinates = indigocat_apply_proper_motion(&coordinates, pmra, pmdec, 2000.0, now);
	coordinates = indigocat_precess(&coordinates, 2000.0, now);
	*ra = coordinates.ra * RAD2DEG / 15;
	*dec = coordinates.dec * RAD2DEG;
}

void indigocat_jnow_to_j2k(double *ra, double *dec) {
	equatorial_coords_s coordinates = { *ra * 15 * DEG2RAD, *dec * DEG2RAD };
	double now = 2000 + ((time(NULL) / 86400.0 + 2440587.5 - 0.477677 / 86400.0) - 2451545.0) / 365.25;
	coordinates = indigocat_precess(&coordinates, now, 2000.0);
	*ra = coordinates.ra * RAD2DEG / 15;
	*dec = coordinates.dec * RAD2DEG;
}

