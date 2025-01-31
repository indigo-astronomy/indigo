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

/** INDIGO CCD Driver base
 \file indigo_ccd_driver.h
 */

#ifndef indigo_ccd_h
#define indigo_ccd_h

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_fits.h>

typedef enum {
	CCD_JPEG_STRETCH_SLIGHT = 0,
	CCD_JPEG_STRETCH_MODERATE,
	CCD_JPEG_STRETCH_NORMAL,
	CCD_JPEG_STRETCH_HARD,
	CCD_JPEG_STRETCH_COUNT
} ccd_jpeg_stretch_level;

typedef struct {
	float target_background;
	float clipping_point;
} ccd_jpeg_stretch_params_t;

static const ccd_jpeg_stretch_params_t ccd_jpeg_stretch_params_lut[] ={
	{0.05, -2.8},
	{0.15, -2.8},
	{0.25, -2.8},
	{0.40, -2.5}
};

#ifdef __cplusplus
extern "C" {
#endif

/** CCD Main group name string.
 */
#define CCD_MAIN_GROUP                    "Camera"

/** Image group name string.
 */
#define CCD_IMAGE_GROUP                   "Image"

/** Cooler group name string.
 */
#define CCD_COOLER_GROUP                  "Cooler"

/** Advanced group name string.
 */
#define CCD_ADVANCED_GROUP                ADVANCED_GROUP

/** Device context pointer.
 */
#define CCD_CONTEXT                ((indigo_ccd_context *)device->device_context)

/** CCD_INFO property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_INFO_PROPERTY                 (CCD_CONTEXT->ccd_info_property)

/** CCD_INFO.WIDTH property item pointer.
 */
#define CCD_INFO_WIDTH_ITEM               (CCD_INFO_PROPERTY->items+0)

/** CCD_INFO.HEIGHT property item pointer.
 */
#define CCD_INFO_HEIGHT_ITEM              (CCD_INFO_PROPERTY->items+1)

/** CCD_INFO.MAX_HORIZONAL_BIN property item pointer.
 */
#define CCD_INFO_MAX_HORIZONAL_BIN_ITEM   (CCD_INFO_PROPERTY->items+2)

/** CCD_INFO.MAX_VERTICAL_BIN property item pointer.
 */
#define CCD_INFO_MAX_VERTICAL_BIN_ITEM    (CCD_INFO_PROPERTY->items+3)

/** CCD_INFO.PIXEL_SIZE property item pointer.
 */
#define CCD_INFO_PIXEL_SIZE_ITEM          (CCD_INFO_PROPERTY->items+4)

/** CCD_INFO.PIXEL_WIDTH property item pointer.
 */
#define CCD_INFO_PIXEL_WIDTH_ITEM         (CCD_INFO_PROPERTY->items+5)

/** CCD_INFO.PIXEL_HEIGHT property item pointer.
 */
#define CCD_INFO_PIXEL_HEIGHT_ITEM        (CCD_INFO_PROPERTY->items+6)

/** CCD_INFO.BITS_PER_PIXEL property item pointer.
 */
#define CCD_INFO_BITS_PER_PIXEL_ITEM      (CCD_INFO_PROPERTY->items+7)

/** CCD_LENS property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_LENS_PROPERTY                 (CCD_CONTEXT->ccd_lens_property)

/** CCD_LENS.APERTURE property item pointer.
 */

#define CCD_LENS_APERTURE_ITEM      			(CCD_LENS_PROPERTY->items+0)

/** CCD_LENS.FOCAL_LENGTH property item pointer.
 */

#define CCD_LENS_FOCAL_LENGTH_ITEM      			(CCD_LENS_PROPERTY->items+1)

/** CCD_UPLOAD_MODE property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_UPLOAD_MODE_PROPERTY          (CCD_CONTEXT->ccd_upload_mode_property)

/** CCD_UPLOAD_MODE.CLIENT property item pointer.
 */
#define CCD_UPLOAD_MODE_CLIENT_ITEM       (CCD_UPLOAD_MODE_PROPERTY->items+0)

/** CCD_UPLOAD_MODE.LOCAL property item pointer.
 */
#define CCD_UPLOAD_MODE_LOCAL_ITEM        (CCD_UPLOAD_MODE_PROPERTY->items+1)

/** CCD_UPLOAD_MODE.BOTH property item pointer.
 */
#define CCD_UPLOAD_MODE_BOTH_ITEM         (CCD_UPLOAD_MODE_PROPERTY->items+2)

/** CCD_UPLOAD_MODE.NONE property item pointer.
 */
#define CCD_UPLOAD_MODE_NONE_ITEM         (CCD_UPLOAD_MODE_PROPERTY->items+3)

/** CCD_PREVIEW property pointer, property is mandatory, read-write property, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_PREVIEW_PROPERTY         		(CCD_CONTEXT->ccd_preview_property)

/** CCD_PREVIEW.DISABLED property item pointer.
 */
#define CCD_PREVIEW_DISABLED_ITEM        (CCD_PREVIEW_PROPERTY->items+0)

/** CCD_PREVIEW.ENABLED property item pointer.
 */
#define CCD_PREVIEW_ENABLED_ITEM        (CCD_PREVIEW_PROPERTY->items+1)

/** CCD_PREVIEW.ENABLED_WITH_HISTOGRAM  property item pointer.
 */
#define CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM        (CCD_PREVIEW_PROPERTY->items+2)

/** CCD_LOCAL_MODE property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_LOCAL_MODE_PROPERTY           (CCD_CONTEXT->ccd_local_mode_property)

/** CCD_LOCAL_MODE.DIR property item pointer.
 */
#define CCD_LOCAL_MODE_DIR_ITEM           (CCD_LOCAL_MODE_PROPERTY->items+0)

/** CCD_LOCAL_MODE.PREFIX property item pointer.
 */
#define CCD_LOCAL_MODE_PREFIX_ITEM        (CCD_LOCAL_MODE_PROPERTY->items+1)

/** CCD_LOCAL_MODE.OBJECT property item pointer.
 */
#define CCD_LOCAL_MODE_OBJECT_ITEM        (CCD_LOCAL_MODE_PROPERTY->items+2)

/** CCD_EXPOSURE property pointer, property is mandatory, property change request handler should set property items and state and call indigo_ccd_change_property().
 */
#define CCD_EXPOSURE_PROPERTY             (CCD_CONTEXT->ccd_exposure_property)

/** CCD_EXPOSURE.EXPOSURE property item pointer.
 */
#define CCD_EXPOSURE_ITEM                 (CCD_EXPOSURE_PROPERTY->items+0)

/** CCD_STREAMING property pointer, property is optional.
 */
#define CCD_STREAMING_PROPERTY             (CCD_CONTEXT->ccd_streaming_property)

/** CCD_STREAMING.EXPOSURE property item pointer.
 */
#define CCD_STREAMING_EXPOSURE_ITEM       (CCD_STREAMING_PROPERTY->items+0)

/** CCD_STREAMING.COUNT property item pointer.
 */
#define CCD_STREAMING_COUNT_ITEM          (CCD_STREAMING_PROPERTY->items+1)

/** CCD_ABORT property pointer, property is mandatory, property change request handler should set property items and state and call indigo_ccd_change_property().
 */
#define CCD_ABORT_EXPOSURE_PROPERTY       (CCD_CONTEXT->ccd_abort_exposure_property)

/** CCD_EXPOSURE.ABORT_EXPOSURE property item pointer.
 */
#define CCD_ABORT_EXPOSURE_ITEM           (CCD_ABORT_EXPOSURE_PROPERTY->items+0)

/** CCD_FRAME property pointer, property is mandatory, should be set read-only if subframe can't be set, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_FRAME_PROPERTY                (CCD_CONTEXT->ccd_frame_property)

/** CCD_FRAME.LEFT property item pointer.
 */
#define CCD_FRAME_LEFT_ITEM               (CCD_FRAME_PROPERTY->items+0)

/** CCD_FRAME.TOP property item pointer.
 */
#define CCD_FRAME_TOP_ITEM                (CCD_FRAME_PROPERTY->items+1)

/** CCD_FRAME.WIDTH property item pointer.
 */
#define CCD_FRAME_WIDTH_ITEM              (CCD_FRAME_PROPERTY->items+2)

/** CCD_FRAME.HEIGHT property item pointer.
 */
#define CCD_FRAME_HEIGHT_ITEM             (CCD_FRAME_PROPERTY->items+3)

/** CCD_FRAME.BITS_PER_PIXEL property item pointer.
 */
#define CCD_FRAME_BITS_PER_PIXEL_ITEM     (CCD_FRAME_PROPERTY->items+4)

/** CCD_BIN property pointer, property is mandatory, should be set read-only if binning can't be changed, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_BIN_PROPERTY                  (CCD_CONTEXT->ccd_bin_property)

/** CCD_BIN.HORIZONTAL property item pointer.
 */
#define CCD_BIN_HORIZONTAL_ITEM           (CCD_BIN_PROPERTY->items+0)

/** CCD_BIN.VERTICAL property item pointer.
 */
#define CCD_BIN_VERTICAL_ITEM             (CCD_BIN_PROPERTY->items+1)

/** CCD_MODE property pointer, property is mandatory.
 */
#define CCD_MODE_PROPERTY									(CCD_CONTEXT->ccd_mode_property)

/** CCD_MODE.MODE property item pointer.
 */
#define CCD_MODE_ITEM											(CCD_MODE_PROPERTY->items+0)

/** CCD_READ_MODE property pointer, property is optional.
 */
#define CCD_READ_MODE_PROPERTY						(CCD_CONTEXT->ccd_read_mode_property)

/** CCD_READ_MODE.HIGH_SPEED property item pointer.
 */
#define CCD_READ_MODE_HIGH_SPEED_ITEM			(CCD_READ_MODE_PROPERTY->items+0)

/** CCD_READ_MODE.LOW_NOISE property item pointer.
 */
#define CCD_READ_MODE_LOW_NOISE_ITEM			(CCD_READ_MODE_PROPERTY->items+1)

/** CCD_GAIN property pointer, property is optional, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_GAIN_PROPERTY                 (CCD_CONTEXT->ccd_gain_property)

/** CCD_GAIN.GAIN property item pointer.
 */
#define CCD_GAIN_ITEM                     (CCD_GAIN_PROPERTY->items+0)

/** CCD_EGAIN property pointer, property is optional, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_EGAIN_PROPERTY                 (CCD_CONTEXT->ccd_egain_property)

/** CCD_EGAIN.EGAIN property item pointer.
 */
#define CCD_EGAIN_ITEM                     (CCD_EGAIN_PROPERTY->items+0)

/** CCD_OFFSET property pointer, property is optional, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_OFFSET_PROPERTY               (CCD_CONTEXT->ccd_offset_property)

/** CCD_OFFSET.OFFSET property item pointer.
 */
#define CCD_OFFSET_ITEM										(CCD_OFFSET_PROPERTY->items+0)

/** CCD_GAMMA property pointer, property is optional, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_GAMMA_PROPERTY                (CCD_CONTEXT->ccd_gamma_property)

/** CCD_GAMMA.GAMMA property item pointer.
 */
#define CCD_GAMMA_ITEM                    (CCD_GAMMA_PROPERTY->items+0)

/** CCD_FRAME_TYPE property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_FRAME_TYPE_PROPERTY           (CCD_CONTEXT->ccd_frame_type_property)

/** CCD_FRAME_TYPE.LIGHT property item pointer
 */
#define CCD_FRAME_TYPE_LIGHT_ITEM         (CCD_FRAME_TYPE_PROPERTY->items+0)

/** CCD_FRAME_TYPE.BIAS property item pointer.
 */
#define CCD_FRAME_TYPE_BIAS_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+1)

/** CCD_FRAME_TYPE.DARK property item pointer.
 */
#define CCD_FRAME_TYPE_DARK_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+2)

/** CCD_FRAME_TYPE.FLAT property item pointer.
 */
#define CCD_FRAME_TYPE_FLAT_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+3)

/** CCD_FRAME_TYPE.DARKFLAT property item pointer.
 */
#define CCD_FRAME_TYPE_DARKFLAT_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+4)

/** CCD_IMAGE_FORMAT property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_IMAGE_FORMAT_PROPERTY         (CCD_CONTEXT->ccd_image_format_property)

/** CCD_IMAGE_FORMAT.FITS property item pointer.
 */
#define CCD_IMAGE_FORMAT_FITS_ITEM        (CCD_IMAGE_FORMAT_PROPERTY->items+0)

/** CCD_IMAGE_FORMAT.XISF property item pointer.
 */
#define CCD_IMAGE_FORMAT_XISF_ITEM        (CCD_IMAGE_FORMAT_PROPERTY->items+1)

/** CCD_IMAGE_FORMAT.RAW property item pointer.
 */
#define CCD_IMAGE_FORMAT_RAW_ITEM         (CCD_IMAGE_FORMAT_PROPERTY->items+2)

/** CCD_IMAGE_FORMAT.JPEG property item pointer.
 */
#define CCD_IMAGE_FORMAT_JPEG_ITEM        (CCD_IMAGE_FORMAT_PROPERTY->items+3)

/** CCD_IMAGE_FORMAT.TIFF property item pointer.
 */
#define CCD_IMAGE_FORMAT_TIFF_ITEM        (CCD_IMAGE_FORMAT_PROPERTY->items+4)

/** CCD_IMAGE_FORMAT.JPEG_AVI property item pointer.
 */
#define CCD_IMAGE_FORMAT_JPEG_AVI_ITEM    (CCD_IMAGE_FORMAT_PROPERTY->items+5)

/** CCD_IMAGE_FORMAT.RAW_SER property item pointer.
 */
#define CCD_IMAGE_FORMAT_RAW_SER_ITEM    (CCD_IMAGE_FORMAT_PROPERTY->items+6)

/** CCD_IMAGE_FORMAT.NATIVE property item pointer (DSLR only)
 */
#define CCD_IMAGE_FORMAT_NATIVE_ITEM      (CCD_IMAGE_FORMAT_PROPERTY->items+3)

/** CCD_IMAGE_FORMAT.NATIVE_AVI property item pointer. (DSLR only)
 */
#define CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM  (CCD_IMAGE_FORMAT_PROPERTY->items+4)



/** CCD_IMAGE_FILE property pointer, property is mandatory, read-only property.
 */
#define CCD_IMAGE_FILE_PROPERTY           (CCD_CONTEXT->ccd_image_file_property)

/** CCD_IMAGE_FILE.FILE property item pointer.
 */
#define CCD_IMAGE_FILE_ITEM               (CCD_IMAGE_FILE_PROPERTY->items+0)

/** CCD_IMAGE property pointer, property is mandatory, read-only property.
 */
#define CCD_IMAGE_PROPERTY                (CCD_CONTEXT->ccd_image_property)

/** CCD_IMAGE.IMAGE property item pointer.
 */
#define CCD_IMAGE_ITEM                    (CCD_IMAGE_PROPERTY->items+0)

/** CCD_PREVIEW_IMAGE property pointer, property is mandatory, read-only property.
 */
#define CCD_PREVIEW_IMAGE_PROPERTY        (CCD_CONTEXT->ccd_preview_image_property)

/** CCD_PREVIEW_IMAGE.IMAGE property item pointer.
 */
#define CCD_PREVIEW_IMAGE_ITEM            (CCD_PREVIEW_IMAGE_PROPERTY->items+0)

/** CCD_PREVIEW_IMAGE property pointer, property is mandatory, read-only property.
 */
#define CCD_PREVIEW_HISTOGRAM_PROPERTY        (CCD_CONTEXT->ccd_preview_histogram_property)

/** CCD_PREVIEW_HISTOGRAM.IMAGE property item pointer.
 */
#define CCD_PREVIEW_HISTOGRAM_ITEM            (CCD_PREVIEW_HISTOGRAM_PROPERTY->items+0)

/** CCD_TEMPERATURE property pointer, property change request should be fully handled by device driver.
 */
#define CCD_TEMPERATURE_PROPERTY          (CCD_CONTEXT->ccd_temperature_property)

/** CCD_TEMPERATURE.TEMPERATURE property item pointer, property is optional.
 */
#define CCD_TEMPERATURE_ITEM              (CCD_TEMPERATURE_PROPERTY->items+0)

/** CCD_COOLER property pointer, property is optional, property change request should be fully handled by device driver.
 */
#define CCD_COOLER_PROPERTY               (CCD_CONTEXT->ccd_cooler_property)

/** CCD_COOLER.ON property item pointer.
 */
#define CCD_COOLER_ON_ITEM                (CCD_COOLER_PROPERTY->items+0)

/** CCD_COOLER.OFF property item pointer.
 */
#define CCD_COOLER_OFF_ITEM               (CCD_COOLER_PROPERTY->items+1)

/** CCD_COOLER_POWER property pointer, property is optional, read-only property, should be set by device driver.
 */
#define CCD_COOLER_POWER_PROPERTY         (CCD_CONTEXT->ccd_cooler_power_property)

/** CCD_COOLER_POWER.POWER property item pointer.
 */
#define CCD_COOLER_POWER_ITEM             (CCD_COOLER_POWER_PROPERTY->items+0)

/** CCD_FITS_HEADERS property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_FITS_HEADERS_PROPERTY         (CCD_CONTEXT->ccd_fits_headers)

/** CCD_SET_FITS_HEADER property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_SET_FITS_HEADER_PROPERTY      (CCD_CONTEXT->ccd_set_fits_header)

/** CCD_SET_FITS_HEADER.NAME property item pointer.
 */
#define CCD_SET_FITS_HEADER_NAME_ITEM      (CCD_SET_FITS_HEADER_PROPERTY->items+0)

/** CCD_SET_FITS_HEADER.VALUE property item pointer.
 */
#define CCD_SET_FITS_HEADER_VALUE_ITEM      (CCD_SET_FITS_HEADER_PROPERTY->items+1)

/** CCD_REMOVE_FITS_HEADER property pointer, property is mandatory, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_REMOVE_FITS_HEADER_PROPERTY   (CCD_CONTEXT->ccd_remove_fits_header)

/** CCD_REMOVE_FITS_HEADER.NAME property item pointer.
 */
#define CCD_REMOVE_FITS_HEADER_NAME_ITEM      (CCD_REMOVE_FITS_HEADER_PROPERTY->items+0)

/** FITS logical record length used for header size rounding.
 */
#define FITS_LOGICAL_RECORD_LENGTH	2880

/** Number of logical records allocated for FITS header.
 */
#define MAX_FITS_LOGICAL_RECORDS	3

/** FITS header size, it should be added to image buffer size, raw data should start at this offset.
 */
#define FITS_HEADER_SIZE  (MAX_FITS_LOGICAL_RECORDS * FITS_LOGICAL_RECORD_LENGTH)

/** CCD_JPEG_SETTINGS property pointer, property is mandatory, read-write property, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_JPEG_SETTINGS_PROPERTY         (CCD_CONTEXT->ccd_jpeg_settings)

/** CCD_JPEG_SETTINGS.QUALITY property item pointer.
 */
#define CCD_JPEG_SETTINGS_QUALITY_ITEM     (CCD_JPEG_SETTINGS_PROPERTY->items+0)

/** CCD_JPEG_SETTINGS.TARGET_BACKGROUND property item pointer.
 */
#define CCD_JPEG_SETTINGS_TARGET_BACKGROUND_ITEM     (CCD_JPEG_SETTINGS_PROPERTY->items+1)

/** CCD_JPEG_SETTINGS.CLIPPING_POINT property item pointer.
 */
#define CCD_JPEG_SETTINGS_CLIPPING_POINT_ITEM     (CCD_JPEG_SETTINGS_PROPERTY->items+2)

/** CCD_JPEG_STRETCH_PRESETS property pointer, property is mandatory, read-write property, property change request is fully handled by indigo_ccd_change_property().
 */
#define CCD_JPEG_STRETCH_PRESETS_PROPERTY         (CCD_CONTEXT->ccd_jpeg_stretch_presets)

/** CCD_JPEG_STRETCH_PRESETS.SLIGHT property item pointer.
 */
#define  CCD_JPEG_STRETCH_PRESETS_SLIGHT_ITEM      (CCD_JPEG_STRETCH_PRESETS_PROPERTY->items+0)

/** CCD_JPEG_STRETCH_PRESETS.MODERATE property item pointer.
 */
#define  CCD_JPEG_STRETCH_PRESETS_MODERATE_ITEM      (CCD_JPEG_STRETCH_PRESETS_PROPERTY->items+1)

/** CCD_JPEG_STRETCH_PRESETS.NORMAL property item pointer.
 */
#define  CCD_JPEG_STRETCH_PRESETS_NORMAL_ITEM      (CCD_JPEG_STRETCH_PRESETS_PROPERTY->items+2)

/** CCD_JPEG_STRETCH_PRESETS.HARD property item pointer.
 */
#define  CCD_JPEG_STRETCH_PRESETS_HARD_ITEM      (CCD_JPEG_STRETCH_PRESETS_PROPERTY->items+3)

/** CCD_RBI_FLUSH property pointer.
 */
#define CCD_RBI_FLUSH_PROPERTY          (CCD_CONTEXT->ccd_rbi_flush_property)

/** CCD_RBI_FLUSH.EXPOSURE property item pointer.
 */
#define CCD_RBI_FLUSH_EXPOSURE_ITEM     (CCD_RBI_FLUSH_PROPERTY->items + 0)
/** CCD_RBI_FLUSH.COUNT property item pointer.
 */
#define CCD_RBI_FLUSH_COUNT_ITEM        (CCD_RBI_FLUSH_PROPERTY->items + 1)

/** CCD_RBI_FLUSH_ENABLE property pointer.
 */
#define CCD_RBI_FLUSH_ENABLE_PROPERTY   (CCD_CONTEXT->ccd_rbi_flush_enable_property)

/** CCD_RBI_FLUSH_ENABLE.ENABLE property item pointer.
*/
#define CCD_RBI_FLUSH_ENABLED_ITEM      (CCD_RBI_FLUSH_ENABLE_PROPERTY->items + 0)

/** CCD_RBI_FLUSH_ENABLE.DISABLE property item pointer.
 */
#define CCD_RBI_FLUSH_DISABLED_ITEM     (CCD_RBI_FLUSH_ENABLE_PROPERTY->items + 1)


/** CCD device context structure.
 */
typedef struct {
	indigo_device_context device_context;         ///< device context base
	bool countdown_canceled;									///< countdown canceled
	bool countdown_enabled;									///< countdown enabled
	double countdown_endtime;									///< countdown end time
	indigo_timer *countdown_timer;								///< countdown timer
	void *preview_image;													///< preview image buffer
	unsigned long preview_image_size;							///< preview image buffer size
	void *preview_histogram;											///< preview histogram buffer
	unsigned long preview_histogram_size;					///< preview histogram buffer size
	void *video_stream;														///< video stream control structure
	indigo_property *ccd_info_property;           ///< CCD_INFO property pointer
	indigo_property *ccd_lens_property;						///< CCD_LENS property pointer
	indigo_property *ccd_upload_mode_property;    ///< CCD_UPLOAD_MODE property pointer
	indigo_property *ccd_preview_property;				///< CCD_PREVIEW property pointer
	indigo_property *ccd_local_mode_property;     ///< CCD_LOCAL_MODE property pointer
	indigo_property *ccd_mode_property;	          ///< CCD_MODE property pointer
	indigo_property *ccd_read_mode_property;	  	///< CCD_READ_MODE property pointer
	indigo_property *ccd_exposure_property;       ///< CCD_EXPOSURE property pointer
	indigo_property *ccd_streaming_property;      ///< CCD_STREAMING property pointer
	indigo_property *ccd_abort_exposure_property; ///< CCD_ABORT_EXPOSURE property pointer
	indigo_property *ccd_frame_property;          ///< CCD_FRAME property pointer
	indigo_property *ccd_bin_property;            ///< CCD_BIN property pointer
	indigo_property *ccd_offset_property;         ///< CCD_OFFSET property pointer
	indigo_property *ccd_gain_property;           ///< CCD_GAIN property pointer
	indigo_property *ccd_egain_property;          ///< CCD_EGAIN property pointer
	indigo_property *ccd_gamma_property;          ///< CCD_GAMMA property pointer
	indigo_property *ccd_frame_type_property;     ///< CCD_FRAME_TYPE property pointer
	indigo_property *ccd_image_format_property;   ///< CCD_IMAGE_FORMAT property pointer
	indigo_property *ccd_image_property;          ///< CCD_IMAGE property pointer
	indigo_property *ccd_preview_image_property;  ///< CCD_PREVIEW_IMAGE property pointer
	indigo_property *ccd_preview_histogram_property;  ///< CCD_PREVIEW_HISTOGRAM property pointer
	indigo_property *ccd_image_file_property;     ///< CCD_IMAGE_FILE property pointer
	indigo_property *ccd_temperature_property;    ///< CCD_TEMPERATURE property pointer
	indigo_property *ccd_cooler_property;         ///< CCD_COOLER property pointer
	indigo_property *ccd_cooler_power_property;   ///< CCD_COOLER_POWER property pointer
	indigo_property *ccd_fits_headers;						///< CCD_FITS_HEADERS property pointer
	indigo_property *ccd_set_fits_header;					///< CCD_SET_FITS_HEADER property pointer
	indigo_property *ccd_remove_fits_header;			///< CCD_REMOVE_FITS_HEADER property pointer
	indigo_property *ccd_jpeg_settings;						///< CCD_JPEG_SETTINGS property pointer
	indigo_property *ccd_jpeg_stretch_presets;				///< CCD_JPEG_STRETCH_PRESETS property pointer
	indigo_property *ccd_rbi_flush_enable_property; ///< CCD_RBI_FLUSH_ENABLE property pointer
	indigo_property *ccd_rbi_flush_property;			///< CCD_RBI_FLUSH property pointer
} indigo_ccd_context;

/** Suspend countdown.
 */
extern void indigo_ccd_suspend_countdown(indigo_device *device);

/** Resume countdown.
 */
extern void indigo_ccd_resume_countdown(indigo_device *device);

/** Set shortest exposure in case of a bias frame, otherwise does nothing.
    The intended use is in exposure propery handling.
 */
extern void indigo_use_shortest_exposure_if_bias(indigo_device *device);

/** Attach callback function.
 */
extern indigo_result indigo_ccd_attach(indigo_device *device, const char* driver_name, unsigned version);
/** Enumerate properties callback function.
 */
extern indigo_result indigo_ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
/** Change property callback function.
 */
extern indigo_result indigo_ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
/** Detach callback function.
 */
extern indigo_result indigo_ccd_detach(indigo_device *device);

/** Convert RAW data to JPEG
 */
extern void indigo_raw_to_jpeg(indigo_device *device, void *data_in, int frame_width, int frame_height, int bpp, const char *bayerpat, void **data_out, unsigned long *size_out, void **histogram_data, unsigned long *histogram_size, double B, double C);

/** Process raw image in image buffer (starting on data + FITS_HEADER_SIZE offset).
 */
extern void indigo_process_image(indigo_device *device, void *data, int frame_width, int frame_height, int bpp, bool little_endian, bool byte_order_rgb, indigo_fits_keyword *keywords, bool streaming);

/** Process DSLR image in image buffer (starting on data).
 */
extern void indigo_process_dslr_image(indigo_device *device, void *data, int blobsize, const char *suffix, bool streaming);

/** Process DSLR preview image in image buffer (starting on data).
 */
extern void indigo_process_dslr_preview_image(indigo_device *device, void *data, int blobsize);

/** Finalize video stream.
 */
extern void indigo_finalize_video_stream(indigo_device *device);

/** Finalize DSLR video stream.
 */
extern void indigo_finalize_dslr_video_stream(indigo_device *device);

/** Set alert state on dependent properties.
 */
extern indigo_result indigo_ccd_failure_cleanup(indigo_device *device);

/** Set corect states to related proeprties upon aborting exposure.
 */
indigo_result indigo_ccd_abort_exposure_cleanup(indigo_device *device);

/** Set FITS header
 */
extern indigo_result indigo_set_fits_header(indigo_client *client, char *device, char *name, char *format, ...);

/** Remove FITS header
 */
extern indigo_result indigo_remove_fits_header(indigo_client *client, char *device, char *name);



#ifdef __cplusplus
}
#endif

#endif /* indigo_ccd_h */
