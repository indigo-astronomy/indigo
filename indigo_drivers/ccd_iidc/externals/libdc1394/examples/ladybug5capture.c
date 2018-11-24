/*
 * File: ladybug5capture.c
 *
 * Capture program prototype for the spherical 6-CCD Ladybug 5
 *   camera from Point Grey
 *
 * Written by Martin Peris <martin_peris@cyberdyne.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Notes:
 *
 * - Uses the first camera on the bus, then prints the camera information
 *  and sets up the camera in RAW8 mode (mode 7_0), capture NFRAMES
 *  frames to the HDD and prints the information of each frame.
 *
 * - In RAW8 mode the 6 images come in a big array of 2528x12484 pixels.
 *   Each individual image is supposed to be 2448x2048 but the camera 
 *   introduces some padding.
 *   
 * - Ladybug 5 sensors are arranged in "portrait" orientation to increase
 *   the vertical field of view. As a result, height measurements appear
 *   as width, and width measurements appear as height
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <dc1394/dc1394.h>

#define NFRAMES 10

void print_format7_info(dc1394format7modeset_t *info)
{
    for (int i = 0; i < DC1394_VIDEO_MODE_FORMAT7_NUM; i++) {
        if (info->mode[i].present) {
            printf("------ Camera format 7_%d information ------\n", i);
            printf("Size x                             :        %u\n", info->mode[i].size_x);
            printf("Size y                             :        %u\n", info->mode[i].size_y);
            printf("Max Size x                         :        %u\n", info->mode[i].max_size_x);
            printf("Max Size y                         :        %u\n", info->mode[i].max_size_y);
            printf("Pos x                              :        %u\n", info->mode[i].pos_x);
            printf("Pos y                              :        %u\n", info->mode[i].pos_y);
            printf("Unit Size x                        :        %u\n", info->mode[i].unit_size_x);
            printf("Unit Size y                        :        %u\n", info->mode[i].unit_size_y);
            printf("Unit Pos x                         :        %u\n", info->mode[i].unit_pos_x);
            printf("Unit Pos y                         :        %u\n", info->mode[i].unit_pos_y);
            printf("Color codings                      :        ");
            for (int j = 0; j < info->mode[i].color_codings.num; j++) {
                printf("%u%c ", info->mode[i].color_codings.codings[j], j == info->mode[i].color_codings.num-1 ? ' ':',');
            }
            printf("\n");
            printf("Color coding                       :        %u\n", info->mode[i].color_coding);
            printf("Pixnum                             :        %u\n", info->mode[i].pixnum);
            printf("Packet size                        :        %u\n", info->mode[i].packet_size);
            printf("Unit Packet size                   :        %u\n", info->mode[i].unit_packet_size);
            printf("Max Packet size                    :        %u\n", info->mode[i].max_packet_size);
        }
    }
}

void print_frame_info(dc1394video_frame_t *frame)
{
    printf("------ Frame information ------\n");
    printf("Size x                             :        %u\n", frame->size[0]);
    printf("Size y                             :        %u\n", frame->size[1]);
    printf("Pos x                              :        %u\n", frame->position[0]);
    printf("Pos y                              :        %u\n", frame->position[1]);
    printf("Color coding                       :        %u\n", frame->color_coding);
    printf("Color filter                       :        %u\n", frame->color_filter);
    printf("yuv byte order                     :        %u\n", frame->yuv_byte_order);
    printf("data depth                         :        %u\n", frame->data_depth);
    printf("stride                             :        %u\n", frame->stride);
    printf("video mode                         :        %d\n", frame->video_mode);
    printf("total bytes                        :        %lu\n", frame->total_bytes);
    printf("image bytes                        :        %u\n", frame->image_bytes);
    printf("padding bytes                      :        %u\n", frame->padding_bytes);
    printf("packet size                        :        %u\n", frame->packet_size);
    printf("packets per frame                  :        %u\n", frame->packets_per_frame);
    printf("timestamp                          :        %lu\n", frame->timestamp);
    printf("frames behind                      :        %u\n", frame->frames_behind);
    printf("id                                 :        %u\n", frame->id);
    printf("allocated image bytes              :        %lu\n", frame->allocated_image_bytes);
    printf("little emdian                      :        %s\n", frame->little_endian == DC1394_TRUE ? "Yes" : "No");
    printf("data in padding                    :        %s\n", frame->data_in_padding == DC1394_TRUE ? "Yes" : "No");
}

int main(int argc, const char * argv[]) {

    dc1394_t *d;
    dc1394camera_list_t *list;
    dc1394error_t err;
    dc1394camera_t *camera;
    dc1394format7modeset_t modeset;
    dc1394video_frame_t *frame;
    FILE* imagefile;
    char filename[256];
    int i = 0;

    d = dc1394_new();
    if (!d) {
        return 1;
    }

    err = dc1394_camera_enumerate(d, &list);
    DC1394_ERR_RTN(err, "Failed to enumerate cameras");
    if (list->num == 0) {
        dc1394_log_error("No cameras found");
        dc1394_free(d);
        return 1;
    }
    printf("Detected %d cameras\n", list->num);

    // Assume that Ladybug 5 is detected as camera #0
    camera = dc1394_camera_new(d, list->ids[0].guid);
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %llx", list->ids[0].guid);
        dc1394_free(d);
    }
    dc1394_camera_free_list(list);
    printf("Using camera %s %s\n", camera->vendor, camera->model);

    // Report camera info
    err = dc1394_camera_print_info(camera, stdout);
    DC1394_ERR_RTN(err, "Could not print camera info");


    // Setup video mode, etc...
    err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
    DC1394_ERR_RTN(err, "Could not set B mode");
    err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_MAX);
    DC1394_ERR_RTN(err, "Could not set max speed");
    err = dc1394_video_set_mode(camera, DC1394_VIDEO_MODE_FORMAT7_0);
    DC1394_ERR_RTN(err, "Could not set DC1394_VIDEO_MODE_FORMAT7_0");

    // Get format7 mode info
    err = dc1394_format7_get_modeset(camera, &modeset);
    DC1394_ERR_RTN(err, "Could not get format 7 mode info\n");
    print_format7_info(&modeset);


    // Set format 7 roi
    err = dc1394_format7_set_roi(camera,
                                 DC1394_VIDEO_MODE_FORMAT7_0,
                                 modeset.mode[0].color_coding,
                                 modeset.mode[0].max_packet_size,
                                 modeset.mode[0].pos_x,
                                 modeset.mode[0].pos_y,
                                 modeset.mode[0].max_size_x,
                                 modeset.mode[0].max_size_y);
    DC1394_ERR_RTN(err, "Could not set max ROI");

    // Set capture
    err = dc1394_capture_setup(camera, 10, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_RTN(err, "Could not setup capture");
    err = dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_RTN(err, "Could not start transmission");

    while (i < NFRAMES) {
        // Capture image
        printf("Capturing image %d\n", i);
        err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
        DC1394_ERR_RTN(err, "Could not dequeue a frame");
        
        // Do something with the image
        print_frame_info(frame);

        // Save the image
        sprintf(filename, "%05d.pgm",i);
        imagefile = fopen(filename, "wb");
        if ( imagefile == NULL ) {
            printf("Could not save image\n");
            continue;
        }
        fprintf(imagefile, "P5\n%u %u 255\n", frame->size[0], frame->size[1]);
        fwrite(frame->image, 1, frame->image_bytes, imagefile);
        fclose(imagefile);
        printf("Saved image %s\n", filename);

        err = dc1394_capture_enqueue(camera, frame);
        DC1394_ERR_RTN(err, "Could enqueue a frame");

        i++;
    }

    dc1394_camera_free(camera);
    dc1394_free(d);
    return 0;
}
