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

#define DRIVER_VERSION 0x0004
#define DRIVER_NAME	"indigo_ccd_andor"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#include "indigo_driver_xml.h"
#include "indigo_ccd_andor.h"

/* Make it work with older SDK */

// #define SDK_PRIOR_2_92_30005_0

// #define NO_SETHIGHCAPACITY
#ifdef NO_SETHIGHCAPACITY
static unsigned int SetHighCapacity(int state) {
	INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetHighCapacity() call is not Supported by this version of the SDK.");
	return DRV_SUCCESS;
}
#endif

#ifndef AC_CAMERATYPE_IVAC_CCD
	#define AC_CAMERATYPE_IVAC_CCD     23
#endif

#ifndef AC_CAMERATYPE_IKONXL
	#define AC_CAMERATYPE_IKONXL       28
#endif

#ifndef AC_CAMERATYPE_ISTAR_SCMOS
	#define AC_CAMERATYPE_ISTAR_SCMOS  30
#endif

#ifndef AC_CAMERATYPE_IKONLR
	#define AC_CAMERATYPE_IKONLR       31
#endif

/* Temperature update interval in seconds */
#define TEMP_UPDATE         2.0

/* gp_bits is used as boolean */
#define is_connected                    gp_bits

/* ANDOR specifiv properties */
#define AQUISITION_GROUP_NAME           "Aquisition"
#define VSSPEED_PROPERTY_NAME           "ANDOR_VSSPEED"
#define VSAMPLITUDE_PROPERTY_NAME       "ANDOR_VSAMPLITUDE"
#define HREADOUT_PROPERTY_NAME          "ANDOR_HREADOUT"
#define PREAMPGAIN_PROPERTY_NAME        "ANDOR_PREAMPGAIN"
#define HIGHCAPACITY_PROPERTY_NAME      "ANDOR_HIGHCAPACITY"
#define BASELINECLAMP_PROPERTY_NAME     "ANDOR_BASELINECLAMP"
#define BASELINEOFFSET_PROPERTY_NAME    "ANDOR_BASELINEOFFSET"

#define COOLER_GROUP_NAME               "Cooler"
#define FANCONTROL_PROPERTY_NAME        "ANDOR_FANCONTROL"
#define COOLERMODE_PROPERTY_NAME        "ANDOR_COOLERMODE"

#define PRIVATE_DATA                    ((andor_private_data *)device->private_data)
#define VSSPEED_PROPERTY                PRIVATE_DATA->vsspeed_property
#define VSAMPLITUDE_PROPERTY            PRIVATE_DATA->vsamplitude_property
#define HREADOUT_PROPERTY               PRIVATE_DATA->hreadout_property
#define PREAMPGAIN_PROPERTY             PRIVATE_DATA->preampgain_property
#define HIGHCAPACITY_PROPERTY           PRIVATE_DATA->highcapacity_property
#define BASELINECLAMP_PROPERTY          PRIVATE_DATA->baselineclamp_property
#define BASELINEOFFSET_PROPERTY         PRIVATE_DATA->baselineoffset_property
#define FANCONTROL_PROPERTY             PRIVATE_DATA->fancontrol_property
#define COOLERMODE_PROPERTY             PRIVATE_DATA->coolermode_property

#define BASELINEOFFSET_OFFSET_ITEM      (BASELINEOFFSET_PROPERTY->items)

#define CAP_GET_TEMPERATURE             (PRIVATE_DATA->caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURE)
#define CAP_GET_TEMPERATURE_RANGE       (PRIVATE_DATA->caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURERANGE)
#define CAP_GET_TEMPERATURE_DURING_ACQUISITION (PRIVATE_DATA->caps.ulFeatures & AC_FEATURES_TEMPERATUREDURINGACQUISITION)
#define CAP_FANCONTROL                  (PRIVATE_DATA->caps.ulFeatures & AC_FEATURES_FANCONTROL)
#define CAP_MIDFANCONTROL               (PRIVATE_DATA->caps.ulFeatures & AC_FEATURES_MIDFANCONTROL)
#define CAP_SET_TEMPERATURE             (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_TEMPERATURE)
#define CAP_SET_VREADOUT                (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_VREADOUT)
#define CAP_SET_VSAMPLITUDE             (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_VSAMPLITUDE)
#define CAP_SET_HREADOUT                (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_HREADOUT)
#define CAP_SET_HIGHCAPACITY            (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_HIGHCAPACITY)
#define CAP_SET_PREAMPGAIN              (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN)
#define CAP_SET_BASELINECLAMP           (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_BASELINECLAMP)
#define CAP_GET_BASELINECLAMP           (PRIVATE_DATA->caps.ulGetFunctions & AC_GETFUNCTION_BASELINECLAMP)
#define CAP_SET_BASELINEOFFSET          (PRIVATE_DATA->caps.ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET)

#define HREADOUT_ITEM_FORMAT            "CHANNEL_%d_AMP_%d_SPEED_%d"

