#ifndef __VSOP87_H
#define __VSOP87_H

#include <transform.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vsop {
	float A;
	float B;
	float C;
};

/*
* Thanks to Messrs. Bretagnon and Francou for publishing planetary
* solution VSOP87.
*/

extern void vsop87_to_fk5(heliocentric_coords_s * position, double JD);
extern double vsop87_calc_series(const struct vsop * data, int terms, double t);

#ifdef __cplusplus
};
#endif

#endif
