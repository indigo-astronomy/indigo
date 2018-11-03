//
//  make_list.c
//  IndigoApps
//
//  Created by Peter Polakovic on 09/02/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include "indigo_client.h"

void group(int argc, char **argv, char *group, char *prefix) {
	indigo_driver_info info;
	printf(" <devGroup group=\"%s\">\n", group);
	for (int i = 1; i < argc; i++) {
		if (strstr(argv[i], prefix)) {
			indigo_driver_entry *driver;
			indigo_load_driver(argv[i], false, &driver);
			indigo_available_drivers[0].driver(INDIGO_DRIVER_INFO, &info);
			printf("  <device label=\"%s\">\n", info.description);
			printf("   <driver name=\"%s\"%s>%s</driver>\n", info.description, info.multi_device_support ? " mdpd=\"true\"" : "",  info.name);
			printf("   <version>%d.%d</version>\n", INDIGO_VERSION_MAJOR(info.version), INDIGO_VERSION_MINOR(info.version));
			printf("  </device>\n");
			indigo_remove_driver(driver);
		}
	}
	printf(" </devGroup>\n");
}

void version(char *name) {
	indigo_driver_info info;
	indigo_driver_entry *driver;
	indigo_load_driver(name, false, &driver);
	indigo_available_drivers[0].driver(INDIGO_DRIVER_INFO, &info);
	printf("%d.%d", INDIGO_VERSION_MAJOR(info.version), INDIGO_VERSION_MINOR(info.version));
	indigo_remove_driver(driver);
}


int main(int argc, char **argv) {
	if (argc == 3 && !strcmp(argv[1], "-v")) {
		version(argv[2]);
	} else {
		printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		printf("<driversList>\n");
		group(argc, argv, "Telescopes", "indigo_mount_");
		group(argc, argv, "CCDs", "indigo_ccd_");
		group(argc, argv, "Focusers", "indigo_focuser_");
		group(argc, argv, "Filter Wheels", "indigo_wheel_");
		group(argc, argv, "Domes", "indigo_dome_");
		group(argc, argv, "Auxiliary", "indigo_aux_");
		printf("</driversList>\n");
	}
}
