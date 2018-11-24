/*
 * 1394-Based Digital Camera Control Library
 *
 * IIDC-over-USB using libusb backend for dc1394
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#ifdef HAVE_MACOSX
#include <CoreFoundation/CoreFoundation.h>
#endif
#include "usb/usb.h"

// LIBUSB_CALL only defined for latest libusb versions.
#ifndef LIBUSB_CALL
#define LIBUSB_CALL
#endif

/* Callback whenever a bulk transfer finishes. */
static void
LIBUSB_CALL callback (struct libusb_transfer * transfer)
{
	// Get a software timestamp as soon as possible in this callback. Note that this timestamp
	// is not as accurate as the bus timestamp we get with the IEEE1394 interface. For more
	// accurate timings, consider using either a hardware external trigger or the camera
	// timestamps that can be inserted within the video frame (see Point Grey documentation)
	struct timeval filltime;
	gettimeofday(&filltime,NULL);

    struct usb_frame * f = transfer->user_data;
    platform_camera_t * craw = f->pcam;

    if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
        dc1394_log_warning ("usb: Bulk transfer %d cancelled", f->frame.id);
        return;
    }

    dc1394_log_debug ("usb: Bulk transfer %d complete, %d of %d bytes",
            f->frame.id, transfer->actual_length, transfer->length);
    int status = BUFFER_FILLED;
    if (transfer->actual_length < transfer->length)
        status = BUFFER_CORRUPT;

    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        dc1394_log_error ("usb: Bulk transfer %d failed with code %d",
                f->frame.id, transfer->status);
        status = BUFFER_ERROR;
    }

    pthread_mutex_lock (&craw->mutex);
    f->status = status;
	f->frame.timestamp = filltime.tv_sec*1000000 + filltime.tv_usec;
    craw->frames_ready++;
    pthread_mutex_unlock (&craw->mutex);

    if (write (craw->notify_pipe[1], "+", 1)!=1) {
        dc1394_log_error ("usb: Failed to write to notify pipe");
        // we may need to set the status to BUFFER_ERROR here
    }
}

#ifdef HAVE_MACOSX
static void
socket_callback_usb (CFSocketRef s, CFSocketCallBackType type,
                 CFDataRef address, const void * data, void * info)
{
    platform_camera_t * craw = info;
    dc1394capture_t * capture = &(craw->capture);
    if (capture->callback) {
        capture->callback (craw->camera, capture->callback_user_data);
    }
}
#endif

static void *
capture_thread (void * arg)
{
    platform_camera_t * craw = arg;

    dc1394_log_debug ("usb: Helper thread starting");

    while (1) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 100000,
        };
        libusb_handle_events_timeout(craw->thread_context, &tv);
        pthread_mutex_lock (&craw->mutex);
        if (craw->kill_thread)
            break;
        pthread_mutex_unlock (&craw->mutex);
    }
    pthread_mutex_unlock (&craw->mutex);
    dc1394_log_debug ("usb: Helper thread ending");
    return NULL;
}

static int
find_interface (libusb_device * usb_dev, uint8_t endpoint_address)
{
    struct libusb_device_descriptor usb_desc;
    struct libusb_config_descriptor * config_desc;
    int ret;
    int i, a, e;
    uint8_t ea;

    ret = libusb_get_device_descriptor(usb_dev, &usb_desc);

    if (ret < 0) {
        dc1394_log_error ("usb: Failed to get device descriptor");
        return DC1394_FAILURE;
    }

    if (usb_desc.bNumConfigurations) {
        ret = libusb_get_active_config_descriptor (usb_dev, &config_desc);
        if (ret) {
            dc1394_log_error ("usb: Failed to get config descriptor");
            return DC1394_FAILURE;
        }

        for (i = 0; i < config_desc->bNumInterfaces; i++) {
            for (a = 0; a < config_desc->interface[i].num_altsetting; a++) {
                for (e = 0; e < config_desc->interface[i].altsetting[a].bNumEndpoints; e++) {
                    ea = config_desc->interface[i].altsetting[a].endpoint[e].bEndpointAddress;
                    // Return the interface number for the first suitable interface
                    if (ea == endpoint_address)
                        return i;
                }
            }
        }
    }

    return DC1394_FAILURE;
}

