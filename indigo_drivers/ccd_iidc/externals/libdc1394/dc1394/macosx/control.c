/*
 * 1394-Based Digital Camera Control Library
 *
 * Mac OS X Digital Camera Control Code
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
#include <sys/time.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/firewire/IOFireWireLib.h>

#include "config.h"
#include "platform.h"
#include "internal.h"
#include "macosx.h"

static platform_t *
dc1394_macosx_new (void)
{
    platform_t * p = calloc (1, sizeof (platform_t));
    return p;
}

static void
dc1394_macosx_free (platform_t * p)
{
    free (p);
}

struct _platform_device_t {
    io_object_t node;
};

static platform_device_list_t *
dc1394_macosx_get_device_list (platform_t * p)
{
    platform_device_list_t * list;
    uint32_t allocated_size = 64;
    kern_return_t res;
    mach_port_t master_port;
    io_iterator_t iterator;
    io_object_t node;
    CFMutableDictionaryRef dict;

    list = calloc (1, sizeof (platform_device_list_t));
    if (!list)
        return NULL;
    list->devices = malloc(allocated_size * sizeof(platform_device_t *));
    if (!list->devices) {
        free (list);
        return NULL;
    }

    res = IOMasterPort (MACH_PORT_NULL, &master_port);
    if (res != KERN_SUCCESS)
        return NULL;

    dict = IOServiceMatching ("IOFireWireDevice");
    if (!dict)
        return NULL;

    res = IOServiceGetMatchingServices (master_port, dict, &iterator);

    while ((node = IOIteratorNext (iterator))) {
        platform_device_t * device = malloc (sizeof (platform_device_t));
        if (!device) {
            IOObjectRelease (node);
            continue;
        }

        device->node = node;
        list->devices[list->num_devices] = device;
        list->num_devices++;

        if (list->num_devices >= allocated_size) {
            allocated_size += 64;
            list->devices = realloc (list->devices,
                                     allocated_size * sizeof (platform_device_t *));
            if (!list->devices)
                return NULL;
        }
    }
    IOObjectRelease (iterator);

    return list;
}

static void
dc1394_macosx_free_device_list (platform_device_list_t * d)
{
    int i;
    for (i = 0; i < d->num_devices; i++) {
        IOObjectRelease (d->devices[i]->node);
        free (d->devices[i]);
    }
    free (d->devices);
    free (d);
}

static int
dc1394_macosx_device_get_config_rom (platform_device_t * device,
    uint32_t * quads, int * num_quads)
{
    CFTypeRef prop;
    prop = IORegistryEntryCreateCFProperty (device->node,
                                            CFSTR ("FireWire Device ROM"), kCFAllocatorDefault, 0);
    if (!prop)
        return -1;
    CFDataRef data = CFDictionaryGetValue (prop, CFSTR ("Offset 0"));
    if (!data) {
        CFRelease (prop);
        return -1;
    }

    int nquads = CFDataGetLength (data) / 4;
    if (*num_quads > nquads)
        *num_quads = nquads;
    const uint8_t * d = CFDataGetBytePtr (data);
    int i;
    for (i = 0; i < *num_quads; i++)
        quads[i] = (d[4*i] << 24) | (d[4*i+1] << 16) | (d[4*i+2] << 8) | d[4*i+3];

    CFRelease (prop);
    return 0;
}

static platform_camera_t *
dc1394_macosx_camera_new (platform_t * p, platform_device_t * device,
    uint32_t unit_directory_offset)
{
    kern_return_t res;
    platform_camera_t * camera;
    IOFireWireLibDeviceRef iface = NULL;
    io_iterator_t iterator;
    io_object_t node;

    res = IORegistryEntryGetChildIterator (device->node, kIOServicePlane,
            &iterator);
    if (res != KERN_SUCCESS) {
        dc1394_log_error ("Failed to iterate device's children\n");
        return NULL;
    }

    while ((node = IOIteratorNext (iterator))) {
        io_name_t name;
        IOFireWireLibConfigDirectoryRef dir;
        int entries;
        int i;
        UInt32 val;
        IOCFPlugInInterface ** plugin_interface = NULL;
        SInt32 score;

        IORegistryEntryGetName (node, name);
        if (strcmp (name, "IOFireWireUnit")) {
            IOObjectRelease (node);
            continue;
        }

        res = IOCreatePlugInInterfaceForService (node, kIOFireWireLibTypeID,
                kIOCFPlugInInterfaceID, &plugin_interface, &score);
        IOObjectRelease (node);
        if (res != KERN_SUCCESS)
            continue;

        iface = NULL;
        (*plugin_interface)->QueryInterface (plugin_interface,
                CFUUIDGetUUIDBytes (kIOFireWireDeviceInterfaceID),
                (void**) &iface);
        IODestroyPlugInInterface (plugin_interface);
        if (!iface)
            continue;

        dir = (*iface)->GetConfigDirectory (iface,
                CFUUIDGetUUIDBytes (kIOFireWireConfigDirectoryInterfaceID));
        if (!dir) {
            (*iface)->Release (iface);
            continue;
        }

        (*dir)->GetNumEntries (dir, &entries);
        val = 0;
        for (i = 0; i < entries; i++) {
            int key;
            (*dir)->GetIndexKey (dir, i, &key);
            if (key == kConfigUnitDependentInfoKey) {
                (*dir)->GetIndexOffset_UInt32 (dir, i, &val);
                break;
            }
        }
        (*dir)->Release (dir);
        if (val && val == (unit_directory_offset - 0x400) / 4)
            break;
        (*iface)->Release (iface);
    }
    IOObjectRelease (iterator);
    if (!node) {
        dc1394_log_error ("Matching unit not found\n");
        return NULL;
    }

    res = (*iface)->Open (iface);
    if (res != kIOReturnSuccess) {
        dc1394_log_error ("Failed to gain exclusive access on unit (err %d)\n",
                res);
        (*iface)->Release (iface);
        return NULL;
    }

    camera = calloc (1, sizeof (platform_camera_t));
    camera->iface = iface;
    (*camera->iface)->GetBusGeneration (camera->iface,
                                        &(camera->generation));
    return camera;
}

static void dc1394_macosx_camera_free (platform_camera_t * cam)
{
    (*cam->iface)->Close (cam->iface);
    (*cam->iface)->Release (cam->iface);
    free (cam);
}

static void
dc1394_macosx_camera_set_parent (platform_camera_t * cam,
        dc1394camera_t * parent)
{
    cam->camera = parent;
}

static dc1394error_t
dc1394_macosx_camera_print_info (platform_camera_t * camera, FILE *fd)
{
    fprintf(fd,"------ Camera platform-specific information ------\n");
    fprintf(fd,"Interface                       :     %p\n", camera->iface);
    fprintf(fd,"Generation                      :     %lu\n", camera->generation);
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_macosx_camera_read (platform_camera_t * cam, uint64_t offset,
    uint32_t * quads, int num_quads)
{
    IOFireWireLibDeviceRef d = cam->iface;
    FWAddress full_addr;
    int i, retval;
    UInt32 length;
    UInt64 addr = CONFIG_ROM_BASE + offset;

    full_addr.addressHi = addr >> 32;
    full_addr.addressLo = addr & 0xffffffff;

    length = 4 * num_quads;
    if (num_quads > 1)
        retval = (*d)->Read (d, (*d)->GetDevice (d), &full_addr, quads, &length,
                             false, cam->generation);
    else
        retval = (*d)->ReadQuadlet (d, (*d)->GetDevice (d), &full_addr,
                                    (UInt32 *) quads, false, cam->generation);
    if (retval != 0) {
        dc1394_log_error("Error reading (%x)...",retval);
        return DC1394_FAILURE;
    }
    for (i = 0; i < num_quads; i++)
        quads[i] = ntohl (quads[i]);
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_macosx_camera_write (platform_camera_t * cam, uint64_t offset,
    const uint32_t * quads, int num_quads)
{
    IOFireWireLibDeviceRef d = cam->iface;
    FWAddress full_addr;
    int i, retval;
    UInt32 length;
    UInt64 addr = CONFIG_ROM_BASE + offset;
    uint32_t values[num_quads];

    full_addr.addressHi = addr >> 32;
    full_addr.addressLo = addr & 0xffffffff;

    for (i = 0; i < num_quads; i++)
        values[i] = htonl (quads[i]);

    length = 4 * num_quads;
    if (num_quads > 1)
        retval = (*d)->Write (d, (*d)->GetDevice (d), &full_addr, values, &length,
                              false, cam->generation);
    else
        retval = (*d)->WriteQuadlet (d, (*d)->GetDevice (d), &full_addr, values[0],
                                     false, cam->generation);
    if (retval != 0) {
        dc1394_log_error("Error writing (%x)...",retval);
        return DC1394_FAILURE;
    }
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_macosx_reset_bus (platform_camera_t * cam)
{
    IOFireWireLibDeviceRef d = cam->iface;

    if ((*d)->BusReset (d) == 0)
        return DC1394_SUCCESS;
    else
        return DC1394_FAILURE;
}

static dc1394error_t
dc1394_macosx_read_cycle_timer (platform_camera_t * cam,
        uint32_t * cycle_timer, uint64_t * local_time)
{
    IOFireWireLibDeviceRef d = cam->iface;
    struct timeval tv;

    if ((*d)->GetCycleTime (d, (UInt32 *) cycle_timer) != 0)
        return DC1394_FAILURE;

    gettimeofday (&tv, NULL);
    *local_time = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_macosx_camera_get_node(platform_camera_t *cam, uint32_t *node,
        uint32_t * generation)
{
    IOFireWireLibDeviceRef d = cam->iface;
    UInt32 gen;
    UInt16 nodeid;
    while (1) {
        IOReturn ret;
        if ((*d)->GetBusGeneration (d, &gen) != 0)
            return DC1394_FAILURE;
        ret = (*d)->GetRemoteNodeID (d, gen, &nodeid);
        if (ret == kIOFireWireBusReset)
            continue;
        else if (ret == kIOReturnSuccess)
            break;
        else
            return DC1394_FAILURE;
    }

    if (node)
        *node = nodeid & 0x3f;
    if (generation)
        *generation = gen;
    return DC1394_SUCCESS;
}

static platform_dispatch_t
macosx_dispatch = {
    .platform_new = dc1394_macosx_new,
    .platform_free = dc1394_macosx_free,

    .get_device_list = dc1394_macosx_get_device_list,
    .free_device_list = dc1394_macosx_free_device_list,
    .device_get_config_rom = dc1394_macosx_device_get_config_rom,

    .camera_new = dc1394_macosx_camera_new,
    .camera_free = dc1394_macosx_camera_free,
    .camera_set_parent = dc1394_macosx_camera_set_parent,

    .camera_read = dc1394_macosx_camera_read,
    .camera_write = dc1394_macosx_camera_write,

    .reset_bus = dc1394_macosx_reset_bus,
    .camera_print_info = dc1394_macosx_camera_print_info,
    .camera_get_node = dc1394_macosx_camera_get_node,
    .read_cycle_timer = dc1394_macosx_read_cycle_timer,

    .capture_setup = dc1394_macosx_capture_setup,
    .capture_stop = dc1394_macosx_capture_stop,
    .capture_dequeue = dc1394_macosx_capture_dequeue,
    .capture_enqueue = dc1394_macosx_capture_enqueue,
    .capture_get_fileno = dc1394_macosx_capture_get_fileno,
    .capture_is_frame_corrupt = dc1394_macosx_capture_is_frame_corrupt,
    .capture_set_callback = dc1394_macosx_capture_set_callback,
    .capture_schedule_with_runloop = dc1394_macosx_capture_schedule_with_runloop,
};

void
macosx_init(dc1394_t * d)
{
    register_platform (d, &macosx_dispatch, "macosx");
}
