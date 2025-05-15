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

#include <indigo/indigocat/indigocat_nutation.h>
#include <indigo/indigocat/indigocat_dynamical_time.h>

#define TERMS 63
#define LN_NUTATION_EPOCH_THRESHOLD 0.1

struct nutation_arguments
{
    double D;
    double M;
    double MM;
    double F;
    double O;
};

struct nutation_coefficients
{
    double longitude1;
    double longitude2;
    double obliquity1;
    double obliquity2;
};

static const struct nutation_arguments arguments[TERMS] = {
    {0.0,	0.0,	0.0,	0.0,	1.0},
    {-2.0,	0.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	0.0,	2.0},
    {0.0,	1.0,	0.0,	0.0,	0.0},
    {0.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	1.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	2.0,	1.0},
    {0.0,	0.0,	1.0,	2.0,	2.0},
    {-2.0,	-1.0,	0.0,	2.0,	2.0},
    {-2.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	0.0,	0.0,	2.0,	1.0},
    {0.0,	0.0,	-1.0,	2.0,	2.0},
    {2.0,	0.0,	0.0,	0.0,	0.0},
    {0.0,	0.0,	1.0,	0.0,	1.0},
    {2.0,	0.0,	-1.0,	2.0,	2.0},
    {0.0,	0.0,	-1.0,	0.0,	1.0},
    {0.0,	0.0,	1.0,	2.0,	1.0},
    {-2.0,	0.0,	2.0,	0.0,	0.0},
    {0.0,	0.0,	-2.0,	2.0,	1.0},
    {2.0,	0.0,	0.0,	2.0,	2.0},
    {0.0,	0.0,	2.0,	2.0,	2.0},
    {0.0,	0.0,	2.0,	0.0,	0.0},
    {-2.0,	0.0,	1.0,	2.0,	2.0},
    {0.0,	0.0,	0.0,	2.0,	0.0},
    {-2.0,	0.0,	0.0,	2.0,	0.0},
    {0.0,	0.0,	-1.0,	2.0,	1.0},
    {0.0,	2.0,	0.0,	0.0,	0.0},
    {2.0,	0.0,	-1.0,	0.0,	1.0},
    {-2.0,	2.0,	0.0,	2.0,	2.0},
    {0.0,	1.0,	0.0,	0.0,	1.0},
    {-2.0,	0.0,	1.0,	0.0,	1.0},
    {0.0,	-1.0,	0.0,	0.0,	1.0},
    {0.0,	0.0,	2.0,	-2.0,	0.0},
    {2.0,	0.0,	-1.0,	2.0,	1.0},
    {2.0,	0.0,	1.0,	2.0,	2.0},
    {0.0,	1.0,	0.0,	2.0,	2.0},
    {-2.0,	1.0,	1.0,	0.0,	0.0},
    {0.0,	-1.0,	0.0,	2.0,	2.0},
    {2.0,	0.0,	0.0,	2.0,	1.0},
    {2.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	0.0,	2.0,	2.0,	2.0},
    {-2.0,	0.0,	1.0,	2.0,	1.0},
    {2.0,	0.0,	-2.0,	0.0,	1.0},
    {2.0,	0.0,	0.0,	0.0,	1.0},
    {0.0,	-1.0,	1.0,	0.0,	0.0},
    {-2.0,	-1.0,	0.0,	2.0,	1.0},
    {-2.0,	0.0,	0.0,	0.0,	1.0},
    {0.0,	0.0,	2.0,	2.0,	1.0},
    {-2.0,	0.0,	2.0,	0.0,	1.0},
    {-2.0,	1.0,	0.0,	2.0,	1.0},
    {0.0,	0.0,	1.0,	-2.0,	0.0},
    {-1.0,	0.0,	1.0,	0.0,	0.0},
    {-2.0,	1.0,	0.0,	0.0,	0.0},
    {1.0,	0.0,	0.0,	0.0,	0.0},
    {0.0,	0.0,	1.0,	2.0,	0.0},
    {0.0,	0.0,	-2.0,	2.0,	2.0},
    {-1.0,	-1.0,	1.0,	0.0,	0.0},
    {0.0,	1.0,	1.0,	0.0,	0.0},
    {0.0,	-1.0,	1.0,	2.0,	2.0},
    {2.0,	-1.0,	-1.0,	2.0,	2.0},
    {0.0,	0.0,	3.0,	2.0,	2.0},
    {2.0,	-1.0,	0.0,	2.0,	2.0}};

