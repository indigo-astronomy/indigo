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
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "config.h"
#include "platform.h"
#include "internal.h"
#include "juju.h"

#define ptr_to_u64(p) ((__u64)(unsigned long)(p))
#define u64_to_ptr(p) ((void *)(unsigned long)(p))


static platform_t *
dc1394_juju_new (void)
{
    DIR * dir;
    struct dirent * de;
    int num_devices = 0;

    dir = opendir("/dev");
    if (!dir) {
        dc1394_log_error("Failed to create juju: opendir: %m");
        return NULL;
    }
    while ((de = readdir(dir))) {
        if (strncmp(de->d_name, "fw", 2) != 0)
            continue;
        dc1394_log_debug("Juju: Found /dev/%s", de->d_name);
        num_devices++;
    }
    closedir(dir);

    if (num_devices == 0) {
        dc1394_log_debug("Juju: Found no devices /dev/fw*");
        return NULL;
    }

    platform_t * p = calloc (1, sizeof (platform_t));
    return p;
}
static void
dc1394_juju_free (platform_t * p)
{
    free (p);
}

struct _platform_device_t {
    uint32_t config_rom[256];
    char filename[32];
};

static platform_device_list_t *
dc1394_juju_get_device_list (platform_t * p)
{
    DIR * dir;
    struct dirent * de;
    platform_device_list_t * list;
    uint32_t allocated_size = 64;

    list = calloc (1, sizeof (platform_device_list_t));
    if (!list)
        return NULL;
    list->devices = malloc(allocated_size * sizeof(platform_device_t *));
    if (!list->devices) {
        free (list);
        return NULL;
    }

    dir = opendir("/dev");
    if (dir == NULL) {
        dc1394_log_error("opendir: %m");
        free (list->devices);
        free (list);
        return NULL;
    }

    while ((de = readdir(dir))) {
        char filename[32];
        int fd;
        platform_device_t * device;
        struct fw_cdev_get_info get_info;
        struct fw_cdev_event_bus_reset reset;

        if (strncmp(de->d_name, "fw", 2) != 0 ||
                de->d_name[2] < '0' || de->d_name[2] > '9')
            continue;

        snprintf(filename, sizeof filename, "/dev/%s", de->d_name);
        fd = open(filename, O_RDWR);
        if (fd < 0) {
            dc1394_log_debug("Juju: Failed to open %s: %s", filename,
                    strerror (errno));
            continue;
        }
        dc1394_log_debug("Juju: Opened %s successfully", filename);

        device = malloc (sizeof (platform_device_t));
        if (!device) {
            close (fd);
            continue;
        }

        get_info.version = FW_CDEV_VERSION;
        get_info.rom = ptr_to_u64(&device->config_rom);
        get_info.rom_length = 1024;
        get_info.bus_reset = ptr_to_u64(&reset);
        if (ioctl(fd, FW_CDEV_IOC_GET_INFO, &get_info) < 0) {
            dc1394_log_error("GET_CONFIG_ROM failed for %s: %m",filename);
            free (device);
            close(fd);
            continue;
        }
        close (fd);

        strcpy (device->filename, filename);
        list->devices[list->num_devices] = device;
        list->num_devices++;

        if (list->num_devices >= allocated_size) {
            allocated_size += 64;
            list->devices = realloc (list->devices, allocated_size * sizeof (platform_device_t *));
            if (!list->devices)
                return NULL;
        }
    }
    closedir(dir);

    return list;
}

static void
dc1394_juju_free_device_list (platform_device_list_t * d)
{
    int i;
    for (i = 0; i < d->num_devices; i++)
        free (d->devices[i]);
    free (d->devices);
    free (d);
}

static int
dc1394_juju_device_get_config_rom (platform_device_t * device,
                                uint32_t * quads, int * num_quads)
{
    if (*num_quads > 256)
        *num_quads = 256;

    memcpy (quads, device->config_rom, *num_quads * sizeof (uint32_t));
    return 0;
}

static juju_iso_info *
add_iso_resource (platform_camera_t *cam)
{
    juju_iso_info *res = calloc (1, sizeof (juju_iso_info));
    if (!res)
        return NULL;
    res->next = cam->iso_resources;
    cam->iso_resources = res;
    return res;
}

