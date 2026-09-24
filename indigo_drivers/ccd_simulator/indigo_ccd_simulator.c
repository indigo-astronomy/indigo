// Copyright (c) 2016-2026 CloudMakers, s. r. o.
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_ccd_simulator.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

//+ include

#include <math.h>
#include <indigo/indigo_align.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigocat/indigocat_star.h>
#include "indigo_ccd_simulator_data.h"

//- include

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_ao_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_ccd_simulator.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300001E
#define DRIVER_NAME          "indigo_ccd_simulator"
#define DRIVER_LABEL         "Camera Simulator"
#define IMAGER_CCD_DEVICE_NAME "CCD Imager Simulator"
#define GUIDER_CCD_DEVICE_NAME "CCD Guider Simulator"
#define BAHTINOV_CCD_DEVICE_NAME "CCD Bahtinov Mask Simulator"
#define DSLR_CCD_DEVICE_NAME "DSLR Simulator"
#define FILE_CCD_DEVICE_NAME "CCD File Simulator"
#define WHEEL_DEVICE_NAME    "CCD Imager Simulator (wheel)"
#define FOCUSER_DEVICE_NAME  "CCD Imager Simulator (focuser)"
#define GUIDER_DEVICE_NAME   "CCD Guider Simulator (guider)"
#define AO_DEVICE_NAME       "CCD Guider Simulator (AO)"
#define PRIVATE_DATA         ((simulator_private_data *)device->private_data)

//+ define

#define FILTER_COUNT         5
#define GUIDER_GUIDE_SCALE   200
#define GUIDER_MAX_STARS     400
#define GUIDER_FOV           7
#define GUIDER_MAX_HOTPIXELS 1500
#define ECLIPSE              360
#define TEMP_UPDATE          5.0
#define DEFOCUS_BLUR_SCALE   15

//- define

#pragma mark - Property definitions

#define GUIDER_MODE_PROPERTY           (PRIVATE_DATA->guider_mode_property)
#define STARS_ITEM                     (GUIDER_MODE_PROPERTY->items + 0)
#define FLIPPED_STARS_ITEM             (GUIDER_MODE_PROPERTY->items + 1)
#define SUN_ITEM                       (GUIDER_MODE_PROPERTY->items + 2)
#define ECLIPSE_ITEM                   (GUIDER_MODE_PROPERTY->items + 3)

#define GUIDER_MODE_PROPERTY_NAME      "GUIDER_MODE"
#define STARS_ITEM_NAME                "STARS"
#define FLIPPED_STARS_ITEM_NAME        "FLIPPED_STARS"
#define SUN_ITEM_NAME                  "SUN"
#define ECLIPSE_ITEM_NAME              "ECLIPSE"

#define SIMULATION_SETUP_PROPERTY      (PRIVATE_DATA->simulation_setup_property)
#define IMAGE_WIDTH_ITEM               (SIMULATION_SETUP_PROPERTY->items + 0)
#define IMAGE_HEIGHT_ITEM              (SIMULATION_SETUP_PROPERTY->items + 1)
#define IMAGE_NOISE_FIX_ITEM           (SIMULATION_SETUP_PROPERTY->items + 2)
#define IMAGE_NOISE_VAR_ITEM           (SIMULATION_SETUP_PROPERTY->items + 3)
#define PER_ERR_CYCLE_ITEM             (SIMULATION_SETUP_PROPERTY->items + 4)
#define PER_ERR_VAL_ITEM               (SIMULATION_SETUP_PROPERTY->items + 5)
#define IMAGE_GRADIENT_ITEM            (SIMULATION_SETUP_PROPERTY->items + 6)
#define IMAGE_ROTATION_ANGLE_ITEM      (SIMULATION_SETUP_PROPERTY->items + 7)
#define AO_ANGLE_ITEM                  (SIMULATION_SETUP_PROPERTY->items + 8)
#define IMAGE_HOTPIXELS_ITEM           (SIMULATION_SETUP_PROPERTY->items + 9)
#define IMAGE_HOTCOL_ITEM              (SIMULATION_SETUP_PROPERTY->items + 10)
#define IMAGE_HOTROW_ITEM              (SIMULATION_SETUP_PROPERTY->items + 11)
#define IMAGE_RA_OFFSET_ITEM           (SIMULATION_SETUP_PROPERTY->items + 12)
#define IMAGE_DEC_OFFSET_ITEM          (SIMULATION_SETUP_PROPERTY->items + 13)
#define LAT_ITEM                       (SIMULATION_SETUP_PROPERTY->items + 14)
#define LONG_ITEM                      (SIMULATION_SETUP_PROPERTY->items + 15)
#define RA_ITEM                        (SIMULATION_SETUP_PROPERTY->items + 16)
#define DEC_ITEM                       (SIMULATION_SETUP_PROPERTY->items + 17)
#define SIDE_OF_PIER_ITEM              (SIMULATION_SETUP_PROPERTY->items + 18)
#define J2000_ITEM                     (SIMULATION_SETUP_PROPERTY->items + 19)
#define MAGNITUDE_LIMIT_ITEM           (SIMULATION_SETUP_PROPERTY->items + 20)
#define ALT_POLAR_ERROR_ITEM           (SIMULATION_SETUP_PROPERTY->items + 21)
#define AZ_POLAR_ERROR_ITEM            (SIMULATION_SETUP_PROPERTY->items + 22)
#define IMAGE_AGE_ITEM                 (SIMULATION_SETUP_PROPERTY->items + 23)

#define SIMULATION_SETUP_PROPERTY_NAME "SIMULATION_SETUP"
#define IMAGE_WIDTH_ITEM_NAME          "IMAGE_WIDTH"
#define IMAGE_HEIGHT_ITEM_NAME         "IMAGE_HEIGHT"
#define IMAGE_NOISE_FIX_ITEM_NAME      "IMAGE_NOISE_FIX"
#define IMAGE_NOISE_VAR_ITEM_NAME      "IMAGE_NOISE_VAR"
#define PER_ERR_CYCLE_ITEM_NAME        "PER_ERR_CYCLE"
#define PER_ERR_VAL_ITEM_NAME          "PER_ERR_VAL"
#define IMAGE_GRADIENT_ITEM_NAME       "IMAGE_GRADIENT"
#define IMAGE_ROTATION_ANGLE_ITEM_NAME "IMAGE_ROTATION_ANGLE"
#define AO_ANGLE_ITEM_NAME             "AO_ANGLE"
#define IMAGE_HOTPIXELS_ITEM_NAME      "IMAGE_HOTPIXELS"
#define IMAGE_HOTCOL_ITEM_NAME         "IMAGE_HOTCOL"
#define IMAGE_HOTROW_ITEM_NAME         "IMAGE_HOTROW"
#define IMAGE_RA_OFFSET_ITEM_NAME      "IMAGE_RA_OFFSET"
#define IMAGE_DEC_OFFSET_ITEM_NAME     "IMAGE_DEC_OFFSET"
#define LAT_ITEM_NAME                  "LAT"
#define LONG_ITEM_NAME                 "LONG"
#define RA_ITEM_NAME                   "RA"
#define DEC_ITEM_NAME                  "DEC"
#define SIDE_OF_PIER_ITEM_NAME         "SIDE_OF_PIER"
#define J2000_ITEM_NAME                "J2000"
#define MAGNITUDE_LIMIT_ITEM_NAME      "MAGNITUDE_LIMIT"
#define ALT_POLAR_ERROR_ITEM_NAME      "ALT_POLAR_ERROR"
#define AZ_POLAR_ERROR_ITEM_NAME       "AZ_POLAR_ERROR"
#define IMAGE_AGE_ITEM_NAME            "IMAGE_AGE"

#define BAHTINOV_SETTINGS_PROPERTY      (PRIVATE_DATA->bahtinov_settings_property)
#define ROTATION_ITEM                   (BAHTINOV_SETTINGS_PROPERTY->items + 0)

#define BAHTINOV_SETTINGS_PROPERTY_NAME "BAHTINOV_SETTINGS"
#define ROTATION_ITEM_NAME              "ROTATION"

#define DSLR_PROGRAM_PROPERTY          (PRIVATE_DATA->dslr_program_property)
#define M_ITEM                         (DSLR_PROGRAM_PROPERTY->items + 0)
#define B_ITEM                         (DSLR_PROGRAM_PROPERTY->items + 1)

#define DSLR_PROGRAM_PROPERTY_NAME     "DSLR_PROGRAM"
#define M_ITEM_NAME                    "M"
#define B_ITEM_NAME                    "B"

#define DSLR_CAPTURE_MODE_PROPERTY      (PRIVATE_DATA->dslr_capture_mode_property)
#define S_ITEM                          (DSLR_CAPTURE_MODE_PROPERTY->items + 0)

#define DSLR_CAPTURE_MODE_PROPERTY_NAME "DSLR_CAPTURE_MODE"
#define S_ITEM_NAME                     "S"

#define DSLR_SHUTTER_PROPERTY          (PRIVATE_DATA->dslr_shutter_property)
#define S_001_ITEM                     (DSLR_SHUTTER_PROPERTY->items + 0)
#define S_01_ITEM                      (DSLR_SHUTTER_PROPERTY->items + 1)
#define S_1_ITEM                       (DSLR_SHUTTER_PROPERTY->items + 2)
#define S_10_ITEM                      (DSLR_SHUTTER_PROPERTY->items + 3)
#define BULB_ITEM                      (DSLR_SHUTTER_PROPERTY->items + 4)

#define DSLR_SHUTTER_PROPERTY_NAME     "DSLR_SHUTTER"
#define S_001_ITEM_NAME                "0.01"
#define S_01_ITEM_NAME                 "0.1"
#define S_1_ITEM_NAME                  "1"
#define S_10_ITEM_NAME                 "10"
#define BULB_ITEM_NAME                 "BULB"

#define DSLR_APERTURE_PROPERTY         (PRIVATE_DATA->dslr_aperture_property)
#define F14_ITEM                       (DSLR_APERTURE_PROPERTY->items + 0)
#define F20_ITEM                       (DSLR_APERTURE_PROPERTY->items + 1)
#define F28_ITEM                       (DSLR_APERTURE_PROPERTY->items + 2)
#define F40_ITEM                       (DSLR_APERTURE_PROPERTY->items + 3)
#define F56_ITEM                       (DSLR_APERTURE_PROPERTY->items + 4)

#define DSLR_APERTURE_PROPERTY_NAME    "DSLR_APERTURE"
#define F14_ITEM_NAME                  "14"
#define F20_ITEM_NAME                  "20"
#define F28_ITEM_NAME                  "28"
#define F40_ITEM_NAME                  "40"
#define F56_ITEM_NAME                  "56"

#define DSLR_COMPRESSION_PROPERTY      (PRIVATE_DATA->dslr_compression_property)
#define JPEG_ITEM                      (DSLR_COMPRESSION_PROPERTY->items + 0)

#define DSLR_COMPRESSION_PROPERTY_NAME "DSLR_COMPRESSION"
#define JPEG_ITEM_NAME                 "JPEG"

#define DSLR_ISO_PROPERTY              (PRIVATE_DATA->dslr_iso_property)
#define ISO100_ITEM                    (DSLR_ISO_PROPERTY->items + 0)
#define ISO200_ITEM                    (DSLR_ISO_PROPERTY->items + 1)
#define ISO400_ITEM                    (DSLR_ISO_PROPERTY->items + 2)

#define DSLR_ISO_PROPERTY_NAME         "DSLR_ISO"
#define ISO100_ITEM_NAME               "100"
#define ISO200_ITEM_NAME               "200"
#define ISO400_ITEM_NAME               "400"

#define DSLR_BATTERY_LEVEL_PROPERTY      (PRIVATE_DATA->dslr_battery_level_property)
#define VALUE_ITEM                       (DSLR_BATTERY_LEVEL_PROPERTY->items + 0)

#define DSLR_BATTERY_LEVEL_PROPERTY_NAME "DSLR_BATTERY_LEVEL"
#define VALUE_ITEM_NAME                  "VALUE"

#define FILE_NAME_PROPERTY             (PRIVATE_DATA->file_name_property)
#define PATH_ITEM                      (FILE_NAME_PROPERTY->items + 0)

#define FILE_NAME_PROPERTY_NAME        "FILE_NAME"
#define PATH_ITEM_NAME                 "PATH"

#define BAYERPAT_PROPERTY              (PRIVATE_DATA->bayerpat_property)
#define BAYERPAT_ITEM                  (BAYERPAT_PROPERTY->items + 0)

#define BAYERPAT_PROPERTY_NAME         "BAYERPAT"
#define BAYERPAT_ITEM_NAME             "BAYERPAT"

#define FOCUSER_SETUP_PROPERTY         (PRIVATE_DATA->focuser_setup_property)
#define FOCUS_ITEM                     (FOCUSER_SETUP_PROPERTY->items + 0)
#define BACKLASH_ITEM                  (FOCUSER_SETUP_PROPERTY->items + 1)
#define BLUR_SCALE_ITEM                (FOCUSER_SETUP_PROPERTY->items + 2)

#define FOCUSER_SETUP_PROPERTY_NAME    "FOCUSER_SETUP"
#define FOCUS_ITEM_NAME                "FOCUS"
#define BACKLASH_ITEM_NAME             "BACKLASH"
#define BLUR_SCALE_ITEM_NAME           "BLUR_SCALE"

#pragma mark - Private data definition

