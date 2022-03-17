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
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

/** INDIGO CCD SBIG driver
 \file indigo_ccd_sbig.c
 */


#define DRIVER_VERSION 0x000B
#define DRIVER_NAME "indigo_ccd_sbig"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <pthread.h>
#include <sys/time.h>

#include <indigo/indigo_driver_xml.h>
#include "indigo_ccd_sbig.h"

#if !(defined(__APPLE__) && defined(__arm64__))

#include <sbigudrv.h>

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

#define DEFAULT_BPP          16     /* Default bits per pixel */

#define MAX_PATH            255     /* Maximal Path Length */

#define TEMP_THRESHOLD      0.5
#define TEMP_CHECK_TIME       3     /* Time between teperature checks (seconds) */

#define SBIG_VENDOR_ID             0x0d97

#define MAX_MODES                  32

#define PRIVATE_DATA               ((sbig_private_data *)device->private_data)
#define EXTERNAL_GUIDE_HEAD        (PRIVATE_DATA->guider_ccd_extended_info4.capabilitiesBits & CB_CCD_EXT_TRACKER_YES)

#define SBIG_ADVANCED_GROUP              "Advanced"

#define SBIG_FREEZE_TEC_PROPERTY         (PRIVATE_DATA->sbig_freeze_tec_property)
#define SBIG_FREEZE_TEC_ENABLED_ITEM     (SBIG_FREEZE_TEC_PROPERTY->items + 0)
#define SBIG_FREEZE_TEC_DISABLED_ITEM    (SBIG_FREEZE_TEC_PROPERTY->items + 1)

#define SBIG_ABG_PROPERTY                (PRIVATE_DATA->sbig_abg_property)
#define SBIG_ABG_LOW_ITEM                (SBIG_ABG_PROPERTY->items + 0)
#define SBIG_ABG_CLK_LOW_ITEM            (SBIG_ABG_PROPERTY->items + 1)
#define SBIG_ABG_CLK_MED_ITEM            (SBIG_ABG_PROPERTY->items + 2)
#define SBIG_ABG_CLK_HI_ITEM             (SBIG_ABG_PROPERTY->items + 3)




#define DEVICE_CONNECTED_MASK            0x01
#define PRIMARY_CCD_MASK                 0x02

#define DEVICE_CONNECTED                 (device->gp_bits & DEVICE_CONNECTED_MASK)
#define PRIMARY_CCD                      (device->gp_bits & PRIMARY_CCD_MASK)

#define set_connected_flag(dev)          ((dev)->gp_bits |= DEVICE_CONNECTED_MASK)
#define clear_connected_flag(dev)        ((dev)->gp_bits &= ~DEVICE_CONNECTED_MASK)

#define set_primary_ccd_flag(dev)        ((dev)->gp_bits |= PRIMARY_CCD_MASK)
#define clear_primary_ccd_flag(dev)      ((dev)->gp_bits &= ~PRIMARY_CCD_MASK)

// -------------------------------------------------------------------------------- SBIG USB interface implementation

typedef struct {
	/* General */
	bool is_usb;
	SBIG_DEVICE_TYPE usb_id;
	unsigned long ip_address;
	short driver_handle;
	char dev_name[MAX_PATH];
	int count_open;

	/* Imager CCD specific */
	indigo_timer *imager_ccd_exposure_timer, *imager_ccd_temperature_timer;
	GetCCDInfoResults0 imager_ccd_basic_info;
	GetCCDInfoResults2 imager_ccd_extended_info1;
	GetCCDInfoResults4 imager_ccd_extended_info4;
	GetCCDInfoResults6 imager_ccd_extended_info6;
	StartExposureParams2 imager_ccd_exp_params;
	ABG_STATE7 imager_abg_state;
	double target_temperature, current_temperature;
	double cooler_power;
	bool freeze_tec;
	bool imager_no_check_temperature;
	unsigned char *imager_buffer;
	indigo_property *sbig_freeze_tec_property;
	indigo_property *sbig_abg_property;

	/* Guider CCD Specific */
	indigo_timer *guider_ccd_exposure_timer, *guider_ccd_temperature_timer;
	GetCCDInfoResults0 guider_ccd_basic_info;
	StartExposureParams2 guider_ccd_exp_params;
	GetCCDInfoResults4 guider_ccd_extended_info4;
	bool guider_no_check_temperature;
	unsigned char *guider_buffer;

	/* Guider Specific */
	indigo_timer *guider_timer_ra, *guider_timer_dec;
	ushort relay_map;

	/* Filter wheel specific */
	indigo_timer *wheel_timer;
	int fw_device;
	int fw_count;
	int fw_current_slot;
	int fw_target_slot;

	/* AO Specific */
	double ao_x_deflection;
	double ao_y_deflection;
} sbig_private_data;

static pthread_mutex_t driver_mutex = PTHREAD_MUTEX_INITIALIZER;

short (*sbig_command)(short, void*, void*);
static void remove_usb_devices(void);
static void remove_eth_devices(void);
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

static char *sbig_error_string(long err) {
	GetErrorStringParams gesp;
	gesp.errorNo = err;
	static GetErrorStringResults gesr;
	int res = sbig_command(CC_GET_ERROR_STRING, &gesp, &gesr);
	if (res == CE_NO_ERROR) {
		return gesr.errorString;
	}
	static char str[128];
	sprintf(str, "Error string not found! Error code: %ld", err);
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

	INDIGO_DRIVER_DEBUG(DRIVER_NAME,"New driver handle = %d", *handle);
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

/*
static int sbig_link_status(GetLinkStatusResults *glsr) {
	int res = sbig_command(CC_GET_LINK_STATUS, NULL, glsr);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_LINK_STATUS error = %d (%s)", res, sbig_error_string(res));
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
*/

static int sbig_set_temperature(double t, bool enable) {
	int res;
	SetTemperatureRegulationParams2 strp2;
	strp2.regulation = enable ? REGULATION_ON : REGULATION_OFF;
	strp2.ccdSetpoint = t;
	res = sbig_command(CC_SET_TEMPERATURE_REGULATION2, &strp2, NULL);

	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_SET_TEMPERATURE_REGULATION2 error = %d (%s)", res, sbig_error_string(res));
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
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "regulation = %s, t = %.2g, setpoint = %.2g, power = %.2g",
			(qtsr2.coolingEnabled != 0) ? "True": "False",
			qtsr2.imagingCCDTemperature,
			qtsr2.ccdSetpoint,
			qtsr2.imagingCCDPower
		);
	}

	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_TEMPERATURE_STATUS error = %d (%s)", res, sbig_error_string(res));
	}
	return res;
}


static int sbig_freeze_tec(bool enable) {
	int res;

	bool cooler_on = false;
	sbig_get_temperature(&cooler_on, NULL, NULL, NULL);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Freeze TEC: cooler_on = %d, enable = %d", cooler_on, enable);
	if (!cooler_on) return CE_NO_ERROR;

	SetTemperatureRegulationParams2 strp2;
	strp2.regulation = enable ? REGULATION_FREEZE : REGULATION_UNFREEZE;
	strp2.ccdSetpoint = 0;
	res = sbig_command(CC_SET_TEMPERATURE_REGULATION2, &strp2, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_SET_TEMPERATURE_REGULATION2 freeze error = %d (%s)", res, sbig_error_string(res));
	}
	return res ;
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
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "*relay_map = Ox%x", *relay_map);

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


static int sbig_ao_tip_tilt(double x_deflection, double y_deflection) {
	assert(fabs(x_deflection) <= 1.0);
	assert(fabs(y_deflection) <= 1.0);

	int res;
	AOTipTiltParams aottp = {
		.xDeflection = round(fmin(4095.0, (1.0 + x_deflection) * 2048.0)),
		.yDeflection = round(fmin(4095.0, (1.0 + y_deflection) * 2048.0))
	};
	res = sbig_command(CC_AO_TIP_TILT, &aottp, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_AO_TIP_TILT error = %d (%s)", res, sbig_error_string(res));
	}
	return res;
}


static int sbig_ao_center() {
	int res;
	res = sbig_command(CC_AO_CENTER, NULL, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_AO_CENTER error = %d (%s)", res, sbig_error_string(res));
	}
	return res;
}


/* indigo CAMERA functions */

