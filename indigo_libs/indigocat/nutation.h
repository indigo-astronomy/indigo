/*
 *  based on Liam Girdwood's code
 */

#ifndef __NUTATION_H
#define __NUTATION_H

#include <transform.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	double longitude;	/* Nutation in longitude, in degrees */
	double obliquity;	/* Nutation in obliquity, in degrees */
	double ecliptic;	/* Mean obliquity of the ecliptic, in degrees */
} nutation_s;

extern void get_nutation(double JD, nutation_s *nutation);

#ifdef __cplusplus
};
#endif

#endif
