/*
 * 1394-Based Digital Camera Control Library
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
#include <inttypes.h>
#include <string.h>

#include <dc1394/control.h>
#include "internal.h"
#include "platform.h"
#include "log.h"

static void
destroy_camera_info (camera_info_t * info)
{
    free (info->vendor);
    free (info->model);
}

static int
add_camera (dc1394_t * d, camera_info_t * info)
{
    int n = d->num_cameras;
    dc1394_log_debug ("Adding camera %"PRIx64":%d %x:%x (%s:%s)",
            info->guid, info->unit, info->vendor_id, info->model_id,
            info->vendor, info->model);

    /* For now, exclude duplicate cameras caused by seeing the same camera
     * on different busses.  A better solution is to let the user choose
     * between these different versions of the same camera, which will require
     * a new API in the future. */
    int i;
    for (i = 0; i < n; i++) {
        if (d->cameras[i].guid == info->guid
                && d->cameras[i].unit == info->unit) {
            dc1394_log_debug ("Rejected camera %"PRIx64" as duplicate",
                    info->unit);
            destroy_camera_info (info);
            return 0;
        }
    }
    d->cameras = realloc (d->cameras, (n + 1) * sizeof (camera_info_t));
    memcpy (d->cameras + n, info, sizeof (camera_info_t));
    d->num_cameras = n + 1;
    return 0;
}

static char *
parse_leaf (uint32_t offset, uint32_t * quads, int num_quads)
{
    if (offset >= num_quads)
        return NULL;
    int num_entries = quads[offset] >> 16;
    if (offset + num_entries >= num_quads)
        return NULL;

    uint32_t * dquads = quads + offset + 1;
    char * str = malloc ((num_entries - 1) * 4 + 1);
    int i;
    for (i = 0; i < num_entries - 2; i++) {
        uint32_t q = dquads[i+2];
        str[4*i+0] = q >> 24;
        str[4*i+1] = (q >> 16) & 0xff;
        str[4*i+2] = (q >> 8) & 0xff;
        str[4*i+3] = q & 0xff;
    }
    str[4*i] = '\0';
    return str;
}

static int
identify_unit (dc1394_t * d, platform_info_t * platform,
        platform_device_t * dev, uint64_t guid,
        uint32_t offset, uint32_t * quads, int num_quads, int unit_num,
        uint32_t vendor_id)
{
    if (offset >= num_quads)
        return -1;
    int num_entries = quads[offset] >> 16;
    if (offset + num_entries >= num_quads)
        return -1;

    camera_info_t info;
    memset (&info, 0, sizeof (camera_info_t));

    info.guid = guid;
    info.unit = unit_num;
    info.device = dev;
    info.vendor_id = vendor_id;
    info.unit_directory = offset;
    info.platform = platform;

    uint32_t * dquads = quads + offset + 1;
    int i;
    for (i = 0; i < num_entries; i++) {
        uint32_t q = dquads[i];
        if ((q >> 24) == 0x12)
            info.unit_spec_ID = q & 0xffffff;
        if ((q >> 24) == 0x13)
            info.unit_sw_version = q & 0xffffff;
        if ((q >> 24) == 0xD4)
            info.unit_dependent_directory = (q & 0xffffff) + offset + i + 1;
        if ((q >> 24) == 0x17)
            info.model_id = q & 0xffffff;
    }

  /*
     Note on Point Grey (PG) cameras:
     Although not always advertised, PG cameras are 'sometimes' compatible
     with IIDC specs. This is especially the case with PG stereo products.
     The following modifications have been tested with a stereo head
     (BumbleBee). Most other cameras should be compatible, please consider
     contributing to the lib if your PG camera is not recognized.

     PG cams sometimes have a Unit_Spec_ID of 0xB09D, instead of the
     0xA02D of classic IIDC cameras. Also, their software revision differs.
     I could only get a 1.14 version from my BumbleBee but other versions
     might exist.

     As PG is regularly providing firmware updates you might also install
     the latest one in your camera for an increased compatibility.

     Damien

     (updated 2005-04-30)
  */

    if ((info.unit_spec_ID != 0xA02D) &&
            (info.unit_spec_ID != 0xB09D))
        return -1;
    if (!info.unit_dependent_directory)
        return -1;

    if (info.unit_dependent_directory >= num_quads)
        goto done;
    num_entries = quads[info.unit_dependent_directory] >> 16;
    if (info.unit_dependent_directory + num_entries >= num_quads)
        goto done;

    dquads = quads + info.unit_dependent_directory + 1;
    for (i = 0; i < num_entries; i++) {
        uint32_t q = dquads[i];
        if ((q >> 24) == 0x81)
            info.vendor = parse_leaf ((q & 0xffffff) +
                    info.unit_dependent_directory + 1 + i,
                    quads, num_quads);
        if ((q >> 24) == 0x82)
            info.model = parse_leaf ((q & 0xffffff) +
                    info.unit_dependent_directory + 1 + i,
                    quads, num_quads);
    }
done:
    info.unit_directory = info.unit_directory * 4 + 0x400;
    info.unit_dependent_directory = info.unit_dependent_directory * 4 + 0x400;
    return add_camera (d, &info);
}

