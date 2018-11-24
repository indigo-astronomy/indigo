/*
 * 1394-Based Digital Camera Control Library
 *
 * Mac OS X Digital Camera Capture Code
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/firewire/IOFireWireLib.h>
#include <IOKit/firewire/IOFireWireLibIsoch.h>
#include <CoreServices/CoreServices.h>

#include "config.h"
#include "internal.h"
#include "macosx/macosx.h"


/**********************/
/* Internal functions */
/**********************/

static IOReturn
supported_channels (IOFireWireLibIsochPortRef rem_port, IOFWSpeed * maxSpeed, UInt64 * chanSupported)
{
    platform_camera_t * craw = (*rem_port)->GetRefCon (rem_port);
    dc1394camera_t * camera = craw->camera;
    dc1394capture_t * capture = &(craw->capture);
    dc1394speed_t iso_speed;
    uint32_t channel;

    if (dc1394_video_get_iso_speed (camera, &iso_speed) == DC1394_SUCCESS) {
        switch (iso_speed) {
        case DC1394_ISO_SPEED_100:
            *maxSpeed = kFWSpeed100MBit;
            break;
        case DC1394_ISO_SPEED_200:
            *maxSpeed = kFWSpeed200MBit;
            break;
        case DC1394_ISO_SPEED_400:
            *maxSpeed = kFWSpeed400MBit;
            break;
        default:
            *maxSpeed = kFWSpeed800MBit;
            break;
        }
    }
    else {
        dc1394_log_warning("could not get ISO speed, using 400 Mb");
        *maxSpeed = kFWSpeed400MBit;
    }

    /* Only the first 16 channels are allowed */
    *chanSupported = 0xFFFFULL << 48;

    /* If automatic IRM allocation is turned off, we only allow the channel
     * that has been already set in the camera. */
    if (!capture->do_irm &&
        dc1394_video_get_iso_channel (camera, &channel) == DC1394_SUCCESS) {
        *chanSupported = 0x1ULL << (63-channel);
    }
    return kIOReturnSuccess;
}

static IOReturn
allocate_port (IOFireWireLibIsochPortRef rem_port, IOFWSpeed speed, UInt32 chan)
{
    platform_camera_t * craw = (*rem_port)->GetRefCon (rem_port);
    dc1394camera_t * camera = craw->camera;
    craw->iso_channel_is_set = 1;
    craw->iso_channel = chan;
    dc1394_video_set_iso_channel(camera, craw->iso_channel);
    return kIOReturnSuccess;
}

static IOReturn
finalize_callback (dc1394capture_t * capture)
{
    CFRunLoopStop (CFRunLoopGetCurrent ());
    return kIOReturnSuccess;
}

#ifndef MIN
    #define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static void
