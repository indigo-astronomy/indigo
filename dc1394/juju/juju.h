/*
 * 1394-Based Digital Camera Control Library
 *
 * Juju backend for dc1394
 *
 * Written by Kristian Hoegsberg <krh@bitplanet.net>
 * Maintained by David Moore <dcm@acm.org>
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

#ifndef __DC1394_JUJU_H__
#define __DC1394_JUJU_H__

#include "firewire-cdev.h"
#include "config.h"
#include "internal.h"
#include "register.h"
#include "offsets.h"

struct _platform_t {
    int dummy;
};

typedef struct _juju_iso_info {
    int got_alloc;
    int got_dealloc;
    int handle;
    int channel;
    int bandwidth;
    struct _juju_iso_info *next;
} juju_iso_info;

struct _platform_camera_t {
    int fd;
    char filename[32];
    int generation;
    uint32_t node_id;
    unsigned int kernel_abi_version;
    int max_response_quads;
    juju_iso_info *iso_resources;
    uint8_t header_size;
    uint8_t broadcast_enabled;

    dc1394camera_t * camera;

    int iso_fd;
    int iso_handle;
    struct juju_frame * frames;
    unsigned char * buffer;
    size_t buffer_size;
    uint32_t flags;
    unsigned int num_frames;
    int current;

    unsigned int iso_channel;
    int capture_is_set;
    int iso_auto_started;
    juju_iso_info *capture_iso_resource;
	uint32_t packets_per_frame; // HPK 20161209
};


struct juju_frame {
    dc1394video_frame_t frame;
    size_t size;
    struct fw_cdev_iso_packet *packets;
};

dc1394error_t
dc1394_juju_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
        uint32_t flags);

dc1394error_t
dc1394_juju_capture_stop(platform_camera_t *craw);

dc1394error_t
dc1394_juju_capture_dequeue (platform_camera_t * craw,
        dc1394capture_policy_t policy, dc1394video_frame_t **frame_return);

dc1394error_t
dc1394_juju_capture_enqueue (platform_camera_t * craw,
        dc1394video_frame_t * frame);

int
dc1394_juju_capture_get_fileno (platform_camera_t * craw);

dc1394error_t
juju_iso_allocate (platform_camera_t *cam, uint64_t allowed_channels,
        int bandwidth_units, juju_iso_info **out);

dc1394error_t
juju_iso_deallocate (platform_camera_t *cam, juju_iso_info * res);

#endif
