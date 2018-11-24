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

#include "config.h"
#include "platform.h"
#include "internal.h"
#include "usb.h"


static platform_t *
dc1394_usb_new (void)
{
    libusb_context *context;
    if (libusb_init(&context) != 0)
        return NULL;

    platform_t * p = calloc (1, sizeof (platform_t));
    p->context = context;
    return p;
}
static void
dc1394_usb_free (platform_t * p)
{
    if (p->context)
        libusb_exit(p->context);
    p->context = NULL;
    free (p);
}

struct _platform_device_t {
    libusb_device * dev;
};

typedef struct _usb_device_list_t {
    platform_device_list_t list;
    libusb_device ** libusb_list;
} usb_device_list_t;

/* This is the list of USB vendor/products that we know support
 * IIDC-over-USB.  Currently, this is the only mechanism of detecting
 * such cameras. */
static struct _vendor_product_t {
    uint16_t vendor;
    uint16_t product;
} usb_products[] = {
    { 0x1e10, 0x2000 }, // Point Grey Firefly MV Color
    { 0x1e10, 0x2001 }, // Point Grey Firefly MV Mono
    { 0x1e10, 0x2002 }, // Point Grey High Res Firefly MV Color
    { 0x1e10, 0x2003 }, // Point Grey High Res Firefly MV Mono
    { 0x1e10, 0x2004 }, // Point Grey Chameleon Color
    { 0x1e10, 0x2005 }, // Point Grey Chameleon Mono
    { 0x1e10, 0x3000 }, // Point Grey Flea 3
    { 0x1e10, 0x3005 }, // Point Grey Flea 3 (FL3-U3-13Y3M)
    { 0x1e10, 0x3006 }, // Point Grey Flea 3 (FL3-U3-13S2C)
    { 0x1e10, 0x3008 }, // Point Grey Flea 3 (FL3-U3-88S2C)
    { 0x1e10, 0x300a }, // Point Grey Flea 3 (FL3-U3-13E4C)
    { 0x1e10, 0x300b }, // Point Grey Flea 3 (FL3-U3-13E4M with 1.43.3.2 FW)
    { 0x1e10, 0x3103 }, // Point Grey Grasshopper 3 (GS3-U3-23S6M)
    { 0x1e10, 0x3300 }, // Point Grey Flea 3 (FL3-U3-13E4M with 2.7.3.0 FW)
	{ 0x1e10, 0x3800 }, // Point Grey Ladybug 5 (LD5-U3-51S5C-44)
    { 0, 0 }
};

static int
is_device_iidc (uint16_t vendor, uint16_t product)
{
    int j = 0;
    while (usb_products[j].vendor != 0 && usb_products[j].product != 0) {
        if (usb_products[j].vendor == vendor &&
                usb_products[j].product == product)
            return 1;
        j++;
    }
    return 0;
}

static platform_device_list_t *
dc1394_usb_get_device_list (platform_t * p)
{
    usb_device_list_t * list;
    libusb_device * dev;
    int i;

    list = calloc (1, sizeof (usb_device_list_t));
    if (!list)
        return NULL;

    if (libusb_get_device_list (p->context, &list->libusb_list) < 0)
        return NULL;

    dev = list->libusb_list[0];
    for (i=0, dev = list->libusb_list[0]; dev; dev = list->libusb_list[++i]) {
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor (dev, &desc) != 0) {
            dc1394_log_warning ("usb: Failed to get descriptor for device %d",
                    i);
            continue;
        }

        if (!is_device_iidc (desc.idVendor, desc.idProduct))
            continue;

        list->list.num_devices++;
        list->list.devices = realloc (list->list.devices,
                list->list.num_devices * sizeof (platform_device_t *));
        platform_device_t * pdev = malloc(sizeof(platform_device_t));
        pdev->dev = dev;
        list->list.devices[list->list.num_devices-1] = pdev;

        dc1394_log_debug ("usb: Found vendor:prod %x:%x at address %x:%x",
                desc.idVendor, desc.idProduct,
                libusb_get_bus_number (dev), libusb_get_device_address (dev));
    }

    return &list->list;
}

