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

//  TODO:
//    SXUSB_READ_PIXELS_DELAYED is actually not suitable for long exposures

/** INDIGO StarlighXpress CCD driver
 \file indigo_ccd_sx.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#include "indigo_ccd_sx.h"
#include "indigo_driver_xml.h"

// -------------------------------------------------------------------------------- SX USB interface implementation


#define SX_VENDOR_ID  0x1278

#define MAX_DEVICES         5

#define USB_REQ_TYPE                0
#define USB_REQ                     1
#define USB_REQ_VALUE_L             2
#define USB_REQ_VALUE_H             3
#define USB_REQ_INDEX_L             4
#define USB_REQ_INDEX_H             5
#define USB_REQ_LENGTH_L            6
#define USB_REQ_LENGTH_H            7
#define USB_REQ_DATA                8

#define USB_REQ_DIR(r)              ((r)&(1<<7))
#define USB_REQ_DATAOUT             0x00
#define USB_REQ_DATAIN              0x80
#define USB_REQ_KIND(r)             ((r)&(3<<5))
#define USB_REQ_VENDOR              (2<<5)
#define USB_REQ_STD                 0
#define USB_REQ_RECIP(r)            ((r)&31)
#define USB_REQ_DEVICE              0x00
#define USB_REQ_IFACE               0x01
#define USB_REQ_ENDPOINT            0x02
#define USB_DATAIN                  0x80
#define USB_DATAOUT                 0x00

#define SXUSB_GET_FIRMWARE_VERSION  255
#define SXUSB_ECHO                  0
#define SXUSB_CLEAR_PIXELS          1
#define SXUSB_READ_PIXELS_DELAYED   2
#define SXUSB_READ_PIXELS           3
#define SXUSB_SET_TIMER             4
#define SXUSB_GET_TIMER             5
#define SXUSB_RESET                 6
#define SXUSB_SET_CCD               7
#define SXUSB_GET_CCD               8
#define SXUSB_SET_STAR2K            9
#define SXUSB_WRITE_SERIAL_PORT     10
#define SXUSB_READ_SERIAL_PORT      11
#define SXUSB_SET_SERIAL            12
#define SXUSB_GET_SERIAL            13
#define SXUSB_CAMERA_MODEL          14
#define SXUSB_LOAD_EEPROM           15
#define SXUSB_SET_A2D               16
#define SXUSB_RED_A2D               17
#define SXUSB_READ_PIXELS_GATED     18
#define SXUSB_BUILD_NUMBER          19
#define SXUSB_COOLER_CONTROL        30
#define SXUSB_COOLER                30
#define SXUSB_COOLER_TEMPERATURE    31
#define SXUSB_SHUTTER_CONTROL       32
#define SXUSB_SHUTTER               32
#define SXUSB_READ_I2CPORT          33

#define SXCCD_CAPS_STAR2K               0x01
#define SXCCD_CAPS_COMPRESS             0x02
#define SXCCD_CAPS_EEPROM               0x04
#define SXCCD_CAPS_GUIDER               0x08
#define SXUSB_CAPS_COOLER               0x10
#define SXUSB_CAPS_SHUTTER              0x20

#define CCD_EXP_FLAGS_FIELD_ODD         0x01
#define CCD_EXP_FLAGS_FIELD_EVEN        0x02
#define CCD_EXP_FLAGS_FIELD_BOTH        (CCD_EXP_FLAGS_FIELD_EVEN|CCD_EXP_FLAGS_FIELD_ODD)
#define CCD_EXP_FLAGS_FIELD_MASK        CCD_EXP_FLAGS_FIELD_BOTH
#define CCD_EXP_FLAGS_SPARE2            0x04
#define CCD_EXP_FLAGS_NOWIPE_FRAME      0x08
#define CCD_EXP_FLAGS_SPARE4            0x10
#define CCD_EXP_FLAGS_TDI               0x20
#define CCD_EXP_FLAGS_NOCLEAR_FRAME     0x40
#define CCD_EXP_FLAGS_NOCLEAR_REGISTER  0x80

#define CCD_EXP_FLAGS_SPARE8            0x01
#define CCD_EXP_FLAGS_SPARE9            0x02
#define CCD_EXP_FLAGS_SPARE10           0x04
#define CCD_EXP_FLAGS_SPARE11           0x08
#define CCD_EXP_FLAGS_SPARE12           0x10
#define CCD_EXP_FLAGS_SHUTTER_MANUAL    0x20
#define CCD_EXP_FLAGS_SHUTTER_OPEN      0x40
#define CCD_EXP_FLAGS_SHUTTER_CLOSE     0x80

#define BULK_IN                     0x0082
#define BULK_OUT                    0x0001

#define BULK_COMMAND_TIMEOUT        2000
#define BULK_DATA_TIMEOUT           10000

#define CHUNK_SIZE                  (10*1024*1024)

#undef PRIVATE_DATA
#define PRIVATE_DATA        ((sx_private_data *)DEVICE_CONTEXT->private_data)

#undef INDIGO_DEBUG
#define INDIGO_DEBUG(c) c

typedef struct {
  libusb_device *dev;
  libusb_device_handle *handle;
  char name[INDIGO_NAME_SIZE];
  indigo_timer *exposure_timer, *temperture_timer;
  unsigned char setup_data[22];
  int model;
  bool is_interlaced;
  bool is_color;
  unsigned short ccd_width;
  unsigned short ccd_height;
  double pix_width;
  double pix_height;
  unsigned short bits_per_pixel;
  unsigned short color_matrix;
  char extra_caps;
  double exposure;
  unsigned short frame_left;
  unsigned short frame_top;
  unsigned short frame_width;
  unsigned short frame_height;
  unsigned short horizontal_bin;
  unsigned short vertical_bin;
  double target_temperature, current_temperature;
  unsigned char *buffer;
  unsigned char *odd, *even;
  pthread_mutex_t usb_mutex;
  bool can_check_temperature;
} sx_private_data;

static struct {
  int product;
  const char *name;
} SX_PRODUCTS[] = {
  { 0x0105, "SXVF-M5" },
  { 0x0305, "SXVF-M5C" },
  { 0x0107, "SXVF-M7" },
  { 0x0307, "SXVF-M7C" },
  { 0x0308, "SXVF-M8C" },
  { 0x0109, "SXVF-M9" },
  { 0x0325, "SXVR-M25C" },
  { 0x0326, "SXVR-M26C" },
  { 0x0115, "SXVR-H5" },
  { 0x0119, "SXVR-H9" },
  { 0x0319, "SXVR-H9C" },
  { 0x0100, "SXVR-H9" },
  { 0x0300, "SXVR-H9C" },
  { 0x0126, "SXVR-H16" },
  { 0x0128, "SXVR-H18" },
  { 0x0135, "SXVR-H35" },
  { 0x0136, "SXVR-H36" },
  { 0x0137, "SXVR-H360" },
  { 0x0139, "SXVR-H390" },
  { 0x0194, "SXVR-H694" },
  { 0x0394, "SXVR-H694C" },
  { 0x0174, "SXVR-H674" },
  { 0x0374, "SXVR-H674C" },
  { 0x0198, "SX-814" },
  { 0x0398, "SX-814C" },
  { 0x0189, "SX-825" },
  { 0x0389, "SX-825C" },
  { 0x0184, "SX-834" },
  { 0x0384, "SX-834C" },
  { 0x0507, "SX LodeStar" },
  { 0x0517, "SX CoStar" },
  { 0x0509, "SX SuperStar" },
  { 0x0525, "SX UltraStar" },
  { 0x0200, "SXMX Camera" },
  { 0, NULL }
};

static indigo_device *devices[MAX_DEVICES] = { NULL, NULL, NULL, NULL, NULL };

static int sx_hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
  struct libusb_device_descriptor descriptor;
  switch (event) {
    case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
      libusb_get_device_descriptor(dev, &descriptor);
      for (int i = 0; SX_PRODUCTS[i].name; i++) {
        if (descriptor.idVendor == 0x1278 && SX_PRODUCTS[i].product == descriptor.idProduct) {
          for (int j = 0; j < MAX_DEVICES; j++) {
            if (devices[j] == NULL) {
              indigo_attach_device(devices[j] = indigo_ccd_sx(dev, SX_PRODUCTS[i].name));
              return 0;
            }
          }
        }
      }
      break;
    case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
      for (int j = 0; j < MAX_DEVICES; j++) {
        if (devices[j] != NULL) {
          indigo_device *device = devices[j];
          if (((sx_private_data*)PRIVATE_DATA)->dev == dev) {
            indigo_detach_device(device);
            free(device);
            devices[j] = NULL;
            return 0;
          }
        }
      }
      break;
  }
  return 0;
};

bool sx_open(indigo_device *device) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device *dev = PRIVATE_DATA->dev;
    int rc = libusb_open(dev, &PRIVATE_DATA->handle);
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    INDIGO_DEBUG(indigo_debug("sx_open: libusb_open [%d] -> %s", __LINE__, libusb_error_name(rc)));
#ifdef LIBUSB_H // not implemented in fake libusb
    if (rc >= 0) {
      if (libusb_kernel_driver_active(handle, 0) == 1) {
        rc = libusb_detach_kernel_driver(handle, 0);
        INDIGO_DEBUG(indigo_debug("sx_open: libusb_detach_kernel_driver [%d] -> %s", __LINE__, libusb_error_name(rc)));
      }
      if (rc >= 0) {
        struct libusb_config_descriptor *config;
        rc = libusb_get_config_descriptor(dev, 0, &config);
        INDIGO_DEBUG(indigo_debug("sx_open: libusb_get_config_descriptor [%d] -> %s", __LINE__, libusb_error_name(rc)));
        if (rc >= 0) {
          int interface = config->interface->altsetting->bInterfaceNumber;
          rc = libusb_claim_interface(handle, interface);
          INDIGO_DEBUG(indigo_debug("sx_open: libusb_claim_interface(%d) [%d] -> %s", __LINE__, interface, libusb_error_name(rc)));
        }
      }
    }
#endif
    int transferred;
    if (rc >= 0) { // reset
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
      setup_data[USB_REQ ] = SXUSB_RESET;
      setup_data[USB_REQ_VALUE_L ] = 0;
      setup_data[USB_REQ_VALUE_H ] = 0;
      setup_data[USB_REQ_INDEX_L ] = 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 0;
      setup_data[USB_REQ_LENGTH_H] = 0;
      rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
      INDIGO_DEBUG(indigo_debug("sx_open: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
      usleep(1000);
    }
    if (rc >= 0) { // read camera model
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
      setup_data[USB_REQ ] = SXUSB_CAMERA_MODEL;
      setup_data[USB_REQ_VALUE_L ] = 0;
      setup_data[USB_REQ_VALUE_H ] = 0;
      setup_data[USB_REQ_INDEX_L ] = 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 2;
      setup_data[USB_REQ_LENGTH_H] = 0;
      rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
      INDIGO_DEBUG(indigo_debug("sx_open: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
      if (rc >=0 && transferred == USB_REQ_DATA) {
        rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 2, &transferred, BULK_COMMAND_TIMEOUT);
        INDIGO_DEBUG(indigo_debug("sx_open: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
        if (rc >=0 && transferred == 2) {
          int result=setup_data[0] | (setup_data[1] << 8);
          PRIVATE_DATA->model = result & 0x1F;
          PRIVATE_DATA->is_color = result & 0x80;
          PRIVATE_DATA->is_interlaced = result & 0x40;
          if (result == 0x84)
            PRIVATE_DATA->is_interlaced = true;
          if (PRIVATE_DATA->model == 0x16 || PRIVATE_DATA->model == 0x17 || PRIVATE_DATA->model == 0x18 || PRIVATE_DATA->model == 0x19)
            PRIVATE_DATA->is_interlaced =  false;
          INDIGO_DEBUG(indigo_debug("sx_open: %s %s model %d\n", PRIVATE_DATA->is_interlaced ? "INTERLACED" : "NON-INTERLACED", PRIVATE_DATA->is_color ? "COLOR" : "MONO", PRIVATE_DATA->model));
        }
      }
    }
    if (rc >= 0) { // read camera params
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
      setup_data[USB_REQ ] = SXUSB_GET_CCD;
      setup_data[USB_REQ_VALUE_L ] = 0;
      setup_data[USB_REQ_VALUE_H ] = 0;
      setup_data[USB_REQ_INDEX_L ] = 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 17;
      setup_data[USB_REQ_LENGTH_H] = 0;
      rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
      INDIGO_DEBUG(indigo_debug("sx_open: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
      if (rc >=0 && transferred == USB_REQ_DATA) {
        rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 17, &transferred, BULK_COMMAND_TIMEOUT);
        INDIGO_DEBUG(indigo_debug("sx_open: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
        if (rc >=0 && transferred == 17) {
          PRIVATE_DATA->ccd_width = setup_data[2] | (setup_data[3] << 8);
          PRIVATE_DATA->ccd_height = setup_data[6] | (setup_data[7] << 8);
          PRIVATE_DATA->pix_width = ((setup_data[8] | (setup_data[9] << 8)) / 256.0);
          PRIVATE_DATA->pix_height = ((setup_data[10] | (setup_data[11] << 8)) / 256.0);
          PRIVATE_DATA->bits_per_pixel = setup_data[14];
          PRIVATE_DATA->color_matrix = setup_data[12] | (setup_data[13] << 8);
          PRIVATE_DATA->extra_caps = setup_data[16];
          if (PRIVATE_DATA->is_interlaced) {
            PRIVATE_DATA->ccd_height *= 2;
            PRIVATE_DATA->pix_height /= 2;
          }
          PRIVATE_DATA->buffer = malloc(2 * PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height + FITS_HEADER_SIZE);
          if (PRIVATE_DATA->is_interlaced) {
            PRIVATE_DATA->even = malloc(PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height);
            PRIVATE_DATA->odd = malloc(PRIVATE_DATA->ccd_width * PRIVATE_DATA->ccd_height);
          }
          INDIGO_DEBUG(indigo_debug("sxGetCameraParams: chip size: %d x %d, pixel size: %4.2f x %4.2f, matrix type: %x", PRIVATE_DATA->ccd_width, PRIVATE_DATA->ccd_height, PRIVATE_DATA->pix_width, PRIVATE_DATA->pix_height, PRIVATE_DATA->color_matrix));
          INDIGO_DEBUG(indigo_debug("sxGetCameraParams: capabilities:%s%s%s%s", (PRIVATE_DATA->extra_caps & SXCCD_CAPS_GUIDER ? " GUIDER" : ""), (PRIVATE_DATA->extra_caps & SXCCD_CAPS_STAR2K ? " STAR2K" : ""), (PRIVATE_DATA->extra_caps & SXUSB_CAPS_COOLER ? " COOLER" : ""), (PRIVATE_DATA->extra_caps & SXUSB_CAPS_SHUTTER ? " SHUTTER" : "")));
        }
      }
    }
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}

bool sx_start_exposure(indigo_device *device, double exposure, bool dark, int frame_left, int frame_top, int frame_width, int frame_height, int horizontal_bin, int vertical_bin) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    int rc = 0;
    int transferred;
    if (rc >= 0) {
      int milis = (int)round(1000 * exposure);
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
      setup_data[USB_REQ ] = SXUSB_READ_PIXELS_DELAYED;
      setup_data[USB_REQ_VALUE_L ] = CCD_EXP_FLAGS_FIELD_BOTH;
      setup_data[USB_REQ_VALUE_H ] = 0;
      setup_data[USB_REQ_INDEX_L ] = 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 10;
      setup_data[USB_REQ_LENGTH_H] = 0;
      setup_data[USB_REQ_DATA + 0] = frame_left & 0xFF;
      setup_data[USB_REQ_DATA + 1] = frame_left >> 8;
      setup_data[USB_REQ_DATA + 2] = frame_top & 0xFF;
      setup_data[USB_REQ_DATA + 3] = frame_top >> 8;
      setup_data[USB_REQ_DATA + 4] = frame_width & 0xFF;
      setup_data[USB_REQ_DATA + 5] = frame_width >> 8;
      setup_data[USB_REQ_DATA + 6] = frame_height & 0xFF;
      setup_data[USB_REQ_DATA + 7] = frame_height >> 8;
      setup_data[USB_REQ_DATA + 8] = horizontal_bin;
      setup_data[USB_REQ_DATA + 9] = vertical_bin;
      setup_data[USB_REQ_DATA + 10] = milis & 0xFF;
      setup_data[USB_REQ_DATA + 11] = (milis>>8) & 0xFF;
      setup_data[USB_REQ_DATA + 12] = (milis>>16) & 0xFF;
      setup_data[USB_REQ_DATA + 13] = (milis>>24) & 0xFF;
      if (PRIVATE_DATA->extra_caps & SXUSB_CAPS_SHUTTER)
        setup_data[USB_REQ_VALUE_H] = dark ? CCD_EXP_FLAGS_SHUTTER_CLOSE : CCD_EXP_FLAGS_SHUTTER_OPEN;
      if (PRIVATE_DATA->is_interlaced) {
        if (setup_data[USB_REQ_DATA + 9] > 1) {
          setup_data[USB_REQ_DATA + 2] = (frame_top/2) & 0xFF;
          setup_data[USB_REQ_DATA + 3] = (frame_top/2) >> 8;
          setup_data[USB_REQ_DATA + 6] = (frame_height/2) & 0xFF;
          setup_data[USB_REQ_DATA + 7] = (frame_height/2) >> 8;
          setup_data[USB_REQ_DATA + 9] = vertical_bin/2;
          rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
          INDIGO_DEBUG(indigo_debug("sx_start_exposure: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
        } else {
          setup_data[USB_REQ_DATA + 2] = (frame_top/2) & 0xFF;
          setup_data[USB_REQ_DATA + 3] = (frame_top/2) >> 8;
          setup_data[USB_REQ_DATA + 6] = (frame_height/2) & 0xFF;
          setup_data[USB_REQ_DATA + 7] = (frame_height/2) >> 8;
          setup_data[USB_REQ_DATA + 9] = 1;
          setup_data[USB_REQ_VALUE_L ] = CCD_EXP_FLAGS_FIELD_EVEN | CCD_EXP_FLAGS_SPARE2;
          rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
          INDIGO_DEBUG(indigo_debug("sx_start_exposure: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
        }
      } else {
        rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA + 14, &transferred, BULK_COMMAND_TIMEOUT);
        INDIGO_DEBUG(indigo_debug("sx_start_exposure: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
      }
    }
    PRIVATE_DATA->frame_left = frame_left;
    PRIVATE_DATA->frame_top = frame_top;
    PRIVATE_DATA->frame_width = frame_width;
    PRIVATE_DATA->frame_height = frame_height;
    PRIVATE_DATA->horizontal_bin = horizontal_bin;
    PRIVATE_DATA->vertical_bin = vertical_bin;
    PRIVATE_DATA->exposure = exposure;
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}


bool sx_clear_regs(indigo_device *device) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    int rc = 0;
    int transferred;
    if (rc >= 0) {
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
      setup_data[USB_REQ ] = SXUSB_CLEAR_PIXELS;
      setup_data[USB_REQ_VALUE_L ] = CCD_EXP_FLAGS_NOWIPE_FRAME;
      setup_data[USB_REQ_VALUE_H ] = 0;
      setup_data[USB_REQ_INDEX_L ] = 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 0;
      setup_data[USB_REQ_LENGTH_H] = 0;
      rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
      INDIGO_DEBUG(indigo_debug("sx_clear_regs: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
    }
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}

bool sx_download_pixels(indigo_device *device, unsigned char *pixels, unsigned long count) {
  libusb_device_handle *handle = PRIVATE_DATA->handle;
  int transferred;
  unsigned long read = 0;
  int rc=0;
  while (read < count && rc >= 0) {
    int size = (int)(count - read);
    if (size > CHUNK_SIZE)
      size = CHUNK_SIZE;
    rc = libusb_bulk_transfer(handle, BULK_IN, pixels + read, size, &transferred, BULK_DATA_TIMEOUT);
    INDIGO_DEBUG(indigo_debug("sx_download_pixels: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
    if (transferred >= 0) {
      read += transferred;
    }
  }
  return rc >= 0;
}

bool sx_read_pixels(indigo_device *device) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    int rc = 0;
    int transferred;
    int frame_width = PRIVATE_DATA->frame_width;
    int frame_height = PRIVATE_DATA->frame_height;
    int horizontal_bin = PRIVATE_DATA->horizontal_bin;
    int vertical_bin = PRIVATE_DATA->vertical_bin;
    int size = (frame_width/horizontal_bin)*(frame_height/vertical_bin);
    if (PRIVATE_DATA->is_interlaced) {
      if (vertical_bin>1) {
        rc = sx_download_pixels(device, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, 2 * size);
      } else {
        unsigned char *even = PRIVATE_DATA->even;
        rc = sx_download_pixels(device, PRIVATE_DATA->even, size);
        if (rc >= 0) {
          setup_data[USB_REQ ] = SXUSB_READ_PIXELS;
          setup_data[USB_REQ_VALUE_L ] = CCD_EXP_FLAGS_FIELD_ODD | CCD_EXP_FLAGS_SPARE2;
          rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA + 10, &transferred, BULK_COMMAND_TIMEOUT);
          if (rc >= 0) {
            INDIGO_DEBUG(indigo_debug("sx_read_pixels: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
            unsigned char *odd = PRIVATE_DATA->odd;
            rc = sx_download_pixels(device, PRIVATE_DATA->odd, size);
            if (rc >= 0) {
              unsigned char *buffer = PRIVATE_DATA->buffer + FITS_HEADER_SIZE;
              int ww = frame_width * 2;
              for (int i = 0, j = 0; i < frame_height; i += 2, j++) {
                memcpy(buffer + i * ww, (void *)odd + (j * ww), ww);
                memcpy(buffer + ((i + 1) * ww), (void *)even + (j * ww), ww);
              }
            }
          }
        }
      }
    } else {
      rc = sx_download_pixels(device, PRIVATE_DATA->buffer + FITS_HEADER_SIZE, 2 * size);
    }
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}

bool sx_abort_exposure(indigo_device *device) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    int rc = 0;
    int transferred;
    if (PRIVATE_DATA->extra_caps & SXUSB_CAPS_SHUTTER) {
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR;
      setup_data[USB_REQ ] = SXUSB_SHUTTER;
      setup_data[USB_REQ_VALUE_L ] = 0;
      setup_data[USB_REQ_VALUE_H ] = CCD_EXP_FLAGS_SHUTTER_CLOSE;
      setup_data[USB_REQ_INDEX_L ] = 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 0;
      setup_data[USB_REQ_LENGTH_H] = 0;
      rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
      INDIGO_DEBUG(indigo_debug("sx_abort_exposure: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
    }
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}

bool sx_set_cooler(indigo_device *device, bool status, double target, double *current) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    int rc = 0;
    int transferred;
    if (PRIVATE_DATA->extra_caps & SXUSB_CAPS_COOLER) {
      unsigned short setTemp = (unsigned short) (target * 10 + 2730);
      setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR;
      setup_data[USB_REQ ] = SXUSB_COOLER;
      setup_data[USB_REQ_VALUE_L ] = setTemp & 0xFF;
      setup_data[USB_REQ_VALUE_H ] = (setTemp >> 8) & 0xFF;
      setup_data[USB_REQ_INDEX_L ] = status ? 1 : 0;
      setup_data[USB_REQ_INDEX_H ] = 0;
      setup_data[USB_REQ_LENGTH_L] = 0;
      setup_data[USB_REQ_LENGTH_H] = 0;
      rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
      INDIGO_DEBUG(indigo_debug("sx_set_cooler: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
      if (rc >=0 && transferred == USB_REQ_DATA) {
        rc = libusb_bulk_transfer(handle, BULK_IN, setup_data, 3, &transferred, BULK_COMMAND_TIMEOUT);
        INDIGO_DEBUG(indigo_debug("sx_set_cooler: libusb_control_transfer [%d] -> %lu bytes %s", __LINE__, transferred, libusb_error_name(rc)));
        if (rc >=0 && transferred == 3) {
          *current = ((setup_data[1]*256)+setup_data[0]-2730)/10.0;
          INDIGO_DEBUG(indigo_debug("sx_set_cooler: cooler: %s, target: %gC, current: %gC", setup_data[2] ? "On" : "Off", target, *current));
        }
      }
    }
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}

bool sx_guide_relays(indigo_device *device, unsigned short mask) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_device_handle *handle = PRIVATE_DATA->handle;
    unsigned char *setup_data = PRIVATE_DATA->setup_data;
    int rc = 0;
    int transferred;
    setup_data[USB_REQ_TYPE ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ ] = SXUSB_SET_STAR2K;
    setup_data[USB_REQ_VALUE_L ] = mask;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    rc = libusb_bulk_transfer(handle, BULK_OUT, setup_data, USB_REQ_DATA, &transferred, BULK_COMMAND_TIMEOUT);
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
    return rc >= 0;
  }
  return false;
}


void sx_close(indigo_device *device) {
  if (!pthread_mutex_lock(&PRIVATE_DATA->usb_mutex)) {
    libusb_close(PRIVATE_DATA->handle);
    free(PRIVATE_DATA->buffer);
    if (PRIVATE_DATA->is_interlaced) {
      free(PRIVATE_DATA->even);
      free(PRIVATE_DATA->odd);
    }
    INDIGO_DEBUG(indigo_debug("sx_close: libusb_close"));
    pthread_mutex_unlock(&PRIVATE_DATA->usb_mutex);
  }
}


// -------------------------------------------------------------------------------- INDIGO driver implementation

static void exposure_timer_callback(indigo_device *device) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_ITEM->number.value = 0;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
    if (sx_read_pixels(device)) {
      CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
      indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure done");
      indigo_process_image(device, PRIVATE_DATA->buffer, PRIVATE_DATA->exposure);
    } else {
      CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
      indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure failed");
    }
  }
  PRIVATE_DATA->can_check_temperature = true;
}

static void clear_reg_timer_callback(indigo_device *device) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    PRIVATE_DATA->can_check_temperature = false;
    sx_clear_regs(device);
    PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 4, exposure_timer_callback);
  }
}

static void ccd_temperature_callback(indigo_device *device) {
  if (PRIVATE_DATA->can_check_temperature) {
    if (sx_set_cooler(device, CCD_COOLER_ON_ITEM->sw.value, PRIVATE_DATA->target_temperature, &PRIVATE_DATA->current_temperature)) {
      double diff = PRIVATE_DATA->current_temperature - PRIVATE_DATA->target_temperature;
      if (CCD_COOLER_ON_ITEM->sw.value)
        CCD_TEMPERATURE_PROPERTY->state = fabs(diff) > 0.5 ? INDIGO_BUSY_STATE : INDIGO_OK_STATE;
      else
        CCD_TEMPERATURE_PROPERTY->state = INDIGO_OK_STATE;
      CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
      CCD_COOLER_PROPERTY->state = INDIGO_OK_STATE;
    } else {
      CCD_COOLER_PROPERTY->state = INDIGO_ALERT_STATE;
      CCD_TEMPERATURE_PROPERTY->state = INDIGO_ALERT_STATE;
    }
    indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
    indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
  }
  PRIVATE_DATA->temperture_timer = indigo_set_timer(device, 5, ccd_temperature_callback);
}

static indigo_result attach(indigo_device *device) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  
  sx_private_data *private_data = device->device_context;
  device->device_context = NULL;
  
  if (indigo_ccd_device_attach(device, private_data->name, INDIGO_VERSION_CURRENT) == INDIGO_OK) {
    DEVICE_CONTEXT->private_data = private_data;
    // -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN
    CCD_BIN_PROPERTY->hidden = false;
    CCD_BIN_HORIZONTAL_ITEM->number.max = CCD_INFO_MAX_HORIZONAL_BIN_ITEM->number.value = 4;
    CCD_BIN_VERTICAL_ITEM->number.max = CCD_INFO_MAX_VERTICAL_BIN_ITEM->number.value = 4;
    CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
    // --------------------------------------------------------------------------------
    pthread_mutex_init(&PRIVATE_DATA->usb_mutex, NULL);
    INDIGO_LOG(indigo_log("%s attached", PRIVATE_DATA->name));
    return indigo_ccd_device_enumerate_properties(device, NULL, NULL);
  }
  return INDIGO_FAILED;
}

static indigo_result change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
  assert(device != NULL);
  assert(device->device_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION -> CCD_INFO, CCD_GUIDE_DEC, CCD_GUIDE_RA, CCD_COOLER, CCD_TEMPERATURE
    indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
    if (CONNECTION_CONNECTED_ITEM->sw.value) {
      if (sx_open(device)) {
        CCD_INFO_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.max = PRIVATE_DATA->ccd_width;
        CCD_INFO_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.max = PRIVATE_DATA->ccd_height;
        CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = round(PRIVATE_DATA->pix_width * 100)/100;
        CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = round(PRIVATE_DATA->pix_height * 100) / 100;
        if (PRIVATE_DATA->extra_caps & SXUSB_CAPS_COOLER) {
          CCD_COOLER_PROPERTY->hidden = false;
          CCD_TEMPERATURE_PROPERTY->hidden = false;
          PRIVATE_DATA->target_temperature = 0;
          ccd_temperature_callback(device);
        }
        if (PRIVATE_DATA->extra_caps & SXCCD_CAPS_STAR2K) {
          CCD_GUIDE_DEC_PROPERTY->hidden = false;
          CCD_GUIDE_RA_PROPERTY->hidden = false;
        }
        PRIVATE_DATA->can_check_temperature = true;
        CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
      } else {
        CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
        indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
      }
    } else {
      indigo_cancel_timer(device, PRIVATE_DATA->temperture_timer);
      sx_close(device);
      CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
    }
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
    sx_start_exposure(device, CCD_EXPOSURE_ITEM->number.value, CCD_FRAME_TYPE_DARK_ITEM->sw.value, CCD_FRAME_LEFT_ITEM->number.value, CCD_FRAME_TOP_ITEM->number.value, CCD_FRAME_WIDTH_ITEM->number.value, CCD_FRAME_HEIGHT_ITEM->number.value, CCD_BIN_HORIZONTAL_ITEM->number.value, CCD_BIN_VERTICAL_ITEM->number.value);
    CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
    indigo_update_property(device, CCD_EXPOSURE_PROPERTY, "Exposure initiated");
    if (CCD_UPLOAD_MODE_LOCAL_ITEM->sw.value) {
      CCD_IMAGE_FILE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_IMAGE_FILE_PROPERTY, NULL);
    } else {
      CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
    }
    if (CCD_EXPOSURE_ITEM->number.value > 4)
      PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value-4, clear_reg_timer_callback);
    else {
      PRIVATE_DATA->can_check_temperature = false;
      PRIVATE_DATA->exposure_timer = indigo_set_timer(device, CCD_EXPOSURE_ITEM->number.value, exposure_timer_callback);
    }
  } else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
    if (CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      sx_abort_exposure(device);
      indigo_cancel_timer(device, PRIVATE_DATA->exposure_timer);
    }
    PRIVATE_DATA->can_check_temperature = true;
    indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
  } else if (indigo_property_match(CCD_COOLER_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_COOLER
    indigo_property_copy_values(CCD_COOLER_PROPERTY, property, false);
    if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
      CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
    }
    return INDIGO_OK;
  } else if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_TEMPERATURE
    indigo_property_copy_values(CCD_TEMPERATURE_PROPERTY, property, false);
    if (CONNECTION_CONNECTED_ITEM->sw.value && !CCD_COOLER_PROPERTY->hidden) {
      PRIVATE_DATA->target_temperature = CCD_TEMPERATURE_ITEM->number.value;
      CCD_TEMPERATURE_ITEM->number.value = PRIVATE_DATA->current_temperature;
      if (CCD_COOLER_OFF_ITEM->sw.value) {
        indigo_set_switch(CCD_COOLER_PROPERTY, CCD_COOLER_ON_ITEM, true);
        CCD_COOLER_PROPERTY->state = INDIGO_BUSY_STATE;
        indigo_update_property(device, CCD_COOLER_PROPERTY, NULL);
      }
      CCD_TEMPERATURE_PROPERTY->state = INDIGO_BUSY_STATE;
      indigo_update_property(device, CCD_TEMPERATURE_PROPERTY, NULL);
      return INDIGO_OK;
    }
    // --------------------------------------------------------------------------------
  }
  return indigo_ccd_device_change_property(device, client, property);
}

static indigo_result detach(indigo_device *device) {
  assert(device != NULL);
  INDIGO_LOG(indigo_log("%s detached", PRIVATE_DATA->name));
  free(PRIVATE_DATA);
  return indigo_ccd_device_detach(device);
}

indigo_device *indigo_ccd_sx(libusb_device *dev, const char *name) {
  static indigo_device device_template = {
    NULL, INDIGO_OK, INDIGO_VERSION_CURRENT,
    attach,
    indigo_ccd_device_enumerate_properties,
    change_property,
    detach
  };
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(dev, &descriptor);
  indigo_device *device = malloc(sizeof(indigo_device));
  if (device != NULL) {
    memcpy(device, &device_template, sizeof(indigo_device));
    sx_private_data *private_data = malloc(sizeof(sx_private_data));
    private_data->dev = dev;
    strncpy(private_data->name, name, INDIGO_NAME_SIZE);
    device->device_context = private_data;
  }
  return device;
}

indigo_result indigo_ccd_sx_register() {
  libusb_init(NULL);
  if (libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, 0x1278, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, sx_hotplug_callback, NULL, NULL) == 0)
    return INDIGO_OK;
  return INDIGO_FAILED;
}

