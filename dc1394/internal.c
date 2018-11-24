/*
 * 1394-Based Digital Camera Control Library
 *
 * Internal functions
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
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

#include "internal.h"
#include "utils.h"
#include "log.h"
#include "register.h"

/*
  These arrays define how many image quadlets there
  are in a packet given a mode and a frame rate
  This is defined in the 1394 digital camera spec
*/
const int quadlets_per_packet_format_0[56] = {
    -1,  -1,  15,  30,  60,  120,  240,  480,
    10,  20,  40,  80, 160,  320,  640, 1280,
    30,  60, 120, 240, 480,  960, 1920, 3840,
    40,  80, 160, 320, 640, 1280, 2560, 5120,
    60, 120, 240, 480, 960, 1920, 3840, 7680,
    20,  40,  80, 160, 320,  640, 1280, 2560,
    40,  80, 160, 320, 640, 1280, 2560, 5120
};

const int quadlets_per_packet_format_1[64] =  {
    -1, 125, 250,  500, 1000, 2000, 4000, 8000,
    -1,  -1, 375,  750, 1500, 3000, 6000,   -1,
    -1,  -1, 125,  250,  500, 1000, 2000, 4000,
    96, 192, 384,  768, 1536, 3072, 6144,   -1,
    144, 288, 576, 1152, 2304, 4608,   -1,   -1,
    48,  96, 192,  384,  768, 1536, 3073, 6144,
    -1, 125, 250,  500, 1000, 2000, 4000, 8000,
    96, 192, 384,  768, 1536, 3072, 6144,   -1
};

const int quadlets_per_packet_format_2[64] =  {
    160, 320,  640, 1280, 2560, 5120,   -1, -1,
    240, 480,  960, 1920, 3840, 7680,   -1, -1,
    80, 160,  320,  640, 1280, 2560, 5120, -1,
    250, 500, 1000, 2000, 4000, 8000,   -1, -1,
    375, 750, 1500, 3000, 6000,   -1,   -1, -1,
    125, 250,  500, 1000, 2000, 4000, 8000, -1,
    160, 320,  640, 1280, 2560, 5120,   -1, -1,
    250, 500, 1000, 2000, 4000, 8000,   -1, -1
};

/********************************************************
 get_quadlets_per_packet

 This routine reports the number of useful image quadlets
 per packet
*********************************************************/
dc1394error_t
get_quadlets_per_packet(dc1394video_mode_t mode, dc1394framerate_t frame_rate, uint32_t *qpp) // ERROR handling to be updated
{
    uint32_t mode_index;
    uint32_t frame_rate_index= frame_rate - DC1394_FRAMERATE_MIN;
    uint32_t format;
    dc1394error_t err;

    err=get_format_from_mode(mode, &format);
    DC1394_ERR_RTN(err,"Invalid mode ID");

    switch(format) {
    case DC1394_FORMAT0:
        mode_index= mode - DC1394_VIDEO_MODE_FORMAT0_MIN;

        if ( ((mode >= DC1394_VIDEO_MODE_FORMAT0_MIN) && (mode <= DC1394_VIDEO_MODE_FORMAT0_MAX)) &&
             ((frame_rate >= DC1394_FRAMERATE_MIN) && (frame_rate <= DC1394_FRAMERATE_MAX)) ) {
            *qpp=quadlets_per_packet_format_0[DC1394_FRAMERATE_NUM*mode_index+frame_rate_index];
        }
        else {
            err=DC1394_INVALID_VIDEO_MODE;
            DC1394_ERR_RTN(err,"Invalid framerate or mode");
        }
        return DC1394_SUCCESS;
    case DC1394_FORMAT1:
        mode_index= mode - DC1394_VIDEO_MODE_FORMAT1_MIN;

        if ( ((mode >= DC1394_VIDEO_MODE_FORMAT1_MIN) && (mode <= DC1394_VIDEO_MODE_FORMAT1_MAX)) &&
             ((frame_rate >= DC1394_FRAMERATE_MIN) && (frame_rate <= DC1394_FRAMERATE_MAX)) ) {
            *qpp=quadlets_per_packet_format_1[DC1394_FRAMERATE_NUM*mode_index+frame_rate_index];
        }
        else {
            err=DC1394_INVALID_VIDEO_MODE;
            DC1394_ERR_RTN(err,"Invalid framerate or mode");
        }
        return DC1394_SUCCESS;
    case DC1394_FORMAT2:
        mode_index= mode - DC1394_VIDEO_MODE_FORMAT2_MIN;

        if ( ((mode >= DC1394_VIDEO_MODE_FORMAT2_MIN) && (mode <= DC1394_VIDEO_MODE_FORMAT2_MAX)) &&
             ((frame_rate >= DC1394_FRAMERATE_MIN) && (frame_rate <= DC1394_FRAMERATE_MAX)) ) {
            *qpp=quadlets_per_packet_format_2[DC1394_FRAMERATE_NUM*mode_index+frame_rate_index];
        }
        else {
            err=DC1394_INVALID_VIDEO_MODE;
            DC1394_ERR_RTN(err,"Invalid framerate or mode");
        }
        return DC1394_SUCCESS;
    case DC1394_FORMAT6:
    case DC1394_FORMAT7:
        err=DC1394_INVALID_VIDEO_FORMAT;
        DC1394_ERR_RTN(err,"Format 6 and 7 don't have qpp");
        break;
    }

    return DC1394_FAILURE;
}

