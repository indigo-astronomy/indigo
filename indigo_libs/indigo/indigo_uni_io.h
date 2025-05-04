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
#define indigo_timezone timezone
#elif defined(INDIGO_WINDOWS)
#include <winsock2.h>
#define INDIGO_PATH_SEPATATOR	'\\'
#define PATH_MAX (_MAX_DRIVE + _MAX_DIR + _MAX_FNAME + _MAX_EXT + 1)
#define NAME_MAX (_MAX_FNAME + _MAX_EXT + 1)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strtok_r strtok_s
#define strdup _strdup
#define stat _stat
#define tzset _tzset
#define indigo_timezone _timezone
#endif

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	INDIGO_FILE_HANDLE = 0,
	INDIGO_COM_HANDLE = 1,
	INDIGO_TCP_HANDLE = 2,
	INDIGO_UDP_HANDLE = 3
} indigo_uni_handle_type;

typedef struct {
	int index;
	indigo_uni_handle_type type;
	union {
		int fd;
#if defined(INDIGO_WINDOWS)
		SOCKET sock;
		HANDLE com;
#endif
	};
	int log_level;
	int last_error;
#if defined(INDIGO_WINDOWS)
	OVERLAPPED ov_read;
	OVERLAPPED ov_write;
#endif
} indigo_uni_handle;

typedef struct {
	indigo_uni_handle *handle;
	void *data;
} indigo_uni_worker_data;

INDIGO_EXTERN indigo_uni_handle *indigo_stdin_handle;
INDIGO_EXTERN indigo_uni_handle *indigo_stdout_handle;
INDIGO_EXTERN indigo_uni_handle *indigo_stderr_handle;

INDIGO_EXTERN indigo_uni_handle *indigo_uni_create_file_handle(int fd, int log_level);

#if defined(INDIGO_WINDOWS)
INDIGO_EXTERN char *indigo_last_wsa_error();
INDIGO_EXTERN char *indigo_last_windows_error();
INDIGO_EXTERN const char *indigo_wchar_to_char(const wchar_t *string);
INDIGO_EXTERN const wchar_t *indigo_char_to_wchar(const char *string);
#define INDIGO_WCHAR_TO_CHAR(string) indigo_wchar_to_char(string)
#define INDIGO_CHAR_TO_WCHAR(string) indigo_char_to_wchar(string)
#define INDIGO_SNPRINTFW(buf, len, fmt, ...) swprintf(buf, len, L##fmt, __VA_ARGS__)
#define INDIGO_STRCPYW(dest, src) wcscpy(dest, src)
#else
#define INDIGO_WCHAR_TO_CHAR(string) (string)
#define INDIGO_CHAR_TO_WCHAR(string) (string)
#define INDIGO_SNPRINTFW(buf, len, fmt, ...) snprintf(buf, len, fmt, __VA_ARGS__)
#define INDIGO_STRCPYW(dest, src) strcpy(dest, src)
#endif


/** Decode last error to string.
 */
INDIGO_EXTERN char *indigo_uni_strerror(indigo_uni_handle *handle);

/** Check if name is URL.
 */
INDIGO_EXTERN bool indigo_uni_is_url(const char *name, const char *prefix);

/** Open existing file.
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_open_file(const char *path, int log_level);

/** Create new file.
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_create_file(const char *path, int log_level);

/** Open serial connection with configuration string of the form "9600-8N1".
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_open_serial_with_config(const char *serial, const char *baudconfig, int log_level);

/** Open serial connection at speed XXXX-8N1.
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_open_serial_with_speed(const char *serial, int speed, int log_level);

/** Open serial connection at speed 9600-8N1.
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_open_serial(const char *serial, int log_level);


/** sets/clears DTR on serial port
 */

INDIGO_EXTERN int indigo_uni_set_dtr(indigo_uni_handle *handle, bool state);

/** sets/clears RTS on serial port
 */

INDIGO_EXTERN int indigo_uni_set_rts(indigo_uni_handle *handle, bool state);

/** sets/clears CTS on serial port
 */

INDIGO_EXTERN int indigo_uni_set_cts(indigo_uni_handle *handle, bool state);

/** Open client socket.
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_open_client_socket(const char *host, int port, int type, int log_level);

#define indigo_uni_client_tcp_socket(host, port, log_level) indigo_uni_open_client_socket(host, port, SOCK_STREAM, log_level);
#define indigo_uni_client_udp_socket(host, port, log_level) indigo_uni_open_client_socket(host, port, SOCK_DGRAM, log_level);

/** Set read timeout.
 */
INDIGO_EXTERN void indigo_uni_set_socket_read_timeout(indigo_uni_handle *handle, long timeout);