typedef struct {
	int count;
	indigo_property *guider_mode_property;
	indigo_property *simulation_setup_property;
	indigo_property *bahtinov_settings_property;
	indigo_property *dslr_program_property;
	indigo_property *dslr_capture_mode_property;
	indigo_property *dslr_shutter_property;
	indigo_property *dslr_aperture_property;
	indigo_property *dslr_compression_property;
	indigo_property *dslr_iso_property;
	indigo_property *dslr_battery_level_property;
	indigo_property *file_name_property;
	indigo_property *bayerpat_property;
	indigo_property *focuser_setup_property;
	//+ data
	indigo_device *imager, *guider_camera, *bahtinov, *dslr, *file;
	char imager_image[FITS_HEADER_SIZE + 2 * IMAGER_WIDTH * IMAGER_HEIGHT + 2880];
	char *guider_image;
	char bahtinov_image[FITS_HEADER_SIZE + BAHTINOV_WIDTH * BAHTINOV_HEIGHT + 2880];
	char dslr_image[FITS_HEADER_SIZE + 3 * DSLR_WIDTH * DSLR_HEIGHT + 2880];
	char *file_image, *raw_file_image;
	indigo_raw_header file_image_header;
	double ra, dec, lat, lon, ew_error, ns_error, lst;
	int side_of_pier, star_count, star_x[GUIDER_MAX_STARS], star_y[GUIDER_MAX_STARS], star_a[GUIDER_MAX_STARS];
	int hotpixel_x[GUIDER_MAX_HOTPIXELS + 1], hotpixel_y[GUIDER_MAX_HOTPIXELS + 1], eclipse;
	double exposure_deadline[5], streaming_deadline[5];
	bool exposure_active[5], streaming_active[5];
	double target_temperature, current_temperature;
	int current_slot, target_position, current_position, backlash_in, backlash_out;
	double ao_ra_offset, ao_dec_offset, guide_rate;
	//- data
} simulator_private_data;

#pragma mark - Low level code

//+ code

static void imager_ccd_ccd_exposure_handler(indigo_device *device);
static void imager_ccd_ccd_streaming_handler(indigo_device *device);
static void guider_ccd_ccd_exposure_handler(indigo_device *device);
static void guider_ccd_ccd_streaming_handler(indigo_device *device);
static void bahtinov_ccd_ccd_exposure_handler(indigo_device *device);
static void bahtinov_ccd_ccd_streaming_handler(indigo_device *device);
static void dslr_ccd_ccd_exposure_handler(indigo_device *device);
static void dslr_ccd_ccd_streaming_handler(indigo_device *device);
static void file_ccd_ccd_exposure_handler(indigo_device *device);
static void file_ccd_ccd_streaming_handler(indigo_device *device);
static void wheel_slot_handler(indigo_device *device);
static void focuser_position_handler(indigo_device *device);
static void focuser_steps_handler(indigo_device *device);
static void guider_guide_ra_handler(indigo_device *device);
static void guider_guide_dec_handler(indigo_device *device);

static int ccd_index(indigo_device *device) {
	if (device == PRIVATE_DATA->imager) {
		return 0;
	}
	if (device == PRIVATE_DATA->guider_camera) {
		return 1;
	}
	if (device == PRIVATE_DATA->bahtinov) {
		return 2;
	}
	if (device == PRIVATE_DATA->dslr) {
		return 3;
	}
	return 4;
}

static int mags[] = { 760000, 305000, 122000, 49000, 20000, 7800, 3100, 1200, 500 };

static void search_stars(indigo_device *device) {
	double lst = indigo_lst(NULL, LONG_ITEM->number.target);
	if (lst - PRIVATE_DATA->lst >= IMAGE_AGE_ITEM->number.value || PRIVATE_DATA->ra != RA_ITEM->number.value || PRIVATE_DATA->dec != DEC_ITEM->number.value || PRIVATE_DATA->side_of_pier != SIDE_OF_PIER_ITEM->number.value || PRIVATE_DATA->lat != LAT_ITEM->number.value || PRIVATE_DATA->lon != LONG_ITEM->number.value || PRIVATE_DATA->ew_error != ALT_POLAR_ERROR_ITEM->number.value || PRIVATE_DATA->ns_error != AZ_POLAR_ERROR_ITEM->number.value) {
		double h2r = M_PI / 12;
		double d2r = M_PI / 180;
		double mount_ra = RA_ITEM->number.value; // where mount thinks it is pointing
		double mount_dec = DEC_ITEM->number.value;
		indigo_spherical_point_t point;
		indigo_ra_dec_to_point(mount_ra, mount_dec, lst, &point);
		indigo_spherical_point_t point_r = indigo_apply_polar_error(&point, ALT_POLAR_ERROR_ITEM->number.target * DEG2RAD, AZ_POLAR_ERROR_ITEM->number.target * DEG2RAD);
		indigo_point_to_ra_dec(&point_r, lst, &mount_ra, &mount_dec);
		mount_ra *= h2r;
		mount_dec *= d2r;
		double cos_mount_dec = cos(mount_dec);
		double sin_mount_dec = sin(mount_dec);
		double angle = M_PI * IMAGE_ROTATION_ANGLE_ITEM->number.target / 180.0; // image rotation
		if (SIDE_OF_PIER_ITEM->number.value == 1) {
			angle += M_PI;
		}
		double ppr = IMAGE_HEIGHT_ITEM->number.target / GUIDER_FOV / d2r; // pixel/radian ratio
		double radius = GUIDER_FOV * d2r * 2;
		double ppr_cos = ppr * cos(angle);
		double ppr_sin = ppr * sin(angle);
		PRIVATE_DATA->star_count = 0;
		for (indigocat_star_entry *star_data = indigocat_get_star_data(); star_data->hip; star_data++) {
			if (star_data->mag > MAGNITUDE_LIMIT_ITEM->number.value) {
				continue;
			}
			double ra = (J2000_ITEM->number.target != 0 ? star_data->ra : star_data->ra_now) * h2r;
			double dec = (J2000_ITEM->number.target != 0 ? star_data->dec : star_data->dec_now) * d2r;
			double cos_dec = cos(dec);
			double sin_dec = sin(dec);
			double sin_dec_dec = sin_mount_dec * sin_dec;
			double cos_dec_dec = cos_mount_dec * cos_dec;
			double cos_ra_ra = cos(ra - mount_ra);
			double distance = acos(sin_dec_dec + cos_dec_dec * cos_ra_ra);
			if (distance > radius) {
				continue;
			}
			double sin_ra_ra = sin(ra - mount_ra);
			double ccc_ss = cos_dec_dec * cos_ra_ra + sin_dec_dec;
			double sx = cos_dec * sin_ra_ra / ccc_ss;
			double sy = (sin_mount_dec * cos_dec * cos_ra_ra - cos_mount_dec * sin_dec) / ccc_ss;
			double x = ppr_cos * sx + ppr_sin * sy + IMAGE_WIDTH_ITEM->number.target / 2;
			double y = ppr_cos * sy - ppr_sin * sx + IMAGE_HEIGHT_ITEM->number.target / 2;
			if (x >= 0 && x < IMAGE_WIDTH_ITEM->number.target && y >= 0 && y < IMAGE_HEIGHT_ITEM->number.target) {
				//printf("HIP%5d %6.4f %+7.4f %6.1f %6.1f\n", star_data->hip, star_data->ra, star_data->dec, x, y);
				PRIVATE_DATA->star_x[PRIVATE_DATA->star_count] = (int)x;
				PRIVATE_DATA->star_y[PRIVATE_DATA->star_count] = (int)y;
				PRIVATE_DATA->star_a[PRIVATE_DATA->star_count] = mags[(int)star_data->mag];
				if (PRIVATE_DATA->star_count++ == GUIDER_MAX_STARS) {
					break;
				}
			} else {
				continue;
			}
		}
		PRIVATE_DATA->ra = RA_ITEM->number.target;
		PRIVATE_DATA->dec = DEC_ITEM->number.target;
		PRIVATE_DATA->side_of_pier = (int)SIDE_OF_PIER_ITEM->number.target;
		PRIVATE_DATA->lat = LAT_ITEM->number.target;
		PRIVATE_DATA->lon = LONG_ITEM->number.target;
		PRIVATE_DATA->ew_error = ALT_POLAR_ERROR_ITEM->number.target;
		PRIVATE_DATA->ns_error = AZ_POLAR_ERROR_ITEM->number.target;
		PRIVATE_DATA->lst = lst;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d stars, center at %g/%g, seen from %g/%g with polar error %g/%g", PRIVATE_DATA->star_count, PRIVATE_DATA->ra, PRIVATE_DATA->dec, PRIVATE_DATA->lat, PRIVATE_DATA->lon, ALT_POLAR_ERROR_ITEM->number.target, AZ_POLAR_ERROR_ITEM->number.target);
	}
}

/* Defocus blur radius in pixels for the current focuser offset. The offset is
   in focuser steps, BLUR_SCALE_ITEM tells how many steps are worth
   one pixel of blur, so a single step moves the star profile by a fraction of a
   pixel and a full defocus takes tens of steps, like a real focuser does. */

static double defocus_radius(indigo_device *device) {
	double scale = BLUR_SCALE_ITEM->number.value;
	if (scale < 1) {
		scale = 1;
	}
	return fabs(FOCUS_ITEM->number.value) / scale;
}

#ifdef USE_DISK_BLUR
static void disk_blur(uint16_t *input_image, uint16_t *output_image, int width, int height, double radius) {
	int limit = (int)ceil(radius);
	/* The disk edge is anti-aliased, otherwise the covered area - and with it the
	   amount of blur - would jump whenever the radius crosses a whole pixel. */
	double inner = (radius - 0.5) * (radius - 0.5);
	double outer = (radius + 0.5) * (radius + 0.5);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double sum = 0;
			double weight_sum = 0;
			for (int dy = -limit; dy <= limit; dy++) {
				for (int dx = -limit; dx <= limit; dx++) {
					int nx = x + dx;
					int ny = y + dy;
					if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
						double distance = dx * dx + dy * dy;
						double weight;
						if (distance <= inner) {
							weight = 1;
						} else if (distance >= outer) {
							continue;
						} else {
							weight = (outer - distance) / (outer - inner);
						}
						sum += weight * input_image[ny * width + nx];
						weight_sum += weight;
					}
				}
			}
			output_image[y * width + x] = weight_sum > 0 ? (uint16_t)round(sum / weight_sum) : input_image[y * width + x];
		}
	}
}
#define blur_image           disk_blur

#else /* use gaussian blur */

/* Gaussian blur approximated by three passes of a box filter, as in
   http://blog.ivank.net/fastest-gaussian-blur.html by Ivan Kuckir, but with the
   extended box filter of Gwosdek et al. (2011) instead of a plain one: the two
   outermost pixels of the kernel are weighted by a fraction, so the box has an
   effective radius of n + alpha rather than a whole number of pixels. Without it
   the blur would be quantized to integer box widths and small focuser moves
   would either do nothing at all or change the star profile abruptly. */

static void box_blur_h(uint16_t *scl, uint16_t *tcl, int w, int h, int n, double alpha) {
	double iarr = 1 / (2 * n + 1 + 2 * alpha);
	for (int i = 0; i < h; i++) {
		uint16_t *src = scl + i * w;
		uint16_t *dst = tcl + i * w;
		int fv = src[0], lv = src[w - 1];
		/* running sum of src[j - n] .. src[j + n], edge pixels extended */
		int64_t val = (int64_t)n * fv;
		for (int j = 0; j <= n; j++) {
			val += j < w ? src[j] : lv;
		}
		for (int j = 0; j < w; j++) {
			int li = j - n - 1, ri = j + n + 1;
			int lp = li < 0 ? fv : src[li];
			int rp = ri >= w ? lv : src[ri];
			dst[j] = (uint16_t)round((val + alpha * (lp + rp)) * iarr);
			int lo = j - n;
			val += rp - (lo < 0 ? fv : src[lo]);
		}
	}
}

static void box_blur_t(uint16_t *scl, uint16_t *tcl, int w, int h, int n, double alpha) {
	double iarr = 1 / (2 * n + 1 + 2 * alpha);
	for (int i = 0; i < w; i++) {
		uint16_t *src = scl + i;
		uint16_t *dst = tcl + i;
		int fv = src[0], lv = src[(h - 1) * w];
		int64_t val = (int64_t)n * fv;
		for (int j = 0; j <= n; j++) {
			val += j < h ? src[j * w] : lv;
		}
		for (int j = 0; j < h; j++) {
			int li = j - n - 1, ri = j + n + 1;
			int lp = li < 0 ? fv : src[li * w];
			int rp = ri >= h ? lv : src[ri * w];
			dst[j * w] = (uint16_t)round((val + alpha * (lp + rp)) * iarr);
			int lo = j - n;
			val += rp - (lo < 0 ? fv : src[lo * w]);
		}
	}
}

