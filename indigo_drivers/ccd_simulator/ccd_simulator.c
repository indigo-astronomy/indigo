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

#include "ccd_simulator.h"
#include "indigo_timer.h"

typedef struct {
  char name[INDIGO_NAME_SIZE];
  int star_x[100], star_y[100], star_a[100];
} ccd_simulator_data;


static indigo_result ccd_simulator_attach(indigo_device *device) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  
  ccd_simulator_data *private_data = device->device_context;
  device->device_context = NULL;
  
  if (indigo_ccd_device_attach(device, private_data->name, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
    PRIVATE_DATA = private_data;
    // -------------------------------------------------------------------------------- SIMULATION
    SIMULATION_PROPERTY->perm = INDIGO_RO_PERM;
    SIMULATION_ENABLED_ITEM->switch_value = true;
    SIMULATION_DISABLED_ITEM->switch_value = false;
    // -------------------------------------------------------------------------------- CCD_INFO
    CCD_INFO_WIDTH_ITEM->number_value = 1600;
    CCD_INFO_HEIGHT_ITEM->number_value = 1200;
    CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number_value = 2;
    CCD_INFO_MAX_VERTICAL_BIN_ITEM->number_value = 2;
    CCD_INFO_PIXEL_SIZE_ITEM->number_value = 5.2;
    CCD_INFO_PIXEL_WIDTH_ITEM->number_value = 5.2;
    CCD_INFO_PIXEL_HEIGHT_ITEM->number_value = 5.2;
    CCD_INFO_BITS_PER_PIXEL_ITEM->number_value = 16;
    // -------------------------------------------------------------------------------- CCD_FRAME
    CCD_FRAME_WIDTH_ITEM->number_max = CCD_FRAME_WIDTH_ITEM->number_value = CCD_INFO_WIDTH_ITEM->number_value;
    CCD_FRAME_HEIGHT_ITEM->number_max = CCD_FRAME_HEIGHT_ITEM->number_value = CCD_INFO_HEIGHT_ITEM->number_value;
    // -------------------------------------------------------------------------------- CCD_IMAGE
    int width = CCD_INFO_WIDTH_ITEM->number_value;
    int height = CCD_INFO_HEIGHT_ITEM->number_value;
    int blob_size = FITS_HEADER_SIZE + 2 * width * height;
    char *blob = malloc(blob_size);
    if (blob == NULL)
      return INDIGO_FAILED;
    CCD_IMAGE_ITEM->blob_size = blob_size;
    strncpy(CCD_IMAGE_ITEM->blob_format, ".fits", INDIGO_NAME_SIZE);
    CCD_IMAGE_ITEM->blob_value = blob;
    for (int i = 0; i < 100; i++) {
      private_data->star_x[i] = rand() % (width - 20) + 10;
      private_data->star_y[i] = rand() % (height - 20) + 10;
      private_data->star_a[i] = 1000 * (rand() % 60);
    }
    return INDIGO_OK;
  }
  return INDIGO_FAILED;
}

static void exposure_timer_callback(indigo_device *device, int timer_id, void *data, double delay) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
    CCD_EXPOSURE_ITEM->number_value = 0;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure done");
    short *raw = (short *)(CCD_IMAGE_ITEM->blob_value+FITS_HEADER_SIZE);
    int width = CCD_FRAME_WIDTH_ITEM->number_value;
    int height = CCD_FRAME_HEIGHT_ITEM->number_value;
    int size = width * height;
    for (int i = 0; i < size; i++)
      raw[i] = (rand() & 0xFF); // noise
    ccd_simulator_data *private_data = PRIVATE_DATA;
    for (int i = 0; i < 100; i++) {
      double centerX = private_data->star_x[i]+rand()/(double)RAND_MAX/5-0.5;
      double centerY = private_data->star_y[i]+rand()/(double)RAND_MAX/5-0.5;
      int a = private_data->star_a[i];
      int xMax = (int)round(centerX)+4;
      int yMax = (int)round(centerY)+4;
      for (int y = yMax-8; y <= yMax; y++) {
        int yw = y*width;
        for (int x = xMax-8; x <= xMax; x++) {
          double xx = centerX-x;
          double yy = centerY-y;
          double v = a*exp(-(xx*xx/2.0+yy*yy/2.0));
          raw[yw+x] += (unsigned short)v;
        }
      }
    }
    indigo_convert_to_fits(device, delay);
    CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
  }
}

static indigo_result ccd_simulator_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
    CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
  } else if (indigo_property_match(CONFIG_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONFIG
    indigo_property_copy_values(CONFIG_PROPERTY, property, false);
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
    CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure initiated");
    CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
    indigo_set_timer(device, 2, NULL, CCD_EXPOSURE_ITEM->number_value, exposure_timer_callback);
  } else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
    if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      indigo_cancel_timer(device, 2);
    }
    indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
  }
  return indigo_ccd_device_change_property(device, client, property);
}

indigo_result ccd_simulator_detach(indigo_device *device) {
  assert(device != NULL);
  free(PRIVATE_DATA);
  return indigo_ccd_device_detach(device);
}

indigo_device *ccd_simulator(char *name) {
  static indigo_device device_template = {
    NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    ccd_simulator_attach,
    indigo_ccd_device_enumerate_properties,
    ccd_simulator_change_property,
    ccd_simulator_detach
  };
  indigo_device *device = malloc(sizeof(indigo_device));
  if (device != NULL) {
    memcpy(device, &device_template, sizeof(indigo_device));
    ccd_simulator_data *private_data = malloc(sizeof(ccd_simulator_data));
    strncpy(private_data->name, name, INDIGO_NAME_SIZE);
    device->device_context = private_data;
  }
  return device;
}
