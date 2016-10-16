//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Code is based on OpenSSAG library
//  Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Orion StarShoot AutoGuider driver
 \file indigo_ccd_ssag.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include "indigo_ccd_ssag.h"
#include "indigo_driver_xml.h"

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((ssag_private_data *)DEVICE_CONTEXT->private_data)

// -------------------------------------------------------------------------------- SX USB interface implementation

typedef struct {
  libusb_device *dev;
  libusb_device_handle *handle;
  int device_count;
  double exposure;
  unsigned char *buffer;
  indigo_timer *exposure_timer;
} ssag_private_data;

typedef enum {
  guide_east  = 0x10,
  guide_south = 0x20,
  guide_north = 0x40,
  guide_west  = 0x80,
} guide_direction;

bool ssag_open(indigo_device *device) {
  return false;
}

bool ssag_start_exposure(indigo_device *device, double exposure) {
  return false;
}

bool ssag_abort_exposure(indigo_device *device) {
  return false;
}

bool ssag_read_pixels(indigo_device *device) {
  return false;
}

bool ssag_guide(indigo_device *device, guide_direction direction, int duration) {
  return false;
}

bool ssag_close(indigo_device *device) {
  return false;
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void exposure_timer_callback(indigo_device *device) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_ITEM->number.value = 0;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
    if (ssag_read_pixels(device)) {
      CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
      indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure done");
      indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->exposure);
    } else {
      CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
      indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
    }
  }
}

static indigo_result ccd_attach(indigo_device *device) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  ssag_private_data *private_data = device->device_context;
  device->device_context = NULL;
  if (indigo_ccd_device_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
    DEVICE_CONTEXT->private_data = private_data;
    // -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_FRAME
    CCD_BIN_PROPERTY->hidden = false;
    CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 1;
    CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 1;
    CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 8;
    CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = 1280;
    CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = 1024;
    CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = 5.2;
    CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
    // --------------------------------------------------------------------------------
    INDIGO_LOG(indigo_log("%s attached", device->name));
    return indigo_ccd_device_enumerate_properties(device, NULL, NULL);
  }
  return INDIGO_FAILED;
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_COOLER, CCD_TEMPERATURE
    indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
    if (CONNECTION_CONNECTED_ITEM->sw.value) {
      if (ssag_open(device)) {
        CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
      } else {
        CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
        indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
      }
    } else {
      ssag_close(device);
      CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
    }
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
    ssag_start_exposure(device, CCD_EXPOSURE_ITEM->number.value);
    CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure initiated");
    if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
      CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
    } else {
      CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
    }
    PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value, exposure_timer_callback);
  } else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
    if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      ssag_abort_exposure(device);
      indigo_cancel_timer(device, PRIVATE_DATA->exposure_timer);
    }
    indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
    // --------------------------------------------------------------------------------
  }
  return indigo_ccd_device_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
  assert(device != NULL);
  INDIGO_LOG(indigo_log("%s detached", device->name));
  return indigo_ccd_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO guider device implementation

static indigo_result guider_attach(indigo_device *device) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  ssag_private_data *private_data = device->device_context;
  device->device_context = NULL;
  if (indigo_guider_device_attach(device, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
    DEVICE_CONTEXT->private_data = private_data;
    INDIGO_LOG(indigo_log("%s attached", device->name));
    return indigo_guider_device_enumerate_properties(device, NULL, NULL);
  }
  return INDIGO_FAILED;
}

static indigo_result guider_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
    if (CONNECTION_CONNECTED_ITEM->sw.value) {
      if (ssag_open(device)) {
        CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
      } else {
        CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
        indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
      }
    } else {
      ssag_close(device);
      CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
    }
  } else if (indigo_property_match(GUIDER_GUIDE_DEC_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- GUIDER_GUIDE_DEC
    indigo_property_copy_values(GUIDER_GUIDE_DEC_PROPERTY, property, false);
    int duration = GUIDER_GUIDE_NORTH_ITEM->number.value;
    if (duration > 0) {
      ssag_guide(device, guide_north, duration);
    } else {
      int duration = GUIDER_GUIDE_SOUTH_ITEM->number.value;
      if (duration > 0) {
        ssag_guide(device, guide_south, duration);
      }
    }
    GUIDER_GUIDE_DEC_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, GUIDER_GUIDE_DEC_PROPERTY, NULL);
    return INDIGO_OK;
  } else if (indigo_property_match(GUIDER_GUIDE_RA_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- GUIDER_GUIDE_RA
    indigo_property_copy_values(GUIDER_GUIDE_RA_PROPERTY, property, false);
    int duration = GUIDER_GUIDE_EAST_ITEM->number.value;
    if (duration > 0) {
      ssag_guide(device, guide_east, duration);
    } else {
      duration = GUIDER_GUIDE_WEST_ITEM->number.value;
      if (duration > 0) {
        ssag_guide(device, guide_west, duration);
      }
    }
    GUIDER_GUIDE_RA_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, GUIDER_GUIDE_RA_PROPERTY, NULL);
    return INDIGO_OK;
    // --------------------------------------------------------------------------------
  }
  return indigo_guider_device_change_property(device, client, property);
}

