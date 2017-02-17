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
#include <ctype.h>

#include "indigo_bus.h"
#include "indigo_client.h"

static bool change_requested = false;

typedef struct {
	char device_name[INDIGO_NAME_SIZE];
	char property_name[INDIGO_NAME_SIZE];
	char item_name[INDIGO_NAME_SIZE];
	char value_string[INDIGO_VALUE_SIZE];
} property_change_request;

static property_change_request change_request;


void trim_ending_spaces(char * str) {
    int len = strlen(str);
    while(isspace(str[len - 1])) str[--len] = 0;
}


int parse_property_string(const char *prop_string, property_change_request *scr) {
	int res;
	char format[1024];
	sprintf(format, "%%%d[^.].%%%d[^.].%%%d[^=]=%%%d[^\r]s", INDIGO_NAME_SIZE, INDIGO_NAME_SIZE, INDIGO_NAME_SIZE, INDIGO_VALUE_SIZE);
	//printf("%s\n", format);
	res = sscanf(prop_string, format, scr->device_name, scr->property_name, scr->item_name, scr->value_string);
	if (res != 4) {
		errno = EINVAL;
		return -1;
	}
	trim_ending_spaces(scr->item_name);
	return res;
}


static indigo_result client_attach(indigo_client *client) {
	//indigo_log("attached to INDI bus...");
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}


static indigo_result client_define_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	indigo_item *item;
	int i;
	static bool called = false;

	if (!called) {
		printf("Protocol version = %x.%x\n", property->version >> 8, property->version & 0xff);
		called = true;
	}

	if (change_requested) {
		if( !strcmp(property->device, change_request.device_name) &&
		    !strcmp(property->name, change_request.property_name) ) {
			static char *items[INDIGO_MAX_ITEMS];
			static char *txt_values[INDIGO_MAX_ITEMS];
			double dbl_values[1];
			items[0] = (char *)malloc(INDIGO_NAME_SIZE);
			strncpy(items[0], change_request.item_name, INDIGO_NAME_SIZE);
			switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				txt_values[0] = (char *)malloc(INDIGO_VALUE_SIZE);
				strncpy(txt_values[0], change_request.value_string, INDIGO_VALUE_SIZE);
				txt_values[0][INDIGO_VALUE_SIZE-1] = 0;
				indigo_change_text_property(client, property->device, property->name, 1, (const char **)items, (const char **)txt_values);
				free(txt_values[0]);
				break;
			case INDIGO_NUMBER_VECTOR:
				dbl_values[0] = strtod(change_request.value_string, NULL);
				indigo_change_number_property(client, property->device, property->name, 1, (const char **)items, (const double *)dbl_values);
				break;
			case INDIGO_SWITCH_VECTOR:
				printf("5\n");
				if (item->sw.value)
					printf("%s.%s.%s = ON\n", property->device, property->name, item->name);
				else
					printf("%s.%s.%s = OFF\n", property->device, property->name, item->name);
				break;
			case INDIGO_LIGHT_VECTOR:
				printf("6\n");
				printf("%s.%s.%s = %d\n", property->device, property->name, item->name, item->light.value);
				break;
			case INDIGO_BLOB_VECTOR:
				printf("7\n");
				printf("%s.%s.%s = <BLOBS NOT SHOWN>\n", property->device, property->name, item->name);
				break;
			}
			free(items[0]);
			printf("MATCHED %s * %s * %s = %s\n", change_request.device_name, change_request.property_name, change_request.item_name, change_request.value_string);
		}
		return INDIGO_OK;
	} else {
		for (i = 0; i < property->count; i++) {
			item = &(property->items[i]);
			switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				printf("%s.%s.%s = \"%s\"\n", property->device, property->name, item->name, item->text.value);
				break;
			case INDIGO_NUMBER_VECTOR:
				printf("%s.%s.%s = %f\n", property->device, property->name, item->name, item->number.value);
				break;
			case INDIGO_SWITCH_VECTOR:
				if (item->sw.value)
					printf("%s.%s.%s = ON\n", property->device, property->name, item->name);
				else
					printf("%s.%s.%s = OFF\n", property->device, property->name, item->name);
				break;
			case INDIGO_LIGHT_VECTOR:
				printf("%s.%s.%s = %d\n", property->device, property->name, item->name, item->light.value);
				break;
			case INDIGO_BLOB_VECTOR:
				printf("%s.%s.%s = <BLOBS NOT SHOWN>\n", property->device, property->name, item->name);
				break;
			}
		}
	}
	return INDIGO_OK;
}


static indigo_result client_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	indigo_item *item;
	int i;
	for (i = 0; i < property->count; i++) {
		item = &(property->items[i]);
		switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			printf("%s.%s.%s = \"%s\"\n", property->device, property->name, item->name, item->text.value);
			break;
		case INDIGO_NUMBER_VECTOR:
			printf("%s.%s.%s = %f\n", property->device, property->name, item->name, item->number.value);
			break;
		case INDIGO_SWITCH_VECTOR:
			if (item->sw.value)
				printf("%s.%s.%s = ON\n", property->device, property->name, item->name);
			else
				printf("%s.%s.%s = OFF\n", property->device, property->name, item->name);
			break;
		case INDIGO_LIGHT_VECTOR:
			printf("%s.%s.%s = %d\n", property->device, property->name, item->name, item->light.value);
			break;
		case INDIGO_BLOB_VECTOR:
			printf("%s.%s.%s = <BLOBS NOT SHOWN>\n", property->device, property->name, item->name);
			break;
		}
	}
	return INDIGO_OK;
}


static indigo_result client_detach(indigo_client *client) {
	//indigo_log("detached from INDI bus...");
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

	indigo_use_host_suffix = false;

	if (argc == 2) {
		if (parse_property_string(argv[1], &change_request) < 0) {
			perror("parse_property_string()");
			return 1;
		}
		change_requested = true;
	}

	indigo_start();
	indigo_attach_client(&client);
	indigo_connect_server("localhost", 7624);
	sleep(2);
	indigo_stop();
	indigo_disconnect_server("localhost", 7624);
	return 0;
}

