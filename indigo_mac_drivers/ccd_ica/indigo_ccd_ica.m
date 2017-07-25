// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO ICA CCD driver
 \file indigo_ccd_ica.m
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_ica"

#import <Cocoa/Cocoa.h>
#import <ImageCaptureCore/ImageCaptureCore.h>

#include "indigo_ccd_driver.h"
#include "indigo_focuser_driver.h"

#import "indigo_ccd_ica.h"
#import "indigo_ica_ptp.h"
#import "indigo_ica_ptp_nikon.h"
#import "indigo_ica_ptp_canon.h"
#import "indigo_ica_ptp_sony.h"

#define PRIVATE_DATA                    ((ica_private_data *)device->private_data)
#define DSLR_LOCK_PROPERTY              (PRIVATE_DATA->dslr_lock_property)
#define DSLR_LOCK_ITEM                  (PRIVATE_DATA->dslr_lock_property->items + 0)
#define DSLR_UNLOCK_ITEM                (PRIVATE_DATA->dslr_lock_property->items + 1)
#define DSLR_MIRROR_LOCKUP_PROPERTY     (PRIVATE_DATA->dslr_mirror_lockup_property)
#define DSLR_MIRROR_LOCKUP_LOCK_ITEM    (PRIVATE_DATA->dslr_mirror_lockup_property->items + 0)
#define DSLR_MIRROR_LOCKUP_UNLOCK_ITEM  (PRIVATE_DATA->dslr_mirror_lockup_property->items + 1)
#define DSLR_AF_PROPERTY                (PRIVATE_DATA->dslr_af_property)
#define DSLR_AF_ITEM                    (PRIVATE_DATA->dslr_af_property->items + 0)
#define DSLR_AVOID_AF_PROPERTY          (PRIVATE_DATA->dslr_avoid_af_property)
#define DSLR_AVOID_AF_ON_ITEM           (PRIVATE_DATA->dslr_avoid_af_property->items + 0)
#define DSLR_AVOID_AF_OFF_ITEM          (PRIVATE_DATA->dslr_avoid_af_property->items + 1)

