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

#ifndef __TRANSFORM_H
#define __TRANSFORM_H

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

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

INDIGO_EXTERN void indigocat_ecliptical_to_equatorial_coords(lonlat_coords_s * object, double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_heliocentric_to_cartesian_coords(heliocentric_coords_s *object, cartesian_coords_s *position);

INDIGO_EXTERN double indigocat_range_degrees(double angle);
INDIGO_EXTERN double range_radians2(double angle);

#ifdef __cplusplus
};
#endif

#endif
