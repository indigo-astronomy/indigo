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
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#if defined(INDIGO_WINDOWS)
#include <io.h>
#include <winsock2.h>
#pragma warning(disable:4996)
#endif

#include "indigo_client_xml.h"
#include "indigo_client.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

static int used_driver_slots = 0;
static int used_subprocess_slots = 0;

static indigo_result add_driver(driver_entry_point entry_point, void *dl_handle, bool init, indigo_driver_entry **driver) {
	int empty_slot = used_driver_slots; /* the first slot after the last used is a good candidate */
	pthread_mutex_lock(&mutex);
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
		return INDIGO_TOO_MANY_ELEMENTS; /* no emty slot found, list is full */
	}

	indigo_driver_info info;
	entry_point(INDIGO_DRIVER_INFO, &info);
	strncpy(indigo_available_drivers[empty_slot].description, info.description, INDIGO_NAME_SIZE); //TO BE CHANGED - DRIVER SHOULD REPORT NAME!!!
	strncpy(indigo_available_drivers[empty_slot].name, info.name, INDIGO_NAME_SIZE);
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
		return result;
	}
	return INDIGO_OK;
}

indigo_result indigo_remove_driver(indigo_driver_entry *driver) {
	assert(driver != NULL);
	pthread_mutex_lock(&mutex);
	driver->driver(INDIGO_DRIVER_SHUTDOWN, NULL); /* deregister */
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
			subprocess->last_error = strerror(errno);
			return NULL;
		}
		subprocess->pid = fork();
		if (subprocess->pid == -1) {
			INDIGO_ERROR(indigo_error("Can't create subprocess %s (%s)", subprocess->executable, strerror(errno)));
			subprocess->last_error = strerror(errno);
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
			free(subprocess->protocol_adapter->device_context);
			free(subprocess->protocol_adapter);
		}
		if (subprocess->pid >= 0) {
			sleep(sleep_interval);
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
	int empty_slot = used_subprocess_slots;
	pthread_mutex_lock(&mutex);
	for (int dc = 0; dc < used_subprocess_slots;  dc++) {
		if (indigo_available_subprocesses[dc].thread_started && !strcmp(indigo_available_subprocesses[dc].executable, executable)) {
			INDIGO_LOG(indigo_log("Subprocess %s already started", indigo_available_subprocesses[dc].executable));
			if (subprocess != NULL)
				*subprocess = &indigo_available_subprocesses[dc];
			pthread_mutex_unlock(&mutex);
			return INDIGO_DUPLICATED;
		} else if (!indigo_available_subprocesses[dc].thread_started) {
			empty_slot = dc;
			break;
		}
	}

	if (empty_slot > INDIGO_MAX_SERVERS) {
		pthread_mutex_unlock(&mutex);
		return INDIGO_TOO_MANY_ELEMENTS;
	}

	strncpy(indigo_available_subprocesses[empty_slot].executable, executable, INDIGO_NAME_SIZE);
	indigo_available_subprocesses[empty_slot].pid = 0;
	indigo_available_subprocesses[empty_slot].last_error = NULL;
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
  strncpy(name, host, INDIGO_NAME_SIZE);
  char *lastone = name + strlen(name) - 1;
  if (*lastone == '.')
    *lastone = 0;
  char * local = strstr(name, ".local");
  if (local != NULL && (!strcmp(local, ".local")))
    *local = 0;
  if (port != 7624) {
    sprintf(name + strlen(name), ":%d", port);
  }
}