struct dslr_properties {
  PTPVendorExtension extension;
	PTPPropertyCode code;
	char *name;
	char *label;
} dslr_properties[] = {
  
  { 0, PTPPropertyCodeExposureProgramMode, DSLR_PROGRAM_PROPERTY_NAME, "Exposure program" },
  { 0, PTPPropertyCodeFNumber, DSLR_APERTURE_PROPERTY_NAME, "Aperture" },
  { 0, PTPPropertyCodeExposureTime, DSLR_SHUTTER_PROPERTY_NAME, "Shutter" },
  { 0, PTPPropertyCodeImageSize, CCD_MODE_PROPERTY_NAME, "Image size" },
  { 0, PTPPropertyCodeCompressionSetting, DSLR_COMPRESSION_PROPERTY_NAME, "Compression" },
  { 0, PTPPropertyCodeWhiteBalance, DSLR_WHITE_BALANCE_PROPERTY_NAME, "White balance" },
  { 0, PTPPropertyCodeExposureIndex, DSLR_ISO_PROPERTY_NAME, "ISO" },
  { 0, PTPPropertyCodeExposureMeteringMode, DSLR_EXPOSURE_METERING_PROPERTY_NAME, "Exposure metering" },
  { 0, PTPPropertyCodeFocusMeteringMode, DSLR_FOCUS_METERING_PROPERTY_NAME, "Focus metering" },
  { 0, PTPPropertyCodeFocusMode, DSLR_FOCUS_MODE_PROPERTY_NAME, "Focus mode" },
  { 0, PTPPropertyCodeBatteryLevel, DSLR_BATTERY_LEVEL_PROPERTY_NAME, "Battery level" },
  { 0, PTPPropertyCodeFocalLength, DSLR_FOCAL_LENGTH_PROPERTY_NAME, "Focal length" },
  { 0, PTPPropertyCodeStillCaptureMode, DSLR_CAPTURE_MODE_PROPERTY_NAME, "Capture mode" },
  { 0, PTPPropertyCodeFlashMode, DSLR_FLASH_MODE_PROPERTY_NAME, "Flash mode" },
  { 0, PTPPropertyCodeExposureBiasCompensation, DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME, "Exposure compensation" },
  
	{ PTPVendorExtensionNikon, PTPPropertyCodeExposureProgramMode, DSLR_PROGRAM_PROPERTY_NAME, "Exposure program" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeFNumber, DSLR_APERTURE_PROPERTY_NAME, "Aperture" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeExposureTime, DSLR_SHUTTER_PROPERTY_NAME, "Shutter" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeImageSize, CCD_MODE_PROPERTY_NAME, "Image size" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeCompressionSetting, DSLR_COMPRESSION_PROPERTY_NAME, "Compression" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeWhiteBalance, DSLR_WHITE_BALANCE_PROPERTY_NAME, "White balance" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeExposureIndex, DSLR_ISO_PROPERTY_NAME, "ISO" },
  { PTPVendorExtensionNikon, PTPPropertyCodeExposureMeteringMode, DSLR_EXPOSURE_METERING_PROPERTY_NAME, "Exposure metering" },
  { PTPVendorExtensionNikon, PTPPropertyCodeNikonAutofocusMode, DSLR_FOCUS_MODE_PROPERTY_NAME, "Focus mode" },
  { PTPVendorExtensionNikon, PTPPropertyCodeBatteryLevel, DSLR_BATTERY_LEVEL_PROPERTY_NAME, "Battery level" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeFocalLength, DSLR_FOCAL_LENGTH_PROPERTY_NAME, "Focal length" },
	{ PTPVendorExtensionNikon, PTPPropertyCodeStillCaptureMode, DSLR_CAPTURE_MODE_PROPERTY_NAME, "Capture mode" },
  { PTPVendorExtensionNikon, PTPPropertyCodeFlashMode, DSLR_FLASH_MODE_PROPERTY_NAME, "Flash mode" },
  { PTPVendorExtensionNikon, PTPPropertyCodeNikonEVStep, DSLR_COMPENSATION_STEP_PROPERTY_NAME, "Compensation step" },
  { PTPVendorExtensionNikon, PTPPropertyCodeExposureBiasCompensation, DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME, "Exposure compensation" },
  { PTPVendorExtensionNikon, PTPPropertyCodeNikonFlashExposureCompensation, DSLR_FLASH_COMPENSATION_PROPERTY_NAME, "Flash compensation" },
  { PTPVendorExtensionNikon, PTPPropertyCodeNikonExternalFlashMode, DSLR_EXT_FLASH_MODE_PROPERTY_NAME, "External flash mode" },
  { PTPVendorExtensionNikon, PTPPropertyCodeNikonExternalFlashCompensation, DSLR_EXT_FLASH_COMPENSATION_PROPERTY_NAME, "External flash compensation" },
  { PTPVendorExtensionNikon, PTPPropertyCodeNikonActivePicCtrlItem, DSLR_PICTURE_STYLE_PROPERTY_NAME, "Picture style" },
  
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonAutoExposureMode, DSLR_PROGRAM_PROPERTY_NAME, "Exposure program" },
  { PTPVendorExtensionNikon, PTPPropertyCodeCanonDriveMode, DSLR_CAPTURE_MODE_PROPERTY_NAME, "Capture mode" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonAperture, DSLR_APERTURE_PROPERTY_NAME, "Aperture" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonShutterSpeed, DSLR_SHUTTER_PROPERTY_NAME, "Shutter" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonImageFormat, CCD_MODE_PROPERTY_NAME, "Image size" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonWhiteBalance, DSLR_WHITE_BALANCE_PROPERTY_NAME, "White balance" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonISOSpeed, DSLR_ISO_PROPERTY_NAME, "ISO" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonMeteringMode, DSLR_EXPOSURE_METERING_PROPERTY_NAME, "Exposure metering" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonFocusMode, DSLR_FOCUS_MODE_PROPERTY_NAME, "Focus mode" },
  { PTPVendorExtensionCanon, PTPPropertyCodeBatteryLevel, DSLR_BATTERY_LEVEL_PROPERTY_NAME, "Battery level" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonExpCompensation, DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME, "Exposure compensation" },
  { PTPVendorExtensionCanon, PTPPropertyCodeCanonDriveMode, DSLR_CAPTURE_MODE_PROPERTY_NAME, "Drive mode" },
  
  { PTPVendorExtensionSony, PTPPropertyCodeExposureProgramMode, DSLR_PROGRAM_PROPERTY_NAME, "Exposure program" },
  { PTPVendorExtensionSony, PTPPropertyCodeFNumber, DSLR_APERTURE_PROPERTY_NAME, "Aperture" },
  { PTPVendorExtensionSony, PTPPropertyCodeSonyShutterSpeed, DSLR_SHUTTER_PROPERTY_NAME, "Shutter" },
  { PTPVendorExtensionSony, PTPPropertyCodeSonyImageSize, CCD_MODE_PROPERTY_NAME, "Image size" },
  { PTPVendorExtensionSony, PTPPropertyCodeCompressionSetting, DSLR_COMPRESSION_PROPERTY_NAME, "Compression" },
  { PTPVendorExtensionSony, PTPPropertyCodeWhiteBalance, DSLR_WHITE_BALANCE_PROPERTY_NAME, "White balance" },
  { PTPVendorExtensionSony, PTPPropertyCodeSonyISO, DSLR_ISO_PROPERTY_NAME, "ISO" },
  { PTPVendorExtensionSony, PTPPropertyCodeExposureMeteringMode, DSLR_EXPOSURE_METERING_PROPERTY_NAME, "Exposure metering" },
  { PTPVendorExtensionSony, PTPPropertyCodeFocusMeteringMode, DSLR_FOCUS_METERING_PROPERTY_NAME, "Focus metering" },
  { PTPVendorExtensionSony, PTPPropertyCodeFocusMode, DSLR_FOCUS_MODE_PROPERTY_NAME, "Focus mode" },
  { PTPVendorExtensionSony, PTPPropertyCodeSonyBatteryLevel, DSLR_BATTERY_LEVEL_PROPERTY_NAME, "Battery level" },
  { PTPVendorExtensionSony, PTPPropertyCodeFocalLength, DSLR_FOCAL_LENGTH_PROPERTY_NAME, "Focal length" },
  { PTPVendorExtensionSony, PTPPropertyCodeStillCaptureMode, DSLR_CAPTURE_MODE_PROPERTY_NAME, "Capture mode" },
  { PTPVendorExtensionSony, PTPPropertyCodeFlashMode, DSLR_FLASH_MODE_PROPERTY_NAME, "Flash mode" },
  { PTPVendorExtensionSony, PTPPropertyCodeExposureBiasCompensation, DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME, "Exposure compensation" },
  { PTPVendorExtensionSony, PTPPropertyCodeSonyDPCCompensation, DSLR_FLASH_COMPENSATION_PROPERTY_NAME, "Flash compensation" },

	{ 0, 0, NULL, NULL }
};

