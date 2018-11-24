/*
 * 1394-Based Digital Camera Control Library
 *
 * Camera control code for Windows
 *
 * Written by
 *   Satofumi Kamimura <satofumi@users.sourceforge.jp>
 *
 * We use CMU 1394 Digital Camera Driver project's source code for this project.
 *   http://www.cs.cmu.edu/~iwan/1394/
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

#include <windows.h>
#include "dc1394/internal.h"
#include "platform_windows.h"


static dc1394error_t
free_resources(platform_camera_t * cam)
{
    DWORD ret = t1394IsochTearDownStream(cam->device->device_path);
    if (ret) {
        dc1394_log_warning("t1394IsochTearDownStream: Error %ld\n", ret);
        return DC1394_FAILURE;
    }
    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_windows_iso_allocate_channel (platform_camera_t * cam,
                                     uint64_t channels_allowed, int * channel)
{
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_windows_iso_release_channel (platform_camera_t * cam, int channel)
{
    // \todo implement
    return DC1394_FAILURE;
}

static dc1394error_t
windows_capture_setup (platform_camera_t * craw, uint32_t num_dma_buffers, uint32_t flags)
{
    int i;

    free_resources(craw);

    ULARGE_INTEGER dma_buffer_size;
    const char * device_path = craw->device->device_path;
    t1394_GetHostDmaCapabilities (device_path, NULL, &dma_buffer_size);

    const dc1394video_frame_t * frame = &craw->capture.frames[0];
    int max_buffer_size = frame->total_bytes; //number of bytes needed
    int max_bytes = frame->packet_size / 2;

    PACQUISITION_BUFFER acquisition_buffer =
        dc1394BuildAcquisitonBuffer(max_buffer_size,
                                    (unsigned long)dma_buffer_size.QuadPart,
                                    max_bytes, 0);

    ISOCH_STREAM_PARAMS stream_params;
    if (acquisition_buffer) {
        stream_params.nMaxBufferSize = acquisition_buffer->subBuffers[0].ulSize;
        stream_params.nNumberOfBuffers = (num_dma_buffers * acquisition_buffer->nSubBuffers) + 1;

        // free the buffer: this wouldn't be necessary if this were merged into StartImageAcquisitionEx
        dc1394FreeAcquisitionBuffer(acquisition_buffer);
        acquisition_buffer = NULL;
    } else {
        dc1394_log_error("windows_capture_setup: failed to determine required buffer size and count!");
        return DC1394_FAILURE;
    }

    dc1394speed_t speed;
    if (dc1394_video_get_iso_speed(craw->camera, &speed) != DC1394_SUCCESS) {
        dc1394_log_error("windows_capture_setup: failed to get iso speed!");
        return DC1394_FAILURE;
    }

    uint32_t channel = -1;
    stream_params.fulSpeed = 1 << speed;
    stream_params.nChannel = channel;
    stream_params.nMaxBytesPerFrame = max_bytes;

    DWORD ret = t1394IsochSetupStream(craw->device->device_path,
                                      &stream_params);
    craw->allocated_channel = stream_params.nChannel;
    if (ret != ERROR_SUCCESS) {
        dc1394_log_error("windows_capture_setup: Error on IsochSetupStream: %d\n", ret);
        return DC1394_FAILURE;
    }

#if 1
    // allocate channel/bandwidth if requested
    if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) {
        if (dc1394_iso_allocate_channel (craw->camera, 0, &craw->allocated_channel)
            != DC1394_SUCCESS) {
            return DC1394_FAILURE;
        }
        if (dc1394_video_set_iso_channel (craw->camera, craw->allocated_channel)
            != DC1394_SUCCESS) {
            return DC1394_FAILURE;
        }
    }
#endif
#if 1
    if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC) {
        unsigned int bandwidth_usage;
        if (dc1394_video_get_bandwidth_usage (craw->camera, &bandwidth_usage)
            != DC1394_SUCCESS) {
            return DC1394_FAILURE;
        }
        if (dc1394_iso_allocate_bandwidth (craw->camera, bandwidth_usage)
            != DC1394_SUCCESS) {
            return DC1394_FAILURE;
        }
        craw->allocated_bandwidth = bandwidth_usage;
    }
#endif

    /* QUEUE the buffers */
    for (i = 0; i < num_dma_buffers; ++i) {
        PACQUISITION_BUFFER buffer =
            dc1394BuildAcquisitonBuffer(max_buffer_size,
                                        dma_buffer_size.QuadPart, max_bytes, i);
        if(!buffer) {
            dc1394_log_error("windows_capture_setup: Error Allocating AcqBuffer %d\n", i);
            return DC1394_FAILURE;
        }

        // add it to our list of buffers
        if (i == 0) {
            craw->pLastBuffer = craw->pFirstBuffer = buffer;
            craw->pLastBuffer->pNextBuffer = craw->pCurrentBuffer = NULL;
        } else {
            craw->pFirstBuffer->pNextBuffer = buffer;
            craw->pFirstBuffer = buffer;
        }
    }

    // all done making buffers
    // open our long term device handle
    HANDLE device;
    if((device = OpenDevice(device_path, TRUE)) == INVALID_HANDLE_VALUE) {
        dc1394_log_error("windows_capture_setup error opening device (%s)\n", craw->device->device_path);
        return DC1394_FAILURE;
    }

    // all done making buffers
    // open our long term device handle
    PACQUISITION_BUFFER buffer;
    for (buffer = craw->pLastBuffer;
         buffer != NULL; buffer = buffer->pNextBuffer) {
        DWORD ret = dc1394AttachAcquisitionBuffer(device, buffer);
        if (ret != ERROR_SUCCESS) {
            dc1394_log_error("windows_capture_setup: Failed to attach buffer %u/%u", buffer->index, num_dma_buffers);
            return DC1394_FAILURE;
        }
    }

    // new: sleep a little while and verify that the buffers were
    // successfully attached this basically catches "Parameter is Incorrect"
    // here instead of confusing users at AcquireImageEx()
    // 50 ms is all it should take for completion routines to fire and
    // propagate in the kernel
    Sleep(50);

    for (buffer = craw->pLastBuffer;
         buffer != NULL; buffer = buffer->pNextBuffer) {
        DWORD dwBytesRet = 0;
        for (unsigned int bb = 0; bb < buffer->nSubBuffers; ++bb) {
            if (!GetOverlappedResult(device,
                                     &(buffer->subBuffers[bb].overLapped),
                                     &dwBytesRet, FALSE)) {
                if (GetLastError() != ERROR_IO_INCOMPLETE) {
                    dc1394_log_error("Buffer validation failed for buffer %u\n", buffer->index,bb);
                    return DC1394_FAILURE;
                }
                // else: this is the actual success case
            } else {
                dc1394_log_error("Buffer %u is unexpectedly ready during pre-listen validation\n", buffer->index, bb);
                return DC1394_FAILURE;
            }
        }
    }

    ret = t1394IsochListen(craw->device->device_path);
    if (ret != ERROR_SUCCESS) {
        dc1394_log_error("Error %08lx on IOCTL_ISOCH_LISTEN\n", ret);
        return DC1394_FAILURE;
    }

    // starting from here we use the ISO channel so we set the flag
    // in the camera struct:
    craw->capture_is_set = 1;

    for (i = 1; i < num_dma_buffers; ++i) {
        memcpy(&craw->capture.frames[i], frame, sizeof (dc1394video_frame_t));
    }
    craw->capture.frames_last_index = num_dma_buffers - 1;

    craw->device_acquisition = device;
    return DC1394_SUCCESS;
}


