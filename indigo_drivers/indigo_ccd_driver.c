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
  if (CCD_DRIVER_CONTEXT == NULL)
    driver->driver_context = malloc(sizeof(indigo_ccd_driver_context));
  if (CCD_DRIVER_CONTEXT != NULL) {
    if (indigo_driver_attach(driver, device, version, 2) == INDIGO_OK) {
      // -------------------------------------------------------------------------------- CCD_INFO
      CCD_INFO_PROPERTY = indigo_init_number_property(NULL, device, "CCD_INFO", "Main", "CCD Info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 4);
      if (CCD_INFO_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_INFO_WIDTH_ITEM, "CCD_MAX_X", "Horizontal resolution", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_HEIGHT_ITEM, "CCD_MAX_Y", "Vertical resolution", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_MAX_HORIZONTAL_BIN_ITEM, "CCD_MAX_BIN_X", "Max vertical binning", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_MAX_VERTICAL_BIN_ITEM, "CCD_MAX_BIN_Y", "Max horizontal binning", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_PIXEL_SIZE_ITEM, "CCD_PIXEL_SIZE", "Pixel size", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_PIXEL_WIDTH_ITEM, "CCD_PIXEL_SIZE_X", "Pixel width", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_PIXEL_HEIGHT_ITEM, "CCD_PIXEL_SIZE_Y", "Pixel height", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_BITS_PER_PIXEL_ITEM, "CCD_BITSPERPIXEL", "Bits/pixel", 0, 0, 0, 0);
      // -------------------------------------------------------------------------------- CCD_EXPOSURE
      CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device, "CCD_EXPOSURE", "Main", "Start exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
      if (CCD_EXPOSURE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_EXPOSURE_ITEM, "CCD_EXPOSURE_VALUE", "Start exposure", 0, 10000, 1, 0);
      // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
      CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device, "CCD_ABORT_EXPOSURE", "Main", "Abort exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
      if (CCD_ABORT_EXPOSURE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(CCD_ABORT_EXPOSURE_ITEM, "ABORT", "Abort exposure", false);
      // -------------------------------------------------------------------------------- CCD_FRAME
      CCD_FRAME_PROPERTY = indigo_init_number_property(NULL, device, "CCD_FRAME", "Main", "Frame size", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
      if (CCD_FRAME_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_FRAME_LEFT_ITEM, "X", "Left", 0, 0, 1, 0);
      indigo_init_number_item(CCD_FRAME_TOP_ITEM, "Y", "Top", 0, 0, 1, 0);
      indigo_init_number_item(CCD_FRAME_WIDTH_ITEM, "WIDTH", "Width", 0, 0, 1, 0);
      indigo_init_number_item(CCD_FRAME_HEIGHT_ITEM, "HEIGHT", "Height", 0, 0, 1, 0);
      // -------------------------------------------------------------------------------- CCD_BIN
      CCD_BIN_PROPERTY = indigo_init_number_property(NULL, device, "CCD_BINNING", "Main", "Binning", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 2);
      if (CCD_BIN_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_BIN_HORIZONTAL_ITEM, "HOR_BIN", "Horizontal binning", 0, 1, 1, 1);
      indigo_init_number_item(CCD_BIN_VERTICAL_ITEM, "VER_BIN", "Vertical binning", 0, 1, 1, 1);
      // -------------------------------------------------------------------------------- CCD_FRAME_TYPE
      CCD_FRAME_TYPE_PROPERTY = indigo_init_switch_property(NULL, device, "CCD_FRAME_TYPE", "Main", "Frame type", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
      if (CCD_FRAME_TYPE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(CCD_FRAME_TYPE_LIGHT_ITEM, "FRAME_LIGHT", "Light frame exposure", true);
      indigo_init_switch_item(CCD_FRAME_TYPE_BIAS_ITEM, "FRAME_BIAS", "Bias frame exposure", false);
      indigo_init_switch_item(CCD_FRAME_TYPE_DARK_ITEM, "FRAME_DARK", "Dark frame exposure", false);
      indigo_init_switch_item(CCD_FRAME_TYPE_FLAT_ITEM, "FRAME_FLAT", "Flat field frame exposure", false);
      // -------------------------------------------------------------------------------- CCD_IMAGE
      CCD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device, "CCD1", "Image", "Image", INDIGO_IDLE_STATE, 1);
      if (CCD_IMAGE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_blob_item(CCD_IMAGE_ITEM, "CCD1", "Primary CCD image");
      return INDIGO_OK;
    }
  }
  return INDIGO_FAILED;
}

indigo_result indigo_ccd_driver_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(driver->driver_context != NULL);
  assert(property != NULL);
  indigo_result result = INDIGO_OK;
  if ((result = indigo_driver_enumerate_properties(driver, client, property)) == INDIGO_OK) {
    if (indigo_is_connected(driver_context)) {
      if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property))
        indigo_define_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property))
        indigo_define_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
      if (indigo_property_match(CCD_FRAME_PROPERTY, property))
        indigo_define_property(driver, CCD_FRAME_PROPERTY, NULL);
      if (indigo_property_match(CCD_BIN_PROPERTY, property))
        indigo_define_property(driver, CCD_BIN_PROPERTY, NULL);
      if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property))
        indigo_define_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
      if (indigo_property_match(CCD_IMAGE_PROPERTY, property))
        indigo_define_property(driver, CCD_IMAGE_PROPERTY, NULL);
    }
  }
  return result;
}

