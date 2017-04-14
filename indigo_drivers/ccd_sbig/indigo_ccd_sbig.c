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

// TODO:
// 1. Handle ethernet disconnects.
// 2. Removing open device is broken!!!
// 3. Binning and readout modes.
// 4. Better error handling.
// 5. Add filter wheel support.
// 6. Add external guider CCD support
// 7. Add Focuser support
// 8. Add AO support
// 9. Add property to freeze TEC for readout

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

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
#define MAX_X_BIN             3     /* Max Horizontal binning */
#define MAX_Y_BIN             3     /* Max Vertical binning */

#define DEFAULT_BPP          16     /* Default bits per pixel */

#define MAX_PATH            255     /* Maximal Path Length */

#define TEMP_THRESHOLD      0.5
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
	bool is_usb;
	SBIG_DEVICE_TYPE usb_id;
	unsigned long ip_address;
	void *primary_ccd;
	short driver_handle;
	char dev_name[MAX_PATH];
	ushort relay_map;
	int count_open;
	indigo_timer *imager_ccd_exposure_timer, *imager_ccd_temperature_timer;
	indigo_timer *guider_ccd_exposure_timer, *guider_ccd_temperature_timer;
	indigo_timer *guider_timer_ra, *guider_timer_dec;
	double target_temperature, current_temperature;
	double cooler_power;

	unsigned char *imager_buffer;
	unsigned char *guider_buffer;

	GetCCDInfoResults0 imager_ccd_basic_info;
	GetCCDInfoResults0 guider_ccd_basic_info;

	GetCCDInfoResults2 imager_ccd_extended_info1;

	GetCCDInfoResults4 imager_ccd_extended_info2;
	GetCCDInfoResults4 guider_ccd_extended_info2;

	GetCCDInfoResults6 imager_ccd_extended_info6;

	StartExposureParams2 imager_ccd_exp_params;
	StartExposureParams2 guider_ccd_exp_params;

	bool imager_no_check_temperature;
	bool guider_no_check_temperature;
	/* indigo_property *some_sbig_property; */
} sbig_private_data;

static pthread_mutex_t driver_mutex = PTHREAD_MUTEX_INITIALIZER;

short (*sbig_command)(short, void*, void*);
static void remove_usb_devices();
static void remove_eth_devices();
static bool plug_device(char *cam_name, unsigned short device_type, unsigned long ip_address);


static double bcd2double(unsigned long bcd) {
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

static char *sbig_error_string(int err) {
	GetErrorStringParams gesp;
	gesp.errorNo = err;
	static GetErrorStringResults gesr;
	int res = sbig_command(CC_GET_ERROR_STRING, &gesp, &gesr);
	if (res == CE_NO_ERROR) {
		return gesr.errorString;
	}
	static char str[128];
	sprintf(str, "Error string not found! Error code: %d", err);
	return str;
}


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


static int get_command_status(unsigned short command, unsigned short *status) {
	QueryCommandStatusParams qcsp = {
		.command = command
	};
	QueryCommandStatusResults qcsr;

	int res = sbig_command(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr);
	if (res == CE_NO_ERROR) {
		if (status) *status = qcsr.status;
	}
	return res;
}


static int sbig_link_status(GetLinkStatusResults *glsr) {
	int res = sbig_command(CC_GET_LINK_STATUS, NULL, glsr);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_LINK_STATUS error = %d (%s)", res, sbig_error_string(res)));
	}
	return res;
}


static bool sbig_check_link() {
	GetLinkStatusResults glsr;
	if (sbig_link_status(&glsr) != CE_NO_ERROR) {
		return false;
	}

	if(glsr.linkEstablished) {
		return true;
	}
	return false;
}


static int sbig_set_temperature(double t, bool enable) {
	int res;
	SetTemperatureRegulationParams2 strp2;
	strp2.regulation = enable ? REGULATION_ON : REGULATION_OFF;
	strp2.ccdSetpoint = t;
	res = sbig_command(CC_SET_TEMPERATURE_REGULATION2, &strp2, 0);

	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_SET_TEMPERATURE_REGULATION error = %d (%s)", res, sbig_error_string(res)));
	}
	return res ;
}


