// Copyright (c) 2021 Rumen G. Bogdanovski
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
// 2.0 by Rumen G.Bogdanovski <rumen@skyarchive.org>

/** INDIGO ASCOM ALPACA bridge agent
 \file alpaca_switch.c
 */

#include <indigo/indigo_aux_driver.h>

#include "alpaca_common.h"

static indigo_alpaca_error alpaca_get_interfaceversion(indigo_alpaca_device *device, int version, uint32_t *value) {
	*value = 1;
	return indigo_alpaca_error_OK;
}

void indigo_alpaca_switch_update_property(indigo_alpaca_device *alpaca_device, indigo_property *property) {
	
}

long indigo_alpaca_switch_get_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length) {
	if (!strcmp(command, "supportedactions")) {
		return snprintf(buffer, buffer_length, "\"Value\": [ ], \"ErrorNumber\": 0, \"ErrorMessage\": \"\"");
	}
	if (!strcmp(command, "interfaceversion")) {
		uint32_t value;
		indigo_alpaca_error result = alpaca_get_interfaceversion(alpaca_device, version, &value);
	return indigo_alpaca_append_value_int(buffer, buffer_length, value, result);
	}
	return 0;
}

long indigo_alpaca_switch_set_command(indigo_alpaca_device *alpaca_device, int version, char *command, char *buffer, long buffer_length, char *param_1, char *param_2) {
	return 0;
}