typedef struct {
	void* camera;
  indigo_device *focuser;
  indigo_property *dslr_lock_property;
  indigo_property *dslr_mirror_lockup_property;
  indigo_property *dslr_af_property;
  indigo_property *dslr_avoid_af_property;
	indigo_property **dslr_properties;
  int dslr_properties_count;
  void *buffer;
  int buffer_size;
  bool bulb;
  indigo_timer *exposure_timer;
} ica_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
  PTPCamera *camera = (__bridge PTPCamera *)(PRIVATE_DATA->camera);
	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		// --------------------------------------------------------------------------------
			CCD_INFO_PROPERTY->hidden = false;
    if (camera.width != 0 && camera.height != 0 && camera.pixelSize != 0) {
      CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = CCD_FRAME_LEFT_ITEM->number.max = camera.width;
      CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = CCD_FRAME_TOP_ITEM->number.max = camera.height;
      CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = camera.pixelSize;
      CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
		} else {
			CCD_INFO_PROPERTY->hidden = CCD_FRAME_PROPERTY->hidden = true;
		}
		CCD_MODE_PROPERTY->hidden = CCD_BIN_PROPERTY->hidden =  true;
    CCD_IMAGE_FORMAT_PROPERTY->perm = CCD_EXPOSURE_PROPERTY->perm = CCD_ABORT_EXPOSURE_PROPERTY->perm = INDIGO_RO_PERM;
    DSLR_LOCK_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_LOCK_PROPERTY_NAME, "DSLR", "Lock camera GUI", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    indigo_init_switch_item(DSLR_LOCK_ITEM, DSLR_LOCK_ITEM_NAME, "Lock", true);
    indigo_init_switch_item(DSLR_UNLOCK_ITEM, DSLR_UNLOCK_ITEM_NAME, "Unlock", false);
    DSLR_MIRROR_LOCKUP_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_MIRROR_LOCKUP_PROPERTY_NAME, "DSLR", "Use mirror lockup", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    indigo_init_switch_item(DSLR_MIRROR_LOCKUP_LOCK_ITEM, DSLR_MIRROR_LOCKUP_LOCK_ITEM_NAME, "Lock", false);
    indigo_init_switch_item(DSLR_MIRROR_LOCKUP_UNLOCK_ITEM, DSLR_MIRROR_LOCKUP_UNLOCK_ITEM_NAME, "Unlock", true);
    DSLR_AF_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_AF_PROPERTY_NAME, "DSLR", "Autofocus", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
    indigo_init_switch_item(DSLR_AF_ITEM, DSLR_AF_ITEM_NAME, "Start autofocus", false);
    DSLR_AVOID_AF_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_AVOID_AF_PROPERTY_NAME, "DSLR", "Avoid AF", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    indigo_init_switch_item(DSLR_AVOID_AF_ON_ITEM, DSLR_AVOID_AF_ON_ITEM_NAME, "On", true);
    indigo_init_switch_item(DSLR_AVOID_AF_OFF_ITEM, DSLR_AVOID_AF_OFF_ITEM_NAME, "Off", false);
		indigo_set_switch(CCD_IMAGE_FORMAT_PROPERTY, CCD_IMAGE_FORMAT_JPEG_ITEM, true);
		// --------------------------------------------------------------------------------
		INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	indigo_result result = INDIGO_OK;
	if ((result = indigo_ccd_enumerate_properties(device, client, property)) == INDIGO_OK) {
		if (IS_CONNECTED) {
			for (int i = 0; i < PRIVATE_DATA->dslr_properties_count; i++)
				if (indigo_property_match(PRIVATE_DATA->dslr_properties[i], property))
					indigo_define_property(device, PRIVATE_DATA->dslr_properties[i], NULL);
      if (indigo_property_match(DSLR_LOCK_PROPERTY, property))
        indigo_define_property(device, DSLR_LOCK_PROPERTY, NULL);
      if (indigo_property_match(DSLR_MIRROR_LOCKUP_PROPERTY, property))
        indigo_define_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
      if (indigo_property_match(DSLR_AF_PROPERTY, property))
        indigo_define_property(device, DSLR_AF_PROPERTY, NULL);
      if (indigo_property_match(DSLR_AVOID_AF_PROPERTY, property))
        indigo_define_property(device, DSLR_AVOID_AF_PROPERTY, NULL);
		}
	}
	return result;
}

