/*
 * 1394-Based Digital Camera Control Library
 *
 * Linux specific type definitions
 *  
 * Written by
 *   Damien Douxchamps <ddouxchamps@users.sf.net>
 *   David Moore <dcm@acm.org>
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

#ifndef __DC1394_LINUX_H__
#define __DC1394_LINUX_H__

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
//#include "linux/raw1394support.h"
#include <dc1394/dc1394.h>
#include "internal.h"

struct _platform_t {
    int dummy;
};

typedef struct __dc1394_capture
{
    unsigned char            *capture_buffer;
    /* components needed for the DMA based video capture */
    const unsigned char     *dma_ring_buffer;
    char                    *dma_device_file;
    unsigned int             dma_buffer_size;
    unsigned int             dma_frame_size;
    unsigned int             num_dma_buffers;
    unsigned int             dma_last_buffer;
    int                      dma_fd;
    raw1394handle_t          handle;
    uint32_t                 flags;

    dc1394video_frame_t     *frames;
} dc1394capture_t;

struct _platform_camera_t {
    raw1394handle_t handle;
    int node, port;
    uint32_t generation;

    dc1394camera_t * camera;

    dc1394capture_t capture;

    int                      capture_is_set;
    int                      allocated_channel;
    unsigned int             allocated_bandwidth;
    int                      iso_auto_started;
    unsigned int             iso_channel;

    // for broadcast:
    int           backup_node_id;
    dc1394bool_t broadcast_is_set;
};

dc1394error_t
dc1394_linux_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
        uint32_t flags);

dc1394error_t
dc1394_linux_capture_stop(platform_camera_t *craw);

dc1394error_t
dc1394_linux_capture_dequeue (platform_camera_t * craw,
        dc1394capture_policy_t policy, dc1394video_frame_t **frame_return);

dc1394error_t
dc1394_linux_capture_enqueue (platform_camera_t * craw,
        dc1394video_frame_t * frame);

int
dc1394_linux_capture_get_fileno (platform_camera_t * craw);

#endif
