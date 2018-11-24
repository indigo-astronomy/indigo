/*
 * 1394-Based Digital Camera Control Library
 *
 * Mac OS X specific headers
 *
 * Written by David Moore <dcm@acm.org>
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

#ifndef __DC1394_MACOSX_MACOSX_H__
#define __DC1394_MACOSX_MACOSX_H__

#include <dc1394/dc1394.h>
#include <IOKit/firewire/IOFireWireLib.h>
#include <CoreServices/CoreServices.h>

#include "platform.h"

typedef enum {
    BUFFER_EMPTY = 0,
    BUFFER_FILLED = 1,
    BUFFER_CORRUPT = 2,
    BUFFER_ENQUEUED = 3,
} buffer_status;

typedef struct _packet_info {
    UInt32 header;
    UInt32 status;
    UInt32 timestamp;
} packet_info;

typedef struct _buffer_info {
    platform_camera_t * craw;
    int              i;
    buffer_status    status;
    struct timeval   filltime;
    int              num_dcls;
    NuDCLRef *       dcl_list;
    packet_info *    pkts;
} buffer_info;

struct _platform_t {
    int dummy;
};

typedef struct __dc1394_capture
{
    unsigned int             num_frames;
    //int                      frame_pages;
    int                      last_dequeued;
    int                      last_enqueued;
    /* components needed for the DMA based video capture */
    IOFireWireLibIsochChannelRef    chan;
    IOFireWireLibRemoteIsochPortRef rem_port;
    IOFireWireLibLocalIsochPortRef  loc_port;
    IOFireWireLibNuDCLPoolRef       dcl_pool;
    IOVirtualRange           databuf;
    buffer_info *            buffers;
    CFRunLoopRef             run_loop;
    CFStringRef              run_loop_mode;
    dc1394capture_callback_t callback;
    void *                   callback_user_data;
    int                      notify_pipe[2];
    uint8_t                  frames_ready;
    MPCriticalRegionID       mutex;
    MPQueueID                termination_queue;
    MPSemaphoreID            thread_init_semaphore;
    MPTaskID                 task;
    CFSocketRef              socket;
    CFRunLoopSourceRef       socket_source;
    uint8_t                  iso_is_allocated;
    uint8_t                  iso_is_started;

    uint32_t                 flags;
    Boolean                  do_irm;

    dc1394video_frame_t     *frames;
} dc1394capture_t;

struct _platform_camera_t {
    IOFireWireLibDeviceRef  iface;
    UInt32                  generation;

    dc1394camera_t * camera;

    dc1394capture_t         capture;

    int                     capture_is_set;
    int                     iso_channel_is_set;
    int                     iso_channel;
    int                     iso_auto_started;
};

dc1394error_t
dc1394_macosx_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
                     uint32_t flags);
dc1394error_t
dc1394_macosx_capture_stop(platform_camera_t *craw);
dc1394error_t
dc1394_macosx_capture_dequeue (platform_camera_t * craw,
                        dc1394capture_policy_t policy,
                        dc1394video_frame_t **frame);
dc1394error_t
dc1394_macosx_capture_enqueue (platform_camera_t * craw,
                        dc1394video_frame_t * frame);
dc1394bool_t
dc1394_macosx_capture_is_frame_corrupt (platform_camera_t * craw,
        dc1394video_frame_t * frame);
int
dc1394_macosx_capture_get_fileno (platform_camera_t * craw);

dc1394error_t
dc1394_macosx_capture_set_callback (platform_camera_t * camera,
        dc1394capture_callback_t callback, void * user_data);

dc1394error_t
dc1394_macosx_capture_schedule_with_runloop (platform_camera_t * camera,
        CFRunLoopRef run_loop, CFStringRef run_loop_mode);

#endif
