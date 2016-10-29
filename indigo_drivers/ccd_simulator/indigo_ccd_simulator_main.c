//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO CCD Simulator driver main
 \file indigo_ccd_simulator_main.c
 */

#include <stdio.h>

#include "indigo_driver_xml.h"

#include "indigo_ccd_simulator.h"

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(0, 1);
	indigo_start();
	indigo_ccd_simulator(true);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(0, NULL, protocol_adapter);
	indigo_ccd_simulator(false);
	indigo_stop();
	return 0;
}

