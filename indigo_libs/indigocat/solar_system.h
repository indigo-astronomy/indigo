// Created by Rumen Bogdanovski, 2022
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

#include <transform.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void mercury_equatorial_coords(double JD, equatorial_coords_s *position);
extern void mercury_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void venus_equatorial_coords(double JD, equatorial_coords_s *position);
extern void venus_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void earth_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void mars_equatorial_coords(double JD, equatorial_coords_s *position);
extern void mars_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void jupiter_equatorial_coords(double JD, equatorial_coords_s *position);
extern void jupiter_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void saturn_equatorial_coords(double JD, equatorial_coords_s *position);
extern void saturn_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void uranus_equatorial_coords(double JD, equatorial_coords_s *position);
extern void uranus_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void neptune_equatorial_coords(double JD, equatorial_coords_s *position);
extern void neptune_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void pluto_equatorial_coords(double JD, equatorial_coords_s *position);
extern void pluto_heliocentric_coords(double JD, heliocentric_coords_s *position);

extern void sun_equatorial_coords(double JD, equatorial_coords_s *position);
extern void sun_geometric_coords(double JD, heliocentric_coords_s *position);

extern void moon_equatorial_coords(double JD, equatorial_coords_s *position);
extern void moon_geocentric_coords(double JD, cartesian_coords_s *moon, double precision);
extern void moon_equatorial_coords_prec(double JD, equatorial_coords_s *position, double precision);
extern void moon_ecliptical_coords(double JD, lonlat_coords_s *position, double precision);

#ifdef __cplusplus
};
#endif

#endif
