/*
 * 1394-Based Digital Camera Control Library
 *
 * Juju backend for dc1394
 * 
 * Written by Kristian Hoegsberg <krh@bitplanet.net>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <poll.h>
#include <inttypes.h>

#include "juju/juju.h"

#define ptr_to_u64(p) ((__u64)(unsigned long)(p))
#define u64_to_ptr(p) ((void *)(unsigned long)(p))

static dc1394error_t
init_frame(platform_camera_t *craw, int index, dc1394video_frame_t *proto)
{
    int N = 8;        /* Number of iso packets per fw_cdev_iso_packet. */
    struct juju_frame *f = craw->frames + index;
    size_t total;
    int i, count;

    memcpy (&f->frame, proto, sizeof f->frame);
    f->frame.image = craw->buffer + index * proto->total_bytes;
    f->frame.id = index;
    count = (proto->packets_per_frame + N - 1) / N;
    f->size = count * sizeof *f->packets;
    f->packets = malloc(f->size);
    if (f->packets == NULL)
        return DC1394_MEMORY_ALLOCATION_FAILURE;

    memset(f->packets, 0, f->size);

    total = proto->packets_per_frame;
    for (i = 0; i < count; i++) {
        if (total < N)
            N = total;
        f->packets[i].control = FW_CDEV_ISO_HEADER_LENGTH(craw->header_size * N)
            | FW_CDEV_ISO_PAYLOAD_LENGTH(proto->packet_size * N);
        total -= N;
    }
    f->packets[0].control |= FW_CDEV_ISO_SKIP;
    f->packets[i - 1].control |= FW_CDEV_ISO_INTERRUPT;

    return DC1394_SUCCESS;
}

static void
release_frame(platform_camera_t *craw, int index)
{
    struct juju_frame *f = craw->frames + index;

    free(f->packets);
}

