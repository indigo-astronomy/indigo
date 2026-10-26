// Copyright (c) 2020-2025 CloudMakers, s. r. o. & Rumen G. Bogdanovski
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
//      & Rumen G. Bogdanovski <rumenastro@gmail.com>

#ifndef EXAMPLE_REMOTE_H
#define EXAMPLE_REMOTE_H
#include <errno.h>
#include <limits.h>
#include <pthread.h>

static pthread_mutex_t completion_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool finished = false;

static bool example_is_finished(void) {
	pthread_mutex_lock(&completion_mutex);
	bool value = finished;
	pthread_mutex_unlock(&completion_mutex);
	return value;
}

static void example_finish(void) {
	pthread_mutex_lock(&completion_mutex);
	finished = true;
	pthread_mutex_unlock(&completion_mutex);
}

/* Positional arguments: host service device timeout_seconds port. */
static bool example_remote_options(int argc, const char *argv[], const char **host, const char **service, char *device, const char *default_device, int *timeout, int *port) {
	if (argc > 6) {
		return false;
	}
	*host = argc > 1 ? argv[1] : "localhost";
	*service = argc > 2 ? argv[2] : "example";
	if (!**host || !**service || strlen(*host) >= INDIGO_NAME_SIZE || strlen(*service) >= INDIGO_NAME_SIZE) {
		return false;
	}
	int length = argc > 3 ? snprintf(device, INDIGO_NAME_SIZE, "%s", argv[3]) : snprintf(device, INDIGO_NAME_SIZE, "%s @ %s", default_device, *service);
	if (length <= 0 || length >= INDIGO_NAME_SIZE) {
		return false;
	}
	*timeout = 120;
	*port = 7624;
	for (int i = 4; i < argc; i++) {
		char *end;
		errno = 0;
		long value = strtol(argv[i], &end, 10);
		if (errno || !*argv[i] || *end || value < 1 || value > (i == 4 ? 3600 : 65535)) {
			return false;
		}
		if (i == 4) {
			*timeout = (int)value;
		} else {
			*port = (int)value;
		}
	}
	return true;
}
#endif
