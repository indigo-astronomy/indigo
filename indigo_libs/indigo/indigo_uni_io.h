// Copyright (c) 2025 CloudMakers, s. r. o.
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

/** INDIGO Unified I/O
 \file indigo_uni_io.h
 */

#ifndef indigo_uni_io_h
#define indigo_uni_io_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <sys/socket.h>
#define INDIGO_PATH_SEPATATOR	'/'
#elif defined(INDIGO_WINDOWS)
#include <winsock2.h>
#define INDIGO_PATH_SEPATATOR	'\\'
#define strcasecmp stricmp
#define stat _stat
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	INDIGO_FILE_HANDLE = 0,
	INDIGO_TCP_HANDLE = 1,
	INDIGO_UDP_HANDLE = 2
} indigo_uni_handle_type;

typedef struct {
	indigo_uni_handle_type type;
	int fd;
	int last_error;
} indigo_uni_handle;

typedef struct {
	indigo_uni_handle *handle;
	void *data;
} indigo_uni_worker_data;

extern indigo_uni_handle indigo_stdin_handle;
extern indigo_uni_handle indigo_stdout_handle;
extern indigo_uni_handle indigo_stderr_handle;

extern indigo_uni_handle *indigo_uni_create_handle(indigo_uni_handle_type type, int fd);

/** Decode last error to string.
 */
extern char *indigo_uni_strerror(indigo_uni_handle *handle);

/** Open existing file.
 */
extern indigo_uni_handle *indigo_uni_open_file(const char *path);

/** Create new file.
 */
extern indigo_uni_handle *indigo_uni_create_file(const char *path);

/** Open serial connection with configuration string of the form "9600-8N1".
 */
extern indigo_uni_handle *indigo_uni_open_serial_with_config(const char *path, const char *baudconfig);

/** Open serial connection at speed XXXX-8N1.
 */
extern indigo_uni_handle *indigo_uni_open_serial_with_speed(const char *path, int speed);

/** Open serial connection at speed 9600-8N1.
 */
extern indigo_uni_handle *indigo_uni_open_serial(const char *path);

/** Open client socket.
 */
extern indigo_uni_handle *indigo_uni_open_client_socket(const char *host, int port, int type);

#define indigo_uni_client_tcp_socket(host, port) indigo_uni_open_client_socket(host, port, SOCK_STREAM);
#define indigo_uni_client_udp_socket(host, port) indigo_uni_open_client_socket(host, port, SOCK_DGRAM);

/** Set read timeout.
 */
extern void indigo_uni_set_socket_read_timeout(indigo_uni_handle *handle, long timeout);

/** Set write timeout.
 */
extern void indigo_uni_set_socket_write_timeout(indigo_uni_handle *handle, long timeout);


/** Open server socket.
 */

extern void indigo_uni_open_tcp_server_socket(int *port, indigo_uni_handle **server_handle, void (*worker)(indigo_uni_worker_data *), void *data, void (*callback)(int));

/** Open TCP or UDP connection depending on the URL prefix tcp:// or udp:// for any other prefix protocol_hint is used.
    If no port is provided in the URL default port is used. protocol_hint will be set to actual protocol used for the connection.
 */
extern indigo_uni_handle *indigo_uni_open_url(const char *url, int default_port, indigo_uni_handle_type protocol_hint);

/** Read available data into buffer.
 */
extern long indigo_uni_read_available(indigo_uni_handle *handle, void *buffer, long length);

/** Read data into buffer.
 */
extern long indigo_uni_read(indigo_uni_handle *handle, void *buffer, long length);

/** Read up to one of terminator characters optionally ignoring some characters (or NULL) into buffer with optional usecs timeout (or -1).
 */
extern long indigo_uni_read_section(indigo_uni_handle *handle, char *buffer, long length, const char *terminators, const char *ignore, long timeout);

#define indigo_uni_read_line(handle, buffer, length) indigo_uni_read_section(handle, buffer, length, "\n", "\r\n", -1)

/** Read formatted.
 */

extern int indigo_uni_scanf(indigo_uni_handle *handle, const char *format, ...);

/** Write buffer.
 */
extern long indigo_uni_write(indigo_uni_handle *handle, const char *buffer, long length);

/** Write formatted.
 */

extern long indigo_uni_printf(indigo_uni_handle *handle, const char *format, ...);

/** Seek.
 */

extern long indigo_uni_seek(indigo_uni_handle *handle, long position, int whence);

/** Lock file.
 */
extern bool indigo_uni_lock_file(indigo_uni_handle *handle);

/** Close handle.
 */
extern void indigo_uni_close(indigo_uni_handle **handle);

/** INDIGO config folder (~/.indigo)
 */

extern const char *indigo_uni_config_folder();

/** Create folder.
 */
extern bool indigo_uni_mkdir(const char *path);

/** Remove file.
 */
extern bool indigo_uni_remove(const char *path);

/** Check if path is readable.
 */
extern bool indigo_uni_is_readable(const char *path);

/** Check if path is writable.
 */
extern bool indigo_uni_is_writable(const char *path);

/** Compress with gzip.
 */

extern void indigo_uni_compress(char *name, char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size);

/** Decompress with gzip.
 */

extern void indigo_uni_decompress(char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size);

#ifdef __cplusplus
}
#endif

#endif /* indigo_uni_io_h */
