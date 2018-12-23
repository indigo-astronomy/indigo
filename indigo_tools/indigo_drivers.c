//
//  indigo_drivers.c
//  IndigoApps
//
//  Created by Peter Polakovic on 09/02/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include "indigo_client.h"

int main(int argc, char **argv) {
	char state[32] = "stable";
	indigo_driver_info info;
	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<drivers>\n");
	int entry = 0;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			strcpy(state, "stable");
		} else if (!strcmp(argv[i], "-u")) {
			strcpy(state, "untested");
		} else if (!strcmp(argv[i], "-d")) {
			strcpy(state, "under development");
		} else {
			indigo_driver_entry *driver;
			if (indigo_load_driver(argv[i], false, &driver) == INDIGO_OK) {
				indigo_available_drivers[entry++].driver(INDIGO_DRIVER_INFO, &info);
				printf(" <driver name=\"%s\" version=\"%d.%d.%d.%d\" state=\"%s\">%s</driver>\n", info.name, INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MAJOR(info.version), INDIGO_VERSION_MINOR(info.version), state, info.description);
				//indigo_remove_driver(driver); // atik driver will fail here :(
			}
		}
	}
	printf("</drivers>\n");
}
