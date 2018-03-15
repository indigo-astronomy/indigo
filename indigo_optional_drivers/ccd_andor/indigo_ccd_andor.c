 // Copyright (c) 2018 Rumen G. Bogdanovski
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
 // 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

 /** INDIGO CCD Andor driver
  \file indigo_ccd_andor.c
  */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME	"indigo_ccd_andor"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_ccd_andor.h"

#define WIDTH               1600
#define HEIGHT              1200
#define TEMP_UPDATE         2.0
#define STARS               100
#define SIN									0.8414709848078965
#define COS									0.5403023058681398

// gp_bits is used as boolean
#define is_connected                     gp_bits

#define PRIVATE_DATA						((andor_private_data *)device->private_data)
#define DSLR_PROGRAM_PROPERTY				PRIVATE_DATA->dslr_program_property

#define CAP_GET_TEMPERATURE (PRIVATE_DATA->caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURE)
#define CAP_GET_TEMPERATURE_RANGE (PRIVATE_DATA->caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURERANGE)
#define CAP_SET_TEMPERATURE (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_TEMPERATURE)
#define CAP_GET_TEMPERATURE_DURING_ACQUISITION (PRIVATE_DATA->caps.ulFeatures & AC_FEATURES_TEMPERATUREDURINGACQUISITION)

typedef struct {
	long handle;
	int index;
	indigo_property *dslr_program_property;

	unsigned char *buffer;
	long buffer_size;
	int adc_channels;
	int bit_depths[10];
	AndorCapabilities caps;
	bool no_check_temperature;
	float target_temperature, current_temperature, cooler_power;
	indigo_timer *exposure_timer, *temperature_timer;
	//double ra_offset, dec_offset;
} andor_private_data;

/* To avoid exposure failue when many cameras are present global mutex is required */
static pthread_mutex_t driver_mutex = PTHREAD_MUTEX_INITIALIZER;

static void get_camera_type(unsigned long type, char *name,  size_t size){
	switch (type) {
	case AC_CAMERATYPE_PDA:
		strncpy(name,"Andor PDA", size);
		return;
	case AC_CAMERATYPE_IXON:
		strncpy(name,"Andor iXon", size);
		return;
	case AC_CAMERATYPE_ICCD:
		strncpy(name,"Andor iCCD", size);
		return;
	case AC_CAMERATYPE_EMCCD:
		strncpy(name,"Andor EMCCD", size);
		return;
	case AC_CAMERATYPE_CCD:
		strncpy(name,"Andor PDA", size);
		return;
	case AC_CAMERATYPE_ISTAR:
		strncpy(name,"Andor iStar", size);
		return;
	case AC_CAMERATYPE_VIDEO:
		strncpy(name,"Non Andor", size);
		return;
	case AC_CAMERATYPE_IDUS:
		strncpy(name,"Andor iDus", size);
		return;
	case AC_CAMERATYPE_NEWTON:
		strncpy(name,"Andor Newton", size);
		return;
	case AC_CAMERATYPE_SURCAM:
		strncpy(name,"Andor Surcam", size);
		return;
	case AC_CAMERATYPE_USBICCD:
		strncpy(name,"Andor USB iCCD", size);
		return;
	case AC_CAMERATYPE_LUCA:
		strncpy(name,"Andor Luca", size);
		return;
	case AC_CAMERATYPE_IKON:
		strncpy(name,"Andor iKon", size);
		return;
	case AC_CAMERATYPE_INGAAS:
		strncpy(name,"Andor InGaAs", size);
		return;
	case AC_CAMERATYPE_IVAC:
		strncpy(name,"Andor iVac", size);
		return;
	case AC_CAMERATYPE_CLARA:
		strncpy(name,"Andor Clara", size);
		return;
	case AC_CAMERATYPE_USBISTAR:
		strncpy(name,"Andor USB iStar", size);
		return;
	case AC_CAMERATYPE_IXONULTRA:
		strncpy(name,"Andor iXon Ultra", size);
		return;
	case AC_CAMERATYPE_IVAC_CCD:
		strncpy(name,"Andor iVac CCD", size);
		return;
	case AC_CAMERATYPE_IKONXL:
		strncpy(name,"Andor iKon XL", size);
		return;
	case AC_CAMERATYPE_ISTAR_SCMOS:
		strncpy(name,"Andor iStar sCMOS", size);
		return;
	case AC_CAMERATYPE_IKONLR:
		strncpy(name,"Andor iKon LR", size);
		return;
	default:
		strncpy(name,"Andor", size);
		return;
	}
}

