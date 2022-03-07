// Copyright (c) 2016 CloudMakers, s. r. o.
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

/** INDIGO CCD Simulator driver
 \file indigo_ccd_simulator.c
 */

#define DRIVER_VERSION 0x0013
#define DRIVER_NAME	"indigo_ccd_simulator"
//#define ENABLE_BACKLASH_PROPERTY

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_cat_data.h>
#include <indigo/indigo_novas.h>
#include <indigo/indigo_align.h>

#include "indigo_ccd_simulator.h"

// related to embedded image size, don't touch!
#define IMAGER_WIDTH        		1600
#define IMAGER_HEIGHT       		1200
#define DSLR_WIDTH        			1600
#define DSLR_HEIGHT       			1200
#define GUIDER_WIDTH        		16000
#define GUIDER_HEIGHT       		12000

// can be changed
#define GUIDER_MAX_MAG					8
#define GUIDER_MAX_STARS				400
#define GUIDER_FOV							7
#define GUIDER_MAX_HOTPIXELS		1500

#define ECLIPSE									360
#define TEMP_UPDATE         		5.0

// gp_bits is used as boolean
#define is_connected                gp_bits

#define PRIVATE_DATA								((simulator_private_data *)device->private_data)
#define DSLR_PROGRAM_PROPERTY				PRIVATE_DATA->dslr_program_property
#define DSLR_CAPTURE_MODE_PROPERTY	PRIVATE_DATA->dslr_capture_mode_property
#define DSLR_APERTURE_PROPERTY			PRIVATE_DATA->dslr_aperture_property
#define DSLR_SHUTTER_PROPERTY				PRIVATE_DATA->dslr_shutter_property
#define DSLR_COMPRESSION_PROPERTY		PRIVATE_DATA->dslr_compression_property
#define DSLR_ISO_PROPERTY						PRIVATE_DATA->dslr_iso_property
#define DSLR_BATTERY_LEVEL_PROPERTY	PRIVATE_DATA->dslr_battery_level_property

#define GUIDER_MODE_PROPERTY				PRIVATE_DATA->guider_mode_property
#define GUIDER_MODE_STARS_ITEM			(GUIDER_MODE_PROPERTY->items + 0)
#define GUIDER_MODE_FLIP_STARS_ITEM	(GUIDER_MODE_PROPERTY->items + 1)
#define GUIDER_MODE_SUN_ITEM				(GUIDER_MODE_PROPERTY->items + 2)
#define GUIDER_MODE_ECLIPSE_ITEM		(GUIDER_MODE_PROPERTY->items + 3)

#define GUIDER_SETTINGS_PROPERTY		PRIVATE_DATA->guider_settings_property
#define GUIDER_IMAGE_WIDTH_ITEM			(GUIDER_SETTINGS_PROPERTY->items + 0)
#define GUIDER_IMAGE_HEIGHT_ITEM		(GUIDER_SETTINGS_PROPERTY->items + 1)
#define GUIDER_IMAGE_NOISE_FIX_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 2)
#define GUIDER_IMAGE_NOISE_VAR_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 3)
#define GUIDER_IMAGE_PERR_SPD_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 4)
#define GUIDER_IMAGE_PERR_VAL_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 5)
#define GUIDER_IMAGE_GRADIENT_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 6)
#define GUIDER_IMAGE_ANGLE_ITEM			(GUIDER_SETTINGS_PROPERTY->items + 7)
#define GUIDER_IMAGE_AO_ANGLE_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 8)
#define GUIDER_IMAGE_HOTPIXELS_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 9)
#define GUIDER_IMAGE_HOTCOL_ITEM		(GUIDER_SETTINGS_PROPERTY->items + 10)
#define GUIDER_IMAGE_HOTROW_ITEM		(GUIDER_SETTINGS_PROPERTY->items + 11)
#define GUIDER_IMAGE_RA_OFFSET_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 12)
#define GUIDER_IMAGE_DEC_OFFSET_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 13)
#define GUIDER_IMAGE_LAT_ITEM				(GUIDER_SETTINGS_PROPERTY->items + 14)
#define GUIDER_IMAGE_LONG_ITEM			(GUIDER_SETTINGS_PROPERTY->items + 15)
#define GUIDER_IMAGE_RA_ITEM				(GUIDER_SETTINGS_PROPERTY->items + 16)
#define GUIDER_IMAGE_DEC_ITEM				(GUIDER_SETTINGS_PROPERTY->items + 17)
#define GUIDER_IMAGE_EPOCH_ITEM			(GUIDER_SETTINGS_PROPERTY->items + 18)
#define GUIDER_IMAGE_ALT_ERROR_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 19)
#define GUIDER_IMAGE_AZ_ERROR_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 20)
#define GUIDER_IMAGE_IMAGE_AGE_ITEM	(GUIDER_SETTINGS_PROPERTY->items + 21)

#define FILE_NAME_PROPERTY					PRIVATE_DATA->file_name_property
#define FILE_NAME_ITEM							(FILE_NAME_PROPERTY->items + 0)

#define FOCUSER_SETTINGS_PROPERTY		PRIVATE_DATA->focuser_settings_property
#define FOCUSER_SETTINGS_FOCUS_ITEM	(FOCUSER_SETTINGS_PROPERTY->items + 0)
#define FOCUSER_SETTINGS_BL_ITEM		(FOCUSER_SETTINGS_PROPERTY->items + 1)


extern unsigned short indigo_ccd_simulator_raw_image[];
extern unsigned char indigo_ccd_simulator_rgb_image[];
extern struct _cat { float ra, dec; unsigned char mag; } indigo_ccd_simulator_cat[];
extern int indigo_ccd_simulator_cat_size;

typedef struct {
	indigo_device *imager, *guider, *dslr, *file;
	indigo_property *dslr_program_property;
	indigo_property *dslr_capture_mode_property;
	indigo_property *dslr_aperture_property;
	indigo_property *dslr_shutter_property;
	indigo_property *dslr_compression_property;
	indigo_property *dslr_iso_property;
	indigo_property *dslr_battery_level_property;
	indigo_property *guider_mode_property;
	indigo_property *guider_settings_property;
	indigo_property *file_name_property;
	indigo_property *focuser_settings_property;
	double ra, dec;
	double lat, lon;
	double ew_error, ns_error;
	double lst;
	int star_count, star_x[GUIDER_MAX_STARS], star_y[GUIDER_MAX_STARS], star_a[GUIDER_MAX_STARS], hotpixel_x[GUIDER_MAX_HOTPIXELS + 1], hotpixel_y[GUIDER_MAX_HOTPIXELS + 1];
	char imager_image[FITS_HEADER_SIZE + 3 * IMAGER_WIDTH * IMAGER_HEIGHT + 2880];
	char guider_image[FITS_HEADER_SIZE + 3 * GUIDER_WIDTH * GUIDER_HEIGHT + 2880];
	char dslr_image[FITS_HEADER_SIZE + 3 * DSLR_WIDTH * DSLR_HEIGHT + 2880];
	char *file_image, *raw_file_image;
	indigo_raw_header file_image_header;
	pthread_mutex_t image_mutex;
	double target_temperature, current_temperature;
	int current_slot;
	int target_position, current_position, backlash_in, backlash_out;
	indigo_timer *imager_exposure_timer, *guider_exposure_timer, *dslr_exposure_timer, *file_exposure_timer, *temperature_timer, *guider_timer;
	double ao_ra_offset, ao_dec_offset;
	int eclipse;
	double guide_rate;
} simulator_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static int mags[] = { 760000, 305000, 122000, 49000, 20000, 7800, 3100, 1200, 500 };

