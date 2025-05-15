// Copyright (c) 2022-2025 Rumen G. Bogdanovski
// All rights reserved.
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
//
//	Created by Rumen G. Bogdanovski, based on Liam Girdwood's code.

#include <math.h>

#include <indigo/indigocat/indigocat_solar_system.h>
#include <indigo/indigocat/indigocat_vsop87.h>
#include <indigo/indigocat/indigocat_transform.h>

#if defined(INDIGO_WINDOWS)
#pragma warning(disable:4305)
#endif

#define PLUTO_COEFFS 43

struct pluto_argument
{
	float J, S, P;
};

struct pluto_longitude
{
	float A,B;
};

struct pluto_latitude
{
	float A,B;
};

struct pluto_radius
{
	float A,B;
};

static const struct pluto_argument argument[PLUTO_COEFFS] = {
	{0, 0, 1},
	{0, 0, 2},
	{0, 0, 3},
	{0, 0, 4},
	{0, 0, 5},
	{0, 0, 6},
	{0, 1, -1},
	{0, 1, 0},
	{0, 1, 1},
	{0, 1, 2},
	{0, 1, 3},
	{0, 2, -2},
	{0, 2, -1},
	{0, 2, 0},
	{1, -1, 0},
	{1, -1, 1},
	{1, 0, -3},
	{1, 0, -2},
	{1, 0, -1},
	{1, 0, 0},
	{1, 0, 1},
	{1, 0, 2},
	{1, 0, 3},
	{1, 0, 4},
	{1, 1, -3},
	{1, 1, -2},
	{1, 1, -1},
	{1, 1, 0},
	{1, 1, 1},
	{1, 1, 3},
	{2, 0, -6},
	{2, 0, -5},
	{2, 0, -4},
	{2, 0, -3},
	{2, 0, -2},
	{2, 0, -1},
	{2, 0, 0},
	{2, 0, 1},
	{2, 0, 2},
	{2, 0, 3},
	{3, 0, -2},
	{3, 0, -1},
	{3, 0, 0}
};


static const struct pluto_longitude longitude[PLUTO_COEFFS] = {
	{-19799805, 19850055},
	{897144, -4954829},
	{611149, 1211027},
	{-341243, -189585},
	{129287, -34992},
	{-38164, 30893},
	{20442, -9987},
	{-4063, -5071},
	{-6016, -3336},
	{-3956, 3039},
	{-667, 3572},
	{1276, 501},
	{1152, -917},
	{630, -1277},
	{2571, -459},
	{899, -1449},
	{-1016, 1043},
	{-2343, -1012},
	{7042, 788},
	{1199, -338},
	{418, -67},
	{120, -274},
	{-60, -159},
	{-82, -29},
	{-36, -20},
	{-40, 7},
	{-14, 22},
	{4, 13},
	{5,2},
	{-1,0},
	{2,0},
	{-4, 5},
	{4, -7},
	{14, 24},
	{-49, -34},
	{163, -48},
	{9, 24},
	{-4, 1},
	{-3,1},
	{1,3},
	{-3, -1},
	{5, -3},
	{0,0}
};

static const struct pluto_latitude latitude[PLUTO_COEFFS] = {
	{-5452852, -14974862},
	{3527812, 1672790},
	{-1050748, 327647},
	{178690, -292153},
	{18650, 100340},
	{-30697, -25823},
	{4878, 11248},
	{226, -64},
	{2030, -836},
	{69, -604},
	{-247, -567},
	{-57, 1},
	{-122, 175},
	{-49, -164},
	{-197, 199},
	{-25, 217},
	{589, -248},
	{-269, 711},
	{185, 193},
	{315, 807},
	{-130, -43},
	{5, 3},
	{2, 17},
	{2, 5},
	{2, 3},
	{3, 1},
	{2, -1},
	{1, -1},
	{0, -1},
	{0, 0},
	{0, -2},
	{2, 2},
	{-7, 0},
	{10, -8},
	{-3, 20},
	{6, 5},
	{14, 17},
	{-2, 0},
	{0, 0},
	{0, 0},
	{0, 1},
	{0, 0},
	{1, 0}
};

static const struct pluto_radius radius[PLUTO_COEFFS] = {
	{66865439, 68951812},
	{-11827535, -332538},
	{1593179, -1438890},
	{-18444, 483220},
	{-65977, -85431},
	{31174, -6032},
	{-5794, 22161},
	{4601, 4032},
	{-1729, 234},
	{-415, 702},
	{239, 723},
	{67, -67},
	{1034, -451},
	{-129, 504},
	{480, -231},
	{2, -441},
	{-3359, 265},
	{7856, -7832},
	{36, 45763},
	{8663, 8547},
	{-809, -769},
	{263, -144},
	{-126, 32},
	{-35, -16},
	{-19, -4},
	{-15, 8},
	{-4, 12},
	{5, 6},
	{3, 1},
	{6, -2},
	{2, 2},
	{-2, -2},
	{14, 13},
	{-63, 13},
	{136, -236},
	{273, 1065},
	{251, 149},
	{-25, -9},
	{9, -2},
	{-8, 7},
	{2, -10},
	{19, 35},
	{10, 2}
};


void indigocat_pluto_equatorial_coords(double JD, equatorial_coords_s * position) {
	heliocentric_coords_s h_sol, h_pluto;
	cartesian_coords_s g_sol, g_pluto;
	double a,b,c;
	double ra, dec, delta, diff, last, t = 0;

	indigocat_sun_geometric_coords(JD, &h_sol);
	indigocat_heliocentric_to_cartesian_coords(&h_sol,  &g_sol);

	do {
		last = t;
		indigocat_pluto_heliocentric_coords (JD - t, &h_pluto);
		indigocat_heliocentric_to_cartesian_coords(&h_pluto, &g_pluto);

		a = g_sol.X + g_pluto.X;
		b = g_sol.Y + g_pluto.Y;
		c = g_sol.Z + g_pluto.Z;

		delta = a*a + b*b + c*c;
		delta = sqrt (delta);
		t = delta * 0.0057755183;
		diff = t - last;
	} while (diff > 0.0001 || diff < -0.0001);

	ra = atan2 (b,a);
	dec = c / delta;
	dec = asin (dec);

	position->ra = indigocat_range_degrees(RAD2DEG * ra);
	position->dec = RAD2DEG * dec;
}

void indigocat_pluto_heliocentric_coords(double JD, heliocentric_coords_s * position) {
	double sum_longitude = 0, sum_latitude = 0, sum_radius = 0;
	double J, S, P;
	double t, a, sin_a, cos_a;
	int i;

	/* get julian centuries since J2000 */
	t = (JD - 2451545) / 36525;

	/* calculate mean longitudes for jupiter, saturn and pluto */
	J =  34.35 + 3034.9057 * t;
   	S =  50.08 + 1222.1138 * t;
   	P = 238.96 +  144.9600 * t;

	for (i=0; i < PLUTO_COEFFS; i++) {
		a = argument[i].J * J + argument[i].S * S + argument[i].P * P;
		sin_a = sin (DEG2RAD * a);
		cos_a = cos (DEG2RAD * a);

		sum_longitude += longitude[i].A * sin_a + longitude[i].B * cos_a;

		sum_latitude += latitude[i].A * sin_a + latitude[i].B * cos_a;

		sum_radius += radius[i].A * sin_a + radius[i].B * cos_a;
	}

	position->L = 238.958116 + 144.96 * t + sum_longitude * 0.000001;
	position->B = -3.908239 + sum_latitude * 0.000001;
	position->R = 40.7241346 + sum_radius * 0.0000001;
}
