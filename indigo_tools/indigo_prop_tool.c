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
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_xml.h>

#define INDIGO_DEFAULT_PORT 7624
#define REMINDER_MAX_SIZE 2048
#define TEXT_LEN_TO_PRINT 80

//#define DEBUG

static bool set_requested = false;
static bool set_script_requested = false;
static bool get_requested = false;
static bool list_state_requested = false;
static bool get_state_requested = false;
static bool print_verbose = false;
static bool save_blobs = false;

typedef struct {
	int item_count;
	char device_name[INDIGO_NAME_SIZE];
	char property_name[INDIGO_NAME_SIZE];
	char item_name[INDIGO_MAX_ITEMS][INDIGO_NAME_SIZE];
	char value_string[INDIGO_MAX_ITEMS][INDIGO_VALUE_SIZE];
} property_change_request;

typedef struct {
	int item_count;
	char device_name[INDIGO_NAME_SIZE];
	char property_name[INDIGO_NAME_SIZE];
	char item_name[INDIGO_MAX_ITEMS][INDIGO_NAME_SIZE];
} property_get_request;

typedef struct {
	char device_name[INDIGO_NAME_SIZE];
	char property_name[INDIGO_NAME_SIZE];
} property_list_request;


static property_change_request change_request;
static property_list_request list_request;
static property_get_request get_request;


