#include <math.h>
#include <vsop87.h>

double vsop87_calc_series(const struct vsop * data, int terms, double t) {
	double value = 0;
	int i;

	for (i=0; i<terms; i++) {
		value += data->A * cos(data->B + data->C * t);
		data++;
	}

	return value;
}

void vsop87_to_fk5(heliocentric_coords_s * position, double JD) {
	double LL, cos_LL, sin_LL, T, delta_L, delta_B, B;

	/* get julian centuries from 2000 */
	T = (JD - 2451545.0)/ 36525.0;

	LL = position->L + ( - 1.397 - 0.00031 * T ) * T;
	LL = DEG2RAD * LL;
	cos_LL = cos(LL);
	sin_LL = sin(LL);
	B = DEG2RAD * position->B;

	delta_L = (-0.09033 / 3600.0) + (0.03916 / 3600.0) * (cos_LL + sin_LL) * tan (B);
	delta_B = (0.03916 / 3600.0) * (cos_LL - sin_LL);

	position->L += delta_L;
	position->B += delta_B;
}