static void gauss_blur(uint16_t *scl, uint16_t *tcl, int w, int h, double r) {
	int length = w * h;
	r = fabs(r);
	/* keep the kernel well inside the frame */
	double max_r = (w < h ? w : h) / 6.0;
	if (r > max_r) {
		r = max_r;
	}
	if (r < 0.05) {
		memcpy(tcl, scl, length * sizeof(uint16_t));
		return;
	}
	/* variances of the three passes add up, so each pass needs r * r / 3 */
	double variance = r * r / 3;
	/* variance of a box of radius n is n * (n + 1) / 3, take the widest one that fits */
	int n = (int)floor((sqrt(1 + 12 * variance) - 1) / 2);
	/* and let alpha make up the difference: variance of the extended box is
	   (2 * sum(k * k, k = 1..n) + 2 * alpha * (n + 1)^2) / (2 * n + 1 + 2 * alpha) */
	double sum_sq = n * (n + 1.0) * (2 * n + 1.0) / 3;
	double denominator = 2 * (n + 1.0) * (n + 1.0) - 2 * variance;
	double alpha = denominator > 0 ? (variance * (2 * n + 1) - sum_sq) / denominator : 0;
	if (alpha < 0) {
		alpha = 0;
	} else if (alpha > 1) {
		alpha = 1;
	}
	/* horizontal and vertical passes are separable and commute, so they can be grouped */
	box_blur_h(scl, tcl, w, h, n, alpha);
	box_blur_h(tcl, scl, w, h, n, alpha);
	box_blur_h(scl, tcl, w, h, n, alpha);
	box_blur_t(tcl, scl, w, h, n, alpha);
	box_blur_t(scl, tcl, w, h, n, alpha);
	box_blur_t(tcl, scl, w, h, n, alpha);
	memcpy(tcl, scl, length * sizeof(uint16_t));
}

#define blur_image           gauss_blur
#endif/* USE_DISK_BLUR */

