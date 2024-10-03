// Copyright (c) 2024 Rumen G. Bogdanovski
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
// 2.0 by Rumen Bogdanovski <rumenastro@gmail.com>

/** INDIGO USB-Serial Utilities
 \file indigo_usbserial_utils.h
 */

#ifndef indigo_usbserial_utils_h
#define indigo_usbserial_utils_h

#include <limits.h>

#define INFO_ITEM_SIZE 256
typedef struct {
	int vendor_id;
	int pproduct_id;
	char vendor_string[INFO_ITEM_SIZE];
	char product_string[INFO_ITEM_SIZE];
	char serial_string[INFO_ITEM_SIZE];
	char path[PATH_MAX];
} indigo_serial_info;

#ifdef __cplusplus
extern "C" {
#endif

extern int indigo_enumerate_usbserial_devices(indigo_serial_info *serial_info, int num_serial_info);
extern void indigo_usbserial_label(indigo_serial_info *serial_info, const int num_serial_info, const char *device_path, char *label);

#ifdef __cplusplus
}
#endif

#endif /* indigo_usbserial_utils_h */
