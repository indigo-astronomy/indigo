// Copyright (c) 2018 CloudMakers, s. r. o.
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

/** INDIGO USB Utilities
 \file indigo_usb_utils.h
 */


#ifndef indigo_usb_utils_h
#define indigo_usb_utils_h
#include <stdio.h>
#include <sys/types.h>

#if defined(INDIGO_MACOS) || defined(INDIGO_LINUX)
#include <libusb-1.0/libusb.h>
#define INDIGO_USB_HOTPLUG_POLLING
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#elif defined(INDIGO_WINDOWS)
#include <libusb.h>
#define INDIGO_USB_HOTPLUG_POLLING
#endif

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

#ifdef INDIGO_USB_HOTPLUG_POLLING
#define libusb_hotplug_register_callback libusb_hotplug_register_callback_sim
#define libusb_hotplug_deregister_callback libusb_hotplug_deregister_callback_poll
#endif

#ifdef __cplusplus
extern "C" {
#endif

INDIGO_EXTERN indigo_result indigo_get_usb_path(libusb_device* handle, char *path);

INDIGO_EXTERN void *indigo_usb_hotplug_thread(void *arg);


#ifdef INDIGO_USB_HOTPLUG_POLLING
INDIGO_EXTERN int libusb_hotplug_register_callback_sim(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor_id, int product_id, int dev_class, libusb_hotplug_callback_fn cb_fn, void *user_data, libusb_hotplug_callback_handle *handle);
INDIGO_EXTERN int libusb_hotplug_deregister_callback_poll(libusb_context *ctx, libusb_hotplug_callback_handle handle);
#endif

#ifdef __cplusplus
}
#endif

#endif /* indigo_usb_utils_h */