static void create_frame(indigo_device *device) {
	if (device == PRIVATE_DATA->dslr) {
		unsigned char *raw = (unsigned char *)(PRIVATE_DATA->dslr_image + FITS_HEADER_SIZE);
		int size = DSLR_WIDTH * DSLR_HEIGHT * 3;
		for (int i = 0; i < size; i++) {
			int rgb = indigo_ccd_simulator_rgb_image[i];
			if (rgb < 0xF0) {
				raw[i] = rgb  + (rand() & 0x0F);
			} else {
				raw[i] = rgb;
			}
		}
		if (CCD_IMAGE_FORMAT_NATIVE_ITEM->sw.value) {
			void *data_out;
			unsigned long size_out;
			indigo_raw_to_jpeg_with_quality(device, PRIVATE_DATA->dslr_image + FITS_HEADER_SIZE, DSLR_WIDTH, DSLR_HEIGHT, 24, NULL, &data_out, &size_out, NULL, NULL, 0, 0, 0, (int)CCD_JPEG_SETTINGS_QUALITY_ITEM->number.target);
			if (CCD_PREVIEW_ENABLED_ITEM->sw.value) {
				indigo_process_dslr_preview_image(device, data_out, (int)size_out);
			}
			indigo_process_dslr_image(device, data_out, (int)size_out, ".jpeg", CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
			free(data_out);
		} else {
			indigo_process_image(device, PRIVATE_DATA->dslr_image, DSLR_WIDTH, DSLR_HEIGHT, 24, true, true, NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
		}
	} else if (device == PRIVATE_DATA->file) {
		int bpp = 8;
		switch (PRIVATE_DATA->file_image_header.signature) {
			case INDIGO_RAW_MONO8:
				bpp = 8;
				break;
			case INDIGO_RAW_MONO16:
				bpp = 16;
				break;
			case INDIGO_RAW_RGB24:
				bpp = 24;
				break;
			case INDIGO_RAW_RGB48:
				bpp = 48;
				break;
		}
		int size = PRIVATE_DATA->file_image_header.width * PRIVATE_DATA->file_image_header.height * bpp / 8;
#if 0 // move image
		static int frame_counter = 0;
		static int x_offset = 0;
		static int y_offset = 0;
		if (frame_counter++ % 2) {
			x_offset = (x_offset + 1) % 10;
		} else {
			y_offset = (y_offset + 1) % 10;
		}
		int offset = (y_offset * PRIVATE_DATA->file_image_header.width + x_offset) * bpp / 8;
		memcpy(PRIVATE_DATA->file_image, PRIVATE_DATA->raw_file_image, FITS_HEADER_SIZE);
		memcpy(PRIVATE_DATA->file_image + FITS_HEADER_SIZE, PRIVATE_DATA->raw_file_image + FITS_HEADER_SIZE + offset, size - offset);
		if (offset) {
			memcpy(PRIVATE_DATA->file_image + FITS_HEADER_SIZE + size - offset, PRIVATE_DATA->raw_file_image + FITS_HEADER_SIZE, offset);
		}
#else
		memcpy(PRIVATE_DATA->file_image, PRIVATE_DATA->raw_file_image, size + FITS_HEADER_SIZE);
#endif
		indigo_fits_keyword keywords[] = {
			{ INDIGO_FITS_STRING, "BAYERPAT", .string = BAYERPAT_ITEM->text.value, "Bayer color pattern" },
			{ 0 }
		};

		double radius = defocus_radius(device);
		if (radius > 0 && PRIVATE_DATA->file_image_header.signature == INDIGO_RAW_MONO16) {
			char *tmp = indigo_alloc_blob_buffer(size + FITS_HEADER_SIZE);
			blur_image((uint16_t *)(PRIVATE_DATA->file_image + FITS_HEADER_SIZE), (uint16_t *)(tmp + FITS_HEADER_SIZE), PRIVATE_DATA->file_image_header.width, PRIVATE_DATA->file_image_header.height, radius);
			indigo_process_image(device, tmp, PRIVATE_DATA->file_image_header.width, PRIVATE_DATA->file_image_header.height, bpp, true, true, strlen(BAYERPAT_ITEM->text.value) == 4 ? keywords : NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
			free(tmp);
		} else {
			indigo_process_image(device, PRIVATE_DATA->file_image, PRIVATE_DATA->file_image_header.width, PRIVATE_DATA->file_image_header.height, bpp, true, true, strlen(BAYERPAT_ITEM->text.value) == 4 ? keywords : NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
		}
	} else if (device == PRIVATE_DATA->bahtinov) {
		double angle = ROTATION_ITEM->number.value * M_PI / 180.0;
		int focus = PRIVATE_DATA->current_position;
		if (focus > BAHTINOV_MAX_STEPS) {
			focus = BAHTINOV_MAX_STEPS;
		} else if (focus < -BAHTINOV_MAX_STEPS) {
			focus = -BAHTINOV_MAX_STEPS;
		}
#ifdef BAHTINOV_ASYMETRIC
		uint8_t (*source_pixels)[BAHTINOV_WIDTH] = (uint8_t (*)[BAHTINOV_HEIGHT]) indigo_ccd_simulator_bahtinov_image[focus + BAHTINOV_MAX_STEPS];
#else
		if (focus < 0) {
			focus = -focus;
			angle += M_PI;
		}
		uint8_t (*source_pixels)[BAHTINOV_WIDTH] = (uint8_t (*)[BAHTINOV_HEIGHT]) indigo_ccd_simulator_bahtinov_image[focus];
#endif
		uint8_t (*target_pixels)[BAHTINOV_WIDTH] = (uint8_t (*)[BAHTINOV_HEIGHT]) (PRIVATE_DATA->bahtinov_image + FITS_HEADER_SIZE);
		if (angle == 0) {
			for (int y = 0; y < BAHTINOV_HEIGHT; y++) {
				for (int x = 0; x < BAHTINOV_WIDTH; x++) {
					target_pixels[y][x] = (source_pixels[y][x] & 0xFC) | (rand() & 0x03);
				}
			}
		} else {
			int cx = BAHTINOV_WIDTH / 2;
			int cy = BAHTINOV_HEIGHT / 2;
			for (int j = 0; j < BAHTINOV_WIDTH; j++) {
				for (int i = 0; i < BAHTINOV_HEIGHT; i++) {
					target_pixels[j][i] = rand() & 0x03;
				}
			}
			double c = cos(angle);
			double s = sin(angle);
			for (int y = 0; y < BAHTINOV_HEIGHT; y++) {
				for (int x = 0; x < BAHTINOV_WIDTH; x++) {
					int src_x = (int)((x - cx) * c + (y - cy) * s + cx);
					int src_y = (int)(-(x - cx) * s + (y - cy) * c + cy);
					if (src_x >= 0 && src_x < BAHTINOV_WIDTH && src_y >= 0 && src_y < BAHTINOV_HEIGHT) {
						target_pixels[y][x] = (source_pixels[src_y][src_x] & 0xFC) | (rand() & 0x03);
					}
				}
			}
		}
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_process_image(device, PRIVATE_DATA->bahtinov_image, BAHTINOV_WIDTH, BAHTINOV_HEIGHT, 8, true, true, NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
		}
	} else {
	uint16_t *raw = (uint16_t *)((device == PRIVATE_DATA->guider_camera ? PRIVATE_DATA->guider_image : PRIVATE_DATA->imager_image) + FITS_HEADER_SIZE);
		int horizontal_bin = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int vertical_bin = (int)CCD_BIN_VERTICAL_ITEM->number.value;
		int frame_left = (int)CCD_FRAME_LEFT_ITEM->number.value / horizontal_bin;
		int frame_top = (int)CCD_FRAME_TOP_ITEM->number.value / vertical_bin;
		int frame_width = (int)CCD_FRAME_WIDTH_ITEM->number.value / horizontal_bin;
		int frame_height = (int)CCD_FRAME_HEIGHT_ITEM->number.value / vertical_bin;
		int size = frame_width * frame_height;
		double gain = (CCD_GAIN_ITEM->number.value / 100);
		int offset = (int)CCD_OFFSET_ITEM->number.value;
		double gamma = CCD_GAMMA_ITEM->number.value;
		bool light_frame = CCD_FRAME_TYPE_LIGHT_ITEM->sw.value || CCD_FRAME_TYPE_FLAT_ITEM->sw.value;

		if (device == PRIVATE_DATA->imager && light_frame) {
			for (int j = 0; j < frame_height; j++) {
				int jj = (frame_top + j) * vertical_bin;
				for (int i = 0; i < frame_width; i++) {
					raw[j * frame_width + i] = indigo_ccd_simulator_raw_image[jj * IMAGER_WIDTH + (frame_left + i) * horizontal_bin];
				}
			}
		} else if (device == PRIVATE_DATA->guider_camera) {
			for (int j = 0; j < frame_height; j++) {
				int jj = j * j;
				for (int i = 0; i < frame_width; i++) {
					raw[j * frame_width + i] = (uint16_t)(IMAGE_GRADIENT_ITEM->number.target * sqrt(i * i + jj));
				}
			}
		} else {
			for (int i = 0; i < size; i++) {
				raw[i] = (rand() & 0x7F);
			}
		}
		if (device == PRIVATE_DATA->guider_camera && light_frame) {
			static time_t start_time = 0;
			if (start_time == 0) {
				start_time = time(NULL);
			}
			search_stars(device);
			/* Continuous periodic error (no snap-back): a smooth, bipolar sine.
			   PER_ERR_VAL is the amplitude (px) and PER_ERR_CYCLE the worm period
			   in seconds (a cycle of 0 disables the periodic error). */
			double pe_seconds = (double)(time(NULL) - start_time);
			double pe_cycle = PER_ERR_CYCLE_ITEM->number.target;
			double ra_offset = (pe_cycle > 0 ? PER_ERR_VAL_ITEM->number.target * sin(2.0 * M_PI * pe_seconds / pe_cycle) : 0.0) + IMAGE_RA_OFFSET_ITEM->number.value;
			double guider_sin = sin(M_PI * IMAGE_ROTATION_ANGLE_ITEM->number.target / 180.0);
			double guider_cos = cos(M_PI * IMAGE_ROTATION_ANGLE_ITEM->number.target / 180.0);
			double ao_sin = sin(M_PI * AO_ANGLE_ITEM->number.target / 180.0);
			double ao_cos = cos(M_PI * AO_ANGLE_ITEM->number.target / 180.0);
			double x_offset = ra_offset * guider_cos - IMAGE_DEC_OFFSET_ITEM->number.value * guider_sin + PRIVATE_DATA->ao_ra_offset * ao_cos - PRIVATE_DATA->ao_dec_offset * ao_sin + (rand() / (double)RAND_MAX)/10.0 - 0.1;
			double y_offset = ra_offset * guider_sin + IMAGE_DEC_OFFSET_ITEM->number.value * guider_cos + PRIVATE_DATA->ao_ra_offset * ao_sin + PRIVATE_DATA->ao_dec_offset * ao_cos + (rand() / (double)RAND_MAX)/10.0 - 0.1;
			bool y_flip = FLIPPED_STARS_ITEM->sw.value;
			if (STARS_ITEM->sw.value || y_flip) {
				for (int i = 0; i < PRIVATE_DATA->star_count; i++) {
					double center_x = (PRIVATE_DATA->star_x[i] + x_offset) / horizontal_bin;
					if (center_x < 0) {
						center_x += IMAGE_WIDTH_ITEM->number.target;
					}
					if (center_x >= IMAGE_WIDTH_ITEM->number.target) {
						center_x -= IMAGE_WIDTH_ITEM->number.target;
					}
					double center_y = (PRIVATE_DATA->star_y[i] + (y_flip ? -y_offset : y_offset)) / vertical_bin;
					if (center_y < 0) {
						center_y += IMAGE_HEIGHT_ITEM->number.target;
					}
					if (center_y >= IMAGE_HEIGHT_ITEM->number.target) {
						center_y -= IMAGE_HEIGHT_ITEM->number.target;
					}
					center_x -= frame_left;
					center_y -= frame_top;
					int a = PRIVATE_DATA->star_a[i];
					int xMax = (int)round(center_x) + 8 / horizontal_bin;
					int yMax = (int)round(center_y) + 8 / vertical_bin;
					for (int y = yMax - 16 / vertical_bin; y <= yMax; y++) {
						if (y < 0 || y >= frame_height) {
							continue;
						}
						int yw = y * frame_width;
						double yy = center_y - y;
						for (int x = xMax - 16 / horizontal_bin; x <= xMax; x++) {
							if (x < 0 || x >= frame_width) {
								continue;
							}
						double xx = center_x - x;
						double v = a * exp(-(xx * xx / 4 + yy * yy / 4));
						double value = raw[yw + x] + v;
						raw[yw + x] = value > 65535 ? 65535 : (uint16_t)value;
						}
					}
				}
			} else {
				double center_x = (IMAGE_WIDTH_ITEM->number.target / 2 + x_offset) / horizontal_bin - frame_left;
				double center_y = (IMAGE_HEIGHT_ITEM->number.target / 2 + y_offset) / vertical_bin - frame_top;
				double eclipse_x = (IMAGE_WIDTH_ITEM->number.target / 2 + PRIVATE_DATA->eclipse + x_offset) / horizontal_bin - frame_left;
				double eclipse_y = (IMAGE_HEIGHT_ITEM->number.target / 2 + PRIVATE_DATA->eclipse + y_offset) / vertical_bin - frame_top;
				for (int y = 0; y <= IMAGE_HEIGHT_ITEM->number.target / vertical_bin; y++) {
					if (y < 0 || y >= frame_height) {
						continue;
					}
					int yw = y * frame_width;
					double yy = (center_y - y) * vertical_bin;
					double eclipse_yy = (eclipse_y - y) * vertical_bin;
					for (int x = 0; x <= IMAGE_WIDTH_ITEM->number.target / horizontal_bin; x++) {
						if (x < 0 || x >= frame_width) {
							continue;
						}
						double xx = (center_x - x) * horizontal_bin;
						double eclipse_xx = (eclipse_x - x) * horizontal_bin;
						double value = 500000 * exp(-((xx * xx + yy * yy) / 20000.0));
						if (ECLIPSE_ITEM->sw.value && eclipse_xx*eclipse_xx+eclipse_yy*eclipse_yy < 50000) {
							value = 0;
						}
						if (value < 65535) {
							raw[yw + x] += (unsigned short)value;
						} else {
							raw[yw + x] = 65535;
						}
					}
				}
				if (ECLIPSE_ITEM->sw.value) {
					PRIVATE_DATA->eclipse++;
					if (PRIVATE_DATA->eclipse > ECLIPSE) {
						PRIVATE_DATA->eclipse = -ECLIPSE;
					}
				}
			}
		}
		for (int i = 0; i < size; i++) {
			double value = raw[i] - offset;
			if (value < 0) {
				value = 0;
			}
			value = gain * pow(value, gamma);
			if (value > 65535) {
				value = 65535;
			}
			raw[i] = (unsigned short)value;
		}
		double radius = defocus_radius(device);
		if (radius > 0) {
			uint16_t *tmp = indigo_safe_malloc(2 * size);
			blur_image(raw, tmp, frame_width, frame_height, radius);
			memcpy(raw, tmp, 2 * size);
			free(tmp);
		}
		int value;
		if (device == PRIVATE_DATA->imager && light_frame) {
			for (int i = 0; i < size; i++) {
				value = raw[i] + (rand() & 0x7F);
				raw[i] = (value > 65535) ? 65535 : value;
			}
		} else if (device == PRIVATE_DATA->guider_camera) {
			for (int i = 0; i < size; i++) {
				value = raw[i] + (rand() % (int)IMAGE_NOISE_VAR_ITEM->number.target) + (int)IMAGE_NOISE_FIX_ITEM->number.target;
				raw[i] = (value > 65535) ? 65535 : value;
			}
		} else {
			for (int i = 0; i < size; i++) {
				raw[i] = (rand() & 0x7F);
			}
		}

		for (int i = 0; i <= IMAGE_HOTPIXELS_ITEM->number.target; i++) {
			int x = PRIVATE_DATA->hotpixel_x[i] / horizontal_bin - frame_left;
			int y = PRIVATE_DATA->hotpixel_y[i] / vertical_bin - frame_top;
			if (x < 0 || x >= frame_width || y < 0 || y > frame_height) {
				continue;
			}
			if (i) {
				raw[y * frame_width + x] = 0xFFFF;
			} else {
				int col_length = (int)fmin(frame_height, IMAGE_HOTCOL_ITEM->number.target);
				int row_length = (int)fmin(frame_width, IMAGE_HOTROW_ITEM->number.target);
				for (int j = 0; j < col_length; j++) {
					raw[j * frame_width + x] = 0xFFFF;
				}
				for (int j = 0; j < row_length; j++) {
					raw[y * frame_width + j] = 0xFFFF;
				}
			}
		}
		int bpp = 16;
		if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value == 8) {
			uint8_t *raw8 = (uint8_t *)raw;
			bpp = 8;
			for (int i = 0; i < size; i++) {
				raw8[i] = (uint8_t)(raw[i] >> 8);
			}
		} else if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value != 16) {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_process_image(device, device == PRIVATE_DATA->guider_camera ? PRIVATE_DATA->guider_image : PRIVATE_DATA->imager_image, frame_width, frame_height, bpp, true, true, NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
		}
	}
}

static void exposure_finalizer(indigo_device *device) {
	int index = ccd_index(device);
	if (!PRIVATE_DATA->exposure_active[index] || !IS_CONNECTED || CCD_EXPOSURE_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	double remaining = PRIVATE_DATA->exposure_deadline[index] - indigo_monotonic_time();
	if (remaining > 0) {
		indigo_execute_handler_in(device, remaining, exposure_finalizer);
		return;
	}
	PRIVATE_DATA->exposure_active[index] = false;
	if (device != PRIVATE_DATA->dslr || !CCD_UPLOAD_MODE_NONE_ITEM->sw.value) {
		create_frame(device);
	}
	CCD_EXPOSURE_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_OK_STATE, NULL);
}

static void streaming_finalizer(indigo_device *device) {
	int index = ccd_index(device);
	if (!PRIVATE_DATA->streaming_active[index] || !IS_CONNECTED || CCD_STREAMING_PROPERTY->state != INDIGO_BUSY_STATE) {
		return;
	}
	double remaining = PRIVATE_DATA->streaming_deadline[index] - indigo_monotonic_time();
	if (remaining > 0) {
		indigo_execute_handler_in(device, remaining, streaming_finalizer);
		return;
	}
	if (device != PRIVATE_DATA->dslr || !CCD_UPLOAD_MODE_NONE_ITEM->sw.value) {
		create_frame(device);
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
		CCD_STREAMING_COUNT_ITEM->number.value--;
	}
	if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
		PRIVATE_DATA->streaming_active[index] = false;
		if (device == PRIVATE_DATA->dslr) {
			indigo_finalize_dslr_video_stream(device);
		} else {
			indigo_finalize_video_stream(device);
		}
		INDIGO_UPDATE_PROPERTY_STATE(CCD_STREAMING_PROPERTY, INDIGO_OK_STATE, NULL);
		return;
	}
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	PRIVATE_DATA->streaming_deadline[index] += CCD_STREAMING_EXPOSURE_ITEM->number.target;
	if (PRIVATE_DATA->streaming_deadline[index] < indigo_monotonic_time()) {
		PRIVATE_DATA->streaming_deadline[index] = indigo_monotonic_time();
	}
	indigo_execute_handler_in(device, fmax(0, PRIVATE_DATA->streaming_deadline[index] - indigo_monotonic_time()), streaming_finalizer);
}

static void start_exposure(indigo_device *device) {
	int index = ccd_index(device);
	double duration = CCD_EXPOSURE_ITEM->number.target > 0 ? CCD_EXPOSURE_ITEM->number.target : 0.1;
	PRIVATE_DATA->exposure_active[index] = true;
	PRIVATE_DATA->exposure_deadline[index] = indigo_monotonic_time() + duration;
	CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.target = duration;
	CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	indigo_ccd_exposure_setup(device);
	indigo_execute_handler_in(device, duration, exposure_finalizer);
}

static void start_streaming(indigo_device *device) {
	int index = ccd_index(device);
	PRIVATE_DATA->streaming_active[index] = true;
	PRIVATE_DATA->streaming_deadline[index] = indigo_monotonic_time() + CCD_STREAMING_EXPOSURE_ITEM->number.target;
	CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	indigo_execute_handler_in(device, CCD_STREAMING_EXPOSURE_ITEM->number.target, streaming_finalizer);
}

static void abort_acquisition(indigo_device *device) {
	int index = ccd_index(device);
	PRIVATE_DATA->exposure_active[index] = PRIVATE_DATA->streaming_active[index] = false;
	indigo_cancel_pending_handler(device, exposure_finalizer);
	indigo_cancel_pending_handler(device, streaming_finalizer);
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		INDIGO_UPDATE_PROPERTY_STATE(CCD_EXPOSURE_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (device == PRIVATE_DATA->dslr) {
			indigo_finalize_dslr_video_stream(device);
		} else {
			indigo_finalize_video_stream(device);
		}
		INDIGO_UPDATE_PROPERTY_STATE(CCD_STREAMING_PROPERTY, INDIGO_ALERT_STATE, NULL);
	}
	CCD_ABORT_EXPOSURE_ITEM->sw.value = false;
	INDIGO_UPDATE_PROPERTY_STATE(CCD_ABORT_EXPOSURE_PROPERTY, INDIGO_OK_STATE, NULL);
}

static bool open_file_image(indigo_device *device) {
	indigo_uni_handle *handle = indigo_uni_open_file(PATH_ITEM->text.value, -INDIGO_LOG_TRACE);
	if (handle == NULL) {
		return false;
	}
	indigo_raw_header header = { 0 };
	bool result = indigo_uni_read(handle, (char *)&header, sizeof(header));
	int bytes = header.signature == INDIGO_RAW_MONO8 ? 1 : header.signature == INDIGO_RAW_MONO16 ? 2 : header.signature == INDIGO_RAW_RGB24 ? 3 : header.signature == INDIGO_RAW_RGB48 ? 6 : 0;
	bool valid_size = bytes > 0 && header.width > 0 && header.height > 0 && (size_t)header.width <= SIZE_MAX / (size_t)header.height / (size_t)bytes;
	size_t size = valid_size ? (size_t)header.width * (size_t)header.height * (size_t)bytes : 0;
	char *raw_image = result && size > 0 && size <= SIZE_MAX - FITS_HEADER_SIZE ? indigo_alloc_blob_buffer(size + FITS_HEADER_SIZE) : NULL;
	char *image = raw_image != NULL ? indigo_alloc_blob_buffer(size + FITS_HEADER_SIZE) : NULL;
	if (raw_image != NULL && image != NULL) {
		result = indigo_uni_read(handle, raw_image + FITS_HEADER_SIZE, size);
	}
	indigo_uni_close(&handle);
	if (!result || raw_image == NULL || image == NULL) {
		indigo_safe_free(raw_image);
		indigo_safe_free(image);
		return false;
	}
	PRIVATE_DATA->file_image_header = header;
	PRIVATE_DATA->raw_file_image = raw_image;
	PRIVATE_DATA->file_image = image;
	CCD_FRAME_LEFT_ITEM->number.value = CCD_FRAME_TOP_ITEM->number.value = 0;
	CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.min = CCD_FRAME_WIDTH_ITEM->number.max = header.width;
	CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.min = CCD_FRAME_HEIGHT_ITEM->number.max = header.height;
	CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = bytes * 8;
	return true;
}

static void configure_ccd(indigo_device *device, int kind) {
	SIMULATION_PROPERTY->hidden = false;
	SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
	indigo_set_switch(SIMULATION_PROPERTY, SIMULATION_ENABLED_ITEM, true);
	CCD_STREAMING_PROPERTY->hidden = false;
	CCD_STREAMING_EXPOSURE_ITEM->number.min = 0.001;
	CCD_STREAMING_EXPOSURE_ITEM->number.max = 0.5;
	CCD_STREAMING_SETTINGS_PROPERTY->hidden = false;
	if (kind == 0 || kind == 1) {
		int width = kind == 0 ? IMAGER_WIDTH : GUIDER_WIDTH;
		int height = kind == 0 ? IMAGER_HEIGHT : GUIDER_HEIGHT;
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = width;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = height;
		CCD_FRAME_WIDTH_ITEM->number.min = CCD_FRAME_HEIGHT_ITEM->number.min = 32;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
		CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.max = 4;
		CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.max = 4;
		CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
		CCD_MODE_PROPERTY->count = 3;
		char label[32];
		snprintf(label, sizeof(label), "RAW %dx%d", width, height);
		indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", label, true);
		snprintf(label, sizeof(label), "RAW %dx%d", width / 2, height / 2);
		indigo_init_switch_item(CCD_MODE_ITEM + 1, "BIN_2x2", label, false);
		snprintf(label, sizeof(label), "RAW %dx%d", width / 4, height / 4);
		indigo_init_switch_item(CCD_MODE_ITEM + 2, "BIN_4x4", label, false);
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = CCD_GAMMA_PROPERTY->hidden = false;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		CCD_LENS_FOCAL_LENGTH_ITEM->number.value = kind == 0 ? 12.7 : 5.1;
		CCD_LENS_PHYSICAL_LENGTH_ITEM->number.value = kind == 0 ? 12.7 : 5.1;
		CCD_LENS_APERTURE_ITEM->number.value = kind == 0 ? 4 : 2;
		CCD_LENS_PROPERTY->state = INDIGO_OK_STATE;
		for (int i = 0; i <= GUIDER_MAX_HOTPIXELS; i++) {
			PRIVATE_DATA->hotpixel_x[i] = rand() % (width - 200) + 100;
			PRIVATE_DATA->hotpixel_y[i] = rand() % (height - 200) + 100;
		}
		if (kind == 0) {
			CCD_COOLER_PROPERTY->hidden = CCD_TEMPERATURE_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = false;
			CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
			PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = 25;
			CCD_EGAIN_PROPERTY->hidden = false;
			CCD_EGAIN_ITEM->number.value = 0.82;
		} else {
			CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = CCD_TEMPERATURE_PROPERTY->hidden = true;
		}
	} else if (kind == 2) {
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = BAHTINOV_WIDTH;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = BAHTINOV_HEIGHT;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_INFO_BITS_PER_PIXEL_ITEM->number.min = CCD_INFO_BITS_PER_PIXEL_ITEM->number.max = 8;
		CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.max = 1;
		CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.max = 1;
		CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 3.75;
		CCD_IMAGE_FORMAT_PROPERTY->count = 7;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_BIN_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = CCD_TEMPERATURE_PROPERTY->hidden = true;
		CCD_OFFSET_PROPERTY->hidden = CCD_GAMMA_PROPERTY->hidden = CCD_GAIN_PROPERTY->hidden = true;
	} else if (kind == 3) {
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = DSLR_WIDTH;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = DSLR_HEIGHT;
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 24;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		CCD_BIN_PROPERTY->hidden = CCD_COOLER_PROPERTY->hidden = CCD_COOLER_POWER_PROPERTY->hidden = CCD_TEMPERATURE_PROPERTY->hidden = true;
		CCD_OFFSET_PROPERTY->hidden = CCD_GAMMA_PROPERTY->hidden = CCD_GAIN_PROPERTY->hidden = true;
		CCD_JPEG_SETTINGS_PROPERTY->hidden = true;
		CCD_UPLOAD_MODE_PROPERTY->count = 4;
	} else {
		CCD_BIN_PROPERTY->hidden = CCD_INFO_PROPERTY->hidden = true;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
	}
}

static void close_file_image(indigo_device *device) {
	indigo_safe_free(PRIVATE_DATA->file_image);
	indigo_safe_free(PRIVATE_DATA->raw_file_image);
	PRIVATE_DATA->file_image = NULL;
	PRIVATE_DATA->raw_file_image = NULL;
}

//- code

//+ wheel.code

static void wheel_move_finalizer(indigo_device *device) {
	PRIVATE_DATA->current_slot = PRIVATE_DATA->current_slot % (int)WHEEL_SLOT_ITEM->number.max + 1;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == (int)WHEEL_SLOT_ITEM->number.target) {
		INDIGO_UPDATE_PROPERTY_STATE(WHEEL_SLOT_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
}

//- wheel.code

//+ focuser.code

static void focuser_move_finalizer(indigo_device *device) {
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_ALERT_STATE) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return;
	}
	int previous_position = PRIVATE_DATA->current_position;
	int step = (int)FOCUSER_SPEED_ITEM->number.value;
	if (PRIVATE_DATA->current_position < PRIVATE_DATA->target_position) {
		PRIVATE_DATA->current_position += step;
		if (PRIVATE_DATA->current_position > PRIVATE_DATA->target_position) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
		}
	} else if (PRIVATE_DATA->current_position > PRIVATE_DATA->target_position) {
		PRIVATE_DATA->current_position -= step;
		if (PRIVATE_DATA->current_position < PRIVATE_DATA->target_position) {
			PRIVATE_DATA->current_position = PRIVATE_DATA->target_position;
		}
	}
	int moved = abs(PRIVATE_DATA->current_position - previous_position);
	if (PRIVATE_DATA->current_position > previous_position) {
		int backlash = moved < PRIVATE_DATA->backlash_out ? moved : PRIVATE_DATA->backlash_out;
		PRIVATE_DATA->backlash_out -= backlash;
		FOCUS_ITEM->number.value += moved - backlash;
	} else if (PRIVATE_DATA->current_position < previous_position) {
		int backlash = moved < PRIVATE_DATA->backlash_in ? moved : PRIVATE_DATA->backlash_in;
		PRIVATE_DATA->backlash_in -= backlash;
		FOCUS_ITEM->number.value -= moved - backlash;
	}
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_SETUP_PROPERTY, NULL);
	if (PRIVATE_DATA->current_position == PRIVATE_DATA->target_position) {
		FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_POSITION_PROPERTY, INDIGO_OK_STATE, NULL);
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else {
		indigo_execute_handler_in(device, 0.1, focuser_move_finalizer);
	}
}

