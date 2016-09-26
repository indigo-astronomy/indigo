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

indigo_result indigo_init_ccd_driver(indigo_driver *driver, char *device) {
  assert(driver != NULL);
  assert(device != NULL);
  if (driver->driver_context == NULL)
    driver->driver_context = malloc(sizeof(indigo_ccd_driver_context));
  indigo_ccd_driver_context *driver_context = driver->driver_context;
  if (driver_context != NULL) {
    if (indigo_init_driver(driver, device) == INDIGO_OK) {
      indigo_property *exposure_property = indigo_init_number_property(NULL, device, "CCD_EXPOSURE", "Main", "Expose", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
      if (exposure_property == NULL)
        return INDIGO_INIT_FAILED;
      indigo_init_number_item(&exposure_property->items[0], "CCD_EXPOSURE_VALUE", "Expose the CCD chip (seconds)", 0, 900, 1, 1);
      driver_context->exposure_property = exposure_property;
      indigo_property *ccd1_property = indigo_init_blob_property(NULL, device, "CCD1", "Main", "Image", INDIGO_IDLE_STATE, 1);
      if (ccd1_property == NULL)
        return INDIGO_INIT_FAILED;
      indigo_init_blob_item(&ccd1_property->items[0], "CCD1", "Primary CCD image");
      driver_context->ccd1_property = ccd1_property;
      driver->driver_context = driver_context;
    }
    return INDIGO_OK;
  }
  return INDIGO_INIT_FAILED;
}

indigo_result indigo_enumerate_ccd_driver_properties(indigo_driver *driver, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  indigo_ccd_driver_context *driver_context = (indigo_ccd_driver_context *)driver->driver_context;
  assert(driver_context != NULL);
  indigo_result result = INDIGO_OK;
  if ((result = indigo_enumerate_driver_properties(driver, property)) == INDIGO_OK) {
    if (indigo_is_connected(driver_context)) {
      if (indigo_property_match(driver_context->exposure_property, property))
        indigo_define_property(driver, driver_context->exposure_property);
      if (indigo_property_match(driver_context->ccd1_property, property))
        indigo_define_property(driver, driver_context->ccd1_property);
    }
  }
  return result;
}