callback (buffer_info * buffer, NuDCLRef dcl)
{
    platform_camera_t * craw;
    dc1394capture_t * capture;
    UInt32 bus, dma_time, sec, cycle, bus_time;
    int usec;
    int i;
    int corrupt = 0;

    if (!buffer) {
        dc1394_log_error("callback buffer is null");
        return;
    }

    craw = buffer->craw;
    capture = &(craw->capture);

    if (buffer->status != BUFFER_EMPTY)
        dc1394_log_error("buffer %d should have been empty",buffer->i);

    /*
     * We do this Notify() here in order to run the "update" step on the
     * NuDCL program.  This is done instead of AppendDCLUpdateList() because
     * that function does not work properly on Mac OS 10.4, as explained
     * here:
     *   http://lists.apple.com/archives/Firewire/2006/Aug/msg00002.html
     *
     * Also, the Notify() can only handle 30 DCLs at a time.
     */
    for (i = 0; i < buffer->num_dcls; i += 30) {
        (*capture->loc_port)->Notify (capture->loc_port,
                                      kFWNuDCLUpdateNotification,
                                      (void **) buffer->dcl_list + i,
                                      MIN (buffer->num_dcls - i, 30));
    }

    for (i = 0; i < buffer->num_dcls; i++) {
        int packet_size = capture->frames[buffer->i].packet_size;
        if ((buffer->pkts[i].status & 0x1F) != 0x11) {
            dc1394_log_warning ("packet %d had error status %x",
                    i, buffer->pkts[i].status);
            corrupt = 1;
        }
        if ((buffer->pkts[i].header & 0x3) != 0 && i > 0) {
            dc1394_log_warning ("packet %d had unexpected sync (%x)",
                    i, buffer->pkts[i].header);
            corrupt = 1;
        }
        if ((buffer->pkts[i].header >> 16) != packet_size) {
            dc1394_log_warning ("packet %d had wrong length (%x)",
                    i, buffer->pkts[i].header);
            corrupt = 1;
        }
    }

    (*craw->iface)->GetBusCycleTime (craw->iface, &bus, &bus_time);
    gettimeofday (&buffer->filltime, NULL);

    /* Get the bus timestamp of when the packet was received */
    dma_time = buffer->pkts[0].timestamp;
    sec = (dma_time & 0xe000000) >> 25;
    cycle = (dma_time & 0x1fff000) >> 12;

    /* convert to microseconds */
    dma_time = sec * 1000000 + cycle * 125;

    /* Get the bus timestamp right now */
    sec = (bus_time & 0xe000000) >> 25;
    cycle = (bus_time & 0x1fff000) >> 12;

    /* convert to microseconds */
    bus_time = sec * 1000000 + cycle * 125 + (bus_time & 0xfff) * 125 / 3072;

    /* Compute how many usec ago the packet was received by comparing
     * the current bus time to the timestamp of the first ISO packet */
    usec = (bus_time + 8000000 - dma_time) % 8000000;

    /* Subtract usec from the current clock time */
    usec = buffer->filltime.tv_usec - usec;
    while (usec < 0) {
        buffer->filltime.tv_sec--;
        usec += 1000000;
    }
    buffer->filltime.tv_usec = usec;

    MPEnterCriticalRegion (capture->mutex, kDurationForever);
    buffer->status = corrupt ? BUFFER_CORRUPT : BUFFER_FILLED;
    capture->frames_ready++;
    MPExitCriticalRegion (capture->mutex);

    write (capture->notify_pipe[1], "+", 1);
}

static void
socket_callback_firewire (CFSocketRef s, CFSocketCallBackType type,
                 CFDataRef address, const void * data, void * info)
{
    platform_camera_t * craw = info;
    dc1394capture_t * capture = &(craw->capture);
    if (capture->callback) {
        capture->callback (craw->camera, capture->callback_user_data);
    }
}