static void exposure_timer_callback(indigo_device *device) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_ITEM->number.value = 0;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
    PTPCamera *camera = (__bridge PTPCamera *)(PRIVATE_DATA->camera);
    [camera stopExposure];
  }
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	PTPCamera *camera = (__bridge PTPCamera *)(PRIVATE_DATA->camera);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			[camera requestOpenSession];
		} else {
			[camera requestCloseSession];
		}
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		return INDIGO_OK;
  } else if (indigo_property_match(DSLR_LOCK_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_LOCK
    indigo_property_copy_values(DSLR_LOCK_PROPERTY, property, false);
    DSLR_LOCK_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, DSLR_LOCK_PROPERTY, NULL);
    if (DSLR_LOCK_ITEM->sw.value)
      [camera lock];
    else
      [camera unlock];
    return INDIGO_OK;
  } else if (indigo_property_match(DSLR_MIRROR_LOCKUP_PROPERTY, property)) {
      // -------------------------------------------------------------------------------- DSLR_MIRROR_LOCKUP
    indigo_property_copy_values(DSLR_MIRROR_LOCKUP_PROPERTY, property, false);
    DSLR_MIRROR_LOCKUP_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
    return INDIGO_OK;
  } else if (indigo_property_match(DSLR_AF_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_AF
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE || DSLR_AF_PROPERTY->state == INDIGO_BUSY_STATE)
      return INDIGO_OK;
    indigo_property_copy_values(DSLR_AF_PROPERTY, property, false);
    if (DSLR_AF_ITEM->sw.value) {
      DSLR_AF_PROPERTY->state = INDIGO_OK_STATE;
      indigo_update_property(device, DSLR_AF_PROPERTY, NULL);
        [camera startAutofocus];
    }
    return INDIGO_OK;
  } else if (indigo_property_match(DSLR_AVOID_AF_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_AVOID_AF
    indigo_property_copy_values(DSLR_AVOID_AF_PROPERTY, property, false);
    DSLR_AVOID_AF_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, DSLR_AVOID_AF_PROPERTY, NULL);
    return INDIGO_OK;
	} else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE || DSLR_AF_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
		[camera startExposureWithMirrorLockup:DSLR_MIRROR_LOCKUP_LOCK_ITEM->sw.value avoidAF:DSLR_AVOID_AF_ON_ITEM->sw.value];
    if (PRIVATE_DATA->bulb)
      PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value, exposure_timer_callback);
    return indigo_ccd_change_property(device, client, property);
	} else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_STREAMING
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE || DSLR_AF_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;
		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
			CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
		} else {
			CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
			CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
      int zoomWidth = (int)(CCD_INFO_WIDTH_ITEM->number.value / CCD_FRAME_WIDTH_ITEM->number.value);
      int zoomHeight = (int)(CCD_INFO_HEIGHT_ITEM->number.value / CCD_FRAME_HEIGHT_ITEM->number.value);
      int zoom = zoomWidth < zoomHeight ? zoomWidth : zoomHeight;
      int x = CCD_FRAME_LEFT_ITEM->number.value + CCD_FRAME_WIDTH_ITEM->number.value / 2;
      int y = CCD_FRAME_TOP_ITEM->number.value + CCD_FRAME_HEIGHT_ITEM->number.value / 2;
//      NSLog(@"%d %d", (int)(CCD_INFO_WIDTH_ITEM->number.value), (int)(CCD_INFO_HEIGHT_ITEM->number.value));
//      NSLog(@"%d %d %d %d %d", (int)(CCD_FRAME_LEFT_ITEM->number.value), (int)(CCD_FRAME_TOP_ITEM->number.value), (int)(CCD_FRAME_WIDTH_ITEM->number.value), (int)(CCD_FRAME_HEIGHT_ITEM->number.value), zoom);
//      NSLog(@"%d %d", x, y);
      [camera startPreviewZoom:zoom x:x y:y];
		}
		return INDIGO_OK;
	} else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE ) {
      indigo_cancel_timer(device, &PRIVATE_DATA->exposure_timer);
      [camera stopExposure];
		} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
			[camera stopPreview];
    } else if (DSLR_AF_PROPERTY->state == INDIGO_BUSY_STATE) {
      [camera stopAutofocus];
      DSLR_AF_PROPERTY->state = INDIGO_ALERT_STATE;
      indigo_update_property(device, DSLR_AF_PROPERTY, NULL);
      return INDIGO_OK;
    }
    return indigo_ccd_change_property(device, client, property);
	}
	// --------------------------------------------------------------------------------
  PTPVendorExtension extension = camera.extension;
	for (int i = 0; i < PRIVATE_DATA->dslr_properties_count; i++) {
		indigo_property *dslr_property = PRIVATE_DATA->dslr_properties[i];
		if (indigo_property_match(dslr_property, property)) {
			indigo_property_copy_values(dslr_property, property, false);
			PTPPropertyCode code = 0;
			for (int i = 0; dslr_properties[i].code; i++) {
				if (dslr_properties[i].extension == extension && !strcmp(dslr_properties[i].name, property->name)) {
					code = dslr_properties[i].code;
          break;
				}
			}
			if (!code)
				code = strtol(property->name, NULL, 16);
			switch (dslr_property->type) {
				case INDIGO_SWITCH_VECTOR: {
					for (int j = 0; j < dslr_property->count; j++) {
						if (dslr_property->items[j].sw.value) {
							dslr_property->state = INDIGO_BUSY_STATE;
							indigo_update_property(device, dslr_property, NULL);
							[camera setProperty:code value:[NSString stringWithCString:dslr_property->items[j].name encoding:NSUTF8StringEncoding]];
							break;
						}
					}
					return INDIGO_OK;
				}
				case INDIGO_NUMBER_VECTOR: {
          dslr_property->state = INDIGO_BUSY_STATE;
          indigo_update_property(device, dslr_property, NULL);
          [camera setProperty:code value:[NSString stringWithFormat:@"%d", (int)dslr_property->items[0].number.value]];
					return INDIGO_OK;
				}
				default: {
          dslr_property->state = INDIGO_BUSY_STATE;
          indigo_update_property(device, dslr_property, NULL);
          [camera setProperty:code value:[NSString stringWithCString:dslr_property->items[0].text.value encoding:NSUTF8StringEncoding]];
					return INDIGO_OK;
				}
			}
		}
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	for (int i = 0; i < PRIVATE_DATA->dslr_properties_count; i++)
		indigo_release_property(PRIVATE_DATA->dslr_properties[i]);
  indigo_release_property(DSLR_LOCK_PROPERTY);
  indigo_release_property(DSLR_MIRROR_LOCKUP_PROPERTY);
  indigo_release_property(DSLR_AF_PROPERTY);
  indigo_release_property(DSLR_AVOID_AF_PROPERTY);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
	return indigo_ccd_detach(device);
}