static bool use_camera(indigo_device *device) {
	at_32 res = SetCurrentCamera(PRIVATE_DATA->handle);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCurrentCamera(%d): Invalid camera handle.", PRIVATE_DATA->handle);
		return false;
	}
	return true;
}

static bool andor_start_exposure(indigo_device *device, double exposure, bool dark, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	unsigned int res;

	pthread_mutex_lock(&driver_mutex);
	if (!use_camera(device)) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	//Set Read Mode to --Image--
	res = SetReadMode(4);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetReadMode(4) = %d", res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	//Set Acquisition mode to --Single scan--
	SetAcquisitionMode(1);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetAcquisitionMode(1) = %d", res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	//Set initial exposure time
	SetExposureTime(exposure);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetExposureTime(%f) = %d", exposure, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	//Initialize Shutter
	if(dark) {
		res = SetShutter(1,2,50,50);
	} else {
		res = SetShutter(1,0,50,50);
	}
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetShutter() = %d", res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	//Setup Image dimensions
	res = SetImage(bin_x, bin_y, offset_x+1, offset_x+frame_width, offset_y+1, offset_y+frame_height);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetImage(%d, %d, %d, %d, %d, %d) = %d", bin_x, bin_y, offset_x+1, offset_x+frame_width, offset_y+1, offset_y+frame_height, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	res = StartAcquisition();
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "StartAcquisition() = %d", res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	pthread_mutex_unlock(&driver_mutex);

	return true;
}

static bool andor_read_pixels(indigo_device *device) {
	long res;
	long wait_cycles = 4000;

	pthread_mutex_lock(&driver_mutex);
	if (!use_camera(device)) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	/* Wait until acquisition finished */
	int status;
	do {
		GetStatus(&status);
		if (status != DRV_ACQUIRING) break;
		usleep(10000);
		wait_cycles--;
	} while (wait_cycles);

	if (wait_cycles == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure Failed!");
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	long num_pixels = (long)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value) *
	                  (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value);

	unsigned char *image = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;

	if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value > 16) {
		res = GetAcquiredData((uint32_t *)image, num_pixels);
		if (res != DRV_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAcquiredData() = %d", res);
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
	} else {
		res = GetAcquiredData16((uint16_t *)image, num_pixels);
		if (res != DRV_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAcquiredData16() = %d", res);
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
	}
	pthread_mutex_unlock(&driver_mutex);
	return true;
}

static void exposure_timer_callback(indigo_device *device) {
	unsigned char *frame_buffer;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	PRIVATE_DATA->exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (andor_read_pixels(device)) {
			frame_buffer = PRIVATE_DATA->buffer;

			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, frame_buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value),
			                    (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA ->no_check_temperature = false;
}

// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->no_check_temperature = true;
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}

static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;

	if (!CAP_GET_TEMPERATURE_DURING_ACQUISITION) PRIVATE_DATA->no_check_temperature = true;

	ok = andor_start_exposure(device,
	                         CCD_EXPOSURE_ITEM->number.target,
	                         CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value,
	                         CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value,
	                         CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
	                         CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value
	);

	if (ok) {
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		} else {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}

		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;

		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (CCD_EXPOSURE_ITEM->number.target > 4) {
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback);
		} else {
			PRIVATE_DATA->no_check_temperature = true;
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
		}
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	return false;
}


static bool andor_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&driver_mutex);

	if (!use_camera(device)) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	long ret = AbortAcquisition();

	pthread_mutex_unlock(&driver_mutex);
	if ((ret == DRV_SUCCESS) || (ret == DRV_IDLE)) return true;
	else return false;
}


