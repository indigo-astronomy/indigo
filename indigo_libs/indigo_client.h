// Copyright (c) 2016 CloudMakers, s. r. o.
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
// 2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO client
 \file indigo_client.h
 */

#ifndef indigo_client_h
#define indigo_client_h

#include "indigo_bus.h"
#include "indigo_driver.h"

#define INDIGO_MAX_DRIVERS	100

typedef struct {
	char description[INDIGO_NAME_SIZE];
	char name[INDIGO_NAME_SIZE];
	driver_entry_point driver;
	void *dl_handle;
} indigo_driver_entry;

extern indigo_driver_entry indigo_available_drivers[INDIGO_MAX_DRIVERS];

extern indigo_result indigo_add_driver(driver_entry_point driver);
extern indigo_result indigo_remove_driver(driver_entry_point driver);

extern indigo_result indigo_load_driver(const char *name);
extern indigo_result indigo_unload_driver(const char *name);

extern indigo_result indigo_connect_server(const char *name);
extern indigo_result indigo_disconnect_server(const char *name);

#endif /* indigo_client_h */
