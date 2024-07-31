//
//  indigo_driver_metadata.c
//  indigo
//
//  Created by Polakovic Peter on 31/07/2024.
//  Copyright © 2024 CloudMakers, s. r. o. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_client.h>

int main(int argc, char **argv) {
	char name[128];
	indigo_driver_info info;
	int entry = 0;
	for (int i = 1; i < argc; i++) {
		strcpy(name, argv[i]);
		int last = strlen(name) - 1;
		if (name[last] == '/')
			name[last] = 0;
		indigo_driver_entry *driver;
		if (indigo_load_driver(name, false, &driver) == INDIGO_OK) {
			indigo_available_drivers[entry++].driver(INDIGO_DRIVER_INFO, &info);
			printf("\"%s\", \"%s\", %04x\n", info.name, info.description, info.version);
		}
	}
}
