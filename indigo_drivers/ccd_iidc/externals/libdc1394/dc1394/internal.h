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

#ifndef __DC1394_INTERNAL_H__
#define __DC1394_INTERNAL_H__

#include "config.h"
#include "offsets.h"
#include "platform.h"

typedef struct _platform_info_t {
    const platform_dispatch_t * dispatch;
    const char * name;
    platform_device_list_t * device_list;
    platform_t * p;
} platform_info_t;

typedef struct _dc1394camera_priv_t {
    dc1394camera_t camera;

    platform_camera_t * pcam;
    platform_info_t * platform;

    uint64_t allocated_channels;
    int allocated_bandwidth;
    int iso_persist;
} dc1394camera_priv_t;

#define DC1394_CAMERA_PRIV(c) ((dc1394camera_priv_t *)c)

typedef struct _camera_info_t {
    uint64_t guid;
    int unit;
    uint32_t unit_directory;
    uint32_t unit_dependent_directory;
    uint32_t unit_spec_ID;
    uint32_t unit_sw_version;
    char * vendor;
    char * model;
    uint32_t vendor_id;
    uint32_t model_id;
    platform_device_t * device;
    platform_info_t * platform;
} camera_info_t;

struct __dc1394_t {
    int num_platforms;
    platform_info_t * platforms;

    int num_cameras;
    camera_info_t * cameras;
};

void juju_init(dc1394_t *d);
void linux_init(dc1394_t *d);
void macosx_init(dc1394_t *d);
void windows_init(dc1394_t *d);
void dc1394_usb_init(dc1394_t *d);

void register_platform (dc1394_t * d, const platform_dispatch_t * dispatch,
        const char * name);

void free_enumeration (dc1394_t * d);
int refresh_enumeration (dc1394_t * d);

/* Definitions which application developers shouldn't care about */
#define CONFIG_ROM_BASE             0xFFFFF0000000ULL

#define DC1394_FEATURE_ON           0x80000000UL
#define DC1394_FEATURE_OFF          0x00000000UL

/* Maximum number of write/read retries */
#define DC1394_MAX_RETRIES          20

/* Maximum number of ISO channels */
/* Note that the maximum currently supported by a chipset is 8 so that 16 is already
   a conservative number. A typical number of channels supported is 4. (TI chipset)
   However, 1394b allows for more channels, hence we use 64 as the limit */
#define DC1394_NUM_ISO_CHANNELS     64

/* A hard compiled factor that makes sure async read and writes don't happen
   too fast */
/* Toshiyuki Umeda: use randomize timings to avoid locking indefinitely.
   If several thread are used the seed will be the same and the wait will therefor be
   identical. A call to srand(getpid()) should be performed after calling the
   raw1394_new_handle but I (Damien) not sure this would solve the problem as the handle
   creation might have happened in the creating (as opposed to created) thread.*/
#define DC1394_SLOW_DOWN            ((rand()%20)+10)

/* Maximum number of characters in vendor and model strings */
#define MAX_CHARS                      256


// Format_0
#define DC1394_VIDEO_MODE_FORMAT0_MIN            DC1394_VIDEO_MODE_160x120_YUV444
#define DC1394_VIDEO_MODE_FORMAT0_MAX            DC1394_VIDEO_MODE_640x480_MONO16
#define DC1394_VIDEO_MODE_FORMAT0_NUM      (DC1394_VIDEO_MODE_FORMAT0_MAX - DC1394_VIDEO_MODE_FORMAT0_MIN + 1)

// Format_1
#define DC1394_VIDEO_MODE_FORMAT1_MIN            DC1394_VIDEO_MODE_800x600_YUV422
#define DC1394_VIDEO_MODE_FORMAT1_MAX            DC1394_VIDEO_MODE_1024x768_MONO16
#define DC1394_VIDEO_MODE_FORMAT1_NUM      (DC1394_VIDEO_MODE_FORMAT1_MAX - DC1394_VIDEO_MODE_FORMAT1_MIN + 1)

