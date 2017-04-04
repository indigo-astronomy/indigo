// Copyright (c) 2017 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen Bogdanovski <rumen@skyarchive.org>


/** INDIGO CCD SBIG driver
 \file indigo_ccd_sbig.c
 */

//#define ETHERNET_DRIVER

#define DRIVER_VERSION 0x0001

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <pthread.h>
#include <sys/time.h>

#include <libsbig/sbigudrv.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#include <dlfcn.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#ifdef ETHERNET_DRIVER
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif /* ETHERNET_DRIVER */

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

#define RELAY_NORTH        0x01     /* Guide relay bits */
#define RELAY_SOUTH        0x02
#define RELAY_WEST         0x04
#define RELAY_EAST         0x08

#define RELAY_MAX_PULSE    5000     /* 50s max pulse */


#define MAX_CCD_TEMP         45     /* Max CCD temperature */
#define MIN_CCD_TEMP       (-55)    /* Min CCD temperature */
#define MAX_X_BIN            16     /* Max Horizontal binning */
#define MAX_Y_BIN            16     /* Max Vertical binning */

#define DEFAULT_BPP          16     /* Default bits per pixel */

#define MIN_N_FLUSHES         0     /* Min number of array flushes before exposure */
#define MAX_N_FLUSHES        16     /* Max number of array flushes before exposure */
#define DEFAULT_N_FLUSHES     1     /* Default number of array flushes before exposure */

#define MIN_NIR_FLOOD         0     /* Min seconds to flood the frame with NIR light */
#define MAX_NIR_FLOOD        16     /* Max seconds to flood the frame with NIR light */
#define DEFAULT_NIR_FLOOD     3     /* Default seconds to flood the frame with NIR light */

#define MIN_FLUSH_COUNT       1     /* Min flushes after flood */
#define MAX_FLUSH_COUNT      10     /* Max flushes after flood */
#define DEFAULT_FLUSH_COUNT   2     /* Default flushes after flood */

#define MAX_PATH            255     /* Maximal Path Length */

#define TEMP_THRESHOLD     0.15
#define TEMP_CHECK_TIME       3     /* Time between teperature checks (seconds) */

#include "indigo_driver_xml.h"
#include "indigo_ccd_sbig.h"

#define SBIG_VENDOR_ID             0x0d97

#define MAX_MODES                  32

#define PRIVATE_DATA               ((sbig_private_data *)device->private_data)
#define PRIMARY_CCD                (device == PRIVATE_DATA->primary_ccd)

#define SBIG_ADVANCED_GROUP              "Advanced"

/*
#define FLI_NFLUSHES_PROPERTY           (PRIVATE_DATA->fli_nflushes_property)
#define FLI_NFLUSHES_PROPERTY_ITEM      (FLI_NFLUSHES_PROPERTY->items + 0)
*/

#undef INDIGO_DEBUG_DRIVER
#define INDIGO_DEBUG_DRIVER(c) c


// -------------------------------------------------------------------------------- SBIG USB interface implementation

#define ms2s(s)      ((s) / 1000.0)
#define s2ms(ms)     ((ms) * 1000)
#define m2um(m)      ((m) * 1e6)  /* meters to umeters */

typedef struct {
	long ul_x, ul_y, lr_x, lr_y;
} image_area;

typedef struct {
	long bin_x, bin_y;
	long width, height;
	int bpp;
} cframe_params;

typedef struct {
	bool is_usb;
	SBIG_DEVICE_TYPE usb_id;
	unsigned long ip_address;
	void *primary_ccd;
	short driver_handle;
	char dev_name[MAX_PATH];
	bool rbi_flood_supported;
	ushort relay_map;
	bool abort_flag;
	int count_open;
	indigo_timer *exposure_timer, *temperature_timer;
	indigo_timer *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	double cooler_power;
	unsigned char *buffer;
	long int buffer_size;

	image_area imager_ccd_area;
	image_area guider_ccd_area;

	GetCCDInfoResults0 imager_basic_info;
	GetCCDInfoResults0 guider_basic_info;

	GetCCDInfoResults2 imager_extended_info1;

	GetCCDInfoResults4 imager_extended_info2;
	GetCCDInfoResults4 guider_extended_info2;

	cframe_params imager_frame_params;
	cframe_params guider_frame_params;
	pthread_mutex_t usb_mutex;
	bool can_check_temperature;
	/* indigo_property *fli_nflushes_property; */
} sbig_private_data;


short (*sbig_command)(short, void*, void*);
static void remove_all_devices();
static void remove_eth_devices();
static bool plug_device(char *cam_name, unsigned short device_type, unsigned long ip_address);


double bcd2double(unsigned long bcd) {
	double value = 0.0;
	double digit = 0.01;
	for(int i = 0; i < 8; i++) {
		value += (bcd & 0x0F) * digit;
		digit *= 10.0;
		bcd >>= 4;
	}
	return value;
}


/* driver commands */

static short get_sbig_handle() {
	GetDriverHandleResults gdhr;
	int res = sbig_command(CC_GET_DRIVER_HANDLE, NULL, &gdhr);
	if ( res == CE_NO_ERROR )
		return gdhr.handle;
	else
		return INVALID_HANDLE_VALUE;
}


static short set_sbig_handle(short handle) {
	SetDriverHandleParams sdhp;
	sdhp.handle = handle;
	return sbig_command(CC_SET_DRIVER_HANDLE, &sdhp, NULL);
}


