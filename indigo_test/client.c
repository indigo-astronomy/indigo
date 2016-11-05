// Copyright (c) 2016 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>

#include "indigo_bus.h"
#include "indigo_client_xml.h"
#include "ccd_simulator/indigo_ccd_simulator.h"

static indigo_property *connection_property;
static indigo_property *ccd_exposure_property;
static indigo_property *ccd_image_property;

static int device_pid;

static indigo_result client_attach(indigo_client *client) {
	connection_property = indigo_init_switch_property(NULL, CCD_SIMULATOR_IMAGER_CAMERA_NAME, "CONNECTION", NULL, NULL, 0, 0, 0, 2);
	indigo_init_switch_item(&connection_property->items[0], "CONNECTED", NULL, false);
	indigo_init_switch_item(&connection_property->items[1], "DISCONNECTED", NULL, true);
	ccd_exposure_property = indigo_init_number_property(NULL, CCD_SIMULATOR_IMAGER_CAMERA_NAME, "CCD_EXPOSURE", NULL, NULL, 0, 0, 1);
	indigo_init_number_item(&ccd_exposure_property->items[0], "EXPOSURE", NULL, 0, 0, 0, 0);
	ccd_image_property = indigo_init_blob_property(NULL, CCD_SIMULATOR_IMAGER_CAMERA_NAME, "CCD_IMAGE", NULL, NULL, 0, 1);
	indigo_init_blob_item(&ccd_image_property->items[0], "IMAGE", NULL);
	indigo_log("attached to INDI bus...");
	return INDIGO_OK;
}

static indigo_result client_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (message)
		indigo_log("message: %s", message);
	if (indigo_property_match(connection_property, property)) {
		connection_property->items[0].sw.value = true;
		connection_property->items[1].sw.value = false;
		indigo_change_property(client, connection_property);
		return INDIGO_OK;
	}
	return INDIGO_OK;
}

static indigo_result client_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	if (message)
		indigo_log("message: %s", message);
	if (indigo_property_match(connection_property, property)) {
		indigo_property_copy_values(connection_property, property, true);
		if (connection_property->items[0].sw.value) {
			indigo_log("connected...");
			ccd_exposure_property->items[0].number.value = 3.0;
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
			indigo_log("exposure %gs...", ccd_exposure_property->items[0].number.value);
		} else if (ccd_exposure_property->state == INDIGO_OK_STATE) {
			indigo_log("exposure done...");
		}
		return INDIGO_OK;
	}
	if (indigo_property_match(ccd_image_property, property)) {
		indigo_property_copy_values(ccd_image_property, property, true);
		if (ccd_image_property->state == INDIGO_OK_STATE) {
			indigo_log("image received (%d bytes)...", ccd_image_property->items[0].blob.size);
			int handle = open("client.fits", O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (handle) {
				int res = write(handle, ccd_image_property->items[0].blob.value, ccd_image_property->items[0].blob.size);
				close(handle);
			}
			connection_property->items[0].sw.value = false;
			connection_property->items[1].sw.value = true;
			indigo_change_property(client, connection_property);
		}
		return INDIGO_OK;
	}
	return INDIGO_OK;
}


static indigo_result client_send_message(struct indigo_client *client, struct indigo_device *device, const char *message) {
	indigo_log("message: %s", message);
	return INDIGO_OK;
}

static indigo_result client_detach(indigo_client *client) {
	indigo_log("detached from INDI bus...");
	kill(device_pid, SIGKILL);
	exit(0);
	return INDIGO_OK;
}

static indigo_client client = {
	"Test", NULL, INDIGO_OK, INDIGO_VERSION, INDIGO_ENABLE_BLOB_ALSO,
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
		indigo_log("Can't create local pipe for device (%s)", strerror(errno));
		return 0;
	}
	device_pid = fork();
	if (device_pid == 0) {
		close(0);
		dup2(output[0], 0);
		close(1);
		dup2(input[1], 1);
		execl("./indigo_ccd_simulator", "indigo_ccd_simulator", NULL);
	} else {
		close(input[1]);
		close(output[0]);
		indigo_start();
		indigo_device *protocol_adapter = indigo_xml_client_adapter(input[0], output[1]);
		indigo_attach_device(protocol_adapter);
		indigo_attach_client(&client);
		indigo_xml_parse(input[0], protocol_adapter, &client);
		indigo_stop();
	}
	return 0;
}

