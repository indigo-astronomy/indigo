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

#include "ccd_simulator.h"

#define DEVICE "CCD Simulator"

static indigo_property *connection_property;
static indigo_property *exposure_property;
static indigo_property *ccd1_property;

static indigo_result ccd_simulator_init(indigo_driver *driver) {
  connection_property = indigo_allocate_switch_property(DEVICE, "CONNECTION", "Main", "Connection", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
  indigo_init_switch_item(&connection_property->items[0], "CONNECTED", "Connected", false);
  indigo_init_switch_item(&connection_property->items[1], "DISCONNECTED", "Disconnected", true);
  exposure_property = indigo_allocate_number_property(DEVICE, "CCD_EXPOSURE", "Main", "Expose", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
  indigo_init_number_item(&exposure_property->items[0], "CCD_EXPOSURE_VALUE", "Expose the CCD chip (seconds)", 0, 900, 1, 1);
  ccd1_property = indigo_allocate_blob_property(DEVICE, "CCD1", "Main", "Image", INDIGO_IDLE_STATE, 1);
  indigo_init_blob_item(&ccd1_property->items[0], "CCD1", "Primary CCD image");
  
  int size = 2*64*64; //2*4096*4096;
  char *blob = malloc(size);
  for (int i = 0; i < size; i++)
    blob[i] = rand();
  ccd1_property->items[0].blob_size = size;
  ccd1_property->items[0].blob_value = blob;
  
  strncpy(ccd1_property->items[0].blob_format, ".bin", NAME_SIZE);
  return INDIGO_OK;
}

static indigo_result ccd_simulator_start(indigo_driver *driver) {
  return INDIGO_OK;
}

static indigo_result ccd_simulator_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  if (*property->device == 0 || !strcmp(property->device, DEVICE)) {
    if (*property->name == 0 || !strcmp(property->name, connection_property->name))
      indigo_define_property(driver, connection_property);
    if (connection_property->items[0].switch_value) {
      if (*property->name == 0 || !strcmp(property->name, exposure_property->name))
        indigo_define_property(driver, exposure_property);
      if (*property->name == 0 || !strcmp(property->name, ccd1_property->name))
        indigo_define_property(driver, ccd1_property);
    }
  }
  return INDIGO_OK;
}

static indigo_result ccd_simulator_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  if (indigo_property_match(connection_property, property)) {
    connection_property->items[0].switch_value = false;
    connection_property->items[1].switch_value = false;
    indigo_property_copy_values(connection_property, property);
    if (connection_property->items[0].switch_value) {
      indigo_define_property(driver, exposure_property);
      indigo_define_property(driver, ccd1_property);
    } else {
      indigo_delete_property(driver, exposure_property);
      indigo_delete_property(driver, ccd1_property);
    }
    connection_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, connection_property);
    return INDIGO_OK;
  }
  if (indigo_property_match(exposure_property, property)) {
    indigo_property_copy_values(exposure_property, property);
    ccd1_property->state = INDIGO_BUSY_STATE;
    indigo_update_property(driver, ccd1_property);
    while (exposure_property->items[0].number_value > 0) {
      exposure_property->state = INDIGO_BUSY_STATE;
      indigo_update_property(driver, exposure_property);
      sleep(1);
      exposure_property->items[0].number_value -= 1;
    }
    exposure_property->items[0].number_value = 0;
    exposure_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, exposure_property);
    ccd1_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, ccd1_property);
    return INDIGO_OK;
  }
  return INDIGO_OK;
}

static indigo_result ccd_simulator_stop(indigo_driver *driver) {
  return INDIGO_OK;
}

indigo_driver *ccd_simulator() {
  static indigo_driver ccd_simulator = {
    0, 0,
    ccd_simulator_init,
    ccd_simulator_start,
    ccd_simulator_enumerate_properties,
    ccd_simulator_change_property,
    ccd_simulator_stop
  };
  return &ccd_simulator;
}
