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

#include <string.h>

#include "indigo_version.h"

void indigo_copy_property_name(int version, indigo_property *property, const char *name) {
  if (version == INDIGO_VERSION_LEGACY) {
    if (!strcmp(name, "CONFIG_PROCESS")) {
      INDIGO_LOG(indigo_trace("Version: CONFIG_PROCESS -> CONFIGURATION"));
      strcpy(property->name, "CONFIGURATION");
      return;
    }
    if (!strcmp(name, "CCD1")) {
      INDIGO_LOG(indigo_trace("Version: CCD1 -> CCD_IMAGE"));
      strcpy(property->name, "CCD_IMAGE");
      return;
    }
  }
  strncpy(property->name, name, INDIGO_NAME_SIZE);
}

const char *indigo_property_name(int version, indigo_property *property) {
  if (version == INDIGO_VERSION_LEGACY) {
    if (!strcmp(property->name, "CCD_IMAGE")) {
      INDIGO_LOG(indigo_trace("Version: CCD_IMAGE -> CCD1"));
      return "CCD1";
    }
    if (!strcmp(property->name, "CONFIGURATION")) {
      INDIGO_LOG(indigo_trace("Version: CONFIGURATION -> CONFIG_PROCESS"));
      return "CONFIG_PROCESS";
    }
  }
  return property->name;
}


void indigo_copy_item_name(int version, indigo_property *property, indigo_item *item, const char *name) {
  if (version == INDIGO_VERSION_LEGACY) {
    if (!strcmp(property->name, "CCD_IMAGE")) {
      if (!strcmp(name, "CCD1")) {
        INDIGO_LOG(indigo_trace("Version: CCD_IMAGE.CCD1 -> CCD_IMAGE.CCD_IMAGE"));
        strcpy(item->name, "CCD_IMAGE");
        return;
      }
    }
  }
  strncpy(item->name, name, INDIGO_NAME_SIZE);
}

const char *indigo_item_name(int version, indigo_property *property, indigo_item *item) {
  if (version == INDIGO_VERSION_LEGACY) {
    if (!strcmp(property->name, "CCD_IMAGE")) {
      if (!strcmp(item->name, "CCD_IMAGE")) {
        INDIGO_LOG(indigo_trace("Version: CCD_IMAGE.CCD_IMAGE -> CCD_IMAGE.CCD1"));
        return "CCD1";
      }
    }
  }
  return item->name;
}