static void search_stars(indigo_device *device) {
	double lst = indigo_lst(NULL, GUIDER_IMAGE_LONG_ITEM->number.target);
	if (lst - PRIVATE_DATA->lst >= GUIDER_IMAGE_IMAGE_AGE_ITEM->number.value || PRIVATE_DATA->ra != GUIDER_IMAGE_RA_ITEM->number.value || PRIVATE_DATA->dec != GUIDER_IMAGE_DEC_ITEM->number.value || PRIVATE_DATA->lat != GUIDER_IMAGE_LAT_ITEM->number.value || PRIVATE_DATA->lon != GUIDER_IMAGE_LONG_ITEM->number.value || PRIVATE_DATA->ew_error != GUIDER_IMAGE_ALT_ERROR_ITEM->number.value || PRIVATE_DATA->ns_error != GUIDER_IMAGE_AZ_ERROR_ITEM->number.value) {
		double h2r = M_PI / 12;
		double d2r = M_PI / 180;
		double mount_ra = GUIDER_IMAGE_RA_ITEM->number.value; // where mount thinks it is pointing
		double mount_dec = GUIDER_IMAGE_DEC_ITEM->number.value;
		indigo_spherical_point_t point;
		indigo_ra_dec_to_point(mount_ra, mount_dec, lst, &point);
		indigo_spherical_point_t point_r = indigo_apply_polar_error(&point, GUIDER_IMAGE_ALT_ERROR_ITEM->number.target * DEG2RAD, GUIDER_IMAGE_AZ_ERROR_ITEM->number.target * DEG2RAD);
		indigo_point_to_ra_dec(&point_r, lst, &mount_ra, &mount_dec);
		mount_ra *= h2r;
		mount_dec *= d2r;
		double cos_mount_dec = cos(mount_dec);
		double sin_mount_dec = sin(mount_dec);
		double angle = M_PI * GUIDER_IMAGE_ANGLE_ITEM->number.target / 180.0; // image rotation
		double ppr = GUIDER_IMAGE_HEIGHT_ITEM->number.target / GUIDER_FOV / d2r; // pixel/radian ratio
		double radius = GUIDER_FOV * d2r * 2;
		double ppr_cos = ppr * cos(angle);
		double ppr_sin = ppr * sin(angle);
		PRIVATE_DATA->star_count = 0;
		for (indigo_star_entry *star_data = indigo_get_star_data(); star_data->hip; star_data++) {
			if (star_data->mag > GUIDER_MAX_MAG)
				continue;
			double ra = (GUIDER_IMAGE_EPOCH_ITEM->number.target != 0 ? star_data->ra : star_data->ra_now) * h2r;
			double dec = (GUIDER_IMAGE_EPOCH_ITEM->number.target != 0 ? star_data->dec : star_data->dec_now) * d2r;
			double cos_dec = cos(dec);
			double sin_dec = sin(dec);
			double sin_dec_dec = sin_mount_dec * sin_dec;
			double cos_dec_dec = cos_mount_dec * cos_dec;
			double cos_ra_ra = cos(ra - mount_ra);
			double distance = acos(sin_dec_dec + cos_dec_dec * cos_ra_ra);
			if (distance > radius)
				continue;
			double sin_ra_ra = sin(ra - mount_ra);
			double ccc_ss = cos_dec_dec * cos_ra_ra + sin_dec_dec;
			double sx = cos_dec * sin_ra_ra / ccc_ss;
			double sy = (sin_mount_dec * cos_dec * cos_ra_ra - cos_mount_dec * sin_dec) / ccc_ss;
			double x = ppr_cos * sx + ppr_sin * sy + GUIDER_IMAGE_WIDTH_ITEM->number.target / 2;
			double y = ppr_cos * sy - ppr_sin * sx + GUIDER_IMAGE_HEIGHT_ITEM->number.target / 2;
			if (x >= 0 && x < GUIDER_IMAGE_WIDTH_ITEM->number.target && y >= 0 && y < GUIDER_IMAGE_HEIGHT_ITEM->number.target) {
				//printf("HIP%5d %6.4f %+7.4f %6.1f %6.1f\n", star_data->hip, star_data->ra, star_data->dec, x, y);
				PRIVATE_DATA->star_x[PRIVATE_DATA->star_count] = x;
				PRIVATE_DATA->star_y[PRIVATE_DATA->star_count] = y;
				PRIVATE_DATA->star_a[PRIVATE_DATA->star_count] = mags[(int)star_data->mag];
				if (PRIVATE_DATA->star_count++ == GUIDER_MAX_STARS)
					break;
			} else {
				continue;
			}
		}
		PRIVATE_DATA->ra = GUIDER_IMAGE_RA_ITEM->number.target;
		PRIVATE_DATA->dec = GUIDER_IMAGE_DEC_ITEM->number.target;
		PRIVATE_DATA->lat = GUIDER_IMAGE_LAT_ITEM->number.target;
		PRIVATE_DATA->lon = GUIDER_IMAGE_LONG_ITEM->number.target;
		PRIVATE_DATA->ew_error = GUIDER_IMAGE_ALT_ERROR_ITEM->number.target;
		PRIVATE_DATA->ns_error = GUIDER_IMAGE_AZ_ERROR_ITEM->number.target;
		PRIVATE_DATA->lst = lst;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d stars, center at %g/%g, seen from %g/%g with polar error %g/%g", PRIVATE_DATA->star_count, PRIVATE_DATA->ra, PRIVATE_DATA->dec, PRIVATE_DATA->lat, PRIVATE_DATA->lon, GUIDER_IMAGE_ALT_ERROR_ITEM->number.target, GUIDER_IMAGE_AZ_ERROR_ITEM->number.target);
	}
}

// gausian blur algorithm is based on the paper http://blog.ivank.net/fastest-gaussian-blur.html by Ivan Kuckir

static void box_blur_h(uint16_t *scl, uint16_t *tcl, int w, int h, double r) {
	double iarr = 1 / (r + r + 1);
	for (int i = 0; i < h; i++) {
		int ti = i * w, li = ti, ri = ti + r;
		int fv = scl[ti], lv = scl[ti + w - 1], val = (r + 1) * fv;
		for (int j = 0; j < r; j++)
			val += scl[ti + j];
		for (int j = 0  ; j <= r ; j++) {
			val += scl[ri++] - fv;
			tcl[ti++] = round(val * iarr);
		}
		for (int j = r + 1; j < w-r; j++) {
			val += scl[ri++] - scl[li++];
			tcl[ti++] = round(val * iarr);
		}
		for (int j = w - r; j < w  ; j++) {
			val += lv - scl[li++];
			tcl[ti++] = round(val * iarr);
		}
	}
}

static void box_blur_t(uint16_t *scl, uint16_t *tcl, int w, int h, double r) {
	double iarr = 1 / ( r + r + 1);
	for (int i = 0; i < w; i++) {
		int ti = i, li = ti, ri = ti + r * w;
		int fv = scl[ti], lv = scl[ti + w * (h - 1)], val = (r + 1) * fv;
		for (int j = 0; j < r; j++)
			val += scl[ti + j * w];
		for (int j = 0  ; j <= r ; j++) {
			val += scl[ri] - fv;
			tcl[ti] = round(val * iarr);
			ri += w;
			ti += w;
		}
		for (int j = r + 1; j<h-r; j++) {
			val += scl[ri] - scl[li];
			tcl[ti] = round(val*iarr);
			li += w;
			ri += w;
			ti += w;
		}
		for (int j = h - r; j < h  ; j++) {
			val += lv - scl[li];
			tcl[ti] = round(val * iarr);
			li += w;
			ti += w;
		}
	}
}

static void box_blur(uint16_t *scl, uint16_t *tcl, int w, int h, double r) {
	int length = w * h;
	for (int i = 0; i < length; i++)
		tcl[i] = scl[i];
	box_blur_h(tcl, scl, w, h, r);
	box_blur_t(scl, tcl, w, h, r);
}

static void gauss_blur(uint16_t *scl, uint16_t *tcl, int w, int h, double r) {
	double ideal = sqrt((12 * r * r / 3) + 1);
	int wl = floor(ideal);
	if (wl % 2 == 0)
		wl--;
	int wu = wl + 2;
	ideal = (12 * r * r - 3 * wl * wl - 12 * wl - 9)/(-4 * wl - 4);
	int m = round(ideal);
	int sizes[3];
	for (int i = 0; i < 3; i++)
		sizes[i] = i < m ? wl : wu;
	box_blur(scl, tcl, w, h, (sizes[0] - 1) / 2);
	box_blur(tcl, scl, w, h, (sizes[1] - 1) / 2);
	box_blur(scl, tcl, w, h, (sizes[2] - 1) / 2);
}