static int sbig_get_temperature(bool *enabled, double *t, double *setpoint, double *power) {
	int res;
	QueryTemperatureStatusResults2 qtsr2;
	QueryTemperatureStatusParams qtsp = {
		.request = TEMP_STATUS_ADVANCED2
	};
	res = sbig_command(CC_QUERY_TEMPERATURE_STATUS, &qtsp, &qtsr2);
	if (res == CE_NO_ERROR) {
		if (enabled) *enabled = (qtsr2.coolingEnabled != 0);
		if (t) *t = qtsr2.imagingCCDTemperature;
		if (setpoint) *setpoint = qtsr2.ccdSetpoint;
		if (power) *power = qtsr2.imagingCCDPower;
		INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: regulation = %s, t = %.2g, setpoint = %.2g, power = %.2g",
			(qtsr2.coolingEnabled != 0) ? "True": "False",
			qtsr2.imagingCCDTemperature,
			qtsr2.ccdSetpoint,
			qtsr2.imagingCCDPower
		));
	}

	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_TEMPERATURE_STATUS error = %d (%s)", res, sbig_error_string(res)));
	}
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


static int sbig_get_bin_mode(indigo_device *device, unsigned short *binning) {
	if (binning == NULL) return CE_BAD_PARAMETER;
	if ((CCD_BIN_HORIZONTAL_ITEM->number.value == 1) && (CCD_BIN_VERTICAL_ITEM->number.value == 1)) {
		*binning = RM_1X1;
	} else if ((CCD_BIN_HORIZONTAL_ITEM->number.value == 2) && (CCD_BIN_VERTICAL_ITEM->number.value == 2)) {
		*binning = RM_2X2;
	} else if ((CCD_BIN_HORIZONTAL_ITEM->number.value == 3) && (CCD_BIN_VERTICAL_ITEM->number.value == 3)) {
		*binning = RM_3X3;
	} else if ((CCD_BIN_HORIZONTAL_ITEM->number.value == 9) && (CCD_BIN_VERTICAL_ITEM->number.value == 9)) {
		*binning = RM_9X9;
	} else {
		INDIGO_ERROR(indigo_error("Bad CCD binning mode, use 1x1, 2x2, 3x3 or 9x9"));
		return CE_BAD_PARAMETER;
	}
	return CE_NO_ERROR;
}


static int sbig_get_resolution(indigo_device *device, int bin_mode, int *width, int *height, double *pixel_width, double *pixel_height) {
	if ((width == NULL) || (height == NULL)) return CE_BAD_PARAMETER;

	GetCCDInfoResults0 *ccd_info;

	if (PRIMARY_CCD) {
		ccd_info = &(PRIVATE_DATA->imager_ccd_basic_info);
	} else {
		ccd_info = &(PRIVATE_DATA->guider_ccd_basic_info);
	}

	for (int i = 0; i < ccd_info->readoutModes; i++) {
		if (ccd_info->readoutInfo[i].mode == bin_mode) {
			if (width) *width = ccd_info->readoutInfo[i].width;
			if (height) *height = ccd_info->readoutInfo[i].width;
			if (pixel_width) *pixel_width = ccd_info->readoutInfo[i].pixelWidth;
			if (pixel_height) *pixel_height = ccd_info->readoutInfo[i].pixelHeight;
			return CE_NO_ERROR;
		}
	}
	return CE_BAD_PARAMETER;
}