static const struct nutation_coefficients coefficients[TERMS] = {
    {-171996.0,	-174.2,	92025.0,8.9},
    {-13187.0,	-1.6,  	5736.0,	-3.1},
    {-2274.0, 	-0.2,  	977.0,	-0.5},
    {2062.0,   	0.2,    -895.0,    0.5},
    {1426.0,    -3.4,    54.0,    -0.1},
    {712.0,    0.1,    -7.0,    0.0},
    {-517.0,    1.2,    224.0,    -0.6},
    {-386.0,    -0.4,    200.0,    0.0},
    {-301.0,    0.0,    129.0,    -0.1},
    {217.0,    -0.5,    -95.0,    0.3},
    {-158.0,    0.0,    0.0,    0.0},
    {129.0,	0.1,	-70.0,	0.0},
    {123.0,	0.0,	-53.0,	0.0},
    {63.0,	0.0,	0.0,	0.0},
    {63.0,	0.1,	-33.0,	0.0},
    {-59.0,	0.0,	26.0,	0.0},
    {-58.0,	-0.1,	32.0,	0.0},
    {-51.0,	0.0,	27.0,	0.0},
    {48.0,	0.0,	0.0,	0.0},
    {46.0,	0.0,	-24.0,	0.0},
    {-38.0,	0.0,	16.0,	0.0},
    {-31.0,	0.0,	13.0,	0.0},
    {29.0,	0.0,	0.0,	0.0},
    {29.0,	0.0,	-12.0,	0.0},
    {26.0,	0.0,	0.0,	0.0},
    {-22.0,	0.0,	0.0,	0.0},
    {21.0,	0.0,	-10.0,	0.0},
    {17.0,	-0.1,	0.0,	0.0},
    {16.0,	0.0,	-8.0,	0.0},
    {-16.0,	0.1,	7.0,	0.0},
    {-15.0,	0.0,	9.0,	0.0},
    {-13.0,	0.0,	7.0,	0.0},
    {-12.0,	0.0,	6.0,	0.0},
    {11.0,	0.0,	0.0,	0.0},
    {-10.0,	0.0,	5.0,	0.0},
    {-8.0,	0.0,	3.0,	0.0},
    {7.0,	0.0,	-3.0,	0.0},
    {-7.0,	0.0,	0.0,	0.0},
    {-7.0,	0.0,	3.0,	0.0},
    {-7.0,	0.0,	3.0,	0.0},
    {6.0,	0.0,	0.0,	0.0},
    {6.0,	0.0,	-3.0,	0.0},
    {6.0,	0.0,	-3.0,	0.0},
    {-6.0,	0.0,	3.0,	0.0},
    {-6.0,	0.0,	3.0,	0.0},
    {5.0,	0.0,	0.0,	0.0},
    {-5.0,	0.0,	3.0,	0.0},
    {-5.0,	0.0,	3.0,	0.0},
    {-5.0,	0.0,	3.0,	0.0},
    {4.0,	0.0,	0.0,	0.0},
    {4.0,	0.0,	0.0,	0.0},
    {4.0,	0.0,	0.0,	0.0},
    {-4.0,	0.0,	0.0,	0.0},
    {-4.0,	0.0,	0.0,	0.0},
    {-4.0,	0.0,	0.0,	0.0},
    {3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0},
    {-3.0,	0.0,	0.0,	0.0}};

/* cache values */
static long double c_JD = 0.0, c_longitude = 0.0, c_obliquity = 0.0, c_ecliptic = 0.0;

void indigocat_get_nutation(double JD, nutation_s *nutation) {
	long double D,M,MM,F,O,T,T2,T3,JDE;
	long double coeff_sine, coeff_cos;
	long double argument;
	int i;

	/* should we bother recalculating nutation */
	if (fabsl(JD - c_JD) > LN_NUTATION_EPOCH_THRESHOLD) {
		/* set the new epoch */
		c_JD = JD;

		/* get julian ephemeris day */
		JDE = indigocat_jd_to_jde(JD);

		T = (JDE - 2451545.0)/36525;
		T2 = T * T;
		T3 = T2 * T;

		D = 297.85036 + 445267.111480 * T - 0.0019142 * T2 + T3 / 189474.0;
		M = 357.52772 + 35999.050340 * T - 0.0001603 * T2 - T3 / 300000.0;
		MM = 134.96298 + 477198.867398 * T + 0.0086972 * T2 + T3 / 56250.0;
		F = 93.2719100 + 483202.017538 * T - 0.0036825 * T2 + T3 / 327270.0;
		O = 125.04452 - 1934.136261 * T + 0.0020708 * T2 + T3 / 450000.0;

		/* convert to radians */
		D = DEG2RAD * D;
		M = DEG2RAD * M;
		MM = DEG2RAD * MM;
		F = DEG2RAD * F;
		O = DEG2RAD * O;

		for (i=0; i< TERMS; i++) {
			/* calc coefficients of sine and cosine */
			coeff_sine = (coefficients[i].longitude1 + (coefficients[i].longitude2 * T));
			coeff_cos = (coefficients[i].obliquity1 + (coefficients[i].obliquity2 * T));

			argument = arguments[i].D * D
				+ arguments[i].M * M
				+ arguments[i].MM * MM
				+ arguments[i].F * F
				+ arguments[i].O * O;

			c_longitude += coeff_sine * sin(argument);
			c_obliquity += coeff_cos * cos(argument);
		}

		/* change to arcsecs */
		c_longitude /= 10000;
		c_obliquity /= 10000;

		/* change to degrees */
		c_longitude /= (60 * 60);
		c_obliquity /= (60 * 60);

		/* calculate mean ecliptic */
		c_ecliptic = 23.0 + 26.0 / 60.0 + 21.448 / 3600.0 - 46.8150/3600 * T - 0.00059/3600 * T2 + 0.001813/3600 * T3;

		/* c_ecliptic += c_obliquity; * Uncomment this if function should
                                         return true obliquity rather than
                                         mean obliquity */
	}

	nutation->longitude = c_longitude;
	nutation->obliquity = c_obliquity;
	nutation->ecliptic = c_ecliptic;
}
