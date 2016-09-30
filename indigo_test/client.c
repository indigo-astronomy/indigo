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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "indigo_bus.h"
#include "indigo_client_xml.h"

static indigo_property *connection_property;
static indigo_property *ccd_exposure_property;
static indigo_property *ccd_image_property;

static int driver_pid;

static indigo_result client_attach(indigo_client *client) {
  connection_property = indigo_init_switch_property(NULL, "CCD Simulator", "CONNECTION", NULL, NULL, 0, 0, 0, 2);
  indigo_init_switch_item(&connection_property->items[0], "CONNECTED", NULL, false);
  indigo_init_switch_item(&connection_property->items[1], "DISCONNECTED", NULL, true);
  ccd_exposure_property = indigo_init_number_property(NULL, "CCD Simulator", "CCD_EXPOSURE", NULL, NULL, 0, 0, 1);
  indigo_init_number_item(&ccd_exposure_property->items[0], "EXPOSURE", NULL, 0, 0, 0, 0);
  ccd_image_property = indigo_init_blob_property(NULL, "CCD Simulator", "CCD_IMAGE", NULL, NULL, 0, 1);
  indigo_init_blob_item(&ccd_image_property->items[0], "IMAGE", NULL);
  indigo_log("connected to INDI bus...");
  return INDIGO_OK;
}

static indigo_result client_define_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property, const char *message) {
  if (message)
    indigo_log("message: %s", message);
  if (indigo_property_match(connection_property, property)) {
    connection_property->items[0].switch_value = true;
    connection_property->items[1].switch_value = false;
    indigo_change_property(client, connection_property);
    return INDIGO_OK;
  }
  return INDIGO_OK;
}

static indigo_result client_update_property(struct indigo_client *client, struct indigo_driver *driver, indigo_property *property, const char *message) {
  if (message)
    indigo_log("message: %s", message);
  if (indigo_property_match(connection_property, property)) {
    indigo_property_copy_values(connection_property, property, true);
    if (connection_property->items[0].switch_value) {
      indigo_log("connected...");
      ccd_exposure_property->items[0].number_value = 3.0;
      indigo_change_property(client, ccd_exposure_property);
    } else {
      indigo_log("disconnected...");
      indigo_stop();
    }
    return INDIGO_OK;
  }
  if (indigo_property_match(ccd_exposure_property, property)) {
    indigo_property_copy_values(ccd_exposure_property, property, true);
    if (ccd_exposure_property->state == INDIGO_BUSY_STATE) {
      indigo_log("exposure %gs...", ccd_exposure_property->items[0].number_value);
    } else if (ccd_exposure_property->state == INDIGO_OK_STATE) {
      indigo_log("exposure done...");
    }
    return INDIGO_OK;
  }
  if (indigo_property_match(ccd_image_property, property)) {
    indigo_property_copy_values(ccd_image_property, property, true);
    if (ccd_image_property->state == INDIGO_OK_STATE) {
      indigo_log("image received (%d bytes)...", ccd_image_property->items[0].blob_size);
      connection_property->items[0].switch_value = false;
      connection_property->items[1].switch_value = true;
      indigo_change_property(client, connection_property);
    }
    return INDIGO_OK;
  }
  return INDIGO_OK;
}


static indigo_result client_send_message(struct indigo_client *client, struct indigo_driver *driver, const char *message) {
  indigo_log("message: %s", message);
  return INDIGO_OK;
}

static indigo_result client_detach(indigo_client *client) {
  indigo_log("disconnected from INDI bus...");
  kill(driver_pid, SIGKILL);
  exit(0);
  return INDIGO_OK;
}

static indigo_client client = {
  NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
  client_attach,
  client_define_property,
  client_update_property,
  NULL,
  client_send_message,
  client_detach
};

int main(int argc, const char * argv[]) {
  indigo_main_argc = argc;
  indigo_main_argv = argv;
  int input[2], output[2];
  if (pipe(input) < 0 || pipe(output) < 0) {
    indigo_log("Can't create local pipe for driver (%s)", strerror(errno));
    return 0;
  }
  driver_pid = fork();
  if (driver_pid == 0) {
    close(0);
    dup2(output[0], 0);
    close(1);
    dup2(input[1], 1);
    execl("./driver", "driver", NULL);
  } else {
    close(input[1]);
    close(output[0]);
    indigo_start();
    indigo_driver *protocol_adapter = xml_client_adapter(input[0], output[1]);
    indigo_attach_driver(protocol_adapter);
    indigo_attach_client(&client);
    indigo_xml_parse(input[0], protocol_adapter, NULL);
    indigo_stop();
  }
  return 0;
}
