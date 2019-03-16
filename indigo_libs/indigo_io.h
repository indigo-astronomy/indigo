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

#ifdef __cplusplus
extern "C" {
#endif

/** Open serial connection at speed 9600.
 */
extern int indigo_open_serial(const char *dev_file);

/** Open serial connection at any speed.
 */
extern int indigo_open_serial_with_speed(const char *dev_file, int speed);
	
/** Open TCP network connection.
 */
extern int indigo_open_tcp(const char *host, int port);

	
/** Open UDP network connection.
 */
extern int indigo_open_udp(const char *host, int port);
	
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
	
#ifdef __cplusplus
}
#endif

#endif /* indigo_io_h */