static short open_driver(short *handle) {
	short res;
	GetDriverHandleResults gdhr;

	res = set_sbig_handle(INVALID_HANDLE_VALUE);
	if (res != CE_NO_ERROR) {
		return res;
	}

	res = sbig_command(CC_OPEN_DRIVER, NULL, NULL);
	if (res != CE_NO_ERROR) {
		return res;
	}

	res = sbig_command(CC_GET_DRIVER_HANDLE, NULL, &gdhr);
	if (res == CE_NO_ERROR) {
		*handle = gdhr.handle;
	}
	return res;
}


static short close_driver(short *handle) {
	short res;

	res = set_sbig_handle(*handle);
	if ( res != CE_NO_ERROR ) {
		return res;
	}

	res = sbig_command(CC_CLOSE_DRIVER, NULL, NULL);
	if ( res == CE_NO_ERROR )
		*handle = INVALID_HANDLE_VALUE;

	return res;
}


static ushort sbig_get_relaymap(short handle, ushort *relay_map) {
	short res;
	QueryCommandStatusParams csq = { .command = CC_ACTIVATE_RELAY };
	QueryCommandStatusResults csr;

	res = set_sbig_handle(handle);
	if ( res != CE_NO_ERROR ) {
		return res;
	}

	res = sbig_command(CC_QUERY_COMMAND_STATUS, &csq, &csr);
	if (res != CE_NO_ERROR) {
		return res;
	}

	*relay_map = csr.status;
	INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: *relay_map = Ox%x", *relay_map));

	return CE_NO_ERROR;
}


static ushort sbig_set_relaymap(short handle, ushort relay_map) {
	short res;
	ActivateRelayParams arp = {0};

	res = set_sbig_handle(handle);
	if ( res != CE_NO_ERROR ) {
		return res;
	}

	if(relay_map & RELAY_EAST) {
		arp.tXPlus = RELAY_MAX_PULSE;
	}

	if(relay_map & RELAY_WEST) {
		arp.tXMinus = RELAY_MAX_PULSE;
	}

	if(relay_map & RELAY_NORTH) {
		arp.tYMinus = RELAY_MAX_PULSE;
	}

	if(relay_map & RELAY_SOUTH) {
		arp.tYPlus = RELAY_MAX_PULSE;
	}

	res = sbig_command(CC_ACTIVATE_RELAY, &arp, NULL);
	if (res != CE_NO_ERROR) {
		return res;
	}

	return CE_NO_ERROR;
}


static ushort sbig_set_relays(short handle, ushort relays) {
	int res;
	ushort relay_map = 0;

	res = sbig_get_relaymap(handle, &relay_map);
	if (res != CE_NO_ERROR) {
		return res;
	}

	relay_map |= relays;
	return sbig_set_relaymap(handle, relay_map);
}


/* indigo CAMERA functions */

static indigo_result sbig_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if ((CONNECTION_CONNECTED_ITEM->sw.value) && (device == PRIVATE_DATA->primary_ccd)) {
		/* if (indigo_property_match(FLI_NFLUSHES_PROPERTY, property))
			indigo_define_property(device, FLI_NFLUSHES_PROPERTY, NULL); */
	}
	return indigo_ccd_enumerate_properties(device, NULL, NULL);
}