static void start_focuser_move(indigo_device *device, int target) {
	PRIVATE_DATA->target_position = target;
	bool inward = target < PRIVATE_DATA->current_position;
	if (inward && !FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
		PRIVATE_DATA->backlash_in = BACKLASH_ITEM->number.value > PRIVATE_DATA->backlash_out ? (int)BACKLASH_ITEM->number.value - PRIVATE_DATA->backlash_out : 0;
		PRIVATE_DATA->backlash_out = 0;
	} else if (!inward && !FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
		PRIVATE_DATA->backlash_out = BACKLASH_ITEM->number.value > PRIVATE_DATA->backlash_in ? (int)BACKLASH_ITEM->number.value - PRIVATE_DATA->backlash_in : 0;
		PRIVATE_DATA->backlash_in = 0;
	}
	indigo_set_switch(FOCUSER_DIRECTION_PROPERTY, inward ? FOCUSER_DIRECTION_MOVE_INWARD_ITEM : FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM, true);
	indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
	FOCUSER_POSITION_PROPERTY->state = FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	indigo_execute_handler_in(device, 0.1, focuser_move_finalizer);
}

//- focuser.code

//+ guider.code

static void guider_ra_finalizer(indigo_device *device) {
	if (SIDE_OF_PIER_ITEM->number.value == 0) {
		IMAGE_RA_OFFSET_ITEM->number.value += cos(M_PI * DEC_ITEM->number.value / 180.0) * PRIVATE_DATA->guide_rate * (GUIDER_GUIDE_WEST_ITEM->number.value - GUIDER_GUIDE_EAST_ITEM->number.value) / GUIDER_GUIDE_SCALE;
	} else {
		IMAGE_RA_OFFSET_ITEM->number.value -= cos(M_PI * DEC_ITEM->number.value / 180.0) * PRIVATE_DATA->guide_rate * (GUIDER_GUIDE_WEST_ITEM->number.value - GUIDER_GUIDE_EAST_ITEM->number.value) / GUIDER_GUIDE_SCALE;
	}
	GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_RA_PROPERTY, INDIGO_OK_STATE, NULL);
	indigo_update_property(PRIVATE_DATA->guider_camera, SIMULATION_SETUP_PROPERTY, NULL);
}

static void guider_dec_finalizer(indigo_device *device) {
	IMAGE_DEC_OFFSET_ITEM->number.value += PRIVATE_DATA->guide_rate * (GUIDER_GUIDE_NORTH_ITEM->number.value - GUIDER_GUIDE_SOUTH_ITEM->number.value) / GUIDER_GUIDE_SCALE;
	GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
	INDIGO_UPDATE_PROPERTY_STATE(GUIDER_GUIDE_DEC_PROPERTY, INDIGO_OK_STATE, NULL);
	indigo_update_property(PRIVATE_DATA->guider_camera, SIMULATION_SETUP_PROPERTY, NULL);
}

//- guider.code

#pragma mark - High level code (imager_ccd)
// device_id: imager_ccd type: ccd

static void imager_ccd_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	//+ imager_ccd.on_timer
	if (CCD_COOLER_ON_ITEM->sw.value) {
		double difference = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
		if (difference > 0) {
			PRIVATE_DATA->current_temperature--;
		} else if (difference < 0) {
			PRIVATE_DATA->current_temperature++;
		}
		CCD_COOLER_POWER_ITEM->number.value = PRIVATE_DATA->current_temperature == PRIVATE_DATA->target_temperature ? 20 : fabs(difference) > 5 ? 100 : 50;
	} else {
		CCD_COOLER_POWER_ITEM->number.value = 0;
	}
	CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
	CCD_TEMPERATURE_PROPERTY->state = PRIVATE_DATA->current_temperature == PRIVATE_DATA->target_temperature ? INDIGO_OK_STATE : INDIGO_BUSY_STATE;
	indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	indigo_execute_handler_in(device, TEMP_UPDATE, imager_ccd_timer_callback);
	//- imager_ccd.on_timer
}

static void imager_ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ imager_ccd.on_disconnect
		indigo_cancel_pending_handler(device, imager_ccd_ccd_exposure_handler);
		indigo_cancel_pending_handler(device, imager_ccd_ccd_streaming_handler);
		abort_acquisition(device);
		//- imager_ccd.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	if (IS_CONNECTED) {
		indigo_execute_handler(device, imager_ccd_timer_callback);
	}
}

static void imager_ccd_ccd_exposure_handler(indigo_device *device) {
	//+ imager_ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	start_exposure(device); // exposure_finalizer owns completion
	//- imager_ccd.CCD_EXPOSURE.on_change
}

static void imager_ccd_ccd_streaming_handler(indigo_device *device) {
	//+ imager_ccd.CCD_STREAMING.on_change
	start_streaming(device); // streaming_finalizer owns completion
	//- imager_ccd.CCD_STREAMING.on_change
}

static void imager_ccd_ccd_abort_exposure_handler(indigo_device *device) {
	//+ imager_ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, imager_ccd_ccd_exposure_handler);
	indigo_cancel_pending_handler(device, imager_ccd_ccd_streaming_handler);
	abort_acquisition(device); // cancel exposure_finalizer and streaming_finalizer
	//- imager_ccd.CCD_ABORT_EXPOSURE.on_change
}