dc1394error_t
queue_frame (platform_camera_t *craw, int index)
{
    struct juju_frame *f = craw->frames + index;
    struct fw_cdev_queue_iso queue;
    int retval;

    queue.size = f->size;
    queue.data = ptr_to_u64(f->frame.image);
    queue.packets = ptr_to_u64(f->packets);
    queue.handle = craw->iso_handle;

    retval = ioctl(craw->iso_fd, FW_CDEV_IOC_QUEUE_ISO, &queue);
    if (retval < 0) {
        dc1394_log_error("queue_iso failed; %m");
        return DC1394_IOCTL_FAILURE;
    }

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_juju_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
        uint32_t flags)
{
    struct fw_cdev_create_iso_context create;
    struct fw_cdev_start_iso start_iso;
    dc1394error_t err;
    dc1394video_frame_t proto;
    int i, j, retval;
    dc1394camera_t * camera = craw->camera;

    if (flags & DC1394_CAPTURE_FLAGS_DEFAULT)
        flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
            DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC;

    craw->flags = flags;

    // if capture is already set, abort
    if (craw->capture_is_set>0)
        return DC1394_CAPTURE_IS_RUNNING;

    // if auto iso is requested, stop ISO (if necessary)
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        dc1394switch_t is_iso_on;
        dc1394_video_get_transmission(camera, &is_iso_on);
        if (is_iso_on == DC1394_ON) {
            err=dc1394_video_set_transmission(camera, DC1394_OFF);
            DC1394_ERR_RTN(err,"Could not stop ISO!");
        }
    }

    if (capture_basic_setup(camera, &proto) != DC1394_SUCCESS) {
        dc1394_log_error("basic setup failed");
        return DC1394_FAILURE;
    }

    if (flags & (DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
                DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC)) {
        uint64_t channels_allowed = 0;
        unsigned int bandwidth_units = 0;
        int channel;
        if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC)
            channels_allowed = 0xffff;
        if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC)
            dc1394_video_get_bandwidth_usage (camera, &bandwidth_units);
        err = juju_iso_allocate (craw, channels_allowed,
                bandwidth_units, &craw->capture_iso_resource);
        if (err == DC1394_SUCCESS) {
            channel = craw->capture_iso_resource->channel;
        } else if (err == DC1394_FUNCTION_NOT_SUPPORTED) {
            channel = craw->node_id & 0x3f;
            dc1394_log_warning ("iso allocation not available in this kernel, "
                    "using channel %d...", channel);
        } else {
            dc1394_log_error ("juju: Failed to allocate iso resources");
            return err;
        }

        if (dc1394_video_set_iso_channel (camera, channel) != DC1394_SUCCESS)
            return DC1394_NO_ISO_CHANNEL;
    }

    if (dc1394_video_get_iso_channel (camera, &craw->iso_channel)
            != DC1394_SUCCESS)
        return DC1394_FAILURE;
    dc1394_log_debug ("juju: Receiving from iso channel %d", craw->iso_channel);

    craw->iso_fd = open(craw->filename, O_RDWR);
    if (craw->iso_fd < 0) {
        dc1394_log_error("error opening file: %s", strerror (errno));
        return DC1394_FAILURE;
    }

    create.type = FW_CDEV_ISO_CONTEXT_RECEIVE;
    create.header_size = craw->header_size;
    create.channel = craw->iso_channel;
    // create.speed = SCODE_400; // not necessary: ignored by kernel for ISO reception.
    err = DC1394_IOCTL_FAILURE;
    if (ioctl(craw->iso_fd, FW_CDEV_IOC_CREATE_ISO_CONTEXT, &create) < 0) {
        dc1394_log_error("failed to create iso context");
        goto error_fd;
    }

    craw->iso_handle = create.handle;

    craw->num_frames = num_dma_buffers;
    craw->current = -1;
    craw->buffer_size = proto.total_bytes * num_dma_buffers;
    craw->buffer =
        mmap(NULL, craw->buffer_size, PROT_READ | PROT_WRITE , MAP_SHARED, craw->iso_fd, 0);
    err = DC1394_IOCTL_FAILURE;
    if (craw->buffer == MAP_FAILED)
        goto error_fd;

    err = DC1394_MEMORY_ALLOCATION_FAILURE;
    craw->frames = malloc (num_dma_buffers * sizeof *craw->frames);
    if (craw->frames == NULL)
        goto error_mmap;

    for (i = 0; i < num_dma_buffers; i++) {
        err = init_frame(craw, i, &proto);
        if (err != DC1394_SUCCESS) {
            dc1394_log_error("error initing frames");
            break;
        }
    }
    if (err != DC1394_SUCCESS) {
        for (j = 0; j < i; j++)
            release_frame(craw, j);
        goto error_mmap;
    }

    for (i = 0; i < num_dma_buffers; i++) {
        err = queue_frame(craw, i);
        if (err != DC1394_SUCCESS) {
            dc1394_log_error("error queuing");
            goto error_frames;
        }
    }

	craw->packets_per_frame = proto.packets_per_frame; // HPK 20161209

    // starting from here we use the ISO channel so we set the flag in
    // the camera struct:
    craw->capture_is_set = 1;

    start_iso.cycle   = -1;
    start_iso.tags = FW_CDEV_ISO_CONTEXT_MATCH_ALL_TAGS;
    start_iso.sync = 1;
    start_iso.handle = craw->iso_handle;
    retval = ioctl(craw->iso_fd, FW_CDEV_IOC_START_ISO, &start_iso);
    err = DC1394_IOCTL_FAILURE;
    if (retval < 0) {
        dc1394_log_error("error starting iso");
        goto error_frames;
    }

    // if auto iso is requested, start ISO
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        err=dc1394_video_set_transmission(camera, DC1394_ON);
        DC1394_ERR_RTN(err,"Could not start ISO!");
        craw->iso_auto_started=1;
    }

    return DC1394_SUCCESS;

error_frames:
    for (i = 0; i < num_dma_buffers; i++)
        release_frame(craw, i);
error_mmap:
    munmap(craw->buffer, craw->buffer_size);
error_fd:
    close(craw->iso_fd);

    return err;
}

dc1394error_t
dc1394_juju_capture_stop(platform_camera_t *craw)
{
    dc1394camera_t * camera = craw->camera;
    struct fw_cdev_stop_iso stop;
    int i;

    if (craw->capture_is_set == 0)
        return DC1394_CAPTURE_IS_NOT_SET;

    stop.handle = craw->iso_handle;
    if (ioctl(craw->iso_fd, FW_CDEV_IOC_STOP_ISO, &stop) < 0)
        return DC1394_IOCTL_FAILURE;

    munmap(craw->buffer, craw->buffer_size);
    close(craw->iso_fd);
    for (i = 0; i<craw->num_frames; i++)
        release_frame(craw, i);
    free (craw->frames);
    craw->frames = NULL;
    craw->capture_is_set = 0;

    if (craw->capture_iso_resource) {
        if (juju_iso_deallocate (craw, craw->capture_iso_resource) < 0)
            dc1394_log_warning ("juju: Failed to deallocate iso resources");
        craw->capture_iso_resource = NULL;
    }

    // stop ISO if it was started automatically
    if (craw->iso_auto_started>0) {
        dc1394error_t err=dc1394_video_set_transmission(camera, DC1394_OFF);
        DC1394_ERR_RTN(err,"Could not stop ISO!");
        craw->iso_auto_started=0;
    }

    return DC1394_SUCCESS;
}