DCLCommand *
CreateDCLProgram (platform_camera_t * craw)
{
    dc1394capture_t * capture = &(craw->capture);
    IOVirtualRange * databuf = &(capture->databuf);
    NuDCLRef dcl = NULL;
    IOFireWireLibNuDCLPoolRef dcl_pool = capture->dcl_pool;
    int packet_size = capture->frames[0].packet_size;
    int ppf = capture->frames[0].packets_per_frame;
    //int bytesperframe = capture->frames[0].total_bytes;
    int i;

    databuf->length = (capture->num_frames + 1) * packet_size * ppf +
        capture->num_frames * ppf * sizeof (packet_info);
    databuf->address = (IOVirtualAddress) mmap (NULL, databuf->length,
            PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    if (!databuf->address || databuf->address == (IOVirtualAddress)-1) {
        dc1394_log_error("mmap failed");
        return NULL;
    }

    for (i = 0; i < capture->num_frames; i++) {
        IOVirtualAddress frame_address = databuf->address +
            i * packet_size * ppf;
        IOVirtualAddress data_address = databuf->address +
            (capture->num_frames + 1) * packet_size * ppf +
            i * ppf * sizeof (packet_info);
        buffer_info * buffer = capture->buffers + i;
        dc1394video_frame_t * frame = capture->frames + i;
        int j;
        IOVirtualRange ranges[2] = {
            {data_address, 4},
            {frame_address, packet_size},
        };

        if (i > 0)
            memcpy (frame, capture->frames, sizeof (dc1394video_frame_t));
        frame->image = (unsigned char *) frame_address;
        frame->id = i;

        buffer->craw = craw;
        buffer->i = i;
        buffer->status = BUFFER_EMPTY;
        buffer->num_dcls = ppf;
        buffer->dcl_list = malloc (ppf * sizeof (NuDCLRef));
        buffer->pkts = (packet_info *) data_address;

        dcl = (*dcl_pool)->AllocateReceivePacket (dcl_pool, NULL,
                                                  4, 2, ranges);
        (*dcl_pool)->SetDCLWaitControl (dcl, true);
        (*dcl_pool)->SetDCLFlags (dcl, kNuDCLDynamic);
        (*dcl_pool)->SetDCLStatusPtr (dcl, &buffer->pkts[0].status);
        (*dcl_pool)->SetDCLTimeStampPtr (dcl, &buffer->pkts[0].timestamp);

        buffer->dcl_list[0] = dcl;

        for (j = 1; j < ppf; j++) {
            ranges[0].address = (IOVirtualAddress) &buffer->pkts[j].header;
            ranges[1].address += packet_size;
            dcl = (*dcl_pool)->AllocateReceivePacket (dcl_pool, NULL,
                                                      4, 2, ranges);
            (*dcl_pool)->SetDCLFlags (dcl, kNuDCLDynamic);
            (*dcl_pool)->SetDCLStatusPtr (dcl, &buffer->pkts[j].status);
            (*dcl_pool)->SetDCLTimeStampPtr (dcl, &buffer->pkts[j].timestamp);
            buffer->dcl_list[j] = dcl;
        }

#if 0
        for (j = 0; j < ppf; j++)
            (*dcl_pool)->AppendDCLUpdateList (dcl, buffer->dcl_list[j]);
#endif
        (*dcl_pool)->SetDCLRefcon (dcl, capture->buffers + i);
        (*dcl_pool)->SetDCLCallback (dcl, (NuDCLCallback) callback);
    }
    (*dcl_pool)->SetDCLBranch (dcl, capture->buffers[0].dcl_list[0]);

    dcl = capture->buffers[capture->num_frames-1].dcl_list[0];
    (*dcl_pool)->SetDCLBranch (dcl, dcl);

    //(*dcl_pool)->PrintProgram (dcl_pool);
    return (*dcl_pool)->GetProgram (dcl_pool);
}

OSStatus
servicing_thread (void * cam_ptr)
{
    platform_camera_t * craw = cam_ptr;
    IOFireWireLibDeviceRef d = craw->iface;

    (*d)->AddCallbackDispatcherToRunLoopForMode (d, CFRunLoopGetCurrent (),
                                                 kCFRunLoopCommonModes);
    (*d)->AddIsochCallbackDispatcherToRunLoopForMode (d, CFRunLoopGetCurrent (),
                                                      kCFRunLoopCommonModes);

    MPSignalSemaphore(craw->capture.thread_init_semaphore);

    CFRunLoopRun ();

    return 0;
}

/*************************************************************
 CAPTURE SETUP
**************************************************************/
dc1394error_t
dc1394_macosx_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
                     uint32_t flags)
{
    dc1394capture_t * capture = &(craw->capture);
    dc1394camera_t * camera = craw->camera;
    dc1394error_t err;
    IOReturn ret;
    IOFireWireLibDeviceRef d = craw->iface;
    IOFWSpeed speed;
    IOFireWireLibIsochChannelRef chan;
    IOFireWireLibRemoteIsochPortRef rem_port;
    IOFireWireLibLocalIsochPortRef loc_port;
    IOFireWireLibNuDCLPoolRef dcl_pool;
    DCLCommand * dcl_program;
    int frame_size;
    int numdcls;
    CFSocketContext socket_context = { 0, craw, NULL, NULL, NULL };

    // if capture is already set, abort
    if (craw->capture_is_set>0)
        return DC1394_CAPTURE_IS_RUNNING;

    craw->capture.flags=flags;
    if (((flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) &&
         (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC)) ||
        (flags & DC1394_CAPTURE_FLAGS_DEFAULT))
        craw->capture.do_irm = true;
    else if (!(flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) &&
             !(flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC))
        craw->capture.do_irm = false;
    else {
        err = DC1394_FAILURE;
        DC1394_ERR_RTN (err, "Bandwidth and channel allocation must be enabled/disabled together in MacOSX");
    }

    // if auto iso is requested, stop ISO (if necessary)
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        dc1394switch_t is_iso_on;
        dc1394_video_get_transmission(camera, &is_iso_on);
        if (is_iso_on == DC1394_ON) {
            err=dc1394_video_set_transmission(camera, DC1394_OFF);
            DC1394_ERR_RTN(err,"Could not stop ISO!");
        }
    }

    capture->frames = malloc (num_dma_buffers * sizeof (dc1394video_frame_t));
    err = capture_basic_setup(camera, capture->frames);
    if (err != DC1394_SUCCESS)
        dc1394_capture_stop (camera);
    DC1394_ERR_RTN (err,"Could not setup capture");

    capture->num_frames = num_dma_buffers;
    pipe (capture->notify_pipe);

    capture->socket = CFSocketCreateWithNative (NULL, capture->notify_pipe[0],
                                                kCFSocketReadCallBack, socket_callback_firewire, &socket_context);
    /* Set flags so that the underlying fd is not closed with the socket */
    CFSocketSetSocketFlags (capture->socket,
                            CFSocketGetSocketFlags (capture->socket) & ~kCFSocketCloseOnInvalidate);
    capture->socket_source = CFSocketCreateRunLoopSource (NULL,
                                                          capture->socket, 0);
    if (!capture->run_loop)
        dc1394_macosx_capture_schedule_with_runloop (craw,
                                              CFRunLoopGetCurrent (), kCFRunLoopCommonModes);
    CFRunLoopAddSource (capture->run_loop, capture->socket_source,
                        capture->run_loop_mode);

    MPCreateCriticalRegion (&capture->mutex);

    MPCreateQueue (&capture->termination_queue);
    MPCreateSemaphore (1, 0, &capture->thread_init_semaphore);
    MPCreateTask (&servicing_thread, craw, 0, capture->termination_queue,
                  NULL, NULL, 0, &capture->task);

    /* wait for thread to start */
    MPWaitOnSemaphore (capture->thread_init_semaphore, kDurationForever);
    MPDeleteSemaphore (capture->thread_init_semaphore);
    capture->thread_init_semaphore = NULL;

    (*d)->TurnOnNotification (d);

    (*d)->GetSpeedToNode (d, craw->generation, &speed);
    chan = (*d)->CreateIsochChannel (d, craw->capture.do_irm,
                                     capture->frames[0].packet_size, speed,
                                     CFUUIDGetUUIDBytes (kIOFireWireIsochChannelInterfaceID));
    if (!chan) {
        dc1394_macosx_capture_stop (craw);
        dc1394_log_error("Could not create IsochChannelInterface");
        return DC1394_FAILURE;
    }
    capture->chan = chan;

    rem_port = (*d)->CreateRemoteIsochPort (d, true,
                                            CFUUIDGetUUIDBytes (kIOFireWireRemoteIsochPortInterfaceID));
    if (!rem_port) {
        dc1394_macosx_capture_stop (craw);
        dc1394_log_error("Could not create RemoteIsochPortInterface");
        return DC1394_FAILURE;
    }
    capture->rem_port = rem_port;
    (*rem_port)->SetAllocatePortHandler (rem_port, &allocate_port);
    (*rem_port)->SetGetSupportedHandler (rem_port, &supported_channels);
    (*rem_port)->SetRefCon ((IOFireWireLibIsochPortRef)rem_port, craw);

    capture->buffers = malloc (capture->num_frames * sizeof (buffer_info));
    capture->last_dequeued = capture->num_frames - 1;
    capture->last_enqueued = capture->num_frames - 1;
    frame_size = capture->frames[0].total_bytes;
    //capture->frame_pages = ((frame_size - 1) / getpagesize()) + 1;

    numdcls = capture->frames[0].packets_per_frame * capture->num_frames;

    dcl_pool = (*d)->CreateNuDCLPool (d, numdcls,
                                      CFUUIDGetUUIDBytes (kIOFireWireNuDCLPoolInterfaceID));
    if (!dcl_pool) {
        dc1394_macosx_capture_stop (craw);
        dc1394_log_error("Could not create NuDCLPoolInterface");
        return DC1394_FAILURE;
    }
    capture->dcl_pool = dcl_pool;

    dcl_program = CreateDCLProgram (craw);
    if (!dcl_program) {
        dc1394_macosx_capture_stop (craw);
        dc1394_log_error("Could not create DCL Program");
        return DC1394_FAILURE;
    }

    loc_port = (*d)->CreateLocalIsochPort (d, false, dcl_program,
                                           kFWDCLSyBitsEvent, 1, 1, nil, 0, &(capture->databuf), 1,
                                           CFUUIDGetUUIDBytes (kIOFireWireLocalIsochPortInterfaceID));
    if (!loc_port) {
        dc1394_macosx_capture_stop (craw);
        dc1394_log_error("Could not create LocalIsochPortInterface");
        return DC1394_FAILURE;
    }
    capture->loc_port = loc_port;

    (*loc_port)->SetRefCon ((IOFireWireLibIsochPortRef) loc_port, capture);
    (*loc_port)->SetFinalizeCallback (loc_port,
                                      (IOFireWireLibIsochPortFinalizeCallback) finalize_callback);

    (*chan)->AddListener (chan, (IOFireWireLibIsochPortRef) loc_port);
    (*chan)->SetTalker (chan, (IOFireWireLibIsochPortRef) rem_port);

    if ((ret = (*chan)->AllocateChannel (chan)) != kIOReturnSuccess) {
        dc1394_macosx_capture_stop (craw);
        if (ret == kIOReturnNoSpace)
            dc1394_log_error("Not enough iso bandwidth or channels are "
                    "available to begin capture");
        else
            dc1394_log_error("Could not allocate channel");
        return DC1394_FAILURE;
    }
    capture->iso_is_allocated = 1;

    if ((*chan)->Start (chan) != kIOReturnSuccess) {
        dc1394_macosx_capture_stop (craw);
        dc1394_log_error("Could not start channel");
        return DC1394_FAILURE;
    }
    capture->iso_is_started = 1;

    craw->capture_is_set=1;

    // if auto iso is requested, start ISO
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        err=dc1394_video_set_transmission(camera, DC1394_ON);
        DC1394_ERR_RTN(err,"Could not start ISO!");
        craw->iso_auto_started=1;
    }

    return DC1394_SUCCESS;
}


