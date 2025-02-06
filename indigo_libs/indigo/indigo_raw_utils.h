// Copyright (c) 2019 CloudMakers, s. r. o.
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

#ifndef indigo_guider_utils_h
#define indigo_guider_utils_h

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

#define INDIGO_MAX_MULTISTAR_COUNT 24

typedef struct {
	double x;             /* Star X */
	double y;             /* Star Y */
	double nc_distance;   /* Normalized distance from center of the frame */
	double luminance;     /* Star Brightness */
	bool oversaturated;		/* The star is oversaturated */
	bool close_to_other;	/* The star is in close proximity to ather star or has a duplicate artefact */
} indigo_star_detection;

typedef enum {
	none = 0,
	centroid,
	donuts
} indigo_guide_algorithm;

typedef enum {
	fwhm = 0,
	hfd,
	peak
} indigo_psf_param;

typedef struct {
	indigo_guide_algorithm algorithm;
	int width;
	int height;
	double snr;
	union {
		double (*fft_x)[2];
		double centroid_x;
	};
	union {
		double (*fft_y)[2];
		double centroid_y;
	};
} indigo_frame_digest;


INDIGO_EXTERN double indigo_stddev(double set[], const int count);
INDIGO_EXTERN double indigo_rmse(double set[], const int count);

INDIGO_EXTERN bool indigo_is_bayered_image(indigo_raw_header *header, size_t data_length);
INDIGO_EXTERN indigo_result indigo_equalize_bayer_channels(indigo_raw_type raw_type, void *data, const int width, const int height);

INDIGO_EXTERN indigo_result indigo_find_stars(indigo_raw_type raw_type, const void *data, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found);
INDIGO_EXTERN indigo_result indigo_find_stars_precise(indigo_raw_type raw_type, const void *data, const uint16_t radius, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found);
INDIGO_EXTERN indigo_result indigo_find_stars_precise_filtered(indigo_raw_type raw_type, const void *data, const uint16_t radius, const int width, const int height, const int stars_max, indigo_star_detection star_list[], int *stars_found);
INDIGO_EXTERN indigo_result indigo_find_stars_precise_clipped(indigo_raw_type raw_type, const void *data, const uint16_t radius, const int width, const int height, const int stars_max, const int include_left, const int include_top, const int include_width, const int include_height, const int exclude_left, const int exclude_top, const int exclude_width, const int exclude_height, indigo_star_detection star_list[], int *stars_found);
INDIGO_EXTERN indigo_result indigo_selection_psf(indigo_raw_type raw_type, const void *data, double x, double y, const int radius, const int width, const int height, double *fwhm, double *hfd, double *peak);

INDIGO_EXTERN indigo_result indigo_selection_frame_digest(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *digest);
INDIGO_EXTERN indigo_result indigo_selection_frame_digest_iterative(indigo_raw_type raw_type, const void *data, double *x, double *y, const int radius, const int width, const int height, indigo_frame_digest *digest, int converge_iterations);
INDIGO_EXTERN indigo_result indigo_reduce_multistar_digest(const indigo_frame_digest *avg_ref, const indigo_frame_digest ref[], const indigo_frame_digest new_digest[], const int count, indigo_frame_digest *digest);
INDIGO_EXTERN indigo_result indigo_reduce_weighted_multistar_digest(const indigo_frame_digest *avg_ref, const indigo_frame_digest ref[], const indigo_frame_digest new_digest[], const int count, indigo_frame_digest *digest);
INDIGO_EXTERN indigo_result indigo_centroid_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *digest);
INDIGO_EXTERN indigo_result indigo_donuts_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, const int border, indigo_frame_digest *digest);
INDIGO_EXTERN indigo_result indigo_donuts_frame_digest_clipped(indigo_raw_type raw_type, const void *data, const int width, const int height, const int include_left, const int include_top, const int include_width, const int include_height, indigo_frame_digest *digest);
INDIGO_EXTERN indigo_result indigo_calculate_drift(const indigo_frame_digest *ref, const indigo_frame_digest *new_digest, double *drift_x, double *drift_y);
INDIGO_EXTERN double indigo_guider_reponse(double p_gain, double i_gain, double guide_cycle_time, double drift, double avg_drift);
INDIGO_EXTERN indigo_result indigo_delete_frame_digest(indigo_frame_digest *fdigest);

//RMSE focus related
INDIGO_EXTERN double indigo_contrast(indigo_raw_type raw_type, const void *data, const uint8_t *saturation_mask, const int width, const int height, bool *saturated);
INDIGO_EXTERN indigo_result indigo_init_saturation_mask(const int width, const int height, uint8_t **mask);
INDIGO_EXTERN indigo_result indigo_update_saturation_mask(indigo_raw_type raw_type, const void *data, const int width, const int height, uint8_t *mask);

//INDIGO_EXTERN double indigo_stddev_masked_8(uint8_t set[], uint8_t mask[], const int count, bool *saturated);
//INDIGO_EXTERN double indigo_stddev_masked_16(uint16_t set[], uint8_t mask[], const int count, bool *saturated);
//INDIGO_EXTERN double indigo_stddev_8(uint8_t set[], const int count, bool *saturated);
//INDIGO_EXTERN double indigo_stddev_16(uint16_t set[], const int count, bool *saturated);
//INDIGO_EXTERN double indigo_stddev_rgba32(uint8_t set[], const int count, bool *saturated);
//INDIGO_EXTERN double indigo_stddev_abgr32(uint8_t set[], const int count, bool *saturated);

INDIGO_EXTERN indigo_result indigo_make_psf_map(indigo_raw_type image_raw_type, const void *image_data, const uint16_t radius, const int image_width, const int image_height, const int stars_max, indigo_raw_type map_raw_type, indigo_psf_param map_type, int map_width, int map_height, unsigned char *map_data, double *psf_min, double *psf_max);

// Bahtinov images analysis related
INDIGO_EXTERN uint8_t* indigo_binarize(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma);
INDIGO_EXTERN void indigo_skeletonize(uint8_t* data, int width, int height);
INDIGO_EXTERN double indigo_bahtinov_error(indigo_raw_type raw_type, const void *data, const int width, const int height, double sigma, double *rho1, double *theta1, double *rho2, double *theta2, double *rho3, double *theta3);

#ifdef __cplusplus
}
#endif

#endif /* indigo_guider_utils_h */