static void *server_thread(indigo_server_entry *server) {
  INDIGO_LOG(indigo_log("Server %s:%d thread started", server->host, server->port));
	pthread_detach(pthread_self());
  while (server->socket >= 0) {
    server->socket = 0;
		struct addrinfo hints = { 0 }, *address = NULL;
		int result;
		hints.ai_family = AF_INET;
    if ((result = getaddrinfo(server->host, NULL, &hints, &address))) {
      INDIGO_LOG(indigo_error("Can't resolve host name %s (%s)", server->host, gai_strerror(result)));
			server->last_error = gai_strerror(result);
    } else if ((server->socket = socket(address->ai_family, SOCK_STREAM, 0)) < 0) {
      INDIGO_LOG(indigo_error("Can't create socket (%s)", strerror(errno)));
			server->last_error = strerror(errno);
    } else {
     	((struct sockaddr_in *)address->ai_addr)->sin_port = htons(server->port);
      if (connect(server->socket, address->ai_addr, address->ai_addrlen) < 0) {
				INDIGO_LOG(indigo_error("Can't connect to socket (%s)", strerror(errno)));
				server->last_error = strerror(errno);
        close(server->socket);
        server->socket = 0;
      }
    }
		if (address)
			freeaddrinfo(address);
    if (server->socket > 0) {
      if (*server->name == 0) {
        indigo_service_name(server->host, server->port, server->name);
      }
      char  url[INDIGO_NAME_SIZE];
      snprintf(url, sizeof(url), "http://%s:%d", server->host, server->port);
      INDIGO_LOG(indigo_log("Server %s:%d (%s, %s) connected", server->host, server->port, server->name, url));
      server->protocol_adapter = indigo_xml_client_adapter(server->name, url, server->socket, server->socket);
      indigo_attach_device(server->protocol_adapter);
      indigo_xml_parse(server->protocol_adapter, NULL);
      indigo_detach_device(server->protocol_adapter);
      free(server->protocol_adapter->device_context);
      free(server->protocol_adapter);
			server->protocol_adapter = NULL;
      close(server->socket);
			server->socket = 0;
      INDIGO_LOG(indigo_log("Server %s:%d disconnected", server->host, server->port));
    }
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
    sleep(5); 
#endif
#if defined(INDIGO_WINDOWS)
    Sleep(5);
#endif
  }
  server->thread_started = false;
  INDIGO_LOG(indigo_log("Server %s:%d thread stopped", server->host, server->port));
  return NULL;
}

indigo_result indigo_connect_server(const char *name, const char *host, int port, indigo_server_entry **server) {
  int empty_slot = used_server_slots;
  pthread_mutex_lock(&mutex);
  for (int dc = 0; dc < used_server_slots; dc++) {
    if (indigo_available_servers[dc].socket > 0 && !strcmp(indigo_available_servers[dc].host, host) && indigo_available_servers[dc].port == port) {
      INDIGO_LOG(indigo_log("Server %s:%d already connected", indigo_available_servers[dc].host, indigo_available_servers[dc].port));
      if (server != NULL)
        *server = &indigo_available_servers[dc];
      pthread_mutex_unlock(&mutex);
      return INDIGO_DUPLICATED;
    } else if (!indigo_available_servers[dc].thread_started) {
      empty_slot = dc;
    }
  }
  if (empty_slot > INDIGO_MAX_SERVERS) {
    pthread_mutex_unlock(&mutex);
    return INDIGO_TOO_MANY_ELEMENTS;
  }
  if (name != NULL) {
    strncpy(indigo_available_servers[empty_slot].name, name, INDIGO_NAME_SIZE);
  } else {
    *indigo_available_servers[empty_slot].name = 0;
  }
  strncpy(indigo_available_servers[empty_slot].host, host, INDIGO_NAME_SIZE);
  indigo_available_servers[empty_slot].port = port;
  indigo_available_servers[empty_slot].socket = 0;
	indigo_available_servers[empty_slot].last_error = NULL;
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

indigo_result indigo_disconnect_server(indigo_server_entry *server) {
  assert(server != NULL);
  pthread_mutex_lock(&mutex);
  if (server->socket > 0)
    close(server->socket);
  server->socket = -1;
  pthread_mutex_unlock(&mutex);
  return INDIGO_OK;
}
