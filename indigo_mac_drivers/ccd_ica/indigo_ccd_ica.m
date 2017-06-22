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

#include "indigo_ccd_ica.h"
#include "indigo_ica_ptp.h"

static struct info {
  const char *match;
  const char *name;
  int width, height;
  float pixel_size;
} info[] = {
  { "D3", "Nikon D3", 4256, 2832, 8.45 },
  { "D300", "Nikon D300", 4288, 2848, 5.50 },
  { "D300S", "Nikon D300S", 4288, 2848, 5.50 },
  { "D3A", "Nikon D3A", 4256, 2832, 8.45 },
  { "D4", "Nikon D4", 4928, 3280, 7.30 },
  { "D4S", "Nikon D4S", 4928, 3280, 7.30 },
  { "D5", "Nikon D5", 5568, 3712, 6.40 },
  { "D500", "Nikon D500", 5568, 3712, 4.23 },
  { "D5000", "Nikon D5000", 4288, 2848, 5.50 },
  { "D5100", "Nikon D5100", 4928, 3264, 4.78 },
  { "D5200", "Nikon D5200", 6000, 4000, 3.92 },
  { "D5300", "Nikon D5300", 6000, 4000, 3.92 },
  { "D5500", "Nikon D5500", 6000, 4000, 3.92 },
  { "D600", "Nikon D600", 6016, 4016, 5.95 },
  { "D610", "Nikon D610", 6016, 4016, 5.95 },
  { "D70", "Nikon D70", 3008, 2000, 7.87 },
  { "D70s", "Nikon D70s", 3008, 2000, 7.87 },
  { "D700", "Nikon D700", 4256, 2832, 8.45 },
  { "D7000", "Nikon D7000", 4928, 3264, 4.78 },
  { "D7100", "Nikon D7100", 6000, 4000, 3.92 },
  { "D7200", "Nikon D7200", 6000, 4000, 3.92 },
  { "D750", "Nikon D750", 6016, 4016, 3.92 },
  { "D800", "Nikon D800", 7360, 4912, 4.88 },
  { "D800E", "Nikon D800E", 7360, 4912, 4.88 },
  { "D810", "Nikon D810", 7360, 4912, 4.88 },
  { "D810A", "Nikon D810A", 7360, 4912, 4.88 },
  { "D90", "Nikon D90", 4288, 2848, 5.50 },
  
  { "Canon EOS REBEL XTI", "Canon Rebel XTI", 3888, 2592, 5.7 },
  { "Canon EOS REBEL XT", "Canon Rebel XT", 3456, 2304, 6.4 },
  { "Canon EOS REBEL XSI", "Canon Rebel XSI", 4272, 2848, 5.19 },
  { "Canon EOS REBEL XS", "Canon Rebel XS", 3888, 2592, 5.7 },
  { "Canon EOS REBEL T1I", "Canon Rebel T1I", 4752, 3168, 4.69 },
  { "Canon EOS REBEL T2I", "Canon Rebel T2I", 5184, 3456, 4.3 },
  { "Canon EOS REBEL T3I", "Canon Rebel T3I", 5184, 3456, 4.3 },
  { "Canon EOS REBEL T3", "Canon Rebel T3", 4272, 2848, 5.19 },
  { "Canon EOS REBEL T4I", "Canon Rebel T4I", 5184, 3456, 4.3 },
  { "Canon EOS REBEL T5I", "Canon Rebel T5I", 5184, 3456, 4.3 },
  { "Canon EOS REBEL T5", "Canon Rebel T5", 5184, 3456, 4.3 },
  { "Canon EOS REBEL T6I", "Canon Rebel T6I", 6000, 4000, 3.71 },
  { "Canon EOS REBEL T6S", "Canon Rebel T6S", 6000, 4000, 3.71 },
  { "Canon EOS REBEL T6", "Canon Rebel T6", 5184, 3456, 4.3 },
  { "Canon EOS REBEL SL1", "Canon Rebel SL1", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X2", "Canon Kiss X2", 4272, 2848, 5.19 },
  { "Canon EOS Kiss X3", "Canon Kiss X3", 4752, 3168, 4.69 },
  { "Canon EOS Kiss X4", "Canon Kiss X4", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X50", "Canon Kiss X50", 4272, 2848, 5.19 },
  { "Canon EOS Kiss X5", "Canon Kiss X5", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X6I", "Canon Kiss X6I", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X7I", "Canon Kiss X7I", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X70", "Canon Kiss X70", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X7", "Canon Kiss X7", 5184, 3456, 4.3 },
  { "Canon EOS Kiss X8I", "Canon Kiss X8I", 6000, 4000, 3.71 },
  { "Canon EOS Kiss X80", "Canon Kiss X80", 5184, 3456, 4.3 },
  { "Canon EOS Kiss F", "Canon Kiss F", 3888, 2592, 5.7 },
  { "Canon EOS 1000D", "Canon EOS 1000D", 3888, 2592, 5.7 },
  { "Canon EOS 1100D", "Canon EOS 1100D", 4272, 2848, 5.19 },
  { "Canon EOS 1200D", "Canon EOS 1200D", 5184, 3456, 4.3 },
  { "Canon EOS 1300D", "Canon EOS 1300D", 5184, 3456, 4.3 },
  { "Canon EOS 8000D", "Canon EOS 8000D", 6000, 4000, 3.71 },
  { "Canon EOS 100D", "Canon EOS 100D", 5184, 3456, 4.3 },
  { "Canon EOS 350D", "Canon EOS 350D", 3456, 2304, 6.4 },
  { "Canon EOS 400D", "Canon EOS 400D", 3888, 2592, 5.7 },
  { "Canon EOS 450D", "Canon EOS 450D", 4272, 2848, 5.19 },
  { "Canon EOS 500D", "Canon EOS 500D", 4752, 3168, 4.69 },
  { "Canon EOS 550D", "Canon EOS 550D", 5184, 3456, 4.3 },
  { "Canon EOS 600D", "Canon EOS 600D", 5184, 3456, 4.3 },
  { "Canon EOS 650D", "Canon EOS 650D", 5184, 3456, 4.3 },
  { "Canon EOS 700D", "Canon EOS 700D", 5184, 3456, 4.3 },
  { "Canon EOS 750D", "Canon EOS 750D", 6000, 4000, 3.71 },
  { "Canon EOS 760D", "Canon EOS 760D", 6000, 4000, 3.71 },
  { "Canon EOS 20D", "Canon EOS 20D", 3520, 2344, 6.4 },
  { "Canon EOS 20DA", "Canon EOS 20DA", 3520, 2344, 6.4 },
  { "Canon EOS 30D", "Canon EOS 30D", 3520, 2344, 6.4 },
  { "Canon EOS 40D", "Canon EOS 40D", 3888, 2592, 5.7 },
  { "Canon EOS 50D", "Canon EOS 50D", 4752, 3168, 4.69 },
  { "Canon EOS 60D", "Canon EOS 60D", 5184, 3456, 4.3 },
  { "Canon EOS 70D", "Canon EOS 70D", 5472, 3648, 6.54 },
  { "Canon EOS 80D", "Canon EOS 80D", 6000, 4000, 3.71 },
  { "Canon EOS 1DS MARK III", "Canon EOS 1DS", 5616, 3744, 6.41 },
  { "Canon EOS 1D MARK III", "Canon EOS 1D", 3888, 2592, 5.7 },
  { "Canon EOS 1D MARK IV", "Canon EOS 1D", 4896, 3264, 5.69 },
  { "Canon EOS 1D X MARK II", "Canon EOS 1D", 5472, 3648, 6.54 },
  { "Canon EOS 1D X", "Canon EOS 1D X", 5472, 3648, 6.54 },
  { "Canon EOS 1D C", "Canon EOS 1D C", 5184, 3456, 4.3 },
  { "Canon EOS 5D MARK II", "Canon EOS 5D", 5616, 3744, 6.41 },
  { "Canon EOS 5DS", "Canon EOS 5DS", 8688, 5792, 4.14 },
  { "Canon EOS 5D", "Canon EOS 5D", 4368, 2912, 8.2 },
  { "Canon EOS 6D", "Canon EOS 6D", 5472, 3648, 6.54 },
  { "Canon EOS 7D MARK II", "Canon EOS 7D", 5472, 3648, 4.07 },
  { "Canon EOS 7D", "Canon EOS 7D", 5184, 3456, 4.3 },
  
  { NULL, NULL, 0, 0, 0 }
};

#define DEVICE @"INDIGO_DEVICE"

#define PRIVATE_DATA        ((ica_private_data *)device->private_data)

#define EXPOSURE_PROGRAM_PROPERTY	PRIVATE_DATA->exposure_program_property
#define APERTURE_PROPERTY					PRIVATE_DATA->aperture_property
#define SHUTTER_PROPERTY					PRIVATE_DATA->shutter_property
#define IMAGE_SIZE_PROPERTY				PRIVATE_DATA->image_size_property
#define COMPRESSION_PROPERTY			PRIVATE_DATA->compression_property
#define WHITE_BALANCE_PROPERTY		PRIVATE_DATA->white_balance_property
#define ISO_PROPERTY							PRIVATE_DATA->iso_property

typedef struct {
  void* camera;
  struct info *info;
  bool tethering_available;
  indigo_property *exposure_program_property;
  indigo_property *aperture_property;
  indigo_property *shutter_property;
  indigo_property *image_size_property;
  indigo_property *compression_property;
  indigo_property *white_balance_property;
  indigo_property *iso_property;
} ica_private_data;

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static indigo_result ccd_attach(indigo_device *device) {
  assert(device != NULL);
  assert(PRIVATE_DATA != NULL);
  if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {
    // --------------------------------------------------------------------------------
    if (PRIVATE_DATA->info) {
      CCD_INFO_PROPERTY->hidden = false;
      CCD_INFO_WIDTH_ITEM->number.value =  PRIVATE_DATA->info->width;
      CCD_INFO_HEIGHT_ITEM->number.value = PRIVATE_DATA->info->height;
      CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = PRIVATE_DATA->info->pixel_size;
    } else {
      CCD_INFO_PROPERTY->hidden = CCD_FRAME_PROPERTY->hidden = true;
    }
    CCD_BIN_PROPERTY->hidden =  CCD_FRAME_PROPERTY->hidden = true;
    CCD_IMAGE_FORMAT_PROPERTY->perm = INDIGO_RO_PERM;
    EXPOSURE_PROGRAM_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_PROGRAM_PROPERTY_NAME, "DSLR", "Exposure program", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    EXPOSURE_PROGRAM_PROPERTY->hidden = true;
    APERTURE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_APERTURE_PROPERTY_NAME, "DSLR", "Aperture", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    APERTURE_PROPERTY->hidden = true;
    SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_SHUTTER_PROPERTY_NAME, "DSLR", "Shutter", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    SHUTTER_PROPERTY->hidden = true;
    IMAGE_SIZE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_IMAGE_SIZE_PROPERTY_NAME, "DSLR", "Image size", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    IMAGE_SIZE_PROPERTY->hidden = true;
    COMPRESSION_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_COMPRESSION_PROPERTY_NAME, "DSLR", "Compression", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    COMPRESSION_PROPERTY->hidden = true;
    WHITE_BALANCE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_WHITE_BALANCE_PROPERTY_NAME, "DSLR", "White balance", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    WHITE_BALANCE_PROPERTY->hidden = true;
    ISO_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_ISO_PROPERTY_NAME, "DSLR", "ISO", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
    ISO_PROPERTY->hidden = true;

    // --------------------------------------------------------------------------------
    INDIGO_DRIVER_LOG(DRIVER_NAME, "%s attached", device->name);
    return indigo_ccd_enumerate_properties(device, NULL, NULL);
  }
  return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
  indigo_result result = INDIGO_OK;
  if ((result = indigo_ccd_enumerate_properties(device, client, property)) == INDIGO_OK) {
    if (IS_CONNECTED) {
      if (indigo_property_match(EXPOSURE_PROGRAM_PROPERTY, property))
        indigo_define_property(device, EXPOSURE_PROGRAM_PROPERTY, NULL);
      if (indigo_property_match(APERTURE_PROPERTY, property))
        indigo_define_property(device, APERTURE_PROPERTY, NULL);
      if (indigo_property_match(SHUTTER_PROPERTY, property))
        indigo_define_property(device, SHUTTER_PROPERTY, NULL);
      if (indigo_property_match(IMAGE_SIZE_PROPERTY, property))
        indigo_define_property(device, IMAGE_SIZE_PROPERTY, NULL);
      if (indigo_property_match(COMPRESSION_PROPERTY, property))
        indigo_define_property(device, COMPRESSION_PROPERTY, NULL);
      if (indigo_property_match(WHITE_BALANCE_PROPERTY, property))
        indigo_define_property(device, WHITE_BALANCE_PROPERTY, NULL);
      if (indigo_property_match(ISO_PROPERTY, property))
        indigo_define_property(device, ISO_PROPERTY, NULL);
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
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    if (CONNECTION_CONNECTED_ITEM->sw.value) {
      [camera requestOpenSession];
    } else {
      [camera requestCloseSession];
    }
    CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CONNECTION_PROPERTY, NULL);
    return INDIGO_OK;
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE)
      return INDIGO_OK;
    indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
    CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
    [camera requestTakePicture];
    return INDIGO_OK;
  } else if (indigo_property_match(APERTURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_APERTURE
    indigo_property_copy_values(APERTURE_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    for (int i = 0; i < APERTURE_PROPERTY->count; i++) {
      if (APERTURE_PROPERTY->items[i].sw.value) {
        APERTURE_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, APERTURE_PROPERTY, NULL);
        [camera setAperture:[NSNumber numberWithInt:atoi(APERTURE_PROPERTY->items[i].name)]];
        break;
      }
    }
    return INDIGO_OK;
  } else if (indigo_property_match(SHUTTER_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_SHUTTER
    indigo_property_copy_values(SHUTTER_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    for (int i = 0; i < SHUTTER_PROPERTY->count; i++) {
      if (SHUTTER_PROPERTY->items[i].sw.value) {
        SHUTTER_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, SHUTTER_PROPERTY, NULL);
        [camera setShutter:[NSNumber numberWithInt:atoi(SHUTTER_PROPERTY->items[i].name)]];
        break;
      }
    }
    return INDIGO_OK;
  } else if (indigo_property_match(IMAGE_SIZE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_IMAGE_SIZE
    indigo_property_copy_values(IMAGE_SIZE_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    for (int i = 0; i < IMAGE_SIZE_PROPERTY->count; i++) {
      if (IMAGE_SIZE_PROPERTY->items[i].sw.value) {
        IMAGE_SIZE_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, IMAGE_SIZE_PROPERTY, NULL);
        [camera setImageSize:[NSString stringWithCString:IMAGE_SIZE_PROPERTY->items[i].name encoding:NSUTF8StringEncoding]];
        break;
      }
    }
    return INDIGO_OK;
  } else if (indigo_property_match(COMPRESSION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_COMPRESSION
    indigo_property_copy_values(COMPRESSION_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    for (int i = 0; i < COMPRESSION_PROPERTY->count; i++) {
      if (COMPRESSION_PROPERTY->items[i].sw.value) {
        COMPRESSION_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, COMPRESSION_PROPERTY, NULL);
        [camera setCompression:[NSNumber numberWithInt:atoi(COMPRESSION_PROPERTY->items[i].name)]];
        break;
      }
    }
    return INDIGO_OK;
  } else if (indigo_property_match(WHITE_BALANCE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_WHITE_BALANCE
    indigo_property_copy_values(WHITE_BALANCE_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    for (int i = 0; i < WHITE_BALANCE_PROPERTY->count; i++) {
      if (WHITE_BALANCE_PROPERTY->items[i].sw.value) {
        WHITE_BALANCE_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, WHITE_BALANCE_PROPERTY, NULL);
        [camera setWhiteBalance:[NSNumber numberWithInt:atoi(WHITE_BALANCE_PROPERTY->items[i].name)]];
        break;
      }
    }
    return INDIGO_OK;
  } else if (indigo_property_match(ISO_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- DSLR_ISO
    indigo_property_copy_values(ISO_PROPERTY, property, false);
    ICCameraDevice *camera = (__bridge ICCameraDevice *)(PRIVATE_DATA->camera);
    for (int i = 0; i < ISO_PROPERTY->count; i++) {
      if (ISO_PROPERTY->items[i].sw.value) {
        ISO_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, ISO_PROPERTY, NULL);
        [camera setISO:[NSNumber numberWithInt:atoi(ISO_PROPERTY->items[i].name)]];
        break;
      }
    }
    return INDIGO_OK;
    // --------------------------------------------------------------------------------
  }
  return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
  assert(device != NULL);
  if (CONNECTION_CONNECTED_ITEM->sw.value)
    indigo_device_disconnect(NULL, device->name);
  indigo_release_property(EXPOSURE_PROGRAM_PROPERTY);
  indigo_release_property(APERTURE_PROPERTY);
  indigo_release_property(SHUTTER_PROPERTY);
  indigo_release_property(IMAGE_SIZE_PROPERTY);
  indigo_release_property(COMPRESSION_PROPERTY);
  indigo_release_property(WHITE_BALANCE_PROPERTY);
  indigo_release_property(ISO_PROPERTY);
  INDIGO_DRIVER_LOG(DRIVER_NAME, "%s detached", device->name);
  return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- ICA interface

@interface ICADelegate : PTPDelegate
@end

@implementation ICADelegate

- (void)cameraAdded:(ICCameraDevice *)camera {
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
  private_data->camera = (__bridge void *)(camera);
  indigo_device *device = malloc(sizeof(indigo_device));
  assert(device != NULL);
  memcpy(device, &ccd_template, sizeof(indigo_device));
  strcpy(device->name, [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
  for (int i = 0; info[i].match; i++) {
    if (!strcmp(info[i].match, device->name)) {
      strcpy(device->name, info[i].name);
      private_data->info = &info[i];
      break;
    }
  }
  device->private_data = private_data;
  [camera.userData setObject:[NSValue valueWithPointer:device] forKey:DEVICE];
  indigo_async((void *)(void *)indigo_attach_device, device);
}

- (void)cameraConnected:(ICCameraDevice*)camera {
  INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  if (device) {
    if ([camera.capabilities containsObject:ICCameraDeviceCanTakePicture])
      [camera requestEnableTethering];
    else
      CCD_EXPOSURE_PROPERTY->perm = CCD_ABORT_EXPOSURE_PROPERTY->perm = camera.tetheredCaptureEnabled ? INDIGO_RW_PERM : INDIGO_RO_PERM;
    indigo_define_property(device, EXPOSURE_PROGRAM_PROPERTY, NULL);
    indigo_define_property(device, APERTURE_PROPERTY, NULL);
    indigo_define_property(device, SHUTTER_PROPERTY, NULL);
    indigo_define_property(device, IMAGE_SIZE_PROPERTY, NULL);
    indigo_define_property(device, COMPRESSION_PROPERTY, NULL);
    indigo_define_property(device, WHITE_BALANCE_PROPERTY, NULL);
    indigo_define_property(device, ISO_PROPERTY, NULL);
    CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
    indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
  }
}

- (void)devicePropertyChanged:(indigo_device *)device value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly property:(indigo_property **)property {
  bool redefine = ((*property)->perm != (readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM));
  redefine = redefine || ((*property)->count != (int)supportedValues.count);
  int index = 0;
  if (!redefine) {
    for (NSObject *object in [supportedValues.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
      if (strcmp((*property)->items[index].name, [object.description cStringUsingEncoding:NSASCIIStringEncoding])) {
        redefine = true;
        break;
      }
      index++;
    }
  }
  (*property)->hidden = false;
  if (redefine) {
    if (IS_CONNECTED)
      indigo_delete_property(device, *property, NULL);
    *property = indigo_resize_property(*property, (int)supportedValues.count);
    (*property)->perm = readOnly ? INDIGO_RO_PERM : INDIGO_RW_PERM;
    int index = 0;
    for (NSObject *object in [supportedValues.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
      indigo_init_switch_item((*property)->items + index, [object.description cStringUsingEncoding:NSASCIIStringEncoding], [supportedValues[object] cStringUsingEncoding:NSASCIIStringEncoding], [object isEqual:value]);
      index++;
    }
    if (IS_CONNECTED)
      indigo_define_property(device, *property, NULL);
  } else {
    int index = 0;
    for (NSObject *object in [supportedValues.allKeys sortedArrayUsingSelector:@selector(compare:)])
      (*property)->items[index++].sw.value = [object isEqual:value];
    (*property)->state = INDIGO_OK_STATE;
    indigo_update_property(device, *property, NULL);
  }
  
}

- (void)cameraExposureProgramChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraExposureProgramChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(EXPOSURE_PROGRAM_PROPERTY)];
}

- (void)cameraApertureChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraApertureChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(APERTURE_PROPERTY)];
}

- (void)cameraShutterChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraShutterChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(SHUTTER_PROPERTY)];
}

- (void)cameraImageSizeChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraShutterChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(IMAGE_SIZE_PROPERTY)];
}

- (void)cameraCompressionChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraShutterChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(COMPRESSION_PROPERTY)];
}

- (void)cameraWhiteBalanceChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraShutterChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(WHITE_BALANCE_PROPERTY)];
}

