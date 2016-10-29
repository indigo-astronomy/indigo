//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Driver base
 \file indigo_driver.h
 */

#ifndef indigo_device_h
#define indigo_device_h

//#define INDIGO_LINUX
//#undef INDIGO_MACOS

#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
#include <pthread.h>
#elif defined(INDIGO_MACOS)
#include <dispatch/dispatch.h>
#endif

#include "indigo_bus.h"

/** Main group name string.
 */
#define MAIN_GROUP                    "Main"

/** Device context pointer.
 */
#define DEVICE_CONTEXT                ((indigo_device_context *)device->device_context)

/** Private data pointer.
 */
#define PRIVATE_DATA                  (DEVICE_CONTEXT->private_data)

/** CONNECTION property pointer, property is mandatory, property change request handler should set property items and state and call indigo_device_change_property() on exit.
 */
#define CONNECTION_PROPERTY           (DEVICE_CONTEXT->connection_property)

/** CONNECTION.CONNECTED property item pointer.
 */
#define CONNECTION_CONNECTED_ITEM     (CONNECTION_PROPERTY->items+0)

/** CONNECTION.DISCONNECTED property item pointer.
 */
#define CONNECTION_DISCONNECTED_ITEM  (CONNECTION_PROPERTY->items+1)

/** INFO property pointer, property is mandatory.
 */
#define INFO_PROPERTY                 (DEVICE_CONTEXT->info_property)

/** INFO.DEVICE_NAME property item pointer.
 */
#define INFO_DEVICE_NAME_ITEM         (INFO_PROPERTY->items+0)

/** INFO.DEVICE_VERSION property item pointer.
 */
#define INFO_DEVICE_VERSION_ITEM      (INFO_PROPERTY->items+1)

/** INFO.DEVICE_INTERFACE property item pointer.
 */
#define INFO_DEVICE_INTERFACE_ITEM    (INFO_PROPERTY->items+2)

/** INFO.FRAMEWORK_NAME property item pointer.
 */
#define INFO_FRAMEWORK_NAME_ITEM      (INFO_PROPERTY->items+3)

/** INFO.FRAMEWORK_VERSION property item pointer.
 */
#define INFO_FRAMEWORK_VERSION_ITEM   (INFO_PROPERTY->items+4)

/** DEBUG property pointer.
 */
#define DEBUG_PROPERTY                (DEVICE_CONTEXT->debug_property)

/** DEBUG.ENABLED property item pointer, property is optional.
 */
#define DEBUG_ENABLED_ITEM            (DEBUG_PROPERTY->items+0)

/** DEBUG.DISABLED property item pointer.
 */
#define DEBUG_DISABLED_ITEM           (DEBUG_PROPERTY->items+1)

/** SIMULATION property pointer, property is optional.
 */
#define SIMULATION_PROPERTY           (DEVICE_CONTEXT->simulation_property)

/** SIMULATION.DISABLED property item pointer.
 */
#define SIMULATION_ENABLED_ITEM       (SIMULATION_PROPERTY->items+0)

/** SIMULATION.DISABLED property item pointer.
 */
#define SIMULATION_DISABLED_ITEM      (SIMULATION_PROPERTY->items+1)

/** CONFIG property pointer, property is mandatory.
 */
#define CONFIG_PROPERTY               (DEVICE_CONTEXT->congfiguration_property)

/** CONFIG.LOAD property item pointer.
 */
#define CONFIG_LOAD_ITEM              (CONFIG_PROPERTY->items+0)

/** CONFIG.SAVE property item pointer.
 */
#define CONFIG_SAVE_ITEM              (CONFIG_PROPERTY->items+1)

/** CONFIG.DEFAULT property item pointer.
 */
#define CONFIG_DEFAULT_ITEM           (CONFIG_PROPERTY->items+2)

/** Device interface (value shout be used for INFO_DEVICE_INTERFACE_ITEM->number.value
 */
