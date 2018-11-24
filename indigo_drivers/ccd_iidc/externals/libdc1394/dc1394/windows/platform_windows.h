/*
 * 1394-Based Digital Camera Control Library
 *
 * Platform specific headers
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

#ifndef __DC1394_PLATFORM_WINDOWS_H__
#define __DC1394_PLATFORM_WINDOWS_H__

#include <1394camapi.h>


enum {
    MAX_DEVICE_SIZE = 63,
    DEVICE_PATH_SIZE = 512,
    CONFIG_ROM_SIZE = 256,
};

typedef struct
{
    uint32_t flags;
    dc1394video_frame_t * frames;
    int num_dma_buffers;
    int frames_last_index;
} dc1394capture_t;

struct _platform_camera_t {
    platform_device_t * device;
    dc1394camera_t * camera;
    dc1394capture_t capture;
    int capture_is_set;
    int allocated_channel;
    unsigned int allocated_bandwidth;
    unsigned int iso_channel;
    int iso_auto_started;

    HANDLE device_acquisition;
    PACQUISITION_BUFFER pFirstBuffer;
    PACQUISITION_BUFFER pLastBuffer;
    PACQUISITION_BUFFER pCurrentBuffer;
};

struct _platform_device_t {
    int node;
    char device_path[DEVICE_PATH_SIZE];
    ULONG config_rom[CONFIG_ROM_SIZE];
    ULONG config_rom_size;
};

struct _platform_t {
    HDEVINFO * dev_info;
};

#endif
