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

#define TOOL_VERSION "0.1"

#include <stdio.h>
#include <indigo/indigo_usbserial_utils.h>

static void print_help(const char *name) {
	printf("INDIGO tool to list USB-Serial devices v.%s built on %s %s.\n", TOOL_VERSION, __DATE__, __TIME__);
	printf("usage: %s [options]\n", name);
	printf("options:\n"
	       "       -h  | --help\n"
	);
}

int main(int argc, const char * argv[]) {

    if (argc != 1) {
        print_help(argv[0]);
        return 0;
    }

    const int num_serial_info = 256;
    indigo_serial_info serial_info[num_serial_info];
    int count = indigo_enumerate_usbserial_devices(serial_info, num_serial_info);
    if (count < 0) {
        printf("Failed to list USB serial devices\n");
        return 1;
    } else if (count == 0) {
        printf("No USB serial devices found\n");
        return 0;
    }

    printf("\n");
    for (int i = 0; i < count; i++) {
        printf("Device path      : %s\n", serial_info[i].path);
        printf("  Vendor ID      : 0x%04X\n", serial_info[i].vendor_id);
        printf("  Product ID     : 0x%04X\n", serial_info[i].product_id);
        printf("  Vendor string  : \"%s\"\n", serial_info[i].vendor_string);
        printf("  Product string : \"%s\"\n", serial_info[i].product_string);
        printf("  Serial #       : %s\n", serial_info[i].serial_string);
        printf("\n");
    }
    return 0;
}