static bool sbig_open(indigo_device *device) {
	int id;
	OpenDeviceParams odp;
	short res;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		odp.deviceType = PRIVATE_DATA->usb_id;
		odp.ipAddress = PRIVATE_DATA->ip_address;
		odp.lptBaseAddress = 0x00;

		if ((res = open_driver(&PRIVATE_DATA->driver_handle)) != CE_NO_ERROR) {
			PRIVATE_DATA->driver_handle = INVALID_HANDLE_VALUE;
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DRIVER error = %d", res));
			return false;
		}

		if ((res = sbig_command(CC_OPEN_DEVICE, &odp, NULL)) != CE_NO_ERROR) {
			close_driver(&PRIVATE_DATA->driver_handle);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DEVICE error = %d", res));
			return false;
		}

		EstablishLinkParams elp = {
			.sbigUseOnly = 0
		};
		EstablishLinkResults elr;

		if ((res = sbig_command(CC_ESTABLISH_LINK, &elp, &elr)) != CE_NO_ERROR) {
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
			close_driver(&PRIVATE_DATA->driver_handle);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_ESTABLISH_LINK error = %d", res));
			return false;
		}

	/*
	long res = FLIOpen(&(PRIVATE_DATA->dev_id), PRIVATE_DATA->dev_file_name, PRIVATE_DATA->domain);
	id = PRIVATE_DATA->dev_id;
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIOpen(%d) = %d", id, res));
		return false;
	}

	res = FLIGetArrayArea(id, &(PRIVATE_DATA->total_area.ul_x), &(PRIVATE_DATA->total_area.ul_y), &(PRIVATE_DATA->total_area.lr_x), &(PRIVATE_DATA->total_area.lr_y));
	if (res) {
		FLIClose(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetArrayArea(%d) = %d", id, res));
		return false;
	}

	res = FLIGetVisibleArea(id, &(PRIVATE_DATA->visible_area.ul_x), &(PRIVATE_DATA->visible_area.ul_y), &(PRIVATE_DATA->visible_area.lr_x), &(PRIVATE_DATA->visible_area.lr_y));
	if (res) {
		FLIClose(id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetVisibleArea(%d) = %d", id, res));
		return false;
	}

	res = FLISetFrameType(id, FLI_FRAME_TYPE_RBI_FLUSH);
	if (res) {
		PRIVATE_DATA->rbi_flood_supported = false;
	} else {
		PRIVATE_DATA->rbi_flood_supported = true;
	}

	long height = PRIVATE_DATA->total_area.lr_y - PRIVATE_DATA->total_area.ul_y;
	long width = PRIVATE_DATA->total_area.lr_x - PRIVATE_DATA->total_area.ul_x;

	//INDIGO_ERROR(indigo_error("indigo_ccd_fli: %ld %ld %ld %ld - %ld, %ld", PRIVATE_DATA->total_area.lr_x, PRIVATE_DATA->total_area.lr_y, PRIVATE_DATA->total_area.ul_x, PRIVATE_DATA->total_area.ul_y, height, width));

	if (PRIVATE_DATA->buffer == NULL) {
		PRIVATE_DATA->buffer_size = width * height * 2 + FITS_HEADER_SIZE;
		PRIVATE_DATA->buffer = (unsigned char*)malloc(PRIVATE_DATA->buffer_size);
	}
	*/
	}
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool sbig_start_exposure(indigo_device *device, double exposure, bool dark, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	long res;

	/* needed to read frame data */
	if (PRIMARY_CCD) {
		PRIVATE_DATA->imager_frame_params.width = frame_width;
		PRIVATE_DATA->imager_frame_params.height = frame_height;
		PRIVATE_DATA->imager_frame_params.bin_x = bin_x;
		PRIVATE_DATA->imager_frame_params.bin_y = bin_y;
		PRIVATE_DATA->imager_frame_params.bpp = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	} else {
		PRIVATE_DATA->guider_frame_params.width = frame_width;
		PRIVATE_DATA->guider_frame_params.height = frame_height;
		PRIVATE_DATA->guider_frame_params.bin_x = bin_x;
		PRIVATE_DATA->guider_frame_params.bin_y = bin_y;
		PRIVATE_DATA->guider_frame_params.bpp = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value;
	}

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	/* FLISetBitDepth() does not seem to work! */
	/*
	res = FLISetBitDepth(id, bit_depth);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetBitDepth(%d) = %d", id, res));
		return false;
	}
	*/
	/*
	res = FLISetHBin(id, bin_x);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetHBin(%d) = %d", id, res));
		return false;
	}

	res = FLISetVBin(id, bin_y);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetVBin(%d) = %d", id, res));
		return false;
	}

	res = FLISetImageArea(id, offset_x, offset_y, right_x, right_y);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetImageArea(%d) = %d", id, res));
		return false;
	}

	res = FLISetExposureTime(id, (long)s2ms(exposure));
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetExposureTime(%d) = %d", id, res));
		return false;
	}

	fliframe_t frame_type = FLI_FRAME_TYPE_NORMAL;
	if (dark) frame_type = FLI_FRAME_TYPE_DARK;
	if (rbi_flood) frame_type = FLI_FRAME_TYPE_DARK | FLI_FRAME_TYPE_FLOOD;
	res = FLISetFrameType(id, frame_type);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetFrameType(%d) = %d", id, res));
		return false;
	}

	res = FLIExposeFrame(id);
	if (res) {
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIExposeFrame(%d) = %d", id, res));
		return false;
	}
	*/
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	return true;
}


static bool sbig_read_pixels(indigo_device *device) {
	long timeleft = 0;
	long res, dev_status;
	long wait_cicles = 4000;

	/*
	do {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = FLIGetExposureStatus(id, &timeleft);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (timeleft) usleep(timeleft);
	} while (timeleft*1000);

	do {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		FLIGetDeviceStatus(id, &dev_status);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if((dev_status != FLI_CAMERA_STATUS_UNKNOWN) && ((dev_status & FLI_CAMERA_DATA_READY) != 0)) {
			break;
		}
		usleep(10000);
		wait_cicles--;
	} while (wait_cicles);

	if (wait_cicles == 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_fli: Exposure Failed! id=%d", id));
		return false;
	}

	long row_size = PRIVATE_DATA->frame_params.width / PRIVATE_DATA->frame_params.bin_x * PRIVATE_DATA->frame_params.bpp / 8;
	long width = PRIVATE_DATA->frame_params.width / PRIVATE_DATA->frame_params.bin_x;
	long height = PRIVATE_DATA->frame_params.height / PRIVATE_DATA->frame_params.bin_y ;
	unsigned char *image = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;

	bool success = true;
	for (int i = 0; i < height; i++) {
		pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
		res = FLIGrabRow(id, image + (i * row_size), width);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			if (success) INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGrabRow(%d) = %d at row %d.", id, res, i));
			success = false;
		}
	}
	return success;
	*/
}


static bool sbig_abort_exposure(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	/*
	long err = FLICancelExposure(PRIVATE_DATA->dev_id);
	FLICancelExposure(PRIVATE_DATA->dev_id);
	FLICancelExposure(PRIVATE_DATA->dev_id);
	PRIVATE_DATA->can_check_temperature = true;
	PRIVATE_DATA->abort_flag = true;
	*/
	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	//if(err) return false;
	//else return true;
}


