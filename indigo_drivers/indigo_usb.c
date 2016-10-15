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
 \file indigo_usb.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "indigo_usb.h"

#if defined(INDIGO_DARWIN)

// Fake libusb subset implementation for macOS :)

#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "indigo_bus.h"

#define MAX_DEV 128

typedef struct libusb_context { } libusb_context;

typedef struct libusb_device_handle {
  libusb_device *device;
  IOUSBInterfaceInterface500 **interface;
  UInt8 endpoint[16];
  UInt16	maxPacketSize[16];
} libusb_device_handle;

typedef struct libusb_device {
  int refcnt;
  IOUSBDeviceInterface500 **device;
  libusb_device_handle *handle;
  USBDeviceAddress address;
  UInt32 location;
} libusb_device;

int libusb_init(libusb_context **context) {
  return 0;
}

void libusb_exit(libusb_context *context) {
}

libusb_device * libusb_ref_device(libusb_device *dev) {
  dev->refcnt++;
  return dev;
}

void libusb_unref_device(libusb_device *dev) {
  int refcnt = --dev->refcnt;
  if (refcnt == 0) {
    IOUSBDeviceInterface500 **device = dev->device;
    (*device)->Release(device);
    free(dev);
  }
}

ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list) {
  CFMutableDictionaryRef matching_dict;
  io_iterator_t iter;
  kern_return_t kr;
  io_service_t service;
  IOCFPlugInInterface **plug_in = NULL;
  SInt32 score;
  IOUSBDeviceInterface500 **device = NULL;
  *list = malloc(MAX_DEV*sizeof(libusb_device *));
  matching_dict = IOServiceMatching(kIOUSBDeviceClassName);
  if (matching_dict == NULL)
    return -1;
  kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iter);
  if (kr != KERN_SUCCESS)
    return -1;
  int i = 0;
  while ((service = IOIteratorNext(iter))) {
    kr = IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plug_in, &score);
    if (kr == KERN_SUCCESS && plug_in) {
      HRESULT res = (*plug_in)->QueryInterface(plug_in, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500), (LPVOID*) &device);
      (*plug_in)->Release(plug_in);
      if (res == 0 && device != NULL) {
        libusb_device *dev = malloc(sizeof(libusb_device));
        (*device)->GetDeviceAddress(device, &dev->address);
        (*device)->GetLocationID(device, &dev->location);
        dev -> device = device;
        dev -> refcnt = 1;
        (*list)[i++] = dev;
      }
    }
    IOObjectRelease(service);
  }
  IOObjectRelease(iter);
  (*list)[i] = NULL;
  return i;
}

void libusb_free_device_list(libusb_device **list, int unref_devices) {
  if (unref_devices) {
    int i = 0;
    struct libusb_device *dev;
    while ((dev = list[i++]) != NULL)
      libusb_unref_device(dev);
  }
  free(list);
}

int libusb_get_device_descriptor(libusb_device *dev, libusb_device_descriptor *desc) {
  memset(desc, 0, sizeof(libusb_device_descriptor));
  IOUSBDeviceInterface500 **device = dev->device;
  HRESULT res = (*device)->GetDeviceVendor(device, &(desc->idVendor));
  res |= (*device)->GetDeviceProduct(device, &(desc->idProduct));
  return res == 0 ? 0 : -1;
}

uint8_t libusb_get_device_address(libusb_device *dev) {
  return dev->address;
}

uint8_t libusb_get_bus_number(libusb_device *dev) {
  return dev->location >> 24;
}

libusb_device * libusb_get_device(libusb_device_handle *handle) {
  return handle->device;
}

int libusb_reset_device(libusb_device_handle *dev) {
  IOUSBDeviceInterface500 **device = dev->device->device;
  if ((*device)->ResetDevice(device) == 0)
    return 0;
  return -1;
}

int libusb_set_configuration(libusb_device_handle *dev, int configuration) {
  return 0;
}

int libusb_get_max_packet_size(libusb_device *dev, unsigned char endpoint) {
  libusb_device_handle *handle = dev->handle;
  if (handle) {
    for (int pipeRef = 0; pipeRef < 16; pipeRef++) {
      if (handle->endpoint[pipeRef] == endpoint) {
        return handle->maxPacketSize[pipeRef];
      }
    }
  }
  return -1;
}

int libusb_get_max_iso_packet_size(libusb_device *dev, unsigned char endpoint) {
  return libusb_get_max_packet_size(dev, endpoint);
}

