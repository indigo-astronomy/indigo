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

#include "indigo_driver.h"

indigo_result indigo_init_driver(indigo_driver *driver, char *device, int version, int interface) {
  assert(driver != NULL);
  assert(device != NULL);
  if (driver->driver_context == NULL)
    driver->driver_context = malloc(sizeof(indigo_driver_context));
  indigo_driver_context *driver_context = driver->driver_context;
  if (driver_context != NULL) {
    indigo_property *connection_property = indigo_init_switch_property(NULL, device, "CONNECTION", "Main", "Connection", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
    if (connection_property == NULL)
      return INDIGO_INIT_FAILED;
    indigo_init_switch_item(&connection_property->items[0], "CONNECTED", "Connected", false);
    indigo_init_switch_item(&connection_property->items[1], "DISCONNECTED", "Disconnected", true);
    driver_context->connection_property = connection_property;
    indigo_property *info_property = indigo_init_text_property(NULL, device, "DRIVER_INFO", "Driver Info", "Options", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 5);
    if (info_property == NULL)
      return INDIGO_INIT_FAILED;
    indigo_init_text_item(&info_property->items[0], "DRIVER_NAME", "Name", device);
    indigo_init_text_item(&info_property->items[1], "DRIVER_VERSION", "Version", "%d.%d", (version >> 8) & 0xFF, version & 0xFF);
    indigo_init_text_item(&info_property->items[2], "DRIVER_INTERFACE", "Interface", "%d", interface);
    indigo_init_text_item(&info_property->items[3], "FRAMEWORK_NAME", "Framework name", "INDIGO PoC");
    indigo_init_text_item(&info_property->items[4], "FRAMEWORK_VERSION", "Framework version", "%d.%d build %d", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
    driver_context->info_property = info_property;
    driver->driver_context = driver_context;
    return INDIGO_OK;
  }
  return INDIGO_INIT_FAILED;
}

indigo_result indigo_enumerate_driver_properties(indigo_driver *driver, indigo_property *property) {
  assert(driver != NULL);
  assert(property != NULL);
  indigo_driver_context *driver_context = driver->driver_context;
  assert(driver_context != NULL);
  if (indigo_property_match(driver_context->connection_property, property))
    indigo_define_property(driver, driver_context->connection_property);
  if (indigo_property_match(driver_context->info_property, property))
    indigo_define_property(driver, driver_context->info_property);
  return INDIGO_OK;
}

