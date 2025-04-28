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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO client
 \file indigo_client.c
 */

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <libgen.h>
#include <dlfcn.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#if defined(INDIGO_MACOS)
#include <CoreFoundation/CoreFoundation.h>
#elif defined(INDIGO_LINUX)
#include <unistd.h>
#elif defined(INDIGO_WINDOWS)
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning(disable:4996)
#endif

#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_client_xml.h>
#include <indigo/indigo_client.h>

char *indigo_client_name = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#if defined(INDIGO_MACOS)
#define SO_NAME ".dylib"
#elif defined(INDIGO_LINUX)
#define SO_NAME ".so"
#elif defined(INDIGO_WINDOWS)
#define SO_NAME ".dll"
#endif

static int used_driver_slots = 0;

indigo_driver_entry indigo_available_drivers[INDIGO_MAX_DRIVERS];

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#define INDIGO_DL_HANDLE	void *
#elif defined(INDIGO_WINDOWS)
#define INDIGO_DL_HANDLE	HMODULE
#endif

static INDIGO_DL_HANDLE load_library(const char* so_name) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	INDIGO_DL_HANDLE dl_handle = dlopen(so_name, RTLD_LAZY);
	if (dl_handle == NULL) {
		indigo_error("Driver %s can't be loaded (%s)", so_name, dlerror());
		return NULL;
	}
	return dl_handle;
#elif defined(INDIGO_WINDOWS)
	INDIGO_DL_HANDLE dl_handle = LoadLibraryA(so_name);
	if (dl_handle == NULL) {
		indigo_error("Driver %s can't be loaded (%s)", so_name, indigo_last_windows_error());
		return NULL;
	}
	return dl_handle;
#else
#pragma message ("TODO: load_library()")
#endif
}

static driver_entry_point get_entry_point(INDIGO_DL_HANDLE dl_handle, const char* entry_point_name) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	driver_entry_point entry_point = dlsym(dl_handle, entry_point_name);
	if (entry_point == NULL) {
		INDIGO_ERROR(indigo_error("Can't load %s() (%s)", entry_point_name, dlerror()));
		dlclose(dl_handle);
		return NULL;
	}
	return entry_point;
#elif defined(INDIGO_WINDOWS)
	driver_entry_point entry_point = (driver_entry_point)GetProcAddress(dl_handle, entry_point_name);;
	if (entry_point == NULL) {
		INDIGO_ERROR(indigo_error("Can't load %s() (%s)", entry_point_name, indigo_last_windows_error()));
		return NULL;
	}
	return entry_point;
#else
#pragma message ("TODO: get_entry_point()")
#endif
}

void unload_library(INDIGO_DL_HANDLE dl_handle) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	dlclose(dl_handle);
#elif defined(INDIGO_WINDOWS)
	FreeLibrary(dl_handle);
#else
#pragma message ("TODO: get_entry_point()")
#endif
}

bool indigo_driver_initialized(char *driver_name) {
	assert(driver_name != NULL);
	for (int dc = 0; dc < used_driver_slots;  dc++) {
		if (!strncmp(indigo_available_drivers[dc].name, driver_name, INDIGO_NAME_SIZE) &&
		    indigo_available_drivers[dc].initialized) {
			INDIGO_DEBUG(indigo_debug("Looked up driver %s is initialized", driver_name));
			return true;
		}
	}
	INDIGO_DEBUG(indigo_debug("Looked up driver %s is NOT initialized", driver_name));
	return false;
}