static bool sbig_set_cooler(indigo_device *device, double target, double *current, double *cooler_power) {
	long res;

	/*
	flidev_t id = PRIVATE_DATA->dev_id;
	long current_status;
	static double old_target = 100.0;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	res = FLIGetTemperature(id, current);
	if(res) INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetTemperature(%d) = %d", id, res));

	if ((target != old_target) && CCD_COOLER_ON_ITEM->sw.value) {
		res = FLISetTemperature(id, target);
		if(res) INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetTemperature(%d) = %d", id, res));
	} else if(CCD_COOLER_OFF_ITEM->sw.value) {

		res = FLISetTemperature(id, 45);
		if(res) INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLISetTemperature(%d) = %d", id, res));
	}

	res = FLIGetCoolerPower(id, (double *)cooler_power);
	if(res) INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetCoolerPower(%d) = %d", id, res));

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
	*/
	return true;
}


static void sbig_close(indigo_device *device) {
	int res;
	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	if (--PRIVATE_DATA->count_open == 0) {
		res = set_sbig_handle(PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d", PRIVATE_DATA->driver_handle, res));
		}

		res = sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_CLOSE_DEVICE error = %d", res));
		}

		res = close_driver(&PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: close_driver(%d) = %d", PRIVATE_DATA->driver_handle, res));
		}

		/*
		long res = FLIClose(PRIVATE_DATA->dev_id);
		pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIClose(%d) = %d", PRIVATE_DATA->dev_id, res));
		}
		if (PRIVATE_DATA->buffer != NULL) {
			free(PRIVATE_DATA->buffer);
			PRIVATE_DATA->buffer = NULL;
		}
		*/
	}

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

// callback for image download
static void exposure_timer_callback(indigo_device *device) {
	PRIVATE_DATA->exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (sbig_read_pixels(device)) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, PRIVATE_DATA->buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->can_check_temperature = true;
}


// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->can_check_temperature = false;
		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
	} else {
		PRIVATE_DATA->exposure_timer = NULL;
	}
}


static void ccd_temperature_callback(indigo_device *device) {
	if (PRIVATE_DATA->can_check_temperature) {
		if (sbig_set_cooler(device, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature, &PRIVATE_DATA->cooler_power)) {
			double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
			if(CCD_COOLER_ON_ITEM->sw.value) {
				CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > TEMP_THRESHOLD ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
			}
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_OK_STATE;
			CCD_COOLER_POWER_ITEM->number.value = PRIVATE_DATA->cooler_power;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
			CCD_COOLER_POWER_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		if (CCD_COOLER_PROPERTY->state != INDIGO_OK_STATE) {
			CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}

		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
		indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, TEMP_CHECK_TIME, &PRIVATE_DATA->temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if ((indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) && (device == PRIVATE_DATA->primary_ccd)) {
		pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);

		/* Use all info property fields */
		INFO_PROPERTY->count = 7;

		/*
		// -------------------------------------------------------------------------------- FLI_NFLUSHES
		FLI_NFLUSHES_PROPERTY = indigo_init_number_property(NULL, device->name, "FLI_NFLUSHES", FLI_ADVANCED_GROUP, "Flush CCD", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
		if (FLI_NFLUSHES_PROPERTY == NULL)
			return INDIGO_FAILED;

		indigo_init_number_item(FLI_NFLUSHES_PROPERTY_ITEM, "FLI_NFLUSHES", "Times (before exposure)", MIN_N_FLUSHES, MAX_N_FLUSHES, 1, DEFAULT_N_FLUSHES);
		*/

		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;
	PRIVATE_DATA->abort_flag = false;

	ok = sbig_start_exposure(device, CCD_EXPOSURE_ITEM->number.target, CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value,
	                                    CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
	                                    CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);

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
			PRIVATE_DATA->can_check_temperature = false;
			PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, exposure_timer_callback);
		}
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	return false;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (sbig_open(device)) {
				GetCCDInfoParams cip;
				short res;
				if (PRIMARY_CCD) {
					pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
					CCD_MODE_PROPERTY->hidden = false;
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_INFO_PROPERTY->hidden = false;

					cip.request = 0; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_basic_info));
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: CC_GET_CCD_INFO(%d)  = %d", cip.request, res));

					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->imager_basic_info.readoutInfo[0].width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->imager_basic_info.readoutInfo[0].height;
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = bcd2double(PRIVATE_DATA->imager_basic_info.readoutInfo[0].pixelWidth);
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = bcd2double(PRIVATE_DATA->imager_basic_info.readoutInfo[0].pixelHeight);
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%.2f", bcd2double(PRIVATE_DATA->imager_basic_info.firmwareVersion));
					sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", PRIVATE_DATA->imager_basic_info.name);

					cip.request = 2; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_extended_info1));
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: CC_GET_CCD_INFO(%d)  = %d", cip.request, res));

					strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->imager_extended_info1.serialNumber, INDIGO_VALUE_SIZE);

					// -------------------------------------------------------------------------------- FLI_CAMERA_MODE
					/*
					flimode_t current_mode;
					int i;
					char mode_name[INDIGO_NAME_SIZE];
					res = FLIGetCameraMode(id, &current_mode);
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetCameraMode(%d) = %d", id, res));
					}
					res = FLIGetCameraModeString(id, 0, mode_name, INDIGO_NAME_SIZE);
					if (res == 0) {
						for (i = 0; i < MAX_MODES; i++) {
							res = FLIGetCameraModeString(id, i, mode_name, INDIGO_NAME_SIZE);
							if (res) break;
							indigo_init_switch_item(FLI_CAMERA_MODE_PROPERTY->items + i, mode_name, mode_name, (i == current_mode));
						}
						FLI_CAMERA_MODE_PROPERTY = indigo_resize_property(FLI_CAMERA_MODE_PROPERTY, i);
					}

					indigo_define_property(device, FLI_CAMERA_MODE_PROPERTY, NULL);

					double size_x, size_y;

					res = FLIGetPixelSize(id, &size_x, &size_y);
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetPixelSize(%d) = %d", id, res));
					}

					res = FLIGetModel(id, INFO_DEVICE_MODEL_ITEM->text.value, INDIGO_VALUE_SIZE);
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetModel(%d) = %d", id, res));
					}

					res = FLIGetSerialString(id, INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE);
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetSerialString(%d) = %d", id, res));
					}

					long hw_rev, fw_rev;
					res = FLIGetFWRevision(id, &fw_rev);
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetFWRevision(%d) = %d", id, res));
					}

				res = FLIGetHWRevision(id, &hw_rev);
				if (res) {
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetHWRevision(%d) = %d", id, res));
				}
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);

				sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%ld", fw_rev);
				sprintf(INFO_DEVICE_HW_REVISION_ITEM->text.value, "%ld", hw_rev);
*/
				indigo_update_property(device, INFO_PROPERTY, NULL);

				//INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetPixelSize(%d) = %f %f", id, size_x, size_y));

				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = MAX_X_BIN;
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = MAX_Y_BIN;

				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = DEFAULT_BPP;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = DEFAULT_BPP;

				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = MAX_X_BIN;
				CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = MAX_Y_BIN;

				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