static void create_frame(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->image_mutex);
	simulator_private_data *private_data = PRIVATE_DATA;
	if (device == PRIVATE_DATA->dslr) {
		unsigned char *raw = (unsigned char *)(PRIVATE_DATA->dslr_image + FITS_HEADER_SIZE);
		int size = DSLR_WIDTH * DSLR_HEIGHT * 3;
		for (int i = 0; i < size; i++) {
			int rgb = indigo_ccd_simulator_rgb_image[i];
			if (rgb < 0xF0)
				raw[i] = rgb  + (rand() & 0x0F);
			else
				raw[i] = rgb;
		}
		void *data_out;
		unsigned long size_out;
		indigo_raw_to_jpeg(device, private_data->dslr_image, DSLR_WIDTH, DSLR_HEIGHT, 24, true, true, &data_out, &size_out, NULL, NULL);
		if (CCD_PREVIEW_ENABLED_ITEM->sw.value)
			indigo_process_dslr_preview_image(device, data_out, (int)size_out);
		indigo_process_dslr_image(device, data_out, (int)size_out, ".jpeg", CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
		free(data_out);
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
#if 1 // move image
		static int frame_counter = 0;
		static int x_offset = 0;
		static int y_offset = 0;
		if (frame_counter++ % 2)
			x_offset = (x_offset + 1) % 10;
		else
			y_offset = (y_offset + 1) % 10;
		int offset = (y_offset * PRIVATE_DATA->file_image_header.width + x_offset) * bpp / 8;
		memcpy(PRIVATE_DATA->file_image, PRIVATE_DATA->raw_file_image, FITS_HEADER_SIZE);
		memcpy(PRIVATE_DATA->file_image + FITS_HEADER_SIZE, PRIVATE_DATA->raw_file_image + FITS_HEADER_SIZE + offset, size - offset);
		if (offset)
			memcpy(PRIVATE_DATA->file_image + FITS_HEADER_SIZE + size - offset, PRIVATE_DATA->raw_file_image + FITS_HEADER_SIZE, offset);
#else
		memcpy(PRIVATE_DATA->file_image, PRIVATE_DATA->raw_file_image, size + FITS_HEADER_SIZE);
#endif
		indigo_process_image(device, PRIVATE_DATA->file_image, PRIVATE_DATA->file_image_header.width, PRIVATE_DATA->file_image_header.height, bpp, true, true, NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
	} else {
		uint16_t *raw = (uint16_t *)((device == PRIVATE_DATA->guider ? private_data->guider_image : private_data->imager_image) + FITS_HEADER_SIZE);
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
		} else if (device == PRIVATE_DATA->guider) {
			for (int j = 0; j < frame_height; j++) {
				int jj = j * j;
				for (int i = 0; i < frame_width; i++) {
					raw[j * frame_width + i] = GUIDER_IMAGE_GRADIENT_ITEM->number.target * sqrt(i * i + jj);
				}
			}
		} else {
			for (int i = 0; i < size; i++)
				raw[i] = (rand() & 0x7F);
		}
		if (device == PRIVATE_DATA->guider && light_frame) {
			static time_t start_time = 0;
			if (start_time == 0)
				start_time = time(NULL);
			search_stars(device);
			double ra_offset = GUIDER_IMAGE_PERR_VAL_ITEM->number.target * sin(GUIDER_IMAGE_PERR_SPD_ITEM->number.target * M_PI * ((time(NULL) - start_time) % 360) / 180) + GUIDER_IMAGE_RA_OFFSET_ITEM->number.value;
			double guider_sin = sin(M_PI * GUIDER_IMAGE_ANGLE_ITEM->number.target / 180.0);
			double guider_cos = cos(M_PI * GUIDER_IMAGE_ANGLE_ITEM->number.target / 180.0);
			double ao_sin = sin(M_PI * GUIDER_IMAGE_AO_ANGLE_ITEM->number.target / 180.0);
			double ao_cos = cos(M_PI * GUIDER_IMAGE_AO_ANGLE_ITEM->number.target / 180.0);
			double x_offset = ra_offset * guider_cos - GUIDER_IMAGE_DEC_OFFSET_ITEM->number.value * guider_sin + PRIVATE_DATA->ao_ra_offset * ao_cos - PRIVATE_DATA->ao_dec_offset * ao_sin + rand() / (double)RAND_MAX/10 - 0.1;
			double y_offset = ra_offset * guider_sin + GUIDER_IMAGE_DEC_OFFSET_ITEM->number.value * guider_cos + PRIVATE_DATA->ao_ra_offset * ao_sin + PRIVATE_DATA->ao_dec_offset * ao_cos + rand() / (double)RAND_MAX/10 - 0.1;
			bool y_flip = GUIDER_MODE_FLIP_STARS_ITEM->sw.value;
			if (GUIDER_MODE_STARS_ITEM->sw.value || y_flip) {
				for (int i = 0; i < PRIVATE_DATA->star_count; i++) {
					double center_x = (private_data->star_x[i] + x_offset) / horizontal_bin;
					if (center_x < 0)
						center_x += GUIDER_IMAGE_WIDTH_ITEM->number.target;
					if (center_x >= GUIDER_IMAGE_WIDTH_ITEM->number.target)
						center_x -= GUIDER_IMAGE_WIDTH_ITEM->number.target;
					double center_y = (private_data->star_y[i] + (y_flip ? -y_offset : y_offset)) / vertical_bin;
					if (center_y < 0)
						center_y += GUIDER_IMAGE_HEIGHT_ITEM->number.target;
					if (center_y >= GUIDER_IMAGE_HEIGHT_ITEM->number.target)
						center_y -= GUIDER_IMAGE_HEIGHT_ITEM->number.target;
					center_x -= frame_left;
					center_y -= frame_top;
					int a = private_data->star_a[i];
					int xMax = (int)round(center_x) + 8 / horizontal_bin;
					int yMax = (int)round(center_y) + 8 / vertical_bin;
					for (int y = yMax - 16 / vertical_bin; y <= yMax; y++) {
						if (y < 0 || y >= frame_height)
							continue;
						int yw = y * frame_width;
						double yy = center_y - y;
						for (int x = xMax - 16 / horizontal_bin; x <= xMax; x++) {
							if (x < 0 || x >= frame_width)
								continue;
							double xx = center_x - x;
							double v = a * exp(-(xx * xx / 4 + yy * yy / 4));
							raw[yw + x] += (unsigned short)v;
						}
					}
				}
			} else {
				double center_x = (GUIDER_IMAGE_WIDTH_ITEM->number.target / 2 + x_offset) / horizontal_bin - frame_left;
				double center_y = (GUIDER_IMAGE_HEIGHT_ITEM->number.target / 2 + y_offset) / vertical_bin - frame_top;
				double eclipse_x = (GUIDER_IMAGE_WIDTH_ITEM->number.target / 2 + PRIVATE_DATA->eclipse + x_offset) / horizontal_bin - frame_left;
				double eclipse_y = (GUIDER_IMAGE_HEIGHT_ITEM->number.target / 2 + PRIVATE_DATA->eclipse + y_offset) / vertical_bin - frame_top;
				for (int y = 0; y <= GUIDER_IMAGE_HEIGHT_ITEM->number.target / vertical_bin; y++) {
					if (y < 0 || y >= frame_height)
						continue;
					int yw = y * frame_width;
					double yy = (center_y - y) * vertical_bin;
					double eclipse_yy = (eclipse_y - y) * vertical_bin;
					for (int x = 0; x <= GUIDER_IMAGE_WIDTH_ITEM->number.target / horizontal_bin; x++) {
						if (x < 0 || x >= frame_width)
							continue;
						double xx = (center_x - x) * horizontal_bin;
						double eclipse_xx = (eclipse_x - x) * horizontal_bin;
						double value = 500000 * exp(-((xx * xx + yy * yy) / 20000.0));
						if (GUIDER_MODE_ECLIPSE_ITEM->sw.value && eclipse_xx*eclipse_xx+eclipse_yy*eclipse_yy < 50000)
							value = 0;
						if (value < 65535)
							raw[yw + x] += (unsigned short)value;
						else
							raw[yw + x] = 65535;
					}
				}
				if (GUIDER_MODE_ECLIPSE_ITEM->sw.value) {
					PRIVATE_DATA->eclipse++;
					if (PRIVATE_DATA->eclipse > ECLIPSE)
						PRIVATE_DATA->eclipse = -ECLIPSE;
				}
			}
		}
		for (int i = 0; i < size; i++) {
			double value = raw[i] - offset;
			if (value < 0)
				value = 0;
			value = gain * pow(value, gamma);
			if (value > 65535)
				value = 65535;
			raw[i] = (unsigned short)value;
		}
		if (FOCUSER_SETTINGS_FOCUS_ITEM->number.value != 0) {
			uint16_t *tmp = indigo_safe_malloc(2 * size);
			gauss_blur(raw, tmp, frame_width, frame_height, FOCUSER_SETTINGS_FOCUS_ITEM->number.value);
			memcpy(raw, tmp, 2 * size);
			free(tmp);
		}

		int value;
		if (device == PRIVATE_DATA->imager && light_frame) {
			for (int i = 0; i < size; i++) {
				value = raw[i] + (rand() & 0x7F);
				raw[i] = (value > 65535) ? 65535 : value;
			}
		} else if (device == PRIVATE_DATA->guider) {
			for (int i = 0; i < size; i++) {
				value = raw[i] + (rand() % (int)GUIDER_IMAGE_NOISE_VAR_ITEM->number.target) + GUIDER_IMAGE_NOISE_FIX_ITEM->number.target;
				raw[i] = (value > 65535) ? 65535 : value;
			}
		} else {
			for (int i = 0; i < size; i++)
				raw[i] = (rand() & 0x7F);
		}

		for (int i = 0; i <= GUIDER_IMAGE_HOTPIXELS_ITEM->number.target; i++) {
			unsigned x = private_data->hotpixel_x[i] / horizontal_bin - frame_left;
			unsigned y = private_data->hotpixel_y[i] / vertical_bin - frame_top;
			if (x < 0 || x >= frame_width || y < 0 || y > frame_height)
				continue;
			if (i) {
				raw[y * frame_width + x] = 0xFFFF;
			} else {
				int col_length = fmin(frame_height, GUIDER_IMAGE_HOTCOL_ITEM->number.target);
				int row_length = fmin(frame_width, GUIDER_IMAGE_HOTROW_ITEM->number.target);
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
			indigo_process_image(device, device == PRIVATE_DATA->guider ? private_data->guider_image : private_data->imager_image, frame_width, frame_height, bpp, true, true, NULL, CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE);
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->image_mutex);
}

static void exposure_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		create_frame(device);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	}
}

static void streaming_timer_callback(indigo_device *device) {
	while (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		indigo_usleep(CCD_STREAMING_EXPOSURE_ITEM->number.target * ONE_SECOND_DELAY);
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE && CCD_STREAMING_COUNT_ITEM->number.value != 0) {
			create_frame(device);
			if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
				CCD_STREAMING_COUNT_ITEM->number.value--;
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		}
	}
	indigo_finalize_video_stream(device);
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
		CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
}

