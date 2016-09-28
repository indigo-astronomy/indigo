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
#include <assert.h>

#include "ccd_simulator.h"

#define DEVICE "CCD Simulator"
#define VERSION INDIGO_VERSION_CURRENT

#define CCD_MAX_X         1600
#define CCD_MAX_Y         1200
#define CCD_PIXEL_SIZE    5
#define CCD_BITSPERPIXEL  16

static indigo_result ccd_simulator_attach(indigo_driver *driver) {
  assert(driver != NULL);
  if (indigo_ccd_driver_attach(driver, DEVICE, VERSION) == INDIGO_OK) {
    indigo_ccd_driver_context *driver_context = driver->driver_context;
    // SIMULATION
    driver_context->simulation_property->perm = INDIGO_RO_PERM;
    driver_context->simulation_property->items[0].switch_value = true;
    driver_context->simulation_property->items[1].switch_value = false;
    // CCD_FRAME
    driver_context->ccd_frame_property->items[2].number_max = driver_context->ccd_frame_property->items[2].number_value = CCD_MAX_X;
    driver_context->ccd_frame_property->items[3].number_max = driver_context->ccd_frame_property->items[3].number_value = CCD_MAX_Y;
    // CCD1
    int size = 2*4096*4096;
    char *blob = malloc(size);
    if (blob == NULL)
      return INDIGO_FAILED;
    for (int i = 0; i < size; i++)
      blob[i] = rand();
    driver_context->ccd1_property->items[0].blob_size = size;
    driver_context->ccd1_property->items[0].blob_value = blob;
    strncpy(driver_context->ccd1_property->items[0].blob_format, ".bin",INDIGO_NAME_SIZE);
    return INDIGO_OK;
  }
  return INDIGO_FAILED;
}

static indigo_result ccd_simulator_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(client != NULL);
  assert(property != NULL);
  indigo_ccd_driver_context *driver_context = (indigo_ccd_driver_context *)driver->driver_context;
  assert(driver_context != NULL);
  if (indigo_property_match(driver_context->connection_property, property)) {
    // CONNECTION
    indigo_property_copy_values(driver_context->connection_property, property);
  } else if (indigo_property_match(driver_context->congfiguration_property, property)) {
    // CONFIG_PROCESS
    indigo_property_copy_values(driver_context->congfiguration_property, property);
  } else if (indigo_property_match(driver_context->ccd_exposure_property, property)) {
    // CCD_EXPOSURE
    indigo_property_copy_values(driver_context->ccd_exposure_property, property);
    driver_context->ccd1_property->state = INDIGO_BUSY_STATE;
    indigo_update_property(driver, driver_context->ccd1_property);
    while (driver_context->ccd_exposure_property->items[0].number_value > 0) {
      driver_context->ccd_exposure_property->state = INDIGO_BUSY_STATE;
      indigo_update_property(driver, driver_context->ccd_exposure_property);
      sleep(1);
      driver_context->ccd_exposure_property->items[0].number_value -= 1;
    }
    driver_context->ccd_exposure_property->items[0].number_value = 0;
    driver_context->ccd_exposure_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->ccd_exposure_property);
    driver_context->ccd1_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->ccd1_property);
  }
  return indigo_ccd_driver_change_property(driver, client, property);
}

indigo_driver *ccd_simulator() {
  static indigo_driver driver_template = {
    NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    ccd_simulator_attach,
    indigo_ccd_driver_enumerate_properties,
    ccd_simulator_change_property,
    indigo_ccd_driver_detach
  };
  indigo_driver *driver = malloc(sizeof(indigo_driver));
  if (driver != NULL) {
    memcpy(driver, &driver_template, sizeof(indigo_driver));
  }
  return driver;
}
