#ifndef __TRANSFORM_H
#define __TRANSFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEG2RAD
#define DEG2RAD  (1.7453292519943295769e-2)
#endif

#ifndef RAD2DEG
#define RAD2DEG  (5.7295779513082320877e1)
#endif

typedef struct {
	double ra;	/* RA. Object right ascension in degrees.*/
	double dec;	/* DEC. Object declination */
} equatorial_coords_s;

typedef struct {
	double L;	/* Heliocentric longitude */
	double B;	/* Heliocentric latitude */
	double R;	/* Heliocentric radius vector */
} heliocentric_coords_s;

typedef struct {
	double lon; /* longitude. Object longitude. */
	double lat; /* latitude. Object latitude */
} lonlat_coords_s;

typedef struct {
	double X;	/* Rectangular X coordinate */
	double Y;	/* Rectangular Y coordinate */
	double Z;	/* Rectangular Z coordinate */
} cartesian_coords_s;

extern void ecliptical_to_equatorial_coords(lonlat_coords_s * object, double JD, equatorial_coords_s *position);
extern void heliocentric_to_cartesian_coords(heliocentric_coords_s *object, cartesian_coords_s *position);

extern double range_degrees(double angle);
extern double range_radians2(double angle);

#ifdef __cplusplus
};
#endif

#endif
