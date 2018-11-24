/*
 * 1394-Based Digital Camera Control Library
 *
 * Basler Smart Feature Framework specific extensions
 * 
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 *
 * Copyright (C) 2006 Tobii Technology AB, Stockholm Sweden
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DC1394_VENDOR_BASLER_SFF_REGISTRY_H__
#define __DC1394_VENDOR_BASLER_SFF_REGISTRY_H__

#include "basler_sff.h"

/*! \file dc1394/vendor/basler_sff_registry.h
    \brief No docs yet

    More details soon
*/

typedef struct sff_feature {
  /** human-readable name of the feature */
  const char* name;

  /** the feature id */
  dc1394basler_sff_feature_t feature_id;

  /** the CSR guid used for looking up the CSR address */
  dc1394basler_sff_guid_t csr_guid;

  /** the CHUNK guid, used for associating a chunk with a feature id */
  dc1394basler_sff_guid_t chunk_guid;

  /** whether this feature has an image chunk */
  dc1394bool_t has_chunk;

  /** whether this feature can be enabled by the generic dc1394_basler_sff_feature_enable() */
  dc1394bool_t generic;

  /** the size of the C type in the image chunk */
  uint32_t data_size;
} sff_feature;

/**
 * Returns a sff feature descriptor by id
 */
const sff_feature* basler_sff_registry_find_by_id (dc1394basler_sff_feature_t feature_id);

/**
 * Returns a sff feature descriptor by CSR guid
 */
const sff_feature* basler_sff_registry_find_by_csr_guid (dc1394basler_sff_guid_t* csr_guid);

/**
 * Returns a sff feature descriptor by CHUNK guid
 */
const sff_feature* basler_sff_registry_find_by_chunk_guid (dc1394basler_sff_guid_t* csr_guid);

#endif