typedef struct {
	long handle;
	int index;
	int serial_number;
	indigo_property *vsspeed_property;
	indigo_property *vsamplitude_property;
	indigo_property *hreadout_property;
	indigo_property *highcapacity_property;
	indigo_property *preampgain_property;
	indigo_property *baselineclamp_property;
	indigo_property *baselineoffset_property;
	indigo_property *fancontrol_property;
	indigo_property *coolermode_property;

	unsigned char *buffer;
	long buffer_size;
	int adc_channels;
	int bit_depths[10];
	AndorCapabilities caps;
	bool no_check_temperature;
	float target_temperature, current_temperature, cooler_power;
	indigo_timer *exposure_timer, *temperature_timer;
} andor_private_data;

/* To avoid exposure failue when many cameras are present global mutex is required */
static pthread_mutex_t driver_mutex = PTHREAD_MUTEX_INITIALIZER;


int round_pow2(int num) {
    int next = num;
    next--;
    next |= next >> 1;
    next |= next >> 2;
    next |= next >> 4;
    next |= next >> 8;
    next |= next >> 16;
    next++;
    int prev = next >> 1;
    return (next - num) > (num - prev) ? prev : next;
}


// -------------------------------------------------------------------------------- INDIGO CCD device implementation
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
		strncpy(name,"Andor Unknown", size);
		return;
	}
}