static void
dc1394_usb_free_device_list (platform_device_list_t * d)
{
    usb_device_list_t * list = (usb_device_list_t *) d;
    int i;
    for (i = 0; i < d->num_devices; i++)
        free (d->devices[i]);
    free (d->devices);
    libusb_free_device_list (list->libusb_list, 1);
    free (d);
}

/* The high 16 bits of the IEEE 1394 address space are mapped to the request
 * byte of USB control transfers.  Only a discrete set addresses are
 * currently supported, as mapped by this function. */
static int
address_to_request (uint64_t address)
{
    switch (address >> 32) {
        case 0xffff:
            return 0x7f;
        case 0xd000:
            return 0x80;
        case 0xd0001:
            return 0x81;
    }
    dc1394_log_error("usb: Invalid high address %x for request",
            address >> 32);
    return -1;
}

#define REQUEST_TIMEOUT_MS  1000

static int
do_read (libusb_device_handle * handle, uint64_t address, uint32_t * quads,
        int num_quads)
{
    int request = address_to_request (address);
    if (request < 0)
        return -1;

    unsigned char buf[num_quads*4];

    /* IEEE 1394 address reads are mapped to USB control transfers as
     * shown here. */
    int ret = libusb_control_transfer (handle, 0xc0, request,
            address & 0xffff, (address >> 16) & 0xffff,
            buf, num_quads * 4, REQUEST_TIMEOUT_MS);
    if (ret < 0)
        return -1;
    int i;
    int ret_quads = (ret + 3) / 4;
    /* Convert from little-endian to host-endian */
    for (i = 0; i < ret_quads; i++) {
        quads[i] = (buf[4*i+3] << 24) | (buf[4*i+2] << 16)
            | (buf[4*i+1] << 8) | buf[4*i];
    }
    return ret_quads;
}

static int
do_write (libusb_device_handle * handle, uint64_t address,
        const uint32_t * quads, int num_quads)
{
    int request = address_to_request (address);
    if (request < 0)
        return -1;

    unsigned char buf[num_quads*4];
    int i;
    /* Convert from host-endian to little-endian */
    for (i = 0; i < num_quads; i++) {
        buf[4*i]   = quads[i] & 0xff;
        buf[4*i+1] = (quads[i] >> 8) & 0xff;
        buf[4*i+2] = (quads[i] >> 16) & 0xff;
        buf[4*i+3] = (quads[i] >> 24) & 0xff;
    }
    /* IEEE 1394 address writes are mapped to USB control transfers as
     * shown here. */
    int ret = libusb_control_transfer (handle, 0x40, request,
            address & 0xffff, (address >> 16) & 0xffff,
            buf, num_quads * 4, REQUEST_TIMEOUT_MS);
    if (ret < 0)
        return -1;
    return ret / 4;
}

static int
dc1394_usb_device_get_config_rom (platform_device_t * device,
                                uint32_t * quads, int * num_quads)
{
    libusb_device_handle * handle;
    if (libusb_open (device->dev, &handle) < 0) {
        dc1394_log_warning ("usb: Failed to open device for config ROM");
        return DC1394_FAILURE;
    }

    if (*num_quads > 256)
        *num_quads = 256;

    /* Read the config ROM one quad at a time because a read longer than
     * the length of the ROM will fail. */
    int i;
    for (i = 0; i < *num_quads; i++) {
        int ret = do_read (handle, CONFIG_ROM_BASE + 0x400 + 4*i,
                quads + i, 1);
        if (ret < 1)
            break;
    }

    if (i == 0) {
        dc1394_log_error ("usb: Failed to read config ROM");
        libusb_close (handle);
        return -1;
    }

    *num_quads = i;
    libusb_close (handle);
    return 0;
}

static platform_camera_t *
dc1394_usb_camera_new (platform_t * p, platform_device_t * device,
        uint32_t unit_directory_offset)
{
    libusb_device_handle * handle;
    platform_camera_t * camera;

    if (libusb_open (device->dev, &handle) < 0) {
        dc1394_log_error ("usb: Failed to open device");
        return NULL;
    }

    if (libusb_set_configuration (handle, 1) < 0) {
        dc1394_log_error ("usb: Failed to set configuration 1 after open");
        libusb_close (handle);
        return NULL;
    }

    camera = calloc (1, sizeof (platform_camera_t));
    camera->handle = handle;
    return camera;
}

