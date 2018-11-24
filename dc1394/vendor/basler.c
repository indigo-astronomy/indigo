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

#include <inttypes.h>

#include "../internal.h"
#include "../register.h"
#include "../utils.h"
#include "../control.h"
#include "basler.h"
#include "basler_sff_registry.h"

/*
 * BASLER CONTROL REGISTERS
 */
#define BASLER_ADDRESS_SFF_INQUIRY  0x010U
#define BASLER_ADDRESS_SFF_ADDRESS  0x020U

/*
 * KNOWN BIT POSITIONS IN SFF FEATURE CSRs
 */
#define BASLER_SFF_CSR_BIT_ENABLE   0x00000001
#define BASLER_SFF_CSR_BIT_PRESENCE  0x80000000

/*
 * Private functions
 */
dc1394error_t
get_sff_address_from_csr_guid (dc1394camera_t* camera, const dc1394basler_sff_guid_t* feature_guid, uint64_t* address)
{
    dc1394error_t err;
    uint32_t data;

    if (camera == NULL || feature_guid == NULL || address == NULL)
        return DC1394_FAILURE;

    /* write GUID like this to BASLER_ADDRESS_SFF_INQUIRY_REGISTER:
     * 0x10   <- D1
     * 0x14   <- D3 | D2
     * 0x18   <- D4[3] | D4[2] | D4[1] | D4[0]
     * 0x1C   <- D4[7] | D4[6] | D4[5] | D4[4]
     */
    data = feature_guid->d1;
    err = dc1394_set_adv_control_register (camera, BASLER_ADDRESS_SFF_INQUIRY, data);
    DC1394_ERR_RTN(err, "Could not write D1 to SFF inquiry register");

    data = ((uint32_t)feature_guid->d3) << 16 | feature_guid->d2;
    err = dc1394_set_adv_control_register (camera, BASLER_ADDRESS_SFF_INQUIRY + 0x4U, data);
    DC1394_ERR_RTN(err, "Could not write D3 | D2 to SFF inquiry register");

    data = ((uint32_t)feature_guid->d4[3] << 24) | ((uint32_t)feature_guid->d4[2] << 16) |
        ((uint32_t)feature_guid->d4[1] << 8) | feature_guid->d4[0];
    err = dc1394_set_adv_control_register (camera, BASLER_ADDRESS_SFF_INQUIRY + 0x8U, data);
    DC1394_ERR_RTN(err, "Could not write D4[3..0] to SFF inquiry register");

    data = ((uint32_t)feature_guid->d4[7] << 24) | ((uint32_t)feature_guid->d4[6] << 16) |
        ((uint32_t)feature_guid->d4[5] << 8) | feature_guid->d4[4];
    err = dc1394_set_adv_control_register (camera, BASLER_ADDRESS_SFF_INQUIRY + 0xCU, data);
    DC1394_ERR_RTN(err, "Could not write D4[7..4] to SFF inquiry register");

    /* read address */
    err = dc1394_get_adv_control_register (camera, BASLER_ADDRESS_SFF_ADDRESS, &data);
    DC1394_ERR_RTN(err, "Could not read first quadlet of address from SFF address register");

    *address = data;

    err = dc1394_get_adv_control_register (camera, BASLER_ADDRESS_SFF_ADDRESS + 0x4U, &data);
    DC1394_ERR_RTN(err, "Could not read second quadlet of address from SFF address register");

    *address |= ((uint64_t)data) << 32;
    *address -= CONFIG_ROM_BASE;
    return DC1394_SUCCESS;
}

/*
 * Tests whether the camera supports Basler SFF
 */