// Format_2
#define DC1394_VIDEO_MODE_FORMAT2_MIN            DC1394_VIDEO_MODE_1280x960_YUV422
#define DC1394_VIDEO_MODE_FORMAT2_MAX            DC1394_VIDEO_MODE_1600x1200_MONO16
#define DC1394_VIDEO_MODE_FORMAT2_NUM           (DC1394_VIDEO_MODE_FORMAT2_MAX - DC1394_VIDEO_MODE_FORMAT2_MIN + 1)

// Format_6
#define DC1394_VIDEO_MODE_FORMAT6_MIN            DC1394_VIDEO_MODE_EXIF
#define DC1394_VIDEO_MODE_FORMAT6_MAX            DC1394_VIDEO_MODE_EXIF
#define DC1394_VIDEO_MODE_FORMAT6_NUM           (DC1394_VIDEO_MODE_FORMAT6_MAX - DC1394_VIDEO_MODE_FORMAT6_MIN + 1)

/* Enumeration of camera image formats */
/* This could disappear from the API I think.*/
enum {
    DC1394_FORMAT0= 384,
    DC1394_FORMAT1,
    DC1394_FORMAT2,
    DC1394_FORMAT6=390,
    DC1394_FORMAT7
};
#define DC1394_FORMAT_MIN           DC1394_FORMAT0
#define DC1394_FORMAT_MAX           DC1394_FORMAT7
//#define DC1394_FORMAT_NUM          (DC1394_FORMAT_MAX - DC1394_FORMAT_MIN + 1)
/* DANGER: FORMAT_NUM should be 5!! FORMAT_NUM is therefor undefined to avoid problems */

#define FEATURE_TO_VALUE_OFFSET(feature, offset)                                 \
    {                                                                            \
    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) )      \
      return DC1394_FAILURE;                                                     \
    else if (feature < DC1394_FEATURE_ZOOM)                                      \
      offset= REG_CAMERA_FEATURE_HI_BASE+(feature - DC1394_FEATURE_MIN)*0x04U;   \
    else if (feature >= DC1394_FEATURE_CAPTURE_SIZE)                             \
      offset= REG_CAMERA_FEATURE_LO_BASE +(feature+12-DC1394_FEATURE_ZOOM)*0x04U;\
    else                                                                         \
      offset= REG_CAMERA_FEATURE_LO_BASE +(feature-DC1394_FEATURE_ZOOM)*0x04U;   \
                                                                                   }

#define FEATURE_TO_INQUIRY_OFFSET(feature, offset)                                   \
    {                                                                                \
    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) )          \
      return DC1394_FAILURE;                                                         \
    else if (feature < DC1394_FEATURE_ZOOM)                                          \
      offset= REG_CAMERA_FEATURE_HI_BASE_INQ+(feature - DC1394_FEATURE_MIN)*0x04U;   \
    else if (feature >= DC1394_FEATURE_CAPTURE_SIZE)                                 \
      offset= REG_CAMERA_FEATURE_LO_BASE_INQ +(feature+12-DC1394_FEATURE_ZOOM)*0x04U;\
    else                                                                             \
      offset= REG_CAMERA_FEATURE_LO_BASE_INQ +(feature-DC1394_FEATURE_ZOOM)*0x04U;   \
    }

/* Internal functions required by two different source files */

dc1394error_t
get_quadlets_per_packet(uint32_t mode, uint32_t frame_rate, uint32_t *qpp);

dc1394error_t
get_quadlets_from_format(dc1394camera_t *camera, uint32_t mode, uint32_t *quads);

dc1394error_t
get_format_from_mode(uint32_t mode, uint32_t *format);

dc1394bool_t
is_feature_bit_set(uint32_t value, uint32_t feature);

/*
dc1394bool_t
_dc1394_iidc_check_video_mode(dc1394camera_t *camera, dc1394video_mode_t *mode);
*/
dc1394error_t capture_basic_setup (dc1394camera_t * camera, dc1394video_frame_t * frame);

#endif /* _DC1394_INTERNAL_H */
