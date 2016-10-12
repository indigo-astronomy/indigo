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

/** Non-LGPL libusb 1.0 replacement for macOS
 \file indigo_usb.h
 */

#ifndef indigo_usb_h
#define indigo_usb_h

#include <stdio.h>

#ifdef __APPLE__

#define libusb_error_name(errcode) (errcode >=0 ? "OK" : "Failed!")
#define LIBUSB_HOTPLUG_MATCH_ANY -1

typedef struct libusb_context libusb_context;
typedef struct libusb_device_handle libusb_device_handle;
typedef struct libusb_device libusb_device;

typedef struct libusb_device_descriptor {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint16_t bcdUSB;
  uint8_t  bDeviceClass;
  uint8_t  bDeviceSubClass;
  uint8_t  bDeviceProtocol;
  uint8_t  bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t  iManufacturer;
  uint8_t  iProduct;
  uint8_t  iSerialNumber;
  uint8_t  bNumConfigurations;
} libusb_device_descriptor;

typedef enum { LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED = 0x01, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT = 0x02 } libusb_hotplug_event;
typedef enum { LIBUSB_HOTPLUG_NO_FLAGS = 0, LIBUSB_HOTPLUG_ENUMERATE = 1<<0 } libusb_hotplug_flag;
typedef int libusb_hotplug_callback_handle;
typedef int (*libusb_hotplug_callback_fn)(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data);

int libusb_init(libusb_context **context);
void libusb_exit(libusb_context *context);

libusb_device * libusb_ref_device(libusb_device *dev);
void libusb_unref_device(libusb_device *dev);

ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list);
void libusb_free_device_list(libusb_device **list, int unref_devices);

int libusb_hotplug_register_callback(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor_id, int product_id, int dev_class, libusb_hotplug_callback_fn cb_fn, void *user_data, libusb_hotplug_callback_handle *handle);

int libusb_get_device_descriptor(libusb_device *dev, libusb_device_descriptor *desc);
uint8_t libusb_get_device_address(libusb_device *dev);
uint8_t libusb_get_bus_number(libusb_device *dev);
int libusb_get_max_packet_size(libusb_device *dev, unsigned char endpoint);
int libusb_get_max_iso_packet_size(libusb_device *dev, unsigned char endpoint);
libusb_device *libusb_get_device(libusb_device_handle *handle);
int libusb_reset_device(libusb_device_handle *dev);
int libusb_set_configuration(libusb_device_handle *dev, int configuration);

int libusb_open(libusb_device *dev,  libusb_device_handle **handle);
void libusb_close(libusb_device_handle *handle);

int libusb_claim_interface(libusb_device_handle *dev, int interface_number);
int libusb_clear_halt(libusb_device_handle *dev,  unsigned char endpoint);

int libusb_bulk_transfer(libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned int timeout);
int libusb_control_transfer(libusb_device_handle *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout);

#else

#include <libusb-1.0/libusb.h>

#endif

#endif /* indigo_usb_h */