dc1394error_t
dc1394_basler_sff_is_available (dc1394camera_t* camera, dc1394bool_t *available)
{
    uint32_t data;
    dc1394error_t err;
    const uint32_t basler_feature_id_1 = 0x0030533b;
    const uint32_t basler_feature_id_2 = 0x73c3f000;

    if (camera == NULL || available == NULL) {
        err = DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "camera or available is NULL");
    }
    *available = DC1394_FALSE;

    err = dc1394_set_adv_control_register (camera, 0x0U, basler_feature_id_1);
    DC1394_ERR_RTN(err, "Could not write the first quadlet of Basler feature ID");

    err = dc1394_set_adv_control_register (camera, 0x4U, basler_feature_id_2);
    DC1394_ERR_RTN(err, "Could not write the second quadlet of Basler feature ID");

    err = dc1394_get_adv_control_register (camera, 0x0U, &data);
    DC1394_ERR_RTN(err, "Could not read from the ACR");
    if (data != 0xffffffff) {
        *available = DC1394_TRUE;
        return DC1394_SUCCESS;
    }

    err = dc1394_get_adv_control_register (camera, 0x4U, &data);
    DC1394_ERR_RTN(err, "Could not read from ACR + 4");
    if (data != 0xffffffff) {
        *available = DC1394_TRUE;
        return DC1394_SUCCESS;
    }

    /* SFF is not supported */
    return DC1394_SUCCESS;
}

/**
 * Tests whether the camera supports the specified SFF feature
 */
dc1394error_t
dc1394_basler_sff_feature_is_available (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, dc1394bool_t *available)
{
    const sff_feature *feature_desc = NULL;
    uint64_t feature_address = 0;
    dc1394error_t err;

    if (camera == NULL || available == NULL) {
        err = DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "dc1394_basler_sff_feature_is_available(): camera or available is NULL");
    }

    feature_desc = basler_sff_registry_find_by_id (feature_id);
    if (feature_desc == NULL) {
        err = DC1394_FAILURE;
        DC1394_ERR_RTN(err, "unknown feature");
    }

    /* if this feature uses has an image chunk, only allow it if the
     * camera IIDC version is >= 1.30 because the _dc1394_capture_setup_basic()
     * does not compute the frame byte size correctly for iidc version < 1.30 */
    if (feature_desc->has_chunk && camera->iidc_version < DC1394_IIDC_VERSION_1_30) {
        err = DC1394_FAILURE;
        DC1394_ERR_RTN(err, "smart features which have image chunks cannot be used with cameras with a iidc_version lower than 1.30");
    }

    /* get address for this feature, if the address is 0x0, then this feature is not available */
    err = get_sff_address_from_csr_guid (camera, &(feature_desc->csr_guid), &feature_address);
    DC1394_ERR_RTN(err, "Cannot get SFF address from GUID");

    if (feature_address == 0)
        *available = DC1394_FALSE;
    else
        *available = DC1394_TRUE;
    return DC1394_SUCCESS;
}

/*
 * enables a specific SFF feature
 */
