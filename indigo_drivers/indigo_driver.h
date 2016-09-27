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

#ifndef indigo_driver_h
#define indigo_driver_h

#include "indigo_bus.h"

typedef struct {
  void *private_data;
  indigo_property *connection_property;
  indigo_property *info_property;
  indigo_property *debug_property;
  indigo_property *simulation_property;
  indigo_property *congfiguration_property;
} indigo_driver_context;

extern indigo_result indigo_driver_attach(indigo_driver *driver, char *device, int version, int interface);
extern indigo_result indigo_driver_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property);
extern indigo_result indigo_driver_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property);
extern indigo_result indigo_driver_detach(indigo_driver *driver);

#define indigo_is_connected(driver_context) driver_context->connection_property->items[0].switch_value
#define indigo_debug_enabled(driver_context) driver_context->debug_property->items[0].switch_value
#define indigo_simulation_enabled(driver_context) driver_context->simulation_property->items[0].switch_value

extern void indigo_save_property(indigo_property *property);
extern indigo_result indigo_save_properties(indigo_driver *driver);

#endif /* indigo_driver_h */
