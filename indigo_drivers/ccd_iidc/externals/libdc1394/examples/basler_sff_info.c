/*
 * 1394-Based Digital Camera Control Library
 * Basler Smart Feature Framework specific extensions
 * Copyright (C) 2006 Mikael Olenfalk, Tobii Technology AB, Stockholm Sweden
 *
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 * Version : 16/02/2005
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
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include <dc1394/dc1394.h>
#include <dc1394/vendor/basler.h>

int list_cameras (dc1394_t * d, dc1394camera_list_t * list)
{
    uint32_t i;
    dc1394bool_t sff_available;
    dc1394camera_t * camera;

    for (i = 0; i < list->num; i++) {
        sff_available = DC1394_FALSE;
        camera = dc1394_camera_new (d, list->ids[i].guid);
        dc1394_basler_sff_is_available (camera, &sff_available);

        printf ("%02d:0x%"PRIx64":%s:%s:%s\n", i, camera->guid,
                camera->vendor, camera->model, sff_available ? "SFF" : "NO SFF");
        dc1394_camera_free (camera);
    }
    return 0;
}

int print_usage ()
{
    printf ("usage: basler_sff_info [--list-cameras|[--guid GUID]]\n");
    return 1;
}

int main (int argc, char **argv)
{
    dc1394camera_t *camera = NULL;
    uint64_t guid = 0x0LL;
    dc1394_t * d;
    dc1394camera_list_t * list;
    dc1394error_t err;

    d = dc1394_new ();
    if (!d)
        return 1;
    err=dc1394_camera_enumerate (d, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras");

    if (list->num == 0) {
        dc1394_log_error("No cameras found");
        return 1;
    }

    /* parse arguments */
    if (argc == 2) {
        if (!strcmp (argv[1], "--list-cameras"))
            list_cameras (d, list);
        else
            print_usage();
        dc1394_camera_free_list (list);
        dc1394_free (d);
        return 0;
    } else if (argc == 3) {
        if (!strcmp (argv[1], "--guid")) {
            if (sscanf (argv[2], "0x%"PRIx64, &guid) == 1) {
            } else {
                dc1394_camera_free_list (list);
                dc1394_free (d);
                return print_usage();
            }
        }
    } else if (argc != 1) {
        dc1394_camera_free_list (list);
        dc1394_free (d);
        return print_usage();
    }

    if (guid == 0x0LL) {
        printf ("I: found %d camera%s, working with camera 0\n", list->num,
                list->num == 1 ? "" : "s");
        camera = dc1394_camera_new (d, list->ids[0].guid);
    } else {
        camera = dc1394_camera_new (d, guid);
        if (camera == NULL) {
            dc1394_log_error("no camera with guid 0x%"PRIx64" found", guid);
            return 1;
        }

        printf ("I: found camera with guid 0x%"PRIx64"\n", guid);
    }

    dc1394_camera_print_info (camera, stdout);
    printf ("\nSFF feature info:\n");
    dc1394_basler_sff_feature_print_all (camera, stdout);
    dc1394_camera_free (camera);
    dc1394_camera_free_list (list);
    dc1394_free (d);
    return 0;
}