static indigo_result focuser_attach(indigo_device *device) {
  assert(device != NULL);
  assert(PRIVATE_DATA != NULL);
  if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
    // --------------------------------------------------------------------------------
    FOCUSER_POSITION_PROPERTY->hidden = true;
    FOCUSER_SPEED_PROPERTY->hidden = true;
    // --------------------------------------------------------------------------------
    INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
    return indigo_focuser_enumerate_properties(device, NULL, NULL);
  }
  return INDIGO_FAILED;
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(DEVICE_CONTEXT != NULL);
  assert(property != NULL);
  PTPCamera *camera = (__bridge PTPCamera *)(PRIVATE_DATA->camera);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
    CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
      [camera focus:(int)FOCUSER_STEPS_ITEM->number.value];
		} else if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value) {
      [camera focus:(int)-FOCUSER_STEPS_ITEM->number.value];
		}
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		return INDIGO_OK;
  } else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
    indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
    if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
      FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
      indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
      FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
    } else {
      FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_ALERT_STATE;
    }
    [camera focus:0];
    FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
    indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
  }
  return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
  assert(device != NULL);
  if (CONNECTION_CONNECTED_ITEM->sw.value)
    indigo_device_disconnect(NULL, device->name);
  INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
  return indigo_focuser_detach(device);
}

// -------------------------------------------------------------------------------- ICA interface

@interface PTPDelegate : NSObject<PTPDelegateProtocol>
@end

@implementation PTPDelegate