static bool sbig_open(indigo_device *device) {
	OpenDeviceParams odp;
	short res;

	pthread_mutex_lock(&driver_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		odp.deviceType = PRIVATE_DATA->usb_id;
		odp.ipAddress = PRIVATE_DATA->ip_address;
		odp.lptBaseAddress = 0x00;

		if ((res = open_driver(&PRIVATE_DATA->driver_handle)) != CE_NO_ERROR) {
			PRIVATE_DATA->driver_handle = INVALID_HANDLE_VALUE;
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&driver_mutex);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DRIVER error = %d (%s)", res, sbig_error_string(res)));
			return false;
		}

		if ((res = sbig_command(CC_OPEN_DEVICE, &odp, NULL)) != CE_NO_ERROR) {
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL); /* Cludge: sometimes it fails with CE_DEVICE_NOT_CLOSED later */
			close_driver(&PRIVATE_DATA->driver_handle);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&driver_mutex);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DEVICE error = %d (%s)", res, sbig_error_string(res)));
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
			pthread_mutex_unlock(&driver_mutex);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_ESTABLISH_LINK error = %d (%s)", res, sbig_error_string(res)));
			return false;
		}
	}
	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static bool sbig_start_exposure(indigo_device *device, double exposure, bool dark, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	int res;
	StartExposureParams2 *sep;
	unsigned short binning_mode;
	unsigned short shutter_mode;

	pthread_mutex_lock(&driver_mutex);

	res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	res = sbig_get_bin_mode(device, &binning_mode);
	if (res != CE_NO_ERROR) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if (dark) {
		shutter_mode = SC_CLOSE_SHUTTER;
	} else {
		shutter_mode = SC_OPEN_SHUTTER;
	}

	if(PRIMARY_CCD) {
		sep = &(PRIVATE_DATA->imager_ccd_exp_params);
		sep->ccd = CCD_IMAGING;
	} else {
		sep = &(PRIVATE_DATA->guider_ccd_exp_params);
		sep->ccd = CCD_TRACKING;

		unsigned short status;
		res = get_command_status(CC_START_EXPOSURE2, &status);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_QUERY_COMMAND_STATUS error = %d (%s)", res, sbig_error_string(res)));
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
		if ((status & 2) == 2) { /* Primary CCD exposure is in progress do not touch the shutter */
			shutter_mode = SC_LEAVE_SHUTTER;
		}
	}

	sep->abgState = (unsigned short)ABG_LOW7;
	sep->openShutter = (unsigned short)shutter_mode;
	sep->exposureTime = (unsigned long)floor(exposure * 100.0 + 0.5);;
	sep->readoutMode = binning_mode;
	sep->left = (unsigned short)(offset_x / bin_x);
	sep->top = (unsigned short)(offset_y / bin_y);
	sep->width = (unsigned short)(frame_width / bin_x);
	sep->height = (unsigned short)(frame_height / bin_y);

	res = sbig_command(CC_START_EXPOSURE2, sep, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_START_EXPOSURE2 = %d (%s)", res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static bool sbig_exposure_complete(indigo_device *device) {
	int ccd;

	pthread_mutex_lock(&driver_mutex);

	int res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if (PRIMARY_CCD) {
		ccd = CCD_IMAGING;
	} else {
		ccd = CCD_TRACKING;
	}

	unsigned short status;
	res = get_command_status(CC_START_EXPOSURE2, &status);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_QUERY_COMMAND_STATUS error = %d (%s)", res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	int mask = 12; // Tracking & external tracking CCD chip mask.
	if (ccd == CCD_IMAGING) {
		mask = 3; // Imaging chip mask.
	}

	if ((status & mask) != mask) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static bool sbig_read_pixels(indigo_device *device) {
	int res;
	long wait_cycles = 4000;
	unsigned char *frame_buffer;

	/* for the exposyre to complete and end it */
	while(!sbig_exposure_complete(device) && wait_cycles--) {
		usleep(1000);
	}
	if (wait_cycles == 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: Exposure error: did not complete in time."));
	}

	pthread_mutex_lock(&driver_mutex);

	res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	StartReadoutParams srp;
	if (PRIMARY_CCD) {
		frame_buffer = PRIVATE_DATA->imager_buffer + FITS_HEADER_SIZE;
		srp.ccd = CCD_IMAGING;
		srp.readoutMode	= PRIVATE_DATA->imager_ccd_exp_params.readoutMode;
		srp.left = PRIVATE_DATA->imager_ccd_exp_params.left;
		srp.top = PRIVATE_DATA->imager_ccd_exp_params.top;
		srp.width = PRIVATE_DATA->imager_ccd_exp_params.width;
		srp.height = PRIVATE_DATA->imager_ccd_exp_params.height;
	} else {
		frame_buffer = PRIVATE_DATA->guider_buffer + FITS_HEADER_SIZE;
		srp.ccd = CCD_TRACKING;
		srp.readoutMode	= PRIVATE_DATA->guider_ccd_exp_params.readoutMode;
		srp.left = PRIVATE_DATA->guider_ccd_exp_params.left;
		srp.top = PRIVATE_DATA->guider_ccd_exp_params.top;
		srp.width = PRIVATE_DATA->guider_ccd_exp_params.width;
		srp.height = PRIVATE_DATA->guider_ccd_exp_params.height;
	}

	/* SBIG REQUIRES explicit end exposure */
	EndExposureParams eep = {
		.ccd = srp.ccd
	};
	res = sbig_command(CC_END_EXPOSURE, &eep, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_END_EXPOSURE error = %d (%s)", res, sbig_error_string(res)));
	}

	res = sbig_command(CC_START_READOUT, &srp, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_START_READOUT error = %d (%s)", res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	ReadoutLineParams rlp = {
		.ccd = srp.ccd,
		.readoutMode = srp.readoutMode,
		.pixelStart = srp.left,
		.pixelLength = srp.width
	};
	for(int line = 0; line < srp.height; line++) {
		res = sbig_command(CC_READOUT_LINE, &rlp, frame_buffer + 2*(line * srp.width));
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_READOUT_LINE error = %d (%s)", res, sbig_error_string(res)));
		}
	}

	EndReadoutParams erp = {
		.ccd = srp.ccd
	};
	res = sbig_command(CC_END_READOUT, &erp, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_END_READOUT error = %d (%s)", res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static bool sbig_abort_exposure(indigo_device *device) {
	EndExposureParams eep;
	pthread_mutex_lock(&driver_mutex);

	int res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if (PRIMARY_CCD) {
		eep.ccd = CCD_IMAGING;
		PRIVATE_DATA->imager_no_check_temperature = false;
	} else {
		eep.ccd = CCD_TRACKING;
		PRIVATE_DATA->guider_no_check_temperature = false;
	}

	res = sbig_command(CC_END_EXPOSURE, &eep, NULL);
	if ( res != CE_NO_ERROR ) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_END_EXPOSURE error = %d (%s)", res, sbig_error_string(res)));
	}
	pthread_mutex_unlock(&driver_mutex);

	if (res == CE_NO_ERROR) return true;
	else return false;
}


static bool sbig_set_cooler(indigo_device *device, double target, double *current, double *cooler_power) {
	long res;
	bool cooler_on;
	double csetpoint;

	pthread_mutex_lock(&driver_mutex);

	res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if (res) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	res = sbig_get_temperature(&cooler_on, current, &csetpoint, cooler_power);
	if (res) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_get_temperature() = %d (%s)", res, sbig_error_string(res)));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if ((cooler_on != CCD_COOLER_ON_ITEM->sw.value) || (csetpoint != target)) {
		res = sbig_set_temperature(target, CCD_COOLER_ON_ITEM->sw.value);
		if(res) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_temperature() = %d (%s)", res, sbig_error_string(res)));
	}

	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static void sbig_close(indigo_device *device) {
	int res;
	pthread_mutex_lock(&driver_mutex);

	if (--PRIVATE_DATA->count_open == 0) {
		res = set_sbig_handle(PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d", PRIVATE_DATA->driver_handle, res));
		}

		res = sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_CLOSE_DEVICE error = %d (%s)", res, sbig_error_string(res)));
		}

		res = close_driver(&PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: close_driver(%d) = %d", PRIVATE_DATA->driver_handle, res));
		}
	}

	pthread_mutex_unlock(&driver_mutex);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

// callback for image download
static void imager_ccd_exposure_timer_callback(indigo_device *device) {
	unsigned char *frame_buffer;
	indigo_fits_keyword *bayer_keys = NULL;
	const static indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = "BGGR", "Bayer color pattern" },
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" },
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" },
		{ 0 }
	};

	PRIVATE_DATA->imager_ccd_exposure_timer = NULL;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		if (sbig_read_pixels(device)) {
			if(PRIMARY_CCD) {
				frame_buffer = PRIVATE_DATA->imager_buffer;
				/* check if colour and no binning => use BGGR patern */
				if (((PRIVATE_DATA->imager_ccd_extended_info6.ccdBits & 0x03) == 0x01) &&
				   (CCD_BIN_HORIZONTAL_ITEM->number.value == 1) &&
				   (CCD_BIN_VERTICAL_ITEM->number.value == 1)) {
					bayer_keys = (indigo_fits_keyword*)keywords;
				}
			} else {
				frame_buffer = PRIVATE_DATA->guider_buffer;
			}
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_process_image(device, frame_buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value),
			                    (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), true, bayer_keys);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->imager_no_check_temperature = false;
}


// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->imager_no_check_temperature = true;
		PRIVATE_DATA->imager_ccd_exposure_timer = indigo_set_timer(device, 4, imager_ccd_exposure_timer_callback);
	} else {
		PRIVATE_DATA->imager_ccd_exposure_timer = NULL;
	}
}


static void imager_ccd_temperature_callback(indigo_device *device) {
	if (!PRIVATE_DATA->imager_no_check_temperature || !PRIVATE_DATA->guider_no_check_temperature) {
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
	indigo_reschedule_timer(device, TEMP_CHECK_TIME, &PRIVATE_DATA->imager_ccd_temperature_timer);
}


static void guider_ccd_temperature_callback(indigo_device *device) {
	if (!PRIVATE_DATA->imager_no_check_temperature || !PRIVATE_DATA->guider_no_check_temperature) {
		pthread_mutex_lock(&driver_mutex);

		int res = set_sbig_handle(PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res)));
			pthread_mutex_unlock(&driver_mutex);
			return;
		}

		if (sbig_get_temperature(NULL, &(CCD_TEMPERATURE_ITEM->number.value), NULL, NULL) == CE_NO_ERROR) {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
	}
	indigo_reschedule_timer(device, TEMP_CHECK_TIME, &PRIVATE_DATA->guider_ccd_temperature_timer);
}


