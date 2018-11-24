/*
 * 1394-Based Digital Camera Control Library
 *
 * Camera control code for Linux
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

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "dc1394/internal.h"
#include "linux.h"
#include "offsets.h"
#include "types.h"

static int
is_device_available (const char * filename)
{
    int fd;
    struct stat sbuf;
    if (!filename)
        return 0;
    if (stat (filename, &sbuf) < 0)
        return 0;
    fd = open (filename, O_RDWR);
    if (fd < 0)
        return 0;
    close (fd);
    dc1394_log_debug("linux: Found %s", filename);
    return 1;
}

static platform_t *
dc1394_linux_new (void)
{
    if (!is_device_available (getenv("RAW1394DEV")) &&
            !is_device_available ("/dev/raw1394")) {
        dc1394_log_debug("linux: Failed to open RAW1394DEV or /dev/raw1394");
        return NULL;
    }

    platform_t * p = calloc (1, sizeof (platform_t));
    return p;
}

static void
dc1394_linux_free (platform_t * p)
{
    free (p);
}

struct _platform_device_t {
    uint32_t config_rom[256];
    int num_quads;
    int port, node, generation;
};

static int
read_retry (struct raw1394_handle * handle, nodeid_t node, nodeaddr_t addr,
            size_t length, quadlet_t * buffer)
{
    int retry = DC1394_MAX_RETRIES;
    while (retry > 0) {
        if (raw1394_read (handle, node, addr, length, buffer) == 0)
            return 0;
        if (errno != EAGAIN)
            return -1;

        usleep (DC1394_SLOW_DOWN);
        retry--;
    }
    return -1;
}

static platform_device_list_t *
dc1394_linux_get_device_list (platform_t * p)
{
    platform_device_list_t * list;
    uint32_t allocated_size = 64;
    raw1394handle_t handle;
    int num_ports, i;

    handle = raw1394_new_handle ();
    if (!handle)
        return NULL;

    num_ports = raw1394_get_port_info (handle, NULL, 0);
    dc1394_log_debug ("linux: Found %d port(s)", num_ports);
    raw1394_destroy_handle (handle);

    list = calloc (1, sizeof (platform_device_list_t));
    if (!list)
        return NULL;
    list->devices = malloc(allocated_size * sizeof(platform_device_t *));
    if (!list->devices) {
        free (list);
        return NULL;
    }

    for (i = 0; i < num_ports; i++) {
        int num_nodes, j;

        handle = raw1394_new_handle_on_port (i);
        if (!handle)
            continue;

        num_nodes = raw1394_get_nodecount (handle);
        dc1394_log_debug ("linux: Port %d opened with %d node(s)",
                i, num_nodes);
        for (j = 0; j < num_nodes; j++) {
            platform_device_t * device;
            uint32_t quad;
            int k;

            if (read_retry (handle, 0xFFC0 | j, CONFIG_ROM_BASE + 0x400, 4, &quad) < 0)
                continue;

            device = malloc (sizeof (platform_device_t));
            if (!device)
                continue;

            device->config_rom[0] = ntohl (quad);
            device->port = i;
            device->node = j;
            device->generation = raw1394_get_generation (handle);
            for (k = 1; k < 256; k++) {
                if (read_retry (handle, 0xFFC0 | j, CONFIG_ROM_BASE + 0x400 + 4*k, 4, &quad) < 0)
                    break;
                device->config_rom[k] = ntohl (quad);
            }
            device->num_quads = k;

            list->devices[list->num_devices] = device;
            list->num_devices++;

            if (list->num_devices >= allocated_size) {
                allocated_size += 64;
                list->devices = realloc (list->devices, allocated_size * sizeof (platform_device_t *));
                if (!list->devices)
                    return NULL;
            }
        }
        raw1394_destroy_handle (handle);
    }

    return list;
}

static void
dc1394_linux_free_device_list (platform_device_list_t * d)
{
    int i;
    for (i = 0; i < d->num_devices; i++)
        free (d->devices[i]);
    free (d->devices);
    free (d);
}

static int
dc1394_linux_device_get_config_rom (platform_device_t * device,
    uint32_t * quads, int * num_quads)
{
    if (*num_quads > device->num_quads)
        *num_quads = device->num_quads;

    memcpy (quads, device->config_rom, *num_quads * sizeof (uint32_t));
    return 0;
}

static platform_camera_t *
dc1394_linux_camera_new (platform_t * p, platform_device_t * device, uint32_t unit_directory_offset)
{
    platform_camera_t * camera;
    raw1394handle_t handle;

    handle = raw1394_new_handle_on_port (device->port);
    if (!handle)
        return NULL;

    if (device->generation != raw1394_get_generation (handle)) {
        dc1394_log_error("generation has changed since bus was scanned");
        raw1394_destroy_handle (handle);
        return NULL;
    }

    camera = calloc (1, sizeof (platform_camera_t));
    camera->handle = handle;
    camera->port = device->port;
    camera->node = device->node;
    camera->generation = device->generation;
    return camera;
}

static void
dc1394_linux_camera_free (platform_camera_t * cam)
{
    if (cam->capture.dma_device_file != NULL) {
        free (cam->capture.dma_device_file);
        cam->capture.dma_device_file = NULL;
    }

    raw1394_destroy_handle (cam->handle);
    free (cam);
}

static void
dc1394_linux_camera_set_parent (platform_camera_t * cam,
        dc1394camera_t * parent)
{
    cam->camera = parent;
}

static dc1394error_t
dc1394_linux_camera_print_info (platform_camera_t * cam, FILE *fd)
{
    fprintf(fd,"------ Camera platform-specific information ------\n");
    fprintf(fd,"Handle                            :     %p\n", cam->handle);
    fprintf(fd,"Port                              :     %d\n", cam->port);
    fprintf(fd,"Node                              :     %d\n", cam->node);
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_camera_read (platform_camera_t * cam, uint64_t offset,
        uint32_t * quads, int num_quads)
{
    int i, retval, retry = DC1394_MAX_RETRIES;

    /* retry a few times if necessary (addition by PDJ) */
    while(retry--)  {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
        fprintf(stderr,"get %d regs at 0x%llx : ",
                num_quads, offset + CONFIG_ROM_BASE);
#endif
        retval = raw1394_read (cam->handle, 0xffc0 | cam->node, offset + CONFIG_ROM_BASE, 4 * num_quads, quads);
#ifdef DC1394_DEBUG_LOWEST_LEVEL
        fprintf(stderr,"0x%lx [...]\n", quads[0]);
#endif

        if (!retval)
            goto out;
        else if (errno != EAGAIN)
            return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );

        // usleep is executed only if the read fails!!!
        usleep(DC1394_SLOW_DOWN);
    }

 out:
    /* conditionally byte swap the value */
    for (i = 0; i < num_quads; i++)
        quads[i] = ntohl (quads[i]);
    return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
}