dc1394error_t
dc1394_basler_sff_feature_enable (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, dc1394switch_t on_off)
{
    const sff_feature *feature_desc = NULL;
    uint64_t feature_address;
    uint32_t data;
    dc1394error_t err;
    dc1394bool_t is_enabled;

    if (camera == NULL) {
        err = DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "camera is NULL");
    }

    feature_desc = basler_sff_registry_find_by_id (feature_id);
    if (feature_desc == NULL)
        return DC1394_FAILURE;

    /* check if this feature can be enabled by the generic function */
    if (!feature_desc->generic) {
        err = DC1394_FUNCTION_NOT_SUPPORTED;
        DC1394_ERR_RTN(err, "cannot enable feature with the generic enable function");
    }

    /* we need to enable the extended data stream first if this chunk has image data */
    if (feature_desc->has_chunk && feature_id != DC1394_BASLER_SFF_EXTENDED_DATA_STREAM) {
        err = dc1394_basler_sff_feature_is_enabled (camera, DC1394_BASLER_SFF_EXTENDED_DATA_STREAM, &is_enabled);
        DC1394_ERR_RTN (err, "Failed to get extended_data_stream status");
        if (!is_enabled) {
            err = dc1394_basler_sff_feature_enable (camera, DC1394_BASLER_SFF_EXTENDED_DATA_STREAM, DC1394_ON);
            DC1394_ERR_RTN(err, "cannot enable Extended_Data_Stream feature prior to enabling feature");
        }
    }

    err = get_sff_address_from_csr_guid (camera, &(feature_desc->csr_guid), &feature_address);
    DC1394_ERR_RTN(err, "Cannot get SFF address from GUID");

    if (feature_address == 0)
        return DC1394_FAILURE;

    err = dc1394_get_register (camera, feature_address, &data);
    DC1394_ERR_RTN(err, "Cannot read SFF feature CSR register");
    //fprintf (stderr, "%s: address = 0x%016"PRIx64" read data = 0x%08x\n", __FUNCTION__, feature_address, data);

    /* enable or disable */
    if (on_off) {
        data |= BASLER_SFF_CSR_BIT_ENABLE;
    } else {
        data &= BASLER_SFF_CSR_BIT_ENABLE;
    }
    err = dc1394_set_register (camera, feature_address, data);
    DC1394_ERR_RTN(err, "cannot write to feature CSR");
    //fprintf (stderr, "%s: address = 0x%016"PRIx64" write data = 0x%08x\n", __FUNCTION__, feature_address, data);

    /* check if it was enabled or disabled correctly */
    err = dc1394_basler_sff_feature_is_enabled (camera, feature_id, &is_enabled);
    DC1394_ERR_RTN(err, "cannot check if feature was enabled or disabled correctly");

    if (on_off != is_enabled) {
        err = DC1394_FAILURE;
        DC1394_ERR_RTN(err, "camera reported that the feature was not in the proper state (enabled or disabled)");
    }

    return DC1394_SUCCESS;
}

/*
 * check if a feature is enabled or not
 */
dc1394error_t
dc1394_basler_sff_feature_is_enabled (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, dc1394bool_t *is_enabled)
{
    const sff_feature *feature_desc = NULL;
    uint64_t feature_address;
    uint32_t data;
    dc1394error_t err;

    if (camera == NULL || is_enabled == NULL) {
        err = DC1394_INVALID_ARGUMENT_VALUE;
        DC1394_ERR_RTN(err, "camera or is_enabled is NULL");
    }

    feature_desc = basler_sff_registry_find_by_id (feature_id);
    if (feature_desc == NULL)
        return DC1394_FAILURE;

    /* check if this feature can be enabled by the generic function */
    if (!feature_desc->generic) {
        err = DC1394_FUNCTION_NOT_SUPPORTED;
        DC1394_ERR_RTN(err, "cannot check feature with the generic enable function");
    }

    err = get_sff_address_from_csr_guid (camera, &(feature_desc->csr_guid), &feature_address);
    DC1394_ERR_RTN(err, "Cannot get SFF address from GUID");

    if (feature_address == 0)
        return DC1394_FAILURE;

    err = dc1394_get_register (camera, feature_address, &data);
    DC1394_ERR_RTN(err, "Cannot read SFF feature CSR register");
    //fprintf (stderr, "%s: address = 0x%016"PRIx64" data = 0x%08x\n", __FUNCTION__, feature_address, data);

    if (data & BASLER_SFF_CSR_BIT_ENABLE) {
        *is_enabled = DC1394_TRUE;
    } else {
        *is_enabled = DC1394_FALSE;
    }
    return DC1394_SUCCESS;
}

/*
 * print a feature
 */