static void dc1394_usb_camera_free (platform_camera_t * cam)
{
    libusb_close (cam->handle);
    cam->handle = NULL;
    free (cam);
}

static void
dc1394_usb_camera_set_parent (platform_camera_t * cam, dc1394camera_t * parent)
{
    cam->camera = parent;
}

static dc1394error_t
dc1394_usb_camera_print_info (platform_camera_t * camera, FILE *fd)
{
    libusb_device *dev = libusb_get_device (camera->handle);
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor (dev, &desc);
    fprintf(fd,"------ Camera platform-specific information ------\n");
    fprintf(fd,"USB Bus Number                    :     %d\n",
            libusb_get_bus_number (dev));
    fprintf(fd,"USB Device Address                :     %d\n",
            libusb_get_device_address (dev));
    fprintf(fd,"Vendor ID                         :     0x%x\n",
            desc.idVendor);
    fprintf(fd,"Product ID                        :     0x%x\n",
            desc.idProduct);
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_usb_camera_read (platform_camera_t * cam, uint64_t offset,
        uint32_t * quads, int num_quads)
{
    if (do_read (cam->handle, CONFIG_ROM_BASE + offset, quads,
                num_quads) != num_quads)
        return DC1394_FAILURE;

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_usb_camera_write (platform_camera_t * cam, uint64_t offset,
        const uint32_t * quads, int num_quads)
{
    if (do_write (cam->handle, CONFIG_ROM_BASE + offset, quads,
                num_quads) != num_quads)
        return DC1394_FAILURE;

    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_usb_camera_get_node(platform_camera_t *cam, uint32_t *node,
        uint32_t * generation)
{
    /* Since node/generation doesn't really apply to USB, we instead
     * put the device address in the "node" argument and "bus number"
     * in the generation argument. */
    if (node)
        *node = libusb_get_device_address (libusb_get_device (cam->handle));
    if (generation)
        *generation = libusb_get_bus_number (libusb_get_device (cam->handle));
    return DC1394_SUCCESS;
}

static dc1394error_t
dc1394_usb_reset_bus (platform_camera_t * cam)
{
    int ret = libusb_reset_device (cam->handle);
    if (ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND)
        return DC1394_SUCCESS;

    return DC1394_FAILURE;
}

static platform_dispatch_t
usb_dispatch = {
    .platform_new = dc1394_usb_new,
    .platform_free = dc1394_usb_free,

    .get_device_list = dc1394_usb_get_device_list,
    .free_device_list = dc1394_usb_free_device_list,
    .device_get_config_rom = dc1394_usb_device_get_config_rom,

    .camera_new = dc1394_usb_camera_new,
    .camera_free = dc1394_usb_camera_free,
    .camera_set_parent = dc1394_usb_camera_set_parent,

    .camera_print_info = dc1394_usb_camera_print_info,
    .camera_get_node = dc1394_usb_camera_get_node,
    .reset_bus = dc1394_usb_reset_bus,

    .camera_read = dc1394_usb_camera_read,
    .camera_write = dc1394_usb_camera_write,

    .capture_setup = dc1394_usb_capture_setup,
    .capture_stop = dc1394_usb_capture_stop,
    .capture_dequeue = dc1394_usb_capture_dequeue,
    .capture_enqueue = dc1394_usb_capture_enqueue,
    .capture_get_fileno = dc1394_usb_capture_get_fileno,
    .capture_is_frame_corrupt = dc1394_usb_capture_is_frame_corrupt,

#ifdef HAVE_MACOSX
    .capture_set_callback = dc1394_usb_capture_set_callback,
    .capture_schedule_with_runloop = dc1394_usb_capture_schedule_with_runloop,
#else
    .capture_set_callback = NULL,
    .capture_schedule_with_runloop = NULL,
#endif
};

void
dc1394_usb_init(dc1394_t * d)
{
    register_platform (d, &usb_dispatch, "usb");
}