int read_file(const char *file_name, char **file_data) {
	int size = 0;
	FILE *f = fopen(file_name, "rb");
	if (f == NULL)  {
		*file_data = NULL;
		return -1;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*file_data = (char *)malloc(size+1);
	if (size != fread(*file_data, sizeof(char), size, f)) {
		free(*file_data);
		file_data = NULL;
		return -2;
	}
	fclose(f);
	(*file_data)[size] = 0;
	return size;
}


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


int process_quotes(char *value) {
	char buf[INDIGO_VALUE_SIZE];
	char *ptr;

	if (!value) {
		errno = EINVAL;
		return -1;
	}

	indigo_copy_value(buf, value);
	ptr = value;
	for (int i = 0; i < strlen(buf); i++) {
		if ((buf[i] == '\\') && (buf[i+1] == '\"')) {
			*ptr++ = '\"';
			i++;
		} else if (buf[i] != '\"') {
			*ptr++ = buf[i];
		}
	}
	*ptr = '\0';
	return 0;
}


int parse_list_property_string(const char *prop_string, property_list_request *plr) {
	int res;
	char format[1024];

	plr->device_name[0] = '\0';
	plr->property_name[0] = '\0';

	if ((prop_string == NULL) || ( *prop_string == '\0')) {
		return 0;
	}

	sprintf(format, "%%%d[^.].%%%ds", INDIGO_NAME_SIZE, INDIGO_NAME_SIZE);
	res = sscanf(prop_string, format, plr->device_name, plr->property_name);
	if (res > 2) {
		errno = EINVAL;
		return -1;
	}
	trim_ending_spaces(plr->property_name);
	return res;
}


int parse_set_property_string(const char *prop_string, property_change_request *scr) {
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
		process_quotes(scr->value_string[scr->item_count-1]);
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
	return scr->item_count;
}


int parse_get_property_string(const char *prop_string, property_get_request *sgr) {
	int res;
	char format[1024];
	char remainder[REMINDER_MAX_SIZE];

	if ((prop_string == NULL) || ( *prop_string == '\0')) {
		errno = EINVAL;
		return -1;
	}

	sprintf(format, "%%%d[^.].%%%d[^.].%%%d[^\r]s", INDIGO_NAME_SIZE, INDIGO_NAME_SIZE, REMINDER_MAX_SIZE);
	res = sscanf(prop_string, format, sgr->device_name, sgr->property_name, remainder);
	if (res != 3) {
		errno = EINVAL;
		return -1;
	}
	trim_spaces(remainder);

	sgr->item_count = 0;
	bool finished = false;
	while(!finished) {
		sprintf(format, "%%%d[^;];%%%d[^\r]s", INDIGO_NAME_SIZE, REMINDER_MAX_SIZE);
		res = sscanf(remainder, format, sgr->item_name[sgr->item_count], remainder);
		trim_spaces(sgr->item_name[sgr->item_count]);
		if (res == 1) {
			sgr->item_count++;
			finished = true;
			break;
		} else if (res == 2) {
			sgr->item_count++;
		} else {
			errno = EINVAL;
			return -1;
		}
	}
	return sgr->item_count;
}


static void save_blob(char *filename, char *data, size_t length) {
	int fd = open(filename, O_WRONLY | O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if (fd == -1) {
		INDIGO_ERROR(indigo_error("Open file %s failed: %s", filename, strerror(errno)));
		return;
	}
	int len = write(fd, data, length);
	if (len <= 0) {
		INDIGO_ERROR(indigo_error("Write blob to file %s failed: %s", filename, strerror(errno)));
	}
	close(fd);
}


void print_property_list(indigo_property *property, const char *message) {
	indigo_item *item;
	int i;
	if (print_verbose) {
		char perm_str[3] = "";
		switch(property->perm) {
		case INDIGO_RW_PERM:
			strcpy(perm_str, "RW");
			break;
		case INDIGO_RO_PERM:
			strcpy(perm_str, "RO");
			break;
		case INDIGO_WO_PERM:
			strcpy(perm_str, "WO");
			break;
		}

		char type_str[20] = "";
		switch(property->type) {
		case INDIGO_TEXT_VECTOR:
			strcpy(type_str, "TEXT_VECTOR");
			break;
		case INDIGO_NUMBER_VECTOR:
			strcpy(type_str, "NUMBER_VECTOR");
			break;
		case INDIGO_SWITCH_VECTOR:
			strcpy(type_str, "SWITCH_VECTOR");
			break;
		case INDIGO_LIGHT_VECTOR:
			strcpy(type_str, "LIGHT_VECTOR");
			break;
		case INDIGO_BLOB_VECTOR:
			strcpy(type_str, "BLOB_VECTOR");
			break;
		}

		char state_str[20] = "";
		switch(property->state) {
		case INDIGO_IDLE_STATE:
			strcpy(state_str, "IDLE");
			break;
		case INDIGO_ALERT_STATE:
			strcpy(state_str, "ALERT");
			break;
		case INDIGO_OK_STATE:
			strcpy(state_str, "OK");
			break;
		case INDIGO_BUSY_STATE:
			strcpy(state_str, "BUSY");
			break;
		}

		printf("Name : %s.%s (%s, %s)\nState: %s\nGroup: %s\nLabel: %s\n", property->device, property->name, perm_str, type_str, state_str, property->group, property->label);
		if (message) {
			printf("Message:\"%s\"\n", message);
		}
		printf("Items:\n");
	}

	for (i = 0; i < property->count; i++) {
		item = &(property->items[i]);
		switch (property->type) {
		case INDIGO_TEXT_VECTOR:
			if (item->text.length > TEXT_LEN_TO_PRINT) {
				printf("%s.%s.%s = \"%.*s\" + %ld characters\n", property->device, property->name, item->name, TEXT_LEN_TO_PRINT, item->text.value, item->text.length - TEXT_LEN_TO_PRINT - 1);
			} else {
				printf("%s.%s.%s = \"%s\"\n", property->device, property->name, item->name, item->text.value);
			}
			break;
		case INDIGO_NUMBER_VECTOR:
			printf("%s.%s.%s = %f\n", property->device, property->name, item->name, item->number.value);
			break;
		case INDIGO_SWITCH_VECTOR:
			printf("%s.%s.%s = %s\n", property->device, property->name, item->name, item->sw.value ? "ON" : "OFF");
			break;
		case INDIGO_LIGHT_VECTOR:
			printf("%s.%s.%s = %s\n", property->device, property->name, item->name, item->light.value ? "ON" : "OFF");
			break;
		case INDIGO_BLOB_VECTOR:
			if ((save_blobs) && (!indigo_use_blob_urls) && (item->blob.size > 0) && (property->state == INDIGO_OK_STATE)) {
				char filename[PATH_MAX];
				snprintf(filename, PATH_MAX, "%s.%s.%s%s", property->device, property->name, item->name, item->blob.format);
				printf("%s.%s.%s = <BLOB => %s>\n", property->device, property->name, item->name, filename);
				save_blob(filename, item->blob.value, item->blob.size);
			} else if ((save_blobs) && (indigo_use_blob_urls) && (item->blob.url[0] != '\0') && (property->state == INDIGO_OK_STATE)) {
				if (indigo_populate_http_blob_item(item)) {
					char filename[PATH_MAX];
					snprintf(filename, PATH_MAX, "%s.%s.%s%s", property->device, property->name, item->name, item->blob.format);
					printf("%s.%s.%s = <%s => %s>\n", property->device, property->name, item->name, item->blob.url, filename);
					save_blob(filename, item->blob.value, item->blob.size);
					free(item->blob.value);
					item->blob.value = NULL;
				} else {
					INDIGO_ERROR(indigo_error("Can not retrieve data from %s", item->blob.url));
				}
			} else if ((!save_blobs) && (indigo_use_blob_urls) && (item->blob.url[0] != '\0') && (property->state == INDIGO_OK_STATE)) {
				printf("%s.%s.%s = <%s>\n", property->device, property->name, item->name, item->blob.url);
			} else if ((!save_blobs) && (!indigo_use_blob_urls) && (item->blob.size > 0) && (property->state == INDIGO_OK_STATE)) {
				printf("%s.%s.%s = <BLOB NOT SHOWN>\n", property->device, property->name, item->name);
			} else {
				// printf("save_blobs = %d, indigo_use_blob_urls = %d, item->blob.size = %d, item->blob.value = %p, property->state = %d\n", save_blobs, indigo_use_blob_urls, item->blob.size, item->blob.value, property->state);
				printf("%s.%s.%s = <NO BLOB DATA>\n", property->device, property->name, item->name);
			}
			break;
		}
	}
	if (print_verbose) printf("\n");
}


static void print_property_list_filtered(indigo_property *property, const char *message, const property_list_request *filter) {
	if ((filter == NULL) || ((filter->device_name[0] == '\0') && (filter->property_name[0] == '\0'))) {
		/* list all properties */
		print_property_list(property, message);
	} else if (filter->property_name[0] == '\0') {
		/* list all poperties of the device */
		if (!strncmp(filter->device_name, property->device, INDIGO_NAME_SIZE)) {
			print_property_list(property, message);
		}
	} else {
		/* list all poperties that match device and property name */
		if ((!strncmp(filter->device_name, property->device, INDIGO_NAME_SIZE)) &&
		   (!strncmp(filter->property_name, property->name, INDIGO_NAME_SIZE))) {
			print_property_list(property, message);
		}
	}
}


static void print_property_get_filtered(indigo_property *property, const char *message, const property_get_request *filter) {
	indigo_item *item;
	char value_string[INDIGO_MAX_ITEMS][PATH_MAX] = {0};
	char filename[PATH_MAX+15];
	int i, r, items_found = 0;

	if (filter == NULL) return;

	if ((strncmp(filter->device_name, property->device, INDIGO_NAME_SIZE)) ||
	   (strncmp(filter->property_name, property->name, INDIGO_NAME_SIZE)))
		return;

	for (r = 0; r < filter->item_count; r++) {
		for (i = 0; i < property->count; i++) {
			item = &(property->items[i]);
			if (strcmp(item->name, filter->item_name[r])) continue;

			switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				if (item->text.length > TEXT_LEN_TO_PRINT) {
					sprintf(value_string[items_found], "%.*s + %ld characters\n", TEXT_LEN_TO_PRINT, item->text.value, item->text.length - TEXT_LEN_TO_PRINT - 1);
				} else {
					sprintf(value_string[items_found], "%s", item->text.value);
				}
				break;
			case INDIGO_NUMBER_VECTOR:
				sprintf(value_string[items_found], "%f", item->number.value);
				break;
			case INDIGO_SWITCH_VECTOR:
				sprintf(value_string[items_found], item->sw.value ? "ON" : "OFF");
				break;
			case INDIGO_LIGHT_VECTOR:
				if (item->light.value)
					sprintf(value_string[items_found], "ON");
				else
					sprintf(value_string[items_found], "OFF");
				break;
			case INDIGO_BLOB_VECTOR:
				if ((save_blobs) && (!indigo_use_blob_urls) && (item->blob.size > 0) && (property->state == INDIGO_OK_STATE)) {
					snprintf(filename, PATH_MAX, "%s.%s.%s%s", property->device, property->name, item->name, item->blob.format);
					sprintf(value_string[items_found], "file://%s/%s", getcwd(NULL, 0), filename);
					save_blob(filename, item->blob.value, item->blob.size);
				} else if ((save_blobs) && (indigo_use_blob_urls) && (item->blob.url[0] != '\0') && (property->state == INDIGO_OK_STATE)) {
					if (indigo_populate_http_blob_item(item)) {
						snprintf(filename, PATH_MAX, "%s.%s.%s%s", property->device, property->name, item->name, item->blob.format);
						sprintf(value_string[items_found], "file://%s/%s", getcwd(NULL, 0), filename);
						save_blob(filename, item->blob.value, item->blob.size);
						free(item->blob.value);
						item->blob.value = NULL;
					} else {
						INDIGO_ERROR(indigo_error("Can not retrieve data from %s", item->blob.url));
					}
				} else if ((!save_blobs) && (indigo_use_blob_urls) && (item->blob.url[0] != '\0') && (property->state == INDIGO_OK_STATE)) {
					sprintf(value_string[items_found], "%s", item->blob.url);
				} else if ((!save_blobs) && (!indigo_use_blob_urls) && (item->blob.size > 0) && (property->state == INDIGO_OK_STATE)) {
					sprintf(value_string[items_found], "<BLOB NOT SHOWN>");
				} else {
					// printf("2 save_blobs = %d, indigo_use_blob_urls = %d, item->blob.size = %d, item->blob.value = %p, property->state = %d\n", save_blobs, indigo_use_blob_urls, item->blob.size, item->blob.value, property->state);
					sprintf(value_string[items_found], "<NO BLOB DATA>");
				}
				break;
			}
			items_found++;
		}
	}
	if (items_found != filter->item_count) return;

	for (i = 0; i < items_found; i++) {
		printf("%s\n", value_string[i]);
	}
}


void print_property_list_state(indigo_property *property, const char *message) {
	if (print_verbose) {
		char perm_str[3] = "";
		switch(property->perm) {
		case INDIGO_RW_PERM:
			strcpy(perm_str, "RW");
			break;
		case INDIGO_RO_PERM:
			strcpy(perm_str, "RO");
			break;
		case INDIGO_WO_PERM:
			strcpy(perm_str, "WO");
			break;
		}

		char type_str[20] = "";
		switch(property->type) {
		case INDIGO_TEXT_VECTOR:
			strcpy(type_str, "TEXT_VECTOR");
			break;
		case INDIGO_NUMBER_VECTOR:
			strcpy(type_str, "NUMBER_VECTOR");
			break;
		case INDIGO_SWITCH_VECTOR:
			strcpy(type_str, "SWITCH_VECTOR");
			break;
		case INDIGO_LIGHT_VECTOR:
			strcpy(type_str, "LIGHT_VECTOR");
			break;
		case INDIGO_BLOB_VECTOR:
			strcpy(type_str, "BLOB_VECTOR");
			break;
		}

		printf("Name : %s.%s (%s, %s)\nGroup: %s\nLabel: %s\n", property->device, property->name, perm_str, type_str, property->group, property->label);
		if (message) {
			printf("Message:\"%s\"\n", message);
		}
	}

	char state_str[20] = "";
	switch(property->state) {
	case INDIGO_IDLE_STATE:
		strcpy(state_str, "IDLE");
		break;
	case INDIGO_ALERT_STATE:
		strcpy(state_str, "ALERT");
		break;
	case INDIGO_OK_STATE:
		strcpy(state_str, "OK");
		break;
	case INDIGO_BUSY_STATE:
		strcpy(state_str, "BUSY");
		break;
	}

	printf("%s.%s = %s\n", property->device, property->name, state_str);

	if (print_verbose) printf("\n");
}


static void print_property_list_state_filtered(indigo_property *property, const char *message, const property_list_request *filter) {
	if ((filter == NULL) || ((filter->device_name[0] == '\0') && (filter->property_name[0] == '\0'))) {
		/* list all properties */
		print_property_list_state(property, message);
	} else if (filter->property_name[0] == '\0') {
		/* list all poperties of the device */
		if (!strncmp(filter->device_name, property->device, INDIGO_NAME_SIZE)) {
			print_property_list_state(property, message);
		}
	} else {
		/* list all poperties that match device and property name */
		if ((!strncmp(filter->device_name, property->device, INDIGO_NAME_SIZE)) &&
		   (!strncmp(filter->property_name, property->name, INDIGO_NAME_SIZE))) {
			print_property_list_state(property, message);
		}
	}
}


void print_property_get_state(indigo_property *property, const char *message) {
	char state_str[20] = "";
	switch(property->state) {
	case INDIGO_IDLE_STATE:
		strcpy(state_str, "IDLE");
		break;
	case INDIGO_ALERT_STATE:
		strcpy(state_str, "ALERT");
		break;
	case INDIGO_OK_STATE:
		strcpy(state_str, "OK");
		break;
	case INDIGO_BUSY_STATE:
		strcpy(state_str, "BUSY");
		break;
	}

	printf("%s\n", state_str);
}


static void print_property_get_state_filtered(indigo_property *property, const char *message, const property_list_request *filter) {
	if (filter != NULL) {
		/* list all poperties that match device and property name */
		if ((!strncmp(filter->device_name, property->device, INDIGO_NAME_SIZE)) &&
		   (!strncmp(filter->property_name, property->name, INDIGO_NAME_SIZE))) {
			print_property_get_state(property, message);
		}
	}
}


static indigo_result client_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}


