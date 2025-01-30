// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
//
// Thanks to Gene Nolan and Leon Palmer for their support.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Rigel Systems nSTEP focuser driver main
 \file indigo_focuser_nstep_main.c
 */

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_focuser_nstep.h"

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(INDIGO_STDIN_HANDLE, INDIGO_STDOUT_HANDLE);
	indigo_start();
	indigo_focuser_nstep(INDIGO_DRIVER_INIT, NULL);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(NULL, protocol_adapter);
	indigo_focuser_nstep(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return 0;
}