dc1394error_t
dc1394_windows_capture_setup(platform_camera_t * craw, uint32_t num_dma_buffers,
                             uint32_t flags)
{
    dc1394camera_t * camera = craw->camera;
    dc1394error_t err;

    if (flags & DC1394_CAPTURE_FLAGS_DEFAULT) {
        flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
            DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC;
    }

    // if capture is already set, abort
    if (craw->capture_is_set > 0) {
        return DC1394_CAPTURE_IS_RUNNING;
    }

    craw->capture.flags = flags;
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

    craw->capture.frames =
        malloc (num_dma_buffers * sizeof (dc1394video_frame_t));
    if (!craw->capture.frames) {
        goto fail;
    }

    err = capture_basic_setup(camera, craw->capture.frames);
    if (err != DC1394_SUCCESS) {
        goto fail;
    }

    err = windows_capture_setup (craw, num_dma_buffers, flags);
    if (err != DC1394_SUCCESS) {
        goto fail;
    }

    // if auto iso is requested, start ISO
    if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
        err=dc1394_video_set_transmission(camera, DC1394_ON);
        DC1394_ERR_RTN(err,"Could not start ISO!");
        craw->iso_auto_started = 1;
    }

    craw->capture.num_dma_buffers = num_dma_buffers;
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