static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if ((device == PRIVATE_DATA->primary_ccd) && (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK)) {
		INFO_PROPERTY->count = 7; 	/* Use all info property fields */
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	} else if ((device != PRIVATE_DATA->primary_ccd) && (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK)) {
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;

	ok = sbig_start_exposure(device,
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
			PRIVATE_DATA->imager_ccd_exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback);
		} else {
			PRIVATE_DATA->imager_no_check_temperature = true;
			PRIVATE_DATA->imager_ccd_exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, imager_ccd_exposure_timer_callback);
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
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			if (sbig_open(device)) {
				GetCCDInfoParams cip;
				short res;
				if (PRIMARY_CCD) {
					pthread_mutex_lock(&driver_mutex);
					CCD_MODE_PROPERTY->hidden = false;
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_INFO_PROPERTY->hidden = false;

					cip.request = CCD_INFO_IMAGING; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_basic_info));
					if (res != CE_NO_ERROR) {
						INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res)));
					}

					//for (int mode = 0; mode < PRIVATE_DATA->imager_ccd_basic_info.readoutModes; mode++) {
					//	INDIGO_ERROR(indigo_error("indigo_ccd_sbig: %d. Mode = 0x%x %dx%d", mode, PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[mode].mode, PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[mode].width, PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[mode].height));
					//}
					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].height;
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = bcd2double(PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].pixelWidth);
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = bcd2double(PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].pixelHeight);
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%.2f", bcd2double(PRIVATE_DATA->imager_ccd_basic_info.firmwareVersion));
					sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", PRIVATE_DATA->imager_ccd_basic_info.name);

					cip.request = CCD_INFO_EXTENDED; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_extended_info1));
					if (res != CE_NO_ERROR) {
						INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res)));
					}

					strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->imager_ccd_extended_info1.serialNumber, INDIGO_VALUE_SIZE);

					indigo_update_property(device, INFO_PROPERTY, NULL);

					//INDIGO_ERROR(indigo_error("indigo_ccd_fli: FLIGetPixelSize(%d) = %f %f", id, size_x, size_y));

					cip.request = CCD_INFO_EXTENDED3; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_extended_info6));
					if (res != CE_NO_ERROR) {
						INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res)));
					}

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

					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.step = 0;

					res = sbig_get_temperature(&(CCD_COOLER_ON_ITEM->sw.value), &(CCD_TEMPERATURE_ITEM->number.value), NULL, &(CCD_COOLER_POWER_ITEM->number.value));
					CCD_COOLER_OFF_ITEM->sw.value = !CCD_COOLER_ON_ITEM->sw.value;
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_get_temperature() = %d (%s)", res, sbig_error_string(res)));
					}

					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
					PRIVATE_DATA->imager_no_check_temperature = false;

					CCD_COOLER_POWER_PROPERTY->hidden = false;
					CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

					PRIVATE_DATA->imager_buffer = indigo_alloc_blob_buffer(2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
					assert(PRIVATE_DATA->imager_buffer != NULL);

					PRIVATE_DATA->imager_ccd_temperature_timer = indigo_set_timer(device, 0, imager_ccd_temperature_callback);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					pthread_mutex_unlock(&driver_mutex);
				} else { /* Secondary CCD */
					pthread_mutex_lock(&driver_mutex);
					CCD_MODE_PROPERTY->hidden = false;
					CCD_COOLER_PROPERTY->hidden = true;
					CCD_INFO_PROPERTY->hidden = false;

					cip.request = CCD_INFO_TRACKING; /* guiding CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->guider_ccd_basic_info));
					if (res != CE_NO_ERROR) {
						INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res)));
					}

					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].height;
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = bcd2double(PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].pixelWidth);
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = bcd2double(PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].pixelHeight);
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%.2f", bcd2double(PRIVATE_DATA->guider_ccd_basic_info.firmwareVersion));
					sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", PRIVATE_DATA->guider_ccd_basic_info.name);

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

					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
					CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.step = 0;

					res = sbig_get_temperature(NULL, &(CCD_TEMPERATURE_ITEM->number.value), NULL, NULL);
					if (res) {
						INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_get_temperature() = %d (%s)", res, sbig_error_string(res)));
					}

					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
					PRIVATE_DATA->guider_no_check_temperature = false;

					CCD_COOLER_POWER_PROPERTY->hidden = true;

					PRIVATE_DATA->guider_buffer = indigo_alloc_blob_buffer(2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
					assert(PRIVATE_DATA->guider_buffer != NULL);

					PRIVATE_DATA->guider_ccd_temperature_timer = indigo_set_timer(device, 0, guider_ccd_temperature_callback);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					pthread_mutex_unlock(&driver_mutex);
				}
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		} else {
			if (PRIMARY_CCD) {
				PRIVATE_DATA->imager_no_check_temperature = false;
				indigo_cancel_timer(device, &PRIVATE_DATA->imager_ccd_temperature_timer);
				if (PRIVATE_DATA->imager_buffer != NULL) {
					free(PRIVATE_DATA->imager_buffer);
					PRIVATE_DATA->imager_buffer = NULL;
				}
			} else {
				PRIVATE_DATA->guider_no_check_temperature = false;
				indigo_cancel_timer(device, &PRIVATE_DATA->guider_ccd_temperature_timer);
				if (PRIVATE_DATA->guider_buffer != NULL) {
					free(PRIVATE_DATA->guider_buffer);
					PRIVATE_DATA->guider_buffer = NULL;
				}
			}
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
		if (PRIMARY_CCD) {
			PRIVATE_DATA->imager_no_check_temperature = false;
		} else {
			PRIVATE_DATA->guider_no_check_temperature = false;
		}
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
	// -------------------------------------------------------------------------------- CCD_COOLER
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
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

	pthread_mutex_lock(&driver_mutex);

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

	pthread_mutex_unlock(&driver_mutex);
}


static void guider_timer_callback_dec(indigo_device *device) {
	int res;
	ushort relay_map = 0;

	pthread_mutex_lock(&driver_mutex);

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

	pthread_mutex_unlock(&driver_mutex);
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
			pthread_mutex_lock(&driver_mutex);
			res = sbig_set_relays(driver_handle, RELAY_NORTH);
			if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_NORTH) = %d", driver_handle, res));
			PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
			PRIVATE_DATA->relay_map |= RELAY_NORTH;
			pthread_mutex_unlock(&driver_mutex);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&driver_mutex);
				res = sbig_set_relays(driver_handle, RELAY_SOUTH);
				if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_SOUTH) = %d", driver_handle, res));
				PRIVATE_DATA->guider_timer_dec = indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec);
				PRIVATE_DATA->relay_map |= RELAY_SOUTH;
				pthread_mutex_unlock(&driver_mutex);
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
			pthread_mutex_lock(&driver_mutex);
			res = sbig_set_relays(driver_handle, RELAY_EAST);
			if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_EAST) = %d", driver_handle, res));
			PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
			PRIVATE_DATA->relay_map |= RELAY_EAST;
			pthread_mutex_unlock(&driver_mutex);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&driver_mutex);
				res = sbig_set_relays(driver_handle, RELAY_WEST);
				if (res != CE_NO_ERROR) INDIGO_ERROR(indigo_error("indigo_ccd_sbig: sbig_set_relays(%d, RELAY_WEST) = %d", driver_handle, res));
				PRIVATE_DATA->guider_timer_ra = indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra);
				PRIVATE_DATA->relay_map |= RELAY_WEST;
				pthread_mutex_unlock(&driver_mutex);
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


static const char *camera_types[] = {
	"Type 0", "Type 1", "Type 2", "Type 3",
	"ST-7", "ST-8", "ST-5C", "TCE",
	"ST-237", "ST-K", "ST-9", "STV", "ST-10",
	"ST-1K", "ST-2K", "STL", "ST-402", "STX",
	"ST-4K", "STT", "ST-i",	"STF-8300",
	"Next Camera", "No Camera"
};


// -------------------------------------------------------------------------------- Ethernet support

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
	if (indigo_device_attach(device, DRIVER_VERSION, 0) == INDIGO_OK) {
		INFO_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		get_host_ip("localhost", &ip);
		strncpy(DEVICE_PORT_ITEM->text.value, "192.168.0.100", INDIGO_VALUE_SIZE);
		strncpy(DEVICE_PORT_PROPERTY->label, "Remote camera", INDIGO_VALUE_SIZE);
		strncpy(DEVICE_PORT_ITEM->label, "IP address / hostname", INDIGO_VALUE_SIZE);
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------

		INDIGO_LOG(indigo_log("%s attached", device->name));
		return indigo_device_enumerate_properties(device, NULL, NULL);
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
				pthread_mutex_lock(&driver_mutex);
				ok = plug_device(NULL, DEV_ETH, ip_address);
				pthread_mutex_unlock(&driver_mutex);
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
	return indigo_device_change_property(device, client, property);
}


static indigo_result eth_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_LOG(indigo_log("%s detached", device->name));
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support
short global_handle = INVALID_HANDLE_VALUE; /* This is global SBIG driver hangle used for attach and detatch cameras */

#define MAX_USB_DEVICES                8
#define MAX_ETH_DEVICES                8
#define MAX_DEVICES                   32

static indigo_device *devices[MAX_DEVICES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static indigo_device *sbig_eth = NULL;

static QueryUSBResults2 usb_cams = {0};
static QueryEthernetResults2 eth_cams = {0};


static inline int usb_to_index(SBIG_DEVICE_TYPE type) {
	return DEV_USB1 - type;
}


static inline SBIG_DEVICE_TYPE index_to_usb(int index) {
	return DEV_USB1 + index;
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
	GetCCDInfoParams gcp;
	GetCCDInfoResults0 gcir0;

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

	short res = set_sbig_handle(global_handle);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error set_sbig_handle(global_handle %d) = %d (%s)", global_handle, res, sbig_error_string(res)));
		/* Something wrong happened need to reopen the global handle */
		if ((res == CE_DRIVER_NOT_OPEN) || (res == CE_BAD_PARAMETER)) {
			res = sbig_command(CC_OPEN_DRIVER, NULL, NULL);
			if (res != CE_NO_ERROR) {
				INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DRIVER reopen error = %d (%s)", res, sbig_error_string(res)));
				return false;
			}
			global_handle = get_sbig_handle();
			if (global_handle == INVALID_HANDLE_VALUE) {
				INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error get_sbig_handle() = %d", global_handle));
				return false;
			}
		}
	}

	OpenDeviceParams odp = {
		.deviceType = device_type,
		.ipAddress = ip_address,
		.lptBaseAddress = 0x00
	};

	if ((res = sbig_command(CC_OPEN_DEVICE, &odp, NULL)) != CE_NO_ERROR) {
		sbig_command(CC_CLOSE_DEVICE, NULL, NULL); /* Cludge: sometimes it fails with CE_DEVICE_NOT_CLOSED later */
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DEVICE error = %d (%s)", res, sbig_error_string(res)));
		return false;
	}

	EstablishLinkParams elp = { .sbigUseOnly = 0 };
	EstablishLinkResults elr;

	if ((res = sbig_command(CC_ESTABLISH_LINK, &elp, &elr)) != CE_NO_ERROR) {
		sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_ESTABLISH_LINK error = %d (%s)", res, sbig_error_string(res)));
		return false;
	}

	/* Find camera name */
	if (cam_name == NULL) {
		gcp.request = CCD_INFO_IMAGING;
		if ((res = sbig_command(CC_GET_CCD_INFO, &gcp, &gcir0)) != CE_NO_ERROR) {
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_CCD_INFO error = %d (%s)", res, sbig_error_string(res)));
			return false;
		}
		cam_name = (char *)camera_types[gcir0.cameraType];
	}

	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: No device slots available."));
		return false;
	}
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: NEW cam: slot=%d device_type=0x%x name='%s' ip=0x%x", slot, device_type, cam_name, ip_address));
	indigo_device *device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &ccd_template, sizeof(indigo_device));
	sbig_private_data *private_data = malloc(sizeof(sbig_private_data));
	assert(private_data);
	memset(private_data, 0, sizeof(sbig_private_data));
	private_data->usb_id = device_type;
	private_data->ip_address = ip_address;
	//int device_index = 0;
	char device_index_str[20] = "NET";
	if (ip_address) {
		private_data->is_usb = false;
	} else {
		private_data->is_usb = true;
		sprintf(device_index_str, "%d", usb_to_index(device_type));
	}
	sprintf(device->name, "SBIG %s CCD #%s", cam_name, device_index_str);
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' attached.", device->name));
	private_data->primary_ccd = device;
	strncpy(private_data->dev_name, cam_name, MAX_PATH);
	device->private_data = private_data;
	indigo_async((void *)(void *)indigo_attach_device, device);
	devices[slot]=device;

	/* Creating guider device */
	slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_ERROR(indigo_error("indigo_ccd_asi: No device slots available."));
		return false;
	}
	device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &guider_template, sizeof(indigo_device));
	sprintf(device->name, "SBIG %s Guider Port #%s", cam_name, device_index_str);
	INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' attached.", device->name));
	device->private_data = private_data;
	indigo_async((void *)(void *)indigo_attach_device, device);
	devices[slot]=device;

	/* Check if there is secondary CCD and create device */
	gcp.request = CCD_INFO_TRACKING;

	if ((res = sbig_command(CC_GET_CCD_INFO, &gcp, &gcir0)) != CE_NO_ERROR) {
		INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: CC_GET_CCD_INFO error = %d (%s), asuming no Secondary CCD", res, sbig_error_string(res)));
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
		sprintf(device->name, "SBIG %s Guider CCD #%s", cam_name, device_index_str);
		INDIGO_LOG(indigo_log("indigo_ccd_sbig: '%s' attached.", device->name));
		device->private_data = private_data;
		indigo_async((void *)(void *)indigo_attach_device, device);
		devices[slot]=device;
	}
	sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
	return true;
}