static indigo_result sbig_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if ((CONNECTION_CONNECTED_ITEM->sw.value) && (PRIMARY_CCD)) {
		if (indigo_property_match(SBIG_FREEZE_TEC_PROPERTY, property))
			indigo_define_property(device, SBIG_FREEZE_TEC_PROPERTY, NULL);
		if (indigo_property_match(SBIG_ABG_PROPERTY, property))
			indigo_define_property(device, SBIG_ABG_PROPERTY, NULL);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Bad CCD binning mode, use 1x1, 2x2, 3x3 or 9x9");
		return CE_BAD_PARAMETER;
	}
	return CE_NO_ERROR;
}


static int sbig_get_resolution(indigo_device *device, int bin_mode, int *width, int *height, double *pixel_width, double *pixel_height) {
	GetCCDInfoResults0 *ccd_info;

	if (PRIMARY_CCD) {
		ccd_info = &(PRIVATE_DATA->imager_ccd_basic_info);
	} else {
		ccd_info = &(PRIVATE_DATA->guider_ccd_basic_info);
	}

	for (int i = 0; i < ccd_info->readoutModes; i++) {
		if (ccd_info->readoutInfo[i].mode == bin_mode) {
			if (width) *width = ccd_info->readoutInfo[i].width;
			if (height) *height = ccd_info->readoutInfo[i].height;
			if (pixel_width) *pixel_width = ccd_info->readoutInfo[i].pixelWidth;
			if (pixel_height) *pixel_height = ccd_info->readoutInfo[i].pixelHeight;
			if ((*width == 0) || (*height == 0)) return CE_BAD_PARAMETER;
			return CE_NO_ERROR;
		}
	}
	return CE_BAD_PARAMETER;
}


static bool sbig_open(indigo_device *device) {
	OpenDeviceParams odp;
	short res;

	if (DEVICE_CONNECTED) return false;

	pthread_mutex_lock(&driver_mutex);
	if (PRIVATE_DATA->count_open++ == 0) {
		odp.deviceType = PRIVATE_DATA->usb_id;
		odp.ipAddress = PRIVATE_DATA->ip_address;
		odp.lptBaseAddress = 0x00;

		if ((res = open_driver(&PRIVATE_DATA->driver_handle)) != CE_NO_ERROR) {
			PRIVATE_DATA->driver_handle = INVALID_HANDLE_VALUE;
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&driver_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_OPEN_DRIVER error = %d (%s)", res, sbig_error_string(res));
			return false;
		}

		if ((res = sbig_command(CC_OPEN_DEVICE, &odp, NULL)) != CE_NO_ERROR) {
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL); /* Cludge: sometimes it fails with CE_DEVICE_NOT_CLOSED later */
			close_driver(&PRIVATE_DATA->driver_handle);
			PRIVATE_DATA->count_open--;
			pthread_mutex_unlock(&driver_mutex);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_OPEN_DEVICE error = %d (%s)", res, sbig_error_string(res));
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
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_ESTABLISH_LINK error = %d (%s)", res, sbig_error_string(res));
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
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
		sep->abgState = (unsigned short)PRIVATE_DATA->imager_abg_state;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Imager ABG mode = %d", PRIVATE_DATA->imager_abg_state);
	} else {
		sep = &(PRIVATE_DATA->guider_ccd_exp_params);
		sep->abgState = (unsigned short)ABG_LOW7;
		sep->ccd = EXTERNAL_GUIDE_HEAD ? CCD_EXT_TRACKING : CCD_TRACKING;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Using %s guider CCD.", EXTERNAL_GUIDE_HEAD ? "external" : "internal");

		unsigned short status;
		res = get_command_status(CC_START_EXPOSURE2, &status);
		if (res != CE_NO_ERROR) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_QUERY_COMMAND_STATUS error = %d (%s)", res, sbig_error_string(res));
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
		if ((status & 2) == 2) { /* Primary CCD exposure is in progress do not touch the shutter */
			shutter_mode = SC_LEAVE_SHUTTER;
		}
	}

	sep->openShutter = (unsigned short)shutter_mode;
	sep->exposureTime = (unsigned long)floor(exposure * 100.0 + 0.5);;
	sep->readoutMode = binning_mode;
	sep->left = (unsigned short)(offset_x / bin_x);
	sep->top = (unsigned short)(offset_y / bin_y);
	sep->width = (unsigned short)(frame_width / bin_x);
	sep->height = (unsigned short)(frame_height / bin_y);

	res = sbig_command(CC_START_EXPOSURE2, sep, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_START_EXPOSURE2 = %d (%s)", res, sbig_error_string(res));
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if (PRIMARY_CCD) {
		ccd = CCD_IMAGING;
	} else {
		ccd = EXTERNAL_GUIDE_HEAD ? CCD_EXT_TRACKING : CCD_TRACKING;
	}

	unsigned short status;
	res = get_command_status(CC_START_EXPOSURE2, &status);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_QUERY_COMMAND_STATUS error = %d (%s)", res, sbig_error_string(res));
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
	long wait_cycles = 6000;  /* 6000*2000us = 12s */
	unsigned char *frame_buffer;

	/* for the exposyre to complete and end it */
	while(!sbig_exposure_complete(device) && wait_cycles--) {
		usleep(2000);
	}
	if (wait_cycles == 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Exposure error: did not complete in time.");
	}

	pthread_mutex_lock(&driver_mutex);

	res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	/* freeze TEC if necessary */
	if ((PRIMARY_CCD) && (PRIVATE_DATA->freeze_tec)) {
		sbig_freeze_tec(true);
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
		srp.ccd = EXTERNAL_GUIDE_HEAD ? CCD_EXT_TRACKING : CCD_TRACKING;
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_END_EXPOSURE error = %d (%s)", res, sbig_error_string(res));
	}

	res = sbig_command(CC_START_READOUT, &srp, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_START_READOUT error = %d (%s)", res, sbig_error_string(res));
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
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_READOUT_LINE error = %d (%s)", res, sbig_error_string(res));
		}
	}

	EndReadoutParams erp = {
		.ccd = srp.ccd
	};
	res = sbig_command(CC_END_READOUT, &erp, NULL);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_END_READOUT error = %d (%s)", res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	/* Unfreeze tec */
	if (PRIMARY_CCD) {
		sbig_freeze_tec(false);
	}

	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static bool sbig_abort_exposure(indigo_device *device) {
	EndExposureParams eep;
	pthread_mutex_lock(&driver_mutex);

	int res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if (PRIMARY_CCD) {
		PRIVATE_DATA->imager_no_check_temperature = false;
		eep.ccd = CCD_IMAGING;
	} else {
		PRIVATE_DATA->guider_no_check_temperature = false;
		eep.ccd = EXTERNAL_GUIDE_HEAD ? CCD_EXT_TRACKING : CCD_TRACKING;
	}

	res = sbig_command(CC_END_EXPOSURE, &eep, NULL);
	if ( res != CE_NO_ERROR ) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_END_EXPOSURE error = %d (%s)", res, sbig_error_string(res));
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	res = sbig_get_temperature(&cooler_on, current, &csetpoint, cooler_power);
	if (res) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_get_temperature() = %d (%s)", res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if ((cooler_on != CCD_COOLER_ON_ITEM->sw.value) || (csetpoint != target)) {
		res = sbig_set_temperature(target, CCD_COOLER_ON_ITEM->sw.value);
		if(res) INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_temperature() = %d (%s)", res, sbig_error_string(res));
	}

	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static void sbig_close(indigo_device *device) {
	int res;

	if (!DEVICE_CONNECTED) return;

	pthread_mutex_lock(&driver_mutex);
	if (--PRIVATE_DATA->count_open == 0) {
		res = set_sbig_handle(PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		}

		res = sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_CLOSE_DEVICE error = %d (%s) - Ignore if device has been unplugged!", res, sbig_error_string(res));
		}

		res = close_driver(&PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "close_driver(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		}
	}
	pthread_mutex_unlock(&driver_mutex);
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static const char *camera_type[] = {
	"Type 0", "Type 1", "Type 2", "Type 3",
	"ST-7", "ST-8", "ST-5C", "TCE",
	"ST-237", "ST-K", "ST-9", "STV", "ST-10",
	"ST-1K", "ST-2K", "STL", "ST-402", "STX",
	"ST-4K", "STT", "ST-i",	"STF-8300",
	"Next Camera", "No Camera"
};


// callback for image download
static void imager_ccd_exposure_timer_callback(indigo_device *device) {
	unsigned char *frame_buffer;
	indigo_fits_keyword *bayer_keys = NULL;
	static indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "BAYERPAT", .string = "BGGR", "Bayer color pattern" }, /* index 0 */
		{ INDIGO_FITS_NUMBER, "XBAYROFF", .number = 0, "X offset of Bayer array" }, /* index 1 */
		{ INDIGO_FITS_NUMBER, "YBAYROFF", .number = 0, "Y offset of Bayer array" }, /* index 2 */
		{ 0 }
	};

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

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
					bayer_keys[1].number = ((int)(CCD_FRAME_LEFT_ITEM->number.value) % 2) ? 1 : 0; /* set XBAYROFF */
					bayer_keys[2].number = ((int)(CCD_FRAME_TOP_ITEM->number.value) % 2) ? 1 : 0; /* set YBAYROFF */
				}
			} else {
				frame_buffer = PRIVATE_DATA->guider_buffer;
			}
			// FIXME: race between capture and frame size/bpp
			indigo_process_image(device, frame_buffer, (int)(CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value),
			                    (int)(CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value), (int)(CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value), true, true, bayer_keys, false);
			CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		} else {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
		}
	}
	PRIVATE_DATA->imager_no_check_temperature = false;
}