static indigo_result add_driver(driver_entry_point entry_point, void *dl_handle, bool init, indigo_driver_entry **driver) {
	pthread_mutex_lock(&mutex);
	int empty_slot = used_driver_slots; /* the first slot after the last used is a good candidate */
	for (int dc = 0; dc < used_driver_slots;  dc++) {
		if (indigo_available_drivers[dc].driver == entry_point) {
			INDIGO_LOG(indigo_log("Driver %s already loaded", indigo_available_drivers[dc].name));
			if (dl_handle != NULL) {
				unload_library(dl_handle);
			}
			if (driver != NULL) {
				*driver = &indigo_available_drivers[dc];
			}
			pthread_mutex_unlock(&mutex);
			return INDIGO_DUPLICATED;
		} else if (indigo_available_drivers[dc].driver == NULL) {
			empty_slot = dc; /* if there is a gap - fill it */
		}
	}
	if (empty_slot > INDIGO_MAX_DRIVERS) {
		if (dl_handle != NULL) {
			unload_library(dl_handle);
		}
		pthread_mutex_unlock(&mutex);
		indigo_error("[%s:%d] Max driver count reached", __FUNCTION__, __LINE__);
		return INDIGO_TOO_MANY_ELEMENTS; /* no emty slot found, list is full */
	}
	indigo_driver_info info;
	entry_point(INDIGO_DRIVER_INFO, &info);
	indigo_copy_name(indigo_available_drivers[empty_slot].description, info.description); //TO BE CHANGED - DRIVER SHOULD REPORT NAME!!!
	indigo_copy_name(indigo_available_drivers[empty_slot].name, info.name);
	indigo_available_drivers[empty_slot].driver = entry_point;
	indigo_available_drivers[empty_slot].dl_handle = dl_handle;
	INDIGO_LOG(indigo_log("Driver %s %d.%d.%d.%d loaded", info.name, INDIGO_VERSION_MAJOR(info.version >> 16), INDIGO_VERSION_MINOR(info.version >> 16), INDIGO_VERSION_MAJOR(info.version), INDIGO_VERSION_MINOR(info.version)));
	if (empty_slot == used_driver_slots) {
		used_driver_slots++;
	} /* if we are not filling a gap - increase used_slots */
	pthread_mutex_unlock(&mutex);
	if (driver != NULL) {
		*driver = &indigo_available_drivers[empty_slot];
	}
	if (init) {
		int result = entry_point(INDIGO_DRIVER_INIT, NULL);
		indigo_available_drivers[empty_slot].initialized = result == INDIGO_OK;
		if (result != INDIGO_OK) {
			indigo_error("Driver %s failed to initialise", info.name);
		}
		return result;
	}
	return INDIGO_OK;
}

indigo_result indigo_remove_driver(indigo_driver_entry *driver) {
	assert(driver != NULL);
	indigo_result result;
	pthread_mutex_lock(&mutex);
	result = driver->driver(INDIGO_DRIVER_SHUTDOWN, NULL); /* deregister */
	if (result != INDIGO_OK) {
		if (result == INDIGO_BUSY) {
			INDIGO_LOG(indigo_log("Driver %s is in use and can't be unloaded", driver->name));
		} else {
			INDIGO_LOG(indigo_log("Driver %s failed to unload", driver->name));
		}
		pthread_mutex_unlock(&mutex);
		return result;
	}
	if (driver->dl_handle) {
		unload_library(driver->dl_handle);
	}
	INDIGO_LOG(indigo_log("Driver %s unloaded", driver->name));
	driver->description[0] = '\0';
	driver->name[0] = '\0';
	driver->driver = NULL;
	driver->dl_handle = NULL;
	pthread_mutex_unlock(&mutex);
	return INDIGO_OK;
}

indigo_result indigo_add_driver(driver_entry_point entry_point, bool init, indigo_driver_entry **driver) {
	return add_driver(entry_point, NULL, init, driver);
}

indigo_result indigo_load_driver(const char *name, bool init, indigo_driver_entry **driver) {
	char driver_name[INDIGO_NAME_SIZE];
	char so_name[INDIGO_NAME_SIZE];
	char *entry_point_name, *cp;
	strncpy(driver_name, name, sizeof(driver_name));
	strncpy(so_name, name, sizeof(so_name));
	entry_point_name = indigo_uni_basename(driver_name);
	cp = strchr(entry_point_name, '.');
	if (cp) {
		*cp = '\0';
	} else {
		strncat(so_name, SO_NAME, INDIGO_NAME_SIZE);
	}
	INDIGO_DL_HANDLE dl_handle = load_library(so_name);
	if (dl_handle == NULL) {
		return INDIGO_FAILED;
	}
	driver_entry_point entry_point = get_entry_point(dl_handle, entry_point_name);
	if (entry_point == NULL) {
		unload_library(dl_handle);
		return INDIGO_NOT_FOUND;
	}
	return add_driver(entry_point, dl_handle, init, driver);
}

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