static void imager_ccd_ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ imager_ccd.CCD_BIN.on_change
	int horizontal = (int)CCD_BIN_HORIZONTAL_ITEM->number.target;
	int vertical = (int)CCD_BIN_VERTICAL_ITEM->number.target;
	if ((horizontal == 1 || horizontal == 2 || horizontal == 4) && horizontal == vertical) {
		CCD_BIN_HORIZONTAL_ITEM->number.value = horizontal;
		CCD_BIN_VERTICAL_ITEM->number.value = vertical;
	} else {
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- imager_ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void imager_ccd_ccd_cooler_handler(indigo_device *device) {
	CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
	//+ imager_ccd.CCD_COOLER.on_change
	CCD_COOLER_POWER_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_OK_STATE : INDIGO_IDLE_STATE;
	if (CCD_COOLER_OFF_ITEM->sw.value) {
		CCD_COOLER_POWER_ITEM->number.value = 0;
	}
	indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	//- imager_ccd.CCD_COOLER.on_change
	indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
}

static void imager_ccd_ccd_temperature_handler(indigo_device *device) {
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	//+ imager_ccd.CCD_TEMPERATURE.on_change
	PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.target;
	CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
	CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
	//- imager_ccd.CCD_TEMPERATURE.on_change
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
}

#pragma mark - Device API (imager_ccd)

static indigo_result imager_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result imager_ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		//+ imager_ccd.on_attach
		PRIVATE_DATA->imager = device;
		configure_ccd(device, 0);
		//- imager_ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		CCD_COOLER_PROPERTY->hidden = false;
		CCD_TEMPERATURE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return imager_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result imager_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result imager_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(imager_ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress, an exposure can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, imager_ccd_ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress, streaming can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, imager_ccd_ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, imager_ccd_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, imager_ccd_ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_COOLER_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_COOLER_PROPERTY, imager_ccd_ccd_cooler_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_TEMPERATURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_TEMPERATURE_PROPERTY, imager_ccd_ccd_temperature_handler);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result imager_ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		imager_ccd_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (guider_ccd)
// device_id: guider_ccd type: ccd

static void guider_ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_define_property(device, GUIDER_MODE_PROPERTY, NULL);
			indigo_define_property(device, SIMULATION_SETUP_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider_ccd.on_disconnect
		indigo_cancel_pending_handler(device, guider_ccd_ccd_exposure_handler);
		indigo_cancel_pending_handler(device, guider_ccd_ccd_streaming_handler);
		abort_acquisition(device);
		//- guider_ccd.on_disconnect
		indigo_delete_property(device, GUIDER_MODE_PROPERTY, NULL);
		indigo_delete_property(device, SIMULATION_SETUP_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_ccd_ccd_exposure_handler(indigo_device *device) {
	//+ guider_ccd.CCD_EXPOSURE.on_change
	indigo_use_shortest_exposure_if_bias(device);
	start_exposure(device); // exposure_finalizer owns completion
	//- guider_ccd.CCD_EXPOSURE.on_change
}

static void guider_ccd_ccd_streaming_handler(indigo_device *device) {
	//+ guider_ccd.CCD_STREAMING.on_change
	start_streaming(device); // streaming_finalizer owns completion
	//- guider_ccd.CCD_STREAMING.on_change
}

static void guider_ccd_ccd_abort_exposure_handler(indigo_device *device) {
	//+ guider_ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, guider_ccd_ccd_exposure_handler);
	indigo_cancel_pending_handler(device, guider_ccd_ccd_streaming_handler);
	abort_acquisition(device); // cancel exposure_finalizer and streaming_finalizer
	//- guider_ccd.CCD_ABORT_EXPOSURE.on_change
}

static void guider_ccd_ccd_bin_handler(indigo_device *device) {
	CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider_ccd.CCD_BIN.on_change
	int horizontal = (int)CCD_BIN_HORIZONTAL_ITEM->number.target;
	int vertical = (int)CCD_BIN_VERTICAL_ITEM->number.target;
	if ((horizontal == 1 || horizontal == 2 || horizontal == 4) && horizontal == vertical) {
		CCD_BIN_HORIZONTAL_ITEM->number.value = horizontal;
		CCD_BIN_VERTICAL_ITEM->number.value = vertical;
	} else {
		CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- guider_ccd.CCD_BIN.on_change
	indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
}

static void guider_ccd_simulation_setup_handler(indigo_device *device) {
	SIMULATION_SETUP_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider_ccd.SIMULATION_SETUP.on_change
	if (J2000_ITEM->number.target != 0 && J2000_ITEM->number.target != 2000) {
		J2000_ITEM->number.value = J2000_ITEM->number.target = 2000;
	}
	PRIVATE_DATA->ra = PRIVATE_DATA->dec = 0;
	int width = (int)IMAGE_WIDTH_ITEM->number.target;
	int height = (int)IMAGE_HEIGHT_ITEM->number.target;
	IMAGE_HOTCOL_ITEM->number.max = IMAGE_RA_OFFSET_ITEM->number.max = IMAGE_DEC_OFFSET_ITEM->number.max = height;
	IMAGE_HOTROW_ITEM->number.max = width;
	CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = width;
	CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = height;
	snprintf(CCD_MODE_ITEM[0].label, INDIGO_VALUE_SIZE, "RAW %dx%d", width, height);
	snprintf(CCD_MODE_ITEM[1].label, INDIGO_VALUE_SIZE, "RAW %dx%d", width / 2, height / 2);
	snprintf(CCD_MODE_ITEM[2].label, INDIGO_VALUE_SIZE, "RAW %dx%d", width / 4, height / 4);
	PRIVATE_DATA->guider_image = indigo_safe_realloc(PRIVATE_DATA->guider_image, FITS_HEADER_SIZE + 2 * (size_t)width * height + 2880);
	if (IS_CONNECTED) {
		// clients must see the new sensor size, otherwise they keep and send back the old frame
		indigo_update_property(device, CCD_INFO_PROPERTY, NULL);
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		indigo_delete_property(device, CCD_MODE_PROPERTY, NULL);
		indigo_define_property(device, CCD_MODE_PROPERTY, NULL);
	}
	//- guider_ccd.SIMULATION_SETUP.on_change
	indigo_update_property(device, SIMULATION_SETUP_PROPERTY, NULL);
}

#pragma mark - Device API (guider_ccd)

static indigo_result guider_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider_ccd.on_attach
		PRIVATE_DATA->guider_camera = device;
		configure_ccd(device, 1);
		PRIVATE_DATA->guider_image = indigo_alloc_blob_buffer(FITS_HEADER_SIZE + 2 * GUIDER_WIDTH * GUIDER_HEIGHT + 2880);
		//- guider_ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		CCD_BIN_PROPERTY->hidden = false;
		GUIDER_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, GUIDER_MODE_PROPERTY_NAME, MAIN_GROUP, "Simulation Mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (GUIDER_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(STARS_ITEM, STARS_ITEM_NAME, "Stars", true);
		indigo_init_switch_item(FLIPPED_STARS_ITEM, FLIPPED_STARS_ITEM_NAME, "Stars (flipped)", false);
		indigo_init_switch_item(SUN_ITEM, SUN_ITEM_NAME, "Sun", false);
		indigo_init_switch_item(ECLIPSE_ITEM, ECLIPSE_ITEM_NAME, "Eclipse", false);
		SIMULATION_SETUP_PROPERTY = indigo_init_number_property(NULL, device->name, SIMULATION_SETUP_PROPERTY_NAME, MAIN_GROUP, "Simulation Setup", INDIGO_OK_STATE, INDIGO_RW_PERM, 24);
		if (SIMULATION_SETUP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(IMAGE_WIDTH_ITEM, IMAGE_WIDTH_ITEM_NAME, "Image width (px)", 400, 16000, 0, 1600);
		indigo_init_number_item(IMAGE_HEIGHT_ITEM, IMAGE_HEIGHT_ITEM_NAME, "Image height (px)", 300, 12000, 0, 1200);
		indigo_init_number_item(IMAGE_NOISE_FIX_ITEM, IMAGE_NOISE_FIX_ITEM_NAME, "Image noise offset", 0, 5000, 0, 500);
		indigo_init_number_item(IMAGE_NOISE_VAR_ITEM, IMAGE_NOISE_VAR_ITEM_NAME, "Image noise range", 1, 1000, 0, 100);
		indigo_init_number_item(PER_ERR_CYCLE_ITEM, PER_ERR_CYCLE_ITEM_NAME, "Periodic error cycle (s)", 0, 1800, 0, 432);
		indigo_init_number_item(PER_ERR_VAL_ITEM, PER_ERR_VAL_ITEM_NAME, "Periodic error value (px)", 0, 10, 0, 2);
		indigo_init_number_item(IMAGE_GRADIENT_ITEM, IMAGE_GRADIENT_ITEM_NAME, "Image gradient intensity", 0, 0.5, 0, 0.2);
		indigo_init_number_item(IMAGE_ROTATION_ANGLE_ITEM, IMAGE_ROTATION_ANGLE_ITEM_NAME, "Image rotation angle (deg)", 0, 360, 0, 36);
		indigo_init_number_item(AO_ANGLE_ITEM, AO_ANGLE_ITEM_NAME, "AO angle (deg)", 0, 360, 0, 74);
		indigo_init_number_item(IMAGE_HOTPIXELS_ITEM, IMAGE_HOTPIXELS_ITEM_NAME, "Hot pixel count", 0, 1500, 0, 0);
		indigo_init_number_item(IMAGE_HOTCOL_ITEM, IMAGE_HOTCOL_ITEM_NAME, "Hot column length (px)", 0, 1200, 0, 0);
		indigo_init_number_item(IMAGE_HOTROW_ITEM, IMAGE_HOTROW_ITEM_NAME, "Hot row length (px)", 0, 1600, 0, 0);
		indigo_init_number_item(IMAGE_RA_OFFSET_ITEM, IMAGE_RA_OFFSET_ITEM_NAME, "RA offset (px)", 0, 1200, 0, 0);
		indigo_init_number_item(IMAGE_DEC_OFFSET_ITEM, IMAGE_DEC_OFFSET_ITEM_NAME, "DEC offset (px)", 0, 1200, 0, 0);
		indigo_init_number_item(LAT_ITEM, LAT_ITEM_NAME, "Latitude", -90, 90, 0, 48.1485965);
		indigo_init_number_item(LONG_ITEM, LONG_ITEM_NAME, "Longitude", -180, 360, 0, 17.1077478);
		indigo_init_number_item(RA_ITEM, RA_ITEM_NAME, "RA", 0, 24, 0, 18.84);
		indigo_init_number_item(DEC_ITEM, DEC_ITEM_NAME, "Dec", -90, 90, 0, 38.75);
		indigo_init_number_item(SIDE_OF_PIER_ITEM, SIDE_OF_PIER_ITEM_NAME, "Side of pier", 0, 1, 0, 0);
		indigo_init_number_item(J2000_ITEM, J2000_ITEM_NAME, "J2000", 0, 2050, 0, 2000);
		indigo_init_number_item(MAGNITUDE_LIMIT_ITEM, MAGNITUDE_LIMIT_ITEM_NAME, "Magnitude limit", -2, 12, 1, 8);
		indigo_init_number_item(ALT_POLAR_ERROR_ITEM, ALT_POLAR_ERROR_ITEM_NAME, "Altitude polar error", -30, 30, 0, 0);
		indigo_init_number_item(AZ_POLAR_ERROR_ITEM, AZ_POLAR_ERROR_ITEM_NAME, "Azimuth polar error", -30, 30, 0, 0);
		indigo_init_number_item(IMAGE_AGE_ITEM, IMAGE_AGE_ITEM_NAME, "Max image age (s)", 0, 3600, 0, 0.0166667);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(GUIDER_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(SIMULATION_SETUP_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result guider_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(guider_ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress, an exposure can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, guider_ccd_ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress, streaming can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, guider_ccd_ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, guider_ccd_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_BIN_PROPERTY, property)) {
		INDIGO_COPY_TARGETS_PROCESS_CHANGE(CCD_BIN_PROPERTY, guider_ccd_ccd_bin_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_MODE_PROPERTY, property)) {
		indigo_property_copy_values(GUIDER_MODE_PROPERTY, property, false);
		GUIDER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(SIMULATION_SETUP_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(SIMULATION_SETUP_PROPERTY, guider_ccd_simulation_setup_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, GUIDER_MODE_PROPERTY);
			indigo_save_property(device, NULL, SIMULATION_SETUP_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result guider_ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_ccd_connection_handler(device);
	}
	//+ guider_ccd.on_detach
	indigo_safe_free(PRIVATE_DATA->guider_image);
	PRIVATE_DATA->guider_image = NULL;
	//- guider_ccd.on_detach
	indigo_release_property(GUIDER_MODE_PROPERTY);
	indigo_release_property(SIMULATION_SETUP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (bahtinov_ccd)
// device_id: bahtinov_ccd type: ccd

static void bahtinov_ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_define_property(device, BAHTINOV_SETTINGS_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ bahtinov_ccd.on_disconnect
		indigo_cancel_pending_handler(device, bahtinov_ccd_ccd_exposure_handler);
				indigo_cancel_pending_handler(device, bahtinov_ccd_ccd_streaming_handler);
				abort_acquisition(device);
		//- bahtinov_ccd.on_disconnect
		indigo_delete_property(device, BAHTINOV_SETTINGS_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void bahtinov_ccd_ccd_exposure_handler(indigo_device *device) {
	//+ bahtinov_ccd.CCD_EXPOSURE.on_change
	start_exposure(device); // exposure_finalizer owns completion
	//- bahtinov_ccd.CCD_EXPOSURE.on_change
}

static void bahtinov_ccd_ccd_streaming_handler(indigo_device *device) {
	//+ bahtinov_ccd.CCD_STREAMING.on_change
	start_streaming(device); // streaming_finalizer owns completion
	//- bahtinov_ccd.CCD_STREAMING.on_change
}

static void bahtinov_ccd_ccd_abort_exposure_handler(indigo_device *device) {
	//+ bahtinov_ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, bahtinov_ccd_ccd_exposure_handler);
				indigo_cancel_pending_handler(device, bahtinov_ccd_ccd_streaming_handler);
				abort_acquisition(device); // cancel exposure_finalizer and streaming_finalizer
	//- bahtinov_ccd.CCD_ABORT_EXPOSURE.on_change
}

#pragma mark - Device API (bahtinov_ccd)

static indigo_result bahtinov_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result bahtinov_ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ bahtinov_ccd.on_attach
		PRIVATE_DATA->bahtinov = device;
		configure_ccd(device, 2);
		//- bahtinov_ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		BAHTINOV_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, BAHTINOV_SETTINGS_PROPERTY_NAME, MAIN_GROUP, "Bahtinov mask settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (BAHTINOV_SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(ROTATION_ITEM, ROTATION_ITEM_NAME, "Angle", 0, 180, 1, 35);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return bahtinov_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result bahtinov_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(BAHTINOV_SETTINGS_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result bahtinov_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(bahtinov_ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress, an exposure can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, bahtinov_ccd_ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress, streaming can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, bahtinov_ccd_ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, bahtinov_ccd_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(BAHTINOV_SETTINGS_PROPERTY, property)) {
		indigo_property_copy_values(BAHTINOV_SETTINGS_PROPERTY, property, false);
		BAHTINOV_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, BAHTINOV_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, BAHTINOV_SETTINGS_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result bahtinov_ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		bahtinov_ccd_connection_handler(device);
	}
	indigo_release_property(BAHTINOV_SETTINGS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (dslr_ccd)
// device_id: dslr_ccd type: ccd

static void dslr_ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);
			indigo_define_property(device, DSLR_CAPTURE_MODE_PROPERTY, NULL);
			indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, DSLR_APERTURE_PROPERTY, NULL);
			indigo_define_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
			indigo_define_property(device, DSLR_ISO_PROPERTY, NULL);
			indigo_define_property(device, DSLR_BATTERY_LEVEL_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ dslr_ccd.on_disconnect
		indigo_cancel_pending_handler(device, dslr_ccd_ccd_exposure_handler);
				indigo_cancel_pending_handler(device, dslr_ccd_ccd_streaming_handler);
				abort_acquisition(device);
		//- dslr_ccd.on_disconnect
		indigo_delete_property(device, DSLR_PROGRAM_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_CAPTURE_MODE_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_APERTURE_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_ISO_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_BATTERY_LEVEL_PROPERTY, NULL);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void dslr_ccd_ccd_exposure_handler(indigo_device *device) {
	//+ dslr_ccd.CCD_EXPOSURE.on_change
	start_exposure(device); // exposure_finalizer owns completion
	//- dslr_ccd.CCD_EXPOSURE.on_change
}

static void dslr_ccd_ccd_streaming_handler(indigo_device *device) {
	//+ dslr_ccd.CCD_STREAMING.on_change
	start_streaming(device); // streaming_finalizer owns completion
	//- dslr_ccd.CCD_STREAMING.on_change
}

static void dslr_ccd_ccd_abort_exposure_handler(indigo_device *device) {
	//+ dslr_ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, dslr_ccd_ccd_exposure_handler);
				indigo_cancel_pending_handler(device, dslr_ccd_ccd_streaming_handler);
				abort_acquisition(device); // cancel exposure_finalizer and streaming_finalizer
	//- dslr_ccd.CCD_ABORT_EXPOSURE.on_change
}

static void dslr_ccd_dslr_program_handler(indigo_device *device) {
	DSLR_PROGRAM_PROPERTY->state = INDIGO_OK_STATE;
	//+ dslr_ccd.DSLR_PROGRAM.on_change
	DSLR_SHUTTER_PROPERTY->hidden = B_ITEM->sw.value;
	indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
	indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
	//- dslr_ccd.DSLR_PROGRAM.on_change
	indigo_update_property(device, DSLR_PROGRAM_PROPERTY, NULL);
}

#pragma mark - Device API (dslr_ccd)

static indigo_result dslr_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result dslr_ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ dslr_ccd.on_attach
		PRIVATE_DATA->dslr = device;
		configure_ccd(device, 3);
		//- dslr_ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		DSLR_PROGRAM_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_PROGRAM_PROPERTY_NAME, "DSLR", "Program mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (DSLR_PROGRAM_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(M_ITEM, M_ITEM_NAME, "Manual", true);
		indigo_init_switch_item(B_ITEM, B_ITEM_NAME, "Bulb", false);
		DSLR_CAPTURE_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_CAPTURE_MODE_PROPERTY_NAME, "DSLR", "Drive mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (DSLR_CAPTURE_MODE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(S_ITEM, S_ITEM_NAME, "Single frame", true);
		DSLR_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_SHUTTER_PROPERTY_NAME, "DSLR", "Shutter time", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
		if (DSLR_SHUTTER_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(S_001_ITEM, S_001_ITEM_NAME, "1/100", true);
		indigo_init_switch_item(S_01_ITEM, S_01_ITEM_NAME, "1/10", false);
		indigo_init_switch_item(S_1_ITEM, S_1_ITEM_NAME, "1", false);
		indigo_init_switch_item(S_10_ITEM, S_10_ITEM_NAME, "10", false);
		indigo_init_switch_item(BULB_ITEM, BULB_ITEM_NAME, "Bulb", false);
		DSLR_APERTURE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_APERTURE_PROPERTY_NAME, "DSLR", "Aperture", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
		if (DSLR_APERTURE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(F14_ITEM, F14_ITEM_NAME, "f/1.4", true);
		indigo_init_switch_item(F20_ITEM, F20_ITEM_NAME, "f/2", false);
		indigo_init_switch_item(F28_ITEM, F28_ITEM_NAME, "f/2.8", false);
		indigo_init_switch_item(F40_ITEM, F40_ITEM_NAME, "f/4", false);
		indigo_init_switch_item(F56_ITEM, F56_ITEM_NAME, "f/5.6", false);
		DSLR_COMPRESSION_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_COMPRESSION_PROPERTY_NAME, "DSLR", "Compression", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		if (DSLR_COMPRESSION_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(JPEG_ITEM, JPEG_ITEM_NAME, "JPEG", true);
		DSLR_ISO_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_ISO_PROPERTY_NAME, "DSLR", "ISO", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
		if (DSLR_ISO_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(ISO100_ITEM, ISO100_ITEM_NAME, "100", true);
		indigo_init_switch_item(ISO200_ITEM, ISO200_ITEM_NAME, "200", false);
		indigo_init_switch_item(ISO400_ITEM, ISO400_ITEM_NAME, "400", false);
		DSLR_BATTERY_LEVEL_PROPERTY = indigo_init_number_property(NULL, device->name, DSLR_BATTERY_LEVEL_PROPERTY_NAME, "DSLR", "Battery level", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (DSLR_BATTERY_LEVEL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(VALUE_ITEM, VALUE_ITEM_NAME, "Value", 0, 100, 0, 50);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return dslr_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result dslr_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_PROGRAM_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_CAPTURE_MODE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_SHUTTER_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_APERTURE_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_COMPRESSION_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_ISO_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(DSLR_BATTERY_LEVEL_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result dslr_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(dslr_ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress, an exposure can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, dslr_ccd_ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress, streaming can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, dslr_ccd_ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, dslr_ccd_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DSLR_PROGRAM_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(DSLR_PROGRAM_PROPERTY, dslr_ccd_dslr_program_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DSLR_SHUTTER_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_SHUTTER_PROPERTY, property, false);
		DSLR_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DSLR_APERTURE_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_APERTURE_PROPERTY, property, false);
		DSLR_APERTURE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_APERTURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DSLR_ISO_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_ISO_PROPERTY, property, false);
		DSLR_ISO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_ISO_PROPERTY, NULL);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result dslr_ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		dslr_ccd_connection_handler(device);
	}
	indigo_release_property(DSLR_PROGRAM_PROPERTY);
	indigo_release_property(DSLR_CAPTURE_MODE_PROPERTY);
	indigo_release_property(DSLR_SHUTTER_PROPERTY);
	indigo_release_property(DSLR_APERTURE_PROPERTY);
	indigo_release_property(DSLR_COMPRESSION_PROPERTY);
	indigo_release_property(DSLR_ISO_PROPERTY);
	indigo_release_property(DSLR_BATTERY_LEVEL_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (file_ccd)
// device_id: file_ccd type: ccd

static void file_ccd_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		//+ file_ccd.on_connect
		connection_result = open_file_image(device);
		//- file_ccd.on_connect
		if (connection_result) {
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s", device->name);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ file_ccd.on_disconnect
		indigo_cancel_pending_handler(device, file_ccd_ccd_exposure_handler);
		indigo_cancel_pending_handler(device, file_ccd_ccd_streaming_handler);
		abort_acquisition(device);
		close_file_image(device);
		//- file_ccd.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void file_ccd_ccd_exposure_handler(indigo_device *device) {
	//+ file_ccd.CCD_EXPOSURE.on_change
	start_exposure(device); // exposure_finalizer owns completion
	//- file_ccd.CCD_EXPOSURE.on_change
}

static void file_ccd_ccd_streaming_handler(indigo_device *device) {
	//+ file_ccd.CCD_STREAMING.on_change
	start_streaming(device); // streaming_finalizer owns completion
	//- file_ccd.CCD_STREAMING.on_change
}

static void file_ccd_ccd_abort_exposure_handler(indigo_device *device) {
	//+ file_ccd.CCD_ABORT_EXPOSURE.on_change
	indigo_cancel_pending_handler(device, file_ccd_ccd_exposure_handler);
				indigo_cancel_pending_handler(device, file_ccd_ccd_streaming_handler);
				abort_acquisition(device); // cancel exposure_finalizer and streaming_finalizer
	//- file_ccd.CCD_ABORT_EXPOSURE.on_change
}

#pragma mark - Device API (file_ccd)

static indigo_result file_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result file_ccd_attach(indigo_device *device) {
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ file_ccd.on_attach
		PRIVATE_DATA->file = device;
		configure_ccd(device, 4);
		//- file_ccd.on_attach
		CCD_EXPOSURE_PROPERTY->hidden = false;
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_ABORT_EXPOSURE_PROPERTY->hidden = false;
		FILE_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, FILE_NAME_PROPERTY_NAME, MAIN_GROUP, "File name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (FILE_NAME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(PATH_ITEM, PATH_ITEM_NAME, "Path", "");
		BAYERPAT_PROPERTY = indigo_init_text_property(NULL, device->name, BAYERPAT_PROPERTY_NAME, MAIN_GROUP, "BAYERPAT header", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (BAYERPAT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(BAYERPAT_ITEM, BAYERPAT_ITEM_NAME, "BAYERPAT", "");
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return file_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result file_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(FILE_NAME_PROPERTY);
	INDIGO_DEFINE_MATCHING_PROPERTY(BAYERPAT_PROPERTY);
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result file_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(file_ccd_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_EXPOSURE_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE, CCD_EXPOSURE_PROPERTY, "Streaming in progress, an exposure can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_EXPOSURE_PROPERTY, file_ccd_ccd_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_STREAMING_PROPERTY, property)) {
		INDIGO_REJECT_CHANGE_IF(CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE, CCD_STREAMING_PROPERTY, "Exposure in progress, streaming can not be started");
		INDIGO_COPY_VALUES_PROCESS_CHANGE(CCD_STREAMING_PROPERTY, file_ccd_ccd_streaming_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(CCD_ABORT_EXPOSURE_PROPERTY, file_ccd_ccd_abort_exposure_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FILE_NAME_PROPERTY, property)) {
		indigo_property_copy_values(FILE_NAME_PROPERTY, property, false);
		FILE_NAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FILE_NAME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(BAYERPAT_PROPERTY, property)) {
		indigo_property_copy_values(BAYERPAT_PROPERTY, property, false);
		BAYERPAT_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, BAYERPAT_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FILE_NAME_PROPERTY);
			indigo_save_property(device, NULL, BAYERPAT_PROPERTY);
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result file_ccd_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		file_ccd_connection_handler(device);
	}
	indigo_release_property(FILE_NAME_PROPERTY);
	indigo_release_property(BAYERPAT_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

#pragma mark - High level code (wheel)

static void wheel_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ wheel.on_disconnect
		indigo_cancel_pending_handler(device, wheel_slot_handler);
		indigo_cancel_pending_handler(device, wheel_move_finalizer);
		//- wheel.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_slot_handler(indigo_device *device) {
	//+ wheel.WHEEL_SLOT.on_change
	if ((int)WHEEL_SLOT_ITEM->number.target == PRIVATE_DATA->current_slot) {
		WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
		INDIGO_UPDATE_PROPERTY_STATE(WHEEL_SLOT_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		indigo_execute_handler_in(device, 0.5, wheel_move_finalizer);
	}
	//- wheel.WHEEL_SLOT.on_change
}

#pragma mark - Device API (wheel)

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result wheel_attach(indigo_device *device) {
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ wheel.on_attach
		WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = FILTER_COUNT;
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = 1;
		//- wheel.on_attach
		WHEEL_SLOT_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_wheel_enumerate_properties(device, client, property);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(wheel_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(WHEEL_SLOT_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(WHEEL_SLOT_PROPERTY, wheel_slot_handler);
		return INDIGO_OK;
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

#pragma mark - High level code (focuser)

static void focuser_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ focuser.on_disconnect
		indigo_cancel_pending_handler(device, focuser_position_handler);
		indigo_cancel_pending_handler(device, focuser_steps_handler);
		indigo_cancel_pending_handler(device, focuser_move_finalizer);
		PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		//- focuser.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void focuser_position_handler(indigo_device *device) {
	//+ focuser.FOCUSER_POSITION.on_change
	if (FOCUSER_ON_POSITION_SET_SYNC_ITEM->sw.value) {
		PRIVATE_DATA->current_position = (int)FOCUSER_POSITION_ITEM->number.target;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_POSITION_PROPERTY, INDIGO_OK_STATE, NULL);
	} else {
		start_focuser_move(device, (int)FOCUSER_POSITION_ITEM->number.target); // focuser_move_finalizer owns completion
	}
	//- focuser.FOCUSER_POSITION.on_change
}

static void focuser_steps_handler(indigo_device *device) {
	//+ focuser.FOCUSER_STEPS.on_change
	int delta = (int)FOCUSER_STEPS_ITEM->number.value;
	start_focuser_move(device, PRIVATE_DATA->current_position + (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value ? -delta : delta)); // focuser_move_finalizer owns completion
	//- focuser.FOCUSER_STEPS.on_change
}

static void focuser_abort_motion_handler(indigo_device *device) {
	//+ focuser.FOCUSER_ABORT_MOTION.on_change
	indigo_cancel_pending_handler(device, focuser_position_handler);
	indigo_cancel_pending_handler(device, focuser_steps_handler);
	indigo_cancel_pending_handler(device, focuser_move_finalizer);
	PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
	FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
	FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
	INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_POSITION_PROPERTY, INDIGO_ALERT_STATE, NULL);
	indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
	INDIGO_UPDATE_PROPERTY_STATE(FOCUSER_ABORT_MOTION_PROPERTY, INDIGO_OK_STATE, NULL);
	//- focuser.FOCUSER_ABORT_MOTION.on_change
}

static void focuser_direction_handler(indigo_device *device) {
	FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
	//+ focuser.FOCUSER_DIRECTION.on_change
	if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
		PRIVATE_DATA->backlash_out = BACKLASH_ITEM->number.value > PRIVATE_DATA->backlash_in ? (int)BACKLASH_ITEM->number.value - PRIVATE_DATA->backlash_in : 0;
		PRIVATE_DATA->backlash_in = 0;
	} else {
		PRIVATE_DATA->backlash_in = BACKLASH_ITEM->number.value > PRIVATE_DATA->backlash_out ? (int)BACKLASH_ITEM->number.value - PRIVATE_DATA->backlash_out : 0;
		PRIVATE_DATA->backlash_out = 0;
	}
	//- focuser.FOCUSER_DIRECTION.on_change
	indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
}

#pragma mark - Device API (focuser)

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ focuser.on_attach
		FOCUSER_SPEED_ITEM->number.value = 1;
		FOCUSER_ON_POSITION_SET_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_ITEM->number.value = 25;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
		//- focuser.on_attach
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_STEPS_PROPERTY->hidden = false;
		FOCUSER_ABORT_MOTION_PROPERTY->hidden = false;
		FOCUSER_DIRECTION_PROPERTY->hidden = false;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
		FOCUSER_SETUP_PROPERTY = indigo_init_number_property(NULL, device->name, FOCUSER_SETUP_PROPERTY_NAME, MAIN_GROUP, "Focuser Setup", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (FOCUSER_SETUP_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(FOCUS_ITEM, FOCUS_ITEM_NAME, "Focus (steps)", 0, 1000000, 0, 0);
		indigo_init_number_item(BACKLASH_ITEM, BACKLASH_ITEM_NAME, "Backlash (steps)", 0, 1000, 0, 0);
		indigo_init_number_item(BLUR_SCALE_ITEM, BLUR_SCALE_ITEM_NAME, "Focuser blur (steps/px blur)", 1, 1000, 1, 15);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	INDIGO_DEFINE_MATCHING_PROPERTY(FOCUSER_SETUP_PROPERTY);
	return indigo_focuser_enumerate_properties(device, client, property);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(focuser_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_POSITION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_POSITION_PROPERTY, focuser_position_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_STEPS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_STEPS_PROPERTY, focuser_steps_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_URGENT_CHANGE(FOCUSER_ABORT_MOTION_PROPERTY, focuser_abort_motion_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_DIRECTION_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(FOCUSER_DIRECTION_PROPERTY, focuser_direction_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_COMPENSATION_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_BACKLASH_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
		FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(FOCUSER_SETUP_PROPERTY, property)) {
		indigo_property_copy_values(FOCUSER_SETUP_PROPERTY, property, false);
		FOCUSER_SETUP_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SETUP_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_SETUP_PROPERTY);
		}
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connection_handler(device);
	}
	indigo_release_property(FOCUSER_SETUP_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

#pragma mark - High level code (guider)

static void guider_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		//+ guider.on_disconnect
		indigo_cancel_pending_handler(device, guider_guide_ra_handler);
		indigo_cancel_pending_handler(device, guider_guide_dec_handler);
		indigo_cancel_pending_handler(device, guider_ra_finalizer);
		indigo_cancel_pending_handler(device, guider_dec_finalizer);
		//- guider.on_disconnect
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void guider_guide_ra_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_RA.on_change
	indigo_cancel_pending_handler(device, guider_ra_finalizer);
	double duration = GUIDER_GUIDE_EAST_ITEM->number.value > 0 ? GUIDER_GUIDE_EAST_ITEM->number.value : GUIDER_GUIDE_WEST_ITEM->number.value;
	GUIDER_GUIDE_RA_PROPERTY->state = duration > 0 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	if (duration > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_ra_finalizer);
	}
	//- guider.GUIDER_GUIDE_RA.on_change
}

static void guider_guide_dec_handler(indigo_device *device) {
	//+ guider.GUIDER_GUIDE_DEC.on_change
	indigo_cancel_pending_handler(device, guider_dec_finalizer);
	double duration = GUIDER_GUIDE_NORTH_ITEM->number.value > 0 ? GUIDER_GUIDE_NORTH_ITEM->number.value : GUIDER_GUIDE_SOUTH_ITEM->number.value;
	GUIDER_GUIDE_DEC_PROPERTY->state = duration > 0 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	if (duration > 0) {
		indigo_execute_priority_handler_in(device, INDIGO_TASK_PRIORITY_TIME, duration / 1000.0, guider_dec_finalizer);
	}
	//- guider.GUIDER_GUIDE_DEC.on_change
}

static void guider_rate_handler(indigo_device *device) {
	GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
	//+ guider.GUIDER_RATE.on_change
	PRIVATE_DATA->guide_rate = GUIDER_RATE_ITEM->number.value / 100.0;
	//- guider.GUIDER_RATE.on_change
	indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
}

#pragma mark - Device API (guider)

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result guider_attach(indigo_device *device) {
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		//+ guider.on_attach
		GUIDER_RATE_PROPERTY->hidden = false;
		GUIDER_RATE_ITEM->number.value = PRIVATE_DATA->guide_rate = 0.5;
		//- guider.on_attach
		GUIDER_GUIDE_RA_PROPERTY->hidden = false;
		GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
		GUIDER_RATE_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result guider_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_guider_enumerate_properties(device, client, property);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(guider_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_RA_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_RA.on_change_request
		GUIDER_GUIDE_EAST_ITEM->number.value = GUIDER_GUIDE_EAST_ITEM->number.target = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = GUIDER_GUIDE_WEST_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_RA.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_RA_PROPERTY, guider_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		//+ guider.GUIDER_GUIDE_DEC.on_change_request
		GUIDER_GUIDE_NORTH_ITEM->number.value = GUIDER_GUIDE_NORTH_ITEM->number.target = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = GUIDER_GUIDE_SOUTH_ITEM->number.target = 0;
		//- guider.GUIDER_GUIDE_DEC.on_change_request
		INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE_ANYTIME(GUIDER_GUIDE_DEC_PROPERTY, guider_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(GUIDER_RATE_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(GUIDER_RATE_PROPERTY, guider_rate_handler);
		return INDIGO_OK;
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

#pragma mark - High level code (ao)

static void ao_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_send_message(device, OK_PROPERTY, "Connected to %s", device->name);
	} else {
		indigo_cancel_pending_handlers(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ao_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void ao_guide_ra_handler(indigo_device *device) {
	AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
	//+ ao.AO_GUIDE_RA.on_change
	PRIVATE_DATA->ao_ra_offset += AO_GUIDE_EAST_ITEM->number.value - AO_GUIDE_WEST_ITEM->number.value;
	AO_GUIDE_EAST_ITEM->number.value = AO_GUIDE_WEST_ITEM->number.value = 0;
	if (fabs(PRIVATE_DATA->ao_ra_offset) > 100) {
		PRIVATE_DATA->ao_ra_offset = copysign(100, PRIVATE_DATA->ao_ra_offset);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ao.AO_GUIDE_RA.on_change
	indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
}

static void ao_guide_dec_handler(indigo_device *device) {
	AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
	//+ ao.AO_GUIDE_DEC.on_change
	PRIVATE_DATA->ao_dec_offset += AO_GUIDE_NORTH_ITEM->number.value - AO_GUIDE_SOUTH_ITEM->number.value;
	AO_GUIDE_NORTH_ITEM->number.value = AO_GUIDE_SOUTH_ITEM->number.value = 0;
	if (fabs(PRIVATE_DATA->ao_dec_offset) > 100) {
		PRIVATE_DATA->ao_dec_offset = copysign(100, PRIVATE_DATA->ao_dec_offset);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- ao.AO_GUIDE_DEC.on_change
	indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
}

static void ao_reset_handler(indigo_device *device) {
	AO_RESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ ao.AO_RESET.on_change
	if (AO_CENTER_ITEM->sw.value) {
		PRIVATE_DATA->ao_ra_offset = PRIVATE_DATA->ao_dec_offset = 0;
		AO_CENTER_ITEM->sw.value = false;
	}
	//- ao.AO_RESET.on_change
	indigo_update_property(device, AO_RESET_PROPERTY, NULL);
}

#pragma mark - Device API (ao)

static indigo_result ao_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ao_attach(indigo_device *device) {
	if (indigo_ao_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		AO_GUIDE_RA_PROPERTY->hidden = false;
		AO_GUIDE_DEC_PROPERTY->hidden = false;
		AO_RESET_PROPERTY->hidden = false;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ao_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ao_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	return indigo_ao_enumerate_properties(device, client, property);
}

static indigo_result ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(ao_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AO_GUIDE_RA_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AO_GUIDE_RA_PROPERTY, ao_guide_ra_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AO_GUIDE_DEC_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AO_GUIDE_DEC_PROPERTY, ao_guide_dec_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AO_RESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AO_RESET_PROPERTY, ao_reset_handler);
		return INDIGO_OK;
	}
	return indigo_ao_change_property(device, client, property);
}

static indigo_result ao_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ao_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ao_detach(device);
}

#pragma mark - Device templates

static indigo_device imager_ccd_template = INDIGO_DEVICE_INITIALIZER(IMAGER_CCD_DEVICE_NAME, imager_ccd_attach, imager_ccd_enumerate_properties, imager_ccd_change_property, NULL, imager_ccd_detach);

static indigo_device guider_ccd_template = INDIGO_DEVICE_INITIALIZER(GUIDER_CCD_DEVICE_NAME, guider_ccd_attach, guider_ccd_enumerate_properties, guider_ccd_change_property, NULL, guider_ccd_detach);

static indigo_device bahtinov_ccd_template = INDIGO_DEVICE_INITIALIZER(BAHTINOV_CCD_DEVICE_NAME, bahtinov_ccd_attach, bahtinov_ccd_enumerate_properties, bahtinov_ccd_change_property, NULL, bahtinov_ccd_detach);

static indigo_device dslr_ccd_template = INDIGO_DEVICE_INITIALIZER(DSLR_CCD_DEVICE_NAME, dslr_ccd_attach, dslr_ccd_enumerate_properties, dslr_ccd_change_property, NULL, dslr_ccd_detach);

static indigo_device file_ccd_template = INDIGO_DEVICE_INITIALIZER(FILE_CCD_DEVICE_NAME, file_ccd_attach, file_ccd_enumerate_properties, file_ccd_change_property, NULL, file_ccd_detach);

static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(WHEEL_DEVICE_NAME, wheel_attach, wheel_enumerate_properties, wheel_change_property, NULL, wheel_detach);

static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(FOCUSER_DEVICE_NAME, focuser_attach, focuser_enumerate_properties, focuser_change_property, NULL, focuser_detach);

static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(GUIDER_DEVICE_NAME, guider_attach, guider_enumerate_properties, guider_change_property, NULL, guider_detach);

static indigo_device ao_template = INDIGO_DEVICE_INITIALIZER(AO_DEVICE_NAME, ao_attach, ao_enumerate_properties, ao_change_property, NULL, ao_detach);

#pragma mark - Main code

indigo_result indigo_ccd_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static simulator_private_data *private_data = NULL;
	static indigo_device *imager_ccd = NULL;
	static indigo_device *guider_ccd = NULL;
	static indigo_device *bahtinov_ccd = NULL;
	static indigo_device *dslr_ccd = NULL;
	static indigo_device *file_ccd = NULL;
	static indigo_device *wheel = NULL;
	static indigo_device *focuser = NULL;
	static indigo_device *guider = NULL;
	static indigo_device *ao = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			private_data = (simulator_private_data *)indigo_safe_malloc(sizeof(simulator_private_data));
			imager_ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &imager_ccd_template);
			imager_ccd->private_data = private_data;
			indigo_attach_device(imager_ccd);
			guider_ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_ccd_template);
			guider_ccd->private_data = private_data;
			guider_ccd->master_device = imager_ccd;
			indigo_attach_device(guider_ccd);
			bahtinov_ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &bahtinov_ccd_template);
			bahtinov_ccd->private_data = private_data;
			bahtinov_ccd->master_device = imager_ccd;
			indigo_attach_device(bahtinov_ccd);
			dslr_ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &dslr_ccd_template);
			dslr_ccd->private_data = private_data;
			dslr_ccd->master_device = imager_ccd;
			indigo_attach_device(dslr_ccd);
			file_ccd = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &file_ccd_template);
			file_ccd->private_data = private_data;
			file_ccd->master_device = imager_ccd;
			indigo_attach_device(file_ccd);
			wheel = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
			wheel->private_data = private_data;
			wheel->master_device = imager_ccd;
			indigo_attach_device(wheel);
			focuser = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &focuser_template);
			focuser->private_data = private_data;
			focuser->master_device = imager_ccd;
			indigo_attach_device(focuser);
			guider = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider->private_data = private_data;
			guider->master_device = imager_ccd;
			indigo_attach_device(guider);
			ao = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &ao_template);
			ao->private_data = private_data;
			ao->master_device = imager_ccd;
			indigo_attach_device(ao);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(imager_ccd);
			VERIFY_NOT_CONNECTED(guider_ccd);
			VERIFY_NOT_CONNECTED(bahtinov_ccd);
			VERIFY_NOT_CONNECTED(dslr_ccd);
			VERIFY_NOT_CONNECTED(file_ccd);
			VERIFY_NOT_CONNECTED(wheel);
			VERIFY_NOT_CONNECTED(focuser);
			VERIFY_NOT_CONNECTED(guider);
			VERIFY_NOT_CONNECTED(ao);
			last_action = action;
			if (ao != NULL) {
				indigo_detach_device(ao);
				indigo_safe_free(ao);
				ao = NULL;
			}
			if (guider != NULL) {
				indigo_detach_device(guider);
				indigo_safe_free(guider);
				guider = NULL;
			}
			if (focuser != NULL) {
				indigo_detach_device(focuser);
				indigo_safe_free(focuser);
				focuser = NULL;
			}
			if (wheel != NULL) {
				indigo_detach_device(wheel);
				indigo_safe_free(wheel);
				wheel = NULL;
			}
			if (file_ccd != NULL) {
				indigo_detach_device(file_ccd);
				indigo_safe_free(file_ccd);
				file_ccd = NULL;
			}
			if (dslr_ccd != NULL) {
				indigo_detach_device(dslr_ccd);
				indigo_safe_free(dslr_ccd);
				dslr_ccd = NULL;
			}
			if (bahtinov_ccd != NULL) {
				indigo_detach_device(bahtinov_ccd);
				indigo_safe_free(bahtinov_ccd);
				bahtinov_ccd = NULL;
			}
			if (guider_ccd != NULL) {
				indigo_detach_device(guider_ccd);
				indigo_safe_free(guider_ccd);
				guider_ccd = NULL;
			}
			if (imager_ccd != NULL) {
				indigo_detach_device(imager_ccd);
				indigo_safe_free(imager_ccd);
				imager_ccd = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
