// Copyright (c) 2018 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO WeMacro Rail bluetooth driver
 \file indigo_focuser_wemacro_main_bt.c
 */

#include <stdio.h>
#include <string.h>

#include <indigo/indigo_driver_xml.h>

#include "indigo_focuser_wemacro_bt.h"

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(&indigo_stdin_handle, &indigo_stdout_handle);
	indigo_start();
	indigo_focuser_wemacro_bt(INDIGO_DRIVER_INIT, NULL);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(NULL, protocol_adapter);
	indigo_focuser_wemacro_bt(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return 0;
}