dc1394error_t
dc1394_basler_sff_feature_print (dc1394camera_t* camera, dc1394basler_sff_feature_t feature_id, FILE *fd)
{
    dc1394error_t err;
    dc1394bool_t available;
    uint64_t feature_address;
    const sff_feature* feature_desc;

    feature_desc = basler_sff_registry_find_by_id (feature_id);
    if (feature_desc == NULL)
        return DC1394_FAILURE;

    if (camera == NULL) {
    offline:
        fprintf (fd,"Name      : %s\n"
                 "CSR guid  : %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
                 feature_desc->name,
                 feature_desc->csr_guid.d1, feature_desc->csr_guid.d2, feature_desc->csr_guid.d3,
                 feature_desc->csr_guid.d4[0], feature_desc->csr_guid.d4[1], feature_desc->csr_guid.d4[2], feature_desc->csr_guid.d4[3],
                 feature_desc->csr_guid.d4[4], feature_desc->csr_guid.d4[5], feature_desc->csr_guid.d4[6], feature_desc->csr_guid.d4[7]);
        if (feature_desc->has_chunk) {
            fprintf (fd,"Has chunk : false\n"
                     "CHUNK guid: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
                     feature_desc->chunk_guid.d1, feature_desc->chunk_guid.d2, feature_desc->chunk_guid.d3,
                     feature_desc->chunk_guid.d4[0], feature_desc->chunk_guid.d4[1], feature_desc->chunk_guid.d4[2], feature_desc->chunk_guid.d4[3],
                     feature_desc->chunk_guid.d4[4], feature_desc->chunk_guid.d4[5], feature_desc->chunk_guid.d4[6], feature_desc->chunk_guid.d4[7]);
        } else {
            fprintf (fd,"Has chunk : false\n");
        }
        return DC1394_SUCCESS;
    } else {
        dc1394_basler_sff_is_available (camera, &available);
        if (!available)
            goto offline;
        dc1394_basler_sff_feature_is_available (camera, feature_id, &available);
        if (!available)
            goto offline;

        fprintf (fd,"Name      : %s\n"
                 "CSR guid  : %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
                 feature_desc->name,
                 feature_desc->csr_guid.d1, feature_desc->csr_guid.d2, feature_desc->csr_guid.d3,
                 feature_desc->csr_guid.d4[0], feature_desc->csr_guid.d4[1], feature_desc->csr_guid.d4[2], feature_desc->csr_guid.d4[3],
                 feature_desc->csr_guid.d4[4], feature_desc->csr_guid.d4[5], feature_desc->csr_guid.d4[6], feature_desc->csr_guid.d4[7]);
        if (feature_desc->has_chunk) {
            fprintf (fd,"Has chunk : true\n"
                     "CHUNK guid: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
                     feature_desc->chunk_guid.d1, feature_desc->chunk_guid.d2, feature_desc->chunk_guid.d3,
                     feature_desc->chunk_guid.d4[0], feature_desc->chunk_guid.d4[1], feature_desc->chunk_guid.d4[2], feature_desc->chunk_guid.d4[3],
                     feature_desc->chunk_guid.d4[4], feature_desc->chunk_guid.d4[5], feature_desc->chunk_guid.d4[6], feature_desc->chunk_guid.d4[7]);
        } else {
            fprintf (fd,"Has chunk : false\n");
        }
        fprintf (fd,"Available : true\n");

        err = get_sff_address_from_csr_guid (camera, &(feature_desc->csr_guid), &feature_address);
        if (err == DC1394_SUCCESS) {
            fprintf (fd,"Address   : 0x%016"PRIx64"\n", feature_address);
        } else {
            fprintf (fd,"Address   : unavailable\n");
        }
    }
    return DC1394_SUCCESS;
}

dc1394error_t dc1394_basler_sff_feature_print_all (dc1394camera_t* camera, FILE *fd)
{
    uint32_t i = DC1394_BASLER_SFF_FEATURE_MIN;
    while (i < DC1394_BASLER_SFF_FEATURE_MAX) {
        dc1394_basler_sff_feature_print (camera, i, fd);
        fprintf (fd, "\n");
        i++;
    }
    return DC1394_SUCCESS;
}

dc1394bool_t dc1394_basler_sff_check_crc (const uint8_t* frame_buffer, uint32_t frame_size)
{
    uint32_t current_crc, desired_crc;

    /* calculate the checksum, ignoring the last four bytes which is the checksum chunk */
    current_crc = dc1394_checksum_crc16 (frame_buffer, frame_size - sizeof(dc1394basler_sff_crc_checksum_t));

    /* retrieve the desired checksum from the buffer */
    desired_crc = ((uint32_t*)frame_buffer)[frame_size / sizeof(uint32_t) - 1];
    return current_crc == desired_crc;
}

