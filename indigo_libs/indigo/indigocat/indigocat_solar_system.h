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

#ifndef __SOLAR_SYSTEM_H
#define __SOLAR_SYSTEM_H
#include <indigo/indigocat/indigocat_transform.h>

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

INDIGO_EXTERN void indigocat_mercury_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_mercury_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_venus_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_venus_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_earth_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_mars_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_mars_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_jupiter_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_jupiter_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_saturn_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_saturn_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_uranus_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_uranus_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_neptune_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_neptune_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_pluto_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_pluto_heliocentric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_sun_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_sun_geometric_coords(double JD, heliocentric_coords_s *position);

INDIGO_EXTERN void indigocat_moon_equatorial_coords(double JD, equatorial_coords_s *position);
INDIGO_EXTERN void indigocat_moon_geocentric_coords(double JD, cartesian_coords_s *moon, double precision);
INDIGO_EXTERN void indigocat_moon_equatorial_coords_prec(double JD, equatorial_coords_s *position, double precision);
INDIGO_EXTERN void indigocat_moon_ecliptical_coords(double JD, lonlat_coords_s *position, double precision);

#ifdef __cplusplus
};
#endif

#endif