// callback called 4s before image download (e.g. to clear vreg or turn off temperature check)
static void clear_reg_timer_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->imager_no_check_temperature = true;
		indigo_set_timer(device, 4, imager_ccd_exposure_timer_callback, &PRIVATE_DATA->imager_ccd_exposure_timer);
	} else {
		PRIVATE_DATA->imager_ccd_exposure_timer = NULL;
	}
}


static void imager_ccd_temperature_callback(indigo_device *device) {
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
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
	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;
	if (!PRIVATE_DATA->imager_no_check_temperature || !PRIVATE_DATA->guider_no_check_temperature) {
		pthread_mutex_lock(&driver_mutex);

		int res = set_sbig_handle(PRIVATE_DATA->driver_handle);
		if (res) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
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
	if (PRIMARY_CCD && (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK)) {
		INFO_PROPERTY->count = 8; 	/* Use all info property fields */

		SBIG_FREEZE_TEC_PROPERTY = indigo_init_switch_property(NULL, device->name, "SBIG_FREEZE_TEC", SBIG_ADVANCED_GROUP,"Freeze TEC during readout", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (SBIG_FREEZE_TEC_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		SBIG_FREEZE_TEC_PROPERTY->hidden = false;

		indigo_init_switch_item(SBIG_FREEZE_TEC_ENABLED_ITEM, "SBIG_FREEZE_TEC_ENABLED", "Enabled", false);
		indigo_init_switch_item(SBIG_FREEZE_TEC_DISABLED_ITEM, "SBIG_FREEZE_TEC_DISABLED", "Disabled", true);

		SBIG_ABG_PROPERTY = indigo_init_switch_property(NULL, device->name, "SBIG_ABG_STATE", SBIG_ADVANCED_GROUP,"ABG State", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
		if (SBIG_ABG_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}

		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "imager_ccd_extended_info1.imagingABG = %d", PRIVATE_DATA->imager_ccd_extended_info1.imagingABG);
		SBIG_ABG_PROPERTY->hidden = (PRIVATE_DATA->imager_ccd_extended_info1.imagingABG != ABG_PRESENT) ? true : false;

		indigo_init_switch_item(SBIG_ABG_LOW_ITEM, "SBIG_ABG_LOW", "Clock Low, No ABG", true);
		indigo_init_switch_item(SBIG_ABG_CLK_LOW_ITEM, "SBIG_ABG_CLK_LOW", "Clock Low, ABG", false);
		indigo_init_switch_item(SBIG_ABG_CLK_MED_ITEM, "SBIG_ABG_CLK_MED", "Clock Medium, ABG", false);
		indigo_init_switch_item(SBIG_ABG_CLK_HI_ITEM, "SBIG_ABG_CLK_LOW_HI", "Clock High, ABG", false);

		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	} else if ((!PRIMARY_CCD) && (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK)) {
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static bool handle_exposure_property(indigo_device *device, indigo_property *property) {
	long ok;

	ok = sbig_start_exposure(device,
	                         CCD_EXPOSURE_ITEM->number.target,
	                         CCD_FRAME_TYPE_DARK_ITEM->sw.value || CCD_FRAME_TYPE_DARKFLAT_ITEM->sw.value || CCD_FRAME_TYPE_BIAS_ITEM->sw.value,
	                         CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value,
	                         CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value,
	                         CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value
	);

	if (ok) {
		if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
		}
		if (CCD_UPLOAD_MODE_CLIENT_ITEM->sw.value || CCD_UPLOAD_MODE_BOTH_ITEM->sw.value) {
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		}
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);

		if (CCD_EXPOSURE_ITEM->number.target > 4) {
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target - 4, clear_reg_timer_callback, &PRIVATE_DATA->imager_ccd_exposure_timer);
		} else {
			PRIVATE_DATA->imager_no_check_temperature = true;
			indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.target, imager_ccd_exposure_timer_callback, &PRIVATE_DATA->imager_ccd_exposure_timer);
		}
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed.");
	}
	return false;
}

static void ccd_connect_callback(indigo_device *device) {
	char b1[32];
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (sbig_open(device)) {
				GetCCDInfoParams cip;
				short res;
				if (PRIMARY_CCD) {
					pthread_mutex_lock(&driver_mutex);
					CCD_MODE_PROPERTY->hidden = false;
					CCD_COOLER_PROPERTY->hidden = false;
					CCD_INFO_PROPERTY->hidden = false;

					res = set_sbig_handle(PRIVATE_DATA->driver_handle);
					if ( res != CE_NO_ERROR ) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
					}

					cip.request = CCD_INFO_IMAGING; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_basic_info));
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res));
					}

					//for (int mode = 0; mode < PRIVATE_DATA->imager_ccd_basic_info.readoutModes; mode++) {
					//	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%d. Mode = 0x%x %dx%d", mode, PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[mode].mode, PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[mode].width, PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[mode].height);
					//}

					indigo_define_property(device, SBIG_FREEZE_TEC_PROPERTY, NULL);
					indigo_define_property(device, SBIG_ABG_PROPERTY, NULL);

					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].height;
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = bcd2double(PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].pixelWidth);
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = bcd2double(PRIVATE_DATA->imager_ccd_basic_info.readoutInfo[0].pixelHeight);
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%s", indigo_dtoa(bcd2double(PRIVATE_DATA->imager_ccd_basic_info.firmwareVersion), b1));
					sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", PRIVATE_DATA->imager_ccd_basic_info.name);

					cip.request = CCD_INFO_EXTENDED; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_extended_info1));
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res));
					}

					indigo_copy_value(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, PRIVATE_DATA->imager_ccd_extended_info1.serialNumber);

					indigo_update_property(device, INFO_PROPERTY, NULL);

					cip.request = CCD_INFO_EXTENDED3; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_extended_info6));
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res));
					}

					cip.request = CCD_INFO_EXTENDED2_IMAGING; /* imaging CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->imager_ccd_extended_info4));
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res));
					}
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "imager_ccd_extended_info4.capabilitiesBits = 0x%x", PRIVATE_DATA->imager_ccd_extended_info4.capabilitiesBits);

					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = DEFAULT_BPP;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = DEFAULT_BPP;

					CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
					CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;

					CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
					char name[32];
					int count = 0;
					int width, height, max_bin = 1;

					if (sbig_get_resolution(device, RM_1X1, &width, &height, NULL, NULL) == CE_NO_ERROR) {
						sprintf(name, "RAW 16 %dx%d", width, height);
						indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
						count++;
						max_bin = 1;
					}
					if (sbig_get_resolution(device, RM_2X2, &width, &height, NULL, NULL) == CE_NO_ERROR) {
						sprintf(name, "RAW 16 %dx%d", width, height);
						indigo_init_switch_item(CCD_MODE_ITEM+count, "BIN_2x2", name, false);
						count++;
						max_bin = 2;
					}
					if (sbig_get_resolution(device, RM_3X3, &width, &height, NULL, NULL) == CE_NO_ERROR) {
						sprintf(name, "RAW 16 %dx%d", width, height);
						indigo_init_switch_item(CCD_MODE_ITEM+count, "BIN_3x3", name, false);
						count++;
						max_bin = 3;
					}
					CCD_MODE_PROPERTY->count = count;
					CCD_BIN_HORIZONTAL_ITEM->number.max = max_bin;
					CCD_BIN_VERTICAL_ITEM->number.max = max_bin;
					CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
					CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;

					CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;

					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.step = 0;

					res = sbig_get_temperature(&(CCD_COOLER_ON_ITEM->sw.value), &(CCD_TEMPERATURE_ITEM->number.value), NULL, &(CCD_COOLER_POWER_ITEM->number.value));
					CCD_COOLER_OFF_ITEM->sw.value = !CCD_COOLER_ON_ITEM->sw.value;
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_get_temperature() = %d (%s)", res, sbig_error_string(res));
					}

					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
					PRIVATE_DATA->imager_no_check_temperature = false;

					CCD_COOLER_POWER_PROPERTY->hidden = false;
					CCD_COOLER_POWER_PROPERTY->perm = INDIGO_RO_PERM;

					PRIVATE_DATA->imager_buffer = indigo_alloc_blob_buffer(2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
					assert(PRIVATE_DATA->imager_buffer != NULL);

					indigo_set_timer(device, 0, imager_ccd_temperature_callback, &PRIVATE_DATA->imager_ccd_temperature_timer);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					pthread_mutex_unlock(&driver_mutex);
				} else { /* Secondary CCD */
					pthread_mutex_lock(&driver_mutex);
					CCD_MODE_PROPERTY->hidden = false;
					CCD_COOLER_PROPERTY->hidden = true;
					CCD_INFO_PROPERTY->hidden = false;

					res = set_sbig_handle(PRIVATE_DATA->driver_handle);
					if ( res != CE_NO_ERROR ) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
					}

					cip.request = CCD_INFO_TRACKING; /* guiding CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->guider_ccd_basic_info));
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res));
					}

					CCD_INFO_WIDTH_ITEM->number.value = PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].width;
					CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].height;
					CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
					CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;

					CCD_INFO_PIXEL_WIDTH_ITEM->number.value = bcd2double(PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].pixelWidth);
					CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = bcd2double(PRIVATE_DATA->guider_ccd_basic_info.readoutInfo[0].pixelHeight);
					CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%s", indigo_dtoa(bcd2double(PRIVATE_DATA->guider_ccd_basic_info.firmwareVersion), b1));
					sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "%s", PRIVATE_DATA->guider_ccd_basic_info.name);

					indigo_update_property(device, INFO_PROPERTY, NULL);

					cip.request = CCD_INFO_EXTENDED2_TRACKING; /* Guider CCD */
					res = sbig_command(CC_GET_CCD_INFO, &cip, &(PRIVATE_DATA->guider_ccd_extended_info4));
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO(%d) = %d (%s)", cip.request, res, sbig_error_string(res));
					}
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "guider_ccd_extended_info4.capabilitiesBits = 0x%x", PRIVATE_DATA->guider_ccd_extended_info4.capabilitiesBits);

					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = DEFAULT_BPP;
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = DEFAULT_BPP;

					CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
					CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
					CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;

					CCD_MODE_PROPERTY->perm = INDIGO_RW_PERM;
					char name[32];
					int count = 0;
					int width, height, max_bin = 1;

					if (sbig_get_resolution(device, RM_1X1, &width, &height, NULL, NULL) == CE_NO_ERROR) {
						sprintf(name, "RAW 16 %dx%d", width, height);
						indigo_init_switch_item(CCD_MODE_ITEM, "BIN_1x1", name, true);
						count++;
						max_bin = 1;
					}
					if (sbig_get_resolution(device, RM_2X2, &width, &height, NULL, NULL) == CE_NO_ERROR) {
						sprintf(name, "RAW 16 %dx%d", width, height);
						indigo_init_switch_item(CCD_MODE_ITEM+count, "BIN_2x2", name, false);
						count++;
						max_bin = 2;
					}
					if (sbig_get_resolution(device, RM_3X3, &width, &height, NULL, NULL) == CE_NO_ERROR) {
						sprintf(name, "RAW 16 %dx%d", width, height);
						indigo_init_switch_item(CCD_MODE_ITEM+count, "BIN_3x3", name, false);
						count++;
						max_bin = 3;
					}
					CCD_MODE_PROPERTY->count = count;
					CCD_BIN_HORIZONTAL_ITEM->number.max = max_bin;
					CCD_BIN_VERTICAL_ITEM->number.max = max_bin;
					CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
					CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;

					CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = DEFAULT_BPP;

					CCD_TEMPERATURE_PROPERTY->hidden = false;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
					CCD_TEMPERATURE_ITEM->number.min = MIN_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.max = MAX_CCD_TEMP;
					CCD_TEMPERATURE_ITEM->number.step = 0;

					res = sbig_get_temperature(NULL, &(CCD_TEMPERATURE_ITEM->number.value), NULL, NULL);
					if (res) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_get_temperature() = %d (%s)", res, sbig_error_string(res));
					}

					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
					PRIVATE_DATA->guider_no_check_temperature = false;

					CCD_COOLER_POWER_PROPERTY->hidden = true;

					PRIVATE_DATA->guider_buffer = indigo_alloc_blob_buffer(2 * CCD_INFO_WIDTH_ITEM->number.value * CCD_INFO_HEIGHT_ITEM->number.value + FITS_HEADER_SIZE);
					assert(PRIVATE_DATA->guider_buffer != NULL);

					indigo_set_timer(device, 0, guider_ccd_temperature_callback, &PRIVATE_DATA->guider_ccd_temperature_timer);
					CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
					pthread_mutex_unlock(&driver_mutex);
				}
				set_connected_flag(device);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else {  /* Disconnect */
		if (DEVICE_CONNECTED) {
			if (PRIMARY_CCD) {
				PRIVATE_DATA->imager_no_check_temperature = false;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->imager_ccd_temperature_timer);
				indigo_delete_property(device, SBIG_FREEZE_TEC_PROPERTY, NULL);
				indigo_delete_property(device, SBIG_ABG_PROPERTY, NULL);
				if (PRIVATE_DATA->imager_buffer != NULL) {
					free(PRIVATE_DATA->imager_buffer);
					PRIVATE_DATA->imager_buffer = NULL;
				}
			} else { /* Secondary CCD */
				PRIVATE_DATA->guider_no_check_temperature = false;
				indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_ccd_temperature_timer);
				if (PRIVATE_DATA->guider_buffer != NULL) {
					free(PRIVATE_DATA->guider_buffer);
					PRIVATE_DATA->guider_buffer = NULL;
				}
			}
			sbig_close(device);
			clear_connected_flag(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	// -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_EXPOSURE
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			indigo_use_shortest_exposure_if_bias(device);
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
	// -------------------------------------------------------------------------------- CCD_BIN
	} else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		int prev_bin_x = (int)CCD_BIN_HORIZONTAL_ITEM->number.value;
		int prev_bin_y = (int)CCD_BIN_VERTICAL_ITEM->number.value;

		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);

		/* SBIG requires BIN_X and BIN_Y to be equal, so keep them entangled */
		if ((int)CCD_BIN_HORIZONTAL_ITEM->number.value != prev_bin_x) {
			CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.value;
		} else if ((int)CCD_BIN_VERTICAL_ITEM->number.value != prev_bin_y) {
			CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.value;
		}
		/* let the base base class handle the rest with the manipulated property values */
		return indigo_ccd_change_property(device, client, CCD_BIN_PROPERTY);
	// -------------------------------------------------------------------------------- CCD_COOLER
	} else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
		indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
			CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CCD_TEMPERATURE
	} else if (indigo_property_match_w(CCD_TEMPERATURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
			CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			if (CCD_COOLER_ON_ITEM->sw.value) {
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f", PRIVATE_DATA->target_temperature);
			} else {
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target Temperature = %.2f but the cooler is OFF, ", PRIVATE_DATA->target_temperature);
			}
		}
		return INDIGO_OK;
	// ------------------------------------------------------------------------------- CCD_FRAME
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.target = 8 * (int)(CCD_FRAME_WIDTH_ITEM->number.value / 8);
		CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.target = 2 * (int)(CCD_FRAME_HEIGHT_ITEM->number.value / 2);

		if (CCD_FRAME_WIDTH_ITEM->number.value / CCD_BIN_HORIZONTAL_ITEM->number.value < 64) {
			CCD_FRAME_WIDTH_ITEM->number.value = 64 * CCD_BIN_HORIZONTAL_ITEM->number.value;
		}
		if (CCD_FRAME_HEIGHT_ITEM->number.value / CCD_BIN_VERTICAL_ITEM->number.value < 64) {
			CCD_FRAME_HEIGHT_ITEM->number.value = 64 * CCD_BIN_VERTICAL_ITEM->number.value;
		}

		if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value < 12.0) {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8.0;
		} else {
			CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16.0;
		}

		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- FREEZE TEC
	} else if ((PRIMARY_CCD) && (indigo_property_match(SBIG_FREEZE_TEC_PROPERTY, property))) {
		indigo_property_copy_values(SBIG_FREEZE_TEC_PROPERTY, property, false);
		SBIG_FREEZE_TEC_PROPERTY->state = INDIGO_OK_STATE;
		if (SBIG_FREEZE_TEC_ENABLED_ITEM->sw.value) {
			PRIVATE_DATA->freeze_tec = true;
		} else {
			PRIVATE_DATA->freeze_tec = false;
		}
		if (IS_CONNECTED) {
			indigo_update_property(device, SBIG_FREEZE_TEC_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// --------------------------------------------------------------------------------- ABG
	} else if ((PRIMARY_CCD) && (indigo_property_match(SBIG_ABG_PROPERTY, property))) {
		indigo_property_copy_values(SBIG_ABG_PROPERTY, property, false);
		SBIG_ABG_PROPERTY->state = INDIGO_OK_STATE;

		if (SBIG_ABG_LOW_ITEM->sw.value) {
			PRIVATE_DATA->imager_abg_state = ABG_LOW7;
		} else if (SBIG_ABG_CLK_LOW_ITEM->sw.value) {
			PRIVATE_DATA->imager_abg_state = ABG_CLK_LOW7;
		} else if (SBIG_ABG_CLK_MED_ITEM->sw.value) {
			PRIVATE_DATA->imager_abg_state = ABG_CLK_MED7;
		} else if (SBIG_ABG_CLK_HI_ITEM->sw.value) {
			PRIVATE_DATA->imager_abg_state = ABG_CLK_HI7;
		} else {
			PRIVATE_DATA->imager_abg_state = ABG_LOW7;
		}
		if (IS_CONNECTED) {
			indigo_update_property(device, SBIG_ABG_PROPERTY, NULL);
		}
		return INDIGO_OK;
	// -------------------------------------------------------------------------------- CONFIG
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, SBIG_FREEZE_TEC_PROPERTY);
			indigo_save_property(device, NULL, SBIG_ABG_PROPERTY);
		}
	}
	// -----------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	if (PRIMARY_CCD) {
		indigo_release_property(SBIG_FREEZE_TEC_PROPERTY);
		indigo_release_property(SBIG_ABG_PROPERTY);
	}

	return indigo_ccd_detach(device);
}