static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	indigo_item *item;
	int i;
	int r;
	static bool called = false;

	if (!called && print_verbose) {
		printf("Protocol version = %x.%x\n\n", property->version >> 8, property->version & 0xff);
		called = true;
	}

	if ((property->type == INDIGO_BLOB_VECTOR) && (save_blobs || indigo_use_blob_urls)) {
		indigo_enable_blob(client, property, indigo_use_blob_urls ? INDIGO_ENABLE_BLOB_URL : INDIGO_ENABLE_BLOB_ALSO);
	}

	if (set_requested) {
		if (!strcmp(property->device, change_request.device_name) && !strcmp(property->name, change_request.property_name)) {
			static char *items[INDIGO_MAX_ITEMS] = {NULL};
			static char *txt_values[INDIGO_MAX_ITEMS] = {NULL};
			static bool bool_values[INDIGO_MAX_ITEMS] = {false};
			double dbl_values[INDIGO_MAX_ITEMS];

			for (i = 0; i< change_request.item_count; i++) {
				items[i] = (char *)malloc(INDIGO_NAME_SIZE);
				indigo_copy_name(items[i], change_request.item_name[i]);
			}

			switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				if (set_script_requested) {
					bool name_provided = false;
					bool file_provided = false;
					for (i = 0; i < change_request.item_count; i++) {
						if (!strcmp(AGENT_SCRIPTING_SCRIPT_ITEM_NAME, change_request.item_name[i])) {
							int res = read_file(change_request.value_string[i], &txt_values[i]);
							if (res < 0) {
								fprintf(stderr, "Can't read '%s' file: %s\n", change_request.value_string[i], strerror(errno));
								exit(1);
							}
							file_provided = true;
						} else {
							if (!strcmp(AGENT_SCRIPTING_SCRIPT_NAME_ITEM_NAME, change_request.item_name[i])) {
								name_provided = true;
							}
							txt_values[i] = (char *)malloc(INDIGO_VALUE_SIZE);
							indigo_copy_value(txt_values[i], change_request.value_string[i]);
							txt_values[i][INDIGO_VALUE_SIZE-1] = 0;
						}
					}
					if ((!name_provided || !file_provided) && !strcmp(AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME, property->name)) {
						fprintf(stderr, "Property %s requires both %s and %s items to be set\n", AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME, AGENT_SCRIPTING_SCRIPT_ITEM_NAME, AGENT_SCRIPTING_SCRIPT_NAME_ITEM_NAME);
						exit(1);
					}
				} else {
					for (i = 0; i < change_request.item_count; i++) {
						txt_values[i] = (char *)malloc(INDIGO_VALUE_SIZE);
						indigo_copy_value(txt_values[i], change_request.value_string[i]);
						txt_values[i][INDIGO_VALUE_SIZE-1] = 0;
					}
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
				if (property->perm == INDIGO_WO_PERM) {
					for (r = 0; r < change_request.item_count; r++) {
						for (i = 0; i < property->count; i++) {
							item = &(property->items[i]);
							if (strcmp(item->name, change_request.item_name[r])) continue;
							int size = read_file(change_request.value_string[r], (char**)&item->blob.value);
							if (size < 0) {
								fprintf(stderr, "Can't read '%s' file: %s\n", change_request.value_string[r], strerror(errno));
								exit(1);
							} else {
								item->blob.size = size;
							}
							if (INDIGO_OK == indigo_change_blob_property_1(client, property->device, property->name, item->name, item->blob.value, item->blob.size, "", item->blob.url)) {
								printf("%s.%s.%s = <%s => %s>\n", property->device, property->name, item->name, change_request.value_string[r], item->blob.url);
							} else {
								fprintf(stderr, "Can't upload %s to %s\n", change_request.value_string[r], item->blob.url);
								free(item->blob.value);
								exit(1);
							}
							free(item->blob.value);
						}
					}
				} else {
					printf("%s.%s.%s = <BLOB NOT SHOWN>\n", property->device, property->name, item->name);
				}
				break;
			}
#ifdef DEBUG
			printf("MATCHED:\n");
			for (i = 0; i< change_request.item_count; i++) {
				printf("%s.%s.%s = %s\n", change_request.device_name, change_request.property_name, change_request.item_name[i], change_request.value_string[i]);
			}
#endif
			for (i = 0; i< change_request.item_count; i++) {
				free(items[i]);
			}
		}
		return INDIGO_OK;
	} else if (list_state_requested) {
		print_property_list_state_filtered(property, message, &list_request);
	} else if (get_requested) {
		print_property_get_filtered(property, message, &get_request);
	} else if (get_state_requested) {
		print_property_get_state_filtered(property, message, &list_request);
	} else {
		print_property_list_filtered(property, message, &list_request);
	}
	return INDIGO_OK;
}