/*
				CCD_TEMPERATURE_PROPERTY->hidden = false;
				CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
				CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
				CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
				CCD_TEMPERATURE_ITEM->number.step = 0;
				/*
				res = FLIGetTemperature(id,&(CCD_TEMPERATURE_ITEM->number.value));
				if (res) {
					INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetTemperature(%d) = %d", id, res));
				}
				PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
				PRIVATE_DATA->can_check_temperature = true;

				CCD_COOLER_POWER_PROPERTY->hidden = false;
				CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

				PRIVATE_DATA->temperature_timer = indigo_set_timer(device, 0, ccd_temperature_callback);
				*/
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
				} else {
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			PRIVATE_DATA->can_check_temperature = false;
			indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
			sbig_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			handle_exposure_property(device, property);
		}
	// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			sbig_abort_exposure(device);
		}
		PRIVATE_DATA->can_check_temperature = true;
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	// -------------------------------------------------------------------------------- CCD_COOLER
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		//INDIGO_ERROR(indigo_error("indigo_ccd_asi: COOOLER = %d %d", CCD_COOLER_OFF_ITEM->sw.value, CCD_COOLER_ON_ITEM->sw.value));
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			if (CCD_COOLER_ON_ITEM->sw.value)
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
			else
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f but the cooler is OFF, ", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
	// ------------------------------------------------------------------------------- CCD_FRAME
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 8 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 8);
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 2 * (int)(CCD_FRAME_HEIGHT_ITEM->number.value / 2);
		if (CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value < 64)
			CCD_FRAME_WIDTH_ITEM->number.value = 64 * CCD_BIN_HORIZONTAL_ITEM->number.value;
		if (CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value < 64)
			CCD_FRAME_HEIGHT_ITEM->number.value = 64 * CCD_BIN_VERTICAL_ITEM->number.value;
		/* FLISetBitDepth() does not seem to work so this should be always 16 bits */
		if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value < 12.0) {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8.0;
		} else {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16.0;
		}

		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			/* indigo_save_property(device, NULL, FLI_NFLUSHES_PROPERTY); */
		}
	}
	// -----------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' detached.", device->name));

	if (device == PRIVATE_DATA->primary_ccd) {
		/* indigo_release_property(FLI_NFLUSHES_PROPERTY); */
	}

	return indigo_ccd_detach(device);
}

/* indigo GUIDER functions */

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		//INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void guider_timer_callback_ra(indigo_device *device) {
	int res;
	ushort relay_map = 0;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	PRIVATE_DATA->guider_timer_ra = NULL;
	int driver_handle = PRIVATE_DATA->driver_handle;

	res = sbig_get_relaymap(driver_handle, &relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_get_relaymap(%d) = %d", driver_handle, res));
	}

	relay_map &= ~(RELAY_EAST | RELAY_WEST);

	res = sbig_set_relaymap(driver_handle, relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relaymap(%d) = %d", driver_handle, res));
	}

	if (PRIVATE_DATA->relay_map & (RELAY_EAST | RELAY_WEST)) {
		GUIDER_GUIDE_EAST_ITEM->number.value = 0;
		GUIDER_GUIDE_WEST_ITEM->number.value = 0;
		GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
	}
	PRIVATE_DATA->relay_map = relay_map;

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}


static void guider_timer_callback_dec(indigo_device *device) {
	int res;
	ushort relay_map = 0;

	pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);

	PRIVATE_DATA->guider_timer_ra = NULL;
	int driver_handle = PRIVATE_DATA->driver_handle;

	res = sbig_get_relaymap(driver_handle, &relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_get_relaymap(%d) = %d", driver_handle, res));
	}

	relay_map &= ~(RELAY_NORTH | RELAY_SOUTH);

	res = sbig_set_relaymap(driver_handle, relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relaymap(%d) = %d", driver_handle, res));
	}

	if (PRIVATE_DATA->relay_map & (RELAY_NORTH | RELAY_SOUTH)) {
		GUIDER_GUIDE_NORTH_ITEM->number.value = 0;
		GUIDER_GUIDE_SOUTH_ITEM->number.value = 0;
		GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
	}
	PRIVATE_DATA->relay_map = relay_map;

	pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
}