-(void)cameraAdded:(PTPCamera *)camera {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
	static indigo_device ccd_template = {
		"", false, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	};
	ica_private_data *private_data = malloc(sizeof(ica_private_data));
	assert(private_data);
	memset(private_data, 0, sizeof(ica_private_data));
	private_data->camera = (__bridge void *)camera;
	indigo_device *device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &ccd_template, sizeof(indigo_device));
	strcpy(device->name, [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
	device->private_data = private_data;
	camera.userData = [NSValue valueWithPointer:device];
	indigo_async((void *)(void *)indigo_attach_device, device);
}

-(void)cameraConnected:(PTPCamera*)camera {
  [camera requestEnableTethering];
  sleep(1);
	indigo_log("%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	if (device) {
		for (int i = 0; i < PRIVATE_DATA->dslr_properties_count; i++)
			indigo_define_property(device, PRIVATE_DATA->dslr_properties[i], NULL);
    indigo_define_property(device, DSLR_LOCK_PROPERTY, NULL);
    indigo_define_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
    indigo_define_property(device, DSLR_AF_PROPERTY, NULL);
    indigo_define_property(device, DSLR_AVOID_AF_PROPERTY, NULL);
    if (DSLR_LOCK_ITEM->sw.value)
      [camera lock];
    else
      [camera unlock];
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	}
}


-(int)propertyIndex:(PTPCamera *)camera code:(PTPPropertyCode)code type:(indigo_property_type)type {
	char name[INDIGO_NAME_SIZE], label[INDIGO_VALUE_SIZE], *group = "Advanced";
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	indigo_property *property;
	sprintf(name, "%04x", code);
	strncpy(label, [[camera.propertyClass propertyCodeName:code] cStringUsingEncoding:NSUTF8StringEncoding] + 15, INDIGO_NAME_SIZE);
  PTPVendorExtension extension = camera.extension;
	for (int i = 0; i < dslr_properties[i].code; i++) {
		if (dslr_properties[i].extension == extension && code == dslr_properties[i].code) {
			strcpy(name, dslr_properties[i].name);
			strcpy(label, dslr_properties[i].label);
			group = "DSLR";
      break;
		}
	}
	for (int i = 0; i < PRIVATE_DATA->dslr_properties_count; i++) {
		property = PRIVATE_DATA->dslr_properties[i];
		if (!strcmp(name, property->name)) {
			return i;
		}
	}
  if (type == 0)
    return -1;
	if (PRIVATE_DATA->dslr_properties)
		PRIVATE_DATA->dslr_properties = realloc(PRIVATE_DATA->dslr_properties, ++PRIVATE_DATA->dslr_properties_count * sizeof(indigo_property *));
	else
		PRIVATE_DATA->dslr_properties = malloc((PRIVATE_DATA->dslr_properties_count = 1) * sizeof(indigo_property *));
	switch (type) {
		case INDIGO_SWITCH_VECTOR: {
			property = indigo_init_switch_property(NULL, device->name, name, group, label, INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
			break;
		}
		case INDIGO_NUMBER_VECTOR: {
			property = indigo_init_number_property(NULL, device->name, name, group, label, INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
			indigo_init_number_item(property->items, "VALUE", "Value", 0, 65535, 1, 0);
			break;
		}
		default: {
			property = indigo_init_text_property(NULL, device->name, name, group, label, INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
			indigo_init_text_item(property->items, "VALUE", "Value", "");
			break;
		}
	}
	PRIVATE_DATA->dslr_properties[PRIVATE_DATA->dslr_properties_count - 1] = property;
	return PRIVATE_DATA->dslr_properties_count - 1;
}

-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code value:(NSString *)value values:(NSArray<NSString *> *)values labels:(NSArray<NSString *> *)labels readOnly:(BOOL)readOnly {
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	int index = [self propertyIndex:camera code:code type:INDIGO_SWITCH_VECTOR];
  indigo_property *property = PRIVATE_DATA->dslr_properties[index];
  bool redefine = (property->perm != (readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM));
	redefine = redefine || (property->count != (int)values.count);
	if (!redefine) {
		int index = 0;
		for (NSString *key in values) {
			char name[INDIGO_NAME_SIZE];
			strncpy(name, [key cStringUsingEncoding:NSUTF8StringEncoding], INDIGO_NAME_SIZE);
			if (strcmp(property->items[index].name, name)) {
				redefine = true;
				break;
			}
			index++;
		}
	}
	switch (code) {
    case PTPPropertyCodeExposureTime:
    case PTPPropertyCodeSonyShutterSpeed:
    case PTPPropertyCodeCanonShutterSpeed: {
      if (property->perm == INDIGO_RW_PERM) {
        int intValue = value.intValue;
        if ((code == PTPPropertyCodeCanonShutterSpeed && intValue == 0x0C) || (code == PTPPropertyCodeSonyShutterSpeed && intValue == 0) || (code == PTPPropertyCodeExposureTime && intValue == 0x7FFFFFFF)) {
          if (IS_CONNECTED && CCD_EXPOSURE_ITEM->number.max == 0) {
            indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL);
            CCD_EXPOSURE_ITEM->number.min = 0;
            CCD_EXPOSURE_ITEM->number.max = 10000;
            indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
          }
          PRIVATE_DATA->bulb = true;
        } else {
          if (IS_CONNECTED && CCD_EXPOSURE_ITEM->number.max != 0)
            indigo_delete_property(device, CCD_EXPOSURE_PROPERTY, NULL); {
            CCD_EXPOSURE_ITEM->number.value = CCD_EXPOSURE_ITEM->number.min = CCD_EXPOSURE_ITEM->number.max = 0;
            indigo_define_property(device, CCD_EXPOSURE_PROPERTY, NULL);
          }
          PRIVATE_DATA->bulb = false;
        }
      }
      break;
    }
		case PTPPropertyCodeCompressionSetting: {
			if (value.intValue >= 4 && value.intValue <= 8) {
				if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
					indigo_set_switch(CCD_IMAGE_FORMAT_PROPERTY, CCD_IMAGE_FORMAT_RAW_ITEM, true);
					indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
				}
			} else {
				if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
					indigo_set_switch(CCD_IMAGE_FORMAT_PROPERTY, CCD_IMAGE_FORMAT_JPEG_ITEM, true);
					indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
				}
			}
			break;
		}
    case PTPPropertyCodeCanonImageFormat: {
      if (value.intValue == 0x10060002 || value.intValue == 0x10060004 || value.intValue ==  0x10060006) {
        if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value) {
          indigo_set_switch(CCD_IMAGE_FORMAT_PROPERTY, CCD_IMAGE_FORMAT_RAW_ITEM, true);
          indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
        }
      } else {
        if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value) {
          indigo_set_switch(CCD_IMAGE_FORMAT_PROPERTY, CCD_IMAGE_FORMAT_JPEG_ITEM, true);
          indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
        }
      }
      break;
    }
	}
	property->hidden = false;
	if (redefine) {
		if (IS_CONNECTED)
			indigo_delete_property(device, property, NULL);
		PRIVATE_DATA->dslr_properties[index] = property = indigo_resize_property(property, (int)values.count);
		property->perm = readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM;
		int i = 0;
    char name[INDIGO_NAME_SIZE];
    char label[INDIGO_VALUE_SIZE];
		for (NSString *key in values) {
			strncpy(name, [key cStringUsingEncoding:NSUTF8StringEncoding], INDIGO_NAME_SIZE);
			strncpy(label, [labels[i] cStringUsingEncoding:NSUTF8StringEncoding], INDIGO_VALUE_SIZE);
			indigo_init_switch_item(property->items + i, name, label, [key isEqual:value]);
			i++;
		}
		if (IS_CONNECTED)
			indigo_define_property(device, property, NULL);
	} else {
    bool update = false;
		int i = 0;
    for (NSObject *object in values) {
      bool selected = [object isEqual:value];
      if (property->items[i].sw.value != selected) {
        property->items[i].sw.value = selected;
        update = true;
      }
      i++;
    }
    if (update) {
      property->state = INDIGO_OK_STATE;
      indigo_update_property(device, property, NULL);
    }
	}
}