/* indigo GUIDER functions */

static indigo_result guider_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_guider_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		return indigo_guider_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void guider_timer_callback_ra(indigo_device *device) {
	int res;
	ushort relay_map = 0;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&driver_mutex);

	PRIVATE_DATA->guider_timer_ra = NULL;
	int driver_handle = PRIVATE_DATA->driver_handle;

	res = sbig_get_relaymap(driver_handle, &relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_get_relaymap(%d) = %d (%s)", driver_handle, res, sbig_error_string(res));
	}

	relay_map &= ~(RELAY_EAST | RELAY_WEST);

	res = sbig_set_relaymap(driver_handle, relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_relaymap(%d) = %d (%s)", driver_handle, res, sbig_error_string(res));
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

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&driver_mutex);

	PRIVATE_DATA->guider_timer_ra = NULL;
	int driver_handle = PRIVATE_DATA->driver_handle;

	res = sbig_get_relaymap(driver_handle, &relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_get_relaymap(%d) = %d (%s)", driver_handle, res, sbig_error_string(res));
	}

	relay_map &= ~(RELAY_NORTH | RELAY_SOUTH);

	res = sbig_set_relaymap(driver_handle, relay_map);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_relaymap(%d) = %d (%s)", driver_handle, res, sbig_error_string(res));
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