static void
remove_iso_resource (platform_camera_t *cam, juju_iso_info * res)
{
    juju_iso_info **ptr = &cam->iso_resources;
    while (*ptr) {
        if (*ptr == res) {
            *ptr = res->next;
            free (res);
            return;
        }
        ptr = &(*ptr)->next;
    }
}

static platform_camera_t *
dc1394_juju_camera_new (platform_t * p, platform_device_t * device, uint32_t unit_directory_offset)
{
    int fd;
    platform_camera_t * camera;
    struct fw_cdev_get_info get_info;
    struct fw_cdev_event_bus_reset reset;

    fd = open(device->filename, O_RDWR);
    if (fd < 0) {
        dc1394_log_error("could not open device %s: %m", device->filename);
        return NULL;
    }

    get_info.version = FW_CDEV_VERSION;
    get_info.rom = 0;
    get_info.rom_length = 0;
    get_info.bus_reset = ptr_to_u64(&reset);
    if (ioctl(fd, FW_CDEV_IOC_GET_INFO, &get_info) < 0) {
        dc1394_log_error("IOC_GET_INFO failed for a device %s: %m",device->filename);
        close(fd);
        return NULL;
    }

    dc1394_log_debug("Juju: kernel API has version %d", get_info.version);

    camera = calloc (1, sizeof (platform_camera_t));
    camera->fd = fd;
    camera->generation = reset.generation;
    camera->node_id = reset.node_id;
    strcpy (camera->filename, device->filename);

    camera->header_size = 4;
    if (get_info.version >= 2)
        camera->header_size = 8;

    camera->kernel_abi_version=get_info.version;

    return camera;
}

static void dc1394_juju_camera_free (platform_camera_t * cam)
{
    while (cam->iso_resources)
        remove_iso_resource (cam, cam->iso_resources);
    close (cam->fd);
    free (cam);
}

static void
dc1394_juju_camera_set_parent (platform_camera_t * cam, dc1394camera_t * parent)
{
    cam->camera = parent;
}

