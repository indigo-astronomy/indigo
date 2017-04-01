// Copyright (c) 2017 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen Bogdanovski <rumen@skyarchive.org>

/** INDIGO CCD SBIG Ethernrt driver main
 \file indigo_ccd_sbig_eth_main.c
 */

#include <stdio.h>
#include "indigo_driver_xml.h"

#include "indigo_ccd_sbig_eth.h"

int main(int argc, const char * argv[]) {
	indigo_main_argc = argc;
	indigo_main_argv = argv;
	indigo_client *protocol_adapter = indigo_xml_device_adapter(0, 1);
	indigo_start();
	indigo_ccd_sbig_eth(INDIGO_DRIVER_INIT, NULL);
	indigo_attach_client(protocol_adapter);
	indigo_xml_parse(NULL, protocol_adapter);
	indigo_ccd_sbig_eth(INDIGO_DRIVER_SHUTDOWN, NULL);
	indigo_stop();
	return 0;
}
