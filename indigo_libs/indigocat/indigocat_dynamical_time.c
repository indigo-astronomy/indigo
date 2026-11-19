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

#include <stdio.h>

#include <indigo/indigocat/indigocat_dynamical_time.h>

#define TERMS 187

struct year_TD {
	int year;
	double TD;
};

/* Stephenson and Houlden  for years prior to 948 A.D.*/
static double get_dynamical_diff_sh1(double JD);

/* Stephenson and Houlden  for years between 948 A.D. and 1620 A.D.*/
static double get_dynamical_diff_sh2(double JD);

/* Table 9.a pg 72 for years 1620..1992.*/
static double get_dynamical_diff_table(double JD);

/* get the dynamical time diff in the near past 1992 .. 2000 */
static double get_dynamical_diff_near(double JD);

/* observed values for 2000 .. 2026, held constant afterwards */
static double get_dynamical_diff_observed(double JD);


/* dynamical time in seconds for every second year from 1620 to 1992 */
static const double delta_t[TERMS] = {
    124.0, 115.0, 106.0, 98.0, 91.0,
    85.0, 79.0, 74.0, 70.0, 65.0,
    62.0, 58.0, 55.0, 53.0, 50.0,
    48.0, 46.0, 44.0, 42.0, 40.0,
    37.0, 35.0, 33.0, 31.0, 28.0,
    26.0, 24.0, 22.0, 20.0, 18.0,
    16.0, 14.0, 13.0, 12.0, 11.0,
    10.0, 9.0, 9.0, 9.0, 9.0,
    9.0, 9.0, 9.0, 9.0, 10.0,
    10.0, 10.0, 10.0, 10.0, 11.0,
    11.0, 11.0, 11.0, 11.0, 11.0,
    11.0, 12.0, 12.0, 12.0, 12.0,
    12.0, 12.0, 13.0, 13.0, 13.0,
    13.0, 14.0, 14.0, 14.0, 15.0,
    15.0, 15.0, 15.0, 16.0, 16.0,
    16.0, 16.0, 16.0, 17.0, 17.0,
    17.0, 17.0, 17.0, 17.0, 17.0,
    17.0, 16.0, 16.0, 15.0, 14.0,
    13.7, 13.1, 12.7, 12.5, 12.5,
    12.5, 12.5, 12.5, 12.5, 12.3,
    12.0, 11.4, 10.6, 9.6, 8.6,
    7.5, 6.6, 6.0, 5.7, 5.6,
    5.7, 5.9, 6.2, 6.5, 6.8,
    7.1, 7.3, 7.5, 7.7, 7.8,
    7.9, 7.5, 6.4, 5.4, 2.9,
    1.6, -1.0, -2.7, -3.6, -4.7,
    -5.4, -5.2, -5.5, -5.6, -5.8,
    -5.9, -6.2, -6.4, -6.1, -4.7,
    -2.7, 0.0, 2.6, 5.4, 7.7,
    10.5, 13.4, 16.0, 18.2, 20.2,
    21.2, 22.4, 23.5, 23.9, 24.3,
    24.0, 23.9, 23.9, 23.7, 24.0,
    24.3, 25.3, 26.2, 27.3, 28.2,
    29.1, 30.0, 30.7, 31.4, 32.2,
    33.1, 34.0, 35.0, 36.5, 38.3,
    40.2, 42.2, 44.5, 46.5, 48.5,
    50.5, 52.2, 53.8, 54.9, 55.8,
    56.9, 58.3
};

/* Stephenson and Houlden  for years prior to 948 A.D.*/
static double get_dynamical_diff_sh1(double JD) {
    double TD,E;

    /* number of centuries from 948 */
    E = (JD - 2067314.5) / 36525.0;

    TD = 1830.0 - 405.0 * E + 46.5 * E * E;
    return (TD);
}

/* Stephenson and Houlden for years between 948 A.D. and 1620 A.D.*/
static double get_dynamical_diff_sh2(double JD) {
    double TD,t;

    /* number of centuries from 1850 */
    t = (JD - 2396758.5) / 36525.0;

    TD = 22.5 * t * t;
    return TD;
}

