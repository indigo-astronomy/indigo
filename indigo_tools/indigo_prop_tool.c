// Copyright (c) 2017 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen Bogdanovski <rumen@skyarchive.org>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>

#include "indigo_bus.h"
#include "indigo_client.h"

static bool connected = false;

static indigo_result client_attach(indigo_client *client) {
	indigo_log("attached to INDI bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result client_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	printf("%s.%s\n", property->device, property->name);
	return INDIGO_OK;
}

static indigo_result client_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	return INDIGO_OK;
}


static indigo_result client_detach(indigo_client *client) {
	indigo_log("detached from INDI bus...");
	exit(0);
	return INDIGO_OK;
}

static indigo_client client = {
	"indigo_prop_tool", NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, INDIGO_ENABLE_BLOB_ALSO,
	client_attach,
	client_define_property,
	client_update_property,
	NULL,
	NULL,
	client_detach
};

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	
	indigo_start();
	indigo_connect_server("localhost", 7624);
	indigo_attach_client(&client);
	sleep(2);
	indigo_stop();
	indigo_disconnect_server("localhost", 7624);
	return 0;
}