/*****************************************************
 CAPTURE_STOP
*****************************************************/

dc1394error_t
dc1394_macosx_capture_stop(platform_camera_t *craw)
{
    dc1394camera_t * camera = craw->camera;
    dc1394capture_t * capture = &(craw->capture);
    IOVirtualRange * databuf = &(capture->databuf);

    if (capture->iso_is_started) {
        IOReturn res;
        res = (*capture->chan)->Stop (capture->chan);

        /* Wait for thread termination */
        MPWaitOnQueue (capture->termination_queue, NULL, NULL, NULL,
                       kDurationForever);
    }
    else if (capture->task) {
        dc1394_log_warning("Forcefully killing servicing task...");
        MPTerminateTask (capture->task, 0);
        MPWaitOnQueue (capture->termination_queue, NULL, NULL, NULL,
                       kDurationForever);
    }
    capture->task = NULL;
    capture->iso_is_started = 0;

    if (capture->iso_is_allocated)
        (*capture->chan)->ReleaseChannel (capture->chan);
    capture->iso_is_allocated = 0;

    if (capture->chan)
        (*capture->chan)->Release (capture->chan);
    if (capture->loc_port)
        (*capture->loc_port)->Release (capture->loc_port);
    if (capture->rem_port)
        (*capture->rem_port)->Release (capture->rem_port);
    if (capture->dcl_pool)
        (*capture->dcl_pool)->Release (capture->dcl_pool);
    capture->chan = NULL;
    capture->loc_port = NULL;
    capture->rem_port = NULL;
    capture->dcl_pool = NULL;

    if (databuf->address)
        munmap ((void *) databuf->address, databuf->length);
    databuf->address = 0;

    if (capture->buffers) {
        int i;
        for (i = 0; i < capture->num_frames; i++)
            free (capture->buffers[i].dcl_list);
        free (capture->buffers);
    }
    capture->buffers = NULL;

    if (capture->termination_queue)
        MPDeleteQueue (capture->termination_queue);
    capture->termination_queue = NULL;

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

    if (capture->mutex)
        MPDeleteCriticalRegion (capture->mutex);
    capture->mutex = NULL;

    if (capture->notify_pipe[0] != 0 || capture->notify_pipe[1] != 0) {
        close (capture->notify_pipe[0]);
        close (capture->notify_pipe[1]);
    }
    capture->notify_pipe[0] = capture->notify_pipe[1] = 0;

    if (capture->frames)
        free (capture->frames);
    capture->frames = NULL;

    craw->capture_is_set=0;

    // stop ISO if it was started automatically
    if (craw->iso_auto_started>0) {
        dc1394error_t err=dc1394_video_set_transmission(camera, DC1394_OFF);
        DC1394_ERR_RTN(err,"Could not stop ISO!");
        craw->iso_auto_started=0;
    }

    return DC1394_SUCCESS;
}