static void ccd_temperature_callback(indigo_device *device) {
	if (CCD_COOLER_ON_ITEM->sw.value) {
		double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
		if (diff > 0) {
			if (diff > 5) {
				if (CCD_COOLER_POWER_ITEM->number.value != 100) {
					CCD_COOLER_POWER_ITEM->number.value = 100;
					indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
				}
			} else {
				if (CCD_COOLER_POWER_ITEM->number.value != 50) {
					CCD_COOLER_POWER_ITEM->number.value = 50;
					indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
				}
			}
			CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = --(PRIVATE_DATA->current_temperature);
		} else if (diff < 0) {
			if (CCD_COOLER_POWER_ITEM->number.value > 0) {
				CCD_COOLER_POWER_ITEM->number.value = 0;
				indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
			}
			CCD_TEMPERATURE_PROPERTY->state = CCD_COOLER_ON_ITEM->sw.value ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			CCD_TEMPERATURE_ITEM->number.value = ++(PRIVATE_DATA->current_temperature);
		} else {
			if (CCD_COOLER_ON_ITEM->sw.value) {
				if (CCD_COOLER_POWER_ITEM->number.value != 20) {
					CCD_COOLER_POWER_ITEM->number.value = 20;
					indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
				}
			} else {
				if (CCD_COOLER_POWER_ITEM->number.value != 0) {
					CCD_COOLER_POWER_ITEM->number.value = 0;
					indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
				}
			}
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else {
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	indigo_reschedule_timer(device, TEMP_UPDATE, &PRIVATE_DATA->temperature_timer);
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = false;
		SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
		SIMULATION_ENABLED_ITEM->sw.value = true;
		SIMULATION_DISABLED_ITEM->sw.value = false;
		if (device == PRIVATE_DATA->dslr) {
			DSLR_PROGRAM_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_PROGRAM_PROPERTY_NAME, "DSLR", "Program mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
			indigo_init_switch_item(DSLR_PROGRAM_PROPERTY->items + 0, "M", "Manual", true);
			indigo_init_switch_item(DSLR_PROGRAM_PROPERTY->items + 1, "B", "Bulb", false);
			DSLR_CAPTURE_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_CAPTURE_MODE_PROPERTY_NAME, "DSLR", "Drive mode", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			indigo_init_switch_item(DSLR_CAPTURE_MODE_PROPERTY->items + 0, "S", "Single frame", true);
			DSLR_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_SHUTTER_PROPERTY_NAME, "DSLR", "Shutter time", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
			indigo_init_switch_item(DSLR_SHUTTER_PROPERTY->items + 0, "0.01", "1/100", true);
			indigo_init_switch_item(DSLR_SHUTTER_PROPERTY->items + 1, "0.1", "1/10", false);
			indigo_init_switch_item(DSLR_SHUTTER_PROPERTY->items + 2, "1", "1", false);
			indigo_init_switch_item(DSLR_SHUTTER_PROPERTY->items + 3, "10", "10", false);
			indigo_init_switch_item(DSLR_SHUTTER_PROPERTY->items + 4, "BULB", "Bulb", false);
			DSLR_APERTURE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_APERTURE_PROPERTY_NAME, "DSLR", "Aperture", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 5);
			indigo_init_switch_item(DSLR_APERTURE_PROPERTY->items + 0, "14", "f/1.4", true);
			indigo_init_switch_item(DSLR_APERTURE_PROPERTY->items + 1, "20", "f/2", false);
			indigo_init_switch_item(DSLR_APERTURE_PROPERTY->items + 2, "28", "f/2.8", false);
			indigo_init_switch_item(DSLR_APERTURE_PROPERTY->items + 3, "40", "f/4", false);
			indigo_init_switch_item(DSLR_APERTURE_PROPERTY->items + 4, "56", "f/5.6", false);
			CCD_MODE_PROPERTY->perm = INDIGO_RO_PERM;
			CCD_MODE_PROPERTY->count = 1;
			indigo_init_switch_item(CCD_MODE_PROPERTY->items + 0, "1600x1200", "1600x1200", true);
			DSLR_COMPRESSION_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_COMPRESSION_PROPERTY_NAME, "DSLR", "Compression", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
			indigo_init_switch_item(DSLR_COMPRESSION_PROPERTY->items + 0, "JPEG", "JPEG", true);
			DSLR_ISO_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_ISO_PROPERTY_NAME, "DSLR", "ISO", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
			indigo_init_switch_item(DSLR_ISO_PROPERTY->items + 0, "100", "100", true);
			indigo_init_switch_item(DSLR_ISO_PROPERTY->items + 1, "200", "200", false);
			indigo_init_switch_item(DSLR_ISO_PROPERTY->items + 2, "400", "400", false);
			DSLR_BATTERY_LEVEL_PROPERTY = indigo_init_number_property(NULL, device->name, DSLR_BATTERY_LEVEL_PROPERTY_NAME, "DSLR", "Battery level", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
			indigo_init_number_item(DSLR_BATTERY_LEVEL_PROPERTY->items + 0, "VALUE", "Value", 0, 100, 0, 50);
			CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = DSLR_WIDTH;
			CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = DSLR_HEIGHT;
			CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.max = 1;
			CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.max = 1;
			CCD_IMAGE_FORMAT_PROPERTY = indigo_resize_property(CCD_IMAGE_FORMAT_PROPERTY, 3);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_NATIVE_ITEM, CCD_IMAGE_FORMAT_NATIVE_ITEM_NAME, "Native", true);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM, CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM_NAME, "Native + AVI", false);
			indigo_init_switch_item(CCD_IMAGE_FORMAT_RAW_ITEM, CCD_IMAGE_FORMAT_RAW_ITEM_NAME, "RAW", false);
			CCD_JPEG_SETTINGS_PROPERTY->hidden = true;
			CCD_OFFSET_PROPERTY->hidden = true;
			CCD_GAMMA_PROPERTY->hidden = true;
			CCD_GAIN_PROPERTY->hidden = true;
			CCD_FRAME_PROPERTY->hidden = true;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 24;
			CCD_BIN_PROPERTY->hidden = true;
			CCD_COOLER_PROPERTY->hidden = true;
			CCD_COOLER_POWER_PROPERTY->hidden = true;
			CCD_TEMPERATURE_PROPERTY->hidden = true;
		} else if (device == PRIVATE_DATA->file) {
			FILE_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, "FILE_NAME", MAIN_GROUP, "File name", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			indigo_init_text_item(FILE_NAME_ITEM, "PATH", "Path", "");
			CCD_BIN_PROPERTY->hidden = true;
			CCD_INFO_PROPERTY->hidden = true;
			CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		} else {
			CCD_IMAGE_FORMAT_PROPERTY->count = 7;
			if (device == PRIVATE_DATA->guider) {
				GUIDER_MODE_PROPERTY = indigo_init_switch_property(NULL, device->name, "GUIDER_MODE", MAIN_GROUP, "Simulation Mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
				indigo_init_switch_item(GUIDER_MODE_STARS_ITEM, "STARS", "Stars", true);
				indigo_init_switch_item(GUIDER_MODE_FLIP_STARS_ITEM, "FLIP", "Stars (flipped)", false);
				indigo_init_switch_item(GUIDER_MODE_SUN_ITEM, "SUN", "Sun", false);
				indigo_init_switch_item(GUIDER_MODE_ECLIPSE_ITEM, "ECLIPSE", "Eclipse", false);
				PRIVATE_DATA->eclipse = -ECLIPSE;
				GUIDER_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, "SIMULATION_SETUP", MAIN_GROUP, "Simulation Setup", INDIGO_OK_STATE, INDIGO_RW_PERM, 22);
				indigo_init_number_item(GUIDER_IMAGE_WIDTH_ITEM, "IMAGE_WIDTH", "Image width (px)", 400, 16000, 0, 1600);
				indigo_init_number_item(GUIDER_IMAGE_HEIGHT_ITEM, "IMAGE_HEIGHT", "Image height (px)", 300, 12000, 0, 1200);
				indigo_init_number_item(GUIDER_IMAGE_NOISE_FIX_ITEM, "IMAGE_NOISE_FIX", "Image noise offset", 0, 5000, 0, 500);
				indigo_init_number_item(GUIDER_IMAGE_NOISE_VAR_ITEM, "IMAGE_NOISE_VAR", "Image noise range", 1, 1000, 0, 100);
				indigo_init_number_item(GUIDER_IMAGE_PERR_SPD_ITEM, "PER_ERR_SPD", "Periodic error speed", 0, 1, 0, 0.5);
				indigo_init_number_item(GUIDER_IMAGE_PERR_VAL_ITEM, "PER_ERR_VAL", "Periodic error value", 0, 10, 0, 5);
				indigo_init_number_item(GUIDER_IMAGE_GRADIENT_ITEM, "IMAGE_GRADIENT", "Image gradient intensity", 0, 0.5, 0, 0.2);
				indigo_init_number_item(GUIDER_IMAGE_ANGLE_ITEM, "IMAGE_ROTATION_ANGLE", "Image rotation angle (°)", 0, 360, 0, 36);
				indigo_init_number_item(GUIDER_IMAGE_AO_ANGLE_ITEM, "AO_ANGLE", "AO angle (°)", 0, 360, 0, 74);
				indigo_init_number_item(GUIDER_IMAGE_HOTPIXELS_ITEM, "IMAGE_HOTPIXELS", "Hot pixel count", 0, GUIDER_MAX_HOTPIXELS, 0, 0);
				indigo_init_number_item(GUIDER_IMAGE_HOTCOL_ITEM, "IMAGE_HOTCOL", "Hot column length (px)", 0, GUIDER_IMAGE_HEIGHT_ITEM->number.target, 0, 0);
				indigo_init_number_item(GUIDER_IMAGE_HOTROW_ITEM, "IMAGE_HOTROW", "Hot row length (px)", 0, GUIDER_IMAGE_WIDTH_ITEM->number.target, 0, 0);
				indigo_init_number_item(GUIDER_IMAGE_RA_OFFSET_ITEM, "IMAGE_RA_OFFSET", "RA offset (px)", 0, GUIDER_IMAGE_HEIGHT_ITEM->number.target, 0, 0);
				indigo_init_number_item(GUIDER_IMAGE_DEC_OFFSET_ITEM, "IMAGE_DEC_OFFSET", "DEC offset (px)", 0, GUIDER_IMAGE_HEIGHT_ITEM->number.target, 0, 0);
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_LAT_ITEM, "LAT", "Latitude (-90 to +90° +N)", -90, 90, 0, 48.1485965);
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_LONG_ITEM, "LONG", "Longitude (0 to 360° +E)", -180, 360, 0, 17.1077478);
				// deault is close to Vega
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_RA_ITEM, "RA", "RA (h)", 0, +24, 0, 18.84);
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_DEC_ITEM, "DEC", "Dec (°)", -90, +90, 0, 38.75);
				indigo_init_number_item(GUIDER_IMAGE_EPOCH_ITEM, "J2000", "J2000 (1=J2000, 0=JNow)", 0, 1, 0, 1);
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_ALT_ERROR_ITEM, "ALT_POLAR_ERROR", "Altitude polar error (°)", -30, +30, 0, 0);
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_AZ_ERROR_ITEM, "AZ_POLAR_ERROR", "Azimuth polar error (°)", -30, +30, 0, 0);
				indigo_init_sexagesimal_number_item(GUIDER_IMAGE_IMAGE_AGE_ITEM, "IMAGE_AGE", "Max image age (s)", 0, 3600, 0, 1.0 / 60.0);
				CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = GUIDER_IMAGE_WIDTH_ITEM->number.target;
				CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = GUIDER_IMAGE_HEIGHT_ITEM->number.target;
				PRIVATE_DATA->ra = PRIVATE_DATA->dec = -1000;
			} else {
				CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = IMAGER_WIDTH;
				CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = IMAGER_HEIGHT;
			}
			// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_MODE, CCD_FRAME
			CCD_FRAME_WIDTH_ITEM->number.min = CCD_FRAME_HEIGHT_ITEM->number.min = 32;
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 8;
			CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.max = 4;
			CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.max = 4;
			CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
			CCD_MODE_PROPERTY->count = 3;
			char name[32];
			sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value, (int)CCD_INFO_HEIGHT_ITEM->number.value);
			indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
			sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value/2, (int)CCD_INFO_HEIGHT_ITEM->number.value/2);
			indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
			sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value/4, (int)CCD_INFO_HEIGHT_ITEM->number.value/4);
			indigo_init_switch_item(CCD_MODE_ITEM+2, "BIN_4x4", name, false);
			CCD_INFO_PIXEL_SIZE_ITEM->number.value = 5.2;
			CCD_INFO_PIXEL_WIDTH_ITEM->number.value = 5.2;
			CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
			CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
			// -------------------------------------------------------------------------------- CCD_GAIN, CCD_OFFSET, CCD_GAMMA
			CCD_GAIN_PROPERTY->hidden = CCD_OFFSET_PROPERTY->hidden = CCD_GAMMA_PROPERTY->hidden = false;
			// -------------------------------------------------------------------------------- CCD_IMAGE
			for (int i = 0; i <= GUIDER_MAX_HOTPIXELS; i++) {
				PRIVATE_DATA->hotpixel_x[i] = rand() % ((int)CCD_INFO_WIDTH_ITEM->number.value - 200) + 100;
				PRIVATE_DATA->hotpixel_y[i] = rand() % ((int)CCD_INFO_HEIGHT_ITEM->number.value - 200) + 100;
			}
			// -------------------------------------------------------------------------------- CCD_COOLER, CCD_TEMPERATURE, CCD_COOLER_POWER
			if (device == PRIVATE_DATA->imager) {
				CCD_COOLER_PROPERTY->hidden = false;
				CCD_TEMPERATURE_PROPERTY->hidden = false;
				CCD_COOLER_POWER_PROPERTY->hidden = false;
				PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = 25;
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_COOLER_POWER_ITEM->number.value = 0;
			} else {
				CCD_COOLER_PROPERTY->hidden = true;
				CCD_COOLER_POWER_PROPERTY->hidden = true;
				CCD_TEMPERATURE_PROPERTY->hidden = true;
			}
		}
		// -------------------------------------------------------------------------------- CCD_STREAMING
		CCD_STREAMING_PROPERTY->hidden = false;
		CCD_STREAMING_EXPOSURE_ITEM->number.min = 0.001;
		CCD_STREAMING_EXPOSURE_ITEM->number.max = 0.5;
		// --------------------------------------------------------------------------------
		if (device == PRIVATE_DATA->imager)
			ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_ccd_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (device == PRIVATE_DATA->dslr) {
				if (indigo_property_match(DSLR_PROGRAM_PROPERTY, property))
					indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);
				if (indigo_property_match(DSLR_CAPTURE_MODE_PROPERTY, property))
					indigo_define_property(device, DSLR_CAPTURE_MODE_PROPERTY, NULL);
				if (indigo_property_match(DSLR_APERTURE_PROPERTY, property))
					indigo_define_property(device, DSLR_APERTURE_PROPERTY, NULL);
				if (indigo_property_match(DSLR_SHUTTER_PROPERTY, property))
					indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
				if (indigo_property_match(DSLR_COMPRESSION_PROPERTY, property))
					indigo_define_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
				if (indigo_property_match(DSLR_ISO_PROPERTY, property))
					indigo_define_property(device, DSLR_ISO_PROPERTY, NULL);
				if (indigo_property_match(DSLR_BATTERY_LEVEL_PROPERTY, property))
					indigo_define_property(device, DSLR_BATTERY_LEVEL_PROPERTY, NULL);
			}
		}
		if (device == PRIVATE_DATA->file) {
			if (indigo_property_match(FILE_NAME_PROPERTY, property))
				indigo_define_property(device, FILE_NAME_PROPERTY, NULL);
		}
		if (device == PRIVATE_DATA->guider) {
			if (indigo_property_match(GUIDER_MODE_PROPERTY, property))
				indigo_define_property(device, GUIDER_MODE_PROPERTY, NULL);
			if (indigo_property_match(GUIDER_SETTINGS_PROPERTY, property))
				indigo_define_property(device, GUIDER_SETTINGS_PROPERTY, NULL);
		}
	}
	return result;
}

