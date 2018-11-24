/*
 * 1394-Based Digital Camera Control Library
 *
 * Camera Capture Code for Linux
 *  
 * Written by
 *   Chris Urmson <curmson@ri.cmu.edu>
 *   Damien Douxchamps <ddouxchamps@users.sf.net>
 *   Dan Dennedy <ddennedy@users.sf.net>
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

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include "config.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <unistd.h>

#include "kernel-video1394.h"
#include "linux.h"
#include "internal.h"

/**********************/
/* Internal functions */
/**********************/


dc1394error_t
open_dma_device(platform_camera_t * craw)
{
    char filename[64];
    struct stat statbuf;

    // if the dma device file has been set manually, use that device name
    if (craw->capture.dma_device_file!=NULL) {
        if ( (craw->capture.dma_fd = open(craw->capture.dma_device_file,O_RDWR)) < 0 ) {
            return DC1394_INVALID_VIDEO1394_DEVICE;
        }
        else {
            return DC1394_SUCCESS;
        }
    }

    // automatic mode: try to open several usual device files.
    sprintf(filename,"/dev/video1394/%d",craw->port);
    if ( stat (filename, &statbuf) == 0 &&
         S_ISCHR (statbuf.st_mode) &&
         (craw->capture.dma_fd = open(filename,O_RDWR)) >= 0 ) {
        return DC1394_SUCCESS;
    }

    sprintf(filename,"/dev/video1394-%d",craw->port);
    if ( stat (filename, &statbuf) == 0 &&
         S_ISCHR (statbuf.st_mode) &&
         (craw->capture.dma_fd = open(filename,O_RDWR)) >= 0 ) {
        return DC1394_SUCCESS;
    }

    if (craw->port==0) {
        sprintf(filename,"/dev/video1394");
        if ( stat (filename, &statbuf) == 0 &&
             S_ISCHR (statbuf.st_mode) &&
             (craw->capture.dma_fd = open(filename,O_RDWR)) >= 0 ) {
            return DC1394_SUCCESS;
        }
    }

    return DC1394_FAILURE;
}