static int used_subprocess_slots = 0;
indigo_subprocess_entry indigo_available_subprocesses[INDIGO_MAX_SERVERS];

static void *subprocess_thread(indigo_subprocess_entry *subprocess) {
	INDIGO_LOG(indigo_log("Subprocess %s thread started", subprocess->executable));
	pthread_detach(pthread_self());
	int sleep_interval = 5;
	while (subprocess->pid >= 0) {
		int input[2], output[2];
		indigo_uni_handle *in = indigo_uni_create_file_handle(input[0], INDIGO_LOG_TRACE);
		indigo_uni_handle *out = indigo_uni_create_file_handle(output[0], INDIGO_LOG_TRACE);
		if (pipe(input) < 0 || pipe(output) < 0) {
			INDIGO_ERROR(indigo_error("Can't create local pipe for subprocess %s (%s)", subprocess->executable, strerror(errno)));
			strncpy(subprocess->last_error, strerror(errno), sizeof(subprocess->last_error));
			return NULL;
		}
		subprocess->pid = fork();
		if (subprocess->pid == -1) {
			INDIGO_ERROR(indigo_error("Can't create subprocess %s (%s)", subprocess->executable, strerror(errno)));
			strncpy(subprocess->last_error, strerror(errno), sizeof(subprocess->last_error));
		} else if (subprocess->pid == 0) {
			close(0);
			dup2(output[0], 0);
			close(1);
			dup2(input[1], 1);
			execlp(subprocess->executable, subprocess->executable, NULL);
			INDIGO_ERROR(indigo_error("Can't execute driver %s (%s)", subprocess->executable, strerror(errno)));
			exit(0);
		} else {
			close(input[1]);
			close(output[0]);
			char *slash = strrchr(subprocess->executable, '/');
			subprocess->protocol_adapter = indigo_xml_client_adapter(slash ? slash + 1 : subprocess->executable, "", &in, &out);
			indigo_attach_device(subprocess->protocol_adapter);
			indigo_xml_parse(subprocess->protocol_adapter, NULL);
			indigo_detach_device(subprocess->protocol_adapter);
			indigo_release_xml_client_adapter(subprocess->protocol_adapter);
		}
		if (subprocess->pid >= 0) {
			 indigo_usleep(sleep_interval * 1000000);
			if (sleep_interval < 60) {
				sleep_interval *= 2;
			}
		} else {
			sleep_interval = 5;
		}
	}
	subprocess->thread_started = false;
	INDIGO_LOG(indigo_log("Subprocess %s thread stopped", subprocess->executable));
	return NULL;
}

indigo_result indigo_start_subprocess(const char *executable, indigo_subprocess_entry **subprocess) {
	pthread_mutex_lock(&mutex);
	int empty_slot = used_subprocess_slots;
	for (int dc = 0; dc < used_subprocess_slots;  dc++) {
		if (indigo_available_subprocesses[dc].thread_started && !strcmp(indigo_available_subprocesses[dc].executable, executable)) {
			INDIGO_LOG(indigo_log("Subprocess %s already started", indigo_available_subprocesses[dc].executable));
			if (subprocess != NULL) {
				*subprocess = &indigo_available_subprocesses[dc];
			}
			pthread_mutex_unlock(&mutex);
			return INDIGO_DUPLICATED;
		}
	}
	for (int dc = 0; dc < used_subprocess_slots;  dc++) {
		if (!indigo_available_subprocesses[dc].thread_started) {
			empty_slot = dc;
			break;
		}
	}
	
	if (empty_slot > INDIGO_MAX_SERVERS) {
		pthread_mutex_unlock(&mutex);
		indigo_error("[%s:%d] Max subprocess count reached", __FUNCTION__, __LINE__);
		return INDIGO_TOO_MANY_ELEMENTS;
	}
	
	indigo_copy_name(indigo_available_subprocesses[empty_slot].executable, executable);
	indigo_available_subprocesses[empty_slot].pid = 0;
	*indigo_available_subprocesses[empty_slot].last_error = 0;
	if (pthread_create(&indigo_available_subprocesses[empty_slot].thread, NULL, (void*)(void *)subprocess_thread, &indigo_available_subprocesses[empty_slot]) != 0) {
		indigo_available_subprocesses[empty_slot].thread_started = false;
		pthread_mutex_unlock(&mutex);
		return INDIGO_FAILED;
	}
	indigo_available_subprocesses[empty_slot].thread_started = true;
	if (empty_slot == used_subprocess_slots) {
		used_subprocess_slots++;
	}
	pthread_mutex_unlock(&mutex);
	if (subprocess != NULL) {
		*subprocess = &indigo_available_subprocesses[empty_slot];
	}
	return INDIGO_OK;
}

