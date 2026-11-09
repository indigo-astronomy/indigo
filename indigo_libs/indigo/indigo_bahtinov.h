// Copyright (c) 2019-2026 CloudMakers, s. r. o.
// Copyright (c) 2026 Rumen Bogdanovski
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
// 3.0 by Rumen Bogdanovski <rumenastro@gmail.com>: Bahtinov mask analysis moved from indigo_raw_utils.c and rewritten

#ifndef indigo_bahtinov_h
#define indigo_bahtinov_h

#include <stdbool.h>
#include <stdint.h>

#include <indigo/indigo_bus.h>

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

/** Result of a Bahtinov mask pattern analysis.
 */
typedef struct {
	double error;				///< signed focus error in pixels: distance of the outer spikes crossing from the central spike
	double rho[3];			///< spikes in normal form x * cos(theta) + y * sin(theta) = rho in pixel index coordinates, [0] is the central spike
	double theta[3];		///< spike angles in radians, in range [0, pi)
	double angle[2];		///< outer spike angles relative to the central spike in degrees ([0] negative, [1] positive)
	double snr[3];			///< signal to noise ratios of the central and the outer spikes
	double confidence;	///< ratio of the best and the best competing outer spike pair (free angle search only, 0 otherwise)
} indigo_bahtinov_result;

INDIGO_EXTERN uint8_t* indigo_binarize(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma);
INDIGO_EXTERN void indigo_skeletonize(uint8_t* data, int width, int height);

/** Analyze an image of a star seen through a Bahtinov mask.
 *  Works with broadband images (continuous spikes), narrowband images (dotted spikes) and undebayered frames.
 *  mask_angle - known outer spike angles relative to the central spike in degrees (e.g. { -12.6, 12.7 }) or NULL to search all angles.
 *  A known angle makes faint and narrowband patterns measurable, but a wrong angle is not always detected and gives wrong results.
 *  Returns false if no reliable pattern was found.
 */
INDIGO_EXTERN bool indigo_bahtinov_analyze(indigo_raw_type raw_type, const void *data, const int width, const int height, const double *mask_angle, indigo_bahtinov_result *result);

/** Compatibility wrapper of indigo_bahtinov_analyze() with free angle search, sigma is ignored.
 *  Returns the absolute focus error in pixels or -1 if no reliable pattern was found.
 */
INDIGO_EXTERN double indigo_bahtinov_error(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma, double *rho1, double *theta1, double *rho2, double *theta2, double *rho3, double *theta3);

#ifdef __cplusplus
}
#endif

#endif /* indigo_bahtinov_h */
