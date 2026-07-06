/** INDIGO MX-HD mount driver main
 \file indigo_mount_mxhd_main.c
 */

#include <stdio.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_mount_mxhd.h"

int main(int argc, const char *argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(0, 1);
	indigo_start();
	indigo_mount_mxhd(INDIGO_DRIVER_INIT, NULL);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(NULL, protocol_adapter);
	indigo_mount_mxhd(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return 0;
}
