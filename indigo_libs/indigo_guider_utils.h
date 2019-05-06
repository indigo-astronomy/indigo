//
//  indigo_guider_utils.h
//  indigo
//
//  Created by Peter Polakovic on 14/04/2019.
//  Copyright © 2019 CloudMakers, s. r. o. All rights reserved.
//
//  Based on libguider by Rumen Bogdanovski
//  Copyright © 2015 Rumen Bogdanovski All rights reserved.
//

#ifndef indigo_guider_utils_h
#define indigo_guider_utils_h

#include <stdio.h>


typedef enum { none = 0, centroid, donuts } indigo_guide_algorithm;

typedef struct {
	indigo_guide_algorithm algorithm;
	int width;
	int height;
	union {
		double (*fft_x)[2];
		double centroid_x;
	};
	union {
		double (*fft_y)[2];
		double centroid_y;
	};
} indigo_frame_digest;

extern indigo_result indigo_centroid_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, indigo_frame_digest *c);
extern indigo_result indigo_donuts_frame_digest(indigo_raw_type raw_type, const void *data, const int width, const int height, int gradient_removal, indigo_frame_digest *fdigest);
extern indigo_result indigo_calculate_drift(const indigo_frame_digest *ref, const indigo_frame_digest *new, double *drift_x, double *drift_y);
extern indigo_result indigo_delete_frame_digest(indigo_frame_digest *fdigest);

#endif /* indigo_guider_utils_h */