/*
 * Initializes the struct for iterating
 */
dc1394error_t dc1394_basler_sff_chunk_iterate_init (dc1394basler_sff_t* chunk, void *frame_buffer, uint32_t frame_size, dc1394bool_t has_crc_checksum)
{
    if (chunk == NULL || frame_buffer == NULL || frame_size == 0)
        return DC1394_FAILURE;

    chunk->feature_id = DC1394_BASLER_SFF_FEATURE_MAX; /* invalid feature id */
    chunk->frame_buffer = frame_buffer;
    chunk->frame_size = frame_size;

    /* if this frame has a checksum, we move the end position by
     * sizeof(crc_checksum) to the beginning, because we are not
     * interested in parsing it */
    if (has_crc_checksum)
        chunk->frame_size -= sizeof(dc1394basler_sff_crc_checksum_t);
    chunk->current_iter = chunk->frame_buffer + chunk->frame_size;
    chunk->chunk_data = NULL;
    return DC1394_SUCCESS;
}

/**
 * Iterates over the available SFF chunks in the frame buffer
 */
dc1394error_t dc1394_basler_sff_chunk_iterate (dc1394basler_sff_t* chunk)
{
    dc1394basler_sff_chunk_tail_t* tail;
    const sff_feature* feature_desc;

    if (chunk == NULL || chunk->current_iter == NULL || chunk->frame_buffer == NULL)
        return DC1394_INVALID_ARGUMENT_VALUE;

    if (chunk->frame_buffer >= chunk->current_iter ||
        chunk->current_iter - chunk->frame_buffer <= sizeof(dc1394basler_sff_chunk_tail_t))
        return DC1394_BASLER_NO_MORE_SFF_CHUNKS;

    /* try to extract at least one chunk tail */
    tail = chunk->current_iter - sizeof(dc1394basler_sff_chunk_tail_t);

    /* check if the size field is correct */
    if (~(tail->chunk_size) != tail->inverted_chunk_size)
        return DC1394_BASLER_CORRUPTED_SFF_CHUNK;

    if (chunk->current_iter - chunk->frame_buffer < tail->chunk_size)
        return DC1394_BASLER_CORRUPTED_SFF_CHUNK;

    /* check if we know this chunk's type */
    feature_desc = basler_sff_registry_find_by_chunk_guid (&(tail->chunk_guid));
    if (feature_desc == NULL)
        return DC1394_BASLER_UNKNOWN_SFF_CHUNK;

    chunk->feature_id = feature_desc->feature_id;

    /* set data pointer, here we cannot use the size field from the chunk tail
     * because one chunk type the extended data stream has not the same in size
     * in the image stream as has the C type, therefore for clarities sake
     * the size of the C type is stored in the feature_desc */
    chunk->chunk_data = chunk->current_iter - feature_desc->data_size;

    /* but we must move the current_iter by the size in the tail */
    chunk->current_iter -= tail->chunk_size;
    return DC1394_SUCCESS;
}

/**
 * Finds a specific SFF chunk in the frame buffer
 */
dc1394error_t dc1394_basler_sff_chunk_find (dc1394basler_sff_feature_t feature_id, void** chunk_data, void* frame_buffer, uint32_t frame_size, dc1394bool_t has_crc_checksum)
{
    dc1394basler_sff_t chunk;
    dc1394bool_t found = DC1394_FALSE;
    dc1394error_t err;

    err = dc1394_basler_sff_chunk_iterate_init (&chunk, frame_buffer, frame_size, has_crc_checksum);
    DC1394_ERR_RTN(err, "dc1394_basler_sff_chunk_find(): dc1394_basler_sff_chunk_iterate_init() failed");

    while ((err = dc1394_basler_sff_chunk_iterate (&chunk)) == DC1394_SUCCESS) {
        if (chunk.feature_id == feature_id) {
            found = DC1394_TRUE;
            break;
        }
    }

    if (!found)
        return DC1394_FAILURE;

    if (chunk_data)
        *chunk_data = chunk.chunk_data;
    return DC1394_SUCCESS;
}
