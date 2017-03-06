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

#define REMINDER_MAX_SIZE 2048

static bool change_requested = false;

typedef struct {
	int item_count;
	char device_name[INDIGO_NAME_SIZE];
	char property_name[INDIGO_NAME_SIZE];
	char item_name[INDIGO_MAX_ITEMS][INDIGO_NAME_SIZE];
	char value_string[INDIGO_MAX_ITEMS][INDIGO_VALUE_SIZE];
} property_change_request;

static property_change_request change_request;


void trim_ending_spaces(char * str) {
	int len = strlen(str);
	while(isspace(str[len - 1])) str[--len] = '\0';
}


char* str_upper_case(char *str) {
	int i = 0;

	while (str[i]) {
		str[i] = toupper(str[i]);
		i++;
	}
	return str;
}


void trim_spaces(char * str) {
	/* trim ending */
	int len = strlen(str);
	while(isspace(str[len - 1])) str[--len] = '\0';

	/* trim begining */
	int skip = 0;
	while(isspace(str[skip])) skip++;
	if (skip) {
		len = 0;
		while (str[len + skip] != '\0') {
			str[len] = str[len + skip];
			len++;
		}
		str[len] = '\0';
	}
}


int parse_property_string(const char *prop_string, property_change_request *scr) {
	int res;
	char format[1024];
	char remainder[REMINDER_MAX_SIZE];

	if ((prop_string == NULL) || ( *prop_string == '\0')) {
		errno = EINVAL;
		return -1;
	}

	sprintf(format, "%%%d[^.].%%%d[^.].%%%d[^=]=%%%d[^\r]s", INDIGO_NAME_SIZE, INDIGO_NAME_SIZE, INDIGO_NAME_SIZE, REMINDER_MAX_SIZE);
	res = sscanf(prop_string, format, scr->device_name, scr->property_name, scr->item_name[0], remainder);
	if (res != 4) {
		errno = EINVAL;
		return -1;
	}
	trim_ending_spaces(scr->item_name[0]);
	trim_spaces(remainder);

	scr->item_count = 1;
	bool finished = false;
	while(!finished) {
		sprintf(format, "%%%d[^;];%%%d[^\r]s", INDIGO_VALUE_SIZE, REMINDER_MAX_SIZE);
		res = sscanf(remainder, format, scr->value_string[scr->item_count-1], remainder);
		trim_spaces(scr->value_string[scr->item_count-1]);
		if (res == 1) {
			finished = true;
			break;
		} else if (res == 2) {
			trim_spaces(remainder);
			scr->item_count++;
			sprintf(format, "%%%d[^=]=%%%d[^\r]s", INDIGO_NAME_SIZE, REMINDER_MAX_SIZE);
			res = sscanf(remainder, format, scr->item_name[scr->item_count-1], remainder);
			if (res != 2) {
				errno = EINVAL;
				return -1;
			}
			trim_spaces(scr->item_name[scr->item_count-1]);
			trim_spaces(remainder);
		} else {
			errno = EINVAL;
			return -1;
		}
	}
	//for(int i=0; i < scr->item_count; i++)
	//	printf("%d -> %s.%s.%s = %s\n", i, scr->device_name, scr->property_name, scr->item_name[i], scr->value_string[i]);
	return scr->item_count;
}