-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code value:(NSNumber *)value min:(NSNumber *)min max:(NSNumber *)max step:(NSNumber *)step readOnly:(BOOL)readOnly {
  indigo_device *device = [(NSValue *)camera.userData pointerValue];
	int index = [self propertyIndex:camera code:code type:INDIGO_NUMBER_VECTOR];
	indigo_property *property = PRIVATE_DATA->dslr_properties[index];
	bool redefine = (property->perm != (readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM));
	redefine = redefine || (property->items[0].number.min != min.intValue);
	redefine = redefine || (property->items[0].number.max != max.intValue);
	redefine = redefine || (property->items[0].number.step != step.intValue);
	property->hidden = false;
	if (redefine) {
		if (IS_CONNECTED)
			indigo_delete_property(device, property, NULL);
		property->perm = readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM;
		property->items[0].number.min = min.intValue;
		property->items[0].number.max = max.intValue;
		property->items[0].number.step = step.intValue;
		property->items[0].number.value = value.intValue;
		if (IS_CONNECTED)
			indigo_define_property(device, property, NULL);
	} else if (property->items[0].number.value != value.intValue) {
		property->items[0].number.value = value.intValue;
    property->state = INDIGO_OK_STATE;
		indigo_update_property(device, property, NULL);
	}
}

-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code value:(NSString *)value readOnly:(BOOL)readOnly {
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	int index = [self propertyIndex:camera code:code type:INDIGO_TEXT_VECTOR];
	indigo_property *property = PRIVATE_DATA->dslr_properties[index];
	bool redefine = (property->perm != (readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM));
  char string[INDIGO_VALUE_SIZE];
  strncpy(string, [value cStringUsingEncoding:NSUTF8StringEncoding], INDIGO_VALUE_SIZE);
	property->hidden = false;
	if (redefine) {
		if (IS_CONNECTED)
			indigo_delete_property(device, property, NULL);
		property->perm = readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM;
		strcpy(property->items[0].text.value,string);
		if (IS_CONNECTED)
			indigo_define_property(device, property, NULL);
	} else if (strcmp(property->items[0].text.value, string)) {
		strcpy(property->items[0].text.value, string);
    property->state = INDIGO_OK_STATE;
		indigo_update_property(device, property, NULL);
	}
}

-(void)cameraPropertyChanged:(PTPCamera *)camera code:(PTPPropertyCode)code readOnly:(BOOL)readOnly {
  indigo_device *device = [(NSValue *)camera.userData pointerValue];
  int index = [self propertyIndex:camera code:code type:0];
  if (index >= 0) {
    indigo_property *property = PRIVATE_DATA->dslr_properties[index];
    indigo_property_perm perm = (readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM);
    if (property->perm != perm) {
      property->perm = perm;
      property->state = INDIGO_OK_STATE;
      indigo_update_property(device, property, NULL);
    }
  }
}

-(void)cameraFrame:(PTPCamera*)camera left:(int)left top:(int)top width:(int)width height:(int)height {
  indigo_device *device = [(NSValue *)camera.userData pointerValue];
  CCD_FRAME_LEFT_ITEM->number.value = left;
  CCD_FRAME_TOP_ITEM->number.value = top;
  if (width > 0)
    CCD_FRAME_WIDTH_ITEM->number.value = width;
  else
    CCD_FRAME_WIDTH_ITEM->number.value = CCD_INFO_WIDTH_ITEM->number.value;
  if (height > 0)
    CCD_FRAME_HEIGHT_ITEM->number.value = height;
  else
    CCD_FRAME_HEIGHT_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.value;
  CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
  indigo_update_property(device, CCD_FRAME_PROPERTY, NULL);
}

-(void)cameraExposureDone:(PTPCamera*)camera data:(NSData *)data filename:(NSString *)filename {
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	filename = filename.lowercaseString;
	NSString *extension = [@"." stringByAppendingString:filename.pathExtension];
	if ([extension isEqualToString:@".jpg"])
		extension = @".jpeg";
  int length = (int)data.length;
  if (PRIVATE_DATA->buffer == NULL)
    PRIVATE_DATA->buffer = malloc(length);
  else if (PRIVATE_DATA->buffer_size < length)
    PRIVATE_DATA->buffer = realloc(PRIVATE_DATA->buffer, length);
  memcpy(PRIVATE_DATA->buffer, data.bytes, length);
	indigo_process_dslr_image(device, PRIVATE_DATA->buffer, length, [extension cStringUsingEncoding:NSUTF8StringEncoding]);
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
  }
	if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (CCD_STREAMING_COUNT_ITEM->number.value > 0) {
			CCD_STREAMING_COUNT_ITEM->number.value--;
			if (CCD_STREAMING_COUNT_ITEM->number.value == 0) {
				[camera stopPreview];
				CCD_STREAMING_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
		}
	}
}

