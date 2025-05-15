// Copyright (c) 2016-2025 CloudMakers, s. r. o.
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

#include <indigo/indigo_bus.h>

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

#define MDNS_INDIGO_TYPE    "_indigo._tcp"
#define MDNS_HTTP_TYPE      "_http._tcp"

/** TCP port to run on.
 */
INDIGO_EXTERN int indigo_server_tcp_port;

/** Use bonjour.
 */
INDIGO_EXTERN bool indigo_use_bonjour;

/** TCP port is ephemeral.
 */
INDIGO_EXTERN bool indigo_is_ephemeral_port;

/** Use BLOB double-buffering.
 */
INDIGO_EXTERN bool indigo_use_blob_buffering;

/** Use BLOB compression (valid with indigo_use_blob_buffering only).
 */
INDIGO_EXTERN bool indigo_use_blob_compression;

/** Add static document.
 */
INDIGO_EXTERN void indigo_server_add_resource(const char *path, unsigned char *data, unsigned length, const char *content_type);

/** Add file document.
 */
INDIGO_EXTERN void indigo_server_add_file_resource(const char *path, const char *file_name, const char *content_type);

/** Add URI handler.
 */
INDIGO_EXTERN void indigo_server_add_handler(const char *path, bool (*handler)(indigo_uni_handle *handle, char *method, char *path, char *params));

/** Remove document.
 */
INDIGO_EXTERN void indigo_server_remove_resource(const char *path);
	
/** Remove all documents.
 */
INDIGO_EXTERN void indigo_server_remove_resources(void);
	
/** Start network server (function will block until server is active).
 */
INDIGO_EXTERN indigo_result indigo_server_start(void (*callback)(int));

/** Shutdown network server (function will block until server is active).
 */
INDIGO_EXTERN void indigo_server_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* indigo_server_tcp_h */

