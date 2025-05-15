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

#include <indigo/indigocat/indigocat_solar_system.h>
#include <indigo/indigocat/indigocat_vsop87.h>
#include <indigo/indigocat/indigocat_transform.h>
#include <indigo/indigocat/indigocat_nutation.h>

#if defined(INDIGO_WINDOWS)
#pragma warning(disable:4305)
#endif

void indigocat_sun_geometric_coords(double JD, heliocentric_coords_s * position) {
	indigocat_earth_heliocentric_coords(JD, position);

	position->L += 180.0;
	position->L = indigocat_range_degrees(position->L);
	position->B *= -1.0;
}

void indigocat_sun_equatorial_coords(double JD, equatorial_coords_s * position) {
	heliocentric_coords_s sol;
	lonlat_coords_s LB;
	nutation_s nutation;
	double aberration;

	indigocat_sun_geometric_coords(JD, &sol);

	/* add nutation */
	indigocat_get_nutation(JD, &nutation);
	sol.L += nutation.longitude;

	/* aberration */
	aberration = (20.4898 / (360 * 60 * 60)) / sol.R;
	sol.L -= aberration;

	/* transform to equatorial */
	LB.lat = sol.B;
	LB.lon = sol.L;
	indigocat_ecliptical_to_equatorial_coords(&LB, JD, position);
}