-(void)cameraExposureFailed:(PTPCamera*)camera message:(NSString *)message {
  indigo_device *device = [(NSValue *)camera.userData pointerValue];
  CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
  indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
    if (message)
      indigo_update_property(device, CCD_EXPOSURE_PROPERTY, [message cStringUsingEncoding:NSUTF8StringEncoding]);
    else
      indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Failed to exposure");
  } else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_STREAMING_PROPERTY->state = INDIGO_ALERT_STATE;
    if (message)
      indigo_update_property(device, CCD_STREAMING_PROPERTY, [message cStringUsingEncoding:NSUTF8StringEncoding]);
    else
      indigo_update_property(device, CCD_STREAMING_PROPERTY, "Failed to exposure");
  }
}

-(void)cameraFocusDone:(PTPCamera *)camera {
  indigo_device *device = ((ica_private_data *)((indigo_device *)[(NSValue *)camera.userData pointerValue])->private_data)->focuser;
  FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
  indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
}

-(void)cameraFocusFailed:(PTPCamera *)camera message:(NSString *)message {
  indigo_device *device = ((ica_private_data *)((indigo_device *)[(NSValue *)camera.userData pointerValue])->private_data)->focuser;
  FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
  if (message)
    indigo_update_property(device, FOCUSER_STEPS_PROPERTY, [message cStringUsingEncoding:NSUTF8StringEncoding]);
  else
    indigo_update_property(device, FOCUSER_STEPS_PROPERTY, "Failed to focus");
}

-(void)cameraCanExposure:(PTPCamera *)camera {
  indigo_device *device = [(NSValue *)camera.userData pointerValue];
  [camera requestEnableTethering];
  CCD_EXPOSURE_PROPERTY->perm = CCD_ABORT_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
  indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

-(void)cameraCanFocus:(PTPCamera *)camera {
  indigo_device *device = [(NSValue *)camera.userData pointerValue];
  static indigo_device focuser_template = {
    "", false, NULL, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    focuser_attach,
    indigo_focuser_enumerate_properties,
    focuser_change_property,
    NULL,
    focuser_detach
  };
  indigo_device *focuser = malloc(sizeof(indigo_device));
  assert(focuser != NULL);
  memcpy(focuser, &focuser_template, sizeof(indigo_device));
  strcpy(focuser->name, device->name);
  strcat(focuser->name, " (focuser)");
  focuser->private_data = PRIVATE_DATA;
	PRIVATE_DATA->focuser = focuser;
  indigo_async((void *)(void *)indigo_attach_device, focuser);
}

-(void)cameraCanPreview:(PTPCamera *)camera {
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	CCD_STREAMING_PROPERTY->hidden = false;
  CCD_FRAME_PROPERTY->perm = CCD_ABORT_EXPOSURE_PROPERTY->perm = INDIGO_RW_PERM;
}

-(void)cameraDisconnected:(PTPCamera*)camera {
	indigo_log("%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	if (device) {
    indigo_device *focuser = PRIVATE_DATA->focuser;
    if (focuser) {
      indigo_detach_device(focuser);
      free(focuser);
      PRIVATE_DATA->focuser = NULL;
    }
		for (int i = 0; i < PRIVATE_DATA->dslr_properties_count; i++)
			indigo_delete_property(device, PRIVATE_DATA->dslr_properties[i], NULL);
    indigo_delete_property(device, DSLR_LOCK_PROPERTY, NULL);
    indigo_delete_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
    indigo_delete_property(device, DSLR_AF_PROPERTY, NULL);
    indigo_delete_property(device, DSLR_AVOID_AF_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	}
}

-(void)cameraRemoved:(PTPCamera *)camera {
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
	indigo_device *device = [(NSValue *)camera.userData pointerValue];
	if (device) {
		indigo_detach_device(device);
    if (PRIVATE_DATA->buffer)
      free(PRIVATE_DATA->buffer);
    if (PRIVATE_DATA->dslr_properties)
      free(PRIVATE_DATA->dslr_properties);
		free(PRIVATE_DATA);
		free(device);
	}
}

-(void)log:(NSString *)message {
  INDIGO_DRIVER_LOG(DRIVER_NAME, "%s", [message cStringUsingEncoding:NSUTF8StringEncoding]);
}

-(void)debug:(NSString *)message {
  INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", [message cStringUsingEncoding:NSUTF8StringEncoding]);
}

@end

// -------------------------------------------------------------------------------- ICA interface

indigo_result indigo_ccd_ica(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
  static PTPBrowser* browser;
  static PTPDelegate* delegate;
  
	SET_DRIVER_INFO(info, "ICA Camera", __FUNCTION__, DRIVER_VERSION, last_action);
	
  if (delegate == NULL)
    delegate = [[PTPDelegate alloc] init];
	if (browser == NULL)
    browser = [[PTPBrowser alloc] initWithDelegate:delegate];
	
	if (action == last_action)
		return INDIGO_OK;
	
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			[browser start];
			break;
		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			[browser stop];
			break;
		case INDIGO_DRIVER_INFO:
			break;
	}
	
	return INDIGO_OK;
}