// -------------------------------------------------------------------------------- INDIGO CCD device implementation
static void ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&driver_mutex);
	if (!use_camera(device)) {
		pthread_mutex_unlock(&driver_mutex);
		return;
	}
	if (!PRIVATE_DATA->no_check_temperature && CAP_GET_TEMPERATURE) {
		long res = GetTemperatureF(&PRIVATE_DATA->current_temperature);

		if (CCD_COOLER_ON_ITEM->sw.value)
			CCD_TEMPERATURE_PROPERTY->state = (res != DRV_TEMP_STABILIZED) ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
		else
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;

		CCD_TEMPERATURE_ITEM->number.value = round(PRIVATE_DATA->current_temperature * 10) / 10.;
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	pthread_mutex_unlock(&driver_mutex);
	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DSLR_PROGRAM_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_PROGRAM_PROPERTY_NAME, "Advanced", "Program mode", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
		indigo_init_switch_item(DSLR_PROGRAM_PROPERTY->items + 0, "M", "Manual", true);
		INFO_PROPERTY->count = 7;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_ccd_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(DSLR_PROGRAM_PROPERTY, property))
				indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);
		}
	}
	return result;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (!device->is_connected) { /* Do not double open device */
				if (indigo_try_global_lock(device) != INDIGO_OK) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
					indigo_update_property(device, CONNECTION_PROPERTY, "Device is locked");
					return INDIGO_OK;
				}

				indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);

				pthread_mutex_lock(&driver_mutex);
				if(use_camera(device) == false) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					pthread_mutex_unlock(&driver_mutex);
					return INDIGO_OK;
				}

				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				int res = GetHeadModel(INFO_DEVICE_MODEL_ITEM->text.value);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetHeadModel() error: %d", res);
					INFO_DEVICE_MODEL_ITEM->text.value[0] = '\0';
				}
				unsigned int fw_ver, fw_build, dummy;
				GetHardwareVersion(&dummy, &dummy, &dummy, &dummy, &fw_ver, &fw_build);
				snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%d-%d", fw_ver, fw_build);

				int serial_num;
				GetCameraSerialNumber(&serial_num);
				snprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE, "CCD-%d", serial_num);

				indigo_update_property(device, INFO_PROPERTY, NULL);

				int width, height;
				GetDetector(&width, &height);
				CCD_INFO_WIDTH_ITEM->number.value = width;
				CCD_INFO_HEIGHT_ITEM->number.value = height;
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
				if (PRIVATE_DATA->buffer == NULL) {
					PRIVATE_DATA->buffer_size = width * height * 4 + FITS_HEADER_SIZE;
					PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
				}

				float x_size, y_size;
				GetPixelSize(&x_size, &y_size);
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = x_size;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = y_size;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

				int max_bin;
				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				// 4 is Image mode, 0 is horizontal binning
				GetMaximumBinning(4, 0, &max_bin);
				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = max_bin;

				// 4 is Image mode, 1 is vertical binning
				GetMaximumBinning(4, 1, &max_bin);
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;
				CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = max_bin;

				if (CAP_GET_TEMPERATURE) {
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = 0;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
				}
				if (CAP_SET_TEMPERATURE) {
					CCD_COOLER_PROPERTY->hidden = false;
					indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
					int temp_min = -100, temp_max = 20;
					if (CAP_GET_TEMPERATURE_RANGE) GetTemperatureRange(&temp_min, &temp_max);
					CCD_TEMPERATURE_ITEM->number.max = (double)temp_max;
					CCD_TEMPERATURE_ITEM->number.min = (double)temp_min;
					PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = (double)temp_max;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				}

				/* Find available BPPs and use max */
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 0;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 128;
				int max_bpp_channel = 0;
				GetNumberADChannels(&PRIVATE_DATA->adc_channels);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADC Channels: %d", PRIVATE_DATA->adc_channels);
				for (int i = 0; i < PRIVATE_DATA->adc_channels; i++) {
					GetBitDepth(i, &PRIVATE_DATA->bit_depths[i]);
					if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min >= PRIVATE_DATA->bit_depths[i]) {
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = PRIVATE_DATA->bit_depths[i];
					}
					if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max <= PRIVATE_DATA->bit_depths[i]) {
						CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->bit_depths[i];
						CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = PRIVATE_DATA->bit_depths[i];
						max_bpp_channel = i;
					}
				}
				SetADChannel(max_bpp_channel);

				CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_MODE_PROPERTY->count = 4;
				char name[32];
				sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value, (int)CCD_INFO_HEIGHT_ITEM->number.value);
				indigo_init_switch_item(CCD_MODE_ITEM+0, "BIN_1x1", name, true);
				sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value/2, (int)CCD_INFO_HEIGHT_ITEM->number.value/2);
				indigo_init_switch_item(CCD_MODE_ITEM+1, "BIN_2x2", name, false);
				sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value/4, (int)CCD_INFO_HEIGHT_ITEM->number.value/4);
				indigo_init_switch_item(CCD_MODE_ITEM+2, "BIN_4x4", name, false);
				sprintf(name, "RAW %dx%d", (int)CCD_INFO_WIDTH_ITEM->number.value/8, (int)CCD_INFO_HEIGHT_ITEM->number.value/8);
				indigo_init_switch_item(CCD_MODE_ITEM+3, "BIN_8x8", name, false);

				pthread_mutex_unlock(&driver_mutex);
				PRIVATE_DATA->temperature_timer = indigo_set_timer(device, TEMP_UPDATE, ccd_temperature_callback);
				device->is_connected = true;
			}
		} else {
			if (device->is_connected) {  /* Do not double close device */
				indigo_global_unlock(device);
				indigo_delete_property(device, DSLR_PROGRAM_PROPERTY, NULL);
				indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
				if (PRIVATE_DATA->buffer != NULL) {
					free(PRIVATE_DATA->buffer);
					PRIVATE_DATA->buffer = NULL;
				}
				device->is_connected = false;
			}
		}
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (IS_CONNECTED) {
			handle_exposure_property(device, property);
		}
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
			andor_abort_exposure(device);
		}
		PRIVATE_DATA->no_check_temperature = false;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_COOLER
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			pthread_mutex_lock(&driver_mutex);
			if (!use_camera(device)) {
				pthread_mutex_unlock(&driver_mutex);
				return INDIGO_OK;
			}
			long res;
			if (CCD_COOLER_ON_ITEM->sw.value) {
				res = CoolerON();
				if(res == DRV_SUCCESS) {
					SetTemperature((int)PRIVATE_DATA->target_temperature);
					CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
					CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
				} else {
					CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CoolerON() error: %d", res);
				}
			} else {
				res = CoolerOFF();
				if(res == DRV_SUCCESS) {
					CCD_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
					CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
				} else {
					CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CoolerOFF() error: %d", res);
				}
			}
			pthread_mutex_unlock(&driver_mutex);
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_TEMPERATURE
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			pthread_mutex_lock(&driver_mutex);
			if (!use_camera(device)) {
				pthread_mutex_unlock(&driver_mutex);
				return INDIGO_OK;
			}
			long res = SetTemperature((int)PRIVATE_DATA->target_temperature);
			if(res == DRV_SUCCESS) {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetTemperature() error: %d", res);
			}
			pthread_mutex_unlock(&driver_mutex);

			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target temperature %g", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		// ------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value <= 8.0) {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8.0;
		} else if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value <= 16.0) {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16.0;
		} else {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 32.0;
		}

		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;

		for (int i = 0; i < PRIVATE_DATA->adc_channels; i++) {
			if(PRIVATE_DATA->bit_depths[i] == CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value) {
				pthread_mutex_lock(&driver_mutex);
				if (!use_camera(device)) {
					pthread_mutex_unlock(&driver_mutex);
					return INDIGO_OK;
				}
				uint32_t res = SetADChannel(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetADChannel(%d) error: %d", i, PRIVATE_DATA->bit_depths[i]);
					CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bit depth: %d (Channel %d)", PRIVATE_DATA->bit_depths[i], i);
				}
				pthread_mutex_unlock(&driver_mutex);
				break;
			}
		}
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		// --------------------------------------------------------------------------------
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_device_disconnect(NULL, device->name);

		indigo_release_property(DSLR_PROGRAM_PROPERTY);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// --------------------------------------------------------------------------------

