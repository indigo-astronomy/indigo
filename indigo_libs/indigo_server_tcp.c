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
#include "indigo_base64.h"

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
char indigo_server_document_root[INDIGO_VALUE_SIZE] = "";

#define BUFFER_SIZE	1024

static long read_line(int handle, char *buffer, int length) {
	int i = 0;
	char c = '\0';
	long n = 0;
	while (i < length) {
		n = read(handle, &c, 1);
		if (n > 0) {
			if (c == '\r') {
			} else if (c == '\n') {
				buffer[i++] = 0;
				break;
			} else {
				buffer[i++] = c;
			}
		} else {
			break;
		}
	}
	buffer[i] = '\0';
	return n == -1 ? -1 : i;
}

static void write_line(int handle, char *format, ...) {
	char buf[1024];
	va_list args;
	va_start (args, format);
	vsnprintf(buf, 1024, format, args);
	va_end (args);
	strncat(buf, "\r\n", 1024);
	write(handle, buf, strlen(buf));
}

static void start_worker_thread(int *client_socket) {
	int socket = *client_socket;
	indigo_log("Worker thread started");
	server_callback(++client_count);
	char c;
	if (recv(socket, &c, 1, MSG_PEEK) == 1) {
		if (c == '<') {
			indigo_log("Protocol switched to XML");
			indigo_client *protocol_adapter = indigo_xml_device_adapter(socket, socket);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_xml_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
		} else if (c == '{') {
			indigo_log("Protocol switched to JSON");
			indigo_client *protocol_adapter = indigo_json_device_adapter(socket, socket, false);
			assert(protocol_adapter != NULL);
			indigo_attach_client(protocol_adapter);
			indigo_json_parse(NULL, protocol_adapter);
			indigo_detach_client(protocol_adapter);
		} else if (c == 'G') {
			char buffer[BUFFER_SIZE];
			while (read_line(socket, buffer, BUFFER_SIZE) >= 0) {
				indigo_debug("%s", buffer);
				if (strstr(buffer, "..")) {
					close(socket);
					break;
				}
				if (!strncmp(buffer, "GET / ", 6)) {
					char response[256];
					unsigned char shaHash[20];
					while (read_line(socket, buffer, BUFFER_SIZE) >= 0) {
						if (*buffer == 0)
							break;
						if (!strncmp(buffer, "Sec-WebSocket-Key: ", 19)) {
							strcpy(response, buffer + 19);
						}
					}
					strcat(response, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
					memset(shaHash, 0, sizeof(shaHash));
					sha1(shaHash, response, strlen(response));
					write_line(socket, "HTTP/1.1 101 Switching Protocols");
					write_line(socket, "Server: INDIGO/%d.%d-%d", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
					write_line(socket, "Upgrade: websocket");
					write_line(socket, "Connection: upgrade");
					base64_encode((unsigned char *)response, shaHash, 20);
					write_line(socket, "Sec-WebSocket-Accept: %s", response);
					write_line(socket, "");
					indigo_log("Protocol switched to JSON-over-WebSockets");
					indigo_client *protocol_adapter = indigo_json_device_adapter(socket, socket, true);
					assert(protocol_adapter != NULL);
					indigo_attach_client(protocol_adapter);
					indigo_json_parse(NULL, protocol_adapter);
					indigo_detach_client(protocol_adapter);
					break;
				} else {
					bool keep_alive = false;
					char header[BUFFER_SIZE];
					while (read_line(socket, header, BUFFER_SIZE) >= 0) {
						if (*header == 0)
							break;
						if (!strcasecmp(header, "Connection: keep-alive"))
							keep_alive = true;
						INDIGO_DEBUG(indigo_debug("%s", header));
					}
					if (!strncmp(buffer, "GET /blob/", 10)) {
						indigo_item *item;
						if (sscanf(buffer, "GET /blob/%p.", &item) && indigo_validate_blob(item) == INDIGO_OK) {
							write_line(socket, "HTTP/1.1 200 OK");
							write_line(socket, "Server: INDIGO/%d.%d-%d", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							if (!strcmp(item->blob.format, ".jpeg")) {
								write_line(socket, "Content-Type: image/jpeg");
							} else {
								write_line(socket, "Content-Type: application/octet-stream");
								write_line(socket, "Content-Disposition: attachment; filename=\"%p.%s\"", item, item->blob.format);
							}
							if (keep_alive)
								write_line(socket, "Connection: keep-alive");
							write_line(socket, "Content-Length: %ld", item->blob.size);
							write_line(socket, "");
							write(socket, item->blob.value, item->blob.size);
						} else {
							write_line(socket, "HTTP/1.1 404 Not found");
							write_line(socket, "Content-Type: text/plain");
							write_line(socket, "");
							write_line(socket, "BLOB not found!");
							close(socket);
							break;
						}
					} else {
						char file_name[INDIGO_VALUE_SIZE];
						char *space = strchr(buffer + 4, ' ');
						if (space)
							*space = 0;
						strncpy(file_name, indigo_server_document_root, INDIGO_VALUE_SIZE);
						strncat(file_name, buffer + 4, INDIGO_VALUE_SIZE);
						char block[32 * 1024];
						int file = open(file_name, O_RDONLY);
						if (file < 0) {
							write_line(socket, "HTTP/1.1 404 Not found");
							write_line(socket, "Content-Type: text/plain");
							write_line(socket, "");
							write_line(socket, "%s not found!", file_name);
							close(socket);
							break;
						} else {
							long size = lseek(file, 0L, SEEK_END);
							lseek(file, 0L, SEEK_SET);
							write_line(socket, "HTTP/1.1 200 OK");
							write_line(socket, "Server: INDIGO/%d.%d-%d", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF, INDIGO_BUILD);
							if (keep_alive)
								write_line(socket, "Connection: keep-alive");
							if (strstr(file_name, ".html"))
								write_line(socket, "Content-Type: text/html");
							else if (strstr(file_name, ".css"))
								write_line(socket, "Content-Type: text/css");
							else if (strstr(file_name, ".js"))
								write_line(socket, "Content-Type: application/javascript");
							else if (strstr(file_name, ".jpeg"))
								write_line(socket, "Content-Type: image/jpeg");
							write_line(socket, "Content-Length: %ld", size);
							write_line(socket, "");
							long bytes_read, bytes_written;
							while (size > 0) {
								bytes_read = read(file, block, 32 * 1024);
								size -= bytes_read;
								char *data = block;
								while (bytes_read > 0) {
									bytes_written = write(socket, data, bytes_read);
									bytes_read -= bytes_written;
									data += bytes_written;
								}
							}
						}
					}
					if (!keep_alive) {
						close(socket);
						break;
					}
				}
			}
		} else {
			indigo_log("Unrecognised protocol");
		}
	}
	server_callback(--client_count);
	free(client_socket);
	indigo_log("Worker thread finished");
}

static void server_shutdown() {
	shutdown_initiated = true;
	close(server_socket);
}

indigo_result indigo_server_tcp(indigo_server_tcp_callback callback) {
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
	if (listen(server_socket, 5) < 0) {
		indigo_error("Can't listen on server socket (%s)", strerror(errno));
		return INDIGO_CANT_START_SERVER;
	}
	indigo_log("Server started on %d", indigo_server_tcp_port);
	if (*indigo_server_document_root)
		indigo_log("Document root %s", indigo_server_document_root);
	atexit(server_shutdown);
	callback(client_count);
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
	return shutdown_initiated ? INDIGO_OK : INDIGO_FAILED;
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
