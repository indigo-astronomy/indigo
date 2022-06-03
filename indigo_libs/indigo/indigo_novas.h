// Copyright (c) 2017 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO NOVAS wrapper
 \file indigo_novas.h
 */

#ifndef indigo_novas_h
#define indigo_novas_h

#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UT2JD(t) ((t) / 86400.0 + 2440587.5 + DELTA_UTC_UT1)
#define JD UT2JD(time(NULL))
#define JD2000       2451545.0

extern const double DELTA_T;
extern const double DELTA_UTC_UT1;

extern double indigo_mean_gst(time_t *utc);
extern double indigo_lst(time_t *utc, double longitude);
extern void indigo_eq2hor(time_t *utc, double latitude, double longitude, double elevation, double ra, double dec, double *alt, double *az);
extern void indigo_app_star(double promora, double promodec, double parallax, double rv, double *ra, double *dec);
extern void indigo_topo_star(double latitude, double longitude, double elevation, double promora, double promodec, double parallax, double rv, double *ra, double *dec);
extern void indigo_topo_planet(double latitude, double longitude, double elevation, int id, double *ra_j2k, double *dec_j2k, double *ra, double *dec);

#ifdef __cplusplus
}
#endif

#endif /* indigo_novas_h */