#define MAX_DEVICES 8
static indigo_device *devices[MAX_DEVICES] = {NULL};
at_32 device_num = 0;

indigo_result indigo_ccd_andor(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device imager_camera_template = INDIGO_DEVICE_INITIALIZER(
		CCD_ANDOR_CAMERA_NAME,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	at_32 res;
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, CCD_ANDOR_CAMERA_NAME, __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch(action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;

			const char default_path[] = "/usr/local/etc/andor";
			char *andor_path = getenv("ANDOR_SDK_PATH");
			if (andor_path == NULL) andor_path = (char *)default_path;
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ANDOR_SDK_PATH = %s", andor_path);

			char sdk_version[255];
			GetVersionInfo(AT_SDKVersion, sdk_version, sizeof(sdk_version));
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Andor SDK v.%s", sdk_version);

			res = GetAvailableCameras(&device_num);
			if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAvailableCameras() error: %d", res);
			else {
				if (device_num > 0) INDIGO_DRIVER_LOG(DRIVER_NAME, "Detected %d Andor camera(s). Initializing...", device_num);
				else INDIGO_DRIVER_LOG(DRIVER_NAME, "No Andor cameras detected");
			}

			for (int i = 0; i < device_num; i++) {
				andor_private_data *private_data = malloc(sizeof(andor_private_data));
				assert(private_data != NULL);
				memset(private_data, 0, sizeof(andor_private_data));
				indigo_device *device = malloc(sizeof(indigo_device));
				assert(device != NULL);
				memcpy(device, &imager_camera_template, sizeof(indigo_device));

				at_32 handle;
				pthread_mutex_lock(&driver_mutex);
				res = GetCameraHandle(i, &handle);
				if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetCameraHandle() error: %d", res);

				res = SetCurrentCamera(handle);
				if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCurrentCamera() error: %d", res);

				res = Initialize(andor_path);
				if(res != DRV_SUCCESS) {
					switch (res) {
					case DRV_ERROR_NOCAMERA:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: No camera found.");
						break;
					case DRV_USBERROR:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to detect USB device or not USB2.0");
						break;
					case DRV_ERROR_PAGELOCK:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to acquire lock on requested memory.");
						break;
					case DRV_INIERROR:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load DETECTOR.INI.");
						break;
					case DRV_VXDNOTINSTALLED:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: VxD not loaded.");
						break;
					case DRV_COFERROR:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load *.COF");
						break;
					case DRV_FLEXERROR:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load *.RBF");
						break;
					case DRV_ERROR_FILELOAD:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialization error: Unable to load “*.COF” or “*.RBF” files.");
						break;
					default:
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialisation error: %d", res);
					}
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR_SDK_PATH may not be not valid.");
					break;
				}

				private_data->caps.ulSize = sizeof(AndorCapabilities);
				GetCapabilities(&private_data->caps);

				pthread_mutex_unlock(&driver_mutex);

				char camera_type[32];
				get_camera_type(private_data->caps.ulCameraType, camera_type, sizeof(camera_type));
				snprintf(device->name, sizeof(device->name), "%s #%d", camera_type, i);
				private_data->index = i;
				private_data->handle = handle;
				device->private_data = private_data;
				indigo_attach_device(device);
				devices[i] = device;
			}
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			for (int i = 0; i < device_num; i++) {
				if (devices[i] != NULL) {
					andor_private_data *private_data = devices[i]->private_data;
					pthread_mutex_lock(&driver_mutex);
					use_camera(devices[i]);
					ShutDown();
					pthread_mutex_unlock(&driver_mutex);
					indigo_detach_device(devices[i]);
					free(devices[i]);
					devices[i] = NULL;

					if (private_data != NULL) {
						pthread_mutex_destroy(&driver_mutex);
						free(private_data);
					}
				}
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