/** Set write timeout.
 */
INDIGO_EXTERN void indigo_uni_set_socket_write_timeout(indigo_uni_handle *handle, long timeout);

/** Set NODELAY option.
 */
INDIGO_EXTERN void indigo_uni_set_socket_nodelay_option(indigo_uni_handle *handle);

/** Open server socket.
 */
INDIGO_EXTERN void indigo_uni_open_tcp_server_socket(int *port, indigo_uni_handle **server_handle, void (*worker)(indigo_uni_worker_data *), void *data, void (*callback)(int), int log_level);

/** Open TCP or UDP connection depending on the URL prefix tcp:// or udp:// for any other prefix protocol_hint is used.
    If no port is provided in the URL default port is used. protocol_hint will be set to actual protocol used for the connection.
 */
INDIGO_EXTERN indigo_uni_handle *indigo_uni_open_url(const char *url, int default_port, indigo_uni_handle_type protocol_hint, int log_level);

/** Read available data into buffer.
 */
INDIGO_EXTERN long indigo_uni_read_available(indigo_uni_handle *handle, void *buffer, long length);

/** Peek available data into buffer.
 */
INDIGO_EXTERN long indigo_uni_peek_available(indigo_uni_handle *handle, void *buffer, long length);

/** Read data into buffer.
 */
INDIGO_EXTERN long indigo_uni_read(indigo_uni_handle *handle, void *buffer, long length);

/** Discard all pending input
 */

INDIGO_EXTERN long indigo_uni_discard(indigo_uni_handle *handle);

/** Read up to one of terminator characters optionally ignoring some characters (or NULL) into buffer with optional usecs timeout (or -1).
 */
INDIGO_EXTERN long indigo_uni_read_section(indigo_uni_handle *handle, char *buffer, long length, const char *terminators, const char *ignore, long timeout);

#define indigo_uni_read_line(handle, buffer, length) indigo_uni_read_section(handle, buffer, length, "\n", "\r\n", INDIGO_DELAY(5))

/** Read formatted.
 */

INDIGO_EXTERN int indigo_uni_scanf_line(indigo_uni_handle *handle, const char *format, ...);

/** Write buffer.
 */
INDIGO_EXTERN long indigo_uni_write(indigo_uni_handle *handle, const char *buffer, long length);

/** Write formatted string.
 */

INDIGO_EXTERN long indigo_uni_printf(indigo_uni_handle *handle, const char *format, ...);

/** Write formatted string.
 */

INDIGO_EXTERN long indigo_uni_vprintf(indigo_uni_handle *handle, const char *format, va_list args);

/** Write formatted line.
 */

INDIGO_EXTERN long indigo_uni_vprintf_line(indigo_uni_handle *handle, const char *format, va_list args);

/** Seek.
 */

INDIGO_EXTERN long indigo_uni_seek(indigo_uni_handle *handle, long position, int whence);

/** Lock file.
 */
INDIGO_EXTERN bool indigo_uni_lock_file(indigo_uni_handle *handle);

/** Close handle.
 */
INDIGO_EXTERN void indigo_uni_close(indigo_uni_handle **handle);

/** Home folder (~/)
 */

INDIGO_EXTERN const char* indigo_uni_home_folder(void);

/** INDIGO config folder (~/.indigo)
 */

INDIGO_EXTERN const char *indigo_uni_config_folder(void);

/** Create folder.
 */
INDIGO_EXTERN bool indigo_uni_mkdir(const char *path);

/** Remove file.
 */
INDIGO_EXTERN bool indigo_uni_remove(const char *path);

/** Check if path is readable.
 */
INDIGO_EXTERN bool indigo_uni_is_readable(const char *path);

/** Check if path is writable.
 */
INDIGO_EXTERN bool indigo_uni_is_writable(const char *path);

/** Make real path.
 */
INDIGO_EXTERN char* indigo_uni_realpath(const char* path, char* resolved_path);

/** Make base name.
 */
INDIGO_EXTERN char* indigo_uni_basename(const char* path);

/** Scan folder for files matching filter.
 */

INDIGO_EXTERN int indigo_uni_scandir(const char* folder, char ***list, bool (*filter)(const char *));

/** Compress with gzip.
 */

INDIGO_EXTERN void indigo_uni_compress(char *name, char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size);

/** Decompress with gzip.
 */

INDIGO_EXTERN void indigo_uni_decompress(char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size);

/** Convert time to components
 */

INDIGO_EXTERN void indigo_gmtime(time_t* seconds, struct tm* tm);

INDIGO_EXTERN time_t indigo_timegm(struct tm* tm);

#ifdef __cplusplus
}
#endif

#endif /* indigo_uni_io_h */