static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	ushort res;
	int driver_handle = PRIVATE_DATA->driver_handle;

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (sbig_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
				GUIDER_GUIDE_RA_PROPERTY->hidden = false;
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			sbig_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = sbig_set_relays(driver_handle, RELAY_NORTH);
			if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_NORTH) = %d", driver_handle, res));
			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			PRIVATE_DATA->relay_map |= RELAY_NORTH;
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = sbig_set_relays(driver_handle, RELAY_SOUTH);
				if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_SOUTH) = %d", driver_handle, res));
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
				PRIVATE_DATA->relay_map |= RELAY_SOUTH;
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			}
		}

		if (PRIVATE_DATA->relay_map & (RELAY_NORTH | RELAY_SOUTH))
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
		indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_ra);
		int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
			res = sbig_set_relays(driver_handle, RELAY_EAST);
			if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_EAST) = %d", driver_handle, res));
			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
			PRIVATE_DATA->relay_map |= RELAY_EAST;
			pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&PRIVATE_DATA->usb_mutex);
				res = sbig_set_relays(driver_handle, RELAY_WEST);
				if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_WEST) = %d", driver_handle, res));
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
				PRIVATE_DATA->relay_map |= RELAY_WEST;
				pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
			}
		}

		if (PRIVATE_DATA->relay_map & (RELAY_EAST | RELAY_WEST))
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_BUSY_STATE;
		else
			GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;

		indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
		// --------------------------------------------------------------------------------
	}
	return indigo_guider_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' detached.", device->name));
	return indigo_guider_detach(device);
}


static const char *CAM_NAMES[] = {
	"Type 0", "Type 1", "Type 2", "Type 3",
	"ST-7", "ST-8", "ST-5C", "TCE",
	"ST-237", "ST-K", "ST-9", "STV", "ST-10",
	"ST-1K", "ST-2K", "STL", "ST-402", "STX",
	"ST-4K", "STT", "ST-i",	"STF-8300"
};


// -------------------------------------------------------------------------------- Ethernet support
#ifdef ETHERNET_DRIVER

bool get_host_ip(char *hostname , unsigned long *ip) {
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: getaddrinfo(): %s\n", gai_strerror(rv)));
		return false;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if(p->ai_family == AF_INET) {
			*ip = ((struct sockaddr_in *)(p->ai_addr))->sin_addr.s_addr;
			/* ip should be litle endian */
			*ip = (*ip >> 24) | ((*ip << 8) & 0x00ff0000) | ((*ip >> 8) & 0x0000ff00) | (*ip << 24);
			INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: IP: 0x%X\n", *ip));
			freeaddrinfo(servinfo);
			return true;
		}
	}
	freeaddrinfo(servinfo);
	return false;
}


static indigo_result eth_attach(indigo_device *device) {
	assert(device != NULL);
	unsigned long ip;
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		get_host_ip("localhost", &ip);
		strncpy(DEVICE_PORT_ITEM->text.value, "192.168.0.100", INDIGO_VALUE_SIZE);
		strncpy(DEVICE_PORT_PROPERTY->label, "Camera address", INDIGO_VALUE_SIZE);
		strncpy(DEVICE_PORT_ITEM->label, "Camera IP/URL", INDIGO_VALUE_SIZE);
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------

		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static indigo_result eth_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		char message[1024] = {0};
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			snprintf(message, 1024, "Conneting to %s. This may take several minutes.", DEVICE_PORT_ITEM->text.value);
			indigo_update_property(device, CONNECTION_PROPERTY, message);
			unsigned long ip_address;
			bool ok;
			ok = get_host_ip(DEVICE_PORT_ITEM->text.value, &ip_address);
			if (ok) {
				ok = plug_device(device->name, DEV_ETH, ip_address);
			}
			if (ok) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				message[0] = '\0';
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, true);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				snprintf(message, 1024, "Conneting to %s failed.", DEVICE_PORT_ITEM->text.value);
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			remove_eth_devices();
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}

		if (message[0] == '\0')
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		else
			indigo_update_property(device, CONNECTION_PROPERTY, message);

		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result eth_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_ccd_detach(device);
}

#endif /* ETHERNET_DRIVER */

// -------------------------------------------------------------------------------- hot-plug support

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
short global_handle = INVALID_HANDLE_VALUE; /* This is global SBIG driver hangle used for attach and detatch cameras */

#define MAX_USB_DEVICES                8
#define MAX_ETH_DEVICES                8
#define MAX_DEVICES                   32

static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static indigo_device *sbig_eth = NULL;

static QueryUSBResults2 usb_cams = {0};
static QueryEthernetResults2 eth_cams = {0};


static inline int usb_to_index(SBIG_DEVICE_TYPE type) {
	return DEV_ETH - type;
}


static inline SBIG_DEVICE_TYPE index_to_usb(int index) {
	return DEV_ETH + index;
}


static int find_available_device_slot() {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		if (devices[slot] == NULL) return slot;
	}
	return -1;
}


static int find_device_slot(CAMERA_TYPE usb_id) {
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		if (PRIVATE_DATA->usb_id == usb_id) return slot;
	}
	return -1;
}


