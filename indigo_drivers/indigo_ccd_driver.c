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
#include <assert.h>

#include "indigo_ccd_driver.h"
#include "indigo_timer.h"

indigo_result indigo_ccd_driver_attach(indigo_driver *driver, char *device, int version) {
  assert(driver != NULL);
  assert(device != NULL);
  if (driver->driver_context == NULL)
    driver->driver_context = malloc(sizeof(indigo_ccd_driver_context));
  indigo_ccd_driver_context *driver_context = driver->driver_context;
  if (driver_context != NULL) {
    if (indigo_driver_attach(driver, device, version, 2) == INDIGO_OK) {
      // CCD_EXPOSURE
      indigo_property *exposure_property = indigo_init_number_property(NULL, device, "CCD_EXPOSURE", "Main", "Start expose", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
      if (exposure_property == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(&exposure_property->items[0], "CCD_EXPOSURE_VALUE", "Start exposure (seconds)", 0, 10000, 1, 0);
      driver_context->ccd_exposure_property = exposure_property;
      // CCD_ABORT_EXPOSURE
      indigo_property *ccd_abort_exposure_property = indigo_init_switch_property(NULL, device, "CCD_ABORT_EXPOSURE", "Main", "Abort exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
      if (ccd_abort_exposure_property == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(&ccd_abort_exposure_property->items[0], "ABORT", "Abort exposure", false);
      driver_context->ccd_abort_exposure_property = ccd_abort_exposure_property;
      // CCD_FRAME
      indigo_property *ccd_frame_property = indigo_init_number_property(NULL, device, "CCD_FRAME", "Main", "Frame size", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
      if (ccd_frame_property == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(&ccd_frame_property->items[0], "X", "Left-most pixel position", 0, 0, 1, 0);
      indigo_init_number_item(&ccd_frame_property->items[1], "Y", "Top-most pixel position", 0, 0, 1, 0);
      indigo_init_number_item(&ccd_frame_property->items[2], "WIDTH", "Frame width in pixels", 0, 0, 1, 0);
      indigo_init_number_item(&ccd_frame_property->items[3], "HEIGHT", "Frame height in pixels", 0, 0, 1, 0);
      driver_context->ccd_frame_property = ccd_frame_property;
      // CCD_BINNING
      indigo_property *binning_property = indigo_init_number_property(NULL, device, "CCD_BINNING", "Main", "Binning", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
      if (binning_property == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(&binning_property->items[0], "HOR_BIN", "Horizontal binning", 0, 1, 1, 1);
      indigo_init_number_item(&binning_property->items[1], "VER_BIN", "Vertical binning", 0, 1, 1, 1);
      driver_context->ccd_binning_property = binning_property;
      // CCD_FRAME_TYPE
      indigo_property *ccd_frame_type_property = indigo_init_switch_property(NULL, device, "CCD_FRAME_TYPE", "Main", "Frame type", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
      if (ccd_frame_type_property == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(&ccd_frame_type_property->items[0], "FRAME_LIGHT", "Light frame exposure", true);
      indigo_init_switch_item(&ccd_frame_type_property->items[1], "FRAME_BIAS", "Bias frame exposure", false);
      indigo_init_switch_item(&ccd_frame_type_property->items[2], "FRAME_DARK", "Dark frame exposure", false);
      indigo_init_switch_item(&ccd_frame_type_property->items[3], "FRAME_FLAT", "Flat field frame exposure", false);
      driver_context->ccd_frame_type_property = ccd_frame_type_property;
      // CCD1
      indigo_property *ccd1_property = indigo_init_blob_property(NULL, device, "CCD1", "Image", "Image", INDIGO_IDLE_STATE, 1);
      if (ccd1_property == NULL)
        return INDIGO_FAILED;
      indigo_init_blob_item(&ccd1_property->items[0], "CCD1", "Primary CCD image");
      driver_context->ccd1_property = ccd1_property;
      driver->driver_context = driver_context;
      return INDIGO_OK;
    }
  }
  return INDIGO_FAILED;
}

indigo_result indigo_ccd_driver_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  indigo_ccd_driver_context *driver_context = (indigo_ccd_driver_context *)driver->driver_context;
  assert(driver_context != NULL);
  indigo_result result = INDIGO_OK;
  if ((result = indigo_driver_enumerate_properties(driver, client, property)) == INDIGO_OK) {
    if (indigo_is_connected(driver_context)) {
      if (indigo_property_match(driver_context->ccd_exposure_property, property))
        indigo_define_property(driver, driver_context->ccd_exposure_property, NULL);
      if (indigo_property_match(driver_context->ccd_abort_exposure_property, property))
        indigo_define_property(driver, driver_context->ccd_abort_exposure_property, NULL);
      if (indigo_property_match(driver_context->ccd_frame_property, property))
        indigo_define_property(driver, driver_context->ccd_frame_property, NULL);
      if (indigo_property_match(driver_context->ccd_binning_property, property))
        indigo_define_property(driver, driver_context->ccd_binning_property, NULL);
      if (indigo_property_match(driver_context->ccd_frame_type_property, property))
        indigo_define_property(driver, driver_context->ccd_frame_type_property, NULL);
      if (indigo_property_match(driver_context->ccd1_property, property))
        indigo_define_property(driver, driver_context->ccd1_property, NULL);
    }
  }
  return result;
}

static void countdown_timer_callback(indigo_driver *driver, int timer_id, void *data) {
  indigo_ccd_driver_context *driver_context = (indigo_ccd_driver_context *)driver->driver_context;
  if (driver_context->ccd_exposure_property->state == INDIGO_BUSY_STATE) {
    driver_context->ccd_exposure_property->items[0].number_value -= 1;
    if (driver_context->ccd_exposure_property->items[0].number_value >= 1) {
      indigo_update_property(driver, driver_context->ccd_exposure_property, NULL);
      indigo_set_timer(driver, 1, NULL, 1.0, countdown_timer_callback);
    }
  }
}

indigo_result indigo_ccd_driver_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  indigo_ccd_driver_context *driver_context = (indigo_ccd_driver_context *)driver->driver_context;
  assert(driver_context != NULL);
  if (indigo_property_match(driver_context->connection_property, property)) {
    // CONNECTION
    if (driver_context->connection_property->items[0].switch_value) {
      indigo_define_property(driver, driver_context->ccd_exposure_property, NULL);
      indigo_define_property(driver, driver_context->ccd_abort_exposure_property, NULL);
      indigo_define_property(driver, driver_context->ccd_frame_property, NULL);
      indigo_define_property(driver, driver_context->ccd_binning_property, NULL);
      indigo_define_property(driver, driver_context->ccd_frame_type_property, NULL);
      indigo_define_property(driver, driver_context->ccd1_property, NULL);
    } else {
      indigo_delete_property(driver, driver_context->ccd_exposure_property, NULL);
      indigo_delete_property(driver, driver_context->ccd_abort_exposure_property, NULL);
      indigo_delete_property(driver, driver_context->ccd_frame_property, NULL);
      indigo_delete_property(driver, driver_context->ccd_binning_property, NULL);
      indigo_delete_property(driver, driver_context->ccd_frame_type_property, NULL);
      indigo_delete_property(driver, driver_context->ccd1_property, NULL);
    }
  } else if (indigo_property_match(driver_context->congfiguration_property, property)) {
    // CONFIG_PROCESS
    if (driver_context->congfiguration_property->items[1].switch_value) {
      if (driver_context->ccd_frame_property->perm == INDIGO_RW_PERM)
        indigo_save_property(driver_context->ccd_frame_property);
      if (driver_context->ccd_binning_property->perm == INDIGO_RW_PERM)
        indigo_save_property(driver_context->ccd_binning_property);
      if (driver_context->ccd_frame_type_property->perm == INDIGO_RW_PERM)
        indigo_save_property(driver_context->ccd_frame_type_property);
    }
  } else if (indigo_property_match(driver_context->ccd_exposure_property, property)) {
    // CCD_EXPOSURE
    if (driver_context->ccd_exposure_property->state == INDIGO_BUSY_STATE) {
      if (driver_context->ccd_exposure_property->items[0].number_value >= 1) {
        indigo_set_timer(driver, 1, NULL, 1.0, countdown_timer_callback);
      }
    }
  } else if (indigo_property_match(driver_context->ccd_abort_exposure_property, property)) {
    // CCD_ABORT_EXPOSURE
    if (driver_context->ccd_exposure_property->state == INDIGO_BUSY_STATE) {
      indigo_cancel_timer(driver, 1);
      driver_context->ccd_exposure_property->state = INDIGO_ALERT_STATE;
      driver_context->ccd_exposure_property->items[0].number_value = 0;
      indigo_update_property(driver, driver_context->ccd_exposure_property, NULL);
      driver_context->ccd1_property->state = INDIGO_ALERT_STATE;
      indigo_update_property(driver, driver_context->ccd1_property, NULL);
      driver_context->ccd_abort_exposure_property->state = INDIGO_OK_STATE;
    } else {
      driver_context->ccd_abort_exposure_property->state = INDIGO_ALERT_STATE;
    }
    driver_context->ccd_abort_exposure_property->items[0].switch_value = false;
    indigo_update_property(driver, driver_context->ccd_abort_exposure_property, driver_context->ccd_abort_exposure_property->state == INDIGO_OK_STATE ? "Exposure canceled" : "Failed to cancel exposure");
  } else if (indigo_property_match(driver_context->ccd_frame_property, property)) {
    // CCD_CCD_FRAME
    indigo_property_copy_values(driver_context->ccd_frame_property, property, false);
    driver_context->ccd_frame_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->ccd_frame_property, NULL);
  } else if (indigo_property_match(driver_context->ccd_binning_property, property)) {
    // CCD_BINNING
    indigo_property_copy_values(driver_context->ccd_binning_property, property, false);
    driver_context->ccd_binning_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->ccd_binning_property, NULL);
  } else if (indigo_property_match(driver_context->ccd_frame_type_property, property)) {
    // CCD_CCD_FRAME_TYPE
    indigo_property_copy_values(driver_context->ccd_frame_type_property, property, false);
    driver_context->ccd_frame_type_property->state = INDIGO_OK_STATE;
    indigo_update_property(driver, driver_context->ccd_frame_type_property, NULL);
  }
  return indigo_driver_change_property(driver, client, property);
}

indigo_result indigo_ccd_driver_detach(indigo_driver *driver) {
  assert(driver != NULL);
  return INDIGO_OK;
}

void *indigo_make_fits(int naxis, int naxis1, int naxis2, int naxis3, int bitpix, indigo_fits_header* headers, void *data) {
  return NULL;
}