static void enumerate_devices() {
	int res = sbig_command(CC_QUERY_USB2, NULL, &usb_cams);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_QUERY_USB2 error = %d (%s)", res, sbig_error_string(res)));
	}
	//INDIGO_LOG(indigo_log("indigo_ccd_sbig: usb_cams = %d", usb_cams.camerasFound));
	//INDIGO_LOG(indigo_log("indigo_ccd_sbig: usb_type = %d", usb_cams.usbInfo[0].cameraType ));
	//INDIGO_LOG(indigo_log("indigo_ccd_sbig: cam name = %s", usb_cams.usbInfo[0].name));
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
		if ((PRIVATE_DATA) && (!PRIVATE_DATA->is_usb)) continue;
		for (int dev_no = 0; dev_no < MAX_USB_DEVICES; dev_no++) {
			if (!usb_cams.usbInfo[dev_no].cameraFound) continue;
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
	pthread_mutex_lock(&driver_mutex);

	short res = set_sbig_handle(global_handle);
	if (res != CE_NO_ERROR) {
		INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error set_sbig_handle(global_handle) = %d (%s)", res, sbig_error_string(res)));
	}

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			char cam_name[MAX_PATH];
			int usb_id = find_plugged_device(cam_name);
			if (usb_id < 0) {
				INDIGO_DEBUG(indigo_debug("indigo_ccd_sbig: No SBIG Camera plugged."));
				pthread_mutex_unlock(&driver_mutex);
				return 0;
			}

			plug_device(NULL, usb_id, 0);
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
						pthread_mutex_unlock(&driver_mutex);
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
	pthread_mutex_unlock(&driver_mutex);
	return 0;
};