static void guider_connect_callback(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (sbig_open(device)) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				GUIDER_GUIDE_DEC_PROPERTY->hidden = false;
				GUIDER_GUIDE_RA_PROPERTY->hidden = false;
				set_connected_flag(device);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else { /* disconnect */
		if (DEVICE_CONNECTED) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_dec);
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->guider_timer_ra);
			sbig_close(device);
			clear_connected_flag(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		}
	}
	indigo_guider_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	ushort res;
	int driver_handle = PRIVATE_DATA->driver_handle;

	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, guider_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
		indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
		indigo_cancel_timer(device, &PRIVATE_DATA->guider_timer_dec);
		int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
		if (duration > 0) {
			pthread_mutex_lock(&driver_mutex);
			res = sbig_set_relays(driver_handle, RELAY_NORTH);
			if (res != CE_NO_ERROR) INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_relays(%d, RELAY_NORTH) = %d (%s)", driver_handle, res, sbig_error_string(res));
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
			PRIVATE_DATA->relay_map |= RELAY_NORTH;
			pthread_mutex_unlock(&driver_mutex);
		} else {
			int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&driver_mutex);
				res = sbig_set_relays(driver_handle, RELAY_SOUTH);
				if (res != CE_NO_ERROR) INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_relays(%d, RELAY_SOUTH) = %d (%s)", driver_handle, res, sbig_error_string(res));
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_dec, &PRIVATE_DATA->guider_timer_dec);
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
			if (res != CE_NO_ERROR) INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_relays(%d, RELAY_EAST) = %d (%s)", driver_handle, res, sbig_error_string(res));
			indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
			PRIVATE_DATA->relay_map |= RELAY_EAST;
			pthread_mutex_unlock(&driver_mutex);
		} else {
			int duration = GUIDER_GUIDE_WEST_ITEM->number.value;
			if (duration > 0) {
				pthread_mutex_lock(&driver_mutex);
				res = sbig_set_relays(driver_handle, RELAY_WEST);
				if (res != CE_NO_ERROR) INDIGO_DRIVER_ERROR(DRIVER_NAME, "sbig_set_relays(%d, RELAY_WEST) = %d (%s)", driver_handle, res, sbig_error_string(res));
				indigo_set_timer(device, duration/1000.0, guider_timer_callback_ra, &PRIVATE_DATA->guider_timer_ra);
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
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		guider_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_guider_detach(device);
}

// -------------------------------------------------------------------------------- Ethernet support

bool get_host_ip(char *hostname , unsigned long *ip) {
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "getaddrinfo(): %s\n", gai_strerror(rv));
		return false;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if(p->ai_family == AF_INET) {
			*ip = ((struct sockaddr_in *)(p->ai_addr))->sin_addr.s_addr;
			/* ip should be litle endian */
			*ip = (*ip >> 24) | ((*ip << 8) & 0x00ff0000) | ((*ip >> 8) & 0x0000ff00) | (*ip << 24);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "IP: 0x%X\n", *ip);
			freeaddrinfo(servinfo);
			return true;
		}
	}
	freeaddrinfo(servinfo);
	return false;
}


static indigo_result eth_attach(indigo_device *device) {
	assert(device != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, 0) == INDIGO_OK) {
		INFO_PROPERTY->count = 2;
		// -------------------------------------------------------------------------------- SIMULATION
		SIMULATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- DEVICE_PORT
		DEVICE_PORT_PROPERTY->hidden = false;
		indigo_copy_value(DEVICE_PORT_ITEM->text.value, "192.168.0.100");
		indigo_copy_value(DEVICE_PORT_PROPERTY->label, "Remote camera");
		indigo_copy_value(DEVICE_PORT_ITEM->label, "IP address / hostname");
		// -------------------------------------------------------------------------------- DEVICE_PORTS
		DEVICE_PORTS_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_device_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


static void eth_connect_callback(indigo_device *device) {
	char message[1024] = {0};
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			snprintf(message, 1024, "Conneting to %s. This may take several minutes.", DEVICE_PORT_ITEM->text.value);
			indigo_update_property(device, CONNECTION_PROPERTY, message);
			unsigned long ip_address;
			bool ok;
			ok = get_host_ip(DEVICE_PORT_ITEM->text.value, &ip_address);
			if (ok) {
				ok = plug_device(NULL, DEV_ETH, ip_address);
			}
			if (ok) {
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				set_connected_flag(device);
				message[0] = '\0';
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, true);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				snprintf(message, 1024, "Conneting to %s failed.", DEVICE_PORT_ITEM->text.value);
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else { /* disconnect */
		if (DEVICE_CONNECTED) {
			remove_eth_devices();
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			clear_connected_flag(device);
		}
	}
	if (message[0] == '\0')
		indigo_device_change_property(device, NULL, CONNECTION_PROPERTY);
	else
		indigo_device_change_property(device, NULL, CONNECTION_PROPERTY);
}


static indigo_result eth_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	// -------------------------------------------------------------------------------- CONNECTION
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, eth_connect_callback, NULL);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}


