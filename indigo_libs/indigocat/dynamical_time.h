/*
 *  based on Liam Girdwood's code
 */

#ifndef __DYNAMICAL_TIME_H
#define __DYNAMICAL_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Calculate approximate dynamical time difference from julian day in seconds */
double  get_dynamical_time_diff(double JD);

/* brief Calculate julian ephemeris day (JDE) */
double  jd_to_jde(double JD);

#ifdef __cplusplus
};
#endif

#endif
