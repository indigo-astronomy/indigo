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
 \file indigo_client.c
 */

#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <dlfcn.h>

#include "indigo_client.h"

indigo_driver_entry indigo_available_drivers[INDIGO_MAX_DRIVERS];

static int used_slots = 0;

static indigo_result add_driver(driver_entry_point driver, void *dl_handle) {
	int empty_slot = used_slots; /* the first slot after the last used is a good candidate */
	for (int dc = 0; dc < used_slots;  dc++) {
		if (indigo_available_drivers[dc].driver == driver) {
			INDIGO_LOG(indigo_log("Driver '%s' already loaded.", indigo_available_drivers[dc].name));
			if (dl_handle != NULL)
				dlclose(dl_handle);
			return INDIGO_OK;
		} else if (indigo_available_drivers[dc].driver == NULL) {
			empty_slot = dc; /* if there is a gap - fill it */
		}
	}
	
	if (empty_slot > INDIGO_MAX_DRIVERS) {
		if (dl_handle != NULL)
			dlclose(dl_handle);
		return INDIGO_TOO_MANY_ELEMENTS; /* no emty slot found, list is full */
	}
	
	indigo_driver_info info;
	driver(INDIGO_DRIVER_INFO, &info);
	strncpy(indigo_available_drivers[empty_slot].description, info.description, INDIGO_NAME_SIZE); //TO BE CHANGED - DRIVER SHOULD REPORT NAME!!!
	strncpy(indigo_available_drivers[empty_slot].name, info.name, INDIGO_NAME_SIZE);
	indigo_available_drivers[empty_slot].driver = driver;
	indigo_available_drivers[empty_slot].dl_handle = dl_handle;
	INDIGO_LOG(indigo_log("Driver %s v.%d.%02d loaded.", info.name, DRIVER_VERSION_MAJOR(info.version), DRIVER_VERSION_MINOR(info.version)));
	
	if (empty_slot == used_slots)
		used_slots++; /* if we are not filling a gap - increase used_slots */
	
	return INDIGO_OK;
}

indigo_result indigo_add_driver(driver_entry_point driver) {
	return add_driver(driver, NULL);
}

indigo_result indigo_load_driver(const char *name) {
	char driver_name[INDIGO_NAME_SIZE];
	char *entry_point_name, *cp;
	void *dl_handle;
	driver_entry_point driver;	
	strncpy(driver_name, name, sizeof(driver_name));
	entry_point_name = basename(driver_name);
	cp = strchr(entry_point_name, '.');
	if (cp) *cp = '\0';
	dl_handle = dlopen(name, RTLD_LAZY);
	if (!dl_handle) {
		INDIGO_LOG(indigo_log("Driver %s can not be loaded.", entry_point_name));
		return INDIGO_FAILED;
	}
	driver = dlsym(dl_handle, entry_point_name);
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		INDIGO_LOG(indigo_log("Cannot load %s(): %s", entry_point_name, dlsym_error));
		dlclose(dl_handle);
		return INDIGO_NOT_FOUND;
	}
	return add_driver(driver, dl_handle);
}

indigo_result remove_driver(int dc) {
	indigo_available_drivers[dc].driver(INDIGO_DRIVER_SHUTDOWN, NULL); /* deregister */
	if (indigo_available_drivers[dc].dl_handle) {
		dlclose(indigo_available_drivers[dc].dl_handle);
	}
	INDIGO_LOG(indigo_log("Driver %s removed.", indigo_available_drivers[dc].name));
	indigo_available_drivers[dc].description[0] = '\0';
	indigo_available_drivers[dc].name[0] = '\0';
	indigo_available_drivers[dc].driver = NULL;
	indigo_available_drivers[dc].dl_handle = NULL;
	return INDIGO_OK;
}

indigo_result indigo_remove_driver(driver_entry_point driver) {
	for (int dc = 0; dc < used_slots; dc++)
		if (indigo_available_drivers[dc].driver == driver)
			return remove_driver(dc);
	return INDIGO_NOT_FOUND;
}


indigo_result indigo_unload_driver(const char *entry_point_name) {
	if (entry_point_name[0] == '\0') return INDIGO_OK;
	for (int dc = 0; dc < used_slots; dc++)
		if (!strncmp(indigo_available_drivers[dc].name, entry_point_name, INDIGO_NAME_SIZE))
			return remove_driver(dc);
	INDIGO_LOG(indigo_log("Driver %s not found. Is it loaded?", entry_point_name));
	return INDIGO_NOT_FOUND;
}