/*****************************************************
 capture_linux_setup

 This sets up the dma for the given camera.
 capture_basic_setup must be called before

******************************************************/
static dc1394error_t
capture_linux_setup(platform_camera_t * craw, uint32_t num_dma_buffers)
{
    struct video1394_mmap vmmap;
    struct video1394_wait vwait;
    uint32_t i;
    dc1394video_frame_t * f;

    memset(&vmmap, 0, sizeof(vmmap));
    memset(&vwait, 0, sizeof(vwait));

    if (open_dma_device(craw) != DC1394_SUCCESS) {
        dc1394_log_warning("Could not open video1394 device file in /dev");
        return DC1394_INVALID_VIDEO1394_DEVICE;
    }

    vmmap.sync_tag= 1;
    vmmap.nb_buffers= num_dma_buffers;
    vmmap.flags= VIDEO1394_SYNC_FRAMES;
    vmmap.buf_size= craw->capture.frames[0].total_bytes; //number of bytes needed
    vmmap.channel= craw->iso_channel;

    /* tell the video1394 system that we want to listen to the given channel */
    if (ioctl(craw->capture.dma_fd, VIDEO1394_IOC_LISTEN_CHANNEL, &vmmap) < 0) {
        dc1394_log_error("VIDEO1394_IOC_LISTEN_CHANNEL ioctl failed: %s", strerror(errno));
        close (craw->capture.dma_fd);
        return DC1394_IOCTL_FAILURE;
    }
    // starting from here we use the ISO channel so we set the flag in the camera struct:
    craw->capture_is_set=1;

    craw->capture.dma_frame_size= vmmap.buf_size;
    craw->capture.num_dma_buffers= vmmap.nb_buffers;
    craw->capture.dma_last_buffer= -1;
    vwait.channel= craw->iso_channel;

    /* QUEUE the buffers */
    for (i= 0; i < vmmap.nb_buffers; i++) {
        vwait.buffer= i;

        if (ioctl(craw->capture.dma_fd,VIDEO1394_IOC_LISTEN_QUEUE_BUFFER,&vwait) < 0) {
            dc1394_log_error("VIDEO1394_IOC_LISTEN_QUEUE_BUFFER ioctl failed");
            ioctl(craw->capture.dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &(vwait.channel));
            craw->capture_is_set=0;
            close (craw->capture.dma_fd);
            return DC1394_IOCTL_FAILURE;
        }

    }

    craw->capture.dma_ring_buffer= mmap(0, vmmap.nb_buffers * vmmap.buf_size,
                                        PROT_READ|PROT_WRITE,MAP_SHARED, craw->capture.dma_fd, 0);

    /* make sure the ring buffer was allocated */
    if (craw->capture.dma_ring_buffer == (uint8_t*)(-1)) {
        dc1394_log_error("mmap failed!");
        ioctl(craw->capture.dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &vmmap.channel);
        craw->capture_is_set=0;
        close (craw->capture.dma_fd);

        // This should be display if the user has low memory
        if (vmmap.nb_buffers * vmmap.buf_size > sysconf (_SC_PAGESIZE) * sysconf (_SC_AVPHYS_PAGES)) {
            dc1394_log_error("Unable to allocate DMA buffer.\nThe requested size (0x%ux or %ud MiB) is bigger than the available memory (0x%lux or %lud MiB).\nPlease free some memory before allocating the buffers",
			     vmmap.nb_buffers * vmmap.buf_size,
			     vmmap.nb_buffers * vmmap.buf_size/1048576,
			     sysconf (_SC_PAGESIZE) * sysconf (_SC_AVPHYS_PAGES),
			     sysconf (_SC_PAGESIZE) * sysconf (_SC_AVPHYS_PAGES)/1048576);
        } else {
            // if it's not low memory, then it's the vmalloc limit.
            // VMALLOC_RESERVED not sufficient (default is 128MiB in recent kernels)
            dc1394_log_error("Unable to allocate DMA buffer. The requested size (0x%x) may be larger than the usual default VMALLOC_RESERVED limit of 128MiB. To verify this, look for the following line in dmesg:\n'allocation failed: out of vmalloc space'\nIf you see this, reboot with the kernel boot parameter:\n'vmalloc=k'\nwhere k (in bytes) is larger than your requested DMA ring buffer size.\nNote that other processes share the vmalloc space so you may need a\nlarge amount of vmalloc memory.", vmmap.nb_buffers * vmmap.buf_size);
            //}
        }
        return DC1394_IOCTL_FAILURE;
    }

    craw->capture.dma_buffer_size= vmmap.buf_size * vmmap.nb_buffers;

    for (i = 0; i < num_dma_buffers; i++) {
        f = craw->capture.frames + i;
        if (i > 0)
            memcpy (f, craw->capture.frames, sizeof (dc1394video_frame_t));
        f->image = (unsigned char *)(craw->capture.dma_ring_buffer +
                                     i * craw->capture.dma_frame_size);
        f->id = i;
    }

    return DC1394_SUCCESS;
}


