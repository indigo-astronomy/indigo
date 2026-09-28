// Copyright (C) 2023 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>

#if defined(INDIGO_LINUX)
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#endif
#if defined(INDIGO_MACOS)
#include <dns_sd.h>
#include <unistd.h>
#include <poll.h>
#include <CoreFoundation/CoreFoundation.h>
#endif
#if defined(INDIGO_WINDOWS)
#include <dns_sd.h>
#endif

#include <pthread.h>

#include <indigo/indigo_service_discovery.h>
#include <indigo/indigo_uni_io.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static struct service_struct {
	char name[INDIGO_NAME_SIZE];
	int count;
	struct service_struct *next;
} *services = NULL;

/* returns number of service instances exists */
static int add_service(const char *name) {
	pthread_mutex_lock(&mutex);
	struct service_struct *service = services;
	while (service) {
		if (!strncmp(name, service->name, INDIGO_NAME_SIZE)) {
			int count = ++service->count;
			pthread_mutex_unlock(&mutex);
			return count;
		}
		service = service->next;
	}
	service = indigo_safe_malloc(sizeof(struct service_struct));
	strncpy(service->name, name, INDIGO_NAME_SIZE);
	service->next = services;
	services = service;
	int count = ++service->count;
	pthread_mutex_unlock(&mutex);
	return count;
}