static int
identify_camera (dc1394_t * d, platform_info_t * platform,
        platform_device_t * dev)
{
    uint64_t guid;
    uint32_t quads[256];
    int num_quads = 256;
    if (platform->dispatch->device_get_config_rom (dev, quads,
                &num_quads) < 0) {
        dc1394_log_warning ("Failed to get config ROM from %s device",
                platform->name);
        return -1;
    }

    dc1394_log_debug ("Got %d quads of config ROM", num_quads);

    if (num_quads < 7)
        return -1;

    /* Require 4 quadlets in the bus info block */
    if ((quads[0] >> 24) != 0x4) {
        dc1394_log_debug ("Expected 4 quadlets in bus info block, got %d",
                quads[0] >> 24);
        return -1;
    }

    /* Require "1394" as the bus identity */
    if (quads[1] != 0x31333934)
        return -1;

    guid = ((uint64_t)quads[3] << 32) | quads[4];

    int num_entries = quads[5] >> 16;
    if (num_quads < num_entries + 6)
        return -1;
    int unit = 0;
    uint32_t vendor_id = 0;
    int i;
    for (i = 0; i < num_entries; i++) {
        uint32_t q = quads[6+i];
        if ((q >> 24) == 0x03)
            vendor_id = q & 0xffffff;
        if ((q >> 24) == 0xD1) {
            uint32_t offset = (q & 0xffffff) + 6 + i;
            identify_unit (d, platform, dev, guid, offset, quads, num_quads,
                    unit++, vendor_id);
        }
    }
    return 0;
}

void
free_enumeration (dc1394_t * d)
{
    int i;
    for (i = 0; i < d->num_platforms; i++) {
        platform_info_t * p = d->platforms + i;
        if (p->device_list)
            p->dispatch->free_device_list (p->device_list);
        p->device_list = NULL;
    }

    for (i = 0; i < d->num_cameras; i++)
        destroy_camera_info (d->cameras + i);
    free (d->cameras);
    d->num_cameras = 0;
    d->cameras = NULL;
}

int
refresh_enumeration (dc1394_t * d)
{
    free_enumeration (d);

    dc1394_log_debug ("Enumerating cameras...");
    int i;
    for (i = 0; i < d->num_platforms; i++) {
        platform_info_t * p = d->platforms + i;
        if (!p->p)
            continue;
        dc1394_log_debug("Enumerating platform %s", p->name);
        p->device_list = p->dispatch->get_device_list (p->p);
        if (!p->device_list) {
            dc1394_log_warning("Platform %s failed to get device list",
                    p->name);
            continue;
        }

        platform_device_t ** list = p->device_list->devices;
        int j;
        dc1394_log_debug ("Platform %s has %d device(s)",
                p->name, p->device_list->num_devices);
        for (j = 0; j < p->device_list->num_devices; j++)
            if (identify_camera (d, p, list[j]) < 0)
                dc1394_log_debug ("Failed to identify %s device %d",
                        p->name, j);
    }

    return 0;
}

dc1394error_t
dc1394_camera_enumerate (dc1394_t * d, dc1394camera_list_t **list)
{
    if (refresh_enumeration (d) < 0)
        return DC1394_FAILURE;

    dc1394camera_list_t * l;

    l = calloc (1, sizeof (dc1394camera_list_t));
    *list = l;
    if (d->num_cameras == 0)
        return DC1394_SUCCESS;

    l->ids = malloc (d->num_cameras * sizeof (dc1394camera_id_t));
    l->num = 0;

    int i;
    for (i = 0; i < d->num_cameras; i++) {
        l->ids[i].guid = d->cameras[i].guid;
        l->ids[i].unit = d->cameras[i].unit;
        l->num++;
    }
    return DC1394_SUCCESS;
}

/*
  Free a list of cameras returned by dc1394_enumerate_cameras()
 */
void
dc1394_camera_free_list (dc1394camera_list_t *list)
{
    if (list)
        free (list->ids);
    list->ids = NULL;
    free (list);
}