static void ccd_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!device->is_connected) { /* Do not double open device */
			device->is_connected = true;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			if (device == PRIVATE_DATA->dslr) {
				indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);
				indigo_define_property(device, DSLR_CAPTURE_MODE_PROPERTY, NULL);
				indigo_define_property(device, DSLR_APERTURE_PROPERTY, NULL);
				indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
				indigo_define_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
				indigo_define_property(device, DSLR_ISO_PROPERTY, NULL);
				indigo_define_property(device, DSLR_BATTERY_LEVEL_PROPERTY, NULL);
			} else if (device == PRIVATE_DATA->file) {
				int fd = open(FILE_NAME_ITEM->text.value, O_RDONLY, 0);
				if (fd == -1)
					goto failure;
				if (!indigo_read(fd, (char *)&PRIVATE_DATA->file_image_header, sizeof(PRIVATE_DATA->file_image_header)))
					goto failure;
				CCD_FRAME_TOP_ITEM->number.value = CCD_FRAME_LEFT_ITEM->number.value = 0;
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.min = CCD_FRAME_WIDTH_ITEM->number.max = PRIVATE_DATA->file_image_header.width;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.min = CCD_FRAME_HEIGHT_ITEM->number.max = PRIVATE_DATA->file_image_header.height;
				unsigned long size = 0;
				switch (PRIVATE_DATA->file_image_header.signature) {
					case INDIGO_RAW_MONO8:
						size = PRIVATE_DATA->file_image_header.width * PRIVATE_DATA->file_image_header.height;
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8;
						break;
					case INDIGO_RAW_MONO16:
						size = 2 * PRIVATE_DATA->file_image_header.width * PRIVATE_DATA->file_image_header.height;
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16;
						break;
					case INDIGO_RAW_RGB24:
						size = 3 *PRIVATE_DATA->file_image_header.width * PRIVATE_DATA->file_image_header.height;
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 24;
						break;
					case INDIGO_RAW_RGB48:
						size = 6 * PRIVATE_DATA->file_image_header.width * PRIVATE_DATA->file_image_header.height;
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 48;
						break;
				}
				PRIVATE_DATA->raw_file_image = indigo_alloc_blob_buffer(size + FITS_HEADER_SIZE);
				PRIVATE_DATA->file_image = indigo_alloc_blob_buffer(size + FITS_HEADER_SIZE);
				if (!indigo_read(fd, (char *)PRIVATE_DATA->raw_file_image + FITS_HEADER_SIZE, size)) {
					goto failure;
				}
				close(fd);
			} else if (device == PRIVATE_DATA->imager) {
				indigo_set_timer(device, TEMP_UPDATE, ccd_temperature_callback, &PRIVATE_DATA->temperature_timer);
			}
		}
	} else {
		if (device->is_connected) {  /* Do not double close device */
			if (device == PRIVATE_DATA->imager) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->temperature_timer);
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->imager_exposure_timer);
			} else if (device == PRIVATE_DATA->file) {
				if (PRIVATE_DATA->file_image) {
					free(PRIVATE_DATA->file_image);
					PRIVATE_DATA->file_image = NULL;
				}
				if (PRIVATE_DATA->raw_file_image) {
					free(PRIVATE_DATA->raw_file_image);
					PRIVATE_DATA->raw_file_image = NULL;
				}
			} else if (device == PRIVATE_DATA->guider) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_exposure_timer);
			} else if (device == PRIVATE_DATA->dslr) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->dslr_exposure_timer);
				indigo_delete_property(device, DSLR_PROGRAM_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_CAPTURE_MODE_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_APERTURE_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_ISO_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_BATTERY_LEVEL_PROPERTY, NULL);
			} else if (device == PRIVATE_DATA->file) {
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->file_exposure_timer);
			}
			device->is_connected = false;
			indigo_global_unlock(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	return;
failure:
	indigo_global_unlock(device);
	device->is_connected = false;
	CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
	indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
	indigo_update_property(device, CONNECTION_PROPERTY, NULL);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(FILE_NAME_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FILE_NAME
		indigo_property_copy_values(FILE_NAME_PROPERTY, property, false);
		FILE_NAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FILE_NAME_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		if (device == PRIVATE_DATA->imager)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value > 0 ? CCD_EXPOSURE_ITEM->number.value : 0.1, exposure_timer_callback, &PRIVATE_DATA->imager_exposure_timer);
		else if (device == PRIVATE_DATA->guider)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value > 0 ? CCD_EXPOSURE_ITEM->number.value : 0.1, exposure_timer_callback, &PRIVATE_DATA->guider_exposure_timer);
		else if (device == PRIVATE_DATA->dslr)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value > 0 ? CCD_EXPOSURE_ITEM->number.value : 0.1, exposure_timer_callback, &PRIVATE_DATA->dslr_exposure_timer);
		else if (device == PRIVATE_DATA->file)
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value > 0 ? CCD_EXPOSURE_ITEM->number.value : 0.1, exposure_timer_callback, &PRIVATE_DATA->file_exposure_timer);
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
		if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		indigo_use_shortest_exposure_if_bias(device);
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		indigo_set_timer(device, 0, streaming_timer_callback, NULL);
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
		if (device == PRIVATE_DATA->imager)
			indigo_cancel_timer(device, &PRIVATE_DATA->imager_exposure_timer);
		else if (device == PRIVATE_DATA->guider)
			indigo_cancel_timer(device, &PRIVATE_DATA->guider_exposure_timer);
		else if (device == PRIVATE_DATA->dslr)
			indigo_cancel_timer(device, &PRIVATE_DATA->dslr_exposure_timer);
		else if (device == PRIVATE_DATA->file)
			indigo_cancel_timer(device, &PRIVATE_DATA->file_exposure_timer);
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		int h = CCD_BIN_HORIZONTAL_ITEM->number.value;
		int v = CCD_BIN_VERTICAL_ITEM->number.value;
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		if (!(CCD_BIN_HORIZONTAL_ITEM->number.value == 1 || CCD_BIN_HORIZONTAL_ITEM->number.value == 2 || CCD_BIN_HORIZONTAL_ITEM->number.value == 4) || CCD_BIN_HORIZONTAL_ITEM->number.value != CCD_BIN_VERTICAL_ITEM->number.value) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = h;
			CCD_BIN_VERTICAL_ITEM->number.value = v;
			CCD_BIN_PROPERTY->state = INDIGO_ALERT_STATE;
			if (IS_CONNECTED) {
				indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
			}
			return INDIGO_OK;
		}
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CCD_COOLER_ON_ITEM->sw.value) {
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_COOLER_POWER_ITEM->number.value = 0;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_IDLE_STATE;
		}
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_w(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
		CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
		CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target temperature %g", PRIVATE_DATA->target_temperature);
		return INDIGO_OK;
	} else if (DSLR_APERTURE_PROPERTY && indigo_property_match(DSLR_PROGRAM_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_PROGRAM
		indigo_property_copy_values(DSLR_PROGRAM_PROPERTY, property, false);
		indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
		if (DSLR_PROGRAM_PROPERTY->items[1].sw.value)
			DSLR_SHUTTER_PROPERTY->hidden = true;
		else
			DSLR_SHUTTER_PROPERTY->hidden = false;
		indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
		DSLR_PROGRAM_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_PROGRAM_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (DSLR_APERTURE_PROPERTY && indigo_property_match(DSLR_APERTURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_APERTURE
		indigo_property_copy_values(DSLR_APERTURE_PROPERTY, property, false);
		DSLR_APERTURE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_APERTURE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (DSLR_SHUTTER_PROPERTY && indigo_property_match(DSLR_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_SHUTTER
		indigo_property_copy_values(DSLR_SHUTTER_PROPERTY, property, false);
		DSLR_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (DSLR_ISO_PROPERTY && indigo_property_match(DSLR_ISO_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- DSLR_ISO
		indigo_property_copy_values(DSLR_ISO_PROPERTY, property, false);
		DSLR_ISO_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_ISO_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (GUIDER_MODE_PROPERTY && indigo_property_match(GUIDER_MODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_MODE
		indigo_property_copy_values(GUIDER_MODE_PROPERTY, property, false);
		GUIDER_MODE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_MODE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (GUIDER_MODE_PROPERTY && indigo_property_match(GUIDER_SETTINGS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_SETTINGS
		indigo_property_copy_values(GUIDER_SETTINGS_PROPERTY, property, false);
		if (GUIDER_IMAGE_EPOCH_ITEM->number.target != 0)
			GUIDER_IMAGE_EPOCH_ITEM->number.target = GUIDER_IMAGE_EPOCH_ITEM->number.value = 1;
		PRIVATE_DATA->ra = PRIVATE_DATA->dec = 0;
		GUIDER_IMAGE_HOTCOL_ITEM->number.max = GUIDER_IMAGE_HEIGHT_ITEM->number.target;
		GUIDER_IMAGE_HOTROW_ITEM->number.max = GUIDER_IMAGE_WIDTH_ITEM->number.target;
		GUIDER_IMAGE_RA_OFFSET_ITEM->number.max = GUIDER_IMAGE_HEIGHT_ITEM->number.target;
		GUIDER_IMAGE_DEC_OFFSET_ITEM->number.max = GUIDER_IMAGE_HEIGHT_ITEM->number.target;
		CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = CCD_FRAME_WIDTH_ITEM->number.max = GUIDER_IMAGE_WIDTH_ITEM->number.target;
		CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = CCD_FRAME_HEIGHT_ITEM->number.max = GUIDER_IMAGE_HEIGHT_ITEM->number.target;
		sprintf(CCD_MODE_ITEM[0].label, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value, (int)CCD_INFO_HEIGHT_ITEM->number.value);
		sprintf(CCD_MODE_ITEM[1].label, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value / 2, (int)CCD_INFO_HEIGHT_ITEM->number.value / 2);
		sprintf(CCD_MODE_ITEM[2].label, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value / 4, (int)CCD_INFO_HEIGHT_ITEM->number.value / 4);
		if (IS_CONNECTED) {
			indigo_delete_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_delete_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_delete_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_define_property(device, CCD_INFO_PROPERTY, NULL);
			indigo_define_property(device, CCD_FRAME_PROPERTY, NULL);
			indigo_define_property(device, CCD_MODE_PROPERTY, NULL);
		}
		PRIVATE_DATA->lst = 0;
		GUIDER_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CCD_FITS_HEADERS
	} else if (GUIDER_MODE_PROPERTY && indigo_property_match(CCD_FITS_HEADERS_PROPERTY, property)) {
		if (device == PRIVATE_DATA->guider) {
			bool update = false;
			for (int i = 0; i < property->count; i++) {
				indigo_item *item = property->items + i;
				int d, m, s;
				if (sscanf(item->text.value, "OBJCTRA='%d %d %d'", &d, &m, &s) == 3) {
					GUIDER_IMAGE_RA_ITEM->number.value = GUIDER_IMAGE_RA_ITEM->number.target = d + m / 60.0 + s / 3600.0;
					update = true;
				}
				if (sscanf(item->text.value, "OBJCTDEC='%d %d %d'", &d, &m, &s) == 3) {
					GUIDER_IMAGE_DEC_ITEM->number.value = GUIDER_IMAGE_DEC_ITEM->number.target = (abs(d) + m / 60.0 + s / 3600.0) * (d >= 0 ? 1 : -1);
					update = true;
				}
				if (sscanf(item->text.value, "SITELAT='%d %d %d'", &d, &m, &s) == 3) {
					GUIDER_IMAGE_LAT_ITEM->number.value = GUIDER_IMAGE_LAT_ITEM->number.target = (abs(d) + m / 60.0 + s / 3600.0) * (d >= 0 ? 1 : -1);
					update = true;
				}
				if (sscanf(item->text.value, "SITELONG='%d %d %d'", &d, &m, &s) == 3) {
					GUIDER_IMAGE_LONG_ITEM->number.value = GUIDER_IMAGE_LONG_ITEM->number.target = (abs(d) + m / 60.0 + s / 3600.0) * (d >= 0 ? 1 : -1);
					update = true;
				}
			}
			if (update)
				indigo_update_property(device, GUIDER_SETTINGS_PROPERTY, NULL);
		}
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			if (device == PRIVATE_DATA->guider) {
				indigo_save_property(device, NULL, GUIDER_SETTINGS_PROPERTY);
			} else if (device == PRIVATE_DATA->file) {
				indigo_save_property(device, NULL, FILE_NAME_PROPERTY);
			}
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	if (device == PRIVATE_DATA->dslr) {
		indigo_release_property(DSLR_PROGRAM_PROPERTY);
		indigo_release_property(DSLR_CAPTURE_MODE_PROPERTY);
		indigo_release_property(DSLR_APERTURE_PROPERTY);
		indigo_release_property(DSLR_SHUTTER_PROPERTY);
		indigo_release_property(DSLR_COMPRESSION_PROPERTY);
		indigo_release_property(DSLR_ISO_PROPERTY);
		indigo_release_property(DSLR_BATTERY_LEVEL_PROPERTY);
	} else if (device == PRIVATE_DATA->file) {
		indigo_release_property(FILE_NAME_PROPERTY);
	} else if (device == PRIVATE_DATA->guider) {
		indigo_release_property(GUIDER_MODE_PROPERTY);
		indigo_release_property(GUIDER_SETTINGS_PROPERTY);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static void guider_timer_callback(indigo_device *device) {
	PRIVATE_DATA->guider_timer = NULL;
	bool update_setup = false;
	if (GUIDER_GUIDE_NORTH_ITEM->number.value != 0 || GUIDER_GUIDE_SOUTH_ITEM->number.value != 0) {
		GUIDER_IMAGE_DEC_OFFSET_ITEM->number.value += PRIVATE_DATA->guide_rate * (GUIDER_GUIDE_NORTH_ITEM->number.value - GUIDER_GUIDE_SOUTH_ITEM->number.value) / 200;
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		update_setup = true;
	}
	if (GUIDER_GUIDE_EAST_ITEM->number.value != 0 || GUIDER_GUIDE_WEST_ITEM->number.value != 0) {
		GUIDER_IMAGE_RA_OFFSET_ITEM->number.value += cos(M_PI * GUIDER_IMAGE_DEC_ITEM->number.value / 180.0) * PRIVATE_DATA->guide_rate * (GUIDER_GUIDE_WEST_ITEM->number.value - GUIDER_GUIDE_EAST_ITEM->number.value) / 200;
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		update_setup = true;
	}
	if (update_setup)
		indigo_update_property(PRIVATE_DATA->guider, GUIDER_SETTINGS_PROPERTY, NULL);
}

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GUIDER_RATE_PROPERTY->hidden = false;
		PRIVATE_DATA->guide_rate = GUIDER_RATE_ITEM->number.value / 100.0;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void guider_connect_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer);
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer);
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_set_timer(device, duration/1000.0, guider_timer_callback, &PRIVATE_DATA->guider_timer);
			}
		}
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_RATE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_RATE
		indigo_property_copy_values(GUIDER_RATE_PROPERTY, property, false);
		PRIVATE_DATA->guide_rate = GUIDER_RATE_ITEM->number.value / 100.0;
		GUIDER_RATE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_RATE_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO AO device implementation

static indigo_result ao_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ao_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		AO_RESET_PROPERTY->count = 1;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ao_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ao_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_ao_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ao_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_GUIDE_DEC
		indigo_property_copy_values(AO_GUIDE_DEC_PROPERTY, property, false);
		AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		if (AO_GUIDE_NORTH_ITEM->number.value || AO_GUIDE_SOUTH_ITEM->number.value) {
			PRIVATE_DATA->ao_dec_offset += (AO_GUIDE_NORTH_ITEM->number.value - AO_GUIDE_SOUTH_ITEM->number.value) / 30.0;
			AO_GUIDE_NORTH_ITEM->number.value = 0;
			AO_GUIDE_SOUTH_ITEM->number.value = 0;
			if (PRIVATE_DATA->ao_dec_offset > 100) {
				PRIVATE_DATA->ao_dec_offset = 100;
				AO_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (PRIVATE_DATA->ao_dec_offset < -100) {
				PRIVATE_DATA->ao_dec_offset = -100;
				AO_GUIDE_DEC_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_GUIDE_RA
		indigo_property_copy_values(AO_GUIDE_RA_PROPERTY, property, false);
		AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		if (AO_GUIDE_EAST_ITEM->number.value || AO_GUIDE_WEST_ITEM->number.value) {
			PRIVATE_DATA->ao_ra_offset += (AO_GUIDE_EAST_ITEM->number.value - AO_GUIDE_WEST_ITEM->number.value) / 30.0;
			AO_GUIDE_EAST_ITEM->number.value = 0;
			if (PRIVATE_DATA->ao_ra_offset > 100) {
				PRIVATE_DATA->ao_ra_offset = 100;
				AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			} else if (PRIVATE_DATA->ao_ra_offset < -100) {
				PRIVATE_DATA->ao_ra_offset = -100;
				AO_GUIDE_RA_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_RESET
		indigo_property_copy_values(AO_RESET_PROPERTY, property, false);
		if (AO_CENTER_ITEM->sw.value) {
			PRIVATE_DATA->ao_ra_offset = 0;
			PRIVATE_DATA->ao_dec_offset = 0;
			AO_CENTER_ITEM->sw.value = false;
			AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
			AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
			AO_RESET_PROPERTY->state = INDIGO_OK_STATE;
		}
		indigo_update_property(device, AO_RESET_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_ao_change_property(device, client, property);
}

static indigo_result ao_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ao_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ao_detach(device);
}
// -------------------------------------------------------------------------------- INDIGO wheel device implementation

#define FILTER_COUNT	5

static void wheel_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void wheel_timer_callback(indigo_device *device) {
	PRIVATE_DATA->current_slot = (PRIVATE_DATA->current_slot) % (int)WHEEL_SLOT_ITEM->number.max + 1;
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->current_slot;
	if (PRIVATE_DATA->current_slot == WHEEL_SLOT_ITEM->number.target) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target;
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
	} else {
		indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
	}
}

static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT, WHEEL_SLOT_NAME
		WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = FILTER_COUNT;
		WHEEL_SLOT_ITEM->number.value = WHEEL_SLOT_ITEM->number.target = PRIVATE_DATA->current_slot = 1;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, wheel_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(WHEEL_SLOT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- WHEEL_SLOT
		indigo_property_copy_values(WHEEL_SLOT_PROPERTY, property, false);
		if (WHEEL_SLOT_ITEM->number.value < 1 || WHEEL_SLOT_ITEM->number.value > WHEEL_SLOT_ITEM->number.max) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			WHEEL_SLOT_ITEM->number.value = 0;
			indigo_set_timer(device, 0.5, wheel_timer_callback, NULL);
		}
		indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_wheel_change_property(device, client, property);
}

static indigo_result wheel_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		wheel_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_wheel_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static void focuser_timer_callback(indigo_device *device) {
	if (FOCUSER_POSITION_PROPERTY->state == INDIGO_ALERT_STATE) {
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->target_position = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
	} else {
		if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value && PRIVATE_DATA->current_position < PRIVATE_DATA->target_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			int steps = FOCUSER_SPEED_ITEM->number.value;
			if (PRIVATE_DATA->target_position - PRIVATE_DATA->current_position < steps) {
				steps = PRIVATE_DATA->target_position - PRIVATE_DATA->current_position;
			}
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->current_position + steps;
			if (PRIVATE_DATA->backlash_out > steps) {
				PRIVATE_DATA->backlash_out -= steps;
			} else {
				FOCUSER_SETTINGS_FOCUS_ITEM->number.value += steps - PRIVATE_DATA->backlash_out;
				PRIVATE_DATA->backlash_out = 0;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "position = %d, focus = %d, backlash_out = %d", (int)FOCUSER_POSITION_ITEM->number.value, (int)FOCUSER_SETTINGS_FOCUS_ITEM->number.value, PRIVATE_DATA->backlash_out);
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_SETTINGS_PROPERTY, NULL);
			indigo_set_timer(device, 0.1, focuser_timer_callback, NULL);
		} else if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value && PRIVATE_DATA->current_position > PRIVATE_DATA->target_position) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
			int steps = FOCUSER_SPEED_ITEM->number.value;
			if (PRIVATE_DATA->current_position - PRIVATE_DATA->target_position < steps) {
				steps = PRIVATE_DATA->current_position - PRIVATE_DATA->target_position;
			}
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position = PRIVATE_DATA->current_position - steps;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			if (PRIVATE_DATA->backlash_in > steps) {
				PRIVATE_DATA->backlash_in -= steps;
			} else {
				FOCUSER_SETTINGS_FOCUS_ITEM->number.value -= steps - PRIVATE_DATA->backlash_in;
				PRIVATE_DATA->backlash_in = 0;
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "position = %d, focus = %d, backlash_in = %d", (int)FOCUSER_POSITION_ITEM->number.value, (int)FOCUSER_SETTINGS_FOCUS_ITEM->number.value, PRIVATE_DATA->backlash_in);
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			indigo_update_property(device, FOCUSER_SETTINGS_PROPERTY, NULL);
			indigo_set_timer(device, 0.1, focuser_timer_callback, NULL);
		} else {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
	}
}

static void focuser_connect_callback(indigo_device *device) {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		FOCUSER_SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, "FOCUSER_SETUP", MAIN_GROUP, "Focuser Setup", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_number_item(FOCUSER_SETTINGS_FOCUS_ITEM, "FOCUS", "Focus", FOCUSER_POSITION_ITEM->number.min, FOCUSER_POSITION_ITEM->number.max, 0, 0);
		indigo_init_number_item(FOCUSER_SETTINGS_BL_ITEM, "BACKLASH", "Backlash", 0, 1000, 0, 0);
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.value = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->perm = INDIGO_RW_PERM;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE / FOCUSER_COMPENSATION
		FOCUSER_TEMPERATURE_PROPERTY->hidden = false;
		FOCUSER_TEMPERATURE_ITEM->number.value = 25;
		FOCUSER_COMPENSATION_PROPERTY->hidden = false;
		FOCUSER_MODE_PROPERTY->hidden = false;
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
#ifdef ENABLE_BACKLASH_PROPERTY
		FOCUSER_BACKLASH_PROPERTY->hidden = false;
#endif
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_focuser_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (indigo_property_match(FOCUSER_SETTINGS_PROPERTY, property))
			indigo_define_property(device, FOCUSER_SETTINGS_PROPERTY, NULL);
	}
	return result;
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, focuser_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		PRIVATE_DATA->target_position = FOCUSER_POSITION_ITEM->number.target;
		if (PRIVATE_DATA->target_position < PRIVATE_DATA->current_position) {
			if (!FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				PRIVATE_DATA->backlash_in = FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_out ? (FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_out) : 0;
				PRIVATE_DATA->backlash_out = 0;
			}
			indigo_set_switch(FOCUSER_DIRECTION_PROPERTY, FOCUSER_DIRECTION_MOVE_INWARD_ITEM, true);
			FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		} else {
			if (!FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
				PRIVATE_DATA->backlash_out = FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_in ? (FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_in) : 0;
				PRIVATE_DATA->backlash_in = 0;
			}
			indigo_set_switch(FOCUSER_DIRECTION_PROPERTY, FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM, true);
			FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "backlash_in = %d, backlash_out = %d", PRIVATE_DATA->backlash_in, PRIVATE_DATA->backlash_out);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0.5, focuser_timer_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position - FOCUSER_STEPS_ITEM->number.value;
		} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
			PRIVATE_DATA->target_position = PRIVATE_DATA->current_position + FOCUSER_STEPS_ITEM->number.value;
		}
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		indigo_set_timer(device, 0.5, focuser_timer_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_COMPENSATION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_COMPENSATION
		indigo_property_copy_values(FOCUSER_COMPENSATION_PROPERTY, property, false);
		FOCUSER_COMPENSATION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_COMPENSATION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_BACKLASH_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_BACKLASH
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_BACKLASH_PROPERTY, property, false);
			FOCUSER_BACKLASH_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, FOCUSER_BACKLASH_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value && FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE) {
			FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
			FOCUSER_POSITION_ITEM->number.value = PRIVATE_DATA->current_position;
			indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- FOCUSER_DIRECTION
	} else if (indigo_property_match(FOCUSER_DIRECTION_PROPERTY, property)) {
		bool was_outward = FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value;
		indigo_property_copy_values(FOCUSER_DIRECTION_PROPERTY, property, false);
		if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value && !was_outward) {
			PRIVATE_DATA->backlash_out = FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_in ? (FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_in) : 0;
			PRIVATE_DATA->backlash_in = 0;
		} else if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value && was_outward) {
			PRIVATE_DATA->backlash_in = FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_out ? (FOCUSER_SETTINGS_BL_ITEM->number.value - PRIVATE_DATA->backlash_out) : 0;
			PRIVATE_DATA->backlash_out = 0;
		}
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "backlash_in = %d, backlash_out = %d", PRIVATE_DATA->backlash_in, PRIVATE_DATA->backlash_out);
		FOCUSER_DIRECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, FOCUSER_DIRECTION_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_SETTINGS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SETTINGS
		indigo_property_copy_values(FOCUSER_SETTINGS_PROPERTY, property, false);
		FOCUSER_SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SETTINGS_PROPERTY, NULL);
		return INDIGO_OK;
		// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, FOCUSER_SETTINGS_PROPERTY);
		}
		// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		focuser_connect_callback(device);
	}
	indigo_release_property(FOCUSER_SETTINGS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------

static simulator_private_data *private_data = NULL;

static indigo_device *imager_ccd = NULL;
static indigo_device *imager_wheel = NULL;
static indigo_device *imager_focuser = NULL;

static indigo_device *guider_ccd = NULL;
static indigo_device *guider_guider = NULL;
static indigo_device *guider_ao = NULL;

static indigo_device *dslr = NULL;

static indigo_device *file = NULL;

indigo_result indigo_ccd_simulator(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device imager_camera_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_IMAGER_CAMERA_NAME,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	static indigo_device imager_wheel_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_WHEEL_NAME,
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
	);
	static indigo_device imager_focuser_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_FOCUSER_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
	);
	static indigo_device guider_camera_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_GUIDER_CAMERA_NAME,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_GUIDER_NAME,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);
	static indigo_device ao_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_AO_NAME,
		ao_attach,
		indigo_ao_enumerate_properties,
		ao_change_property,
		NULL,
		ao_detach
	);
	static indigo_device dslr_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_DSLR_NAME,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	static indigo_device file_template = INDIGO_DEVICE_INITIALIZER(
		CCD_SIMULATOR_FILE_NAME,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Camera Simulator", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(simulator_private_data));
			pthread_mutex_init(&private_data->image_mutex, NULL);
			imager_ccd = indigo_safe_malloc_copy(sizeof(indigo_device), &imager_camera_template);
			imager_ccd->private_data = private_data;
			private_data->imager = imager_ccd;
			indigo_attach_device(imager_ccd);
			imager_wheel = indigo_safe_malloc_copy(sizeof(indigo_device), &imager_wheel_template);
			imager_wheel->private_data = private_data;
			imager_wheel->master_device = imager_ccd;
			indigo_attach_device(imager_wheel);
			imager_focuser = indigo_safe_malloc_copy(sizeof(indigo_device), &imager_focuser_template);
			imager_focuser->private_data = private_data;
			imager_focuser->master_device = imager_ccd;
			indigo_attach_device(imager_focuser);
			guider_ccd = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_camera_template);
			guider_ccd->private_data = private_data;
			guider_ccd->master_device = imager_ccd;
			private_data->guider = guider_ccd;
			indigo_attach_device(guider_ccd);
			guider_guider = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
			guider_guider->private_data = private_data;
			guider_guider->master_device = imager_ccd;
			indigo_attach_device(guider_guider);
			guider_ao = indigo_safe_malloc_copy(sizeof(indigo_device), &ao_template);
			guider_ao->private_data = private_data;
			guider_ao->master_device = imager_ccd;
			indigo_attach_device(guider_ao);

			dslr = indigo_safe_malloc_copy(sizeof(indigo_device), &dslr_template);
			dslr->private_data = private_data;
			dslr->master_device = imager_ccd;
			private_data->dslr = dslr;
			indigo_attach_device(dslr);

			file = indigo_safe_malloc_copy(sizeof(indigo_device), &file_template);
			file->private_data = private_data;
			file->master_device = imager_ccd;
			private_data->file = file;
			indigo_attach_device(file);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(imager_ccd);
			VERIFY_NOT_CONNECTED(imager_wheel);
			VERIFY_NOT_CONNECTED(imager_focuser);
			VERIFY_NOT_CONNECTED(guider_ccd);
			VERIFY_NOT_CONNECTED(guider_guider);
			VERIFY_NOT_CONNECTED(guider_ao);
			VERIFY_NOT_CONNECTED(dslr);
			last_action = action;
			if (imager_ccd != NULL) {
				indigo_detach_device(imager_ccd);
				free(imager_ccd);
				imager_ccd = NULL;
			}
			if (imager_wheel != NULL) {
				indigo_detach_device(imager_wheel);
				free(imager_wheel);
				imager_wheel = NULL;
			}
			if (imager_focuser != NULL) {
				indigo_detach_device(imager_focuser);
				free(imager_focuser);
				imager_focuser = NULL;
			}
			if (guider_ccd != NULL) {
				indigo_detach_device(guider_ccd);
				free(guider_ccd);
				guider_ccd = NULL;
			}
			if (guider_guider != NULL) {
				indigo_detach_device(guider_guider);
				free(guider_guider);
				guider_guider = NULL;
			}
			if (guider_ao != NULL) {
				indigo_detach_device(guider_ao);
				free(guider_ao);
				guider_ao = NULL;
			}
			if (dslr != NULL) {
				indigo_detach_device(dslr);
				free(dslr);
				dslr = NULL;
			}
			if (file != NULL) {
				indigo_detach_device(file);
				free(file);
				file = NULL;
			}
			if (private_data != NULL) {
				pthread_mutex_destroy(&private_data->image_mutex);
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
