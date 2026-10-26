// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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

/** INDIGO TCP wire protocol server
 \file indigo_server_tcp.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <dns_sd.h>
#include <sys/stat.h>

#if defined(INDIGO_LINUX)
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_driver_json.h>
#include <indigo/indigo_client_xml.h>
#include <indigo/indigo_base64.h>
#include <indigo/indigo_uni_io.h>
#include <indigo/indigo_timer.h>

#define SHA1_SIZE 20
#if _MSC_VER
# define _sha1_restrict __restrict
#else
# define _sha1_restrict __restrict__
#endif

#define INDIGO_PRINTF(...) if (!indigo_uni_printf(__VA_ARGS__)) goto failure

void sha1(unsigned char h[SHA1_SIZE], const void *_sha1_restrict p, size_t n);

static indigo_uni_handle *server_handle = NULL;
static bool startup_initiated = true;
static bool shutdown_initiated = false;
static int client_count = 0;
static pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static void (*server_callback)(int);

int indigo_server_tcp_port = 7624;
bool indigo_use_bonjour = true;
bool indigo_is_ephemeral_port = false;
bool indigo_use_blob_buffering = true;
bool indigo_use_blob_compression = false;

static pthread_mutex_t resource_list_mutex = PTHREAD_MUTEX_INITIALIZER;
// Serialize retirement so overlapping removals also wait for already withdrawn entries.
static pthread_mutex_t resource_retirement_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t resource_list_condition = PTHREAD_COND_INITIALIZER;

static struct resource {
	const char *path;
	unsigned char *data;
	unsigned length;
	const char *file_name;
	char *content_type;
	bool (*handler)(indigo_uni_handle *handle, char *method, char *path, char *params);
	unsigned users;
	struct resource *next;
} *resources = NULL;

// The registry owns metadata; workers retain entries until their response completes.
static struct resource *acquire_resource(const char *path) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = resources;
	while (resource) {
		if (!strncmp(resource->path, path, strlen(resource->path))) {
			resource->users++;
			break;
		}
		resource = resource->next;
	}
	pthread_mutex_unlock(&resource_list_mutex);
	return resource;
}

static void release_resource(struct resource *resource) {
	if (resource) {
		pthread_mutex_lock(&resource_list_mutex);
		if (--resource->users == 0) {
			pthread_cond_broadcast(&resource_list_condition);
		}
		pthread_mutex_unlock(&resource_list_mutex);
	}
}

// Called with the registry mutex held; condition waits let workers release their references.
static void retire_resource(struct resource *resource) {
	while (resource->users) {
		pthread_cond_wait(&resource_list_condition, &resource_list_mutex);
	}
	free((void *)resource->path);
	free((void *)resource->file_name);
	free(resource->content_type);
	free(resource);
}

#define BUFFER_SIZE	1024

// RFC 6455 GUID appended to Sec-WebSocket-Key before hashing
#define WEBSOCKET_GUID	"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#ifndef INDIGO_SERVER_INITIAL_TIMEOUT
#define INDIGO_SERVER_INITIAL_TIMEOUT 5000000L
#endif

static void client_count_change(int delta) {
	pthread_mutex_lock(&client_count_mutex);
	server_callback(client_count += delta);
	pthread_mutex_unlock(&client_count_mutex);
}

#ifndef INDIGO_BLOB_UPLOAD_PROGRESS_TIMEOUT
#define INDIGO_BLOB_UPLOAD_PROGRESS_TIMEOUT 5.0
#endif
#ifndef INDIGO_BLOB_UPLOAD_TOTAL_TIMEOUT
#define INDIGO_BLOB_UPLOAD_TOTAL_TIMEOUT 60.0
#endif

static bool upload_wait(indigo_uni_handle *handle, double deadline) {
	double remaining = deadline - indigo_monotonic_time();
	if (remaining <= 0) {
		return false;
	}
	if (remaining > INDIGO_BLOB_UPLOAD_PROGRESS_TIMEOUT) {
		remaining = INDIGO_BLOB_UPLOAD_PROGRESS_TIMEOUT;
	}
	return indigo_uni_wait_for_data(handle, (long)(remaining * 1000000)) > 0;
}

// Require an actual complete line; the generic serial line reader also returns partial lines.
static long upload_header_line(indigo_uni_handle *handle, char *buffer, size_t capacity, double deadline) {
	size_t length = 0;
	while (length + 1 < capacity) {
		char c;
		if (!upload_wait(handle, deadline) || indigo_uni_read_available(handle, &c, 1) != 1) {
			return -1;
		}
		if (c == '\n') {
			if (length && buffer[length - 1] == '\r') {
				length--;
			}
			buffer[length] = 0;
			return (long)length;
		}
		if (c == 0) {
			return -1;
		}
		buffer[length++] = c;
	}
	return -1;
}

static bool parse_upload_length(const char *text, size_t *size) {
	while (*text == ' ' || *text == '\t') {
		text++;
	}
	if (*text < '0' || *text > '9') {
		return false;
	}
	size_t value = 0;
	while (*text >= '0' && *text <= '9') {
		unsigned digit = *text++ - '0';
		if (value > (INDIGO_BLOB_UPLOAD_LIMIT - digit) / 10) {
			return false;
		}
		value = value * 10 + digit;
	}
	while (*text == ' ' || *text == '\t') {
		text++;
	}
	if (*text) {
		return false;
	}
	*size = value;
	return true;
}

static int receive_blob_upload(indigo_uni_handle *handle, indigo_blob_entry *entry) {
	if (!indigo_blob_upload_allowed(entry)) {
		return 403;
	}
	char header[BUFFER_SIZE];
	size_t size = 0;
	bool have_length = false;
	bool complete = false;
	double deadline = indigo_monotonic_time() + INDIGO_BLOB_UPLOAD_TOTAL_TIMEOUT;
	for (int i = 0; i < 64; i++) {
		long length = upload_header_line(handle, header, sizeof(header), deadline);
		if (length < 0) {
			return 400;
		}
		if (length == 0) {
			complete = true;
			break;
		}
		char *colon = strchr(header, ':');
		if (colon == NULL || colon == header) {
			return 400;
		}
		for (char *p = header; p < colon; p++) {
			if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || strchr("!#$%&'*+-.^_`|~", *p))) {
				return 400;
			}
		}
		*colon++ = 0;
		if (!strcasecmp(header, "Content-Length")) {
			if (have_length || !parse_upload_length(colon, &size)) {
				return 400;
			}
			have_length = true;
		} else if (!strcasecmp(header, "Transfer-Encoding")) {
			return 400;
		}
	}
	if (!complete || !have_length) {
		return 400;
	}
	void *buffer = indigo_allocate_blob_upload(size);
	if (buffer == NULL) {
		return 503;
	}
	size_t received = 0;
	int status = 200;
	int log_level = handle->log_level;
	handle->log_level = -abs(log_level);
	while (received < size) {
		if (!upload_wait(handle, deadline)) {
			status = 408;
			break;
		}
		long count = indigo_uni_read_available(handle, (char *)buffer + received, (long)(size - received));
		if (count <= 0) {
			status = 400;
			break;
		}
		received += count;
	}
	handle->log_level = log_level;
	if (status == 200 && !indigo_publish_blob_upload(entry, buffer, size)) {
		status = 404;
	}
	if (status != 200) {
		indigo_free_blob_upload(buffer, size);
	}
	return status;
}

static void start_worker_thread(indigo_uni_worker_data *data) {
	indigo_rename_thread("Server worker");
	indigo_uni_handle *handle = data->handle;
	INDIGO_TRACE(indigo_trace("%d <- // Worker thread started", handle->index));
	client_count_change(1);
	long res = 0;
	char c;
	void *free_on_exit = NULL;
	indigo_uni_handle *file_handle = NULL;
	indigo_blob_entry *release_at_exit = NULL;
	struct resource *resource_at_exit = NULL;
	if (indigo_uni_wait_for_data(handle, INDIGO_SERVER_INITIAL_TIMEOUT) > 0 && indigo_uni_peek_available(handle, &c, 1) == 1) {
		if (c == '<') {
			INDIGO_TRACE(indigo_trace("%d <- // Protocol switched to XML", handle->index));
			indigo_client *protocol_adapter = indigo_xml_device_adapter(data->handle, data->handle);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_xml_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
			indigo_release_xml_device_adapter(protocol_adapter);
		} else if (c == '{') {
			INDIGO_TRACE(indigo_trace("%d <- // Protocol switched to JSON", handle->index));
			indigo_client *protocol_adapter = indigo_json_device_adapter(data->handle, data->handle, false);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_json_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
			indigo_release_json_device_adapter(protocol_adapter);
		} else if (c == 'G' || c == 'P') {
			char request[BUFFER_SIZE];
			char header[BUFFER_SIZE];
			while ((res = indigo_uni_read_line(handle, request, sizeof(request) - 1)) > 0) {
				bool keep_alive = true;
				if (!strncmp(request, "GET /", 5)) {
					char *path = request + 4;
					char *space = strchr(path, ' ');
					if (space) {
						*space = 0;
					}
					char *params = strchr(path, '?');
					if (params) {
						*params++ = 0;
					}
					char websocket_key[256] = "";
					bool websocket_key_too_long = false;
					bool use_gzip = false;
					bool use_imagebytes = false;
					while (indigo_uni_read_line(handle, header, sizeof(header) - 1) > 0) {
						if (!strncasecmp(header, "Sec-WebSocket-Key: ", 19)) {
							// the GUID is appended in place before hashing, so the key must leave room for it and the NUL
							websocket_key_too_long = strlen(header + 19) >= sizeof(websocket_key) - strlen(WEBSOCKET_GUID);
							if (websocket_key_too_long) {
								*websocket_key = '\0';
							} else {
								strcpy(websocket_key, header + 19);
							}
						}
						if (!strcasecmp(header, "Connection: close"))
							keep_alive = false;
						if (!strncasecmp(header, "Accept-Encoding:", 16)) {
							if (strstr(header + 16, "gzip"))
								use_gzip = true;
						}
						if (!strncasecmp(header, "Accept:", 7)) {
							if (strstr(header + 7, "application/imagebytes"))
								use_imagebytes = true;
						}
					}
					if (!strcmp(path, "/")) {
						if (websocket_key_too_long) {
							INDIGO_PRINTF(handle, "HTTP/1.1 400 Bad Request\r\n");
							INDIGO_PRINTF(handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(handle, "\r\n");
							INDIGO_PRINTF(handle, "Sec-WebSocket-Key too long!\r\n");
							INDIGO_TRACE(indigo_trace("%d <- // Sec-WebSocket-Key too long", handle->index));
							goto failure;
						} else if (*websocket_key) {
							unsigned char shaHash[SHA1_SIZE];
							memset(shaHash, 0, sizeof(shaHash));
							strcat(websocket_key, WEBSOCKET_GUID);
							sha1(shaHash, websocket_key, strlen(websocket_key));
							INDIGO_PRINTF(handle, "HTTP/1.1 101 Switching Protocols\r\n");
							INDIGO_PRINTF(handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(handle, "Upgrade: websocket\r\n");
							INDIGO_PRINTF(handle, "Connection: upgrade\r\n");
							base64_encode((unsigned char *)websocket_key, shaHash, 20);
							INDIGO_PRINTF(handle, "Sec-WebSocket-Accept: %s\r\n", websocket_key);
							INDIGO_PRINTF(handle, "\r\n");
							INDIGO_TRACE(indigo_trace("%d <- // Protocol switched to JSON-over-WebSockets", handle->index));
							indigo_client *protocol_adapter = indigo_json_device_adapter(handle, handle, true);
							assert(protocol_adapter != NULL);
							indigo_attach_client(protocol_adapter);
							indigo_json_parse(NULL, protocol_adapter);
							indigo_detach_client(protocol_adapter);
							indigo_release_json_device_adapter(protocol_adapter);
						} else {
							INDIGO_PRINTF(handle, "HTTP/1.1 301 OK\r\n");
							INDIGO_PRINTF(handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(handle, "Location: /mng.html\r\n");
							INDIGO_PRINTF(handle, "Content-type: text/html\r\n");
							INDIGO_PRINTF(handle, "\r\n");
							INDIGO_PRINTF(handle, "<a href='/mng.html'>INDIGO Server Manager</a>");
						}
						keep_alive = false;
					} else if (!strncmp(path, "/blob/", 6)) {
						indigo_item *item;
						indigo_blob_entry *entry;
						if (sscanf(path, "/blob/%p.", &item) && (entry = release_at_exit = indigo_acquire_blob(item))) {
							indigo_item copy;
							bool copied = indigo_snapshot_blob(entry, &copy);
							indigo_release_blob(entry);
							release_at_exit = NULL;
							if (!copied) {
								INDIGO_PRINTF(handle, "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
								goto failure;
							}
							if (copy.blob.size == 0 && copy.blob.url[0]) {
								if (!indigo_populate_http_blob_item(&copy)) {
									free(copy.blob.value);
									INDIGO_PRINTF(handle, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
									goto failure;
								}
							}
							free_on_exit = copy.blob.value;
							long working_size = copy.blob.size;
							INDIGO_PRINTF(handle, "HTTP/1.1 200 OK\r\n");
							if (indigo_use_blob_buffering && use_gzip && indigo_use_blob_compression && strcmp(copy.blob.format, ".jpeg") && working_size > 0) {
								// Compression can expand incompressible bytes; leave room for deflate and gzip framing.
								size_t capacity = (size_t)working_size + (size_t)working_size / 1000 + 64;
								void *compressed = capacity <= UINT_MAX ? malloc(capacity) : NULL;
								unsigned compressed_size = (unsigned)capacity;
								if (compressed) {
									indigo_uni_compress("image", copy.blob.value, (unsigned)working_size, compressed, &compressed_size);
								}
								if (compressed && compressed_size > 0) {
									free(copy.blob.value);
									copy.blob.value = free_on_exit = compressed;
									INDIGO_PRINTF(handle, "Content-Encoding: gzip\r\n");
									INDIGO_PRINTF(handle, "X-Uncompressed-Content-Length: %ld\r\n", working_size);
									working_size = compressed_size;
								} else {
									free(compressed);
								}
							}
							INDIGO_PRINTF(handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							if (!strcmp(copy.blob.format, ".jpeg")) {
								INDIGO_PRINTF(handle, "Content-Type: image/jpeg\r\n");
							} else {
								INDIGO_PRINTF(handle, "Content-Type: application/octet-stream\r\n");
								INDIGO_PRINTF(handle, "Content-Disposition: attachment; filename=\"%p%s\"\r\n", item, copy.blob.format);
							}
							if (keep_alive) {
								INDIGO_PRINTF(handle, "Connection: keep-alive\r\n");
							}
							INDIGO_PRINTF(handle, "Content-Length: %ld\r\n\r\n", working_size);
							handle->log_level = -abs(handle->log_level);
							if (working_size && indigo_uni_write(handle, copy.blob.value, working_size) != working_size) {
								handle->log_level = abs(handle->log_level);
								goto failure;
							}
							handle->log_level = abs(handle->log_level);
							free(free_on_exit);
							free_on_exit = NULL;
						} else {
							INDIGO_PRINTF(handle, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
							goto failure;
						}
					} else {
						struct resource *resource = resource_at_exit = acquire_resource(path);
						if (resource == NULL) {
							INDIGO_PRINTF(handle, "HTTP/1.1 404 Not found\r\n");
							INDIGO_PRINTF(handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(handle, "\r\n");
							INDIGO_PRINTF(handle, "%s not found!\r\n", path);
							INDIGO_TRACE(indigo_trace("%d <- // %s not found", handle->index, path));
							goto failure;
						} else if (resource->handler) {
							keep_alive = resource->handler(handle, use_imagebytes ? "GET/IMAGEBYTES" : (use_gzip ? "GET/GZIP" : "GET"), path, params);
						} else if (resource->data) {
							INDIGO_PRINTF(handle, "HTTP/1.1 200 OK\r\n");
							INDIGO_PRINTF(handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(handle, "Content-Type: %s\r\n", resource->content_type);
							INDIGO_PRINTF(handle, "Content-Length: %d\r\n", resource->length);
							INDIGO_PRINTF(handle, "Content-Encoding: gzip\r\n");
							INDIGO_PRINTF(handle, "\r\n");
							handle->log_level = -abs(handle->log_level);
							if (indigo_uni_write(handle, (const char *)resource->data, resource->length) < 0) {
								handle->log_level = abs(handle->log_level);
								goto failure;
							}
							handle->log_level = abs(handle->log_level);
						} else if (resource->file_name) {
							char file_name[1024];
							if (*resource->file_name == '/') {
								strncpy(file_name, resource->file_name, sizeof(file_name) - 1);
								file_name[sizeof(file_name) - 1] = '\0';
							} else {
								snprintf(file_name, sizeof(file_name), "%s%c%s", indigo_uni_config_folder(), INDIGO_PATH_SEPATATOR, resource->file_name);
							}
							struct stat file_stat;
							if (stat(file_name, &file_stat) < 0 || (file_handle = indigo_uni_open_file(file_name, INDIGO_LOG_DEBUG)) == NULL) {
								INDIGO_PRINTF(handle, "HTTP/1.1 404 Not found\r\n");
								INDIGO_PRINTF(handle, "Content-Type: text/plain\r\n");
								INDIGO_PRINTF(handle, "\r\n");
								INDIGO_PRINTF(handle, "%s not found\r\n", file_name);
								INDIGO_TRACE(indigo_trace("%d <- // Failed to stat/open file %s", handle->index, file_name));
								goto failure;
							} else {
								const char *base_name = strrchr(file_name, '/');
								base_name = base_name ? base_name + 1 : file_name;
								INDIGO_PRINTF(handle, "HTTP/1.1 200 OK\r\n");
								INDIGO_PRINTF(handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
								INDIGO_PRINTF(handle, "Content-Type: %s\r\n", resource->content_type);
								INDIGO_PRINTF(handle, "Content-Disposition: attachment; filename=%s\r\n", base_name);
								INDIGO_PRINTF(handle, "Content-Length: %lld\r\n", (long long)file_stat.st_size);
								INDIGO_PRINTF(handle, "\r\n");
								long remaining = file_stat.st_size;
								char buffer[128 * 1024];
								while (remaining > 0) {
									long count = indigo_uni_read_available(file_handle, buffer, remaining < sizeof(buffer) ? remaining : sizeof(buffer));
									if (count <= 0) {
										goto failure;
									}
									handle->log_level = -abs(handle->log_level);
									if (indigo_uni_write(handle, buffer, count) != count) {
										handle->log_level = abs(handle->log_level);
										goto failure;
									}
									handle->log_level = abs(handle->log_level);
									remaining -= count;
								}
								indigo_uni_close(&file_handle);
							}
						}
					}
				} else if (!strncmp(request, "PUT /", 5)) {
					char *path = request + 4;
					char *space = strchr(path, ' ');
					if (space) {
						*space = 0;
					}
					if (!strncmp(path, "/blob/", 6)) {
						indigo_item *item;
						indigo_blob_entry *entry;
						if (sscanf(path, "/blob/%p.", &item) && (entry = release_at_exit = indigo_acquire_blob(item))) {
							int status = receive_blob_upload(handle, entry);
							indigo_release_blob(entry);
							release_at_exit = NULL;
							const char *reason = status == 200 ? "OK" : status == 408 ? "Request Timeout" : status == 404 ? "Not Found" : status == 403 ? "Forbidden" : status == 503 ? "Service Unavailable" : "Bad Request";
							INDIGO_PRINTF(handle, "HTTP/1.1 %d %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", status, reason);
						} else {
							INDIGO_PRINTF(handle, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
						}
						keep_alive = false;
					} else {
						struct resource *resource = resource_at_exit = acquire_resource(path);
						if (resource == NULL) {
							INDIGO_PRINTF(handle, "HTTP/1.1 404 Not found\r\n");
							INDIGO_PRINTF(handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(handle, "\r\n");
							INDIGO_PRINTF(handle, "%s not found!\r\n", path);
							INDIGO_TRACE(indigo_trace("%d <- // %s not found", handle->index, path));
							goto failure;
						} else if (resource->handler) {
							keep_alive = resource->handler(handle, "PUT", path, NULL);
						}
					}
				}
				release_resource(resource_at_exit);
				resource_at_exit = NULL;
				if (!keep_alive) {
					break;
				}
			}
		} else {
			INDIGO_TRACE(indigo_trace("%d -> // Unrecognised protocol", handle->index));
		}
	}
failure:
	indigo_uni_close(&file_handle);
	release_resource(resource_at_exit);
	INDIGO_TRACE(indigo_trace("%d <- // Worker thread finished", handle->index));
	indigo_uni_close(&handle);
	client_count_change(-1);
	indigo_safe_free(data);
	if (free_on_exit) {
		indigo_safe_free(free_on_exit);
	}
	if (release_at_exit) {
		indigo_release_blob(release_at_exit);
	}
}

void indigo_server_add_resource(const char *path, unsigned char *data, unsigned length, const char *content_type) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = indigo_safe_malloc(sizeof(struct resource));
	resource->path = indigo_safe_malloc_copy(strlen(path) + 1, (void *)path);
	resource->data = data;
	resource->length = length;
	resource->content_type = indigo_safe_malloc_copy(strlen(content_type) + 1, (void *)content_type);
	resource->handler = NULL;
	resource->next = resources;
	resources = resource;
	pthread_mutex_unlock(&resource_list_mutex);
	INDIGO_TRACE(indigo_trace("Resource %s (%d, %s) added", path, length, content_type));
}

void indigo_server_add_file_resource(const char *path, const char *file_name, const char *content_type) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = indigo_safe_malloc(sizeof(struct resource));
	resource->path = indigo_safe_malloc_copy(strlen(path) + 1, (void *)path);
	resource->file_name = indigo_safe_malloc_copy(strlen(file_name) + 1, (void *)file_name);
	resource->content_type = indigo_safe_malloc_copy(strlen(content_type) + 1, (void *)content_type);
	resource->handler = NULL;
	resource->next = resources;
	resources = resource;
	pthread_mutex_unlock(&resource_list_mutex);
	INDIGO_TRACE(indigo_trace("Resource %s (%s, %s) added", path, file_name, content_type));
}

void indigo_server_add_handler(const char *path, bool (*handler)(indigo_uni_handle *handle, char *method, char *path, char *params)) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = indigo_safe_malloc(sizeof(struct resource));
	resource->path = indigo_safe_malloc_copy(strlen(path) + 1, (void *)path);
	resource->file_name = NULL;
	resource->content_type = NULL;
	resource->handler = handler;
	resource->next = resources;
	resources = resource;
	pthread_mutex_unlock(&resource_list_mutex);
	INDIGO_TRACE(indigo_trace("Resource %s handler added", path));
}

void indigo_server_remove_resource(const char *path) {
	pthread_mutex_lock(&resource_retirement_mutex);
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = resources;
	struct resource *prev = NULL;
	while (resource) {
		if (!strcmp(resource->path, path)) {
			if (prev == NULL) {
				resources = resource->next;
			} else {
				prev->next = resource->next;
			}
			retire_resource(resource);
			pthread_mutex_unlock(&resource_list_mutex);
			pthread_mutex_unlock(&resource_retirement_mutex);
			INDIGO_TRACE(indigo_trace("Resource %s removed", path));
			return;
		}
		prev = resource;
		resource = resource->next;
	}
	pthread_mutex_unlock(&resource_list_mutex);
	pthread_mutex_unlock(&resource_retirement_mutex);
}

void indigo_server_remove_resources(void) {
	pthread_mutex_lock(&resource_retirement_mutex);
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = resources;
	resources = NULL;
	while (resource) {
		struct resource *next = resource->next;
		INDIGO_TRACE(indigo_trace("Resource %s removed", resource->path));
		retire_resource(resource);
		resource = next;
	}
	pthread_mutex_unlock(&resource_list_mutex);
	pthread_mutex_unlock(&resource_retirement_mutex);
}

void indigo_default_server_callback(int count) {
	static DNSServiceRef sd_http = NULL;
	static DNSServiceRef sd_indigo = NULL;
	static pthread_mutex_t dns_mutex = PTHREAD_MUTEX_INITIALIZER;
	if (startup_initiated) {
		startup_initiated = false;
		if (indigo_use_bonjour) {
#if defined(INDIGO_LINUX)
			/* UGLY but the only way to suppress compat mode warning messages on Linux */
			setenv("AVAHI_COMPAT_NOWARN", "1", 1);