static dc1394error_t
dc1394_juju_camera_print_info (platform_camera_t * camera, FILE *fd)
{
    fprintf(fd,"------ Camera platform-specific information ------\n");
    fprintf(fd,"Device filename                   :     %s\n", camera->filename);
    return DC1394_SUCCESS;
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct _juju_response_info {
    int got_response;
    uint32_t rcode;
    uint32_t *data;
    int num_quads;
    int actual_num_quads;
} juju_response_info;

static int
juju_handle_event (platform_camera_t * cam)
{
    union {
        struct {
            struct fw_cdev_event_response r;
            __u32 buffer[cam->max_response_quads];
        } response;
        struct fw_cdev_event_bus_reset reset;
        struct fw_cdev_event_iso_resource resource;
    } u;
    int len, i;
    juju_response_info *resp_info;
    juju_iso_info *iso_info;

    len = read (cam->fd, &u, sizeof u);
    if (len < 0) {
        dc1394_log_error("juju: Read failed: %m");
        return -1;
    }

    switch (u.reset.type) {
    case FW_CDEV_EVENT_BUS_RESET:
        cam->generation = u.reset.generation;
        cam->node_id = u.reset.node_id;
        dc1394_log_debug ("juju: Bus reset, gen %d, node 0x%x",
                cam->generation, cam->node_id);
        break;

    case FW_CDEV_EVENT_RESPONSE:
        if (!u.response.r.closure) {
            dc1394_log_warning ("juju: Unsolicited response, rcode %x len %d",
                    u.response.r.rcode, u.response.r.length);
            break;
        }
        resp_info = u64_to_ptr(u.response.r.closure);
        resp_info->rcode = u.response.r.rcode;
        resp_info->actual_num_quads = u.response.r.length/4;
        resp_info->got_response = 1;
        if (resp_info->rcode || !resp_info->data)
            break;
        if (cam->max_response_quads < resp_info->actual_num_quads) {
            dc1394_log_error ("juju: read buffer too small, have %d needed %d",
                    cam->max_response_quads, resp_info->actual_num_quads);
            break;
        }

        len = MIN(resp_info->actual_num_quads, resp_info->num_quads);
        for (i = 0; i < len; i++)
            resp_info->data[i] = ntohl (u.response.r.data[i]);
        break;

    case FW_CDEV_EVENT_ISO_RESOURCE_ALLOCATED:
        if (!u.resource.closure) {
            dc1394_log_warning ("juju: Spurious ISO allocation event: "
                    "handle %d, chan %d, bw %d", u.resource.handle,
                    u.resource.channel, u.resource.bandwidth);
            break;
        }
        iso_info = u64_to_ptr(u.resource.closure);
        if (iso_info->handle != u.resource.handle)
            dc1394_log_warning ("juju: ISO alloc handle was %d, expected %d",
                    u.resource.handle, iso_info->handle);
        dc1394_log_debug ("juju: Allocated handle %d: chan %d bw %d",
                u.resource.handle, u.resource.channel, u.resource.bandwidth);
        iso_info->got_alloc = 1;
        iso_info->channel = u.resource.channel;
        iso_info->bandwidth = u.resource.bandwidth;
        break;

    case FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED:
        if (!u.resource.closure) {
            dc1394_log_warning ("juju: Spurious ISO deallocation event: "
                    "handle %d, chan %d, bw %d", u.resource.handle,
                    u.resource.channel, u.resource.bandwidth);
            break;
        }
        iso_info = u64_to_ptr(u.resource.closure);
        if (iso_info->handle != u.resource.handle)
            dc1394_log_warning ("juju: ISO dealloc handle was %d, expected %d",
                    u.resource.handle, iso_info->handle);
        dc1394_log_debug ("juju: Deallocated handle %d: chan %d bw %d",
                u.resource.handle, u.resource.channel, u.resource.bandwidth);
        iso_info->got_dealloc = 1;
        iso_info->channel = u.resource.channel;
        iso_info->bandwidth = u.resource.bandwidth;
        break;

    default:
        dc1394_log_warning ("juju: Unhandled event type %d",
                u.reset.type);
        break;
    }

    return 0;
}

static dc1394error_t
do_transaction(platform_camera_t * cam, int tcode, uint64_t offset,
        const uint32_t * in, uint32_t * out, uint32_t num_quads)
{
    struct fw_cdev_send_request request;
    juju_response_info resp;
    int i, len;
    uint32_t in_buffer[in ? num_quads : 0];
    int retry = DC1394_MAX_RETRIES;

    for (i = 0; in && i < num_quads; i++)
        in_buffer[i] = htonl (in[i]);

    resp.data = out;
    resp.num_quads = out ? num_quads : 0;
    cam->max_response_quads = resp.num_quads;

    request.closure = ptr_to_u64(&resp);
    request.offset = CONFIG_ROM_BASE + offset;
    request.data = ptr_to_u64(in_buffer);
    request.length = num_quads * 4;
    request.tcode = tcode;

    while (retry > 0) {
        int retval;

        request.generation = cam->generation;

        int iotype = FW_CDEV_IOC_SEND_REQUEST;
        if (cam->broadcast_enabled && (tcode == TCODE_WRITE_BLOCK_REQUEST ||
                    tcode == TCODE_WRITE_QUADLET_REQUEST))
            iotype = FW_CDEV_IOC_SEND_BROADCAST_REQUEST;

        len = ioctl (cam->fd, iotype, &request);
        if (len < 0) {
            dc1394_log_error("juju: Send request failed: %m");
            return DC1394_FAILURE;
        }

        resp.got_response = 0;
        while (!resp.got_response)
            if ((retval = juju_handle_event (cam)) < 0)
                return retval;

        if (resp.rcode == 0) {
            if (resp.num_quads != resp.actual_num_quads)
                dc1394_log_warning("juju: Expected response len %d, got %d",
                        resp.num_quads, resp.actual_num_quads);
            return DC1394_SUCCESS;
        }

        if (resp.rcode != RCODE_BUSY
                && resp.rcode != RCODE_CONFLICT_ERROR
                && resp.rcode != RCODE_GENERATION) {
            dc1394_log_debug ("juju: Response error, rcode 0x%x",
                    resp.rcode);
            return DC1394_FAILURE;
        }

        /* retry if we get any of the rcodes listed above */
        dc1394_log_debug("juju: retry rcode 0x%x tcode 0x%x offset %"PRIx64,
                resp.rcode, tcode, offset);
        usleep (DC1394_SLOW_DOWN);
        retry--;
    }

    dc1394_log_error("juju: Max retries for tcode 0x%x, offset %"PRIx64,
            tcode, offset);
    return DC1394_FAILURE;
}

static dc1394error_t
dc1394_juju_camera_read (platform_camera_t * cam, uint64_t offset, uint32_t * quads, int num_quads)
{
    int tcode;

    if (num_quads > 1)
        tcode = TCODE_READ_BLOCK_REQUEST;
    else
        tcode = TCODE_READ_QUADLET_REQUEST;

    dc1394error_t err = do_transaction(cam, tcode, offset, NULL, quads, num_quads);
    return err;
}

static dc1394error_t
dc1394_juju_camera_write (platform_camera_t * cam, uint64_t offset, const uint32_t * quads, int num_quads)
{
    int tcode;

    if (num_quads > 1)
        tcode = TCODE_WRITE_BLOCK_REQUEST;
    else
        tcode = TCODE_WRITE_QUADLET_REQUEST;

    return do_transaction(cam, tcode, offset, quads, NULL, num_quads);
}

static dc1394error_t
dc1394_juju_reset_bus (platform_camera_t * cam)
{
    struct fw_cdev_initiate_bus_reset initiate;

    initiate.type = FW_CDEV_SHORT_RESET;
    if (ioctl (cam->fd, FW_CDEV_IOC_INITIATE_BUS_RESET, &initiate) == 0)
        return DC1394_SUCCESS;
    else
        return DC1394_FAILURE;
}

static dc1394error_t
dc1394_juju_camera_get_node(platform_camera_t *cam, uint32_t *node,
        uint32_t * generation)
{
    if (node)
        *node = cam->node_id & 0x3f;  // mask out the bus ID
    if (generation)
        *generation = cam->generation;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_juju_read_cycle_timer (platform_camera_t * cam,
        uint32_t * cycle_timer, uint64_t * local_time)
{
    struct fw_cdev_get_cycle_timer tm;

    if (ioctl(cam->fd, FW_CDEV_IOC_GET_CYCLE_TIMER, &tm) < 0) {
        if (errno == EINVAL)
            return DC1394_FUNCTION_NOT_SUPPORTED;
        dc1394_log_error("Juju: get_cycle_timer ioctl failed: %m");
        return DC1394_FAILURE;
    }

    if (cycle_timer)
        *cycle_timer = tm.cycle_timer;
    if (local_time)
        *local_time = tm.local_time;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_juju_set_broadcast(platform_camera_t * craw, dc1394bool_t pwr)
{
    if (pwr == DC1394_FALSE) {
        craw->broadcast_enabled = 0;
        return DC1394_SUCCESS;
    }

    if (craw->broadcast_enabled)
        return DC1394_SUCCESS;

    /* Test if the ioctl is available by sending a broadcast write to
     * offset 0 that the kernel will always reject with EACCES if it is. */
    struct fw_cdev_send_request request;
    memset(&request, 0, sizeof(struct fw_cdev_send_request));
    request.tcode = TCODE_WRITE_BLOCK_REQUEST;

    if (ioctl(craw->fd, FW_CDEV_IOC_SEND_BROADCAST_REQUEST, &request) != -1) {
        dc1394_log_error("Juju: broadcast test succeeded unexpectedly\n");
        return DC1394_FUNCTION_NOT_SUPPORTED;
    }
    if (errno == EINVAL)
        return DC1394_FUNCTION_NOT_SUPPORTED;

    craw->broadcast_enabled = 1;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_juju_get_broadcast(platform_camera_t * craw, dc1394bool_t *pwr)
{
    if (craw->broadcast_enabled)
        *pwr = DC1394_TRUE;
    else
        *pwr = DC1394_FALSE;

    return DC1394_SUCCESS;
}

dc1394error_t
juju_iso_allocate (platform_camera_t *cam, uint64_t allowed_channels,
        int bandwidth_units, juju_iso_info **out)
{
    // Check kernel ABI version for ISO allocation support 
    if (cam->kernel_abi_version < 2)
        return DC1394_FUNCTION_NOT_SUPPORTED;

    juju_iso_info *res = add_iso_resource (cam);
    if (!res)
        return DC1394_MEMORY_ALLOCATION_FAILURE;

    struct fw_cdev_allocate_iso_resource request = {
        .closure = ptr_to_u64(res),
        .channels = allowed_channels,
        .bandwidth = bandwidth_units,
    };
    if (ioctl (cam->fd, FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE, &request) < 0) {
        remove_iso_resource (cam, res);
        if (errno == EINVAL)
            return DC1394_INVALID_ARGUMENT_VALUE;
        return DC1394_FAILURE;
    }
    res->handle = request.handle;
    dc1394_log_debug ("juju: Attempting iso allocation: "
            "handle %d, chan 0x%"PRIx64", bw %d", request.handle,
            request.channels, request.bandwidth);

    int ret;
    while (!res->got_alloc)
        if ((ret = juju_handle_event (cam)) < 0)
            return ret;

    if (allowed_channels && res->channel < 0) {
        remove_iso_resource (cam, res);
        return DC1394_NO_ISO_CHANNEL;
    }
    if (bandwidth_units && !res->bandwidth) {
        remove_iso_resource (cam, res);
        return DC1394_NO_BANDWIDTH;
    }

    if (out)
        *out = res;
    return DC1394_SUCCESS;
}

dc1394error_t
juju_iso_deallocate (platform_camera_t *cam, juju_iso_info * res)
{
    // Check kernel ABI version for ISO allocation support 
    if (cam->kernel_abi_version < 2)
        return DC1394_FUNCTION_NOT_SUPPORTED;

    if (res->got_dealloc) {
        dc1394_log_warning ("juju: ISO resource was already released");
        remove_iso_resource (cam, res);
        return DC1394_SUCCESS;
    }

    struct fw_cdev_allocate_iso_resource request = {
        .handle = res->handle,
    };
    if (ioctl (cam->fd, FW_CDEV_IOC_DEALLOCATE_ISO_RESOURCE, &request) < 0) {
        if (errno == EINVAL)
            return DC1394_INVALID_ARGUMENT_VALUE;
        return DC1394_FAILURE;
    }

    int ret;
    while (!res->got_dealloc)
        if ((ret = juju_handle_event (cam)) < 0)
            return ret;

    remove_iso_resource (cam, res);
    return DC1394_SUCCESS;
}

#if 0
static dc1394error_t
dc1394_juju_iso_allocate_channel(platform_camera_t *cam, uint64_t allowed,
            int *out_channel)
{
}

static dc1394error_t
dc1394_juju_iso_release_channel(platform_camera_t *cam, int channel)
{
}

static dc1394error_t
dc1394_juju_iso_allocate_bandwidth(platform_camera_t *cam, int units)
{
}

static dc1394error_t
dc1394_juju_iso_release_bandwidth(platform_camera_t *cam, int units)
{
}
#endif

static platform_dispatch_t
juju_dispatch = {
    .platform_new = dc1394_juju_new,
    .platform_free = dc1394_juju_free,

    .get_device_list = dc1394_juju_get_device_list,
    .free_device_list = dc1394_juju_free_device_list,
    .device_get_config_rom = dc1394_juju_device_get_config_rom,

    .camera_new = dc1394_juju_camera_new,
    .camera_free = dc1394_juju_camera_free,
    .camera_set_parent = dc1394_juju_camera_set_parent,

    .camera_read = dc1394_juju_camera_read,
    .camera_write = dc1394_juju_camera_write,

    .reset_bus = dc1394_juju_reset_bus,
    .camera_print_info = dc1394_juju_camera_print_info,
    .camera_get_node = dc1394_juju_camera_get_node,
    .read_cycle_timer = dc1394_juju_read_cycle_timer,
    .set_broadcast = dc1394_juju_set_broadcast,
    .get_broadcast = dc1394_juju_get_broadcast,

    .capture_setup = dc1394_juju_capture_setup,
    .capture_stop = dc1394_juju_capture_stop,
    .capture_dequeue = dc1394_juju_capture_dequeue,
    .capture_enqueue = dc1394_juju_capture_enqueue,
    .capture_get_fileno = dc1394_juju_capture_get_fileno,
    .capture_set_callback = NULL,
    .capture_schedule_with_runloop = NULL,

    //.iso_allocate_channel = dc1394_juju_iso_allocate_channel,
};

void
juju_init(dc1394_t * d)
{
    register_platform (d, &juju_dispatch, "juju");
}