static indigo_result eth_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		eth_connect_callback(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- FILTER WHEEL

static const char *cfw_type[] = {
	"CFW-Unknown", "CFW-2", "CFW-5",
	"CFW-8", "CFW-L", "CFW-402", "CFW-Auto",
	"CFW-6A", "CFW-10", "CFW-10 Serial",
	"CFW-9", "CFW-L8", "CFW-L8G", "CFW-1603",
	"FW5-STX", "FW5-8300", "FW8-8300",
	"FW7-STX", "FW8-STT", "FW5-STF"
};


static void wheel_timer_callback(indigo_device *device) {
	int res;

	if (!CONNECTION_CONNECTED_ITEM->sw.value) return;

	pthread_mutex_lock(&driver_mutex);
	res = set_sbig_handle(PRIVATE_DATA->driver_handle);
	if ( res != CE_NO_ERROR ) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return;
	}

	CFWParams cfwp = {
		.cfwModel = PRIVATE_DATA->fw_device,
		.cfwCommand = CFWC_QUERY
	};
	CFWResults cfwr;
	res = sbig_command(CC_CFW, &cfwp, &cfwr);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CFWC_QUERY error = %d (%s).", res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return;
	}
	pthread_mutex_unlock(&driver_mutex);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CFWC_QUERY cfwr.cfwPosition = %d", cfwr.cfwPosition);

	PRIVATE_DATA->fw_current_slot = cfwr.cfwPosition;
	if ((cfwr.cfwStatus == CFWS_IDLE) && (cfwr.cfwPosition == 0)) {
		/* Some FWs do not report their position */
		PRIVATE_DATA->fw_current_slot = PRIVATE_DATA->fw_target_slot;
	}
	WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->fw_current_slot;
	if (cfwr.cfwStatus == CFWS_IDLE) {
		WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		indigo_reschedule_timer(device, 0.5, &(PRIVATE_DATA->wheel_timer));
	}
	indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
}


static indigo_result wheel_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_wheel_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		return indigo_wheel_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void wheel_connect_callback(indigo_device *device) {
	int res;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (sbig_open(device)) {
				pthread_mutex_lock(&driver_mutex);
				res = set_sbig_handle(PRIVATE_DATA->driver_handle);
				if ( res != CE_NO_ERROR ) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
					pthread_mutex_unlock(&driver_mutex);
					return;
				}

				CFWParams cfwp = {
					.cfwModel = PRIVATE_DATA->fw_device,
					.cfwCommand = CFWC_OPEN_DEVICE
				};
				CFWResults cfwr;
				res = sbig_command(CC_CFW, &cfwp, &cfwr);
				if (res != CE_NO_ERROR) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CFWC_OPEN_DEVICE error = %d (%s).", res, sbig_error_string(res));
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					pthread_mutex_unlock(&driver_mutex);
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					return;
				}

				WHEEL_SLOT_ITEM->number.max = WHEEL_SLOT_NAME_PROPERTY->count = WHEEL_SLOT_OFFSET_PROPERTY->count = PRIVATE_DATA->fw_count;
				cfwp.cfwCommand = CFWC_QUERY;
				res = sbig_command(CC_CFW, &cfwp, &cfwr);
				if (res != CE_NO_ERROR) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CFWC_QUERY error = %d (%s).", res, sbig_error_string(res));
					cfwp.cfwCommand = CFWC_CLOSE_DEVICE;
					sbig_command(CC_CFW, &cfwp, &cfwr);
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					pthread_mutex_unlock(&driver_mutex);
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
					return;
				}

				if (cfwr.cfwPosition == 0) {
					INDIGO_DRIVER_LOG(DRIVER_NAME, "The attached filter wheel does not report current filter.");
					/*  Attached filter wheel does not report current poition => GOTO filter 1 */
					PRIVATE_DATA->fw_target_slot = 1;
					cfwp.cfwCommand = CFWC_GOTO;
					cfwp.cfwParam1 = PRIVATE_DATA->fw_target_slot;
					res = sbig_command(CC_CFW, &cfwp, &cfwr);
					if (res != CE_NO_ERROR) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "CFWC_GOTO error = %d (%s).", res, sbig_error_string(res));
						cfwp.cfwCommand = CFWC_CLOSE_DEVICE;
						sbig_command(CC_CFW, &cfwp, &cfwr);
						CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
						pthread_mutex_unlock(&driver_mutex);
						indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
						return;
					}
					/* set position to 1 */
					cfwr.cfwPosition = 1;
				}

				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CFWC_QUERY at connect cfwr.cfwPosition = %d", cfwr.cfwPosition);
				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_unlock(&driver_mutex);
				indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
				set_connected_flag(device);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else { /* disconnect */
		if(DEVICE_CONNECTED) {
			indigo_cancel_timer_sync(device, &PRIVATE_DATA->wheel_timer);
			pthread_mutex_lock(&driver_mutex);
			res = set_sbig_handle(PRIVATE_DATA->driver_handle);
			if ( res != CE_NO_ERROR ) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
				pthread_mutex_unlock(&driver_mutex);
				return;
			}
			CFWParams cfwp = {
				.cfwModel = PRIVATE_DATA->fw_device,
				.cfwCommand = CFWC_CLOSE_DEVICE
			};
			CFWResults cfwr;
			sbig_command(CC_CFW, &cfwp, &cfwr);
			pthread_mutex_unlock(&driver_mutex);
			sbig_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			clear_connected_flag(device);
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_wheel_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result wheel_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	int res;
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
		} else if (WHEEL_SLOT_ITEM->number.value == PRIVATE_DATA->fw_current_slot) {
			WHEEL_SLOT_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			WHEEL_SLOT_PROPERTY->state = INDIGO_BUSY_STATE;
			PRIVATE_DATA->fw_target_slot = WHEEL_SLOT_ITEM->number.value;
			WHEEL_SLOT_ITEM->number.value = PRIVATE_DATA->fw_current_slot;

			INDIGO_DRIVER_LOG(DRIVER_NAME, "Requested filter %d", PRIVATE_DATA->fw_target_slot);

			pthread_mutex_lock(&driver_mutex);
			res = set_sbig_handle(PRIVATE_DATA->driver_handle);
			if ( res != CE_NO_ERROR ) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				pthread_mutex_unlock(&driver_mutex);
				return INDIGO_FAILED;
			}

			CFWParams cfwp = {
				.cfwModel = PRIVATE_DATA->fw_device,
				.cfwCommand = CFWC_GOTO,
				.cfwParam1 = PRIVATE_DATA->fw_target_slot
			};
			CFWResults cfwr;
			res = sbig_command(CC_CFW, &cfwp, &cfwr);
			if (res != CE_NO_ERROR) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CFWC_GOTO error = %d (%s).", res, sbig_error_string(res));
				WHEEL_SLOT_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, WHEEL_SLOT_PROPERTY, NULL);
				pthread_mutex_unlock(&driver_mutex);
				return INDIGO_FAILED;
			}
			pthread_mutex_unlock(&driver_mutex);
			indigo_set_timer(device, 0.5, wheel_timer_callback, &PRIVATE_DATA->wheel_timer);
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

// -------------------------------------------------------------------------------- INDIGO AO device implementation

static indigo_result ao_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ao_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		AO_GUIDE_NORTH_ITEM->number.max = AO_GUIDE_SOUTH_ITEM->number.max = AO_GUIDE_EAST_ITEM->number.max = AO_GUIDE_WEST_ITEM->number.max = 100;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ao_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static void ao_connect_callback(indigo_device *device) {
	int res = CE_NO_ERROR;
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (!DEVICE_CONNECTED) {
			if (sbig_open(device)) {
				pthread_mutex_lock(&driver_mutex);
				res = set_sbig_handle(PRIVATE_DATA->driver_handle);
				if ( res != CE_NO_ERROR ) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
					pthread_mutex_unlock(&driver_mutex);
					return;
				}

				CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
				pthread_mutex_unlock(&driver_mutex);
				set_connected_flag(device);
			} else {
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
			}
		}
	} else { /* disconnect */
		if(DEVICE_CONNECTED) {
			pthread_mutex_lock(&driver_mutex);
			res = set_sbig_handle(PRIVATE_DATA->driver_handle);
			if ( res != CE_NO_ERROR ) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "set_sbig_handle(%d) = %d (%s)", PRIVATE_DATA->driver_handle, res, sbig_error_string(res));
				pthread_mutex_unlock(&driver_mutex);
				return;
			}
			pthread_mutex_unlock(&driver_mutex);
			sbig_close(device);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			clear_connected_flag(device);
		}
	}
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_ao_change_property(device, NULL, CONNECTION_PROPERTY);
}

