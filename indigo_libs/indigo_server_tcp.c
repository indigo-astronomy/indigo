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

/** INDIGO TCP wire protocol server
 \file indigo_server_tcp.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <dns_sd.h>
#include <sys/stat.h>

#if defined(INDIGO_LINUX)
#include <signal.h>
#include <unistd.h>
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_server_tcp.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_driver_json.h>
#include <indigo/indigo_client_xml.h>
#include <indigo/indigo_base64.h>
#include <indigo/indigo_uni_io.h>

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
static void (*server_callback)(int);

int indigo_server_tcp_port = 7624;
bool indigo_use_bonjour = true;
bool indigo_is_ephemeral_port = false;
bool indigo_use_blob_buffering = true;
bool indigo_use_blob_compression = false;

static pthread_mutex_t resource_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct resource {
	const char *path;
	unsigned char *data;
	unsigned length;
	const char *file_name;
	char *content_type;
	bool (*handler)(indigo_uni_handle *handle, char *method, char *path, char *params);
	struct resource *next;
} *resources = NULL;

#define BUFFER_SIZE	1024

static void start_worker_thread(indigo_uni_worker_data *data) {
	indigo_uni_handle **handle = &data->handle;
	INDIGO_TRACE(indigo_trace("%d <- // Worker thread started", (*handle)->index));
	server_callback(++client_count);
	long res = 0;
	char c;
	void *free_on_exit = NULL;
	pthread_mutex_t *unlock_at_exit = NULL;
	if (indigo_uni_peek_available(*handle, &c, 1) == 1) {
		if (c == '<') {
			INDIGO_TRACE(indigo_trace("%d <- // Protocol switched to XML", (*handle)->index));
			indigo_client *protocol_adapter = indigo_xml_device_adapter(&data->handle, &data->handle);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_xml_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
			indigo_release_xml_device_adapter(protocol_adapter);
		} else if (c == '{') {
			INDIGO_TRACE(indigo_trace("%d <- // Protocol switched to JSON", (*handle)->index));
			indigo_client *protocol_adapter = indigo_json_device_adapter(&data->handle, &data->handle, false);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_json_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
			indigo_release_json_device_adapter(protocol_adapter);
		} else if (c == 'G' || c == 'P') {
			char request[BUFFER_SIZE];
			char header[BUFFER_SIZE];
			while ((res = indigo_uni_read_line(*handle, request, BUFFER_SIZE)) >= 0) {
				bool keep_alive = true;
				if (!strncmp(request, "GET /", 5)) {
					char *path = request + 4;
					char *space = strchr(path, ' ');
					if (space)
						*space = 0;
					char *params = strchr(path, '?');
					if (params)
						*params++ = 0;
					char websocket_key[256] = "";
					bool use_gzip = false;
					bool use_imagebytes = false;
					while (indigo_uni_read_line(*handle, header, BUFFER_SIZE) > 0) {
						if (!strncasecmp(header, "Sec-WebSocket-Key: ", 19))
							strncpy(websocket_key, header + 19, sizeof(websocket_key));
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
						if (*websocket_key) {
							unsigned char shaHash[SHA1_SIZE];
							memset(shaHash, 0, sizeof(shaHash));
							strcat(websocket_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
							sha1(shaHash, websocket_key, strlen(websocket_key));
							INDIGO_PRINTF(*handle, "HTTP/1.1 101 Switching Protocols\r\n");
							INDIGO_PRINTF(*handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(*handle, "Upgrade: websocket\r\n");
							INDIGO_PRINTF(*handle, "Connection: upgrade\r\n");
							base64_encode((unsigned char *)websocket_key, shaHash, 20);
							INDIGO_PRINTF(*handle, "Sec-WebSocket-Accept: %s\r\n", websocket_key);
							INDIGO_PRINTF(*handle, "\r\n");
							INDIGO_TRACE(indigo_trace("%d <- // Protocol switched to JSON-over-WebSockets", (*handle)->index));
							indigo_client *protocol_adapter = indigo_json_device_adapter(handle, handle, true);
							assert(protocol_adapter != NULL);
							indigo_attach_client(protocol_adapter);
							indigo_json_parse(NULL, protocol_adapter);
							indigo_detach_client(protocol_adapter);
							indigo_release_json_device_adapter(protocol_adapter);
						} else {
							INDIGO_PRINTF(*handle, "HTTP/1.1 301 OK\r\n");
							INDIGO_PRINTF(*handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(*handle, "Location: /mng.html\r\n");
							INDIGO_PRINTF(*handle, "Content-type: text/html\r\n");
							INDIGO_PRINTF(*handle, "\r\n");
							INDIGO_PRINTF(*handle, "<a href='/mng.html'>INDIGO Server Manager</a>");
						}
						keep_alive = false;
					} else if (!strncmp(path, "/blob/", 6)) {
						indigo_item *item;
						indigo_blob_entry *entry;
						if (sscanf(path, "/blob/%p.", &item) && (entry = indigo_validate_blob(item))) {
							pthread_mutex_lock(unlock_at_exit = &entry->mutext);
							long working_size = entry->size;
							if (working_size == 0) {
								assert(entry->content == NULL);
								indigo_item item_copy = *item;
								item_copy.blob.size = 0;
								item_copy.blob.value = NULL;
								if (indigo_populate_http_blob_item(&item_copy)) {
									working_size = entry->size = item_copy.blob.size;
									entry->content = item_copy.blob.value;
								} else {
									indigo_error("%d <- // Failed to populate BLOB", (*handle)->index);
								}
							}
							void *working_copy = indigo_use_blob_buffering ? (free_on_exit = malloc(working_size)) : entry->content;
							if (working_copy) {
								char working_format[INDIGO_NAME_SIZE];
								strcpy(working_format, entry->format);
								INDIGO_PRINTF(*handle, "HTTP/1.1 200 OK\r\n");
								if (indigo_use_blob_buffering) {
									if (use_gzip && indigo_use_blob_compression && strcmp(entry->format, ".jpeg")) {
										unsigned compressed_size = (unsigned)working_size;
										indigo_uni_compress("image", entry->content, (unsigned)working_size, working_copy, &compressed_size);
										INDIGO_PRINTF(*handle, "Content-Encoding: gzip\r\n");
										INDIGO_PRINTF(*handle, "X-Uncompressed-Content-Length: %ld\r\n", working_size);
										working_size = compressed_size;
									} else {
										memcpy(working_copy, entry->content, working_size);
									}
									pthread_mutex_unlock(&entry->mutext);
									unlock_at_exit = NULL;
								}
								INDIGO_PRINTF(*handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
								if (!strcmp(entry->format, ".jpeg")) {
									INDIGO_PRINTF(*handle, "Content-Type: image/jpeg\r\n");
								} else {
									INDIGO_PRINTF(*handle, "Content-Type: application/octet-stream\r\n");
									INDIGO_PRINTF(*handle, "Content-Disposition: attachment; filename=\"%p%s\"\r\n", item, working_format);
								}
								if (keep_alive)
									INDIGO_PRINTF(*handle, "Connection: keep-alive\r\n");
								INDIGO_PRINTF(*handle, "Content-Length: %ld\r\n", working_size);
								INDIGO_PRINTF(*handle, "\r\n");
								if (indigo_uni_write(*handle, working_copy, working_size) < 0) {
									goto failure;
								}
								if (indigo_use_blob_buffering) {
									free(working_copy);
									free_on_exit = NULL;
								} else {
									pthread_mutex_unlock(&entry->mutext);
									unlock_at_exit = NULL;
								}
							} else {
								pthread_mutex_unlock(&entry->mutext);
								unlock_at_exit = NULL;
								INDIGO_PRINTF(*handle, "HTTP/1.1 404 Not found\r\n");
								INDIGO_PRINTF(*handle, "Content-Type: text/plain\r\n");
								INDIGO_PRINTF(*handle, "\r\n");
								INDIGO_PRINTF(*handle, "Out of buffer memory!\r\n");
								INDIGO_TRACE(indigo_trace("%d <- // Out of buffer memory", (*handle)->index));
								goto failure;
							}
						} else {
							INDIGO_PRINTF(*handle, "HTTP/1.1 404 Not found\r\n");
							INDIGO_PRINTF(*handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(*handle, "\r\n");
							INDIGO_PRINTF(*handle, "BLOB not found!\r\n");
							INDIGO_TRACE(indigo_trace("%d <- // BLOB not found", (*handle)->index));
							goto failure;
						}
					} else {
						pthread_mutex_lock(&resource_list_mutex);
						struct resource *resource = resources;
						while (resource) {
							if (!strncmp(resource->path, path, strlen(resource->path)))
								break;
							resource = resource->next;
						}
						pthread_mutex_unlock(&resource_list_mutex);
						if (resource == NULL) {
							INDIGO_PRINTF(*handle, "HTTP/1.1 404 Not found\r\n");
							INDIGO_PRINTF(*handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(*handle, "\r\n");
							INDIGO_PRINTF(*handle, "%s not found!\r\n", path);
							INDIGO_TRACE(indigo_trace("%d <- // %s not found", (*handle)->index, path));
							goto failure;
						} else if (resource->handler) {
							keep_alive = resource->handler(*handle, use_imagebytes ? "GET/IMAGEBYTES" : (use_gzip ? "GET/GZIP" : "GET"), path, params);
						} else if (resource->data) {
							INDIGO_PRINTF(*handle, "HTTP/1.1 200 OK\r\n");
							INDIGO_PRINTF(*handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							INDIGO_PRINTF(*handle, "Content-Type: %s\r\n", resource->content_type);
							INDIGO_PRINTF(*handle, "Content-Length: %d\r\n", resource->length);
							INDIGO_PRINTF(*handle, "Content-Encoding: gzip\r\n");
							INDIGO_PRINTF(*handle, "\r\n");
							indigo_uni_write(*handle, (const char *)resource->data, resource->length);
							INDIGO_TRACE(indigo_trace("%d <- // %d bytes", (*handle)->index, resource->length));
						} else if (resource->file_name) {
							char file_name[256];
							if (*resource->file_name == '/') {
								strcpy(file_name, resource->file_name);
							} else {
								sprintf(file_name, "%s%c%s", indigo_uni_config_folder(), INDIGO_PATH_SEPATATOR, resource->file_name);
							}
							indigo_uni_handle *file_handle = { 0 };
							struct stat file_stat;
							if (stat(file_name, &file_stat) < 0 || (file_handle = indigo_uni_open_file(file_name, INDIGO_LOG_DEBUG)) == NULL) {
								INDIGO_PRINTF(file_handle, "HTTP/1.1 404 Not found\r\n");
								INDIGO_PRINTF(file_handle, "Content-Type: text/plain\r\n");
								INDIGO_PRINTF(file_handle, "\r\n");
								INDIGO_PRINTF(file_handle, "%s not found\r\n", file_name);
								INDIGO_TRACE(indigo_trace("%d <- // Failed to stat/open file %s", (*handle)->index, file_name));
								goto failure;
							} else {
								const char *base_name = strrchr(file_name, '/');
								base_name = base_name ? base_name + 1 : file_name;
								INDIGO_PRINTF(file_handle, "HTTP/1.1 200 OK\r\n");
								INDIGO_PRINTF(file_handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
								INDIGO_PRINTF(file_handle, "Content-Type: %s\r\n", resource->content_type);
								INDIGO_PRINTF(file_handle, "Content-Disposition: attachment; filename=%s\r\n", base_name);
								INDIGO_PRINTF(file_handle, "Content-Length: %d\r\n", file_stat.st_size);
								INDIGO_PRINTF(file_handle, "\r\n");
								long remaining = file_stat.st_size;
								char buffer[128 * 1024];
								while (remaining > 0) {
									long count = indigo_uni_read_available(file_handle, buffer, remaining < sizeof(buffer) ? remaining : sizeof(buffer));
									if (count < 0) {
										break;
									}
									if (indigo_uni_write(*handle, buffer, count) < 0) {
										goto failure;
									}
									remaining -= count;
								}
								indigo_uni_close(&file_handle);
							}
						}
					}
				} else if (!strncmp(request, "PUT /", 5)) {
					char *path = request + 4;
					char *space = strchr(path, ' ');
					if (space)
						*space = 0;
					if (!strncmp(path, "/blob/", 6)) {
						indigo_item *item;
						indigo_blob_entry *entry;
						if (sscanf(path, "/blob/%p.", &item) && (entry = indigo_validate_blob(item))) {
							int content_length = 0;
							char header[BUFFER_SIZE];
							while (indigo_uni_read_line(*handle, header, INDIGO_BUFFER_SIZE) > 0) {
								if (!strncasecmp(header, "Content-Length:", 15)) {
									content_length = atoi(header + 15);
								}
							}
							pthread_mutex_lock(unlock_at_exit = &entry->mutext);
							entry->content = indigo_safe_realloc(entry->content, entry->size = content_length);
							if (entry->content) {
								if (!indigo_uni_read(*handle, entry->content, content_length)) {
									goto failure;
								}
								pthread_mutex_unlock(&entry->mutext);
								unlock_at_exit = NULL;
								INDIGO_PRINTF(*handle, "HTTP/1.1 200 OK\r\n");
								INDIGO_PRINTF(*handle, "Server: INDIGO/%d.%d-%s\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
								INDIGO_PRINTF(*handle, "Content-Length: 0\r\n");
								INDIGO_PRINTF(*handle, "\r\n");
							} else {
								INDIGO_PRINTF(*handle, "HTTP/1.1 404 Not found\r\n");
								INDIGO_PRINTF(*handle, "Content-Type: text/plain\r\n");
								INDIGO_PRINTF(*handle, "\r\n");
								INDIGO_PRINTF(*handle, "Out of buffer memory!\r\n");
								INDIGO_TRACE(indigo_trace("%d <- // Out of buffer memory", (*handle)->index));
								goto failure;
							}
						} else {
							INDIGO_PRINTF(*handle, "HTTP/1.1 404 Not found\r\n");
							INDIGO_PRINTF(*handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(*handle, "\r\n");
							INDIGO_PRINTF(*handle, "BLOB not found!\r\n");
							INDIGO_TRACE(indigo_trace("%d <- // BLOB not found", (*handle)->index));
							goto failure;
						}
					} else {
						pthread_mutex_lock(&resource_list_mutex);
						struct resource *resource = resources;
						while (resource) {
							if (!strncmp(resource->path, path, strlen(resource->path)))
								break;
							resource = resource->next;
						}
						pthread_mutex_unlock(&resource_list_mutex);
						if (resource == NULL) {
							INDIGO_PRINTF(*handle, "HTTP/1.1 404 Not found\r\n");
							INDIGO_PRINTF(*handle, "Content-Type: text/plain\r\n");
							INDIGO_PRINTF(*handle, "\r\n");
							INDIGO_PRINTF(*handle, "%s not found!\r\n", path);
							INDIGO_TRACE(indigo_trace("%d <- // %s not found", (*handle)->index, path));
							goto failure;
						} else if (resource->handler) {
							keep_alive = resource->handler(*handle, "PUT", path, NULL);
						}
					}
				}
				if (!keep_alive) {
					break;
				}
			}
		} else {
			INDIGO_TRACE(indigo_trace("%d -> // Unrecognised protocol", (*handle)->index));
		}
	}
failure:
	if ((*handle) != NULL) {
		INDIGO_TRACE(indigo_trace("%d <- // Worker thread finished", (*handle)->index));
		indigo_uni_close(handle);
	}
	server_callback(--client_count);
	indigo_safe_free(data);
	if (free_on_exit) {
		indigo_safe_free(free_on_exit);
	}
	if (unlock_at_exit) {
		pthread_mutex_unlock(unlock_at_exit);
	}
}

void indigo_server_add_resource(const char *path, unsigned char *data, unsigned length, const char *content_type) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = indigo_safe_malloc(sizeof(struct resource));
	resource->path = path;
	resource->data = data;
	resource->length = length;
	resource->content_type = (char *)content_type;
	resource->handler = NULL;
	resource->next = resources;
	resources = resource;
	pthread_mutex_unlock(&resource_list_mutex);
	INDIGO_TRACE(indigo_trace("Resource %s (%d, %s) added", path, length, content_type));
}

void indigo_server_add_file_resource(const char *path, const char *file_name, const char *content_type) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = indigo_safe_malloc(sizeof(struct resource));
	resource->path = path;
	resource->file_name = file_name;
	resource->content_type = (char *)content_type;
	resource->handler = NULL;
	resource->next = resources;
	resources = resource;
	pthread_mutex_unlock(&resource_list_mutex);
	INDIGO_TRACE(indigo_trace("Resource %s (%s, %s) added", path, file_name, content_type));
}

void indigo_server_add_handler(const char *path, bool (*handler)(indigo_uni_handle *handle, char *method, char *path, char *params)) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = indigo_safe_malloc(sizeof(struct resource));
	resource->path = path;
	resource->file_name = NULL;
	resource->content_type = NULL;
	resource->handler = handler;
	resource->next = resources;
	resources = resource;
	pthread_mutex_unlock(&resource_list_mutex);
	INDIGO_TRACE(indigo_trace("Resource %s handler added", path));
}

void indigo_server_remove_resource(const char *path) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = resources;
	struct resource *prev = NULL;
	while (resource) {
		if (!strcmp(resource->path, path)) {
			if (prev == NULL)
				resources = resource->next;
			else
				prev->next = resource->next;
			free(resource);
			pthread_mutex_unlock(&resource_list_mutex);
			INDIGO_TRACE(indigo_trace("Resource %s removed", path));
			return;
		}
		prev = resource;
		resource = resource->next;
	}
	pthread_mutex_unlock(&resource_list_mutex);
}

void indigo_server_remove_resources(void) {
	pthread_mutex_lock(&resource_list_mutex);
	struct resource *resource = resources;
	while (resource) {
		struct resource *tmp = resource;
		resource = resource->next;
		INDIGO_TRACE(indigo_trace("Resource %s removed", tmp->path));
		free(tmp);
	}
	resources = NULL;
	pthread_mutex_unlock(&resource_list_mutex);
}

static void default_server_callback(int count) {
	static DNSServiceRef sd_http;
	static DNSServiceRef sd_indigo;
	if (startup_initiated) {
		startup_initiated = false;
		if (indigo_use_bonjour) {
#if defined(INDIGO_LINUX)
			/* UGLY but the only way to suppress compat mode warning messages on Linux */
			setenv("AVAHI_COMPAT_NOWARN", "1", 1);