static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (set_requested) {
		print_property_list(property, message);
	} else if (get_requested) {
		print_property_get_filtered(property, message, &get_request);
	} else if (list_state_requested) {
		print_property_list_state_filtered(property, message, &list_request);
	} else if (get_state_requested) {
		print_property_get_state_filtered(property, message, &list_request);
	} else {
		print_property_list_filtered(property, message, &list_request);
	}
	return INDIGO_OK;
}


static indigo_result client_detach(indigo_client *client) {
	exit(0);
	return INDIGO_OK;
}


static indigo_client client = {
	"indigo_prop_tool", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
	client_attach,
	client_define_property,
	client_update_property,
	NULL,
	NULL,
	client_detach
};


static void print_help(const char *name) {
	printf("INDIGO property manipulation tool v.%d.%d-%s built on %s %s.\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD, __DATE__, __TIME__);
	printf("usage: %s [options] device.property.item=value[;item=value;..]\n", name);
	printf("       %s set [options] device.property.item=value[;item=value;..]\n", name);
	printf("       %s set_script [options] agent.property.SCRIPT=filename[;NAME=filename]\n", name);
	printf("       %s get [options] device.property.item[;item;..]\n", name);
	printf("       %s get_state [options] device.property\n", name);
	printf("       %s list [options] [device[.property]]\n", name);
	printf("       %s list_state [options] [device[.property]]\n", name);
	printf("set write-only BLOBs:\n");
	printf("       %s set [options] device.property.item=filename[;NAME=filename]\n", name);
	printf("options:\n"
	       "       -h  | --help\n"
	       "       -b  | --save-blobs\n"
	       "       -l  | --use_legacy_blobs\n"
	       "       -e  | --extended-info\n"
	       "       -v  | --enable-log\n"
	       "       -vv | --enable-debug\n"
	       "       -vvv| --enable-trace\n"
	       "       -r  | --remote-server host[:port]   (default: localhost)\n"
	       "       -p  | --port port                   (default: 7624)\n"
	       "       -T  | --token token\n"
	       "       -t  | --time-to-wait seconds        (default: 2)\n"
	);
}


int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_use_host_suffix = false;
	indigo_use_blob_urls = true;

	if (argc < 2) {
		print_help(argv[0]);
		return 0;
	}

	double time_to_wait = 2;
	int port = INDIGO_DEFAULT_PORT;
	char hostname[255] = "localhost";
	char const *prop_string = NULL;

	set_requested = true;
	int arg_base = 1;
	if(!strcmp(argv[1], "set")) {
		set_requested = true;
		arg_base = 2;
	} else if (!strcmp(argv[1], "list")) {
		set_requested = false;
		arg_base = 2;
	} else if (!strcmp(argv[1], "get")) {
		set_requested = false;
		get_requested = true;
		arg_base = 2;
	} else if (!strcmp(argv[1], "list_state")) {
		set_requested = false;
		list_state_requested = true;
		arg_base = 2;
	} else if (!strcmp(argv[1], "get_state")) {
		set_requested = false;
		get_state_requested = true;
		arg_base = 2;
	} else if (!strcmp(argv[1], "set_script")) {
		set_requested = true;
		set_script_requested = true;
		arg_base = 2;
	}

	for (int i = arg_base; i < argc; i++) {
		if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--extended-info")) {
			print_verbose = true;
		} else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--save-blobs")) {
			if (list_state_requested) {
				fprintf(stderr, "Blobs can not be saved with 'state' command\n");
				return 1;
			}
			save_blobs = true;
		} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--use_legacy-blobs")) {
			indigo_use_blob_urls = false;
		} else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--remote-server")) {
			if (argc > i+1) {
				i++;
				char port_str[100];
				if (sscanf(argv[i], "%[^:]:%s", hostname, port_str) > 1) {
					port = atoi(port_str);
				}
			} else {
				fprintf(stderr, "No hostname specified\n");
				return 1;
			}
		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) {
			if (argc > i+1) {
				i++;
				port = atoi(argv[i]);
			} else {
				fprintf(stderr, "No port specified\n");
				return 1;
			}
		} else if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "--token")) {
			if (argc > i+1) {
				i++;
				indigo_set_master_token(indigo_string_to_token(argv[i]));
			} else {
				fprintf(stderr, "No token specified\n");
				return 1;
			}
		} else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--time-to-wait")) {
			if (argc > i+1) {
				i++;
				time_to_wait = atof(argv[i]);
			} else {
				fprintf(stderr, "No time to wait specified\n");
				return 1;
			}
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_help(argv[0]);
			return 0;
		} else if (argv[i][0] == '-') {
			/* skip unknown options */
		} else {
			prop_string = argv[i];
		}
	}

	if (port <= 0) {
		fprintf(stderr, "Invalied port specified\n");
		return 1;
	}

	if (time_to_wait < 0) {
		fprintf(stderr, "Invalid time to wait specified\n");
		return 1;
	}

	if (set_requested) {
		if (parse_set_property_string(prop_string, &change_request) < 0) {
			fprintf(stderr, "Invalid property string format\n");
			return 1;
		}
		#ifdef DEBUG
		for (int i = 0; i< change_request.item_count; i++) {
			printf("PARSED: %s.%s.%s = %s\n", change_request.device_name, change_request.property_name, change_request.item_name[i],  change_request.value_string[i]);
		}
		#endif
	} else if (get_requested) {
		if (parse_get_property_string(prop_string, &get_request) < 0) {
			fprintf(stderr, "Invalid property string format\n");
			return 1;
		}
		#ifdef DEBUG
		for (int i = 0; i< get_request.item_count; i++) {
			printf("PARSED: %s.%s.%s\n", get_request.device_name, get_request.property_name, get_request.item_name[i]);
		}
		#endif
	} else if (list_state_requested) {
		if (parse_list_property_string(prop_string, &list_request) < 0) {
			fprintf(stderr, "Invalid property string format\n");
			return 1;
		}
		#ifdef DEBUG
		printf("PARSED: %s * %s\n", list_request.device_name, list_request.property_name);
		#endif
	} else if (get_state_requested) {
		/* Device and property is needed so != 2 */
		if (parse_list_property_string(prop_string, &list_request) != 2) {
			fprintf(stderr, "Invalid property string format\n");
			return 1;
		}
		#ifdef DEBUG
		printf("PARSED: %s * %s\n", list_request.device_name, list_request.property_name);
		#endif
	} else {
		if (parse_list_property_string(prop_string, &list_request) < 0) {
			fprintf(stderr, "Invalid property string format\n");
			return 1;
		}
		#ifdef DEBUG
		printf("PARSED: %s * %s\n", list_request.device_name, list_request.property_name);
		#endif
	}

	indigo_start();
	indigo_attach_client(&client);
	indigo_server_entry *server;
	indigo_connect_server(hostname, hostname, port, &server);
	int wait_connection = 1000;
	bool connected = false;
	char error_message[INDIGO_VALUE_SIZE] = {0};
	while (wait_connection--) {
		if (true == (connected = indigo_connection_status(server, error_message))) {
			break;
		} else {
			indigo_usleep(10000);
		}
	}
	if (connected) {
		indigo_usleep(time_to_wait * ONE_SECOND_DELAY);
	} else {
		fprintf(stderr, "Connection failed: %s\n", error_message);
	}
	indigo_stop();
	indigo_disconnect_server(server);
	return 0;
}