static dc1394error_t
dc1394_linux_camera_write (platform_camera_t * cam, uint64_t offset,
        const uint32_t * quads, int num_quads)
{
    int i, retval, retry= DC1394_MAX_RETRIES;
    uint32_t value[num_quads];

    /* conditionally byte swap the value (addition by PDJ) */
    for (i = 0; i < num_quads; i++)
        value[i] = htonl (quads[i]);

    /* retry a few times if necessary */
    while(retry--) {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
        fprintf(stderr,"set %d regs at 0x%llx to value 0x%lx [...]\n",
                num_quads, offset + CONFIG_ROM_BASE, value[0]);
#endif
        retval = raw1394_write(cam->handle, 0xffc0 | cam->node, offset + CONFIG_ROM_BASE, 4 * num_quads, value);

        if (!retval || (errno != EAGAIN))
            return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );

        // usleep is executed only if the read fails!!!
        usleep(DC1394_SLOW_DOWN);
    }

    return DC1394_RAW1394_FAILURE;
}

static dc1394error_t
dc1394_linux_reset_bus (platform_camera_t * cam)
{
    if (raw1394_reset_bus (cam->handle) == 0)
        return DC1394_SUCCESS;
    else
        return DC1394_FAILURE;
}

static dc1394error_t
dc1394_linux_read_cycle_timer (platform_camera_t * cam,
        uint32_t * cycle_timer, uint64_t * local_time)
{
    quadlet_t quad;
    struct timeval tv;

    if (raw1394_read (cam->handle, raw1394_get_local_id (cam->handle),
                      CSR_REGISTER_BASE + CSR_CYCLE_TIME, sizeof (quadlet_t), &quad) < 0)
        return DC1394_FAILURE;

    gettimeofday (&tv, NULL);
    *cycle_timer = ntohl (quad);
    *local_time = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_set_broadcast(platform_camera_t * craw, dc1394bool_t pwr)
{
    if (pwr==DC1394_TRUE) {
        if (craw->broadcast_is_set==DC1394_FALSE) {
            craw->backup_node_id=craw->node;
            craw->node=63;
            craw->broadcast_is_set=DC1394_TRUE;
        }
    }
    else if (pwr==DC1394_FALSE) {
        if (craw->broadcast_is_set==DC1394_TRUE) {
            craw->node=craw->backup_node_id;
            craw->broadcast_is_set=DC1394_FALSE;
        }
    }
    else
        return DC1394_INVALID_ARGUMENT_VALUE;

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_get_broadcast(platform_camera_t * craw, dc1394bool_t *pwr)
{
    *pwr=craw->broadcast_is_set;

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_iso_set_persist (platform_camera_t * cam)
{
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_iso_allocate_channel (platform_camera_t * cam,  uint64_t channels_allowed, int * channel)
{
    int i;

    for (i = 0; i < 64; i++) {
        if (!((channels_allowed >> i) & 1))
            continue;
        if (raw1394_channel_modify (cam->handle, i, RAW1394_MODIFY_ALLOC) == 0)
            break;
    }

    if (i == 64) {
        dc1394_log_error ("Error: Failed to allocate iso channel");
        return DC1394_NO_ISO_CHANNEL;
    }

    *channel = i;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_iso_release_channel (platform_camera_t * cam, int channel)
{
    if (raw1394_channel_modify (cam->handle, channel, RAW1394_MODIFY_FREE) < 0) {
        dc1394_log_error("Error: Could not free iso channel");
        return DC1394_FAILURE;
    }

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_iso_allocate_bandwidth (platform_camera_t * cam,
        int bandwidth_units)
{
    if (raw1394_bandwidth_modify (cam->handle, bandwidth_units, RAW1394_MODIFY_ALLOC) < 0) {
        dc1394_log_error ("Error: Failed to allocate iso bandwidth");
        return DC1394_NO_BANDWIDTH;
    }

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_iso_release_bandwidth (platform_camera_t * cam,
        int bandwidth_units)
{
    if (raw1394_bandwidth_modify (cam->handle, bandwidth_units, RAW1394_MODIFY_FREE) < 0) {
        dc1394_log_error ("Error: Failed to free iso bandwidth");
        return DC1394_FAILURE;
    }

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_linux_camera_get_node(platform_camera_t *cam, uint32_t *node,
        uint32_t * generation)
{
    if (node)
        *node = cam->node;
    if (generation)
        *generation = cam->generation;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_camera_get_linux_port(dc1394camera_t *camera, uint32_t *port)
{

    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    platform_camera_t * craw = cpriv->pcam;

    *port=craw->port;

    return DC1394_SUCCESS;
}

static platform_dispatch_t
linux_dispatch = {
    .platform_new = dc1394_linux_new,
    .platform_free = dc1394_linux_free,

    .get_device_list = dc1394_linux_get_device_list,
    .free_device_list = dc1394_linux_free_device_list,
    .device_get_config_rom = dc1394_linux_device_get_config_rom,

    .camera_new = dc1394_linux_camera_new,
    .camera_free = dc1394_linux_camera_free,
    .camera_set_parent = dc1394_linux_camera_set_parent,

    .camera_read = dc1394_linux_camera_read,
    .camera_write = dc1394_linux_camera_write,

    .reset_bus = dc1394_linux_reset_bus,
    .camera_print_info = dc1394_linux_camera_print_info,
    .camera_get_node = dc1394_linux_camera_get_node,
    .set_broadcast = dc1394_linux_set_broadcast,
    .get_broadcast = dc1394_linux_get_broadcast,
    .read_cycle_timer = dc1394_linux_read_cycle_timer,

    .capture_setup = dc1394_linux_capture_setup,
    .capture_stop = dc1394_linux_capture_stop,
    .capture_dequeue = dc1394_linux_capture_dequeue,
    .capture_enqueue = dc1394_linux_capture_enqueue,
    .capture_get_fileno = dc1394_linux_capture_get_fileno,
    .capture_set_callback = NULL,
    .capture_schedule_with_runloop = NULL,

    .iso_set_persist = dc1394_linux_iso_set_persist,
    .iso_allocate_channel = dc1394_linux_iso_allocate_channel,
    .iso_release_channel = dc1394_linux_iso_release_channel,
    .iso_allocate_bandwidth = dc1394_linux_iso_allocate_bandwidth,
    .iso_release_bandwidth = dc1394_linux_iso_release_bandwidth,
};

void
linux_init(dc1394_t * d)
{
    register_platform (d, &linux_dispatch, "linux");
}