static bool plug_device(char *cam_name, unsigned short device_type, unsigned long ip_address) {
	static indigo_device ccd_template = {
		"", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		sbig_enumerate_properties,
		ccd_change_property,
		ccd_detach
	};

	static indigo_device guider_template = {
		"", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		guider_detach
	};

	if (cam_name == NULL) {
		return false;
	}

	short res;
	OpenDeviceParams odp = {
		.deviceType = device_type,
		.ipAddress = ip_address,
		.lptBaseAddress = 0x00
	};

	if ((res = sbig_command(CC_OPEN_DEVICE, &odp, NULL)) != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DEVICE error = %d", res));
		return false;
	}

	EstablishLinkParams elp = { .sbigUseOnly = 0 };
	EstablishLinkResults elr;

	if ((res = sbig_command(CC_ESTABLISH_LINK, &elp, &elr)) != CE_NO_ERROR) {
		sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_ESTABLISH_LINK error = %d", res));
		return false;
	}

	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: No device slots available."));
		return false;
	}
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: NEW cam: slot = %d device_type = 0x%x name ='%s'", slot, device_type, cam_name));
	indigo_device *device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &ccd_template, sizeof(indigo_device));
	sprintf(device->name, "%s #%d", cam_name, usb_to_index(device_type));
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' attached.", device->name));
	sbig_private_data *private_data = malloc(sizeof(sbig_private_data));
	assert(private_data);
	memset(private_data, 0, sizeof(sbig_private_data));
	private_data->usb_id = device_type;
	private_data->ip_address = ip_address;
	if (ip_address) {
		private_data->is_usb = false;
	} else {
		private_data->is_usb = true;
	}
	private_data->primary_ccd = device;
	strncpy(private_data->dev_name, cam_name, MAX_PATH);
	device->private_data = private_data;
	indigo_async((void *)(void *)indigo_attach_device, device);
	devices[slot]=device;

	slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_asi: No device slots available."));
		return false;
	}
	device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &guider_template, sizeof(indigo_device));
	sprintf(device->name, "%s Guider #%d", cam_name, usb_to_index(device_type));
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' attached.", device->name));
	device->private_data = private_data;
	indigo_async((void *)(void *)indigo_attach_device, device);
	devices[slot]=device;

	/* Check if there is secondary CCD */
	GetCCDInfoParams gcp = { .request = CCD_INFO_TRACKING };
	GetCCDInfoResults0 gcir0;

	if ((res = sbig_command(CC_GET_CCD_INFO, &gcp, &gcir0)) != CE_NO_ERROR) {
		INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: CC_GET_CCD_INFO error = %d, asuming no Secondary CCD", res));
		sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		return true;
	} else {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_ERROR(indigo_error("indigo_ccd_asi: No device slots available."));
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
			return false;
		}
		device = malloc(sizeof(indigo_device));
		assert(device != NULL);
		memcpy(device, &ccd_template, sizeof(indigo_device));
		sprintf(device->name, "%s Guider CCD #%d", cam_name, usb_to_index(device_type));
		INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' attached.", device->name));
		device->private_data = private_data;
		indigo_async((void *)(void *)indigo_attach_device, device);
		devices[slot]=device;
	}
	sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
	return true;
}


#ifndef ETHERNET_DRIVER

static void enumerate_devices() {
	int res = sbig_command(CC_QUERY_USB2, NULL, &usb_cams);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: command CC_QUERY_USB2 error = %d", res));
	}

	INDIGO_LOG(indigo_log("indigo_ccd_sbig: usb_cams = %d", usb_cams.camerasFound));
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: usb_type = %d", usb_cams.usbInfo[0].cameraType ));
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: cam name = %s", usb_cams.usbInfo[0].name));
}


static int find_plugged_device(char *dev_name) {
	enumerate_devices();
	for (int dev_no = 0; dev_no < MAX_USB_DEVICES; dev_no++) {
		bool found = false;
		if (!usb_cams.usbInfo[dev_no].cameraFound) continue;
		for(int slot = 0; slot < MAX_DEVICES; slot++) {
			indigo_device *device = devices[slot];
			if (device == NULL) continue;
			if (PRIVATE_DATA->usb_id == index_to_usb(dev_no)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		} else {
			assert(dev_name!=NULL);
			strncpy(dev_name, usb_cams.usbInfo[dev_no].name, MAX_PATH);
			return index_to_usb(dev_no);
		}
	}
	return -1;
}


static int find_unplugged_device(char *dev_name) {
	enumerate_devices();
	for(int slot = 0; slot < MAX_DEVICES; slot++) {
		bool found = false;
		indigo_device *device = devices[slot];
		if (device == NULL) continue;
		for (int dev_no = 0; dev_no < MAX_USB_DEVICES; dev_no++) {
			if ((!usb_cams.usbInfo[dev_no].cameraFound) || (!PRIVATE_DATA->is_usb)) continue;
			if (PRIVATE_DATA->usb_id == index_to_usb(dev_no)) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		} else {
			assert(dev_name!=NULL);
			strncpy(dev_name, PRIVATE_DATA->dev_name, MAX_PATH);
			return slot;
		}
	}
	return -1;
}


static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	pthread_mutex_lock(&device_mutex);

	short res = set_sbig_handle(global_handle);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error set_sbig_handle(global_handle) = %d", res));
	}

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			char cam_name[MAX_PATH];
			int usb_id = find_plugged_device(cam_name);
			if (usb_id < 0) {
				INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: No SBIG Camera plugged."));
				pthread_mutex_unlock(&device_mutex);
				return 0;
			}

			plug_device(cam_name, usb_id, 0);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			int slot, usb_index;
			char cam_name[MAX_PATH];
			bool removed = false;
			sbig_private_data *private_data = NULL;
			while ((usb_index = find_unplugged_device(cam_name)) != -1) {
				slot = find_device_slot(index_to_usb(usb_index));
				INDIGO_LOG(indigo_log("!!!! indigo_ccd_sbig: '%s' usb_id=0x%x, slot=%d", cam_name, usb_index, slot));
				while (slot >= 0) {
					indigo_device **device = &devices[slot];
					if (*device == NULL) {
						pthread_mutex_unlock(&device_mutex);
						return 0;
					}
					indigo_detach_device(*device);
					if ((*device)->private_data) {
						private_data = (*device)->private_data;
					}
					free(*device);
					*device = NULL;
					removed = true;
					slot = find_device_slot(index_to_usb(usb_index));
				}

				if (private_data) {
					/* close driver and device here */
					free(private_data);
					private_data = NULL;
				}
			}

			if (!removed) {
				INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: No SBIG Camera unplugged!"));
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
	return 0;
};

