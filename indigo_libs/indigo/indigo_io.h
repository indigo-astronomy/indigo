// Copyright (c) 2016 CloudMakers, s. r. o.
// Copyright (c) 2016 Rumen G.Bogdanovski
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

/** INDIGO Bus
 \file indigo_io.h
 */

#ifndef indigo_io_h
#define indigo_io_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	INDIGO_PROTOCOL_TCP = 0,
	INDIGO_PROTOCOL_UDP = 1
} indigo_network_protocol;

/** Open serial connection at speed 9600.
 */
extern int indigo_open_serial(const char *dev_file);

/** Open serial connection at any speed.
 */
extern int indigo_open_serial_with_speed(const char *dev_file, int speed);

/** Open serial connection with configuration string of the form "9600-8N1".
 */
extern int indigo_open_serial_with_config(const char *dev_file, const char *baudconfig);

/** Open TCP network connection.
 */
extern int indigo_open_tcp(const char *host, int port);


/** Open UDP network connection.
 */
extern int indigo_open_udp(const char *host, int port);

/** Check if the specified name begins with "tcp://", "udp://" or "prefix://". prefix can be null.
 */
extern bool indigo_is_device_url(const char *name, const char *prefix);

/** Open TCP or UDP connection depending on the URL prefix tcp:// or udp:// for any other prefix protocol_hint is used.
    If no port is provided in the URL default port is used. protocol_hint will be set to actual protocol used for the connection.
 */
extern int indigo_open_network_device(const char *url, int default_port, indigo_network_protocol *protocol_hint);

/** Read buffer.
 */
extern int indigo_read(int handle, char *buffer, long length);

#if defined(INDIGO_WINDOWS)
/** Read buffer from socket.
 */
extern int indigo_recv(int handle, char *buffer, long length);

/** Read buffer from socket.
 */
extern int indigo_close(int handle);
#endif

/** Read line.
 */
extern int indigo_read_line(int handle, char *buffer, int length);

/** Write buffer.
 */
extern bool indigo_write(int handle, const char *buffer, long length);

/** Write formatted.
 */

extern bool indigo_printf(int handle, const char *format, ...);

/** Read formatted.
 */

extern int indigo_scanf(int handle, const char *format, ...);

/** Wait for data available.
 */

extern int indigo_select(int handle, long usec);

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

/** Compress with gzip.
 */

extern void indigo_compress(char *name, char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size);

/** Decompress with gzip.
 */

extern void indigo_decompress(char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size);

#endif

#ifdef __cplusplus
}
#endif

#endif /* indigo_io_h */