/**********************************************************
 get_quadlets_from_format

 This routine reports the number of quadlets that make up a
 frame given the format and mode
***********************************************************/
dc1394error_t
get_quadlets_from_format(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *quads)
{
    uint32_t w, h, color_coding;
    uint32_t bpp;
    dc1394error_t err;

    err=dc1394_get_image_size_from_video_mode(camera, video_mode, &w, &h);
    DC1394_ERR_RTN(err, "Invalid mode ID");

    err=dc1394_get_color_coding_from_video_mode(camera, video_mode, &color_coding);
    DC1394_ERR_RTN(err, "Invalid mode ID");

    err=dc1394_get_color_coding_bit_size(color_coding, &bpp);
    DC1394_ERR_RTN(err, "Invalid color mode ID");

    *quads=(w*h*bpp)/32;

    return err;
}

dc1394bool_t
is_feature_bit_set(uint32_t value, dc1394feature_t feature)
{

    if (feature >= DC1394_FEATURE_ZOOM) {
        if (feature >= DC1394_FEATURE_CAPTURE_SIZE) {
            feature+= 12;
        }
        feature-= DC1394_FEATURE_ZOOM;
    }
    else {
        feature-= DC1394_FEATURE_MIN;
    }

    value&=(0x80000000UL >> feature);

    if (value>0)
        return DC1394_TRUE;
    else
        return DC1394_FALSE;
}

dc1394error_t
get_format_from_mode(dc1394video_mode_t mode, uint32_t *format)
{
    dc1394error_t err=DC1394_SUCCESS;

    if ((mode>=DC1394_VIDEO_MODE_FORMAT0_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT0_MAX)) {
        *format=DC1394_FORMAT0;
    }
    else if ((mode>=DC1394_VIDEO_MODE_FORMAT1_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT1_MAX)) {
        *format=DC1394_FORMAT1;
    }
    else if ((mode>=DC1394_VIDEO_MODE_FORMAT2_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT2_MAX)) {
        *format=DC1394_FORMAT2;
    }
    else if ((mode>=DC1394_VIDEO_MODE_FORMAT6_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT6_MAX)) {
        *format=DC1394_FORMAT6;
    }
    else if ((mode>=DC1394_VIDEO_MODE_FORMAT7_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT7_MAX)) {
        *format=DC1394_FORMAT7;
    }
    else {
        err=DC1394_INVALID_VIDEO_MODE;
        DC1394_ERR_RTN(err, "The supplied mode does not correspond to any format");
    }

    return err;
}


dc1394error_t
capture_basic_setup (dc1394camera_t * camera, dc1394video_frame_t * frame)
{
    dc1394error_t err;
    uint32_t bpp;
    dc1394video_mode_t video_mode;
    dc1394framerate_t framerate;

    frame->camera = camera;

    err=dc1394_video_get_mode(camera,&video_mode);
    DC1394_ERR_RTN(err, "Unable to get current video mode");
    frame->video_mode = video_mode;

    err=dc1394_get_image_size_from_video_mode(camera, video_mode, frame->size, frame->size + 1);
    DC1394_ERR_RTN(err,"Could not get width/height from format/mode");

    if (dc1394_is_video_mode_scalable(video_mode)==DC1394_TRUE) {

        err=dc1394_format7_get_packet_size(camera, video_mode,
                                           &frame->packet_size);
        DC1394_ERR_RTN(err, "Unable to get format 7 bytes per packet");

        err=dc1394_format7_get_packets_per_frame(camera, video_mode,
                                                 &frame->packets_per_frame);
        DC1394_ERR_RTN(err, "Unable to get format 7 packets per frame");

        err = dc1394_format7_get_image_position (camera, video_mode,
                                                 frame->position, frame->position + 1);
        DC1394_ERR_RTN(err, "Unable to get format 7 image position");

        dc1394_format7_get_color_filter (camera, video_mode, &frame->color_filter);
    }
    else {
        err=dc1394_video_get_framerate(camera,&framerate);
        DC1394_ERR_RTN(err, "Unable to get current video framerate");

        err=get_quadlets_per_packet(video_mode, framerate, &frame->packet_size);
        DC1394_ERR_RTN(err, "Unable to get quadlets per packet");
        frame->packet_size *= 4;

        err= get_quadlets_from_format(camera, video_mode, &frame->packets_per_frame);
        DC1394_ERR_RTN(err,"Could not get quadlets per frame");
        frame->packets_per_frame /= frame->packet_size/4;

        frame->position[0] = 0;
        frame->position[1] = 0;
        frame->color_filter = 0;
    }

    dc1394_log_debug("Mode %d, %dx%d, packet size %d, "
            "packets per frame %d\n",
            frame->video_mode, frame->size[0], frame->size[1],
            frame->packet_size, frame->packets_per_frame);

    if ((frame->packet_size <=0 )||
        (frame->packets_per_frame <= 0)) {
        return DC1394_FAILURE;
    }

    frame->yuv_byte_order = DC1394_BYTE_ORDER_UYVY;

    frame->total_bytes = frame->packets_per_frame * frame->packet_size;

    err = dc1394_get_color_coding_from_video_mode (camera, video_mode, &frame->color_coding);
    DC1394_ERR_RTN(err, "Unable to get color coding");

    frame->data_depth=0; // to avoid valgrind warnings
    err = dc1394_video_get_data_depth (camera, &frame->data_depth);
    DC1394_ERR_RTN(err, "Unable to get data depth");

    err = dc1394_get_color_coding_bit_size (frame->color_coding, &bpp);
    DC1394_ERR_RTN(err, "Unable to get bytes per pixel");

    frame->stride = (bpp * frame->size[0])/8;
    frame->image_bytes = frame->size[1] * frame->stride;
    frame->padding_bytes = frame->total_bytes - frame->image_bytes;

    frame->little_endian=0;   // not used before 1.32 is out.
    frame->data_in_padding=0; // not used before 1.32 is out.

    return DC1394_SUCCESS;
}

