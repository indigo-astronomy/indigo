//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
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
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
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
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "indigo_ccd_sx.h"
#include "indigo_driver_xml.h"

#define SX_VENDOR_ID  0x1278

#define MAX_DEVICES         5

#define EXPOSURE_TIMER      10
#define TEMPERATURE_TIMER   11

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((sx_private_data *)DEVICE_CONTEXT->private_data)

typedef struct {
  libusb_device *dev;
  libusb_device_handle *handle;
  char name[INDIGO_NAME_SIZE];
  double target_temperature, current_temperature;
} sx_private_data;

static struct {
  int product;
  const char *name;
} SX_PRODUCTS[] = {
  { 0x0105, "SXVF-M5" },
  { 0x0305, "SXVF-M5C" },
  { 0x0107, "SXVF-M7" },
  { 0x0307, "SXVF-M7C" },
  { 0x0308, "SXVF-M8C" },
  { 0x0109, "SXVF-M9" },
  { 0x0325, "SXVR-M25C" },
  { 0x0326, "SXVR-M26C" },
  { 0x0115, "SXVR-H5" },
  { 0x0119, "SXVR-H9" },
  { 0x0319, "SXVR-H9C" },
  { 0x0100, "SXVR-H9" },
  { 0x0300, "SXVR-H9C" },
  { 0x0126, "SXVR-H16" },
  { 0x0128, "SXVR-H18" },
  { 0x0135, "SXVR-H35" },
  { 0x0136, "SXVR-H36" },
  { 0x0137, "SXVR-H360" },
  { 0x0139, "SXVR-H390" },
  { 0x0194, "SXVR-H694" },
  { 0x0394, "SXVR-H694C" },
  { 0x0174, "SXVR-H674" },
  { 0x0374, "SXVR-H674C" },
  { 0x0198, "SX-814" },
  { 0x0398, "SX-814C" },
  { 0x0189, "SX-825" },
  { 0x0389, "SX-825C" },
  { 0x0184, "SX-834" },
  { 0x0384, "SX-834C" },
  { 0x0507, "SX LodeStar" },
  { 0x0517, "SX CoStar" },
  { 0x0509, "SX SuperStar" },
  { 0x0525, "SX UltraStar" },
  { 0x0200, "SXMX Camera" },
  { 0, NULL }
};

static indigo_device *devices[MAX_DEVICES] = { NULL, NULL, NULL, NULL, NULL };

static void exposure_timer_callback(indigo_device *device, unsigned timer_id, double delay) {
  // TBD
}

static void ccd_temperature_callback(indigo_device *device, unsigned timer_id, double delay) {
  // TBD
}

static indigo_result attach(indigo_device *device) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  
  sx_private_data *private_data = device->device_context;
  device->device_context = NULL;
  
  if (indigo_ccd_device_attach(device, private_data->name, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
    DEVICE_CONTEXT->private_data = private_data;
    // -------------------------------------------------------------------------------- SIMULATION
    SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
    SIMULATION_ENABLED_ITEM->switch_value = false;
    SIMULATION_DISABLED_ITEM->switch_value = true;
    // -------------------------------------------------------------------------------- CCD_INFO
    
    // TBD
    
    // -------------------------------------------------------------------------------- CCD_FRAME

    // TBD
    
    // -------------------------------------------------------------------------------- CCD_BIN

    // TBD
    
    // -------------------------------------------------------------------------------- CCD_IMAGE

    // -------------------------------------------------------------------------------- CCD_COOLER, CCD_TEMPERATURE, CCD_COOLER_POWER
    
    // TBD
    
    // --------------------------------------------------------------------------------
    INDIGO_LOG(indigo_log("%s attached", PRIVATE_DATA->name));
    return indigo_ccd_device_enumerate_properties(device, NULL, NULL);
  }
  return INDIGO_FAILED;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
    CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
    CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure initiated");
    if (CCD_UPLOAD_MODE_LOCAL_ITEM->switch_value) {
      CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
    } else {
      CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
    }
    indigo_set_timer(device, EXPOSURE_TIMER, CCD_EXPOSURE_ITEM->number_value, exposure_timer_callback);
  } else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
    if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      indigo_cancel_timer(device, EXPOSURE_TIMER);
    }
    indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
  } else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_COOLER
    indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
    if (CCD_COOLER_ON_ITEM->switch_value) {
      CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RW_PERM;
      CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
      PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number_value;
    } else {
      CCD_TEMPERATURE_PROPERTY->perm = INDIGO_RO_PERM;
      CCD_TEMPERATURE_PROPERTY->state = INDIGO_IDLE_STATE;
      CCD_COOLER_POWER_ITEM->number_value = 0;
      PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number_value = 25;
    }
    indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
    indigo_update_property(device, CCD_COOLER_POWER_PROPERTY, NULL);
    indigo_define_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
  } else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_TEMPERATURE
    indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
    PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number_value;
    CCD_TEMPERATURE_ITEM->number_value = PRIVATE_DATA->current_temperature;
    CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, "Target temperature %g", PRIVATE_DATA->target_temperature);
    // --------------------------------------------------------------------------------
  }
  return indigo_ccd_device_change_property(device, client, property);
}

static indigo_result detach(indigo_device *device) {
  assert(device != NULL);
  INDIGO_LOG(indigo_log("%s detached", PRIVATE_DATA->name));
  free(PRIVATE_DATA);
  return indigo_ccd_device_detach(device);
}

indigo_device *indigo_ccd_sx(libusb_device *dev, const char *name) {
  static indigo_device device_template = {
    NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    attach,
    indigo_ccd_device_enumerate_properties,
    change_property,
    detach
  };
  libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(dev, &descriptor);
  indigo_device *device = malloc(sizeof(indigo_device));
  if (device != NULL) {
    memcpy(device, &device_template, sizeof(indigo_device));
    sx_private_data *private_data = malloc(sizeof(sx_private_data));
    private_data->dev = dev;
    strncpy(private_data->name, name, INDIGO_NAME_SIZE);
    device->device_context = private_data;
  }
  return device;
}

static int indigo_ccd_sx_hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
  libusb_device_descriptor descriptor;
  switch (event) {
    case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
      libusb_get_device_descriptor(dev, &descriptor);
      for (int i = 0; SX_PRODUCTS[i].name; i++) {
        if (descriptor.idVendor == 0x1278 && SX_PRODUCTS[i].product == descriptor.idProduct) {
          for (int j = 0; j < MAX_DEVICES; j++) {
            if (devices[j] == NULL) {
              indigo_attach_device(devices[j] = indigo_ccd_sx(dev, SX_PRODUCTS[i].name));
              return 0;
            }
          }
        }
      }
      break;
    case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
      for (int j = 0; j < MAX_DEVICES; j++) {
        if (devices[j] != NULL) {
          indigo_device *device = devices[j];
          if (((sx_private_data*)PRIVATE_DATA)->dev == dev) {
            indigo_detach_device(device);
            free(device);
            devices[j] = NULL;
            return 0;
          }
        }
      }
      break;
  }
  return 0;
};

indigo_result indigo_ccd_sx_register() {
  if (libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, 0x1278, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, indigo_ccd_sx_hotplug_callback, NULL, NULL) == 0)
    return INDIGO_OK;
  return INDIGO_FAILED;
}

