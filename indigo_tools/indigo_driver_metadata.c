//
//  indigo_driver_metadata.c
//  indigo
//
//  Created by Polakovic Peter on 31/07/2024.
//  Copyright © 2024 CloudMakers, s. r. o. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <indigo/indigo_client.h>

int main(int argc, char **argv) {
	// the build passes the full path of the driver, which can be as long as the checkout location makes it
	char name[PATH_MAX];
	indigo_driver_info info;
	int entry = 0;
	for (int i = 1; i < argc; i++) {
		int length = snprintf(name, sizeof(name), "%s", argv[i]);
		if (length <= 0 || length >= (int)sizeof(name)) {
			fprintf(stderr, "Invalid driver path '%s'\n", argv[i]);
			continue;
		}
		int last = length - 1;
		if (name[last] == '/') {
			name[last] = 0;
		}
		indigo_driver_entry *driver;
		if (indigo_load_driver(name, false, &driver) == INDIGO_OK) {
			indigo_available_drivers[entry++].driver(INDIGO_DRIVER_INFO, &info);
			printf("\"%s\", \"%s\", %08x\n", info.name, info.description, info.version);
		}
	}
}
