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

/** INDIGO USB Utilities
 \file indigo_usb_utils.c
 */

#include <string.h>

#include "indigo_bus.h"
#include "indigo_usb_utils.h"

indigo_result indigo_get_usb_path(libusb_device* handle, char *path) {
	uint8_t data[10];
	char buf[10];
	int i;
	
	data[0] = libusb_get_bus_number(handle);
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
