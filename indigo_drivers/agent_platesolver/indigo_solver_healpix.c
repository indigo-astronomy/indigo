// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
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

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO quad based plate solver - HEALPix ring scheme helpers
 \file indigo_solver_healpix.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "indigo_solver_healpix.h"

static inline uint32_t isqrt_uint64(uint32_t arg) {
	return (uint32_t)(sqrt((float)arg + 0.5));
}

uint32_t indigo_angular_distance_to_nside(float distance) {
	uint32_t x = ceil(1.0233267079464885 / distance);
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

uint32_t indigo_healpix_ang2pix_ring(uint32_t nside, float ra, float dec) {
	float phi = ra ;
	float theta = M_PI / 2 - dec;
	float z = cos(theta);
	float sth = sin(theta);
	bool have_sth = true;
	float za = fabs(z);
	float tt = fmod(phi * (2.0 / M_PI), 4.0);
	if (tt < 0.0) {
		tt += 4.0;
	}
	uint32_t nl4 = 4U * (uint32_t)nside;
	uint32_t ncap = 2U * (uint32_t)nside * ((uint32_t)nside - 1U);
	uint32_t npix = 12U * (uint32_t)nside * (uint32_t)nside;
	uint32_t pix;
	if (za <= 2.0 / 3.0) { // Equatorial region
		float temp1 = (float)nside * (0.5 + tt);
		float temp2 = (float)nside * z * 0.75;
		uint32_t jp = (uint32_t)(temp1 - temp2);
		uint32_t jm = (uint32_t)(temp1 + temp2);
		uint32_t ir = (uint32_t)nside + 1U + jp - jm;
		int kshift = 1 - ((int)ir & 1);
		uint32_t t1 = jp + jm - (uint32_t)nside + (uint32_t)kshift + 1U + nl4 + nl4;
		uint32_t ip = (t1 >> 1) % nl4;
		pix = ncap + (ir - 1U) * nl4 + ip;
	} else { // Polar regions
		float tp = tt - floor(tt);
		float tmp;
		if (za < 0.99 || !have_sth) {
			tmp = (float)nside * sqrt(3.0 * (1.0 - za));
		} else {
			tmp = (float)nside * sth / sqrt((1.0 + za) / 3.0);
		}
		uint32_t jp = (uint32_t)(tp * tmp);
		uint32_t jm = (uint32_t)((1.0 - tp) * tmp);
		uint32_t ir = jp + jm + 1U;
		uint32_t ip = (uint32_t)(tt * (float)ir) % (4U * ir);
		if (z > 0.0) {
			pix = 2U * ir * (ir - 1U) + ip;
		} else {
			pix = npix - 2U * ir * (ir + 1U) + ip;
		}
	}
	return pix;
}

void indigo_healpix_pix2ang_ring(uint32_t nside, uint32_t ipix, float *ra, float *dec) {
	float fact1 = 2.0 / (3.0 * (float)nside);
	float fact2 = 1.0 / (3.0 * (float)nside * (float)nside);
	uint32_t nl2 = 2U * (uint32_t)nside;
	uint32_t nl4 = 4U * (uint32_t)nside;
	uint32_t ncap = nl2 * ((uint32_t)nside - 1U);
	uint32_t npix = 12U * (uint32_t)nside * (uint32_t)nside;
	float z, phi, sth = 0.0;
	bool have_sth = false;
	if (ipix < ncap) { // North Polar cap
		uint32_t iring = (1U + isqrt_uint64(1U + 2U * ipix)) >> 1;
		uint32_t iphi = ipix + 1U - 2U * iring * (iring - 1U);
		float tmp = (float)(iring * iring) * fact2;
		z = 1.0 - tmp;
		if (z > 0.99) {
			sth = sqrt(tmp * (2.0 - tmp));
			have_sth = true;
		}
		phi = ((float)iphi - 0.5) * (M_PI / 2.0) / (float)iring;
	} else if (ipix < (npix - ncap)) { // Equatorial region
		uint32_t ip = ipix - ncap;
		uint32_t tmp = ip / nl4;
		uint32_t iring = tmp + (uint32_t)nside;
		uint32_t iphi = ip - nl4 * tmp + 1U;
		float fodd = (((iring + (uint32_t)nside) & 1U) != 0) ? 1.0 : 0.5;
		z = ((float)nl2 - (float)iring) * fact1;
		phi = ((float)iphi - fodd) * M_PI * 0.75 * fact1;
	} else { // South Polar cap
		uint32_t ip = npix - ipix;
		uint32_t iring = (1U + isqrt_uint64(2U * ip - 1U)) >> 1;
		uint32_t iphi = 4U * iring + 1U - (ip - 2U * iring * (iring - 1U));
		float tmp = (float)(iring * iring) * fact2;
		z = tmp - 1.0;
		if (z < -0.99) {
			sth = sqrt(tmp * (2.0 - tmp));
			have_sth = true;
		}
		phi = ((float)iphi - 0.5) * (M_PI / 2.0) / (float)iring;
	}
	// Normalize phi to [0, 2*pi)
	if (phi < 0.0) {
		phi += 2.0 * M_PI;
	}
	if (phi >= 2.0 * M_PI) {
		phi -= 2.0 * M_PI;
	}
	float theta;
	if (have_sth) {
		theta = atan2(sth, z);
	} else {
		theta = acos(z);
	}
	*ra = phi;
	*dec = M_PI / 2 - theta;
}

static const int jrll[12] = { 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };
static const int jpll[12] = { 1, 3, 5, 7, 0, 2, 4, 6, 1, 3, 5, 7 };
static const int nb_xoffset[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
static const int nb_yoffset[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

static const int nb_facearray[9][12] = {
	{ 8, 9, 10, 11, -1, -1, -1, -1, 10, 11, 8, 9 }, // S
	{ 5, 6, 7, 4, 8, 9, 10, 11, 9, 10, 11, 8 }, // SE
	{ -1, -1, -1, -1, 5, 6, 7, 4, -1, -1, -1, -1 }, // E
	{ 4, 5, 6, 7, 11, 8, 9, 10, 11, 8, 9, 10 }, // SW
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, // center
	{ 1, 2, 3, 0, 0, 1, 2, 3, 5, 6, 7, 4 }, // NE
	{ -1, -1, -1, -1, 7, 4, 5, 6, -1, -1, -1, -1 }, // W
	{ 3, 0, 1, 2, 3, 0, 1, 2, 4, 5, 6, 7 }, // NW
	{ 2, 3, 0, 1, -1, -1, -1, -1, 0, 1, 2, 3 } // N
};

static const int nb_swaparray[9][3] = {
	{ 0, 0, 3 }, // S
	{ 0, 0, 6 }, // SE
	{ 0, 0, 0 }, // E
	{ 0, 0, 5 }, // SW
	{ 0, 0, 0 }, // center
	{ 5, 0, 0 }, // NE
	{ 0, 0, 0 }, // W
	{ 6, 0, 0 }, // NW
	{ 3, 0, 0 } // N
};

static uint32_t special_div(uint32_t a, uint32_t b) {
	uint32_t t = (a >= (b << 1));
	a -= t * (b << 1);
	return (t << 1) + (a >= b);
}

static void ring2xyf(uint32_t nside, uint32_t pix, int *ix, int *iy, int *face_num) {
	int32_t iring, iphi, nr;
	int kshift;
	int32_t nl2 = 2 * (int32_t)nside;
	int32_t ncap = 2 * (int32_t)nside * ((int32_t)nside - 1);
	int32_t npix = 12 * (int32_t)nside * (int32_t)nside;
	if ((int32_t)pix < ncap) { // North Polar cap
		iring = (1 + (int32_t)isqrt_uint64(1U + 2U * pix)) >> 1;
		iphi = pix + 1U - 2U * (uint32_t)iring * ((uint32_t)iring - 1U);
		kshift = 0;
		nr = iring;
		*face_num = (int)special_div((uint32_t)iphi - 1U, (uint32_t)nr);
	} else if ((int32_t)pix < (npix - ncap)) { // Equatorial
		int32_t ip = (int32_t)pix - ncap;
		int32_t tmp_order = (int32_t)log2(nside);
		int32_t tmp = (tmp_order >= 0) ? ip >> (tmp_order + 2) : ip / (4 * (int32_t)nside);
		iring = tmp + (int32_t)nside;
		iphi = ip - tmp * 4 * (int32_t)nside + 1;
		kshift = ((iring + (int32_t)nside) & 1) ? 1 : 0;
		nr = (int32_t)nside;
		int32_t ire = tmp + 1;
		int32_t irm = nl2 + 1 - tmp;
		int32_t ifm = iphi - (ire >> 1) + (int32_t)nside - 1;
		int32_t ifp = iphi - (irm >> 1) + (int32_t)nside - 1;
		if (tmp_order >= 0) {
			ifm >>= tmp_order;
			ifp >>= tmp_order;
		} else {
			ifm /= (int32_t)nside;
			ifp /= (int32_t)nside;
		}
		*face_num = (ifp == ifm) ? (int)(ifp | 4) : ((ifp < ifm) ? (int)ifp : (int)(ifm + 8));
	} else { // South Polar cap
		int32_t ip = npix - pix;
		iring = (1 + (int32_t)isqrt_uint64(2U * (uint32_t)ip - 1U)) >> 1;
		iphi = 4U * (uint32_t)iring + 1U - ((uint32_t)ip - 2U * (uint32_t)iring * ((uint32_t)iring - 1U));
		kshift = 0;
		nr = iring;
		iring = 2 * nl2 - iring;
		*face_num = (int)special_div((uint32_t)iphi - 1U, (uint32_t)nr) + 8;
	}
	int32_t irt = iring - ((int32_t)jrll[*face_num] * (int32_t)nside) + 1;
	int32_t ipt = 2 * iphi - (int32_t)jpll[*face_num] * nr - kshift - 1;
	if (ipt >= nl2) {
		ipt -= 8 * (int32_t)nside;
	}
	*ix = (int)((ipt - irt) >> 1);
	*iy = (int)((-(ipt + irt)) >> 1);
}

static void get_ring_info_small(uint32_t nside, uint32_t ring, uint32_t *startpix, uint32_t *ringpix, int *shifted) {
	uint32_t ncap = 2U * nside * (nside - 1U);
	uint32_t npix = 12U * nside * nside;
	if (ring < nside) {
		*shifted = 1;
		*ringpix = 4U * ring;
		*startpix = 2U * ring * (ring - 1U);
	} else if (ring < 3U * nside) {
		*shifted = ((ring - nside) & 1U) == 0 ? 1 : 0;
		*ringpix = 4U * nside;
		*startpix = ncap + (ring - nside) * *ringpix;
	} else {
		*shifted = 1;
		uint32_t nr = 4U * nside - ring;
		*ringpix = 4U * nr;
		*startpix = npix - 2U * nr * (nr + 1U);
	}
}

static uint32_t xyf2ring(uint32_t nside, int ix, int iy, int face_num) {
	uint32_t nl4 = 4U * nside;
	int32_t jr = (int32_t)jrll[face_num] * (int32_t)nside - (int32_t)ix - (int32_t)iy - 1;
	uint32_t nr, n_before;
	int shifted;
	get_ring_info_small(nside, (uint32_t)jr, &n_before, &nr, &shifted);
	nr >>= 2;
	int kshift = 1 - shifted;
	int32_t jp = ((int32_t)jpll[face_num] * (int32_t)nr + (int32_t)ix - (int32_t)iy + 1 + (int32_t)kshift) / 2;
	if (jp < 1) {
		jp += (int32_t)nl4;
	}
	return n_before + (uint32_t)jp - 1U;
}

void indigo_healpix_neighbours_ring(uint32_t nside, uint32_t ipix, int32_t *neigh) {
	int ix, iy, face_num;
	ring2xyf(nside, ipix, &ix, &iy, &face_num);
	int nsm1 = (int)nside - 1;
	if ((ix > 0) && (ix < nsm1) && (iy > 0) && (iy < nsm1)) {
		for (int m = 0; m < 8; ++m) {
			neigh[m] = (int32_t)xyf2ring(nside, ix + nb_xoffset[m], iy + nb_yoffset[m], face_num);
		}
	} else {
		for (int i = 0; i < 8; ++i) {
			int x = ix + nb_xoffset[i];
			int y = iy + nb_yoffset[i];
			int nbnum = 4;
			if (x < 0) {
				x += (int)nside;
				nbnum -= 1;
			} else if (x >= (int)nside) {
				x -= (int)nside;
				nbnum += 1;
			}
			if (y < 0) {
				y += (int)nside;
				nbnum -= 3;
			} else if (y >= (int)nside) {
				y -= (int)nside;
				nbnum += 3;
			}
			int f = nb_facearray[nbnum][face_num];
			if (f >= 0) {
				int bits = nb_swaparray[nbnum][face_num >> 2];
				if (bits & 1){
					x = (int)nside - x - 1;
				}
				if (bits & 2) {
					y = (int)nside - y - 1;
				}
				if (bits & 4) {
					int temp = x;
					x = y;
					y = temp;
				}
				neigh[i] = (int32_t)xyf2ring(nside, x, y, f);
			} else {
				neigh[i] = -1;
			}
		}
	}
}
