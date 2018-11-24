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

#include <memory.h>
#include <stdint.h>

#include "../control.h"
#include "basler_sff_registry.h"

const struct sff_feature const sff_feature_registry[] = {
    /* DC1394_BASLER_SFF_EXTENDED_DATA_STREAM */
    {
        /* name       */ "Extended Data Stream",
        /* feature_id */ DC1394_BASLER_SFF_EXTENDED_DATA_STREAM,
        /* csr_guid   */ {0x4e7abcb0, 0x1b84, 0x11d8, {0x96, 0x51, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0x94ed7c88, 0x1c0f, 0x11d8, {0x82, 0xe0, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* has_chunk  */ DC1394_TRUE,
        /* generic    */ DC1394_TRUE,
        /* data_size  */ sizeof(dc1394basler_sff_extended_data_stream_t)
    },

    /* DC1394_BASLER_SFF_FRAME_COUNTER */
    {
        /* name       */ "Frame Counter",
        /* feature_id */ DC1394_BASLER_SFF_FRAME_COUNTER,
        /* csr_guid   */ {0x4433c4a4, 0x1b84, 0x11d8, {0x86, 0xb2, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0x8c5db844, 0x1c0f, 0x11d8, {0x96, 0x5f, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* has_chunk  */ DC1394_TRUE,
        /* generic    */ DC1394_TRUE,
        /* data_size  */ sizeof(dc1394basler_sff_frame_counter_t)
    },

    /* DC1394_BASLER_SFF_CYCLE_TIME_STAMP */
    {
        /* name       */ "Cycle Time Stamp",
        /* feature_id */ DC1394_BASLER_SFF_CYCLE_TIME_STAMP,
        /* csr_guid   */ {0x5590d58e, 0x1b84, 0x11d8, {0x84, 0x47, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0x994dd430, 0x1c0f, 0x11d8, {0x8f, 0x6b, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* has_chunk  */ DC1394_TRUE,
        /* generic    */ DC1394_TRUE,
        /* data_size  */ sizeof(dc1394basler_sff_cycle_time_stamp_t)
    },

    /* DC1394_BASLER_SFF_DCAM_VALUES */
    {
        /* name       */ "DCAM Values",
        /* feature_id */ DC1394_BASLER_SFF_DCAM_VALUES,
        /* csr_guid   */ {0x494de528, 0x1b84, 0x11d8, {0x8a, 0x0c, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0x911c8982, 0x1c0f, 0x11d8, {0x8a, 0xf0, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* has_chunk  */ DC1394_TRUE,
        /* generic    */ DC1394_TRUE,
        /* data_size  */ sizeof(dc1394basler_sff_dcam_values_t)
    },

    /* DC1394_BASLER_SFF_CRC_CHECKSUM */
    {
        /* name       */ "CRC Checksum",
        /* feature_id */ DC1394_BASLER_SFF_CRC_CHECKSUM,
        /* csr_guid   */ {0x3b34004e, 0x1b84, 0x11d8, {0x83, 0xb3, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_TRUE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_TEST_IMAGES */
    {
        /* name       */ "Test Images",
        /* feature_id */ DC1394_BASLER_SFF_TEST_IMAGES,
        /* csr_guid   */ {0x2a411342, 0xc0ca, 0x4368, {0xb4, 0x6e, 0xee, 0x5d, 0xee, 0xbf, 0x05, 0x48}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_EXTENDED_VERSION_INFO */
    {
        /* name       */ "Extended Version Info",
        /* feature_id */ DC1394_BASLER_SFF_EXTENDED_VERSION_INFO,
        /* csr_guid   */ {0x2b2d8714, 0xc15e, 0x4176, {0xa2, 0x35, 0x6e, 0xf8, 0x43, 0xd7, 0x47, 0xb4}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_LOOKUP_TABLE */
    {
        /* name       */ "Lookup Table",
        /* feature_id */ DC1394_BASLER_SFF_LOOKUP_TABLE,
        /* csr_guid   */ {0xb28c667c, 0xdf9d, 0x11d7, {0x86, 0x93, 0x00, 0x0c, 0x6e, 0x0b, 0xd1, 0xb0}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_TRUE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_TRIGGER_FLAG_AND_COUNTER */
    {
        /* name       */ "Trigger Flag and Counter",
        /* feature_id */ DC1394_BASLER_SFF_TRIGGER_FLAG_AND_COUNTER,
        /* csr_guid   */ {0x16c31a78, 0x3f75, 0x11d8, {0x94, 0xec, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_OUTPUT_PORT_0_CONFIGURATION */
    {
        /* name       */ "Output Port 0 Configuration",
        /* feature_id */ DC1394_BASLER_SFF_OUTPUT_PORT_0_CONFIGURATION,
        /* csr_guid   */ {0x5a889d7e, 0x41e5, 0x11d8, {0x84, 0x5b, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_OUTPUT_PORT_1_CONFIGURATION */
    {
        /* name       */ "Output Port 1 Configuration",
        /* feature_id */ DC1394_BASLER_SFF_OUTPUT_PORT_1_CONFIGURATION,
        /* csr_guid   */ {0x949d820a, 0x4513, 0x11d8, {0x9e, 0xb1, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_OUTPUT_PORT_2_CONFIGURATION */
    {
        /* name       */ "Output Port 2 Configuration",
        /* feature_id */ DC1394_BASLER_SFF_OUTPUT_PORT_2_CONFIGURATION,
        /* csr_guid   */ {0xc14e5072, 0x4513, 0x11d8, {0x81, 0xf3, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    },

    /* DC1394_BASLER_SFF_OUTPUT_PORT_3_CONFIGURATION */
    {
        /* name       */ "Output Port 3 Configuration",
        /* feature_id */ DC1394_BASLER_SFF_OUTPUT_PORT_3_CONFIGURATION,
        /* csr_guid   */ {0x949d820a, 0x4513, 0x11d8, {0x9e, 0xb1, 0x00, 0x10, 0x5a, 0x5b, 0xae, 0x55}},
        /* chunk_guid */ {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} /* not used */ },
        /* has_chunk  */ DC1394_FALSE,
        /* generic    */ DC1394_FALSE,
        /* data_size  */ 0
    }
};
const uint32_t sff_feature_registry_size = sizeof (sff_feature_registry) / sizeof (sff_feature);

/*
 * finds a feature by its API id
 */
const sff_feature* basler_sff_registry_find_by_id (dc1394basler_sff_feature_t feature_id)
{
    if (feature_id < DC1394_BASLER_SFF_FEATURE_MIN ||
        feature_id >= DC1394_BASLER_SFF_FEATURE_MAX ||
        feature_id >= sff_feature_registry_size)
        return NULL;
    return &(sff_feature_registry[feature_id]);
}

/*
 * finds a feature by its CSR GUID
 */
const sff_feature* basler_sff_registry_find_by_csr_guid (dc1394basler_sff_guid_t* csr_guid)
{
    uint32_t i;
    if (csr_guid == NULL)
        return NULL;

    for(i = 0; i < sff_feature_registry_size; i++) {
        if (!memcmp (&(sff_feature_registry[i].csr_guid), csr_guid, sizeof(dc1394basler_sff_guid_t)))
            return &(sff_feature_registry[i]);
    }
    return NULL;
}

/*
 * finds a feature by its CHUNK GUID
 */
const sff_feature* basler_sff_registry_find_by_chunk_guid (dc1394basler_sff_guid_t* chunk_guid)
{
    uint32_t i;
    if (chunk_guid == NULL)
        return NULL;

    for(i = 0; i < sff_feature_registry_size; i++) {
        if (!memcmp (&(sff_feature_registry[i].chunk_guid), chunk_guid, sizeof(dc1394basler_sff_guid_t)))
            return &(sff_feature_registry[i]);
    }
    return NULL;
}
