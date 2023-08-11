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

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef INDIGO_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif
#if defined(INDIGO_WINDOWS)
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning(disable:4996)
#endif

#include <indigo/indigo_client_xml.h>
#include <indigo/indigo_client.h>

char *indigo_client_name = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#if defined(INDIGO_WINDOWS)
static bool is_pre_vista() {
	char buffer[MAX_PATH];
	HKEY hKey;
	DWORD dwBufLen;
	LONG lret;
	HRESULT hr = E_FAIL;
	DWORD* reservedNULL = NULL;
	DWORD reservedZero = 0;
	bool bRet = false;
	lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", reservedZero, KEY_READ, &hKey);
	if (lret == ERROR_SUCCESS) {
		dwBufLen = MAX_PATH;
		lret = RegQueryValueEx(hKey, "CurrentVersion", reservedNULL, NULL, (BYTE*)buffer, &dwBufLen);
		if (lret == ERROR_SUCCESS) {
			if (indigo_atod(buffer) < 6.0) // Vista is 6.0
				bRet = true;
		}
		RegCloseKey(hKey);
	}
	return bRet;
}
#endif

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

static int used_driver_slots = 0;
static int used_subprocess_slots = 0;


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
			if (dl_handle != NULL)
				dlclose(dl_handle);
			if (driver != NULL)
				*driver = &indigo_available_drivers[dc];
			pthread_mutex_unlock(&mutex);
			return INDIGO_DUPLICATED;
		} else if (indigo_available_drivers[dc].driver == NULL) {
			empty_slot = dc; /* if there is a gap - fill it */
		}
	}

	if (empty_slot > INDIGO_MAX_DRIVERS) {
		if (dl_handle != NULL)
			dlclose(dl_handle);
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
	INDIGO_LOG(indigo_log("Driver %s %d.%d.%d.%d loaded", info.name, INDIGO_VERSION_MAJOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MINOR(INDIGO_VERSION_CURRENT), INDIGO_VERSION_MAJOR(info.version), INDIGO_VERSION_MINOR(info.version)));

	if (empty_slot == used_driver_slots)
		used_driver_slots++; /* if we are not filling a gap - increase used_slots */
	pthread_mutex_unlock(&mutex);

	if (driver != NULL)
		*driver = &indigo_available_drivers[empty_slot];

	if (init) {
		int result = entry_point(INDIGO_DRIVER_INIT, NULL);
		indigo_available_drivers[empty_slot].initialized = result == INDIGO_OK;
		if (result != INDIGO_OK)
			indigo_error("Failed to initialise driver");
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
		dlclose(driver->dl_handle);
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

#if defined(INDIGO_MACOS)
#define SO_NAME ".dylib"
#elif defined(INDIGO_LINUX)
#define SO_NAME ".so"
#endif

indigo_result indigo_load_driver(const char *name, bool init, indigo_driver_entry **driver) {
	char driver_name[INDIGO_NAME_SIZE];
	char so_name[INDIGO_NAME_SIZE];
	char *entry_point_name, *cp;
	void *dl_handle;
	driver_entry_point entry_point;

	strncpy(driver_name, name, sizeof(driver_name));
	strncpy(so_name, name, sizeof(so_name));

	entry_point_name = basename(driver_name);
	cp = strchr(entry_point_name, '.');
	if (cp)
		*cp = '\0';
	else
		strncat(so_name, SO_NAME, INDIGO_NAME_SIZE);

	dl_handle = dlopen(so_name, RTLD_LAZY);
	if (!dl_handle) {
		const char* dlsym_error = dlerror();
		INDIGO_ERROR(indigo_error("Driver %s can't be loaded (%s)", entry_point_name, dlsym_error));
		return INDIGO_FAILED;
	}
	entry_point = dlsym(dl_handle, entry_point_name);
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		INDIGO_ERROR(indigo_error("Can't load %s() (%s)", entry_point_name, dlsym_error));
		dlclose(dl_handle);
		return INDIGO_NOT_FOUND;
	}
	return add_driver(entry_point, dl_handle, init, driver);
}

indigo_driver_entry indigo_available_drivers[INDIGO_MAX_DRIVERS];
indigo_subprocess_entry indigo_available_subprocesses[INDIGO_MAX_SERVERS];

static void *subprocess_thread(indigo_subprocess_entry *subprocess) {
	INDIGO_LOG(indigo_log("Subprocess %s thread started", subprocess->executable));
	pthread_detach(pthread_self());
	int sleep_interval = 5;
	while (subprocess->pid >= 0) {
		int input[2], output[2];
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
			subprocess->protocol_adapter = indigo_xml_client_adapter(slash ? slash + 1 : subprocess->executable, "", input[0], output[1]);
			indigo_attach_device(subprocess->protocol_adapter);
			indigo_xml_parse(subprocess->protocol_adapter, NULL);
			indigo_detach_device(subprocess->protocol_adapter);
			if (subprocess->protocol_adapter) {
				if (subprocess->protocol_adapter->device_context) {
					free(subprocess->protocol_adapter->device_context);
				}
				free(subprocess->protocol_adapter);
			}
		}
		if (subprocess->pid >= 0) {
			 indigo_usleep(sleep_interval * 1000000);
			if (sleep_interval < 60)
				sleep_interval *= 2;
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
			if (subprocess != NULL)
				*subprocess = &indigo_available_subprocesses[dc];
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
	if (empty_slot == used_subprocess_slots)
		used_subprocess_slots++;
	pthread_mutex_unlock(&mutex);
	if (subprocess != NULL)
		*subprocess = &indigo_available_subprocesses[empty_slot];
	return INDIGO_OK;
}

indigo_result indigo_kill_subprocess(indigo_subprocess_entry *subprocess) {
	assert(subprocess != NULL);
	pthread_mutex_lock(&mutex);
	if (subprocess->pid > 0)
		kill(subprocess->pid, SIGKILL);
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
		char text[INET_ADDRSTRLEN];
		struct addrinfo hints = { 0 }, *address = NULL;
		int result;
		hints.ai_family = AF_INET;
		if ((result = getaddrinfo(server->host, NULL, &hints, &address))) {
			INDIGO_LOG(indigo_log("Can't resolve host name %s (%s)", server->host, gai_strerror(result)));
			strncpy(server->last_error, gai_strerror(result), sizeof(server->last_error));
		} else if ((server->socket = socket(address->ai_family, SOCK_STREAM, 0)) < 0) {
			INDIGO_LOG(indigo_log("Can't create socket (%s)", strerror(errno)));
			strncpy(server->last_error, strerror(errno), sizeof(server->last_error));
		} else {
			((struct sockaddr_in *)address->ai_addr)->sin_port = htons(server->port);
			result = connect(server->socket, address->ai_addr, address->ai_addrlen);
#if defined(INDIGO_WINDOWS)
			if (is_pre_vista()) {
				char *p;
				memset(text, 0, sizeof(text));
				p = inet_ntoa(((struct sockaddr_in *)address->ai_addr)->sin_addr);
				if (p)
					strncpy(text, p, strlen(p));
			} else {
				HMODULE hMod = LoadLibrary(TEXT("ws2_32.dll"));
				if (hMod) {
					PCTSTR(WINAPI *fn)(INT, PVOID, PTSTR, size_t) = GetProcAddress(hMod, "inet_ntop");
					if (fn)
						(*fn)(AF_INET, &((struct sockaddr_in *)address->ai_addr)->sin_addr, text, sizeof(text));
				}
			}
#else
			inet_ntop(AF_INET, &((struct sockaddr_in *)address->ai_addr)->sin_addr, text, sizeof(text));
#endif
			if (result < 0) {
				close(server->socket);
				server->socket = -1;
				INDIGO_LOG(indigo_log("Can't connect to socket %s:%d (%s)", text, ntohs(((struct sockaddr_in *)address->ai_addr)->sin_port), strerror(errno)));
				strncpy(server->last_error, strerror(errno), sizeof(server->last_error));
			}
		}
		if (address) {
			freeaddrinfo(address);
		} else {
			strncpy(text, server->host, sizeof(text));
		}
		if (server->socket >= 0) {
			server->last_error[0] = '\0';
			if (*server->name == 0) {
				indigo_service_name(server->host, server->port, server->name);
			}
			char  url[INDIGO_NAME_SIZE];
			snprintf(url, sizeof(url), "http://%s:%d", text, server->port);
			INDIGO_LOG(indigo_log("Server %s:%d (%s, %s) connected", server->host, server->port, server->name, url));
#if defined(INDIGO_WINDOWS)
			indigo_send_message(server->protocol_adapter, "connected");
#endif
			server->protocol_adapter = indigo_xml_client_adapter(server->name, url, server->socket, server->socket);
			indigo_attach_device(server->protocol_adapter);
			indigo_xml_parse(server->protocol_adapter, NULL);
			indigo_detach_device(server->protocol_adapter);
			if (server->protocol_adapter) {
				if (server->protocol_adapter->device_context) {
					free(server->protocol_adapter->device_context);
				}
				free(server->protocol_adapter);
			}
			server->protocol_adapter = NULL;
#if defined(INDIGO_WINDOWS)
			closesocket(server->socket);
#else
			close(server->socket);
#endif
			INDIGO_LOG(indigo_log("Server %s:%d disconnected", server->host, server->port));
#if defined(INDIGO_WINDOWS)
			indigo_send_message(server->protocol_adapter, "disconnected");
#endif
			int timeout = 10;
			while (!server->shutdown && timeout--) {
				indigo_usleep(0.1 * ONE_SECOND_DELAY);
			}
		} else {
			int timeout = 50;
			while (!server->shutdown && timeout--) {
				indigo_usleep(0.1 * ONE_SECOND_DELAY);
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
			if (server != NULL)
			*server = &indigo_available_servers[dc];
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
	indigo_available_servers[empty_slot].socket = -1;
	indigo_available_servers[empty_slot].connection_id = connection_id;
	*indigo_available_servers[empty_slot].last_error = 0;
	indigo_available_servers[empty_slot].shutdown = false;
	if (pthread_create(&indigo_available_servers[empty_slot].thread, NULL, (void*) (void *) server_thread, &indigo_available_servers[empty_slot]) != 0) {
		pthread_mutex_unlock(&mutex);
		return INDIGO_FAILED;
	}
	indigo_available_servers[empty_slot].thread_started = true;
	if (empty_slot == used_server_slots)
		used_server_slots++;
	pthread_mutex_unlock(&mutex);
	if (server != NULL)
		*server = &indigo_available_servers[empty_slot];
	return INDIGO_OK;
}

bool indigo_connection_status(indigo_server_entry *server, char *last_error) {
	bool connected = false;

	if (last_error != NULL && last_error[0] != 0) last_error[0] = 0;
	if (server == NULL) return false;

	pthread_mutex_lock(&mutex);
	if (server->socket >= 0) connected = true;
	if (last_error != NULL) strncpy(last_error, server->last_error, sizeof(server->last_error));
	pthread_mutex_unlock(&mutex);

	return connected;
}

indigo_result indigo_disconnect_server(indigo_server_entry *server) {
	assert(server != NULL);
	pthread_mutex_lock(&mutex);
	if (server->socket >= 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		shutdown(server->socket, SHUT_RDWR);
#endif
#if defined(INDIGO_WINDOWS)
		shutdown(server->socket, SD_BOTH);
		Sleep(500);
#endif
	}
	server->shutdown = true;
	bool thread_runing = server->thread_started;
	pthread_mutex_unlock(&mutex);
	int timeout = 5;
	while (thread_runing && timeout--) {
		pthread_mutex_lock(&mutex);
		thread_runing = server->thread_started;
		pthread_mutex_unlock(&mutex);
		indigo_usleep(0.1 * ONE_SECOND_DELAY);
	}
	server->socket = -1;
	return INDIGO_OK;
}

indigo_result indigo_format_number(char *buffer, int buffer_size, char *format, double value) {
	int format_length = (int)strlen(format);
	double d = fabs(value);
	double m = 60.0 * (d - floor(d));
	double s = 60.0 * (m - floor(m));
	if (!strcmp(format + format_length - 3, "10m")) {
		snprintf(buffer, buffer_size, "%d:%02d:%06.3f", (int)value, (int)m, s);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "9m")) {
		snprintf(buffer, buffer_size, "%d:%02d:%05.2f", (int)value, (int)m, s);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "8m")) {
		snprintf(buffer, buffer_size, "%d:%02d:%04.1f", (int)value, (int)m, s);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "6m")) {
		snprintf(buffer, buffer_size, "%d:%02d:%02d", (int)value, (int)m, (int)round(s));
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "5m")) {
		snprintf(buffer, buffer_size, "%d:%04.1f", (int)value, m);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 2, "4m")) {
		snprintf(buffer, buffer_size, "%d:%02d", (int)value, (int)m);
		return INDIGO_OK;
	} else if (!strcmp(format + format_length - 1, "m")) {
		snprintf(buffer, buffer_size, "%d:%02d:%04.1f", (int)value, (int)m, s);
		return INDIGO_OK;
	} else {
		return snprintf(buffer, buffer_size, format, value) == 1 ? INDIGO_OK : INDIGO_FAILED;
	}
	return INDIGO_FAILED;
}
