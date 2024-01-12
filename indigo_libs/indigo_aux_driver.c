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

/** INDIGO AUX Driver base
 \file indigo_aux_driver.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <indigo/indigo_aux_driver.h>

indigo_result indigo_aux_attach(indigo_device *device, const char* driver_name, unsigned version, int interface) {
	assert(device != NULL);
	if (indigo_device_attach(device, driver_name, version, interface) == INDIGO_OK) {
		return INDIGO_OK;
	}
	return INDIGO_FAILED;
}

indigo_result indigo_aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	return indigo_device_enumerate_properties(device, client, property);
}

indigo_result indigo_aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(property != NULL);
	return indigo_device_change_property(device, client, property);
}

indigo_result indigo_aux_detach(indigo_device *device) {
	assert(device != NULL);
	return indigo_device_detach(device);
}

double indigo_aux_dewpoint(double temperature, double rh) {
	double ln_rh = log(rh/100.0);
	double a = (17.625 * temperature) / (243.04 + temperature);
	return (243.04 * (ln_rh + a)) / (17.625 - ln_rh - a);
}