static indigo_result guider_detach(indigo_device *device) {
  assert(device != NULL);
  INDIGO_LOG(indigo_log("%s detached", device->name));
  return indigo_guider_device_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES 10

static indigo_device *devices[MAX_DEVICES];

static int ssag_hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
  static indigo_device ccd_template = {
    "", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    ccd_attach,
    indigo_ccd_device_enumerate_properties,
    ccd_change_property,
    ccd_detach
  };
  static indigo_device guider_template = {
    "", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    guider_attach,
    indigo_guider_device_enumerate_properties,
    guider_change_property,
    guider_detach
  };
  struct libusb_device_descriptor descriptor;
  switch (event) {
    case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
      libusb_get_device_descriptor(dev, &descriptor);
      for (int i = 0; SX_PRODUCTS[i].name; i++) {
        if (descriptor.idVendor == 0x1278 && SX_PRODUCTS[i].product == descriptor.idProduct) {
          struct libusb_device_descriptor descriptor;
          libusb_get_device_descriptor(dev, &descriptor);
          ssag_private_data *private_data = malloc(sizeof(ssag_private_data));
          memset(private_data, 0, sizeof(ssag_private_data));
          private_data->dev = dev;
          indigo_device *device = malloc(sizeof(indigo_device));
          if (device != NULL) {
            memcpy(device, &ccd_template, sizeof(indigo_device));
            strcpy(device->name, SX_PRODUCTS[i].name);
            device->device_context = private_data;
            for (int j = 0; j < MAX_DEVICES; j++) {
              if (devices[j] == NULL) {
                indigo_attach_device(devices[j] = device);
                break;
              }
            }
          }
          device = malloc(sizeof(indigo_device));
          if (device != NULL) {
            memcpy(device, &guider_template, sizeof(indigo_device));
            strcpy(device->name, SX_PRODUCTS[i].name);
            strcat(device->name, " guider");
            device->device_context = private_data;
            for (int j = 0; j < MAX_DEVICES; j++) {
              if (devices[j] == NULL) {
                indigo_attach_device(devices[j] = device);
                break;
              }
            }
          }
          return 0;
        }
      }
      break;
    }
    case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
      ssag_private_data *private_data = NULL;
      for (int j = 0; j < MAX_DEVICES; j++) {
        if (devices[j] != NULL) {
          indigo_device *device = devices[j];
          if (PRIVATE_DATA->dev == dev) {
            private_data = PRIVATE_DATA;
            indigo_detach_device(device);
            free(device);
            devices[j] = NULL;
          }
        }
      }
      if (private_data != NULL)
        free(private_data);
      break;
    }
  }
  return 0;
};

#ifdef INDIGO_LINUX
static void *hotplug_thread(void *arg) {
  while (true) {
    libusb_handle_events(NULL);
  }
  return NULL;
}
#endif

indigo_result indigo_ccd_sx() {
  for (int i = 0; i < MAX_DEVICES; i++) {
    devices[i] = 0;
  }
  libusb_init(NULL);
  int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, ssag_hotplug_callback, NULL, NULL);
  INDIGO_DEBUG(indigo_debug("indigo_ccd_ssag: libusb_hotplug_register_callback [%d] ->  %s", __LINE__, libusb_error_name(rc)));
#ifdef INDIGO_LINUX
  pthread_t hotplug_thread_handle;
  pthread_create(&hotplug_thread_handle, NULL, hotplug_thread, NULL);
#endif
  return rc >= 0;
}
