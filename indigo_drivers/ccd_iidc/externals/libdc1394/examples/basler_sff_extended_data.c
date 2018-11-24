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
#include <inttypes.h>
#include <string.h>

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
    printf ("usage: basler_sff_extended_data [--list-cameras|[--guid GUID]]\n");
    return 1;
}

int main (int argc, char **argv)
{
    dc1394error_t err;
    dc1394camera_t *camera = NULL;
    dc1394bool_t sff_available;
    uint32_t i, max_height, max_width;
    uint64_t guid = 0x0LL, total_bytes;
    uint8_t* buffer;
    dc1394basler_sff_t chunk;
    dc1394basler_sff_extended_data_stream_t* sff_ext;
    dc1394basler_sff_dcam_values_t* sff_dcam;

    dc1394_t * d;
    dc1394camera_list_t * list;

    d = dc1394_new ();
    if (!d)
        return 1;

    err=dc1394_camera_enumerate (d, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras\n");

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
    dc1394_camera_free_list (list);

    /*
     * setup format7 with a roi allocating a quarter of the screen and bounce around the roi, while changing gain and brightness
     */
    err = dc1394_video_set_mode (camera, DC1394_VIDEO_MODE_FORMAT7_0);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot choose format7_0");
    printf ("I: video mode is format7_0\n");

    err = dc1394_feature_set_value (camera, DC1394_FEATURE_GAIN, 100);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set gain");
    printf ("I: gain is 100\n");

    err = dc1394_feature_set_value (camera, DC1394_FEATURE_SHUTTER, 50);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set shutter");
    printf ("I: shutter is 50\n");

    err = dc1394_feature_set_value (camera, DC1394_FEATURE_BRIGHTNESS, 150);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set brightness");
    printf ("I: brightness is 150\n");

    err = dc1394_format7_get_max_image_size (camera, DC1394_VIDEO_MODE_FORMAT7_0, &max_width, &max_height);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot get max image size for format7_0");
    printf ("I: max image size is: height = %d, width = %d\n", max_height, max_width);

    err = dc1394_format7_set_roi (camera, DC1394_VIDEO_MODE_FORMAT7_0, DC1394_COLOR_CODING_MONO8,
                                  DC1394_USE_RECOMMENDED, 0, 0, max_width / 2, max_height / 2);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set roi");
    printf ("I: roi is (0, 0) - (%d, %d)\n", max_width / 2, max_height / 2);

    err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot get total bytes");
    printf ("I: total bytes is %"PRIu64" before SFF enabled\n", total_bytes);

    err = dc1394_basler_sff_is_available (camera, &sff_available);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot check if SFF is available");

    if (!sff_available) {
        dc1394_camera_free (camera);
        dc1394_log_error("SFF is not available");
        return 2;
    } else {
        printf ("I: SFF is available\n");
    }

    err = dc1394_basler_sff_feature_enable (camera, DC1394_BASLER_SFF_EXTENDED_DATA_STREAM, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera), "cannot enable Extended_Data_Stream");

    err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera), "cannot get total bytes");
    printf ("I: total bytes is %"PRIu64" after Extended_Data_Stream was enabled\n", total_bytes);

    err = dc1394_basler_sff_feature_enable (camera, DC1394_BASLER_SFF_DCAM_VALUES, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera), "cannot enable DCAM_Values");

    err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera), "cannot get total bytes");
    printf ("I: total bytes is %"PRIu64" after DCAM_Values was enabled\n", total_bytes);

    err = dc1394_capture_setup (camera, 10, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera), "cannot setup capture");

    err = dc1394_video_set_transmission (camera, DC1394_ON);
    if (err != DC1394_SUCCESS)
        dc1394_capture_stop (camera);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera), "cannot enable transmission");

    i = 0;
    dc1394video_frame_t* frame;
    while (i < 100) {
        err = dc1394_capture_dequeue (camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
        if (err != DC1394_SUCCESS) {
            dc1394_log_error("capture failed (%d)", err);
            break;
        }
        buffer = frame->image;

        /* parse chunks and print */
        // printf ("-- %02d - %d\n", i, dc1394_capture_get_bytes_per_frame(camera));
        dc1394_basler_sff_chunk_iterate_init (&chunk, buffer, total_bytes, DC1394_FALSE);
        while ((err = dc1394_basler_sff_chunk_iterate(&chunk)) == DC1394_SUCCESS) {
            switch (chunk.feature_id) {
            case DC1394_BASLER_SFF_EXTENDED_DATA_STREAM:
                {
                    sff_ext = (dc1394basler_sff_extended_data_stream_t*)(chunk.chunk_data);
                    printf ("top: %04d left: %04d height: %04d width: %04d\n",
                            sff_ext->top, sff_ext->left, sff_ext->height, sff_ext->width);
                    break;
                }
            case DC1394_BASLER_SFF_DCAM_VALUES:
                {
                    sff_dcam = (dc1394basler_sff_dcam_values_t*)(chunk.chunk_data);
                    printf ("gain: %04d brightness: %04d shutter: %04d\n",
                            sff_dcam->gain_csr.value, sff_dcam->brightness_csr.value, sff_dcam->shutter_csr.value);
                    break;
                }
            default:
                break;
            }
        }

        if (err != DC1394_BASLER_NO_MORE_SFF_CHUNKS) {
            dc1394_log_error("error parsing chunks");
            break;
        }

        err = dc1394_capture_enqueue(camera, frame);
        if (err != DC1394_SUCCESS) {
            dc1394_log_error("cannot release buffer");
            break;
        }

        err = dc1394_format7_set_image_position (camera, DC1394_VIDEO_MODE_FORMAT7_0, (((i+1) * 10) % max_height) / 2 - 1, (((i+1) * 10) % max_width) / 2 - 1);
        if (err != DC1394_SUCCESS) {
            dc1394_log_error("cannot set image position %d - %d", (((i+1) * 10) % max_height) / 2 - 1, (((i+1) * 10) % max_width) / 2 - 1);
        }
        i++;
    }

    dc1394_video_set_transmission (camera, DC1394_OFF);
    dc1394_capture_stop (camera);
    dc1394_camera_free (camera);
    dc1394_free (d);
    return 0;
}