int libusb_clear_halt(libusb_device_handle *handle,  unsigned char endpoint) {
  IOUSBInterfaceInterface500 **interface = handle->interface;
  if (handle) {
    for (int pipeRef = 0; pipeRef < 16; pipeRef++) {
      if (handle->endpoint[pipeRef] == endpoint) {
        if ((*interface)->ClearPipeStallBothEnds(interface, pipeRef+1) == 0)
          return 0;
      }
    }
  }
  return -1;
}

int libusb_open(libusb_device *dev,  libusb_device_handle **handle) {
  IOUSBDeviceInterface500 **device = dev->device;
  IOUSBInterfaceInterface500 **interface;
  kern_return_t kr;
  IOUSBFindInterfaceRequest	request;
  IOCFPlugInInterface **plug_in = NULL;
  SInt32 score;
  io_iterator_t iterator;
  io_service_t usb_interface;
  IOUSBConfigurationDescriptorPtr configDesc;
  UInt8 numConfig;
  kr = (*device)->USBDeviceOpenSeize(device);
  if (kr == 0) {
    kr = (*device)->GetNumberOfConfigurations(device, &numConfig);
    kr = (*device)->GetConfigurationDescriptorPtr(device, 0, &configDesc);
    kr = (*device)->SetConfiguration(device, configDesc->bConfigurationValue);
    request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting = kIOUSBFindInterfaceDontCare;
    kr = (*device)->CreateInterfaceIterator(device, &request, &iterator);
    if (kr == 0 && (usb_interface = IOIteratorNext(iterator))) {
      kr = IOCreatePlugInInterfaceForService(usb_interface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plug_in, &score);
      kr |= IOObjectRelease(usb_interface);
      if (kr == 0 && plug_in) {
        HRESULT res = (*plug_in)->QueryInterface(plug_in, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID500), (LPVOID) &interface);
        (*plug_in)->Release(plug_in);
        if (res == 0) {
          kr = (*interface)->USBInterfaceOpen(interface);
          if (kr == 0) {
            *handle = malloc(sizeof(libusb_device_handle));
            (*handle)->device = libusb_ref_device(dev);
            (*handle)->interface = interface;
            dev->handle = *handle;
            UInt8 endpoint_count;
            kr = (*interface)->GetNumEndpoints(interface, &endpoint_count);
            if (endpoint_count > 16)
              endpoint_count = 16;
            for (int pipeRef = 0; pipeRef < endpoint_count; pipeRef++) {
              UInt8	direction, number, transferType, interval;
              UInt16	maxPacketSize;
              kr = (*interface)->GetPipeProperties(interface, pipeRef+1, &direction, &number, &transferType, &maxPacketSize, &interval);
              if (kr == 0) {
                (*handle)->endpoint[pipeRef] = ((direction << 7 &  0x80) | (number & 0x0f));
                (*handle)->maxPacketSize[pipeRef] = maxPacketSize;
              }
            }
            return 0;
          }
        }
      }
    }
  }
  return -1;
}

void libusb_close(libusb_device_handle *handle) {
  if (handle) {
    IOUSBInterfaceInterface500 **interface = handle->interface;
    IOUSBDeviceInterface500 **device = handle->device->device;
    if (interface && *interface) {
      (*interface)->USBInterfaceClose(interface);
      (*interface)->Release(interface);
    }
    if (device && *device)
      (*device)->USBDeviceClose(device);
    handle->device->handle = NULL;
    libusb_unref_device(handle->device);
  }
}

int libusb_bulk_transfer(struct libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned int timeout) {
  IOUSBInterfaceInterface500 **interface = handle->interface;
  for (int pipeRef = 0; pipeRef < 16; pipeRef++) {
    if (handle->endpoint[pipeRef] == endpoint) {
      *transferred = length;
      if (endpoint & 0x80) {
        if ((*interface)->ReadPipeTO(interface, pipeRef+1, data, (unsigned int *)transferred, timeout, timeout) == 0)
          return 0;
      } else {
        if ((*interface)->WritePipeTO(interface, pipeRef+1, data, (unsigned int)length, timeout, timeout) == 0)
          return 0;
      }
    }
  }
  return -1;
}

int libusb_control_transfer(libusb_device_handle *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout) {
  IOUSBInterfaceInterface500 **interface = handle->interface;
  IOUSBDevRequestTO req;
  req.bmRequestType = bmRequestType;
  req.bRequest = bRequest;
  req.wValue = wValue;
  req.wIndex = wIndex;
  req.pData = data;
  req.wLength = wLength;
  req.noDataTimeout = timeout;
  req.completionTimeout = timeout;
  if ((*interface)->ControlRequestTO(interface, 0, &req) == 0)
    return 0;
  return -1;
}

int libusb_claim_interface(libusb_device_handle *dev, int interface_number) {
  return 0;
}

