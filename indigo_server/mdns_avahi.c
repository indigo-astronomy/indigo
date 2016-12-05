// Copyright (c) 2016 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** mDNS AVAHI
 \file mdns_avahi.h
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <pthread.h> 
#include "mdns_avahi.h"

#ifdef HAVE_LIBAVAHI

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

static AvahiEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;
static AvahiClient *client = NULL;
static char *name = NULL;

static char svc_name[255];
static char svc_text[255];
static char svc_type[255];
static int svc_port;

pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sleep_cond = PTHREAD_COND_INITIALIZER;

static pthread_t tid;

static void create_services(AvahiClient *c);

int thread_sleep(int seconds) {
	struct timespec ttw;
	struct timeval now;
	int res;

	gettimeofday(&now,NULL);

	ttw.tv_sec = now.tv_sec + seconds;
	ttw.tv_nsec = now.tv_usec*1000;

	pthread_mutex_lock(&sleep_mutex);
	res = pthread_cond_timedwait(&sleep_cond, &sleep_mutex, &ttw);
	pthread_mutex_unlock(&sleep_mutex);
	return res;
}


static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
	assert(g == group || group == NULL);
	group = g;

	/* Called whenever the entry group state changes */
	switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			/* The entry group has been established successfully */
			LOG_DBG("avahi cleint: Service '%s' of type %s successfully established.", name, svc_type);
			break;

		case AVAHI_ENTRY_GROUP_COLLISION : {
			char *n;
			/* A service name collision with a remote service. */
			n = avahi_alternative_service_name(name);
			avahi_free(name);
			name = n;
			LOG("avahi_client: Service name collision, renaming service to '%s'", name);

			/* And recreate the services */
			create_services(avahi_entry_group_get_client(g));
			break;
		}

		case AVAHI_ENTRY_GROUP_FAILURE :
			LOG("avahi client: Entry group failure: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

			/* Some kind of failure happened while we were registering our services */
			avahi_simple_poll_quit(simple_poll);
		break;

		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
			;
	}
}

static void create_services(AvahiClient *c) {
	char *n;
	int ret;
	assert(c);

	/* If this is the first time we're called, let's create a new
	 * entry group if necessary */
	if (!group)
		if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
			LOG("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(c)));
			goto fail;
		}

	if (avahi_entry_group_is_empty(group)) {
		LOG_DBG("avahi cleint: Adding service '%s' of type '%s'", name, svc_type);

		char *service_text = NULL;
		if (svc_text[0]) service_text = svc_text;

		if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, name, svc_type, NULL, NULL, svc_port, service_text, NULL)) < 0) {
			if (ret == AVAHI_ERR_COLLISION) goto collision;
			LOG("avahi client: Failed to add service: %s", avahi_strerror(ret));
			goto fail;
		}


		/* Tell the server to register the service */
		if ((ret = avahi_entry_group_commit(group)) < 0) {
			LOG("avahi client: Failed to commit entry group: %s", avahi_strerror(ret));
			goto fail;
		}
	}
	return;

collision:
	/* A service name collision with a local service happened. */
	n = avahi_alternative_service_name(name);
	avahi_free(name);
	name = n;
	LOG("avahi client: Service name collision, renaming to '%s'", name);
	avahi_entry_group_reset(group);
	create_services(c);
	return;

fail:
	avahi_simple_poll_quit(simple_poll);
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
	assert(c);
	
	/* Called whenever the client or server state changes */
	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:
			create_services(c);
			break;

		case AVAHI_CLIENT_FAILURE:
			LOG("avahi client: failure: %s", avahi_strerror(avahi_client_errno(c)));
			avahi_simple_poll_quit(simple_poll);
			break;

		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			if (group) avahi_entry_group_reset(group);
			break;

		case AVAHI_CLIENT_CONNECTING:
			;
	}
}

void *avahi_thread(void *data) {
	int error;
	int ret = 1;
	int retrys = 4;

	/* Allocate main loop object */
	if (!(simple_poll = avahi_simple_poll_new())) {
		LOG( "avahi client: Failed to create simple poll object.");
		goto fail;
	}

	name = avahi_strdup(svc_name);

	/* Allocate a new client */
retry:
	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);

	/* Check wether creating the client object succeeded */
	if (!client) {
		LOG( "avahi client: Failed to create client: %s", avahi_strerror(error));
		if (retrys > 0) {
			LOG("Retrying %d more times.", retrys);
			thread_sleep(5);
			retrys--;
			goto retry;
		} else {
			LOG("Giving up...");
			goto fail;
		}
	}

	/* Run the main loop */
	avahi_simple_poll_loop(simple_poll);

	ret = 0;

fail:
	if (client) avahi_client_free(client);

	if (simple_poll) avahi_simple_poll_free(simple_poll);

	avahi_free(name);
	pthread_exit(&ret);
}

int mdns_init(char *name, char *type, char *text, int port) {
	strncpy(svc_name, name, 255);
	strncpy(svc_type, type, 255);
	if (text != NULL) strncpy(svc_text, text, 255);
	else svc_text[0] = '\0';
	svc_port = port;
	return 0;
}

int mdns_start() {
	int rc = pthread_create(&tid, NULL, avahi_thread, NULL);
	if (rc) return rc;
	rc = pthread_detach(tid);
	return rc;
}

int mdns_stop() {
	int rc = pthread_cancel(tid);
	if(rc) return rc;
	rc = pthread_join(tid, NULL);
	return rc;
}

#else /* HAVE_LIBAVAHI */

int mdns_init(char *name, char *type, char *text, int port) {
        return 0;
}

int mdns_start() {
	return 0;
}

int mdns_stop() {
	return 0;
}

#endif /* HAVE_LIBAVAHI */
