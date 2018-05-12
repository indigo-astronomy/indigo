//
//  indigo_usb_utils.c
//  indigo
//
//  Created by Peter Polakovic on 12/05/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#include <string.h>

#include "indigo_bus.h"
#include "indigo_usb_utils.h"

indigo_result indigo_get_usb_path(libusb_device* handle, char *path) {
	uint8_t data[10];
	char buf[10];
	int i;
	
	data[0]=libusb_get_bus_number(handle);
	int n = libusb_get_port_numbers(handle, &data[1], 9);
	if (n != LIBUSB_ERROR_OVERFLOW) {
		sprintf(path,"%X", data[0]);
		for (i = 1; i <= n; i++) {
			sprintf(buf, "%X", data[i]);
			strcat(path, ".");
			strcat(path, buf);
		}
	} else {
		path[0] = '\0';
		return INDIGO_FAILED;
	}
	return INDIGO_OK;
}