#endif
			pthread_mutex_lock(&dns_mutex);
			int result = 0;
			if (!sd_http) {
				result = DNSServiceRegister(&sd_http, 0, 0, indigo_local_service_name, MDNS_HTTP_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
				if (result != kDNSServiceErr_NoError) {
					indigo_error("DNSServiceRegister -> %d", result);
				}
			}
			if (!sd_indigo) {
				result = DNSServiceRegister(&sd_indigo, 0, 0, indigo_local_service_name, MDNS_INDIGO_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
				if (result != kDNSServiceErr_NoError) {
					indigo_error("DNSServiceRegister -> %d", result);
				}
			}
			INDIGO_LOG(indigo_log("Service registered as %s", indigo_local_service_name));
			pthread_mutex_unlock(&dns_mutex);
		}
	} else if (shutdown_initiated) {
		pthread_mutex_lock(&dns_mutex);
		if (indigo_use_bonjour) {
			if (sd_indigo) {
				DNSServiceRefDeallocate(sd_indigo);
				sd_indigo = NULL;
			}
			if (sd_http) {
				DNSServiceRefDeallocate(sd_http);
				sd_http = NULL;
			}
			INDIGO_LOG(indigo_log("Service unregistered"));
		}
		pthread_mutex_unlock(&dns_mutex);
	} else {
		INDIGO_TRACE(indigo_trace("%d clients", count));
	}
}