dc1394error_t
dc1394_windows_capture_stop(platform_camera_t *craw)
{
    DWORD dwBytesRet = 0;
    DWORD ret;

    if (craw->device_acquisition == INVALID_HANDLE_VALUE) {
        dc1394_log_error("StopImageAcquisition: Called with invalid device handle\n");
        return DC1394_FAILURE;
    }

    // Tear down the stream
    ret = free_resources(craw);
    if (ret) {
        dc1394_log_error("free_resources: Error %ld\n", ret);
    }

    // put pCurrentBuffer on the list for the sake of cleanup
    if (craw->pCurrentBuffer != NULL) {
        craw->pCurrentBuffer->pNextBuffer = craw->pLastBuffer;
        craw->pLastBuffer = craw->pCurrentBuffer;
    }

    while (craw->pLastBuffer) {
        if (craw->pLastBuffer != craw->pCurrentBuffer) {
            // check the IO status, just in case
            for (unsigned int ii = 0; ii < craw->pLastBuffer->nSubBuffers; ++ii) {
                if (!GetOverlappedResult(craw->device_acquisition, &craw->pLastBuffer->subBuffers[ii].overLapped, &dwBytesRet, TRUE)) {
                    dc1394_log_warning("dc1394_windows_capture_stop: Warning Buffer %d.%d has not been detached, error = %d\n", craw->pLastBuffer->index,ii,GetLastError());
                }
            }
        }

        // check the IO status, just in case
        for(unsigned int ii = 0; ii<craw->pLastBuffer->nSubBuffers; ++ii) {
            // close event: NOTE: must pre-populate correctly above
            if(craw->pLastBuffer->subBuffers[ii].overLapped.hEvent != NULL) {
                CloseHandle(craw->pLastBuffer->subBuffers[ii].overLapped.hEvent);
                craw->pLastBuffer->subBuffers[ii].overLapped.hEvent = NULL;
            }
        }

        // free data buffer
        if (craw->pLastBuffer->pDataBuf)
            GlobalFree(craw->pLastBuffer->pDataBuf);

        // advance to next buffer
        PACQUISITION_BUFFER pAcqBuffer = craw->pLastBuffer;
        craw->pLastBuffer = craw->pLastBuffer->pNextBuffer;

        // free buffer struct
        GlobalFree(pAcqBuffer);
    }

    // clean up our junk
    if (craw->device_acquisition != INVALID_HANDLE_VALUE) {
        CloseHandle(craw->device_acquisition);
        craw->device_acquisition = INVALID_HANDLE_VALUE;
    }

    craw->pFirstBuffer = craw->pLastBuffer = craw->pCurrentBuffer = NULL;
    free (craw->capture.frames);
	craw->capture.frames=NULL;

    craw->capture_is_set = 0;

    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_windows_capture_dequeue (platform_camera_t * craw,
                                dc1394capture_policy_t policy,
                                dc1394video_frame_t **frame)
{
    LPOVERLAPPED pOverlapped = &(craw->pLastBuffer->subBuffers[craw->pLastBuffer->nSubBuffers - 1].overLapped);
    DWORD dwBytesRet = 0;

    BOOL ready = 0;
	
    // default: return NULL frame if no new frames/error/etc.
    *frame=NULL;
    
	if(craw->capture.frames==NULL || craw->capture_is_set==0) {
		*frame=NULL;
		return DC1394_CAPTURE_IS_NOT_SET;
	}

    switch (policy) {
    case DC1394_CAPTURE_POLICY_WAIT:
        ready=GetOverlappedResult(craw->device_acquisition, pOverlapped, &dwBytesRet, TRUE);
        break;
    case DC1394_CAPTURE_POLICY_POLL:
        ready=GetOverlappedResult(craw->device_acquisition, pOverlapped, &dwBytesRet, FALSE);
        break;
    }

    if (ready) {
        craw->pCurrentBuffer = craw->pLastBuffer;

        // advance the buffer queue
        craw->pLastBuffer = craw->pLastBuffer->pNextBuffer;

        // handle the single-buffer case
        // (again, this would be more clear as simply "head" and "tail"
        if (craw->pLastBuffer == NULL) {
            craw->pFirstBuffer = NULL;
        }
    }
    else if (policy==DC1394_CAPTURE_POLICY_POLL)
        return DC1394_SUCCESS;

    if (!craw->pCurrentBuffer) {
        return DC1394_FAILURE;
    }

    // flatten the buffer before returning the internal pointer
    // note: this can only be made more efficient via the "smart" way discussed in CamRGB.cpp: provide
    // an stl-containerlike iterator mechanism that "hides" the flattened-or-not-ness.  Putting that here
    // would, however, break the external API in a way that might require a majorversion bump...
    dc1394FlattenAcquisitionBuffer(craw->pCurrentBuffer);
    unsigned long length = craw->pCurrentBuffer->ulBufferSize;

    dc1394capture_t * capture = &craw->capture;
    *frame = &capture->frames[capture->frames_last_index];
    (*frame)->image = craw->pCurrentBuffer->pFrameStart;
    (*frame)->image_bytes = length;
    capture->frames_last_index =
        (capture->frames_last_index + 1) % capture->num_dma_buffers;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_windows_capture_enqueue (platform_camera_t * craw,
                                dc1394video_frame_t * frame)
{
    if (craw->pCurrentBuffer != NULL) {
        if(dc1394AttachAcquisitionBuffer(craw->device_acquisition, craw->pCurrentBuffer) != ERROR_SUCCESS) {
            dc1394_log_error("AcquireImage: Error Reattaching current buffer!");
            return DC1394_FAILURE;
        }

        // push m_pCurrentBuffer onto the Buffer Queue
        // Note: m_pFirstBuffer is the most recently attached buffer, which is at the end of the queue (confusing names...)
        if(craw->pFirstBuffer == NULL) {
            // there is only one buffer, and we just attached it
            craw->pLastBuffer = craw->pCurrentBuffer;
        } else {
            // current buffer goes onto the end of the queue
            craw->pFirstBuffer->pNextBuffer = craw->pCurrentBuffer;
        }

        // current buffer is now the most recently attached buffer
        craw->pFirstBuffer = craw->pCurrentBuffer;

        // which marks the end of the line
        craw->pFirstBuffer->pNextBuffer = NULL;

        // and means that there is, for now, no "current" buffer
        craw->pCurrentBuffer = NULL;
    }

    return DC1394_SUCCESS;
}