indigo_result indigo_kill_subprocess(indigo_subprocess_entry *subprocess) {
	assert(subprocess != NULL);
	pthread_mutex_lock(&mutex);
	if (subprocess->pid > 0) {
		kill(subprocess->pid, SIGKILL);
	}
	subprocess->pid = -1;
	subprocess->thread_started = false;
	pthread_mutex_unlock(&mutex);
	return INDIGO_OK;
}

#endif

static int used_server_slots = 0;
indigo_server_entry indigo_available_servers[INDIGO_MAX_SERVERS];

void indigo_service_name(const char *host, int port, char *name) {
	indigo_copy_name(name, host);
	char *dot;
	if ((dot = strchr(name, '.'))) {
		*dot = 0;
	}
	if (port != 7624) {
		sprintf(name + strlen(name), ":%d", port);
	}
}

static void *server_thread(indigo_server_entry *server) {
	INDIGO_LOG(indigo_log("Server %s:%d thread started", server->host, server->port));
	pthread_detach(pthread_self());
	while (!server->shutdown) {
		server->handle = indigo_uni_client_tcp_socket(server->host, server->port, INDIGO_LOG_TRACE);
		if (server->handle != NULL) {
			if (*server->name == 0) {
				indigo_service_name(server->host, server->port, server->name);
			}
			char  url[INDIGO_NAME_SIZE];
			snprintf(url, sizeof(url), "http://%s:%d", server->host, server->port);
			INDIGO_LOG(indigo_log("Server %s:%d (%s, %s) connected", server->host, server->port, server->name, url));
#if defined(INDIGO_WINDOWS)
			indigo_send_message(server->protocol_adapter, "connected");
#endif
			server->protocol_adapter = indigo_xml_client_adapter(server->name, url, &server->handle, &server->handle);
			indigo_attach_device(server->protocol_adapter);
			indigo_xml_parse(server->protocol_adapter, NULL);
			indigo_detach_device(server->protocol_adapter);
			indigo_release_xml_client_adapter(server->protocol_adapter);
			server->protocol_adapter = NULL;
			indigo_uni_close(&server->handle);
			INDIGO_LOG(indigo_log("Server %s:%d disconnected", server->host, server->port));
#if defined(INDIGO_WINDOWS)
			indigo_send_message(server->protocol_adapter, "disconnected");
#endif
			int timeout = 10;
			while (!server->shutdown && timeout--) {
				indigo_usleep(100000);
			}
		} else {
			int timeout = 50;
			while (!server->shutdown && timeout--) {
				indigo_usleep(100000);
			}
		}
	}
	server->thread_started = false;
	INDIGO_LOG(indigo_log("Server %s:%d thread stopped", server->host, server->port));
	return NULL;
}

indigo_result indigo_connect_server(const char *name, const char *host, int port, indigo_server_entry **server) {
	return indigo_connect_server_id(name, host, port, 0, server);
}