static dc1394error_t
init_frame(platform_camera_t *craw, int index, dc1394video_frame_t *proto, size_t padded_frame_size)
{
    struct usb_frame *f = craw->frames + index;

    memcpy (&f->frame, proto, sizeof f->frame);
    f->frame.image = craw->buffer + index * padded_frame_size;
    f->frame.id = index;
    f->transfer = libusb_alloc_transfer (0);
    f->pcam = craw;
    f->status = BUFFER_EMPTY;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_usb_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
        uint32_t flags)
{
    dc1394video_frame_t proto;
    int i;
    dc1394camera_t * camera = craw->camera;

#ifdef HAVE_MACOSX
    dc1394capture_t * capture = &(craw->capture);
    CFSocketContext socket_context = { 0, craw, NULL, NULL, NULL };
#endif

    // if capture is already set, abort
    if (craw->capture_is_set > 0)
        return DC1394_CAPTURE_IS_RUNNING;


    if (flags & DC1394_CAPTURE_FLAGS_DEFAULT)
        flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
            DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC;

    craw->flags = flags;

    if (capture_basic_setup(camera, &proto) != DC1394_SUCCESS) {
        dc1394_log_error("usb: Basic capture setup failed");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }
    size_t padded_frame_size = proto.total_bytes;
    if (libusb_get_device_speed(libusb_get_device(craw->handle)) == LIBUSB_SPEED_SUPER) {
        proto.total_bytes = proto.image_bytes;
        padded_frame_size = (proto.total_bytes + 1023) & ~1023;
    }
    if (pipe (craw->notify_pipe) < 0) {
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }

#ifdef HAVE_MACOSX
    capture->socket = CFSocketCreateWithNative (NULL, craw->notify_pipe[0],
                                                kCFSocketReadCallBack, socket_callback_usb, &socket_context);
    /* Set flags so that the underlying fd is not closed with the socket */
    CFSocketSetSocketFlags (capture->socket,
                            CFSocketGetSocketFlags (capture->socket) & ~kCFSocketCloseOnInvalidate);
    capture->socket_source = CFSocketCreateRunLoopSource (NULL,
                                                          capture->socket, 0);
    if (!capture->run_loop)
        dc1394_usb_capture_schedule_with_runloop (craw,
                                              CFRunLoopGetCurrent (), kCFRunLoopCommonModes);
    CFRunLoopAddSource (capture->run_loop, capture->socket_source,
                        capture->run_loop_mode);
#endif

    craw->capture_is_set = 1;

    dc1394_log_debug ("usb: Frame size is %"PRId64, proto.total_bytes);

    craw->num_frames = num_dma_buffers;
    craw->current = -1;
    craw->frames_ready = 0;
    craw->queue_broken = 0;
    craw->buffer_size = padded_frame_size * num_dma_buffers;
    craw->buffer = malloc (craw->buffer_size);
    if (craw->buffer == NULL) {
        dc1394_usb_capture_stop (craw);
        return DC1394_MEMORY_ALLOCATION_FAILURE;
    }

    craw->frames = calloc (num_dma_buffers, sizeof *craw->frames);
    if (craw->frames == NULL) {
        dc1394_usb_capture_stop (craw);
        return DC1394_MEMORY_ALLOCATION_FAILURE;
    }

    for (i = 0; i < num_dma_buffers; i++)
        init_frame(craw, i, &proto, padded_frame_size);

    if (libusb_init(&craw->thread_context) != 0) {
        dc1394_log_error ("usb: Failed to create thread USB context");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }

    uint8_t bus = libusb_get_bus_number (libusb_get_device (craw->handle));
    uint8_t addr = libusb_get_device_address (libusb_get_device (craw->handle));

    libusb_device **list, *dev;
    libusb_get_device_list (craw->thread_context, &list);
    for (i = 0, dev = list[0]; dev; dev = list[++i]) {
        if (libusb_get_bus_number (dev) == bus &&
                libusb_get_device_address (dev) == addr)
            break;
    }
    if (!dev) {
        libusb_free_device_list (list, 1);
        dc1394_log_error ("usb: capture thread failed to find device");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }

    if (libusb_open (dev, &craw->thread_handle) < 0) {
        libusb_free_device_list (list, 1);
        dc1394_log_error ("usb: capture thread failed to open device");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }
    libusb_free_device_list (list, 1);

	// Point Grey uses endpoint 0x81, but other manufacturers or models may use another endpoint
    int usb_interface = find_interface (libusb_get_device(craw->thread_handle), 0x81);
    if (usb_interface == DC1394_FAILURE) {
        dc1394_log_error ("usb: capture thread failed to find suitable interface on device");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }
    if (libusb_claim_interface (craw->thread_handle, usb_interface) < 0) {
        dc1394_log_error ("usb: capture thread failed to claim interface");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }

    for (i = 0; i < craw->num_frames; i++) {
        struct usb_frame *f = craw->frames + i;
        libusb_fill_bulk_transfer (f->transfer, craw->thread_handle,
                0x81, f->frame.image, f->frame.total_bytes,
                callback, f, 0);
    }
    for (i = 0; i < craw->num_frames; i++) {
        if (libusb_submit_transfer (craw->frames[i].transfer) < 0) {
            dc1394_log_error ("usb: Failed to submit initial transfer %d", i);
            dc1394_usb_capture_stop (craw);
            return DC1394_FAILURE;
        }
    }

    if (pthread_mutex_init (&craw->mutex, NULL) < 0) {
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }
    craw->mutex_created = 1;
    if (pthread_create (&craw->thread, NULL, capture_thread, craw) < 0) {
        dc1394_log_error ("usb: Failed to launch helper thread");
        dc1394_usb_capture_stop (craw);
        return DC1394_FAILURE;
    }
    craw->thread_created = 1;

    // if auto iso is requested, start ISO
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        dc1394_video_set_transmission(camera, DC1394_ON);
        craw->iso_auto_started = 1;
    }

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_usb_capture_stop(platform_camera_t *craw)
{
    dc1394camera_t * camera = craw->camera;
    int i;
#ifdef HAVE_MACOSX
    dc1394capture_t * capture = &(craw->capture);
#endif

    if (craw->capture_is_set == 0)
        return DC1394_CAPTURE_IS_NOT_SET;

    dc1394_log_debug ("usb: Capture stopping");

    // stop ISO if it was started automatically
    if (craw->iso_auto_started > 0) {
        dc1394_video_set_transmission(camera, DC1394_OFF);
        craw->iso_auto_started = 0;
    }

    if (craw->thread_created) {
#if 0
        for (i = 0; i < craw->num_frames; i++) {
            libusb_cancel_transfer (craw->frames[i].transfer);
        }
#endif
        pthread_mutex_lock (&craw->mutex);
        craw->kill_thread = 1;
        pthread_mutex_unlock (&craw->mutex);
        pthread_join (craw->thread, NULL);
        dc1394_log_debug ("usb: Joined with helper thread");
        craw->kill_thread = 0;
        craw->thread_created = 0;
    }

    if (craw->mutex_created) {
        pthread_mutex_destroy (&craw->mutex);
        craw->mutex_created = 0;
    }

    if (craw->thread_handle) {
        libusb_release_interface (craw->thread_handle, 0);
        libusb_close (craw->thread_handle);
        craw->thread_handle = NULL;
    }

    if (craw->thread_context) {
        libusb_exit (craw->thread_context);
        craw->thread_context = NULL;
    }

#ifdef HAVE_MACOSX
    if (capture->socket_source) {
        CFRunLoopRemoveSource (capture->run_loop, capture->socket_source,
                               capture->run_loop_mode);
        CFRelease (capture->socket_source);
    }
    capture->socket_source = NULL;

    if (capture->socket) {
        CFSocketInvalidate (capture->socket);
        CFRelease (capture->socket);
    }
    capture->socket = NULL;
#endif

    if (craw->frames) {
        for (i = 0; i < craw->num_frames; i++) {
            libusb_free_transfer (craw->frames[i].transfer);
        }
        free (craw->frames);
        craw->frames = NULL;
    }

    free (craw->buffer);
    craw->buffer = NULL;

    if (craw->notify_pipe[0] != 0 || craw->notify_pipe[1] != 0) {
        close (craw->notify_pipe[0]);
        close (craw->notify_pipe[1]);
    }
    craw->notify_pipe[0] = 0;
    craw->notify_pipe[1] = 0;

    craw->capture_is_set = 0;

    return DC1394_SUCCESS;
}

