/*
 * 1394-Based Digital Camera Control Library
 *
 * Platform specific headers
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

#ifndef __DC1394_PLATFORM_H__
#define __DC1394_PLATFORM_H__

#include <stdint.h>
#include <dc1394/camera.h>

#include "config.h"

#ifdef HAVE_MACOSX
#include <CoreFoundation/CoreFoundation.h>
#endif

typedef struct _platform_t platform_t;
typedef struct _platform_device_t platform_device_t;
typedef struct _platform_camera_t platform_camera_t;

typedef struct _platform_device_list_t {
    platform_t * p;
    platform_device_t ** devices;
    int num_devices;
} platform_device_list_t;

typedef struct _platform_dispatch_t {
    platform_t * (*platform_new)(void);
    void (*platform_free)(platform_t *);

    platform_device_list_t * (*get_device_list)(platform_t *);
    void (*free_device_list)(platform_device_list_t *);
    int (*device_get_config_rom)(platform_device_t *, uint32_t *, int *);

    platform_camera_t * (*camera_new)(platform_t *, platform_device_t *,
            uint32_t);
    void (*camera_free)(platform_camera_t *);
    void (*camera_set_parent)(platform_camera_t *, dc1394camera_t *);

    dc1394error_t (*camera_read)(platform_camera_t *, uint64_t,
            uint32_t *, int);
    dc1394error_t (*camera_write)(platform_camera_t *, uint64_t,
            const uint32_t *, int);

    dc1394error_t (*reset_bus)(platform_camera_t *);
    dc1394error_t (*read_cycle_timer)(platform_camera_t *, uint32_t *,
            uint64_t *);
    dc1394error_t (*camera_get_node)(platform_camera_t *, uint32_t *,
            uint32_t *);
    dc1394error_t (*camera_print_info)(platform_camera_t *, FILE *);
    dc1394error_t (*set_broadcast)(platform_camera_t *, dc1394bool_t);
    dc1394error_t (*get_broadcast)(platform_camera_t *, dc1394bool_t *);

    dc1394error_t (*capture_setup)(platform_camera_t *, uint32_t, uint32_t);
    dc1394error_t (*capture_stop)(platform_camera_t *);

    dc1394error_t (*capture_dequeue)(platform_camera_t *,
            dc1394capture_policy_t, dc1394video_frame_t **);
    dc1394error_t (*capture_enqueue)(platform_camera_t *,
            dc1394video_frame_t *);

    int (*capture_get_fileno)(platform_camera_t *);
    dc1394bool_t (*capture_is_frame_corrupt)(platform_camera_t *,
            dc1394video_frame_t *);

    dc1394error_t (*iso_set_persist)(platform_camera_t *);
    dc1394error_t (*iso_allocate_channel)(platform_camera_t *, uint64_t,
            int *);
    dc1394error_t (*iso_release_channel)(platform_camera_t *, int);
    dc1394error_t (*iso_allocate_bandwidth)(platform_camera_t *, int);
    dc1394error_t (*iso_release_bandwidth)(platform_camera_t *, int);
    dc1394error_t (*capture_set_callback)(platform_camera_t * , dc1394capture_callback_t , void * );

#ifdef HAVE_MACOSX
    dc1394error_t (*capture_schedule_with_runloop)(platform_camera_t * , CFRunLoopRef , CFStringRef);
#else
    dc1394error_t (*capture_schedule_with_runloop)(platform_camera_t *);

#endif

} platform_dispatch_t;


#endif
