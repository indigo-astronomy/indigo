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

/** INDIGO TCP wire protocol server
 \file indigo_server_tcp.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "indigo_server_tcp.h"
#include "indigo_driver_xml.h"
#include "indigo_driver_json.h"
#include "indigo_client_xml.h"
#include "indigo_base64.h"
#include "indigo_io.h"

#define SHA1_SIZE 20
#if _MSC_VER
# define _sha1_restrict __restrict
#else
# define _sha1_restrict __restrict__
#endif

void sha1(unsigned char h[static SHA1_SIZE], const void *_sha1_restrict p, size_t n);

static int server_socket;
static struct sockaddr_in server_address;
static bool shutdown_initiated = false;
static int client_count = 0;
static indigo_server_tcp_callback server_callback;

int indigo_server_tcp_port = 7624;

static struct resource {
	char *path;
	unsigned char *data;
	unsigned length;
	char *content_type;
	struct resource *next;
} *resources = NULL;

#define BUFFER_SIZE	1024

static void start_worker_thread(int *client_socket) {
	int socket = *client_socket;
	INDIGO_LOG(indigo_log("Worker thread started socket = %d", socket));
	server_callback(++client_count);
	char c;
	if (recv(socket, &c, 1, MSG_PEEK) == 1) {
		if (c == '<') {
			INDIGO_LOG(indigo_log("Protocol switched to XML"));
			indigo_client *protocol_adapter = indigo_xml_device_adapter(socket, socket);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_xml_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
			indigo_release_xml_device_adapter(protocol_adapter);
		} else if (c == '{') {
			INDIGO_LOG(indigo_log("Protocol switched to JSON"));
			indigo_client *protocol_adapter = indigo_json_device_adapter(socket, socket, false);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_json_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
			indigo_release_json_device_adapter(protocol_adapter);
		} else if (c == 'G') {
			char request[BUFFER_SIZE];
			char header[BUFFER_SIZE];
			while (indigo_read_line(socket, request, BUFFER_SIZE)) {
				if (!strncmp(request, "GET /", 5)) {
					char *path = request + 4;
					char *space = strchr(path, ' ');
					if (space)
						*space = 0;
					char websocket_key[256] = "";
					bool keep_alive = false;
					while (indigo_read_line(socket, header, BUFFER_SIZE)) {
						if (*header == 0)
							break;
						if (!strncasecmp(header, "Sec-WebSocket-Key: ", 19))
							strcpy(websocket_key, header + 19);
						if (!strcasecmp(header, "Connection: keep-alive"))
							keep_alive = true;
					}
					if (!strcmp(path, "/")) {
						if (*websocket_key) {
							unsigned char shaHash[20];
							memset(shaHash, 0, sizeof(shaHash));
							strcat(websocket_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
							sha1(shaHash, websocket_key, strlen(websocket_key));
							indigo_printf(socket, "HTTP/1.1 101 Switching Protocols\r\n");
							indigo_printf(socket, "Server: INDIGO/%d.%d-%d\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							indigo_printf(socket, "Upgrade: websocket\r\n");
							indigo_printf(socket, "Connection: upgrade\r\n");
							base64_encode((unsigned char *)websocket_key, shaHash, 20);
							indigo_printf(socket, "Sec-WebSocket-Accept: %s\r\n", websocket_key);
							indigo_printf(socket, "\r\n");
							INDIGO_LOG(indigo_log("Protocol switched to JSON-over-WebSockets"));
							indigo_client *protocol_adapter = indigo_json_device_adapter(socket, socket, true);
							assert(protocol_adapter != NULL);
							indigo_attach_client(protocol_adapter);
							indigo_json_parse(NULL, protocol_adapter);
							indigo_detach_client(protocol_adapter);
							break;
						} else {
							indigo_printf(socket, "HTTP/1.1 301 OK\r\n");
							indigo_printf(socket, "Server: INDIGO/%d.%d-%d\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							indigo_printf(socket, "Location: /ctrl\r\n");
							indigo_printf(socket, "Content-type: text/html\r\n");
							indigo_printf(socket, "\r\n");
							indigo_printf(socket, "<a href='/ctrl'>INDIGO Control Panel</a>");
							break;
						}
					} else {
						if (!strncmp(path, "/blob/", 6)) {
							indigo_item *item;
							if (sscanf(path, "/blob/%p.", &item) && indigo_validate_blob(item) == INDIGO_OK) {
								indigo_printf(socket, "HTTP/1.1 200 OK\r\n");
								indigo_printf(socket, "Server: INDIGO/%d.%d-%d\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
								if (!strcmp(item->blob.format, ".jpeg")) {
									indigo_printf(socket, "Content-Type: image/jpeg\r\n");
								} else {
									indigo_printf(socket, "Content-Type: application/octet-stream\r\n");
									indigo_printf(socket, "Content-Disposition: attachment; filename=\"%p%s\"\r\n", item, item->blob.format);
								}
								if (keep_alive)
									indigo_printf(socket, "Connection: keep-alive\r\n");
								indigo_printf(socket, "Content-Length: %ld\r\n", item->blob.size);
								indigo_printf(socket, "\r\n");
								indigo_write(socket, item->blob.value, item->blob.size);
								INDIGO_LOG(indigo_log("%s -> OK (%ld bytes)\r\n", request, item->blob.size));
							} else {
								indigo_printf(socket, "HTTP/1.1 404 Not found\r\n");
								indigo_printf(socket, "Content-Type: text/plain\r\n");
								indigo_printf(socket, "\r\n");
								indigo_printf(socket, "BLOB not found!\r\n");
								shutdown(socket,SHUT_RDWR);
								sleep(1);
								close(socket);
								INDIGO_LOG(indigo_log("%s -> Failed", request));
								break;
							}
						} else {
							struct resource *resource = resources;
							while (resource != NULL)
								if (!strcmp(resource->path, path))
									break;
								else
									resource = resource->next;
							if (resource == NULL) {
								indigo_printf(socket, "HTTP/1.1 404 Not found\r\n");
								indigo_printf(socket, "Content-Type: text/plain\r\n");
								indigo_printf(socket, "\r\n");
								indigo_printf(socket, "%s not found!\r\n", path);
								shutdown(socket,SHUT_RDWR);
								sleep(1);
								close(socket);
								INDIGO_LOG(indigo_log("%s -> Failed", request));
								break;
							} else {
								keep_alive = false;
								indigo_printf(socket, "HTTP/1.1 200 OK\r\n");
								indigo_printf(socket, "Server: INDIGO/%d.%d-%d\r\n", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
								if (keep_alive)
									indigo_printf(socket, "Connection: keep-alive\r\n");
								indigo_printf(socket, "Content-Type: %s\r\n", resource->content_type);
								indigo_printf(socket, "Content-Length: %d\r\n", resource->length);
								indigo_printf(socket, "Content-Encoding: gzip\r\n");
								indigo_printf(socket, "\r\n");
								indigo_write(socket, (const char *)resource->data, resource->length);
								INDIGO_LOG(indigo_log("%s -> OK (%d bytes)", request, resource->length));
							}
						}
						if (!keep_alive) {
							shutdown(socket,SHUT_RDWR);
							sleep(1);
							close(socket);
							break;
						}
					}
				}
				else if (*request == 0) {
					shutdown(socket,SHUT_RDWR);
					sleep(1);
					close(socket);
					break;
				}
			}
		} else {
			INDIGO_LOG(indigo_log("Unrecognised protocol"));
		}
	}
	server_callback(--client_count);
	free(client_socket);
	INDIGO_LOG(indigo_log("Worker thread finished"));
}

void indigo_server_shutdown() {
	if (!shutdown_initiated) {
		shutdown_initiated = true;
		shutdown(server_socket, SHUT_RDWR);
		close(server_socket);
	}
}

void indigo_server_add_resource(char *path, unsigned char *data, unsigned length, char *content_type) {
	INDIGO_LOG(indigo_log("Resource %s (%d, %s) added", path, length, content_type));
	struct resource *resource = malloc(sizeof(struct resource));
	resource->path = path;
	resource->data = data;
	resource->length = length;
	resource->content_type = content_type;
	resource->next = resources;
	resources = resource;
}

indigo_result indigo_server_start(indigo_server_tcp_callback callback) {
	server_callback = callback;
	int client_socket;
	int reuse = 1;
	struct sockaddr_in client_name;
	unsigned int name_len = sizeof(client_name);
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
		indigo_error("Can't open server socket (%s)", strerror(errno));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(indigo_server_tcp_port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(server_socket, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		indigo_error("Can't setsockopt for server socket (%s)", strerror(errno));
		return INDIGO_CANT_START_SERVER;
	}
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		indigo_error("Can't bind server socket (%s)", strerror(errno));
		return INDIGO_CANT_START_SERVER;
	}
	unsigned int length = sizeof(server_address);
	if (getsockname(server_socket, (struct sockaddr *)&server_address, &length) == -1) {
		close(server_socket);
		return INDIGO_CANT_START_SERVER;
	}
	if (listen(server_socket, 5) < 0) {
		indigo_error("Can't listen on server socket (%s)", strerror(errno));
		close(server_socket);
		return INDIGO_CANT_START_SERVER;
	}
	indigo_server_tcp_port = ntohs(server_address.sin_port);
	INDIGO_LOG(indigo_log("Server started on %d", indigo_server_tcp_port));
	server_callback(client_count);
	signal(SIGPIPE, SIG_IGN);
	while (1) {
		client_socket = accept(server_socket, (struct sockaddr *)&client_name, &name_len);
		if (client_socket == -1) {
			if (shutdown_initiated)
				break;
			indigo_error("Can't accept connection (%s)", strerror(errno));
		} else {
			pthread_t thread;
			int *pointer = malloc(sizeof(int));
			*pointer = client_socket;
			if (pthread_create(&thread , NULL, (void *(*)(void *))&start_worker_thread, pointer) != 0)
				indigo_error("Can't create worker thread for connection (%s)", strerror(errno));
		}
	}
	shutdown_initiated = false;
	return INDIGO_OK;
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

void sha1(unsigned char h[static SHA1_SIZE], const void *_sha1_restrict p, size_t n) {
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

	for (i = 0; i < 5; ++i)
  h[(i << 2) + 0] = (unsigned char) (r[i] >> 0x18),
  h[(i << 2) + 1] = (unsigned char) (r[i] >> 0x10),
  h[(i << 2) + 2] = (unsigned char) (r[i] >> 0x08),
  h[(i << 2) + 3] = (unsigned char) (r[i] >> 0x00);
}
