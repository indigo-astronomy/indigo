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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "indigo_bus.h"
#include "indigo_server_xml.h"

#include "ccd_simulator/indigo_ccd_simulator.h"
#include "ccd_sx/indigo_ccd_sx.h"
#include "ccd_ssag/indigo_ccd_ssag.h"
#include "ccd_asi/indigo_ccd_asi.h"
//#include "ccd_atik/indigo_ccd_atik.h"

#include "wheel_sx/indigo_wheel_sx.h"

void server_callback(int count) {
	INDIGO_LOG(indigo_log("%d clients", count));
}

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_start();
	
	indigo_ccd_simulator();
	indigo_ccd_sx();
	indigo_ccd_ssag();
	indigo_ccd_asi();
	//indigo_ccd_atik();

	indigo_wheel_sx();

	indigo_server_xml(server_callback);
	return 0;
}

