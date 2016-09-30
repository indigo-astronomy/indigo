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
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
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
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#ifndef indigo_ccd_driver_h
#define indigo_ccd_driver_h

#include "indigo_bus.h"
#include "indigo_driver.h"

#define CCD_DRIVER_CONTEXT                ((indigo_ccd_driver_context *)driver->driver_context)

#define IMAGE_GROUP                       "Image"

#define CCD_INFO_PROPERTY                 (CCD_DRIVER_CONTEXT->ccd_info_property)
#define CCD_INFO_WIDTH_ITEM               (CCD_INFO_PROPERTY->items+0)
#define CCD_INFO_HEIGHT_ITEM              (CCD_INFO_PROPERTY->items+1)
#define CCD_INFO_MAX_HORIZONAL_BIN_ITEM   (CCD_INFO_PROPERTY->items+2)
#define CCD_INFO_MAX_VERTICAL_BIN_ITEM    (CCD_INFO_PROPERTY->items+3)
#define CCD_INFO_PIXEL_SIZE_ITEM          (CCD_INFO_PROPERTY->items+4)
#define CCD_INFO_PIXEL_WIDTH_ITEM         (CCD_INFO_PROPERTY->items+5)
#define CCD_INFO_PIXEL_HEIGHT_ITEM        (CCD_INFO_PROPERTY->items+6)
#define CCD_INFO_BITS_PER_PIXEL_ITEM      (CCD_INFO_PROPERTY->items+7)

#define CCD_EXPOSURE_PROPERTY             (CCD_DRIVER_CONTEXT->ccd_exposure_property)
#define CCD_EXPOSURE_ITEM                 (CCD_EXPOSURE_PROPERTY->items+0)

#define CCD_ABORT_EXPOSURE_PROPERTY       (CCD_DRIVER_CONTEXT->ccd_abort_exposure_property)
#define CCD_ABORT_EXPOSURE_ITEM           (CCD_ABORT_EXPOSURE_PROPERTY->items+0)

#define CCD_FRAME_PROPERTY                (CCD_DRIVER_CONTEXT->ccd_frame_property)
#define CCD_FRAME_LEFT_ITEM               (CCD_FRAME_PROPERTY->items+0)
#define CCD_FRAME_TOP_ITEM                (CCD_FRAME_PROPERTY->items+1)
#define CCD_FRAME_WIDTH_ITEM              (CCD_FRAME_PROPERTY->items+2)
#define CCD_FRAME_HEIGHT_ITEM             (CCD_FRAME_PROPERTY->items+3)

#define CCD_BIN_PROPERTY                  (CCD_DRIVER_CONTEXT->ccd_frame_property)
#define CCD_BIN_HORIZONTAL_ITEM           (CCD_BIN_PROPERTY->items+0)
#define CCD_BIN_VERTICAL_ITEM             (CCD_BIN_PROPERTY->items+1)

#define CCD_FRAME_TYPE_PROPERTY           (CCD_DRIVER_CONTEXT->ccd_frame_type_property)
#define CCD_FRAME_TYPE_LIGHT_ITEM         (CCD_FRAME_TYPE_PROPERTY->items+0)
#define CCD_FRAME_TYPE_BIAS_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+1)
#define CCD_FRAME_TYPE_DARK_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+2)
#define CCD_FRAME_TYPE_FLAT_ITEM          (CCD_FRAME_TYPE_PROPERTY->items+3)

#define CCD_IMAGE_FORMAT_PROPERTY         (CCD_DRIVER_CONTEXT->ccd_frame_type_property)
#define CCD_IMAGE_FORMAT_FITS_ITEM        (CCD_IMAGE_FORMAT_PROPERTY->items+0)
#define CCD_IMAGE_FORMAT_JPEG_ITEM        (CCD_IMAGE_FORMAT_PROPERTY->items+1)
#define CCD_IMAGE_FORMAT_RAW_ITEM         (CCD_IMAGE_FORMAT_PROPERTY->items+2)

#define CCD_IMAGE_PROPERTY                (CCD_DRIVER_CONTEXT->ccd_image_property)
#define CCD_IMAGE_ITEM                    (CCD_IMAGE_PROPERTY->items+0)

#define CCD_TEMPERATURE_PROPERTY          (CCD_DRIVER_CONTEXT->ccd_temperature_property)
#define CCD_TEMPERATURE_ITEM              (CCD_TEMPERATURE_PROPERTY->items+0)

#define CCD_COOLER_PROPERTY               (CCD_DRIVER_CONTEXT->ccd_cooler_property)
#define CCD_COOLER_ON_ITEM                (CCD_COOLER_PROPERTY->items+0)
#define CCD_COOLER_OFF_ITEM               (CCD_COOLER_PROPERTY->items+1)

#define CCD_COOLER_POWER_PROPERTY         (CCD_DRIVER_CONTEXT->ccd_cooler_power_property)
#define CCD_COOLER_POWER_ITEM             (CCD_COOLER_POWER_PROPERTY->items+0)

#define FITS_HEADER_SIZE  2880

typedef struct {
  void *private_data;
  // indigo_driver_context
  indigo_property *connection_property;
  indigo_property *info_property;
  indigo_property *debug_property;
  indigo_property *simulation_property;
  indigo_property *congfiguration_property;
  // indigo_ccd_driver_context
  indigo_property *ccd_info_property;
  indigo_property *ccd_exposure_property;
  indigo_property *ccd_abort_exposure_property;
  indigo_property *ccd_frame_property;
  indigo_property *ccd_binning_property;
  indigo_property *ccd_frame_type_property;
  indigo_property *ccd_image_format;
  indigo_property *ccd_image_property;
  indigo_property *ccd_temperature_property;
  indigo_property *ccd_cooler_property;
  indigo_property *ccd_cooler_power_property;
} indigo_ccd_driver_context;

extern indigo_result indigo_ccd_driver_attach(indigo_driver *driver, char *device, int version);
extern indigo_result indigo_ccd_driver_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property);
extern indigo_result indigo_ccd_driver_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property);
extern indigo_result indigo_ccd_driver_detach(indigo_driver *driver);

typedef struct {
  char name[8];
  enum { INDIGO_FITS_STRING, INDIGO_FITS_NUMBER } type;
  union {
  char string[70];
    double number;
  } value;
} indigo_fits_header;

extern void *indigo_convert_to_fits(indigo_driver *driver, double exposure_time);

#endif /* indigo_ccd_driver_h */