static void countdown_timer_callback(indigo_driver *driver, int timer_id, void *data) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_ITEM->number_value -= 1;
    if (CCD_EXPOSURE_ITEM->number_value >= 1) {
      indigo_update_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      indigo_set_timer(driver, 1, NULL, 1.0, countdown_timer_callback);
    }
  }
}

indigo_result indigo_ccd_driver_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(driver->driver_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    if (CONNECTION_CONNECTED_ITEM->switch_value) {
      indigo_define_property(driver, CCD_INFO_PROPERTY, NULL);
      indigo_define_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_FRAME_PROPERTY, NULL);
      indigo_define_property(driver, CCD_BIN_PROPERTY, NULL);
      indigo_define_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_IMAGE_PROPERTY, NULL);
    } else {
      indigo_delete_property(driver, CCD_INFO_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_FRAME_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_BIN_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_IMAGE_PROPERTY, NULL);
    }
  } else if (indigo_property_match(CONFIGURATION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONFIG_PROCESS
    if (CONFIGURATION_SAVE_ITEM->switch_value) {
      if (CCD_FRAME_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_FRAME_PROPERTY);
      if (CCD_BIN_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_BIN_PROPERTY);
      if (CCD_FRAME_TYPE_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_FRAME_TYPE_PROPERTY);
    }
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      if (CCD_EXPOSURE_ITEM->number_value >= 1) {
        indigo_set_timer(driver, 1, NULL, 1.0, countdown_timer_callback);
      }
    }
  } else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      indigo_cancel_timer(driver, 1);
      CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
      CCD_EXPOSURE_ITEM->number_value = 0;
      indigo_update_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
      indigo_update_property(driver, CCD_IMAGE_PROPERTY, NULL);
      CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
    } else {
      CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
    }
    CCD_ABORT_EXPOSURE_ITEM->switch_value = false;
    indigo_update_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_OK_STATE ? "Exposure canceled" : "Failed to cancel exposure");
  } else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_FRAME
    indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
    CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(driver, CCD_FRAME_PROPERTY, NULL);
  } else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_BIN
    indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
    CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(driver, CCD_BIN_PROPERTY, NULL);
  } else if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_FRAME_TYPE
    indigo_property_copy_values(CCD_FRAME_TYPE_PROPERTY, property, false);
    CCD_FRAME_TYPE_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
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