struct libusb_device_context {
  libusb_device dev;
  io_object_t notification;
  libusb_hotplug_callback_fn callback;
};

struct libusb_callback_context {
  libusb_hotplug_event events;
  int vendor_id;
  int product_id;
  libusb_hotplug_callback_fn callback;
  void *user_data;
  libusb_device device;
  IONotificationPortRef notify_port;
  io_iterator_t added_iterator;
  CFRunLoopRef run_loop;
};

static void device_notification(struct libusb_device_context *device_context, io_service_t service, natural_t messageType, void *messageArgument) {
  if (messageType == kIOMessageServiceIsTerminated) {
    device_context->callback(NULL, (libusb_device *)device_context, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
    IOUSBDeviceInterface500 **device = device_context->dev.device;
    if (device) {
      (*device)->Release(device);
    }
    IOObjectRelease(device_context->notification);
    free(device_context);
  }
}

static void device_added(struct libusb_callback_context *callback_context, io_iterator_t iterator) {
  kern_return_t kr;
  io_service_t usb_device;
  IOCFPlugInInterface **plugin = NULL;
  IOUSBDeviceInterface500 **device = NULL;
  SInt32 score;
  UInt16 vendor;
  UInt16 product;
  while ((usb_device = IOIteratorNext(iterator))) {
    kr = IOCreatePlugInInterfaceForService(usb_device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score);
    if (kr || !plugin) {
      INDIGO_ERROR(indigo_error("Failed to create plugin (0x%08x)", kr));
      continue;
    }
    HRESULT res = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500), (LPVOID*) &device);
    (*plugin)->Release(plugin);
    if (res || device == NULL) {
      INDIGO_ERROR(indigo_error("Failed to query device (%d)", (int) res));
      continue;
    }
    (*device)->GetDeviceVendor(device, &vendor);
    (*device)->GetDeviceProduct(device, &product);
    if ((callback_context->vendor_id == LIBUSB_HOTPLUG_MATCH_ANY || (callback_context->vendor_id == vendor)) && (callback_context->product_id == LIBUSB_HOTPLUG_MATCH_ANY || (callback_context->product_id == product))) {
      struct libusb_device_context *device_context = malloc(sizeof(struct libusb_device_context));
      (*device)->GetDeviceAddress(device, &device_context->dev.address);
      (*device)->GetLocationID(device, &device_context->dev.location);
      device_context->dev.device = device;
      device_context->dev.refcnt = 1;
      device_context->callback = callback_context->callback;
      if ((kr = IOServiceAddInterestNotification(callback_context->notify_port, usb_device, kIOGeneralInterest, (void (*)(void *, io_service_t, natural_t, void *))device_notification, device_context, &(device_context->notification)))) {
        INDIGO_ERROR(indigo_error("Failed to add interest notification (0x%08x)", kr));
      }
      device_context->callback(NULL, (libusb_device *)device_context, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
    }
    IOObjectRelease(usb_device);
  }
}

void *hotplug_thread(struct libusb_callback_context *callback_context) {
  kern_return_t kr;
  CFMutableDictionaryRef matching_dict = IOServiceMatching(kIOUSBDeviceClassName);
  if (matching_dict) {
    CFRunLoopSourceRef run_loop_source = IONotificationPortGetRunLoopSource(callback_context->notify_port = IONotificationPortCreate(kIOMasterPortDefault));
    CFRunLoopAddSource(callback_context->run_loop = CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopDefaultMode);
    if ((kr = IOServiceAddMatchingNotification(callback_context->notify_port, kIOFirstMatchNotification, matching_dict, (void (*)(void *, io_iterator_t))device_added, callback_context, &callback_context->added_iterator))) {
      INDIGO_ERROR(indigo_error("Failed to add service notification (0x%08x)", kr));
      free(callback_context);
      return NULL;
    } else {
      device_added(callback_context, callback_context->added_iterator);
    }
  }
  CFRunLoopRun();
  free(callback_context);
  return NULL;
}

int libusb_hotplug_register_callback(libusb_context *ctx, libusb_hotplug_event events, libusb_hotplug_flag flags, int vendor_id, int product_id, int dev_class, libusb_hotplug_callback_fn callback, void *user_data, libusb_hotplug_callback_handle *handle) {
  struct libusb_callback_context *context = malloc(sizeof(struct libusb_callback_context));
  context->events = events;
  context->vendor_id = vendor_id;
  context->product_id = product_id;
  context->callback = callback;
  context->user_data = user_data;
  pthread_t thread;
  pthread_create(&thread, NULL, (void *(*)(void *))hotplug_thread, context);
  return 0;
}

#endif