/* returns number of service instances exists */
static int remove_service(const char *name) {
	pthread_mutex_lock(&mutex);
	struct service_struct *service = services;
	if (!service) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	if (!strncmp(name, service->name, INDIGO_NAME_SIZE)) {
		int count = --service->count;
		if (count == 0) {
			struct service_struct *buf = service;
			services = buf->next;
			indigo_safe_free(buf);
		}
		pthread_mutex_unlock(&mutex);
		return count;
	}
	while (service->next) {
		if (!strncmp(name, service->next->name, INDIGO_NAME_SIZE)) {
			int count = --service->next->count;
			if (count == 0) {
				struct service_struct *buf = service->next;
				service->next = buf->next;
				indigo_safe_free(buf);
			}
			pthread_mutex_unlock(&mutex);
			return count;
		}
		service = service->next;
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

static void clear_services() {
	pthread_mutex_lock(&mutex);
	while (services) {
		struct service_struct *buf = services;
		services = buf->next;
		indigo_safe_free(buf);
	}
	pthread_mutex_unlock(&mutex);
}

/* services seen by a failed browser are reported as removed, stopping(data) returning true aborts reporting */
static void report_removed_services(void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface_index), bool (*stopping)(void *data), void *data) {
	bool removed = false;
	while (stopping == NULL || !stopping(data)) {
		pthread_mutex_lock(&mutex);
		struct service_struct *service = services;
		if (service) {
			services = service->next;
		}
		pthread_mutex_unlock(&mutex);
		if (service == NULL) {
			break;
		}
		INDIGO_DEBUG(indigo_debug("Service '%s' removed after browser failure", service->name));
		callback(INDIGO_SERVICE_REMOVED_GROUPED, service->name, INDIGO_INTERFACE_ANY);
		indigo_safe_free(service);
		removed = true;
	}
	if (removed && (stopping == NULL || !stopping(data))) {
		callback(INDIGO_SERVICE_END_OF_RECORD, "", INDIGO_INTERFACE_ANY);
	}
}


#if defined(INDIGO_LINUX)

// callbacks run on the threaded poll thread, other threads access the client only under avahi_threaded_poll_lock()
static pthread_mutex_t browser_mutex = PTHREAD_MUTEX_INITIALIZER;
static AvahiThreadedPoll *threaded_poll = NULL;
static AvahiClient *client = NULL;
static AvahiServiceBrowser *sb = NULL;
static bool threaded_poll_running = false;
static __thread bool in_poll_thread = false;

static void resolve_callback(AvahiServiceResolver *r, AvahiIfIndex interface_index, AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event, const char *name, AVAHI_GCC_UNUSED const char *type, AVAHI_GCC_UNUSED const char *domain, const char *host_name, AVAHI_GCC_UNUSED const AvahiAddress *address, uint16_t port, AVAHI_GCC_UNUSED AvahiStringList *txt, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* callback) {
	assert(r);
	in_poll_thread = true;
	/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			INDIGO_ERROR(indigo_error("avahi: Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r)))));
			((void (*)(const char *name, uint32_t interface_index, const char *host, int port))callback)(name, (uint32_t)interface_index, NULL, 0);
			break;
		case AVAHI_RESOLVER_FOUND: {
			INDIGO_DEBUG(indigo_debug("Service '%s' hostname '%s:%u'\n", name, host_name, port));
			((void (*)(const char *name, uint32_t interface_index, const char *host, int port))callback)(name, (uint32_t)interface_index, host_name, port);
			break;
		}
	}
	avahi_service_resolver_free(r);
}

static void browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface_index, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* callback) {
	assert(b);
	in_poll_thread = true;
	int count = 0;
	switch (event) {
		case AVAHI_BROWSER_FAILURE:
			INDIGO_ERROR(indigo_error("avahi: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b)))));
			avahi_threaded_poll_quit(threaded_poll);
			report_removed_services((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback, NULL, NULL);
			return;
		case AVAHI_BROWSER_NEW:
			if ((count = add_service(name)) == 1) {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d) added", name, count));
				((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_ADDED_GROUPED, name, INDIGO_INTERFACE_ANY);
			} else {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d)", name, count));
			}
			INDIGO_DEBUG(indigo_debug("Service '%s' added (interface %d)", name, interface_index));
			((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_ADDED, name, interface_index);
			break;
		case AVAHI_BROWSER_REMOVE:
			INDIGO_DEBUG(indigo_debug("Service '%s' removed (interface %d)", name, interface_index));
			((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_REMOVED, name, interface_index);
			if ((count = remove_service(name)) == 0) {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d) removed", name, count));
				((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_REMOVED_GROUPED, name, INDIGO_INTERFACE_ANY);
			} else {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d)", name, count));
			}
			break;
		case AVAHI_BROWSER_ALL_FOR_NOW:
			((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface))callback)(INDIGO_SERVICE_END_OF_RECORD, "", INDIGO_INTERFACE_ANY);
			break;
	}
}

static void client_callback(AvahiClient *c, AvahiClientState state, void *callback) {
	assert(c);
	if (state == AVAHI_CLIENT_FAILURE) {
		INDIGO_ERROR(indigo_error("avahi: Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c))));
		/* the callback is called synchronously from avahi_client_new() before the poll thread is started */
		if (threaded_poll_running) {
			in_poll_thread = true;
			avahi_threaded_poll_quit(threaded_poll);
			report_removed_services((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback, NULL, NULL);
		}
	}
}

indigo_result indigo_resolve_service(const char *name, uint32_t interface_index, void (*callback)(const char *name, uint32_t interface_index, const char *host, int port)) {
	indigo_result result = INDIGO_OK;
	if (in_poll_thread) {
		/* called from a browser callback, the poll thread already holds the poll lock and keeps the client alive */
		if (!(avahi_service_resolver_new(client, interface_index, AVAHI_PROTO_UNSPEC, name, "_indigo._tcp", NULL, AVAHI_PROTO_UNSPEC, 0, resolve_callback, callback))) {
			INDIGO_ERROR(indigo_error("avahi: Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(client))));
			result = INDIGO_FAILED;
		}
		return result;
	}
	pthread_mutex_lock(&browser_mutex);
	if (client == NULL || !threaded_poll_running) {
		INDIGO_ERROR(indigo_error("avahi: Failed to resolve service '%s': browser is not running\n", name));
		result = INDIGO_FAILED;
	} else {
		avahi_threaded_poll_lock(threaded_poll);
		if (!(avahi_service_resolver_new(client, interface_index, AVAHI_PROTO_UNSPEC, name, "_indigo._tcp", NULL, AVAHI_PROTO_UNSPEC, 0, resolve_callback, callback))) {
			INDIGO_ERROR(indigo_error("avahi: Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(client))));
			result = INDIGO_FAILED;
		}
		avahi_threaded_poll_unlock(threaded_poll);
	}
	pthread_mutex_unlock(&browser_mutex);
	return result;
}

/* must be called with browser_mutex locked and not from the poll thread */
static void stop_service_browser_locked(void) {
	if (threaded_poll && threaded_poll_running) {
		/* joins the poll thread, no callback runs after it returns */
		avahi_threaded_poll_stop(threaded_poll);
	}
	threaded_poll_running = false;
	if (sb) {
		avahi_service_browser_free(sb);
		sb = NULL;
	}
	if (client) {
		avahi_client_free(client);
		client = NULL;
	}
	if (threaded_poll) {
		avahi_threaded_poll_free(threaded_poll);
		threaded_poll = NULL;
	}
	clear_services();
}

void indigo_stop_service_browser(void) {
	if (in_poll_thread) {
		/* called from a browser callback, the poll thread can't join itself; resources are released by the next start or stop */
		avahi_threaded_poll_quit(threaded_poll);
		return;
	}
	pthread_mutex_lock(&browser_mutex);
	stop_service_browser_locked();
	pthread_mutex_unlock(&browser_mutex);
}

indigo_result indigo_start_service_browser(void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface_index)) {
	if (in_poll_thread) {
		INDIGO_ERROR(indigo_error("avahi: Service browser can't be started from a browser callback\n"));
		return INDIGO_FAILED;
	}
	int error;
	pthread_mutex_lock(&browser_mutex);
	stop_service_browser_locked();
	if (!(threaded_poll = avahi_threaded_poll_new())) {
		INDIGO_ERROR(indigo_error("avahi: Failed to create threaded poll object.\n"));
		stop_service_browser_locked();
		pthread_mutex_unlock(&browser_mutex);
		return INDIGO_FAILED;
	}
	client = avahi_client_new(avahi_threaded_poll_get(threaded_poll), 0, client_callback, callback, &error);
	if (!client) {
		INDIGO_ERROR(indigo_error("avahi:Failed to create client: %s\n", avahi_strerror(error)));
		stop_service_browser_locked();
		pthread_mutex_unlock(&browser_mutex);
		return INDIGO_FAILED;
	}
	if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_indigo._tcp", NULL, 0, browse_callback, callback))) {
		INDIGO_ERROR(indigo_error("avahi: Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client))));
		stop_service_browser_locked();
		pthread_mutex_unlock(&browser_mutex);
		return INDIGO_FAILED;
	}
	/* Run the main loop in a separate thread */
	threaded_poll_running = true;
	if (avahi_threaded_poll_start(threaded_poll) < 0) {
		INDIGO_ERROR(indigo_error("avahi: Failed to start threaded poll.\n"));
		threaded_poll_running = false;
		stop_service_browser_locked();
		pthread_mutex_unlock(&browser_mutex);
		return INDIGO_FAILED;
	}
	pthread_mutex_unlock(&browser_mutex);
	return INDIGO_OK;
}
#endif  /* INDIGO_LINUX */

#if defined(INDIGO_MACOS) || defined(INDIGO_WINDOWS)

#if !defined(WINAPI)
#define WINAPI
#endif

static void *service_process_result_handler(DNSServiceRef s_ref) {
	DNSServiceErrorType result = DNSServiceProcessResult(s_ref);
	if (result != kDNSServiceErr_NoError) {
		INDIGO_ERROR(indigo_error("Failed to process result (%d)", result));
	}
	DNSServiceRefDeallocate(s_ref);
	return NULL;
}

static void copy_unescaped(char *to, const char *from) {
	for (int i = 0; *from && i < INDIGO_NAME_SIZE - 1; i++) {
		if (from[0] == '\\') {
			if (from[1] == '\\') {
				*to++ = '\\';
				from += 2;
			} else {
				*to++ = 100 * (from[1] - '0') + 10 * (from[2] - '0') + (from[3] - '0');
				from += 4;
			}
		} else {
			*to++ = *from++;
		}
	}
	*to++ = 0;
}

static void WINAPI resolver_callback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interface_index, DNSServiceErrorType error_code, const char *full_name, const char *host_name, uint16_t port, uint16_t txt_len, const unsigned char *txt_record, void *callback) {
	if (error_code != kDNSServiceErr_NoError) {
		char name[INDIGO_NAME_SIZE], *dot;
		copy_unescaped(name, full_name);
		if ((dot = strchr(name, '.'))) {
			*dot = 0;
		}
		INDIGO_ERROR(indigo_error("Service resolution failed for %s", name));
		((void (*)(const char *name, uint32_t interface_index, const char *host, int port))callback)(name, interface_index, NULL, 0);
	} else if ((flags & kDNSServiceFlagsMoreComing) == 0) {
		char name[INDIGO_NAME_SIZE], host[INDIGO_NAME_SIZE], *dot;
		copy_unescaped(name, full_name);
		if ((dot = strchr(name, '.'))) {
			*dot = 0;
		}
		INDIGO_COPY_NAME(host, host_name);
		if (*(dot = host + strlen(host) - 1) == '.') {
			*dot = 0;
		}
		port = ntohs(port);
		INDIGO_DEBUG(indigo_debug("Service %s resolved to %s:%d", name, host, port));
		((void (*)(const char *name, uint32_t interface_index, const char *host, int port))callback)(name, interface_index, host, port);
	}
}

indigo_result indigo_resolve_service(const char *name, uint32_t interface_index, void (*callback)(const char *name, uint32_t interface_index, const char *host, int port)) {
	INDIGO_DEBUG(indigo_debug("Resolving service %s", name));
	DNSServiceRef sd_ref = NULL;
	DNSServiceErrorType result = DNSServiceResolve(&sd_ref, 0, interface_index, name, "_indigo._tcp", "local.", resolver_callback, callback);
	if (result == kDNSServiceErr_NoError) {
		indigo_async((void *(*)(void *))service_process_result_handler, sd_ref);
		return INDIGO_OK;
	}
	INDIGO_ERROR(indigo_error("Failed to resolve %s (%d)", name, result));
	return INDIGO_FAILED;
}

static void WINAPI browser_callback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interface_index, DNSServiceErrorType error_code, const char *name, const char *type, const char *domain, void *callback) {
	if (strcmp(indigo_local_service_name, name) && !strcmp(domain, "local.")) {
		int count = 0;
		if (flags & kDNSServiceFlagsAdd) {
			if ((count = add_service(name)) == 1) {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d) added", name, count));
				((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_ADDED_GROUPED, name, INDIGO_INTERFACE_ANY);
			} else {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d)", name, count));
			}
			INDIGO_DEBUG(indigo_debug("Service '%s' added (interface %d)", name, interface_index));
			((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_ADDED, name, interface_index);
		} else {
			INDIGO_DEBUG(indigo_debug("Service '%s' removed (interface %d)", name, interface_index));
			((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_REMOVED, name, interface_index);
			if ((count = remove_service(name)) == 0) {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d) removed", name, count));
				((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_REMOVED_GROUPED, name, INDIGO_INTERFACE_ANY);
			} else {
				INDIGO_DEBUG(indigo_debug("Service '%s' (count = %d)", name, count));
			}
		}
		if ((flags & kDNSServiceFlagsMoreComing) == 0) {
			((void (*)(indigo_service_discovery_event event, const char *name, uint32_t interface_index))callback)(INDIGO_SERVICE_END_OF_RECORD, "", INDIGO_INTERFACE_ANY);
		}
	}
}

// DNSServiceRef of the browser is owned by its handler thread, indigo_stop_service_browser() only signals and joins it
typedef struct {
	DNSServiceRef sd_ref;
	void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface_index);
	pthread_t thread;
	pthread_mutex_t mutex;
	bool stop;
	bool detached;
#if !defined(INDIGO_WINDOWS)
	int wakeup[2];
#endif
} browser_session;

#define BROWSER_RETRY_DELAY_MIN		1000
#define BROWSER_RETRY_DELAY_MAX		30000
#define BROWSER_POLL_TIMEOUT			100

static pthread_mutex_t browser_mutex = PTHREAD_MUTEX_INITIALIZER;
static browser_session *browser = NULL;

static bool browser_stopping(browser_session *session) {
	pthread_mutex_lock(&session->mutex);
	bool stop = session->stop;
	pthread_mutex_unlock(&session->mutex);
	return stop;
}

static bool browser_session_stopping(void *data) {
	return browser_stopping((browser_session *)data);
}

static void free_browser_session(browser_session *session) {
	if (session->sd_ref) {
		DNSServiceRefDeallocate(session->sd_ref);
	}
#if !defined(INDIGO_WINDOWS)
	if (session->wakeup[0] >= 0) {
		close(session->wakeup[0]);
	}
	if (session->wakeup[1] >= 0) {
		close(session->wakeup[1]);
	}
#endif
	pthread_mutex_destroy(&session->mutex);
	indigo_safe_free(session);
}

/* returns 1 if the browser has a result to process, 0 on wake-up or timeout, -1 on error */
static int wait_for_browser(browser_session *session, int timeout_ms) {
#if defined(INDIGO_WINDOWS)
	SOCKET fd = (SOCKET)DNSServiceRefSockFD(session->sd_ref);
	if (fd == INVALID_SOCKET) {
		INDIGO_ERROR(indigo_error("Service browser has no socket"));
		return -1;
	}
	fd_set readout;
	FD_ZERO(&readout);
	FD_SET(fd, &readout);
	if (timeout_ms < 0 || timeout_ms > BROWSER_POLL_TIMEOUT) {
		timeout_ms = BROWSER_POLL_TIMEOUT;
	}
	struct timeval tv = { .tv_sec = 0, .tv_usec = timeout_ms * 1000 };
	int result = select(0, &readout, NULL, NULL, &tv);
	if (result == SOCKET_ERROR) {
		INDIGO_ERROR(indigo_error("Service browser failed to wait for result (%d)", WSAGetLastError()));
		return -1;
	}
	return result > 0 && FD_ISSET(fd, &readout) ? 1 : 0;
#else
	int fd = DNSServiceRefSockFD(session->sd_ref);
	if (fd < 0) {
		INDIGO_ERROR(indigo_error("Service browser has no socket"));
		return -1;
	}
	struct pollfd fds[2] = { { .fd = session->wakeup[0], .events = POLLIN }, { .fd = fd, .events = POLLIN } };
	int result = poll(fds, 2, timeout_ms);
	if (result < 0) {
		if (errno == EINTR) {
			return 0;
		}
		INDIGO_ERROR(indigo_error("Service browser failed to wait for result (%s)", strerror(errno)));
		return -1;
	}
	if (result == 0 || fds[0].revents) {
		return 0;
	}
	return (fds[1].revents & (POLLIN | POLLHUP)) ? 1 : -1;
#endif
}

/* waits for the timeout or until the browser is stopped */
static void browser_sleep(browser_session *session, int timeout_ms) {
#if defined(INDIGO_WINDOWS)
	while (timeout_ms > 0 && !browser_stopping(session)) {
		indigo_usleep(BROWSER_POLL_TIMEOUT * 1000);
		timeout_ms -= BROWSER_POLL_TIMEOUT;
	}
#else
	struct pollfd fds[1] = { { .fd = session->wakeup[0], .events = POLLIN } };
	while (poll(fds, 1, timeout_ms) < 0 && errno == EINTR) {
	}
#endif
}

static void *service_browser_handler(void *data) {
	browser_session *session = (browser_session *)data;
	indigo_rename_thread("Service browser");
	INDIGO_DEBUG(indigo_debug("Service browser started"));
	int retry_delay = BROWSER_RETRY_DELAY_MIN;
	while (!browser_stopping(session)) {
		if (session->sd_ref == NULL) {
			browser_sleep(session, retry_delay);
			if (browser_stopping(session)) {
				break;
			}
			retry_delay = retry_delay * 2 > BROWSER_RETRY_DELAY_MAX ? BROWSER_RETRY_DELAY_MAX : retry_delay * 2;
			DNSServiceErrorType result = DNSServiceBrowse(&session->sd_ref, 0, kDNSServiceInterfaceIndexAny, "_indigo._tcp", "local.", browser_callback, session->callback);
			if (result != kDNSServiceErr_NoError) {
				INDIGO_ERROR(indigo_error("Failed to restart service browser (%d)", result));
				session->sd_ref = NULL;
			} else {
				INDIGO_LOG(indigo_log("Service browser restarted"));
			}
			continue;
		}
		int ready = wait_for_browser(session, -1);
		if (ready == 0) {
			continue;
		}
		if (ready > 0) {
			DNSServiceErrorType result = DNSServiceProcessResult(session->sd_ref);
			if (result == kDNSServiceErr_NoError) {
				retry_delay = BROWSER_RETRY_DELAY_MIN;
				continue;
			}
			if (browser_stopping(session)) {
				break;
			}
			INDIGO_ERROR(indigo_error("Service browser failed to process result (%d)", result));
		}
		DNSServiceRefDeallocate(session->sd_ref);
		session->sd_ref = NULL;
		report_removed_services(session->callback, browser_session_stopping, session);
	}
	pthread_mutex_lock(&session->mutex);
	bool detached = session->detached;
	pthread_mutex_unlock(&session->mutex);
	INDIGO_DEBUG(indigo_debug("Service browser stopped"));
	if (detached) {
		free_browser_session(session);
	}
	return NULL;
}

static void stop_browser_session(browser_session *session) {
	if (session == NULL) {
		return;
	}
	pthread_mutex_lock(&session->mutex);
	session->stop = true;
	pthread_mutex_unlock(&session->mutex);
#if !defined(INDIGO_WINDOWS)
	char byte = 0;
	while (write(session->wakeup[1], &byte, 1) < 0 && errno == EINTR) {
	}
#endif
	if (pthread_equal(pthread_self(), session->thread)) {
		/* called from the browser callback, the handler thread frees the session when the callback returns */
		pthread_mutex_lock(&session->mutex);
		session->detached = true;
		pthread_mutex_unlock(&session->mutex);
		pthread_detach(session->thread);
	} else {
		pthread_join(session->thread, NULL);
		free_browser_session(session);
	}
}

indigo_result indigo_start_service_browser(void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface_index)) {
	pthread_mutex_lock(&browser_mutex);
	browser_session *previous = browser;
	browser = NULL;
	pthread_mutex_unlock(&browser_mutex);
	stop_browser_session(previous);
	clear_services();
	browser_session *session = indigo_safe_malloc(sizeof(browser_session));
	session->callback = callback;
	pthread_mutex_init(&session->mutex, NULL);
#if !defined(INDIGO_WINDOWS)
	if (pipe(session->wakeup) < 0) {
		INDIGO_ERROR(indigo_error("Failed to create service browser wake-up pipe (%s)", strerror(errno)));
		session->wakeup[0] = session->wakeup[1] = -1;
		free_browser_session(session);
		return INDIGO_FAILED;
	}
#endif
	DNSServiceErrorType result = DNSServiceBrowse(&session->sd_ref, 0, kDNSServiceInterfaceIndexAny, "_indigo._tcp", "local.", browser_callback, callback);
	if (result != kDNSServiceErr_NoError) {
		INDIGO_ERROR(indigo_error("Failed to start service browser (%d)", result));
		session->sd_ref = NULL;
		free_browser_session(session);
		return INDIGO_FAILED;
	}
	if (pthread_create(&session->thread, NULL, service_browser_handler, session) != 0) {
		INDIGO_ERROR(indigo_error("Failed to start service browser thread"));
		free_browser_session(session);
		return INDIGO_FAILED;
	}
	pthread_mutex_lock(&browser_mutex);
	previous = browser;
	browser = session;
	pthread_mutex_unlock(&browser_mutex);
	stop_browser_session(previous);
	return INDIGO_OK;
}

void indigo_stop_service_browser(void) {
	pthread_mutex_lock(&browser_mutex);
	browser_session *session = browser;
	browser = NULL;
	pthread_mutex_unlock(&browser_mutex);
	stop_browser_session(session);
	clear_services();
}

#endif /* INDIGO_MACOS aand INDIGO_WINDOWS */
