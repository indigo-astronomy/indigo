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
 \file indigo_server_tcp.h
 */

#ifndef indigo_server_tcp_h
#define indigo_server_tcp_h

#include "indigo_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Prototype of callback function for network server (providing number of active clients).
 */
typedef void (*indigo_server_tcp_callback)(int);

/** TCP port to run on.
 */
extern int indigo_server_tcp_port;

/** TCP port is ephemeral.
 */
extern bool indigo_is_ephemeral_port;

/** Add static document.
 */
extern void indigo_server_add_resource(const char *path, unsigned char *data, unsigned length, const char *content_type);

/** Remove static document.
 */
extern void indigo_server_remove_resource(const char *path);
	
/** Start network server (function will block until server is active).
 */
extern indigo_result indigo_server_start(indigo_server_tcp_callback callback);

/** Shutdown network server (function will block until server is active).
 */
extern void indigo_server_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* indigo_server_tcp_h */

