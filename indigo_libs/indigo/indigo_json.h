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

/** INDIGO JSON wire protocol parser
 \file indigo_json.h
 */

#ifndef indigo_json_h
#define indigo_json_h

#include <stdio.h>
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

#define JSON_BUFFER_SIZE	(256 * 1024)

/** JSON wire protocol parser.
 */
INDIGO_EXTERN void indigo_json_parse(indigo_device *device, indigo_client *client);

/** Escape JSON string.
 */
INDIGO_EXTERN const char *indigo_json_escape(const char *string);

/** Generate define message.
 */
INDIGO_EXTERN indigo_result indigo_json_device_adapter_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);

/** Generate update message.
 */
INDIGO_EXTERN indigo_result indigo_json_device_adapter_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);

/** Generate delete message.
 */
INDIGO_EXTERN indigo_result indigo_json_device_adapter_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message);

/** Generate send message.
 */
INDIGO_EXTERN indigo_result indigo_json_device_adapter_message_property(indigo_client *client, indigo_device *device, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* indigo_json_h */