/****************************************************
 CAPTURE
*****************************************************/
#define NEXT_BUFFER(c,i) (((i) == -1) ? 0 : ((i)+1)%(c)->num_frames)
//#define PREV_BUFFER(c,i) (((i) == 0) ? (c)->num_frames-1 : ((i)-1))

dc1394error_t
dc1394_macosx_capture_dequeue (platform_camera_t * craw,
                        dc1394capture_policy_t policy,
                        dc1394video_frame_t **frame)
{
    dc1394capture_t * capture = &(craw->capture);
    int next = NEXT_BUFFER (capture, capture->last_dequeued);
    buffer_info * buffer = capture->buffers + next;
    dc1394video_frame_t * frame_tmp = capture->frames + next;
    char ch;

	if(craw->capture.frames==NULL || craw->capture_is_set==0) {
		*frame=NULL;
		return DC1394_CAPTURE_IS_NOT_SET;
	}

    if ( (policy<DC1394_CAPTURE_POLICY_MIN) || (policy>DC1394_CAPTURE_POLICY_MAX) )
        return DC1394_INVALID_CAPTURE_POLICY;

    // default: return NULL in case of failures or lack of frames
    *frame=NULL;

    if (policy == DC1394_CAPTURE_POLICY_POLL) {
        int status;
        MPEnterCriticalRegion (capture->mutex, kDurationForever);
        status = buffer->status;
        MPExitCriticalRegion (capture->mutex);
        if (status != BUFFER_FILLED && status != BUFFER_CORRUPT)
            return DC1394_SUCCESS;
    }

    read (capture->notify_pipe[0], &ch, 1);

    MPEnterCriticalRegion (capture->mutex, kDurationForever);
    if (buffer->status != BUFFER_FILLED && buffer->status != BUFFER_CORRUPT) {
        dc1394_log_error("expected filled buffer");
        MPExitCriticalRegion (capture->mutex);
        return DC1394_FAILURE;
    }
    capture->frames_ready--;
    frame_tmp->frames_behind = capture->frames_ready;
    MPExitCriticalRegion (capture->mutex);

    capture->last_dequeued = next;

    frame_tmp->timestamp = (uint64_t) buffer->filltime.tv_sec * 1000000 +
        buffer->filltime.tv_usec;

    *frame=frame_tmp;

    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_macosx_capture_enqueue (platform_camera_t * craw,
                        dc1394video_frame_t * frame)
{
    dc1394capture_t * capture = &(craw->capture);
    dc1394camera_t * camera = craw->camera;
    int last = capture->last_enqueued;
    buffer_info * prev_buffer = capture->buffers + last;
    buffer_info * buffer = capture->buffers + frame->id;
    IOFireWireLibNuDCLPoolRef dcl_pool = capture->dcl_pool;
    IOFireWireLibLocalIsochPortRef loc_port = capture->loc_port;
    void * dcl_list[2];

    if (frame->camera != camera) {
        dc1394_log_error("camera does not match frame's camera");
        return DC1394_INVALID_ARGUMENT_VALUE;
    }

    if (buffer->status != BUFFER_FILLED && buffer->status != BUFFER_CORRUPT)
        return DC1394_FAILURE;

    buffer->status = BUFFER_ENQUEUED;

    buffer = NULL;
    while (1) {
        last = NEXT_BUFFER (capture, last);
        if (capture->buffers[last].status != BUFFER_ENQUEUED)
            break;
        capture->last_enqueued = last;
        buffer = capture->buffers + last;
        buffer->status = BUFFER_EMPTY;
    }

    if (buffer) {
        (*dcl_pool)->SetDCLBranch (buffer->dcl_list[0], buffer->dcl_list[0]);
        (*dcl_pool)->SetDCLBranch (prev_buffer->dcl_list[0], prev_buffer->dcl_list[1]);
        dcl_list[0] = prev_buffer->dcl_list[0];
        dcl_list[1] = buffer->dcl_list[0];
        (*loc_port)->Notify (loc_port, kFWNuDCLModifyJumpNotification, dcl_list+1, 1);
        (*loc_port)->Notify (loc_port, kFWNuDCLModifyJumpNotification, dcl_list, 1);
    }
    return DC1394_SUCCESS;
}

dc1394bool_t
dc1394_macosx_capture_is_frame_corrupt (platform_camera_t * craw,
        dc1394video_frame_t * frame)
{
    dc1394capture_t * capture = &(craw->capture);
    buffer_info * buffer = capture->buffers + frame->id;

    if (buffer->status == BUFFER_CORRUPT)
        return DC1394_TRUE;

    return DC1394_FALSE;
}

int
dc1394_macosx_capture_get_fileno (platform_camera_t * craw)
{
    dc1394capture_t * capture = &(craw->capture);

    if (capture->notify_pipe[0] == 0 && capture->notify_pipe[1] == 0)
        return -1;

    return capture->notify_pipe[0];
}

dc1394error_t
dc1394_macosx_capture_schedule_with_runloop (platform_camera_t * craw,
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
dc1394_macosx_capture_set_callback (platform_camera_t * craw,
                             dc1394capture_callback_t callback, void * user_data)
{
    dc1394capture_t * capture = &(craw->capture);
    capture->callback = callback;
    capture->callback_user_data = user_data;
    return DC1394_SUCCESS;
}