/* This function allows you to specify the DMA device filename manually. */
dc1394error_t
dc1394_capture_set_device_filename(dc1394camera_t* camera, char *filename)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    platform_camera_t * craw = cpriv->pcam;
    if (craw->capture.dma_device_file==NULL) {
        craw->capture.dma_device_file=(char*)malloc(64*sizeof(char));
        if (craw->capture.dma_device_file==NULL)
            return DC1394_MEMORY_ALLOCATION_FAILURE;
    }
    // note that the device filename is limited to 64 characters.
    memcpy(craw->capture.dma_device_file,filename,64*sizeof(char));

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_linux_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
                     uint32_t flags)
{
    dc1394camera_t * camera = craw->camera;
    dc1394error_t err;

    if (flags & DC1394_CAPTURE_FLAGS_DEFAULT)
        flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
            DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC;

    // if capture is already set, abort
    if (craw->capture_is_set>0)
        return DC1394_CAPTURE_IS_RUNNING;

    craw->capture.flags=flags;
    craw->allocated_channel = -1;

    // if auto iso is requested, stop ISO (if necessary)
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        dc1394switch_t is_iso_on;
        dc1394_video_get_transmission(camera, &is_iso_on);
        if (is_iso_on == DC1394_ON) {
            err=dc1394_video_set_transmission(camera, DC1394_OFF);
            DC1394_ERR_RTN(err,"Could not stop ISO!");
        }
    }

    // allocate channel/bandwidth if requested
    if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) {
        if (dc1394_iso_allocate_channel (camera, 0, &craw->allocated_channel)
            != DC1394_SUCCESS)
            goto fail;
        if (dc1394_video_set_iso_channel (camera, craw->allocated_channel)
            != DC1394_SUCCESS)
            goto fail;
    }
    if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC) {
        unsigned int bandwidth_usage;
        if (dc1394_video_get_bandwidth_usage (camera, &bandwidth_usage)
            != DC1394_SUCCESS)
            goto fail;
        if (dc1394_iso_allocate_bandwidth (camera, bandwidth_usage)
            != DC1394_SUCCESS)
            goto fail;
        craw->allocated_bandwidth = bandwidth_usage;
    }

    craw->capture.frames = malloc (num_dma_buffers * sizeof (dc1394video_frame_t));

    err=capture_basic_setup(camera, craw->capture.frames);
    if (err != DC1394_SUCCESS)
        goto fail;

    if (dc1394_video_get_iso_channel (camera, &craw->iso_channel)
        != DC1394_SUCCESS)
        goto fail;

    // the capture_is_set flag is set inside this function:
    err=capture_linux_setup (craw, num_dma_buffers);
    if (err != DC1394_SUCCESS)
        goto fail;

    // if auto iso is requested, start ISO
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        err=dc1394_video_set_transmission(camera, DC1394_ON);
        DC1394_ERR_RTN(err,"Could not start ISO!");
        craw->iso_auto_started=1;
    }

    return DC1394_SUCCESS;

 fail:
    // free resources if they were allocated
    if (craw->allocated_channel >= 0) {
        if (dc1394_iso_release_channel (camera, craw->allocated_channel)
            != DC1394_SUCCESS)
            dc1394_log_warning("Warning: Could not free ISO channel");
    }
    if (craw->allocated_bandwidth) {
        if (dc1394_iso_release_bandwidth (camera, craw->allocated_bandwidth)
            != DC1394_SUCCESS)
            dc1394_log_warning("Warning: Could not free bandwidth");
    }
    craw->allocated_channel = -1;
    craw->allocated_bandwidth = 0;

    free (craw->capture.frames);
    craw->capture.frames = NULL;
    dc1394_log_error ("Error: Failed to setup DMA capture");

    return DC1394_FAILURE;
}

/*****************************************************
 CAPTURE_STOP
*****************************************************/

