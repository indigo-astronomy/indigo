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
// 2.0 by Rumen G. Bogdanovski

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#if defined(INDIGO_LINUX)
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#endif
#if defined(INDIGO_MACOS)
#include <dns_sd.h>
#include <CoreFoundation/CoreFoundation.h>
#endif
#if defined(INDIGO_WINDOWS)
#include <dns_sd.h>
#endif

#include <pthread.h>

#include <indigo/indigo_service_discovery.h>

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


#if defined(INDIGO_LINUX)

static AvahiSimplePoll *simple_poll = NULL;
static AvahiClient *client = NULL;
static AvahiServiceBrowser *sb = NULL;

static void resolve_callback(AvahiServiceResolver *r, AvahiIfIndex interface_index, AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event, const char *name, AVAHI_GCC_UNUSED const char *type, AVAHI_GCC_UNUSED const char *domain, const char *host_name, AVAHI_GCC_UNUSED const AvahiAddress *address, uint16_t port, AVAHI_GCC_UNUSED AvahiStringList *txt, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* callback) {
	assert(r);
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
	int count = 0;
	switch (event) {
		case AVAHI_BROWSER_FAILURE:
			INDIGO_ERROR(indigo_error("avahi: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b)))));
			avahi_simple_poll_quit(simple_poll);
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

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
	assert(c);
	if (state == AVAHI_CLIENT_FAILURE) {
		INDIGO_ERROR(indigo_error("avahi: Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c))));
		avahi_simple_poll_quit(simple_poll);
	}
}

indigo_result indigo_resolve_service(const char *name, uint32_t interface_index, void (*callback)(const char *name, uint32_t interface_index, const char *host, int port)) {
	if (!(avahi_service_resolver_new(client, interface_index, AVAHI_PROTO_UNSPEC, name, "_indigo._tcp", NULL, AVAHI_PROTO_UNSPEC, 0, resolve_callback, callback))) {
		INDIGO_ERROR(indigo_error("avahi: Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(client))));
		return INDIGO_FAILED;
	}
	return INDIGO_OK;
}

void indigo_stop_service_browser(void) {
	if (simple_poll) {
		avahi_simple_poll_quit(simple_poll);
	}
	if (sb) {
		avahi_service_browser_free(sb);
		sb = NULL;
	}
	if (client) {
		avahi_client_free(client);
		client = NULL;
	}
	if (simple_poll) {
		avahi_simple_poll_free(simple_poll);
		simple_poll = NULL;
	}
	clear_services();
}

indigo_result indigo_start_service_browser(void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface_index)) {
	int error;
	clear_services();
	if (!(simple_poll = avahi_simple_poll_new())) {
		INDIGO_ERROR(indigo_error("avahi: Failed to create simple poll object.\n"));
		indigo_stop_service_browser();
		return INDIGO_FAILED;
	}

	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);
	if (!client) {
		INDIGO_ERROR(indigo_error("avahi:Failed to create client: %s\n", avahi_strerror(error)));
		indigo_stop_service_browser();
		return INDIGO_FAILED;
	}

	if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_indigo._tcp", NULL, 0, browse_callback, callback))) {
		INDIGO_ERROR(indigo_error("avahi: Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client))));
		indigo_stop_service_browser();
		return INDIGO_FAILED;
	}
	/* Run the main loop in a separate thread */
	indigo_async((void * (*)(void*))avahi_simple_poll_loop, simple_poll);
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
		indigo_copy_name(host, host_name);
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

static DNSServiceRef browser_sd = NULL;

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

static void *service_browser_handler(void *data) {
	INDIGO_DEBUG(indigo_debug("Service browser started"));
	while (browser_sd) {
		DNSServiceProcessResult(browser_sd);
	}
	clear_services();
	INDIGO_DEBUG(indigo_debug("Service browser stopped"));
	return NULL;
}

indigo_result indigo_start_service_browser(void (*callback)(indigo_service_discovery_event event, const char *name, uint32_t interface_index)) {
	clear_services();
	DNSServiceErrorType result = DNSServiceBrowse(&browser_sd, 0, kDNSServiceInterfaceIndexAny, "_indigo._tcp", "local.", browser_callback, callback);
	if (result == kDNSServiceErr_NoError) {
		indigo_async((void *(*)(void *))service_browser_handler, NULL);
		return INDIGO_OK;
	}
	indigo_error("Failed to start service browser (%d)", result);
	return INDIGO_FAILED;
}

void indigo_stop_service_browser(void) {
	DNSServiceRefDeallocate(browser_sd);
	browser_sd = NULL;
}

#endif /* INDIGO_MACOS aand INDIGO_WINDOWS */
