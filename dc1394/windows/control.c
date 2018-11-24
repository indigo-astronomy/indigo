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
#include <stdlib.h>
#include "dc1394/internal.h"
#include "platform_windows.h"
#include "types.h"


// \todo remove this function
extern dc1394error_t
dc1394_windows_iso_allocate_channel (platform_camera_t * cam,
                                     uint64_t channels_allowed, int * channel);

extern dc1394error_t
dc1394_windows_iso_release_channel (platform_camera_t * cam, int channel);

extern dc1394error_t
dc1394_windows_capture_setup(platform_camera_t * craw, uint32_t num_dma_buffers,
                             uint32_t flags);

extern dc1394error_t
dc1394_windows_capture_stop(platform_camera_t *craw);

extern dc1394error_t
dc1394_windows_capture_dequeue (platform_camera_t * craw,
                                dc1394capture_policy_t policy,
                                dc1394video_frame_t **frame);

extern dc1394error_t
dc1394_windows_capture_enqueue (platform_camera_t * craw,
                                dc1394video_frame_t * frame);


static platform_t *
dc1394_windows_new (void)
{
    HDEVINFO * dev_info = t1394CmdrGetDeviceList();
    if (dev_info == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    platform_t * p = calloc (1, sizeof (platform_t));
    if (!p) {
        return NULL;
    }

    p->dev_info = dev_info;
    return p;
}

static void
dc1394_windows_free (platform_t * p)
{
    if (p == NULL) {
        return;
    }

    SetupDiDestroyDeviceInfoList(p->dev_info);
    free (p);
}

static int
read_retry (const char * device_path, ULONG offset, PULONG data)
{
    int retry = DC1394_MAX_RETRIES;
    while (retry > 0) {
        DWORD ret = ReadRegisterUL ((char *)device_path, offset, data);
        if (ret == ERROR_SUCCESS) {
            return 0;
        }

        if (ret != ERROR_SEM_TIMEOUT && ret != ERROR_BUSY) {
            break;
        }
        usleep (DC1394_SLOW_DOWN);
        retry--;
    }
    return -1;
}

static int
write_retry (const char * device_path, ULONG offset, ULONG data)
{
    int retry = DC1394_MAX_RETRIES;
    while (retry > 0) {
        DWORD ret = WriteRegisterUL ((char *)device_path, offset, data);
        if (ret == ERROR_SUCCESS) {
            return 0;
        }

        if (ret != ERROR_SEM_TIMEOUT && ret != ERROR_BUSY) {
            break;
        }
        usleep (DC1394_SLOW_DOWN);
        retry--;
    }
    return -1;
}

static int number_of_devices(platform_t * p)
{
    int n;

    for (n = 0; n < MAX_DEVICE_SIZE; ++n) {
        char device_path[DEVICE_PATH_SIZE];
        ULONG device_path_size = DEVICE_PATH_SIZE;

        if (t1394CmdrGetDevicePath (p->dev_info, n,
                                    device_path, &device_path_size) <= 0) {
            return n;
        }
    }
    return 0;
}

static platform_device_list_t *
dc1394_windows_get_device_list (platform_t * p)
{
    platform_device_list_t * list;
    int device_size;
    int i;

    list = calloc (1, sizeof (platform_device_list_t));
    if (!list) {
        return NULL;
    }

    device_size = number_of_devices(p);
    list->devices = malloc (device_size * sizeof(platform_device_t *));
    if (!list->devices) {
        free (list);
        return NULL;
    }
    list->num_devices = 0;

    // find device path
    for (i = 0; i < device_size; ++i) {
        char device_path[DEVICE_PATH_SIZE];
        ULONG device_path_size = DEVICE_PATH_SIZE;
        ULONG quad;
        platform_device_t * device;
        int j;

        if (t1394CmdrGetDevicePath (p->dev_info, i,
                                    device_path, &device_path_size) <= 0) {
            break;
        }

        if (read_retry (device_path, 0xf0000400, &quad) < 0) {
            break;
        }

        device = malloc (sizeof (platform_device_t));
        if (!device) {
            break;
        }

        strncpy(device->device_path, device_path, device_path_size);
        device->node = i;
        device->config_rom[0] = quad;
        for (j = 1; j < CONFIG_ROM_SIZE; ++j) {
            if (read_retry (device_path, 0xf0000400 + 4*j, &quad) < 0) {
                break;
            }
            device->config_rom[j] = quad;
        }
        device->config_rom_size = j;

        list->devices[list->num_devices] = device;
        ++list->num_devices;
    }
    return list;
}

static void
dc1394_windows_free_device_list (platform_device_list_t * d)
{
    int i;

    if (!d) {
        return;
    }

    for (i = 0; i < d->num_devices; ++i) {
        free (d->devices[i]);
    }
    free (d);
}

static int
dc1394_windows_device_get_config_rom (platform_device_t * device,
                                      uint32_t * quads, int * num_quads)
{
    if (*num_quads > device->config_rom_size) {
        *num_quads = device->config_rom_size;
    }

    memcpy (quads, device->config_rom, *num_quads * sizeof (uint32_t));
    return 0;
}

static platform_camera_t *
dc1394_windows_camera_new (platform_t * p, platform_device_t * device,
                           uint32_t unit_directory_offset)
{
    platform_camera_t * camera = malloc (sizeof (platform_camera_t));
    if (!camera) {
        return NULL;
    }

    camera->device = device;
    camera->camera = NULL;
    camera->capture_is_set = 0;
    camera->allocated_channel = 0;
    camera->allocated_bandwidth = 0;
    camera->iso_channel = 0;
    camera->iso_auto_started = 0;

    camera->capture.flags = 0;
    camera->capture.frames = NULL;

    return camera;
}

static void
dc1394_windows_camera_free (platform_camera_t * cam)
{
    free (cam);
}

static void
dc1394_windows_camera_set_parent (platform_camera_t * cam,
                                  dc1394camera_t * parent)
{
    cam->camera = parent;
}

static dc1394error_t
dc1394_windows_camera_print_info (platform_camera_t * cam, FILE *fd)
{
    fprintf(fd,"------ Camera platform-specific information ------\n");
    fprintf(fd,"Device                            :     %p\n", cam->device);
    fprintf(fd,"Camera                            :     %p\n", cam->camera);

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_windows_camera_read (platform_camera_t * cam, uint64_t offset,
                            uint32_t * quads, int num_quads)
{
    int i;

    for (i = 0; i < num_quads; ++i) {
        if (read_retry (cam->device->device_path,
                        0xf0000000 + offset + 4*i, (PULONG)&quads[i]) < 0) {
            return DC1394_FAILURE;
        }
    }

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_windows_camera_write (platform_camera_t * cam, uint64_t offset,
                             const uint32_t * quads, int num_quads)
{
    int i;

    for (i = 0; i < num_quads; ++i) {
        if (write_retry (cam->device->device_path,
                         0xf0000000 + offset + 4*i, quads[i]) < 0) {
            return DC1394_FAILURE;
        }
    }

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_windows_reset_bus (platform_camera_t * cam)
{
    // \todo implement
    return DC1394_FAILURE;
}

static dc1394error_t
dc1394_windows_read_cycle_timer (platform_camera_t * cam,
                                 uint32_t * cycle_timer, uint64_t * local_time)
{
    // \todo implement
    return DC1394_FAILURE;
}

static dc1394error_t
dc1394_windows_set_broadcast (platform_camera_t * craw, dc1394bool_t pwr)
{
    // \todo implement
    return DC1394_FAILURE;
}

static dc1394error_t
dc1394_windows_get_broadcast (platform_camera_t * craw, dc1394bool_t *pwr)
{
    // \todo implement
    return DC1394_FAILURE;
}

static dc1394error_t
dc1394_windows_iso_set_persist (platform_camera_t * cam)
{
    // \todo implement
    return DC1394_FAILURE;
}

static dc1394error_t
dc1394_windows_iso_allocate_bandwidth (platform_camera_t * cam,
                                       int bandwidth_units)
{
    // \todo implement
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_windows_iso_release_bandwidth (platform_camera_t * cam,
                                      int bandwidth_units)
{
    // \todo implement
    //return DC1394_FAILURE;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_windows_camera_get_node(platform_camera_t * cam, uint32_t *node,
                               uint32_t * generation)
{
    *node = cam->device->node;

    // \todo implement
    return DC1394_FAILURE;
}

dc1394error_t
dc1394_camera_get_windows_port(dc1394camera_t *camera, uint32_t *port)
{
    // \todo implement
    return DC1394_FAILURE;
}


static int
dc1394_windows_capture_get_fileno (platform_camera_t * craw)
{
    // \todo implement
    return -1;
}

static platform_dispatch_t
windows_dispatch = {
    .platform_new = dc1394_windows_new,
    .platform_free = dc1394_windows_free,

    .get_device_list = dc1394_windows_get_device_list,
    .free_device_list = dc1394_windows_free_device_list,
    .device_get_config_rom = dc1394_windows_device_get_config_rom,

    .camera_new = dc1394_windows_camera_new,
    .camera_free = dc1394_windows_camera_free,
    .camera_set_parent = dc1394_windows_camera_set_parent,

    .camera_read = dc1394_windows_camera_read,
    .camera_write = dc1394_windows_camera_write,

    .reset_bus = dc1394_windows_reset_bus,
    .camera_print_info = dc1394_windows_camera_print_info,
    .camera_get_node = dc1394_windows_camera_get_node,
    .set_broadcast = dc1394_windows_set_broadcast,
    .get_broadcast = dc1394_windows_get_broadcast,
    .read_cycle_timer = dc1394_windows_read_cycle_timer,

    .capture_setup = dc1394_windows_capture_setup,
    .capture_stop = dc1394_windows_capture_stop,
    .capture_dequeue = dc1394_windows_capture_dequeue,
    .capture_enqueue = dc1394_windows_capture_enqueue,
    .capture_get_fileno = dc1394_windows_capture_get_fileno,

    .iso_set_persist = dc1394_windows_iso_set_persist,
    .iso_allocate_channel = dc1394_windows_iso_allocate_channel,
    .iso_release_channel = dc1394_windows_iso_release_channel,
    .iso_allocate_bandwidth = dc1394_windows_iso_allocate_bandwidth,
    .iso_release_bandwidth = dc1394_windows_iso_release_bandwidth,
    .capture_set_callback = NULL,
    .capture_schedule_with_runloop = NULL,
};

void
windows_init(dc1394_t * d)
{
    register_platform (d, &windows_dispatch, "windows");
}