dc1394error_t
dc1394_linux_capture_stop(platform_camera_t *craw)
{
    dc1394camera_t * camera = craw->camera;
    int err;

    if (craw->capture_is_set>0) {
        // unlisten
        if (ioctl(craw->capture.dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL,
                    &(craw->iso_channel)) < 0)
            return DC1394_IOCTL_FAILURE;

        // release
        if (craw->capture.dma_ring_buffer) {
            munmap((void*)craw->capture.dma_ring_buffer,craw->capture.dma_buffer_size);
        }


        while (close(craw->capture.dma_fd) != 0) {
            dc1394_log_debug("waiting for dma_fd to close");
            sleep (1);
        }
        craw->capture.dma_fd = -1;

        free (craw->capture.frames);
        craw->capture.frames = NULL;

        // this dma_device file is allocated by the strdup() function and can be freed here without problems.
        free(craw->capture.dma_device_file);
        craw->capture.dma_device_file=NULL;

        // capture is not set anymore
        craw->capture_is_set=0;

        // free ressources if they were allocated
        if (craw->allocated_channel >= 0) {
            if (dc1394_iso_release_channel (camera, craw->allocated_channel)
                    != DC1394_SUCCESS)
                dc1394_log_warning("Warning: Could not free ISO channel");
        }
        if (craw->allocated_bandwidth) {
            if (dc1394_iso_release_bandwidth (camera, craw->allocated_bandwidth)
                    != DC1394_SUCCESS)
                dc1394_log_warning("Warning: Could not free bandwidth");
        }
        craw->allocated_channel = -1;
        craw->allocated_bandwidth = 0;

        // stop ISO if it was started automatically
        if (craw->iso_auto_started>0) {
            err=dc1394_video_set_transmission(camera, DC1394_OFF);
            DC1394_ERR_RTN(err,"Could not stop ISO!");
            craw->iso_auto_started=0;
        }

        // free the additional capture handle
        raw1394_destroy_handle(craw->capture.handle);
    }
    else {
        return DC1394_CAPTURE_IS_NOT_SET;
    }

    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_linux_capture_dequeue (platform_camera_t * craw,
                        dc1394capture_policy_t policy,
                        dc1394video_frame_t **frame)
{
    dc1394capture_t * capture = &(craw->capture);
    struct video1394_wait vwait;
    dc1394video_frame_t * frame_tmp;
    int cb;
    int result=-1;

	if(craw->capture.frames==NULL || craw->capture_is_set==0) {
		*frame=NULL;
		return DC1394_CAPTURE_IS_NOT_SET;
	}

    if ( (policy<DC1394_CAPTURE_POLICY_MIN) || (policy>DC1394_CAPTURE_POLICY_MAX) )
        return DC1394_INVALID_CAPTURE_POLICY;

    // default: return NULL in case of failures or lack of frames
    *frame=NULL;

    memset(&vwait, 0, sizeof(vwait));

    cb = (capture->dma_last_buffer + 1) % capture->num_dma_buffers;
    frame_tmp = capture->frames + cb;

    vwait.channel = craw->iso_channel;
    vwait.buffer = cb;
    switch (policy) {
    case DC1394_CAPTURE_POLICY_POLL:
        result=ioctl(capture->dma_fd, VIDEO1394_IOC_LISTEN_POLL_BUFFER, &vwait);
        break;
    case DC1394_CAPTURE_POLICY_WAIT:
    default:
        while (1) {
            result=ioctl(capture->dma_fd, VIDEO1394_IOC_LISTEN_WAIT_BUFFER,
                    &vwait);
            if (result == 0 || errno != EINTR)
                break;
        }
        break;
    }
    if ( result != 0) {
        if ((policy==DC1394_CAPTURE_POLICY_POLL) && (errno == EINTR)) {
            // when no frames is present, just return
            return DC1394_SUCCESS;
        }
        else {
            dc1394_log_error("VIDEO1394_IOC_LISTEN_WAIT/POLL_BUFFER ioctl failed!");
            return DC1394_IOCTL_FAILURE;
        }
    }

    capture->dma_last_buffer = cb;

    frame_tmp->frames_behind = vwait.buffer;
    frame_tmp->timestamp = (uint64_t) vwait.filltime.tv_sec * 1000000 + vwait.filltime.tv_usec;

    *frame=frame_tmp;

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_linux_capture_enqueue (platform_camera_t * craw,
                        dc1394video_frame_t * frame)
{
    dc1394camera_t * camera = craw->camera;
    struct video1394_wait vwait;

    memset(&vwait, 0, sizeof(vwait));

    if (frame->camera != camera) {
        dc1394_log_error("camera does not match frame's camera");
        return DC1394_INVALID_ARGUMENT_VALUE;
    }

    vwait.channel = craw->iso_channel;
    vwait.buffer = frame->id;

    if (ioctl(craw->capture.dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait) < 0)  {
        dc1394_log_error("VIDEO1394_IOC_LISTEN_QUEUE_BUFFER ioctl failed!");
        return DC1394_IOCTL_FAILURE;
    }

    return DC1394_SUCCESS;
}

int
dc1394_linux_capture_get_fileno (platform_camera_t * craw)
{
    return craw->capture.dma_fd;
}