static void fix_bpp(indigo_device *device) {
	/* Disable 8-bit while andor_read_pixels() does not support 8-bit */
	/* if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value <= 8.0) {
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 8.0;
	} else */
	if (CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value <= 16.0) {
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 16.0;
	} else {
		CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = 32.0;
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


static void init_vsspeed_property(indigo_device *device) {
	int res, option_num;
	res = GetNumberVSSpeeds(&option_num);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberVSSpeeds() for camera %d error: %d", PRIVATE_DATA->handle, res);
		option_num = 0;
	}
	VSSPEED_PROPERTY = indigo_init_switch_property(NULL, device->name, VSSPEED_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Vertical Shift Speed", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, option_num);
	if (VSSPEED_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of VSSPEED_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	for (int i = 0; i < option_num; i++) {
		float speed;
		char item[INDIGO_NAME_SIZE];
		char description[INDIGO_VALUE_SIZE];
		GetVSSpeed(i, &speed);
		snprintf(item, INDIGO_NAME_SIZE, "SPEED_%d", i);
		snprintf(description, INDIGO_VALUE_SIZE, "%.2fus", speed);
		indigo_init_switch_item(VSSPEED_PROPERTY->items + i, item, description, false);
	}
	if (option_num) VSSPEED_PROPERTY->items[0].sw.value = true;

	res = SetVSSpeed(0);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetVSSpeed() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
	indigo_define_property(device, VSSPEED_PROPERTY, NULL);
}


#ifdef SDK_PRIOR_2_92_30005_0  /* SDK is older */

static void init_vsamplitude_property(indigo_device *device) {
	int res, option_num;
	res = GetNumberVSAmplitudes(&option_num);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberVSAmplitudes() for camera %d error: %d", PRIVATE_DATA->handle, res);
		option_num = 0;
	}
	VSAMPLITUDE_PROPERTY = indigo_init_switch_property(NULL, device->name, VSAMPLITUDE_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Vertical Clock Amplitude", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, option_num);
	if (VSAMPLITUDE_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of VSAMPLITUDE_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	if (option_num > 0) indigo_init_switch_item(VSAMPLITUDE_PROPERTY->items + 0, "NORMAL", "Normal", true);
	if (option_num > 1) indigo_init_switch_item(VSAMPLITUDE_PROPERTY->items + 1, "AMPLITUDE_1", "+1", false);
	if (option_num > 2) indigo_init_switch_item(VSAMPLITUDE_PROPERTY->items + 2, "AMPLITUDE_2", "+2", false);
	if (option_num > 3) indigo_init_switch_item(VSAMPLITUDE_PROPERTY->items + 3, "AMPLITUDE_3", "+3", false);
	if (option_num > 4) indigo_init_switch_item(VSAMPLITUDE_PROPERTY->items + 4, "AMPLITUDE_4", "+4", false);
	indigo_define_property(device, VSAMPLITUDE_PROPERTY, NULL);
	res = SetVSAmplitude(0); /* 0 is Normal */
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetVSAmplitude() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
}

#else /* SDK is newer */

static void init_vsamplitude_property(indigo_device *device) {
	int res, option_num;
	res = GetNumberVSAmplitudes(&option_num);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberVSAmplitudes() for camera %d error: %d", PRIVATE_DATA->handle, res);
		option_num = 0;
	}
	VSAMPLITUDE_PROPERTY = indigo_init_switch_property(NULL, device->name, VSAMPLITUDE_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Vertical Clock Amplitude", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, option_num);
	if (VSAMPLITUDE_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of VSAMPLITUDE_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	for (int i = 0; i < option_num; i++) {
		char amplitude[INDIGO_NAME_SIZE];
		char item[INDIGO_NAME_SIZE];
		GetVSAmplitudeString(i, amplitude);
		snprintf(item, INDIGO_NAME_SIZE, "AMPLITUDE_%d", i);
		indigo_init_switch_item(VSAMPLITUDE_PROPERTY->items + i, item, amplitude, false);
	}
	if (option_num) VSAMPLITUDE_PROPERTY->items[0].sw.value = true;

	res = SetVSAmplitude(0);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetVSAmplitude() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
	indigo_define_property(device, VSAMPLITUDE_PROPERTY, NULL);
}

#endif /* SDK_PRIOR_2_92_30005_0 */


static void init_hreadout_property(indigo_device *device) {
	int res, channels, amps, items = 0;

	HREADOUT_PROPERTY = indigo_init_switch_property(NULL, device->name, HREADOUT_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Horisontal Readout", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
	if (HREADOUT_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of HREADOUT_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}

	res = GetNumberADChannels(&channels);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberADChannels() for camera %d error: %d", PRIVATE_DATA->handle, res);
		channels = 0;
	}

	res = GetNumberAmp(&amps);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberAmp() for camera %d error: %d", PRIVATE_DATA->handle, res);
		amps = 0;
	}

	for (int channel = 0; channel < channels; channel++) {
		int depth;
		GetBitDepth(channel, &depth);
		for (int amp = 0; amp < amps; amp++) {
			int speeds;
			char amp_desc[INDIGO_NAME_SIZE];
			GetAmpDesc (amp, amp_desc, sizeof(amp_desc));
			res = GetNumberHSSpeeds(channel, amp, &speeds);
			if (res != DRV_SUCCESS) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberHSSpeeds() for camera %d error: %d", PRIVATE_DATA->handle, res);
				speeds = 0;
			}
			for (int speed = 0; speed < speeds; speed++) {
				float speed_mhz;
				GetHSSpeed(channel, amp, speed, &speed_mhz);
				char item[INDIGO_NAME_SIZE];
				char description[INDIGO_VALUE_SIZE];
				snprintf(item, INDIGO_NAME_SIZE, HREADOUT_ITEM_FORMAT, channel, amp, speed);
				snprintf(description, INDIGO_VALUE_SIZE, "%.2fMHz %dbit %s", speed_mhz, depth, amp_desc);
				HREADOUT_PROPERTY = indigo_resize_property(HREADOUT_PROPERTY, items + 1);
				indigo_init_switch_item(HREADOUT_PROPERTY->items + items, item, description, false);
				items++;
			}
		}
	}

	if (items) HREADOUT_PROPERTY->items[0].sw.value = true;

	res = SetHSSpeed(0,0);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetHSSpeed() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}

	res = SetOutputAmplifier(0);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetOutputAmplifier() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}

	indigo_define_property(device, HREADOUT_PROPERTY, NULL);
}


static void init_preampgain_property(indigo_device *device) {
	int res, option_num;
	res = GetNumberPreAmpGains(&option_num);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberPreAmpGains() for camera %d error: %d", PRIVATE_DATA->handle, res);
		option_num = 0;
	}
	PREAMPGAIN_PROPERTY = indigo_init_switch_property(NULL, device->name, PREAMPGAIN_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Preamp Gain", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, option_num);
	if (PREAMPGAIN_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of PREAMPGAIN_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	for (int i = 0; i < option_num; i++) {
		float gain;
		char item[INDIGO_NAME_SIZE];
		char description[INDIGO_VALUE_SIZE];
		GetPreAmpGain(i, &gain);
		snprintf(item, INDIGO_NAME_SIZE, "GAIN_%d", i);
		snprintf(description, INDIGO_VALUE_SIZE, "%.1fx", gain);
		indigo_init_switch_item(PREAMPGAIN_PROPERTY->items + i, item, description, false);
	}
	if (option_num) PREAMPGAIN_PROPERTY->items[0].sw.value = true;

	res = SetPreAmpGain(0);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetPreampGain() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
	indigo_define_property(device, PREAMPGAIN_PROPERTY, NULL);
}


static void init_highcapacity_property(indigo_device *device) {
	int res;
	HIGHCAPACITY_PROPERTY = indigo_init_switch_property(NULL, device->name, HIGHCAPACITY_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Capacity / Sensitivity", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (HIGHCAPACITY_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of HIGHCAPACITY_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	indigo_init_switch_item(HIGHCAPACITY_PROPERTY->items + 0, "HIGHT_SENSITIVITY", "High Sensitivity", true);
	indigo_init_switch_item(HIGHCAPACITY_PROPERTY->items + 1, "HIGHT_CAPACITY", "High Capacity", false);
	indigo_define_property(device, HIGHCAPACITY_PROPERTY, NULL);
	res = SetHighCapacity(0); /* 0 is High Sensitivity */
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetHighCapacity() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
}


static void init_baselineclamp_property(indigo_device *device) {
	int res;
	BASELINECLAMP_PROPERTY = indigo_init_switch_property(NULL, device->name, BASELINECLAMP_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Baseline Clamp", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (BASELINECLAMP_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of BASELINECLAMP_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	indigo_init_switch_item(BASELINECLAMP_PROPERTY->items + 0, "DISABLE", "Disable", false);
	indigo_init_switch_item(BASELINECLAMP_PROPERTY->items + 1, "ENABLE", "Enable", false);
	int on = 0;
	if (CAP_GET_BASELINECLAMP) {
		res = GetBaselineClamp(&on);
		if (res != DRV_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetBaselineClamp() for camera %d error: %d", PRIVATE_DATA->handle, res);
		}
	} else {
		res = SetBaselineClamp(0);
		on = 0;
		if (res != DRV_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetBaselineClamp() for camera %d error: %d", PRIVATE_DATA->handle, res);
		}
	}
	if ((on == 0) || (on == 1)) BASELINECLAMP_PROPERTY->items[on].sw.value = true;
	indigo_define_property(device, BASELINECLAMP_PROPERTY, NULL);
}


static void init_baselineoffset_property(indigo_device *device) {
	int res;
	BASELINEOFFSET_PROPERTY = indigo_init_number_property(NULL, device->name, BASELINEOFFSET_PROPERTY_NAME, AQUISITION_GROUP_NAME, "Baseline Offset", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
	if (BASELINEOFFSET_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of BASELINEOFFSET_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	indigo_init_number_item(BASELINEOFFSET_OFFSET_ITEM, "OFFSET", "Offset", -1000, 1000, 100, 0);
	indigo_define_property(device, BASELINEOFFSET_PROPERTY, NULL);
	res = SetBaselineOffset(0);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetBaselineOffset(0) for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
}


static void init_fancontrol_property(indigo_device *device) {
	int res;
	FANCONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, FANCONTROL_PROPERTY_NAME, COOLER_GROUP_NAME, "Fan Speed", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 3);
	if (FANCONTROL_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of FANCONTROL_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	indigo_init_switch_item(FANCONTROL_PROPERTY->items + 0, "FULL_SPEED", "Full", false);
	indigo_init_switch_item(FANCONTROL_PROPERTY->items + 1, "LOW_SPEED", "Low", false);
	indigo_init_switch_item(FANCONTROL_PROPERTY->items + 2, "OFF", "Off", false);
	indigo_define_property(device, FANCONTROL_PROPERTY, NULL);
}


static void init_coolermode_property(indigo_device *device) {
	int res;
	COOLERMODE_PROPERTY = indigo_init_switch_property(NULL, device->name, COOLERMODE_PROPERTY_NAME, COOLER_GROUP_NAME, "Cooling on Shutdown", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
	if (COOLERMODE_PROPERTY == NULL) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Initialization of COOLERMODE_PROPERTY for camera %d failed.", PRIVATE_DATA->handle);
		return;
	}
	indigo_init_switch_item(COOLERMODE_PROPERTY->items + 0, "DISABLE_ON_SHUTDOWN", "Disable", true);
	indigo_init_switch_item(COOLERMODE_PROPERTY->items + 1, "KEEP_ON_SHUTDOWN", "Keep ON", false);
	indigo_define_property(device, COOLERMODE_PROPERTY, NULL);
	res = SetCoolerMode(0); /* 0 is DISABLE_ON_SHUTDOWN */
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCoolerMode() for camera %d error: %d", PRIVATE_DATA->handle, res);
	}
}


static bool andor_start_exposure(indigo_device *device, double exposure, bool dark, int offset_x, int offset_y, int frame_width, int frame_height, int bin_x, int bin_y) {
	unsigned int res;

	pthread_mutex_lock(&driver_mutex);
	if (!use_camera(device)) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	/* Set Read Mode to Image */
	res = SetReadMode(4);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetReadMode(4) for camera %d error: %d", PRIVATE_DATA->handle, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	/* Set Acquisition mode to Single scan */
	res = SetAcquisitionMode(1);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetAcquisitionMode(1) for camera %d error: %d", PRIVATE_DATA->handle, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	res = SetExposureTime(exposure);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetExposureTime(%f) for camera %d error: %d", exposure, PRIVATE_DATA->handle, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	if(dark) {
		res = SetShutter(1,2,50,50);
	} else {
		res = SetShutter(1,0,50,50);
	}
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetShutter() for camera %d error: %d", PRIVATE_DATA->handle, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	//Setup Image dimensions
	res = SetImage(bin_x, bin_y, offset_x+1, offset_x+frame_width, offset_y+1, offset_y+frame_height);
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetImage(%d, %d, %d, %d, %d, %d) for camera %d error: %d", bin_x, bin_y, offset_x+1, offset_x+frame_width, offset_y+1, offset_y+frame_height, PRIVATE_DATA->handle, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	res = StartAcquisition();
	if (res != DRV_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "StartAcquisition() for camera %d error: %d", PRIVATE_DATA->handle, res);
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}
	pthread_mutex_unlock(&driver_mutex);

	return true;
}


static bool andor_read_pixels(indigo_device *device) {
	long res;
	long wait_cycles = 12000;

	pthread_mutex_lock(&driver_mutex);
	if (!use_camera(device)) {
		pthread_mutex_unlock(&driver_mutex);
		return false;
	}

	/* Wait until acquisition finished or
	   10000*12000us = 120s => should be
	   enough for the slowest speed */
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
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAcquiredData() for camera %d error: %d", PRIVATE_DATA->handle, res);
			pthread_mutex_unlock(&driver_mutex);
			return false;
		}
	} else {
		res = GetAcquiredData16((uint16_t *)image, num_pixels);
		if (res != DRV_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetAcquiredData16() for camera %d error: %d", PRIVATE_DATA->handle, res);
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
		INFO_PROPERTY->count = 7;
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}


indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_ccd_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			if (indigo_property_match(VSSPEED_PROPERTY, property))
				indigo_define_property(device, VSSPEED_PROPERTY, NULL);
			if (indigo_property_match(VSAMPLITUDE_PROPERTY, property))
				indigo_define_property(device, VSAMPLITUDE_PROPERTY, NULL);
			if (indigo_property_match(HREADOUT_PROPERTY, property))
				indigo_define_property(device, HREADOUT_PROPERTY, NULL);
			if (indigo_property_match(PREAMPGAIN_PROPERTY, property))
				indigo_define_property(device, PREAMPGAIN_PROPERTY, NULL);
			if (indigo_property_match(HIGHCAPACITY_PROPERTY, property))
				indigo_define_property(device, HIGHCAPACITY_PROPERTY, NULL);
			if (indigo_property_match(BASELINECLAMP_PROPERTY, property))
				indigo_define_property(device, BASELINECLAMP_PROPERTY, NULL);
			if (indigo_property_match(BASELINEOFFSET_PROPERTY, property))
				indigo_define_property(device, BASELINEOFFSET_PROPERTY, NULL);
			if (indigo_property_match(FANCONTROL_PROPERTY, property))
				indigo_define_property(device, FANCONTROL_PROPERTY, NULL);
			if (indigo_property_match(COOLERMODE_PROPERTY, property))
				indigo_define_property(device, COOLERMODE_PROPERTY, NULL);
		}
	}
	return result;
}


static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	int res;
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

				pthread_mutex_lock(&driver_mutex);
				if (use_camera(device) == false) {
					CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, false);
					indigo_update_property(device, CONNECTION_PROPERTY, NULL);
					indigo_global_unlock(device);
					pthread_mutex_unlock(&driver_mutex);
					return INDIGO_OK;
				}
				if (CAP_SET_VREADOUT) {
					init_vsspeed_property(device);
				}
				if(CAP_SET_VSAMPLITUDE) {
					init_vsamplitude_property(device);
				}
				if(CAP_SET_HREADOUT) {
					init_hreadout_property(device);
				}
				if(CAP_SET_PREAMPGAIN) {
					init_preampgain_property(device);
				}
				if(CAP_SET_HIGHCAPACITY) {
					init_highcapacity_property(device);
				}
				if(CAP_SET_BASELINECLAMP) {
					init_baselineclamp_property(device);
				}
				if(CAP_SET_BASELINEOFFSET) {
					init_baselineoffset_property(device);
				}
				if(CAP_FANCONTROL) {
					init_fancontrol_property(device);
				}
				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				res = GetHeadModel(INFO_DEVICE_MODEL_ITEM->text.value);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetHeadModel() for camera %d error: %d", PRIVATE_DATA->handle, res);
					INFO_DEVICE_MODEL_ITEM->text.value[0] = '\0';
				}

				unsigned int fw_ver = 0, fw_build = 0, dummy;
				res = GetHardwareVersion(&dummy, &dummy, &dummy, &dummy, &fw_ver, &fw_build);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetHardwareVersion() for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
				snprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, INDIGO_VALUE_SIZE, "%d-%d", fw_ver, fw_build);

				snprintf(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, INDIGO_VALUE_SIZE, "CCD-%d", PRIVATE_DATA->serial_number);
				indigo_update_property(device, INFO_PROPERTY, NULL);

				int width = 0, height = 0;
				res = GetDetector(&width, &height);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetDetector() for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
				CCD_INFO_WIDTH_ITEM->number.value = width;
				CCD_INFO_HEIGHT_ITEM->number.value = height;
				CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = CCD_INFO_WIDTH_ITEM->number.value;
				CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = CCD_INFO_HEIGHT_ITEM->number.value;
				if (PRIVATE_DATA->buffer == NULL) {
					PRIVATE_DATA->buffer_size = width * height * 4 + FITS_HEADER_SIZE;
					PRIVATE_DATA->buffer = (unsigned char*)indigo_alloc_blob_buffer(PRIVATE_DATA->buffer_size);
				}

				float x_size = 0, y_size = 0;
				res = GetPixelSize(&x_size, &y_size);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetPixelSize() for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
				CCD_INFO_PIXEL_WIDTH_ITEM->number.value = x_size;
				CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = y_size;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value;

				int max_bin = 1;
				CCD_BIN_PROPERTY->perm = INDIGO_RW_PERM;
				/* 4 is Image mode, 0 is horizontal binning */
				res = GetMaximumBinning(4, 0, &max_bin);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetMaximumBinning(X) for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
				CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = max_bin;
				CCD_BIN_HORIZONTAL_ITEM->number.value = CCD_BIN_HORIZONTAL_ITEM->number.min = 1;
				CCD_BIN_HORIZONTAL_ITEM->number.max = max_bin;

				/* 4 is Image mode, 1 is vertical binning */
				res = GetMaximumBinning(4, 1, &max_bin);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetMaximumBinning(Y) for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
				CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = max_bin;
				CCD_BIN_VERTICAL_ITEM->number.value = CCD_BIN_VERTICAL_ITEM->number.min = 1;
				CCD_BIN_VERTICAL_ITEM->number.max = max_bin;

				if (CAP_GET_TEMPERATURE) {
					CCD_TEMPERATURE_PROPERTY->hidden = false;
					PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = 0;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
				}
				if (CAP_SET_TEMPERATURE) {
					int cooler_on = false;
					CCD_COOLER_PROPERTY->hidden = false;
					IsCoolerOn(&cooler_on);
					if(cooler_on) {
						indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
					} else {
						indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_OFF_ITEM, true);
					}
					int temp_min = -100, temp_max = 20;
					if (CAP_GET_TEMPERATURE_RANGE) GetTemperatureRange(&temp_min, &temp_max);
					CCD_TEMPERATURE_ITEM->number.max = (double)temp_max;
					CCD_TEMPERATURE_ITEM->number.min = (double)temp_min;
					PRIVATE_DATA->target_temperature = PRIVATE_DATA->current_temperature = CCD_TEMPERATURE_ITEM->number.value = (double)temp_max;
					CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
					init_coolermode_property(device);
				}

				/* Find available BPPs and use max */
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 0;
				CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = 128;
				int max_bpp_channel = 0;
				res = GetNumberADChannels(&PRIVATE_DATA->adc_channels);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetNumberADChannels() for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
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
				fix_bpp(device);
				res = SetADChannel(max_bpp_channel);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetADChannel(%d) error: %d", max_bpp_channel, res);
				}

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
				indigo_cancel_timer(device, &PRIVATE_DATA->temperature_timer);
				indigo_global_unlock(device);
				if (CAP_SET_VREADOUT) {
					indigo_delete_property(device, VSSPEED_PROPERTY, NULL);
				}
				if (CAP_SET_VSAMPLITUDE) {
					indigo_delete_property(device, VSAMPLITUDE_PROPERTY, NULL);
				}
				if (CAP_SET_HREADOUT) {
					indigo_delete_property(device, HREADOUT_PROPERTY, NULL);
				}
				if (CAP_SET_PREAMPGAIN) {
					indigo_delete_property(device, PREAMPGAIN_PROPERTY, NULL);
				}
				if (CAP_SET_HIGHCAPACITY) {
					indigo_delete_property(device, HIGHCAPACITY_PROPERTY, NULL);
				}
				if (CAP_SET_BASELINECLAMP) {
					indigo_delete_property(device, BASELINECLAMP_PROPERTY, NULL);
				}
				if (CAP_SET_BASELINEOFFSET) {
					indigo_delete_property(device, BASELINEOFFSET_PROPERTY, NULL);
				}
				if (CAP_FANCONTROL) {
					indigo_delete_property(device, FANCONTROL_PROPERTY, NULL);
				}
				if (CAP_SET_TEMPERATURE) {
					indigo_delete_property(device, COOLERMODE_PROPERTY, NULL);
				}

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
				CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
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
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CoolerON() for camera %d error: %d", PRIVATE_DATA->handle, res);
				}
			} else {
				res = CoolerOFF();
				if(res == DRV_SUCCESS) {
					CCD_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
					CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
					PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
				} else {
					CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "CoolerOFF() for camera %d error: %d", PRIVATE_DATA->handle, res);
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
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
				return INDIGO_OK;
			}
			long res = SetTemperature((int)PRIVATE_DATA->target_temperature);
			if(res == DRV_SUCCESS) {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			} else {
				CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetTemperature() for camera %d error: %d", PRIVATE_DATA->handle, res);
			}
			pthread_mutex_unlock(&driver_mutex);

			CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target temperature %g", PRIVATE_DATA->target_temperature);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
		// ------------------------------------------------------------------------------- CCD_FRAME
		indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
		fix_bpp(device);
		CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;

		for (int i = 0; i < PRIVATE_DATA->adc_channels; i++) {
			if(PRIVATE_DATA->bit_depths[i] == CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value) {
				pthread_mutex_lock(&driver_mutex);
				if (!use_camera(device)) {
					pthread_mutex_unlock(&driver_mutex);
					CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
					return INDIGO_OK;
				}
				uint32_t res = SetADChannel(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetADChannel(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					CCD_FRAME_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Bit depth: %d (Camera %d, Channel %d)", PRIVATE_DATA->bit_depths[i], PRIVATE_DATA->handle, i);
				}
				pthread_mutex_unlock(&driver_mutex);
				break;
			}
		}
		indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
	}  else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_BIN
		indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
		CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
		int horizontal_bin = round_pow2((int)CCD_BIN_HORIZONTAL_ITEM->number.value);
		int vertical_bin = round_pow2((int)CCD_BIN_VERTICAL_ITEM->number.value);
		CCD_BIN_HORIZONTAL_ITEM->number.value = horizontal_bin;
		CCD_BIN_VERTICAL_ITEM->number.value = vertical_bin;

		char name[32] = "";
		snprintf(name, 32, "BIN_%dx%d", horizontal_bin, vertical_bin);
		for (int i = 0; i < CCD_MODE_PROPERTY->count; i++) {
			indigo_item *item = &CCD_MODE_PROPERTY->items[i];
			item->sw.value = !strcmp(item->name, name);
		}

		CCD_MODE_PROPERTY->state = INDIGO_OK_STATE;
		if (IS_CONNECTED) {
			indigo_update_property(device, CCD_MODE_PROPERTY, NULL);
			indigo_update_property(device, CCD_BIN_PROPERTY, NULL);
		}
		return INDIGO_OK;
	} else if (indigo_property_match(VSSPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- VSSPEED
		indigo_property_copy_values(VSSPEED_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			VSSPEED_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, VSSPEED_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < VSSPEED_PROPERTY->count; i++) {
			if(VSSPEED_PROPERTY->items[i].sw.value) {
				uint32_t res = SetVSSpeed(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetVSSpeed(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					VSSPEED_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "VS Speed for camera %d set to %d", PRIVATE_DATA->handle, i);
					VSSPEED_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, VSSPEED_PROPERTY, NULL);
	} else if (indigo_property_match(VSAMPLITUDE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- VSAMPLITUDE
		indigo_property_copy_values(VSAMPLITUDE_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			VSAMPLITUDE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, VSAMPLITUDE_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < VSAMPLITUDE_PROPERTY->count; i++) {
			if(VSAMPLITUDE_PROPERTY->items[i].sw.value) {
				uint32_t res = SetVSAmplitude(i);
				if (res != DRV_SUCCESS) {
					if (res == DRV_P1INVALID) {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetVSAmplitude(%d): Amplitude Not Supported", i);
					} else {
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetVSAmplitude(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					}
					VSAMPLITUDE_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "VS Amplitude for camera %d set to %d", PRIVATE_DATA->handle, i);
					VSAMPLITUDE_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, VSAMPLITUDE_PROPERTY, NULL);
	} else if (indigo_property_match(HREADOUT_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HREADOUT
		indigo_property_copy_values(HREADOUT_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			HREADOUT_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, HREADOUT_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for (int i = 0; i < HREADOUT_PROPERTY->count; i++) {
			if (HREADOUT_PROPERTY->items[i].sw.value) {
				int channel, amp, speed;
				res = sscanf(HREADOUT_PROPERTY->items[i].name, HREADOUT_ITEM_FORMAT, &channel, &amp, &speed);
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Camera %d %s => Channel = %d, Amp = %d, Speed = %d", PRIVATE_DATA->handle, HREADOUT_PROPERTY->items[i].name, channel, amp, speed);
				res = SetHSSpeed(channel, speed);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetHSSpeed(%d, %d) for camera %d error: %d", channel, speed, PRIVATE_DATA->handle, res);
					HREADOUT_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "ADC Channel for camera %d set to %d, HS Speed set to %d", PRIVATE_DATA->handle, channel, speed);
					HREADOUT_PROPERTY->state = INDIGO_OK_STATE;

					/* Update BPP in CCD_FRAME_PROPERTY*/
					CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = PRIVATE_DATA->bit_depths[channel];
					fix_bpp(device);
					indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
				}
				res = SetOutputAmplifier(amp);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetOutputAmplifier(%d) for camera %d error: %d", amp, PRIVATE_DATA->handle, res);
					HREADOUT_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Output Amplifier for camera %d set to %d", PRIVATE_DATA->handle, amp);
					HREADOUT_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, HREADOUT_PROPERTY, NULL);
	} else if (indigo_property_match(PREAMPGAIN_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- PREAMPGAIN
		indigo_property_copy_values(PREAMPGAIN_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			PREAMPGAIN_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, PREAMPGAIN_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < PREAMPGAIN_PROPERTY->count; i++) {
			if(PREAMPGAIN_PROPERTY->items[i].sw.value) {
				uint32_t res = SetPreAmpGain(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetPreampGain(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					PREAMPGAIN_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Preamp gain for camera %d set to %d", PRIVATE_DATA->handle, i);
					PREAMPGAIN_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, PREAMPGAIN_PROPERTY, NULL);
	} else if (indigo_property_match(HIGHCAPACITY_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- HIGHCAPACITY
		indigo_property_copy_values(HIGHCAPACITY_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			HIGHCAPACITY_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, HIGHCAPACITY_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < HIGHCAPACITY_PROPERTY->count; i++) {
			if(HIGHCAPACITY_PROPERTY->items[i].sw.value) {
				uint32_t res = SetHighCapacity(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetHighCapacity(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					HIGHCAPACITY_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set High Sensitivity/Capacity (0/1) for camera %d: %d", PRIVATE_DATA->handle, i);
					HIGHCAPACITY_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, HIGHCAPACITY_PROPERTY, NULL);
	} else if (indigo_property_match(BASELINECLAMP_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- BASELINECLAMP
		indigo_property_copy_values(BASELINECLAMP_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			BASELINECLAMP_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, BASELINECLAMP_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < BASELINECLAMP_PROPERTY->count; i++) {
			if(BASELINECLAMP_PROPERTY->items[i].sw.value) {
				uint32_t res = SetBaselineClamp(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetBaselineClamp(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					BASELINECLAMP_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set Baseline Clamp for camera %d (0=Disable/1=Enable): %d", PRIVATE_DATA->handle, i);
					BASELINECLAMP_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, BASELINECLAMP_PROPERTY, NULL);
	} else if (indigo_property_match(BASELINEOFFSET_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- BASELINEOFFSET
		indigo_property_copy_values(BASELINEOFFSET_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			BASELINEOFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, BASELINEOFFSET_PROPERTY, NULL);
			return INDIGO_OK;
		}
		int offset = (int)(round(BASELINEOFFSET_OFFSET_ITEM->number.value / 100.0) * 100);
		BASELINEOFFSET_OFFSET_ITEM->number.value = offset;
		uint32_t res = SetBaselineOffset(offset);
		if (res != DRV_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetBaselineOffset(%d) for camera %d error: %d", offset, PRIVATE_DATA->handle, res);
			BASELINEOFFSET_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set Baseline Offset for camera %d: %d", PRIVATE_DATA->handle, offset);
			BASELINEOFFSET_PROPERTY->state = INDIGO_OK_STATE;
		}

		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, BASELINEOFFSET_PROPERTY, NULL);
	} else if (indigo_property_match(FANCONTROL_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FANCONTROL
		indigo_property_copy_values(FANCONTROL_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			FANCONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FANCONTROL_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < FANCONTROL_PROPERTY->count; i++) {
			if(FANCONTROL_PROPERTY->items[i].sw.value) {
				uint32_t res = SetFanMode(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetFanMode(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					FANCONTROL_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set Fan mode for camera %d (0=Full/1=Low/2=off): %d", PRIVATE_DATA->handle, i);
					FANCONTROL_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, FANCONTROL_PROPERTY, NULL);
	} else if (indigo_property_match(COOLERMODE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- COOLERMODE
		indigo_property_copy_values(COOLERMODE_PROPERTY, property, false);
		pthread_mutex_lock(&driver_mutex);
		if (!use_camera(device)) {
			pthread_mutex_unlock(&driver_mutex);
			COOLERMODE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, COOLERMODE_PROPERTY, NULL);
			return INDIGO_OK;
		}
		for(int i = 0; i < COOLERMODE_PROPERTY->count; i++) {
			if(COOLERMODE_PROPERTY->items[i].sw.value) {
				uint32_t res = SetCoolerMode(i);
				if (res != DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCoolerMode(%d) for camera %d error: %d", i, PRIVATE_DATA->handle, res);
					COOLERMODE_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Set Cooler mode for camera %d (0=Disable on Shutdown/1=Keep ON on Shutdown): %d", PRIVATE_DATA->handle, i);
					COOLERMODE_PROPERTY->state = INDIGO_OK_STATE;
				}
				break;
			}
		}
		pthread_mutex_unlock(&driver_mutex);
		indigo_update_property(device, COOLERMODE_PROPERTY, NULL);
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, VSSPEED_PROPERTY);
			indigo_save_property(device, NULL, VSAMPLITUDE_PROPERTY);
			indigo_save_property(device, NULL, HREADOUT_PROPERTY);
			indigo_save_property(device, NULL, PREAMPGAIN_PROPERTY);
			indigo_save_property(device, NULL, HIGHCAPACITY_PROPERTY);
			indigo_save_property(device, NULL, BASELINECLAMP_PROPERTY);
			indigo_save_property(device, NULL, BASELINEOFFSET_PROPERTY);
			indigo_save_property(device, NULL, FANCONTROL_PROPERTY);
			indigo_save_property(device, NULL, COOLERMODE_PROPERTY);
		}
	}
	// --------------------------------------------------------------------------------
	return indigo_ccd_change_property(device, client, property);
}


static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		indigo_device_disconnect(NULL, device->name);
		if (CAP_SET_VREADOUT) {
			indigo_release_property(VSSPEED_PROPERTY);
		}
		if (CAP_SET_VSAMPLITUDE) {
			indigo_release_property(VSAMPLITUDE_PROPERTY);
		}
		if (CAP_SET_HREADOUT) {
			indigo_release_property(HREADOUT_PROPERTY);
		}
		if (CAP_SET_PREAMPGAIN) {
			indigo_release_property(PREAMPGAIN_PROPERTY);
		}
		if (CAP_SET_HIGHCAPACITY) {
			indigo_release_property(HIGHCAPACITY_PROPERTY);
		}
		if (CAP_SET_BASELINECLAMP) {
			indigo_release_property(BASELINECLAMP_PROPERTY);
		}
		if (CAP_SET_BASELINEOFFSET) {
			indigo_release_property(BASELINEOFFSET_PROPERTY);
		}
		if (CAP_FANCONTROL) {
			indigo_release_property(FANCONTROL_PROPERTY);
		}
		if (CAP_SET_TEMPERATURE) {
			indigo_release_property(COOLERMODE_PROPERTY);
		}
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
				if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetCameraHandle() for camera %d error: %d", handle, res);

				res = SetCurrentCamera(handle);
				if (res!= DRV_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "SetCurrentCamera() for camera %d error: %d", handle, res);

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
						INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR SDK initialisation for camera %d error: %d", handle, res);
					}
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "ANDOR_SDK_PATH may not be not valid.");
					break;
				}

				private_data->caps.ulSize = sizeof(AndorCapabilities);
				GetCapabilities(&private_data->caps);

				res = GetCameraSerialNumber(&private_data->serial_number);
				if (res!= DRV_SUCCESS) {
					INDIGO_DRIVER_ERROR(DRIVER_NAME, "GetCameraSerialNumber() for camera %d error: %d", handle, res);
					private_data->serial_number = 0;
				}

				pthread_mutex_unlock(&driver_mutex);

				char camera_type[32];
				get_camera_type(private_data->caps.ulCameraType, camera_type, sizeof(camera_type));
				snprintf(device->name, sizeof(device->name), "%s #%d", camera_type, private_data->serial_number);
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