#define NEXT_BUFFER(c,i) (((i) == -1) ? 0 : ((i)+1)%(c)->num_frames)

dc1394error_t
dc1394_usb_capture_dequeue (platform_camera_t * craw,
        dc1394capture_policy_t policy, dc1394video_frame_t **frame_return)
{
    int next = NEXT_BUFFER (craw, craw->current);
    struct usb_frame * f = craw->frames + next;

    if ((policy < DC1394_CAPTURE_POLICY_MIN)
            || (policy > DC1394_CAPTURE_POLICY_MAX))
        return DC1394_INVALID_CAPTURE_POLICY;

	if(craw->buffer==NULL || craw->capture_is_set==0) {
		*frame_return=NULL;
		return DC1394_CAPTURE_IS_NOT_SET;
	}

    /* default: return NULL in case of failures or lack of frames */
    *frame_return = NULL;

    if (policy == DC1394_CAPTURE_POLICY_POLL) {
        int status;
        pthread_mutex_lock (&craw->mutex);
        status = f->status;
        pthread_mutex_unlock (&craw->mutex);
        if (status == BUFFER_EMPTY)
            return DC1394_SUCCESS;
    }

    if (craw->queue_broken)
        return DC1394_FAILURE;

    char ch;
    if (read (craw->notify_pipe[0], &ch, 1)!=1) {
        dc1394_log_error ("usb: Failed to read from notify pipe");
        return DC1394_FAILURE;
    }

    pthread_mutex_lock (&craw->mutex);
    if (f->status == BUFFER_EMPTY) {
        dc1394_log_error ("usb: Expected filled buffer");
        pthread_mutex_unlock (&craw->mutex);
        return DC1394_FAILURE;
    }
    craw->frames_ready--;
    f->frame.frames_behind = craw->frames_ready;
    pthread_mutex_unlock (&craw->mutex);

    craw->current = next;

    *frame_return = &f->frame;

    if (f->status == BUFFER_ERROR)
        return DC1394_FAILURE;

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_usb_capture_enqueue (platform_camera_t * craw,
        dc1394video_frame_t * frame)
{
    dc1394camera_t * camera = craw->camera;
    struct usb_frame * f = (struct usb_frame *) frame;

    if (frame->camera != camera) {
        dc1394_log_error("usb: Camera does not match frame's camera");
        return DC1394_INVALID_ARGUMENT_VALUE;
    }

    if (f->status == BUFFER_EMPTY) {
        dc1394_log_error ("usb: Frame is not enqueuable");
        return DC1394_FAILURE;
    }

    f->status = BUFFER_EMPTY;
    if (libusb_submit_transfer (f->transfer) != LIBUSB_SUCCESS) {
        craw->queue_broken = 1;
        return DC1394_FAILURE;
    }

    return DC1394_SUCCESS;
}

int
dc1394_usb_capture_get_fileno (platform_camera_t * craw)
{
    if (craw->notify_pipe[0] == 0 && craw->notify_pipe[1] == 0)
        return -1;

    return craw->notify_pipe[0];
}

dc1394bool_t
dc1394_usb_capture_is_frame_corrupt (platform_camera_t * craw,
        dc1394video_frame_t * frame)
{
    struct usb_frame * f = (struct usb_frame *) frame;

    if (f->status == BUFFER_CORRUPT || f->status == BUFFER_ERROR)
        return DC1394_TRUE;

    return DC1394_FALSE;
}


#ifdef HAVE_MACOSX
dc1394error_t
dc1394_usb_capture_schedule_with_runloop (platform_camera_t * craw,
        CFRunLoopRef run_loop, CFStringRef run_loop_mode)
{
    dc1394capture_t * capture = &(craw->capture);
    if (craw->capture_is_set) {
        dc1394_log_warning("schedule_with_runloop must be called before capture_setup");
        return DC1394_FAILURE;
    }

    capture->run_loop = run_loop;
    capture->run_loop_mode = run_loop_mode;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_usb_capture_set_callback (platform_camera_t * craw,
                             dc1394capture_callback_t callback, void * user_data)
{
    dc1394capture_t * capture = &(craw->capture);
    capture->callback = callback;
    capture->callback_user_data = user_data;
    return DC1394_SUCCESS;
}
#endif