indigo_result indigo_server_start(void (*callback)(int)) {
	indigo_rename_thread("Server listener");
	indigo_use_blob_caching = true;
	server_callback = callback ? callback : indigo_default_server_callback;
	shutdown_initiated = false;
	startup_initiated = true;
	indigo_is_ephemeral_port = indigo_server_tcp_port == 0;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	signal(SIGPIPE, SIG_IGN);
#endif
	indigo_uni_open_tcp_server_socket(&indigo_server_tcp_port, &server_handle, start_worker_thread, NULL, server_callback, INDIGO_LOG_TRACE);
	server_callback(0);
	return INDIGO_OK;
}

void indigo_server_shutdown() {
	if (!shutdown_initiated) {
		shutdown_initiated = true;
		indigo_uni_close(&server_handle);
	}
}

/*
 Copyright (c) 2014 Malte Hildingsson, malte (at) afterwi.se

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

static inline void sha1mix(unsigned *_sha1_restrict r, unsigned *_sha1_restrict w) {
	unsigned a = r[0];
	unsigned b = r[1];
	unsigned c = r[2];
	unsigned d = r[3];
	unsigned e = r[4];
	unsigned t, i = 0;

#define rol(x,s) ((x) << (s) | (unsigned) (x) >> (32 - (s)))
#define mix(f,v) do { \
t = (f) + (v) + rol(a, 5) + e + w[i & 0xf]; \
e = d; \
d = c; \
c = rol(b, 30); \
b = a; \
a = t; \
} while (0)

	for (; i < 16; ++i)
  mix(d ^ (b & (c ^ d)), 0x5a827999);

	for (; i < 20; ++i) {
		w[i & 0xf] = rol(w[i + 13 & 0xf] ^ w[i + 8 & 0xf] ^ w[i + 2 & 0xf] ^ w[i & 0xf], 1);
		mix(d ^ (b & (c ^ d)), 0x5a827999);
	}

	for (; i < 40; ++i) {
		w[i & 0xf] = rol(w[i + 13 & 0xf] ^ w[i + 8 & 0xf] ^ w[i + 2 & 0xf] ^ w[i & 0xf], 1);
		mix(b ^ c ^ d, 0x6ed9eba1);
	}

	for (; i < 60; ++i) {
		w[i & 0xf] = rol(w[i + 13 & 0xf] ^ w[i + 8 & 0xf] ^ w[i + 2 & 0xf] ^ w[i & 0xf], 1);
		mix((b & c) | (d & (b | c)), 0x8f1bbcdc);
	}

	for (; i < 80; ++i) {
		w[i & 0xf] = rol(w[i + 13 & 0xf] ^ w[i + 8 & 0xf] ^ w[i + 2 & 0xf] ^ w[i & 0xf], 1);
		mix(b ^ c ^ d, 0xca62c1d6);
	}

#undef mix
#undef rol

	r[0] += a;
	r[1] += b;
	r[2] += c;
	r[3] += d;
	r[4] += e;
}

void sha1(unsigned char h[SHA1_SIZE], const void *_sha1_restrict p, size_t n) {
	size_t i = 0;
	unsigned w[16], r[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};

	for (; i < (n & ~0x3f);) {
		do w[i >> 2 & 0xf] =
			(unsigned)((const unsigned char *) p)[i + 3] << 0x00 |
			(unsigned)((const unsigned char *) p)[i + 2] << 0x08 |
			(unsigned)((const unsigned char *) p)[i + 1] << 0x10 |
			(unsigned)((const unsigned char *) p)[i + 0] << 0x18;
		while ((i += 4) & 0x3f);
		sha1mix(r, w);
	}

	memset(w, 0, sizeof w);

	for (; i < n; ++i)
  w[i >> 2 & 0xf] |= (unsigned)((const unsigned char *) p)[i] << ((3 ^ (i & 3)) << 3);

	w[i >> 2 & 0xf] |= 0x80u << ((3 ^ (i & 3)) << 3);

	if ((n & 0x3f) > 56) {
		sha1mix(r, w);
		memset(w, 0, sizeof w);
	}

	w[15] = (unsigned) n << 3;
	sha1mix(r, w);

	for (i = 0; i < 5; ++i) {
		h[(i << 2) + 0] = (unsigned char) (r[i] >> 0x18);
		h[(i << 2) + 1] = (unsigned char) (r[i] >> 0x10);
		h[(i << 2) + 2] = (unsigned char) (r[i] >> 0x08);
		h[(i << 2) + 3] = (unsigned char) (r[i] >> 0x00);
	}
}