/* Table 9.a pg 72 for years 1620..1992.*/
/* uses interpolation formula 3.3 on pg 25 */
static double get_dynamical_diff_table(double JD) {
    double TD = 0;
    double a,b,c,n;
    int i;

    /* get no days since 1620 and divide by 2 years */
    i = (int)((JD - 2312752.5) / 730.5);

	/* the central entry delta_t[i] needs a neighbour on each side */
	if (i < 1) {
		i = 1;
	} else if (i > (TERMS - 2)) {
		i = TERMS - 2;
	}

	/* calc a,b,c and n, the interval in two year units from the epoch of the central entry */
	a = delta_t[i] - delta_t[i-1];
	b = delta_t[i+1] - delta_t[i];
	c = b - a;
	n = ((JD - (2312752.5 + (730.5 * i))) / 730.5);

	TD = delta_t[i] + n / 2 * (a + b + n * c);

    return TD;
}

/* get the dynamical time diff in the near past 1992 .. 2000 */
static double get_dynamical_diff_near(double JD) {
    double TD = 0;
    /* TD for 1990, 2000, 2010 */
    double delta_T[3] = {56.86, 63.83, 70.0};
    double a,b,c,n;

    /* calculate TD by interpolating value */
    a = delta_T[1] - delta_T[0];
    b = delta_T[2] - delta_T[1];
    c = b - a;

    /* get number of days since 2000 and divide by 10 years */
	n = (JD - 2451544.5) / 3652.5;
	TD = delta_T[1] + (n / 2) * (a + b + n * c);

    return TD;
}

/* Observed TT - UT1 on 1 January of 2000 .. 2026 (IERS / USNO), in seconds.
   The values for 2024 .. 2026 are derived from UT1 - UTC of IERS Bulletin A. */
#define OBSERVED_TERMS 27

static const double observed_delta_t[OBSERVED_TERMS] = {
	63.83, 64.09, 64.30, 64.47, 64.57, 64.69, 64.85, 65.15, 65.46, 65.78,
	66.07, 66.32, 66.60, 66.91, 67.28, 67.64, 68.10, 68.59, 68.97, 69.22,
	69.36, 69.36, 69.29, 69.20, 69.18, 69.14, 69.10
};

/* Linear interpolation of the observed yearly values from 2000 on. After the last
   observed year the last value is held constant: TT - UTC stays 69.184 s until a new
   leap second is announced, UTC is the time scale of the callers' clocks and the future
   change of TT - UT1 cannot be predicted to better than a few seconds per decade. */
static double get_dynamical_diff_observed(double JD) {
	double year = (JD - 2451544.5) / 365.25;
	int i = (int)year;
	if (i >= OBSERVED_TERMS - 1) {
		return observed_delta_t[OBSERVED_TERMS - 1];
	}
	return observed_delta_t[i] + (year - i) * (observed_delta_t[i + 1] - observed_delta_t[i]);
}

double indigocat_get_dynamical_time_diff(double JD) {
	double TD;

	/* check when JD is, and use corresponding formula */
	/* check for date < 948 A.D. */
	if (JD < 2067314.5)
		/* Stephenson and Houlden */
		TD = get_dynamical_diff_sh1 (JD);
	else if (JD < 2312752.5)
		/* check for date 948..1620 A.D. Stephenson and Houlden, extended up to the start of the table */
		TD = get_dynamical_diff_sh2 (JD);
	else if (JD < 2448622.5)
		/* check for value in table 1620..1992  interpolation of table */
		TD = get_dynamical_diff_table (JD);
	else if (JD < 2451544.5)
		/* check for near past 1992..2000 interpolation */
		TD = get_dynamical_diff_near (JD);
	else
		/* observed values from 2000, constant after the last observed year */
		TD = get_dynamical_diff_observed (JD);

	return TD;
}

double indigocat_jd_to_jde(double JD) {
	double JDE;
	double secs_in_day = 24 * 60 * 60;

	JDE = JD +  indigocat_get_dynamical_time_diff (JD) / secs_in_day;

	return JDE;
}