static uint32_t
bus_time_to_usec (uint32_t bus)
{
    uint32_t sec      = (bus & 0xe000000) >> 25;
    uint32_t cycles   = (bus & 0x1fff000) >> 12;
    uint32_t subcycle = (bus & 0x0000fff);
    return sec * 1000000 + cycles * 125 + subcycle * 125 / 3072;
}

dc1394error_t
dc1394_juju_capture_dequeue (platform_camera_t * craw,
        dc1394capture_policy_t policy, dc1394video_frame_t **frame_return)
{
    struct pollfd fds[1];
    struct juju_frame *f;
    int err, len;
    struct fw_cdev_get_cycle_timer tm;

	if(craw->frames==NULL || craw->capture_is_set==0) {
		*frame_return=NULL;
		return DC1394_CAPTURE_IS_NOT_SET;
	}

    struct {
        struct fw_cdev_event_iso_interrupt i;
		__u32 headers[craw->packets_per_frame*2 + 16]; // HPK 20161209
    } iso;

    if ( (policy<DC1394_CAPTURE_POLICY_MIN) || (policy>DC1394_CAPTURE_POLICY_MAX) )
        return DC1394_INVALID_CAPTURE_POLICY;

    // default: return NULL in case of failures or lack of frames
    *frame_return=NULL;

    fds[0].fd = craw->iso_fd;
    fds[0].events = POLLIN;

    while (1) {
        err = poll(fds, 1, (policy == DC1394_CAPTURE_POLICY_POLL) ? 0 : -1);
        if (err < 0) {
            if (errno == EINTR)
                continue;
            dc1394_log_error("poll() failed for device %s.", craw->filename);
            return DC1394_FAILURE;
        } else if (err == 0) {
            return DC1394_SUCCESS;
        }

        len = read (craw->iso_fd, &iso, sizeof iso);
        if (len < 0) {
            dc1394_log_error("Juju: dequeue failed to read a response: %m");
            return DC1394_FAILURE;
        }

        if (iso.i.type == FW_CDEV_EVENT_ISO_INTERRUPT)
            break;
    }

    craw->current = (craw->current + 1) % craw->num_frames;
    f = craw->frames + craw->current;

    dc1394_log_debug("Juju: got iso event, cycle 0x%04x, header_len %d",
            iso.i.cycle, iso.i.header_length);

    f->frame.frames_behind = 0;
    f->frame.timestamp = 0;

    /* Compute timestamp */
    if (ioctl(craw->iso_fd, FW_CDEV_IOC_GET_CYCLE_TIMER, &tm) == 0) {
        /* Current bus time in usec as retrieved by the ioctl */
        uint32_t bus_time = bus_time_to_usec(tm.cycle_timer);
        /* Bus time of the interrupt packet (end of frame) */
        uint32_t dma_time = iso.i.cycle;
        /* Estimated usec between start of frame and end of frame */
        uint32_t diff =
            (craw->frames[0].frame.packets_per_frame - 1) * 125;

        /* If per-packet timestamps are available in the headers use them */
        if (craw->header_size >= 8) {
            uint8_t * b = (uint8_t *)(iso.i.header + 1);
            /* Bus time of the first frame in the packet */
            dma_time = (b[2] << 8) | b[3];
            dc1394_log_debug("Juju: using cycle 0x%04x (diff was %d)",
                    dma_time, diff);
            diff = 0;
        }
        /* Convert to usec */
        dma_time = bus_time_to_usec(dma_time << 12);

        /* Amount to subtract from local_time to get frame start time */
        diff += (bus_time + 8000000 - dma_time) % 8000000;
        dc1394_log_debug("Juju: frame latency %d us", diff);

        f->frame.timestamp = tm.local_time - diff;
    }

    *frame_return = &f->frame;

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_juju_capture_enqueue (platform_camera_t * craw,
        dc1394video_frame_t * frame)
{
    dc1394camera_t * camera = craw->camera;
    int err;

    err = DC1394_INVALID_ARGUMENT_VALUE;
    if (frame->camera != camera)
        DC1394_ERR_RTN(err, "camera does not match frame's camera");

    err = queue_frame (craw, frame->id);
    DC1394_ERR_RTN(err, "Failed to queue frame");

    return DC1394_SUCCESS;
}

int
dc1394_juju_capture_get_fileno (platform_camera_t * craw)
{
    return craw->iso_fd;
}