- (void)cameraISOChanged:(ICCameraDevice *)camera value:(NSObject *)value supportedValues:(NSDictionary *)supportedValues readOnly:(BOOL)readOnly {
  NSLog(@"cameraShutterChanged %@ -> %@", value, supportedValues[value]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  [self devicePropertyChanged:device value:value supportedValues:supportedValues readOnly:readOnly property:&(ISO_PROPERTY)];
}

- (void)cameraExposureDone:(ICCameraDevice*)camera data:(NSData *)data {
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  indigo_process_dslr_image(device, (void *)data.bytes, (int)data.length);
  CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
  indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
  CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
  indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
}

- (void)cameraExposureFailed:(ICCameraDevice*)camera {
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
  indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
  CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
  indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Failed to exposure");
}

- (void)cameraDisconnected:(ICCameraDevice*)camera {
  INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  if (device) {
    indigo_delete_property(device, EXPOSURE_PROGRAM_PROPERTY, NULL);
    indigo_delete_property(device, APERTURE_PROPERTY, NULL);
    indigo_delete_property(device, SHUTTER_PROPERTY, NULL);
    indigo_delete_property(device, IMAGE_SIZE_PROPERTY, NULL);
    indigo_delete_property(device, COMPRESSION_PROPERTY, NULL);
    indigo_delete_property(device, WHITE_BALANCE_PROPERTY, NULL);
    indigo_delete_property(device, ISO_PROPERTY, NULL);
    CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
    indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
  }
}

- (void)cameraRemoved:(ICCameraDevice *)camera {
  INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s", [camera.name cStringUsingEncoding:NSUTF8StringEncoding]);
  indigo_device *device = [camera.userData[DEVICE] pointerValue];
  if (device) {
    indigo_detach_device(device);
    free(PRIVATE_DATA);
    free(device);
  }
}

@end

// -------------------------------------------------------------------------------- ICA interface

indigo_result indigo_ccd_ica(indigo_driver_action action, indigo_driver_info *info) {
  static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
  static ICDeviceBrowser* deviceBrowser;
  static ICADelegate* icaDelegate;
  
  SET_DRIVER_INFO(info, "ICA Camera", __FUNCTION__, DRIVER_VERSION, last_action);
  
  if (deviceBrowser == NULL) {
    deviceBrowser = [[ICDeviceBrowser alloc] init];
    deviceBrowser.delegate = icaDelegate = [[ICADelegate alloc] init];
    deviceBrowser.browsedDeviceTypeMask = ICDeviceTypeMaskCamera | ICDeviceLocationTypeMaskLocal;
  }
  
  if (action == last_action)
    return INDIGO_OK;
  
  switch (action) {
    case INDIGO_DRIVER_INIT:
      last_action = action;
      [deviceBrowser start];
      break;
    case INDIGO_DRIVER_SHUTDOWN:
      last_action = action;
      [deviceBrowser stop];
      break;
    case INDIGO_DRIVER_INFO:
      break;
  }
  
  return INDIGO_OK;
}
