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

static indigo_result ccd_simulator_connect(indigo_driver *driver) {
  assert(driver != NULL);
  if (indigo_init_ccd_driver(driver, DEVICE) == INDIGO_OK) {
    indigo_ccd_driver_context *driver_context = driver->driver_context;
    int size = 2*4096*4096;
    char *blob = malloc(size);
    if (blob == NULL)
      return INDIGO_INIT_FAILED;
    for (int i = 0; i < size; i++)
      blob[i] = rand();
    driver_context->ccd1_property->items[0].blob_size = size;
    driver_context->ccd1_property->items[0].blob_value = blob;
    strncpy(driver_context->ccd1_property->items[0].blob_format, ".bin",INDIGO_NAME_SIZE);
    return INDIGO_OK;
  }
  return INDIGO_INIT_FAILED;
}

static indigo_result ccd_simulator_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(client != NULL);
  assert(property != NULL);
  return indigo_enumerate_ccd_driver_properties(driver, property);
}

static indigo_result ccd_simulator_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(client != NULL);
  assert(property != NULL);
  indigo_ccd_driver_context *driver_context = (indigo_ccd_driver_context *)driver->driver_context;
  assert(driver_context != NULL);
  if (indigo_property_match(driver_context->connection_property, property)) {
    driver_context->connection_property->items[0].switch_value = false;
    driver_context->connection_property->items[1].switch_value = false;
    indigo_property_copy_values(driver_context->connection_property, property);
    if (driver_context->connection_property->items[0].switch_value) {
      indigo_define_property(driver, driver_context->exposure_property);
      indigo_define_property(driver, driver_context->ccd1_property);
    } else {
      indigo_delete_property(driver, driver_context->exposure_property);
      indigo_delete_property(driver, driver_context->ccd1_property);
    }
    driver_context->connection_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->connection_property);
    return INDIGO_OK;
  }
  if (indigo_property_match(driver_context->exposure_property, property)) {
    indigo_property_copy_values(driver_context->exposure_property, property);
    driver_context->ccd1_property->state = INDIGO_BUSY_STATE;
    indigo_update_property(driver, driver_context->ccd1_property);
    while (driver_context->exposure_property->items[0].number_value > 0) {
      driver_context->exposure_property->state = INDIGO_BUSY_STATE;
      indigo_update_property(driver, driver_context->exposure_property);
      sleep(1);
      driver_context->exposure_property->items[0].number_value -= 1;
    }
    driver_context->exposure_property->items[0].number_value = 0;
    driver_context->exposure_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->exposure_property);
    driver_context->ccd1_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->ccd1_property);
    return INDIGO_OK;
  }
  return INDIGO_OK;
}

static indigo_result ccd_simulator_disconnect(indigo_driver *driver) {
  assert(driver != NULL);
  return INDIGO_OK;
}

indigo_driver *ccd_simulator() {
  static indigo_driver driver_template = {
    NULL, INDIGO_OK,
    ccd_simulator_connect,
    ccd_simulator_enumerate_properties,
    ccd_simulator_change_property,
    ccd_simulator_disconnect
  };
  indigo_driver *driver = malloc(sizeof(indigo_driver));
  if (driver != NULL) {
    memcpy(driver, &driver_template, sizeof(indigo_driver));
  }
  return driver;
}