void print_property_string(indigo_property *property, const char *message) {
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
		if (!strcmp(property->device, change_request.device_name) && !strcmp(property->name, change_request.property_name)) {
			static char *items[INDIGO_MAX_ITEMS];
			static char *txt_values[INDIGO_MAX_ITEMS];
			static bool bool_values[INDIGO_MAX_ITEMS];
			double dbl_values[INDIGO_MAX_ITEMS];

			for (i = 0; i< change_request.item_count; i++) {
				items[i] = (char *)malloc(INDIGO_NAME_SIZE);
				strncpy(items[i], change_request.item_name[i], INDIGO_NAME_SIZE);
			}

			switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				for (i = 0; i< change_request.item_count; i++) {
					txt_values[i] = (char *)malloc(INDIGO_VALUE_SIZE);
					strncpy(txt_values[i], change_request.value_string[i], INDIGO_VALUE_SIZE);
					txt_values[i][INDIGO_VALUE_SIZE-1] = 0;
				}

				indigo_change_text_property(client, property->device, property->name, change_request.item_count, (const char **)items, (const char **)txt_values);

				for (i = 0; i< change_request.item_count; i++) {
					free(txt_values[i]);
				}
				break;
			case INDIGO_NUMBER_VECTOR:
				for (i = 0; i< change_request.item_count; i++) {
					dbl_values[i] = strtod(change_request.value_string[i], NULL);
				}
				indigo_change_number_property(client, property->device, property->name, change_request.item_count, (const char **)items, (const double *)dbl_values);
				break;
			case INDIGO_SWITCH_VECTOR:
				for (i = 0; i< change_request.item_count; i++) {
					if (!strcmp("ON", str_upper_case(change_request.value_string[i])))
						bool_values[i] = true;
					else if (!strcmp("OFF", str_upper_case(change_request.value_string[i])))
						bool_values[i] = false;
					else {
						/* should indicate error */
					}
				}
				indigo_change_switch_property(client, property->device, property->name, change_request.item_count, (const char **)items, (const bool *)bool_values);
				break;
			case INDIGO_LIGHT_VECTOR:
				printf("%s.%s.%s = %d\n", property->device, property->name, item->name, item->light.value);
				break;
			case INDIGO_BLOB_VECTOR:
				printf("%s.%s.%s = <BLOBS NOT SHOWN>\n", property->device, property->name, item->name);
				break;
			}

			printf("MATCHED:\n");
			for (i = 0; i< change_request.item_count; i++) {
				free(items[i]);
				printf("%s.%s.%s = %s\n", change_request.device_name, change_request.property_name, change_request.item_name[i], change_request.value_string[i]);
			}
		}
		return INDIGO_OK;
	} else {
		print_property_string(property, message);
	}
	return INDIGO_OK;
}


static indigo_result client_update_property(struct indigo_client *client, struct indigo_device *device, indigo_property *property, const char *message) {
	printf("UPDATED:\n");
	print_property_string(property, message);
	return INDIGO_OK;
}


static indigo_result client_detach(indigo_client *client) {
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


static void print_help(const char *name) {
	printf("usage: %s set|list [params]\n", name);
}


int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;

	if (argc < 2) {
		print_help(argv[0]);
		return 0;
	}

	bool action_set = true;
	char const *prop_string = NULL;
	int arg_base = 1;

	if(!strcmp(argv[1], "set")) {
		action_set = true;
		arg_base = 2;
	} else if (!strcmp(argv[1], "list")) {
		action_set = false;
		arg_base = 2;
	}

	for (int i = arg_base; i < argc; i++) {
		if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
			//verbose list
			printf("1 %d %s\n",  i, argv[i]);
		} else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--remote-server")) {
			//handle remote server
			printf("2 %d %s\n",  i, argv[i]);
		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) {
			//handle port
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0]);
			return 0;
		} else {
			prop_string = argv[i];
		}
	}

	indigo_use_host_suffix = false;

	if (action_set) {
		if (parse_property_string(prop_string, &change_request) < 0) {
			perror("parse_property_string()");
			return 1;
		}
		//for (int i = 0; i< change_request.item_count; i++) {
		//	printf("PARSED: %s.%s.%s = %s\n", change_request.device_name, change_request.property_name, change_request.item_name[i],  change_request.value_string[i]);
		//}
		change_requested = true;
	}

	indigo_start();
	indigo_attach_client(&client);
	indigo_server_entry *server;
	indigo_connect_server("localhost", 7624, &server);
	sleep(2);
	indigo_stop();
	indigo_disconnect_server(server);
	return 0;
}