#endif /* NOT ETHERNET_DRIVER */


static void remove_all_devices() {
	int i;
	sbig_private_data *pds[MAX_USB_DEVICES] = {NULL};

	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) pds[usb_to_index(PRIVATE_DATA->usb_id)] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		device = NULL;
	}

	/* free private data */
	for(i = 0; i < MAX_USB_DEVICES; i++) {
		if (pds[i]) free(pds[i]);
	}
}

static void remove_eth_devices() {
	int i;
	sbig_private_data *pds[MAX_ETH_DEVICES] = {NULL};

	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if ((device == NULL) || ((PRIVATE_DATA) && (PRIVATE_DATA->is_usb))) continue;
		if (PRIVATE_DATA) pds[usb_to_index(PRIVATE_DATA->usb_id)] = PRIVATE_DATA; /* preserve pointers to private data */
		indigo_detach_device(device);
		free(device);
		device = NULL;
	}

	/* free private data */
	for(i = 0; i < MAX_ETH_DEVICES; i++) {
		if (pds[i]) free(pds[i]);
	}
}


static libusb_hotplug_callback_handle callback_handle;

#ifdef ETHERNET_DRIVER
#define ENTRY_POINT indigo_ccd_sbig_eth
#else
#define ENTRY_POINT indigo_ccd_sbig
#endif

indigo_result ENTRY_POINT(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

#ifdef ETHERNET_DRIVER
	static indigo_device sbig_eth_template = {
		"SBIG Ethernet", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		eth_attach,
		indigo_ccd_enumerate_properties,
		eth_change_property,
		eth_detach
	};
	SET_DRIVER_INFO(info, "SBIG Ethernet Camera", __FUNCTION__, DRIVER_VERSION, last_action);
#else
	SET_DRIVER_INFO(info, "SBIG Camera", __FUNCTION__, DRIVER_VERSION, last_action);
#endif /* ETHERNET_DRIVER */

#ifdef __APPLE__
	static void *dl_handle = NULL;
#endif

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;

#ifdef __linux__
		sbig_command = SBIGUnivDrvCommand;
#elif __APPLE__
		dl_handle = dlopen("/Library/Frameworks/SBIGUDrv.framework/SBIGUDrv", RTLD_LAZY);
		if (!dl_handle) {
			const char* dlsym_error = dlerror();
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: SBIG SDK can't be loaded (%s)", dlsym_error));
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: Please install SBIGUDrv framework from http://www.sbig.com"));
			return INDIGO_FAILED;
		}
		sbig_command = dlsym(dl_handle, "SBIGUnivDrvCommand");
		const char* dlsym_error = dlerror();
		if (dlsym_error) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: Can't load %s() (%s)", "SBIGUnivDrvCommand", dlsym_error));
			dlclose(dl_handle);
			return INDIGO_NOT_FOUND;
		}
#endif

		GetDriverInfoParams di_req = {
			.request = DRIVER_STD
		};
		GetDriverInfoResults0 di;

		int res = sbig_command(CC_OPEN_DRIVER, NULL, NULL);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: command CC_OPEN_DRIVER error = %d", res));
			return INDIGO_FAILED;
		}

		global_handle = get_sbig_handle();
		if (global_handle == INVALID_HANDLE_VALUE) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error get_sbig_handle() = %d", global_handle));
		}

		res = sbig_command(CC_GET_DRIVER_INFO, &di_req, &di);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: command CC_GET_DRIVER_INFO error = %d", res));
		} else {
			INDIGO_LOG(indigo_log("indigo_ccd_sbig: Driver: %s (%x.%x)", di.name, di.version >> 8, di.version & 0x00ff));
		}

		// conditional compile here
#ifndef ETHERNET_DRIVER
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SBIG_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_sbig: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;
#else
		sbig_eth = malloc(sizeof(indigo_device));
		assert(sbig_eth != NULL);
		memcpy(sbig_eth, &sbig_eth_template, sizeof(indigo_device));
		sbig_eth->private_data = NULL;
		indigo_attach_device(sbig_eth);
		return INDIGO_OK;
#endif /* NOT ETHERNET_DRIVER */

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_sbig: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_all_devices();

		res = set_sbig_handle(global_handle);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error set_sbig_handle() = %d", res));
		}

		res = sbig_command(CC_CLOSE_DRIVER, NULL, NULL);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: final command CC_CLOSE_DRIVER error = %d", res));
		}

#ifdef __APPLE__
		if (dl_handle)
			dlclose(dl_handle);
#endif

		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