static indigo_result ao_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	int res = CE_NO_ERROR;
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
		if (AO_GUIDE_NORTH_ITEM->number.value > 0) {
			PRIVATE_DATA->ao_y_deflection = AO_GUIDE_NORTH_ITEM->number.value / 100.0;
			res = sbig_ao_tip_tilt(PRIVATE_DATA->ao_x_deflection, PRIVATE_DATA->ao_y_deflection);
		} else if (AO_GUIDE_SOUTH_ITEM->number.value > 0) {
			PRIVATE_DATA->ao_y_deflection = AO_GUIDE_SOUTH_ITEM->number.value / -100.0;
			res = sbig_ao_tip_tilt(PRIVATE_DATA->ao_x_deflection, PRIVATE_DATA->ao_y_deflection);
		} else if (PRIVATE_DATA->ao_y_deflection != 0) {
			PRIVATE_DATA->ao_y_deflection = 0;
			res = sbig_ao_tip_tilt(PRIVATE_DATA->ao_x_deflection, PRIVATE_DATA->ao_y_deflection);
		}
		AO_GUIDE_NORTH_ITEM->number.value = AO_GUIDE_SOUTH_ITEM->number.value = 0;
		AO_GUIDE_DEC_PROPERTY->state = res == CE_NO_ERROR ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_GUIDE_RA_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_GUIDE_RA
		indigo_property_copy_values(AO_GUIDE_RA_PROPERTY, property, false);
		if (AO_GUIDE_WEST_ITEM->number.value > 0) {
			PRIVATE_DATA->ao_x_deflection = AO_GUIDE_WEST_ITEM->number.value / 100.0;
			res = sbig_ao_tip_tilt(PRIVATE_DATA->ao_x_deflection, PRIVATE_DATA->ao_y_deflection);
		} else if (AO_GUIDE_EAST_ITEM->number.value > 0) {
			PRIVATE_DATA->ao_x_deflection = AO_GUIDE_EAST_ITEM->number.value / -100.0;
			res = sbig_ao_tip_tilt(PRIVATE_DATA->ao_x_deflection, PRIVATE_DATA->ao_y_deflection);
		} else if (PRIVATE_DATA->ao_x_deflection != 0) {
			PRIVATE_DATA->ao_x_deflection = 0;
			res = sbig_ao_tip_tilt(PRIVATE_DATA->ao_x_deflection, PRIVATE_DATA->ao_y_deflection);
		}
		AO_GUIDE_WEST_ITEM->number.value = AO_GUIDE_EAST_ITEM->number.value = 0;
		AO_GUIDE_RA_PROPERTY->state = res == CE_NO_ERROR ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(AO_RESET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- AO_RESET
		indigo_property_copy_values(AO_RESET_PROPERTY, property, false);
		if (AO_CENTER_ITEM->sw.value || AO_UNJAM_ITEM->sw.value) {
			res = sbig_ao_center();
			PRIVATE_DATA->ao_x_deflection = PRIVATE_DATA->ao_y_deflection = 0;
			AO_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AO_GUIDE_DEC_PROPERTY, NULL);
			AO_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, AO_GUIDE_RA_PROPERTY, NULL);
		}
		AO_CENTER_ITEM->sw.value = AO_UNJAM_ITEM->sw.value = false;
		AO_RESET_PROPERTY->state = res == CE_NO_ERROR ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
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

// -------------------------------------------------------------------------------- hot-plug support
short global_handle = INVALID_HANDLE_VALUE; /* This is global SBIG driver hangle used for attach and detatch cameras */
pthread_mutex_t hotplug_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_USB_DEVICES                8
#define MAX_ETH_DEVICES                8
#define MAX_DEVICES                   32

static indigo_device *devices[MAX_DEVICES] = {NULL};
static indigo_device *sbig_eth = NULL;

static QueryUSBResults2 usb_cams = {0};


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


static int find_device_slot(SBIG_DEVICE_TYPE usb_id) {
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

	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		sbig_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	static indigo_device guider_template = INDIGO_DEVICE_INITIALIZER(
		"",
		guider_attach,
		indigo_guider_enumerate_properties,
		guider_change_property,
		NULL,
		guider_detach
	);

	static indigo_device wheel_template = INDIGO_DEVICE_INITIALIZER(
		"",
		wheel_attach,
		indigo_wheel_enumerate_properties,
		wheel_change_property,
		NULL,
		wheel_detach
	);

	static indigo_device ao_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ao_attach,
		indigo_ao_enumerate_properties,
		ao_change_property,
		NULL,
		ao_detach
	);

	pthread_mutex_lock(&driver_mutex);
	short res = set_sbig_handle(global_handle);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "error set_sbig_handle(global_handle %d) = %d (%s)", global_handle, res, sbig_error_string(res));
		/* Something wrong happened need to reopen the global handle */
		if ((res == CE_DRIVER_NOT_OPEN) || (res == CE_BAD_PARAMETER)) {
			res = sbig_command(CC_OPEN_DRIVER, NULL, NULL);
			if (res != CE_NO_ERROR) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_OPEN_DRIVER reopen error = %d (%s)", res, sbig_error_string(res));
				pthread_mutex_unlock(&driver_mutex);
				return false;
			}
			global_handle = get_sbig_handle();
			if (global_handle == INVALID_HANDLE_VALUE) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "error get_sbig_handle() = %d", global_handle);
				pthread_mutex_unlock(&driver_mutex);
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
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_OPEN_DEVICE error = %d (%s)", res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	EstablishLinkParams elp = { .sbigUseOnly = 0 };
	EstablishLinkResults elr;

	if ((res = sbig_command(CC_ESTABLISH_LINK, &elp, &elr)) != CE_NO_ERROR) {
		sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_ESTABLISH_LINK error = %d (%s)", res, sbig_error_string(res));
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	/* Find camera name */
	if (cam_name == NULL) {
		gcp.request = CCD_INFO_IMAGING;
		if ((res = sbig_command(CC_GET_CCD_INFO, &gcp, &gcir0)) != CE_NO_ERROR) {
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_CCD_INFO error = %d (%s)", res, sbig_error_string(res));
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
		cam_name = (char *)camera_type[gcir0.cameraType];
	}

	int slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "NEW camera: slot=%d device_type=0x%x name='%s' ip=0x%x", slot, device_type, cam_name, ip_address);
	indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
	sbig_private_data *private_data = indigo_safe_malloc(sizeof(sbig_private_data));
	private_data->usb_id = device_type;
	private_data->ip_address = ip_address;

	char device_index_str[20] = "NET";
	if (ip_address) {
		private_data->is_usb = false;
	} else {
		private_data->is_usb = true;
		sprintf(device_index_str, "%d", usb_to_index(device_type));
	}

	private_data->imager_abg_state = ABG_LOW7;
	sprintf(device->name, "SBIG %s CCD #%s", cam_name, device_index_str);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	set_primary_ccd_flag(device);
	strncpy(private_data->dev_name, cam_name, MAX_PATH);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;

	/* Creating guider device */
	slot = find_available_device_slot();
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	device = indigo_safe_malloc_copy(sizeof(indigo_device), &guider_template);
	sprintf(device->name, "SBIG %s Guider Port #%s", cam_name, device_index_str);
	INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
	device->private_data = private_data;
	indigo_attach_device(device);
	devices[slot]=device;

	/* Check if there is secondary CCD and create device */
	gcp.request = CCD_INFO_TRACKING;

	if ((res = sbig_command(CC_GET_CCD_INFO, &gcp, &gcir0)) != CE_NO_ERROR) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CC_GET_CCD_INFO error = %d (%s), asuming no Secondary CCD", res, sbig_error_string(res));
	} else {
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
			sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
		device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
		sprintf(device->name, "SBIG %s Guider CCD #%s", cam_name, device_index_str);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		device->private_data = private_data;
		clear_primary_ccd_flag(device);
		indigo_attach_device(device);
		devices[slot]=device;
	}

	/* Check it there is filter wheel present */
	CFWParams cfwp = {
		.cfwModel = CFWSEL_AUTO,
		.cfwCommand = CFWC_OPEN_DEVICE,
	};
	CFWResults cfwr;

	if ((res = sbig_command(CC_CFW, &cfwp, &cfwr)) == CE_NO_ERROR) {
		cfwp.cfwCommand = CFWC_GET_INFO;
		cfwp.cfwParam1 = CFWG_FIRMWARE_VERSION;
		if ((res = sbig_command(CC_CFW, &cfwp, &cfwr)) != CE_NO_ERROR) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CFWC_GET_INFO error = %d (%s), asuming no filter wheel", res, sbig_error_string(res));
		} else {
			if ((cfwr.cfwModel == CFWSEL_CFW8) || (cfwr.cfwModel == CFWSEL_CFW6A)) {
				/* These are legacy filter wheels and can not be autodetected
				   set env SBIG_LEGACY_CFW = CFW8 | CFW6A in order to use them
				*/
				cfwr.cfwModel = 0;
				if (getenv("SBIG_LEGACY_CFW") != NULL) {
					if (!strcmp(getenv("SBIG_LEGACY_CFW"),"CFW8")) {
						cfwr.cfwModel = CFWSEL_CFW8;
						cfwr.cfwResult2 = 5;
					} else if(!strcmp(getenv("SBIG_LEGACY_CFW"),"CFW6A")) {
						cfwr.cfwModel = CFWSEL_CFW6A;
						cfwr.cfwResult2 = 6;
					}
				}
			}

			if (cfwr.cfwModel != 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "cfwModel = %d (%s) cfwPosition = %d positions = %d cfwStatus = %d", cfwr.cfwModel, cfw_type[cfwr.cfwModel], cfwr.cfwPosition, cfwr.cfwResult2, cfwr.cfwStatus);
				int slot = find_available_device_slot();
				if (slot < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
					sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
					pthread_mutex_unlock(&driver_mutex);
					return false;
				}

				device = indigo_safe_malloc_copy(sizeof(indigo_device), &wheel_template);
				sprintf(device->name, "SBIG %s #%s", cfw_type[cfwr.cfwModel], device_index_str);
				INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
				private_data->fw_device = cfwr.cfwModel;
				private_data->fw_count = (int)cfwr.cfwResult2;
				device->private_data = private_data;
				indigo_attach_device(device);
				devices[slot]=device;
			}
		}

		cfwp.cfwCommand = CFWC_CLOSE_DEVICE;
		if ((res = sbig_command(CC_CFW, &cfwp, &cfwr)) != CE_NO_ERROR) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CFWC_CLOSE_DEVICE error = %d (%s)", res, sbig_error_string(res));
		}
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "CFWC_OPEN_DEVICE error = %d (%s), asuming no Secondary CCD", res, sbig_error_string(res));
	}

	/* Check it there is an AO device present */
	gcp.request = CCD_INFO_EXTENDED2_IMAGING; /* imaging CCD */
	if ((res = sbig_command(CC_GET_CCD_INFO, &gcp, &(PRIVATE_DATA->imager_ccd_extended_info4))) == CE_NO_ERROR) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "imager_ccd_extended_info4.capabilitiesBits = 0x%x", PRIVATE_DATA->imager_ccd_extended_info4.capabilitiesBits);
		if ((PRIVATE_DATA->imager_ccd_extended_info4.capabilitiesBits & 0x10) || getenv("SBIG_LEGACY_AO") != NULL) {
			if((res = sbig_ao_center()) == CE_NO_ERROR) {
				int slot = find_available_device_slot();
				if (slot < 0) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "No device slots available.");
					sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
					pthread_mutex_unlock(&driver_mutex);
					return false;
				}

				device = indigo_safe_malloc_copy(sizeof(indigo_device), &ao_template);
				sprintf(device->name, "SBIG AO #%s", device_index_str);
				INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
				private_data->ao_x_deflection = private_data->ao_y_deflection = 0;
				device->private_data = private_data;
				indigo_attach_device(device);
				devices[slot] = device;
			}
		}
	}


	sbig_command(CC_CLOSE_DEVICE, NULL, NULL);
	pthread_mutex_unlock(&driver_mutex);
	return true;
}