#endif
			int result = DNSServiceRegister(&sd_http, 0, 0, indigo_local_service_name, MDNS_HTTP_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
			if (result != kDNSServiceErr_NoError) {
				indigo_error("DNSServiceRegister -> %d", result);
			}
			result = DNSServiceRegister(&sd_indigo, 0, 0, indigo_local_service_name, MDNS_INDIGO_TYPE, NULL, NULL, htons(indigo_server_tcp_port), 0, NULL, NULL, NULL);
			if (result != kDNSServiceErr_NoError) {
				indigo_error("DNSServiceRegister -> %d", result);
			}
			INDIGO_LOG(indigo_log("Service registered as %s", indigo_local_service_name));
		}
	} else if (shutdown_initiated) {
		if (indigo_use_bonjour) {
			DNSServiceRefDeallocate(sd_indigo);
			DNSServiceRefDeallocate(sd_http);
		}
		INDIGO_LOG(indigo_log("Service unregistered"));
	} else {
		INDIGO_TRACE(indigo_trace("%d clients", count));
	}
}

indigo_result indigo_server_start(void (*callback)(int)) {
	indigo_use_blob_caching = true;
	server_callback = callback ? callback : default_server_callback;
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
			((const unsigned char *) p)[i + 3] << 0x00 |
			((const unsigned char *) p)[i + 2] << 0x08 |
			((const unsigned char *) p)[i + 1] << 0x10 |
			((const unsigned char *) p)[i + 0] << 0x18;
		while ((i += 4) & 0x3f);
		sha1mix(r, w);
	}

	memset(w, 0, sizeof w);

	for (; i < n; ++i)
  w[i >> 2 & 0xf] |= ((const unsigned char *) p)[i] << ((3 ^ (i & 3)) << 3);

	w[i >> 2 & 0xf] |= 0x80 << ((3 ^ (i & 3)) << 3);

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
