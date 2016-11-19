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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "indigo_client_xml.h"
#include "indigo_client.h"

indigo_driver_entry indigo_available_drivers[INDIGO_MAX_DRIVERS];
indigo_server_entry indigo_available_servers[INDIGO_MAX_SERVERS];

static int used_driver_slots = 0;
static int used_server_slots = 0;

static indigo_result add_driver(driver_entry_point driver, void *dl_handle) {
	int empty_slot = used_driver_slots; /* the first slot after the last used is a good candidate */
	for (int dc = 0; dc < used_driver_slots;  dc++) {
		if (indigo_available_drivers[dc].driver == driver) {
			INDIGO_LOG(indigo_log("Driver %s already loaded.", indigo_available_drivers[dc].name));
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
	INDIGO_LOG(indigo_log("Driver %s %d.%d.%d.%d loaded.", info.name, INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MAJOR(info.version), INDIGO_VERSION_MINOR(info.version)));

	if (empty_slot == used_driver_slots)
		used_driver_slots++; /* if we are not filling a gap - increase used_slots */

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
		INDIGO_LOG(indigo_log("Driver %s can't be loaded.", entry_point_name));
		return INDIGO_FAILED;
	}
	driver = dlsym(dl_handle, entry_point_name);
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		INDIGO_LOG(indigo_log("Can't load %s() (%s)", entry_point_name, dlsym_error));
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
	INDIGO_LOG(indigo_log("Driver %s unloaded.", indigo_available_drivers[dc].name));
	indigo_available_drivers[dc].description[0] = '\0';
	indigo_available_drivers[dc].name[0] = '\0';
	indigo_available_drivers[dc].driver = NULL;
	indigo_available_drivers[dc].dl_handle = NULL;
	return INDIGO_OK;
}

indigo_result indigo_remove_driver(driver_entry_point driver) {
	for (int dc = 0; dc < used_driver_slots; dc++)
		if (indigo_available_drivers[dc].driver == driver)
			return remove_driver(dc);
	return INDIGO_NOT_FOUND;
}

indigo_result indigo_unload_driver(const char *entry_point_name) {
	if (entry_point_name[0] == '\0') return INDIGO_OK;
	for (int dc = 0; dc < used_driver_slots; dc++)
		if (!strncmp(indigo_available_drivers[dc].name, entry_point_name, INDIGO_NAME_SIZE))
			return remove_driver(dc);
	INDIGO_LOG(indigo_log("Driver %s not found. Is it loaded?", entry_point_name));
	return INDIGO_NOT_FOUND;
}

void *server_thread(indigo_server_entry *server) {
	INDIGO_LOG(indigo_log("Server %s:%d thread started.", server->host, server->port));
	while (server->socket >= 0) {
		server->socket = 0;
		struct hostent *host_entry = gethostbyname(server->host);
		if (host_entry == NULL) {
			INDIGO_LOG(indigo_log("Can't resolve host name %s (%s)", server->host, strerror(errno)));
		} else if ((server->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			INDIGO_LOG(indigo_log("Can't create socket (%s)", strerror(errno)));
		} else {
			struct sockaddr_in serv_addr;
			memcpy(&serv_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(server->port);
			if (connect(server->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
				close(server->socket);
				server->socket = 0;
			}
		}
		if (server->socket > 0) {
			INDIGO_LOG(indigo_log("Server %s:%d connected.", server->host, server->port));
			server->protocol_adapter = indigo_xml_client_adapter(server->socket, server->socket);
			indigo_attach_device(server->protocol_adapter);
			indigo_xml_parse(server->socket, server->protocol_adapter, NULL);
			indigo_detach_device(server->protocol_adapter);
			free(server->protocol_adapter->device_context);
			free(server->protocol_adapter);
			close(server->socket);
			INDIGO_LOG(indigo_log("Server %s:%d disconnected.", server->host, server->port));
		} else {
			sleep(5);
		}
	}
	server->thread = NULL;
	INDIGO_LOG(indigo_log("Server %s:%d thread stopped.", server->host, server->port));
	return NULL;
}

indigo_result indigo_connect_server(const char *host, int port) {
	int empty_slot = used_server_slots;
	for (int dc = 0; dc < used_server_slots;  dc++) {
		if (indigo_available_servers[dc].thread_started && !strcmp(indigo_available_servers[dc].host, host) && indigo_available_servers[dc].port == port) {
			INDIGO_LOG(indigo_log("Server %s:%d already connected.", indigo_available_servers[dc].host, indigo_available_servers[dc].port));
			return INDIGO_OK;
		} else if (!indigo_available_servers[dc].thread_started) {
			empty_slot = dc;
		}
	}

	if (empty_slot > INDIGO_MAX_SERVERS)
		return INDIGO_TOO_MANY_ELEMENTS;

	strncpy(indigo_available_servers[empty_slot].host, host, INDIGO_NAME_SIZE);
	indigo_available_servers[empty_slot].port = port;
	indigo_available_servers[empty_slot].socket = 0;
	if (pthread_create(&indigo_available_servers[empty_slot].thread, NULL, (void*)(void *)server_thread, &indigo_available_servers[empty_slot]) != 0) {
		indigo_available_servers[empty_slot].thread_started = false;
		return INDIGO_FAILED;
	}
	indigo_available_servers[empty_slot].thread_started = true;

	if (empty_slot == used_server_slots)
		used_server_slots++;

	return INDIGO_OK;
}

indigo_result indigo_disconnect_server(const char *host, int port) {
	for (int dc = 0; dc < used_server_slots;  dc++) {
		if (indigo_available_servers[dc].thread_started && !strcmp(indigo_available_servers[dc].host, host) && indigo_available_servers[dc].port == port) {
			if (indigo_available_servers[dc].socket > 0)
				close(indigo_available_servers[dc].socket);
			indigo_available_servers[dc].socket = -1;
			indigo_available_servers[dc].thread_started = false;
			return INDIGO_OK;
		}
	}
	return INDIGO_NOT_FOUND;
}
