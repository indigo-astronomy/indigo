// Copyright (c) 2025 CloudMakers, s. r. o.
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

#include <stdio.h>
#include <indigo/indigo_client.h>

static bool driver_filter(const char* name) {
	size_t len = strlen(name);
	return strncmp(name, "indigo_", 7) == 0 && strcmp(name + len - 4, ".dll") == 0;
}

int main(int argc, char** argv) {
	indigo_driver_info info;
	int entry = 0;
	char **list;
	int count = indigo_uni_scandir(".", &list, driver_filter);
	indigo_uni_handle* out = indigo_uni_create_file("indigo_drivers", INDIGO_LOG_PLAIN);
	for (int i = 0; i < count; i++) {
		indigo_driver_entry* driver;
		// a driver that can't be loaded, e.g. without its SDK DLL, is left out of the list
		if (indigo_load_driver(list[i], false, &driver) == INDIGO_OK) {
			indigo_available_drivers[entry++].driver(INDIGO_DRIVER_INFO, &info);
			indigo_uni_printf(out, "\"%s\", \"%s\", %08x\n", info.name, info.description, info.version);
		}
	}
	indigo_uni_close(&out);
}