static void remove_usb_devices() {
	int i;
	sbig_private_data *pds[MAX_USB_DEVICES] = {NULL};

	for(i = 0; i < MAX_DEVICES; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) {
			if (!PRIVATE_DATA->is_usb) continue;
			pds[usb_to_index(PRIVATE_DATA->usb_id)] = PRIVATE_DATA; /* preserve pointers to private data */
		}
		indigo_detach_device(device);
		free(device);
		devices[i] = NULL;
	}

	/* free private data */
	for(i = 0; i < MAX_USB_DEVICES; i++) {
		if (pds[i]) free(pds[i]);
	}
}


static void remove_eth_devices() {
	int i;
	sbig_private_data *private_data = NULL;

	for(i = 0; i < MAX_DEVICES -1; i++) {
		indigo_device *device = devices[i];
		if (device == NULL) continue;
		if (PRIVATE_DATA) {
			if (PRIVATE_DATA->is_usb) continue;
			private_data = PRIVATE_DATA; /* preserve pointer to private data */
		}
		indigo_detach_device(device);
		free(device);
		devices[i] = NULL;
	}
	free(private_data);
}


static libusb_hotplug_callback_handle callback_handle;


indigo_result indigo_ccd_sbig(indigo_driver_action action, indigo_driver_info *info) {

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	static indigo_device sbig_eth_template = {
		"SBIG Ethernet Device", NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		eth_attach,
		indigo_device_enumerate_properties,
		eth_change_property,
		eth_detach
	};

	SET_DRIVER_INFO(info, "SBIG Camera", __FUNCTION__, DRIVER_VERSION, last_action);

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
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_OPEN_DRIVER error = %d (%s)", res, sbig_error_string(res)));
			return INDIGO_FAILED;
		}

		global_handle = get_sbig_handle();
		if (global_handle == INVALID_HANDLE_VALUE) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error get_sbig_handle() = %d", global_handle));
		}

		res = sbig_command(CC_GET_DRIVER_INFO, &di_req, &di);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_GET_DRIVER_INFO error = %d (%s)", res, sbig_error_string(res)));
		} else {
			INDIGO_LOG(indigo_log("indigo_ccd_sbig: Driver: %s (%x.%x)", di.name, di.version >> 8, di.version & 0x00ff));
		}

		sbig_eth = malloc(sizeof(indigo_device));
		assert(sbig_eth != NULL);
		memcpy(sbig_eth, &sbig_eth_template, sizeof(indigo_device));
		sbig_eth->private_data = NULL;
		indigo_attach_device(sbig_eth);

		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SBIG_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_sbig: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, rc < 0 ? libusb_error_name(rc) : "OK"));
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DEBUG_DRIVER(indigo_debug("indigo_ccd_sbig: libusb_hotplug_deregister_callback [%d]", __LINE__));
		remove_usb_devices();
		remove_eth_devices();
		indigo_detach_device(sbig_eth);
		free(sbig_eth);

		res = set_sbig_handle(global_handle);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: error set_sbig_handle() = %d (%s)", res, sbig_error_string(res)));
		}

		res = sbig_command(CC_CLOSE_DRIVER, NULL, NULL);
		if (res != CE_NO_ERROR) {
			INDIGO_ERROR(indigo_error("indigo_ccd_sbig: CC_CLOSE_DRIVER error = %d (%s)", res, sbig_error_string(res)));
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