typedef enum {
	INDIGO_INTERFACE_TELESCOPE = (1 << 0),    ///< Telescope interface
	INDIGO_INTERFACE_CCD       = (1 << 1),    ///< CCD interface
	INDIGO_INTERFACE_GUIDER    = (1 << 2),    ///< Guider interface
	INDIGO_INTERFACE_FOCUSER   = (1 << 3),    ///< Focuser interface
	INDIGO_INTERFACE_WHEEL     = (1 << 4),    ///< Filter wheel interface
	INDIGO_INTERFACE_DOME      = (1 << 5),    ///< Dome interface
	INDIGO_INTERFACE_GPS       = (1 << 6),    ///< GPS interface
	INDIGO_INTERFACE_WEATHER   = (1 << 7),    ///< Weather interface
	INDIGO_INTERFACE_AO        = (1 << 8),    ///< Adaptive Optics Interface
	INDIGO_INTERFACE_DUSTCAP   = (1 << 9),    ///< Dust Cap Interface
	INDIGO_INTERFACE_LIGHTBOX  = (1 << 10),   ///< Light Box Interface
	INDIGO_INTERFACE_AUX       = (1 << 15)    ///< Auxiliary interface
} indigo_device_interface;

/** Timer callback function prototype.
 */
typedef void (*indigo_timer_callback)(indigo_device *device);

/** Timer structure.
 */
typedef struct indigo_timer {
	indigo_device *device;                    ///< device associated with timer
	indigo_timer_callback callback;           ///< callback function pointer
#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
	struct timespec time;                     ///< time to fire (linux only)
	struct indigo_timer *next;                ///< next timer in the queue (linux only)
#elif defined(INDIGO_MACOS)
	bool canceled;                            ///< timer is canceled (darwin only)
#endif
} indigo_timer;

/** Device context structure.
 */
typedef struct {
	void *private_data;                       ///< private data
	int property_save_file_handle;            ///< handle for property save
#if defined(INDIGO_LINUX) || defined(INDIGO_FREEBSD)
	pthread_t timer_thread;                   ///< timer thread (linux only)
	pthread_mutex_t timer_mutex;              ///< timer mutex (linux only)
	int timer_pipe[2];                        ///< timer pipe (linux only)
	indigo_timer *timer_queue;                ///< timer queue (linux only)
#endif
	indigo_property *connection_property;     ///< CONNECTION property pointer
	indigo_property *info_property;           ///< INFO property pointer
	indigo_property *debug_property;          ///< DEBUG property pointer
	indigo_property *simulation_property;     ///< SIMULATION property pointer
	indigo_property *congfiguration_property; ///< CONFIGURATION property pointer
} indigo_device_context;

/** Attach callback function.
 */
extern indigo_result indigo_device_attach(indigo_device *device, indigo_version version, int interface);

/** Enumerate properties callback function.
 */
extern indigo_result indigo_device_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

/** Change property callback function.
 */
extern indigo_result indigo_device_change_property(indigo_device *device, indigo_client *client, indigo_property *property);

/** Detach callback function.
 */
extern indigo_result indigo_device_detach(indigo_device *device);

/** Send connect message.
 */
extern indigo_result indigo_device_connect(indigo_device *device);

/** Send disconnect message.
 */
extern indigo_result indigo_device_disconnect(indigo_device *device);

/** Load properties.
 */
extern indigo_result indigo_load_properties(indigo_device *device, bool default_properties);

/** Save single property.
 */
extern indigo_result indigo_save_property(indigo_device*device, int *file_handle, indigo_property *property);

/** Set timer.
 */
extern indigo_timer *indigo_set_timer(indigo_device *device, double delay, indigo_timer_callback callback);

/** Cancel timer.
 */
extern void indigo_cancel_timer(indigo_device *device, indigo_timer *timer);

/** Start USB event handler thread.
 */
extern void indigo_start_usb_even_handler();

/** Asynchronous execution in thread.
 */
extern void indigo_async(void *fun(void *data), void *data);

#endif /* indigo_device_h */