indigo_result indigo_connect_server_id(const char *name, const char *host, int port, uint32_t connection_id, indigo_server_entry **server) {
	pthread_mutex_lock(&mutex);
	int empty_slot = used_server_slots;
	for (int dc = 0; dc < used_server_slots; dc++) {
		if (indigo_available_servers[dc].thread_started && !strcmp(indigo_available_servers[dc].host, host) && indigo_available_servers[dc].port == port && indigo_available_servers[dc].connection_id == connection_id) {
			INDIGO_LOG(indigo_log("Server %s:%d already connected (id=%d)", indigo_available_servers[dc].host, indigo_available_servers[dc].port, indigo_available_servers[dc].connection_id));
			if (server != NULL) {
				*server = &indigo_available_servers[dc];
			}
			pthread_mutex_unlock(&mutex);
			return INDIGO_DUPLICATED;
		}
	}
	for (int dc = 0; dc < used_server_slots; dc++) {
		if (!indigo_available_servers[dc].thread_started) {
			empty_slot = dc;
		}
	}
	if (empty_slot > INDIGO_MAX_SERVERS) {
		pthread_mutex_unlock(&mutex);
		indigo_error("[%s:%d] Max server count reached", __FUNCTION__, __LINE__);
		return INDIGO_TOO_MANY_ELEMENTS;
	}
	if (name != NULL) {
		indigo_copy_name(indigo_available_servers[empty_slot].name, name);
	} else {
		*indigo_available_servers[empty_slot].name = 0;
	}
	indigo_copy_name(indigo_available_servers[empty_slot].host, host);
	indigo_available_servers[empty_slot].port = port;
	indigo_available_servers[empty_slot].handle = NULL;
	indigo_available_servers[empty_slot].connection_id = connection_id;
	indigo_available_servers[empty_slot].shutdown = false;
	if (pthread_create(&indigo_available_servers[empty_slot].thread, NULL, (void*) (void *) server_thread, &indigo_available_servers[empty_slot]) != 0) {
		pthread_mutex_unlock(&mutex);
		return INDIGO_FAILED;
	}
	indigo_available_servers[empty_slot].thread_started = true;
	if (empty_slot == used_server_slots) {
		used_server_slots++;
	}
	pthread_mutex_unlock(&mutex);
	if (server != NULL) {
		*server = &indigo_available_servers[empty_slot];
	}
	return INDIGO_OK;
}

bool indigo_connection_status(indigo_server_entry *server, char *last_error) {
	pthread_mutex_lock(&mutex);
	if (server != NULL && server->handle != NULL && last_error != NULL) {
		strcpy(last_error, indigo_uni_strerror(server->handle));
	} else if (last_error != NULL) {
		*last_error = 0;
	}
	pthread_mutex_unlock(&mutex);
	return server != NULL && server->handle != NULL;
}

indigo_result indigo_disconnect_server(indigo_server_entry *server) {
	assert(server != NULL);
	pthread_mutex_lock(&mutex);
	if (server->handle != NULL) {
		indigo_uni_close(&server->handle);
	}
	server->shutdown = true;
	bool thread_runing = server->thread_started;
	pthread_mutex_unlock(&mutex);
	int timeout = 5;
	while (thread_runing && timeout--) {
		pthread_mutex_lock(&mutex);
		thread_runing = server->thread_started;
		pthread_mutex_unlock(&mutex);
		indigo_usleep(100000);
	}
	return INDIGO_OK;
}

indigo_result indigo_format_number(char *buffer, int buffer_size, char *format, double value) {
	int format_length = (int)strlen(format);
	if (!strcmp(format + format_length - 3, "10m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%02d:%06.3f"), buffer_size);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "9m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%02d:%05.2f"), buffer_size);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "8m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%02d:%04.1f"), buffer_size);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "6m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%02d:%02d"), buffer_size);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "5m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%04.1f"), buffer_size);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "3m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%02d"), buffer_size);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 1, "m")) {
		strncpy(buffer, indigo_dtos(value, "%d:%02d:%04.1f"), buffer_size);
		return INDIGO_OK;
	} else {
		return snprintf(buffer, buffer_size, format, value) == 1 ? INDIGO_OK : INDIGO_FAILED;
	}
	return INDIGO_FAILED;
}
