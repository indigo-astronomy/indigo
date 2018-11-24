/*
 * 1394-Based Digital Camera Control Library
 *
 * Utilities
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

const char *dc1394_feature_desc[DC1394_FEATURE_NUM] = {
    "Brightness",
    "Exposure",
    "Sharpness",
    "White Balance",
    "Hue",
    "Saturation",
    "Gamma",
    "Shutter",
    "Gain",
    "Iris",
    "Focus",
    "Temperature",
    "Trigger",
    "Trigger Delay",
    "White Shading",
    "Frame Rate",
    "Zoom",
    "Pan",
    "Tilt",
    "Optical Filter",
    "Capture Size",
    "Capture Quality"
};

const char *dc1394_error_strings[DC1394_ERROR_NUM] = {
    "Success",
    "Generic failure",
    "This node is not a camera",
    "Function not supported by this camera",
    "Camera not initialized",
    "Memory allocation failure",
    "Tagged register not found",
    "Could not allocate an ISO channel",
    "Could not allocate bandwidth",
    "IOCTL failure",
    "Capture is not set",
    "Capture is running",
    "RAW1394 failure",
    "Format_7 Error_flag_1 is set",
    "Format_7 Error_flag_2 is set",
    "Invalid argument value",
    "Requested value is out of range",
    "Invalid feature",
    "Invalid video format",
    "Invalid video mode",
    "Invalid framerate",
    "Invalid trigger mode",
    "Invalid trigger source",
    "Invalid ISO speed",
    "Invalid IIDC version",
    "Invalid Format_7 color coding",
    "Invalid Format_7 elementary Bayer tile",
    "Invalid capture mode",
    "Invalid error code",
    "Invalid Bayer method",
    "Invalid video1394 device",
    "Invalid operation mode",
    "Invalid trigger polarity",
    "Invalid feature mode",
    "Invalid log type",
    "Invalid byte order",
    "Invalid stereo method",
    "Basler error: no more SFF chunks",
    "Basler error: corrupted SFF chunk",
    "Basler error: unknown SFF chunk"
};

dc1394error_t
dc1394_get_image_size_from_video_mode(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *w, uint32_t *h)
{
    dc1394error_t err;
    uint32_t sx, sy;

    switch(video_mode) {
    case DC1394_VIDEO_MODE_160x120_YUV444:
        *w = 160;*h=120;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_320x240_YUV422:
        *w = 320;*h=240;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_640x480_YUV411:
    case DC1394_VIDEO_MODE_640x480_YUV422:
    case DC1394_VIDEO_MODE_640x480_RGB8:
    case DC1394_VIDEO_MODE_640x480_MONO8:
    case DC1394_VIDEO_MODE_640x480_MONO16:
        *w =640;*h=480;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_800x600_YUV422:
    case DC1394_VIDEO_MODE_800x600_RGB8:
    case DC1394_VIDEO_MODE_800x600_MONO8:
    case DC1394_VIDEO_MODE_800x600_MONO16:
        *w=800;*h=600;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_1024x768_YUV422:
    case DC1394_VIDEO_MODE_1024x768_RGB8:
    case DC1394_VIDEO_MODE_1024x768_MONO8:
    case DC1394_VIDEO_MODE_1024x768_MONO16:
        *w=1024;*h=768;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_1280x960_YUV422:
    case DC1394_VIDEO_MODE_1280x960_RGB8:
    case DC1394_VIDEO_MODE_1280x960_MONO8:
    case DC1394_VIDEO_MODE_1280x960_MONO16:
        *w=1280;*h=960;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_1600x1200_YUV422:
    case DC1394_VIDEO_MODE_1600x1200_RGB8:
    case DC1394_VIDEO_MODE_1600x1200_MONO8:
    case DC1394_VIDEO_MODE_1600x1200_MONO16:
        *w=1600;*h=1200;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_FORMAT7_0:
    case DC1394_VIDEO_MODE_FORMAT7_1:
    case DC1394_VIDEO_MODE_FORMAT7_2:
    case DC1394_VIDEO_MODE_FORMAT7_3:
    case DC1394_VIDEO_MODE_FORMAT7_4:
    case DC1394_VIDEO_MODE_FORMAT7_5:
    case DC1394_VIDEO_MODE_FORMAT7_6:
    case DC1394_VIDEO_MODE_FORMAT7_7:
        // get the current color mode from the camera
        err=dc1394_format7_get_image_size(camera, video_mode, &sx, &sy);
        if (err!=DC1394_SUCCESS) {
            return err;
        }
        else {
            *w=sx;
            *h=sy;
        }
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_EXIF:
        return DC1394_FAILURE;
    }

    return DC1394_FAILURE;
}


dc1394error_t
dc1394_framerate_as_float(dc1394framerate_t framerate_enum, float *framerate)
{
    switch(framerate_enum)  {
    case DC1394_FRAMERATE_1_875:
        *framerate=1.875;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_3_75:
        *framerate=3.75;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_7_5:
        *framerate=7.5;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_15:
        *framerate=15.0;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_30:
        *framerate=30.0;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_60:
        *framerate=60.0;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_120:
        *framerate=120.0;
        return DC1394_SUCCESS;
    case DC1394_FRAMERATE_240:
        *framerate=240.0;
        return DC1394_SUCCESS;
    }
    return DC1394_INVALID_FRAMERATE;
}

dc1394error_t
dc1394_is_color(dc1394color_coding_t color_coding, dc1394bool_t *is_color)
{
    switch(color_coding)  {
    case DC1394_COLOR_CODING_MONO8:
    case DC1394_COLOR_CODING_MONO16:
    case DC1394_COLOR_CODING_MONO16S:
    case DC1394_COLOR_CODING_RAW8:
    case DC1394_COLOR_CODING_RAW16:
        *is_color=DC1394_FALSE;
        return DC1394_SUCCESS;
    case DC1394_COLOR_CODING_YUV411:
    case DC1394_COLOR_CODING_YUV422:
    case DC1394_COLOR_CODING_YUV444:
    case DC1394_COLOR_CODING_RGB8:
    case DC1394_COLOR_CODING_RGB16:
    case DC1394_COLOR_CODING_RGB16S:
        *is_color=DC1394_TRUE;
        return DC1394_SUCCESS;
    }
    return DC1394_INVALID_COLOR_CODING;
}

dc1394error_t
dc1394_get_color_coding_data_depth(dc1394color_coding_t color_coding, uint32_t * bits)
{
    switch(color_coding) {
    case DC1394_COLOR_CODING_MONO8:
    case DC1394_COLOR_CODING_YUV411:
    case DC1394_COLOR_CODING_YUV422:
    case DC1394_COLOR_CODING_YUV444:
    case DC1394_COLOR_CODING_RGB8:
    case DC1394_COLOR_CODING_RAW8:
        *bits = 8;
        return DC1394_SUCCESS;
    case DC1394_COLOR_CODING_MONO16:
    case DC1394_COLOR_CODING_RGB16:
    case DC1394_COLOR_CODING_MONO16S:
    case DC1394_COLOR_CODING_RGB16S:
    case DC1394_COLOR_CODING_RAW16:
        // shoudn't we return the real bit depth (e.g. 12) instead of systematically 16?
        *bits = 16;
        return DC1394_SUCCESS;
    }
    return DC1394_INVALID_COLOR_CODING;
}

dc1394error_t
dc1394_get_color_coding_bit_size(dc1394color_coding_t color_coding, uint32_t* bits)
{
    switch(color_coding) {
    case DC1394_COLOR_CODING_MONO8:
    case DC1394_COLOR_CODING_RAW8:
        *bits=8;
        return DC1394_SUCCESS;
    case DC1394_COLOR_CODING_YUV411:
        *bits=12;
        return DC1394_SUCCESS;
    case DC1394_COLOR_CODING_MONO16:
    case DC1394_COLOR_CODING_RAW16:
    case DC1394_COLOR_CODING_MONO16S:
    case DC1394_COLOR_CODING_YUV422:
        *bits=16;
        return DC1394_SUCCESS;
    case DC1394_COLOR_CODING_YUV444:
    case DC1394_COLOR_CODING_RGB8:
        *bits=24;
        return DC1394_SUCCESS;
    case DC1394_COLOR_CODING_RGB16:
    case DC1394_COLOR_CODING_RGB16S:
        *bits=48;
        return DC1394_SUCCESS;
    }
    return DC1394_INVALID_COLOR_CODING;
}

dc1394error_t
dc1394_get_color_coding_from_video_mode(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding)
{
    dc1394error_t err;
    dc1394color_coding_t id;

    switch(video_mode) {
    case DC1394_VIDEO_MODE_160x120_YUV444:
        *color_coding=DC1394_COLOR_CODING_YUV444;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_320x240_YUV422:
    case DC1394_VIDEO_MODE_640x480_YUV422:
    case DC1394_VIDEO_MODE_800x600_YUV422:
    case DC1394_VIDEO_MODE_1024x768_YUV422:
    case DC1394_VIDEO_MODE_1280x960_YUV422:
    case DC1394_VIDEO_MODE_1600x1200_YUV422:
        *color_coding=DC1394_COLOR_CODING_YUV422;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_640x480_YUV411:
        *color_coding=DC1394_COLOR_CODING_YUV411;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_640x480_RGB8:
    case DC1394_VIDEO_MODE_800x600_RGB8:
    case DC1394_VIDEO_MODE_1024x768_RGB8:
    case DC1394_VIDEO_MODE_1280x960_RGB8:
    case DC1394_VIDEO_MODE_1600x1200_RGB8:
        *color_coding=DC1394_COLOR_CODING_RGB8;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_640x480_MONO8:
    case DC1394_VIDEO_MODE_800x600_MONO8:
    case DC1394_VIDEO_MODE_1024x768_MONO8:
    case DC1394_VIDEO_MODE_1280x960_MONO8:
    case DC1394_VIDEO_MODE_1600x1200_MONO8:
        *color_coding=DC1394_COLOR_CODING_MONO8;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_800x600_MONO16:
    case DC1394_VIDEO_MODE_640x480_MONO16:
    case DC1394_VIDEO_MODE_1024x768_MONO16:
    case DC1394_VIDEO_MODE_1280x960_MONO16:
    case DC1394_VIDEO_MODE_1600x1200_MONO16:
        *color_coding=DC1394_COLOR_CODING_MONO16;
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_FORMAT7_0:
    case DC1394_VIDEO_MODE_FORMAT7_1:
    case DC1394_VIDEO_MODE_FORMAT7_2:
    case DC1394_VIDEO_MODE_FORMAT7_3:
    case DC1394_VIDEO_MODE_FORMAT7_4:
    case DC1394_VIDEO_MODE_FORMAT7_5:
    case DC1394_VIDEO_MODE_FORMAT7_6:
    case DC1394_VIDEO_MODE_FORMAT7_7:
        // get the current color mode from the camera
        err=dc1394_format7_get_color_coding(camera, video_mode, &id);
        if (err!=DC1394_SUCCESS) {
            return err;
        }
        else {
            *color_coding=id;
        }
        return DC1394_SUCCESS;
    case DC1394_VIDEO_MODE_EXIF:
        return DC1394_FAILURE;
    }

    return DC1394_FAILURE;
}

dc1394bool_t
dc1394_is_video_mode_scalable(dc1394video_mode_t video_mode)
{
    return ((video_mode>=DC1394_VIDEO_MODE_FORMAT7_MIN)&&(video_mode<=DC1394_VIDEO_MODE_FORMAT7_MAX));
}

dc1394bool_t
dc1394_is_video_mode_still_image(dc1394video_mode_t video_mode)
{
    return ((video_mode>=DC1394_VIDEO_MODE_FORMAT6_MIN)&&(video_mode<=DC1394_VIDEO_MODE_FORMAT6_MAX));
}

dc1394bool_t
dc1394_is_same_camera(dc1394camera_id_t id1, dc1394camera_id_t id2)
{
    return ((id1.guid==id2.guid)&&(id1.unit==id2.unit));
}

const char *
dc1394_feature_get_string(dc1394feature_t feature)
{
    if ((feature>DC1394_FEATURE_MAX)||(feature<DC1394_FEATURE_MIN))
        return NULL;

    return dc1394_feature_desc[feature-DC1394_FEATURE_MIN];
}

const char *
dc1394_error_get_string(dc1394error_t error)
{

    if ((error>DC1394_ERROR_MAX)||(error<DC1394_ERROR_MIN))
        return NULL;

    return dc1394_error_strings[-error-DC1394_ERROR_MAX];
}


/*
 * Checksum algorithms
 * Copyright (C) 2006 Mikael Olenfalk, Tobii Technology AB, Stockholm Sweden
 *
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 * Version : 16/02/2005
 */

uint16_t
dc1394_checksum_crc16 (const uint8_t* buffer, uint32_t buffer_size)
{
    uint32_t i, j, c, bit;
    uint32_t crc = 0;
    for (i = 0; i < buffer_size; i++) {
        c = (uint32_t)*buffer++;
        for (j = 0x80; j; j >>= 1) {
            bit = crc & 0x8000;
            crc <<= 1;
            if (c & j) bit ^= 0x8000;
            if (bit) crc ^= 0x1021;
        }
    }
    return (uint16_t)(crc & 0xffff);
}