static void enumerate_devices() {
	pthread_mutex_lock(&driver_mutex);
	int res = set_sbig_handle(global_handle);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "error set_sbig_handle(global_handle) = %d (%s)", res, sbig_error_string(res));
	}

	res = sbig_command(CC_QUERY_USB2, NULL, &usb_cams);
	if (res != CE_NO_ERROR) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_QUERY_USB2 error = %d (%s)", res, sbig_error_string(res));
	}
	pthread_mutex_unlock(&driver_mutex);
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
	//pthread_mutex_lock(&hotplug_mutex);

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			char cam_name[MAX_PATH];

			int usb_id = find_plugged_device(cam_name);
			if (usb_id < 0) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No SBIG Camera plugged.");
				//pthread_mutex_unlock(&hotplug_mutex);
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
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "LEFT '%s' usb_id=0x%x, slot=%d", cam_name, usb_index, slot);
				while (slot >= 0) {
					indigo_device **device = &devices[slot];
					if (*device == NULL) {
						//pthread_mutex_unlock(&hotplug_mutex);
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
					if (private_data->imager_buffer) free(private_data->imager_buffer);
					if (private_data->guider_buffer) free(private_data->guider_buffer);
					free(private_data);
					private_data = NULL;
				}
			}

			if (!removed) {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "No SBIG Camera unplugged!");
			}
		}
	}
	//pthread_mutex_unlock(&hotplug_mutex);
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
		if (pds[i]) {
			sbig_private_data *private_data = (sbig_private_data*)pds[i];
			if (private_data->imager_buffer) free(private_data->imager_buffer);
			if (private_data->guider_buffer) free(private_data->guider_buffer);
			free(pds[i]);
		}
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
	if (private_data) {
		if (private_data->imager_buffer) free(private_data->imager_buffer);
		if (private_data->guider_buffer) free(private_data->guider_buffer);
		free(private_data);
	}
}

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_sbig(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	static indigo_device sbig_eth_template = INDIGO_DEVICE_INITIALIZER(
		"SBIG Ethernet Device",
		eth_attach,
		indigo_device_enumerate_properties,
		eth_change_property,
		NULL,
		eth_detach
	);

	SET_DRIVER_INFO(info, "SBIG Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

#ifdef __APPLE__
	static void *dl_handle = NULL;
#endif

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
#ifdef __linux__
		sbig_command = SBIGUnivDrvCommand;
#elif __APPLE__
		dl_handle = dlopen("/Library/Frameworks/SBIGUDrv.framework/SBIGUDrv", RTLD_LAZY);
		if (!dl_handle) {
			const char* dlsym_error = dlerror();
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SBIG SDK can't be loaded (%s)", dlsym_error);
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Please install SBIGUDrv framework from http://www.sbig.com");
			return INDIGO_FAILED;
		}
		sbig_command = dlsym(dl_handle, "SBIGUnivDrvCommand");
		const char* dlsym_error = dlerror();
		if (dlsym_error) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Can't load %s() (%s)", "SBIGUnivDrvCommand", dlsym_error);
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
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_OPEN_DRIVER error = %d (%s)", res, sbig_error_string(res));
			return INDIGO_FAILED;
		}

		global_handle = get_sbig_handle();
		if (global_handle == INVALID_HANDLE_VALUE) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "error get_sbig_handle() = %d", global_handle);
		}

		res = sbig_command(CC_GET_DRIVER_INFO, &di_req, &di);
		if (res != CE_NO_ERROR) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_GET_DRIVER_INFO error = %d (%s)", res, sbig_error_string(res));
		} else {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "Driver: %s (%x.%x)", di.name, di.version >> 8, di.version & 0x00ff);
		}

		sbig_eth = indigo_safe_malloc_copy(sizeof(indigo_device), &sbig_eth_template);
		sbig_eth->private_data = NULL;
		indigo_attach_device(sbig_eth);

		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, SBIG_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		if (rc >= 0) {
			last_action = action;
			return INDIGO_OK;
		}
		return INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		remove_usb_devices();
		remove_eth_devices();
		indigo_detach_device(sbig_eth);
		free(sbig_eth);

		res = set_sbig_handle(global_handle);
		if (res != CE_NO_ERROR) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "error set_sbig_handle() = %d (%s)", res, sbig_error_string(res));
		}

		res = sbig_command(CC_CLOSE_DRIVER, NULL, NULL);
		if (res != CE_NO_ERROR) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "CC_CLOSE_DRIVER error = %d (%s)", res, sbig_error_string(res));
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

#else

indigo_result indigo_ccd_sbig(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "SBIG Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);
	
	switch(action) {
		case INDIGO_DRIVER_INIT:
		case INDIGO_DRIVER_SHUTDOWN:
			return INDIGO_UNSUPPORTED_ARCH;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}

#endif
