/*
 * 1394-Based Digital Camera Control Library
 *
 * Camera standard offsets
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


#ifndef __DC1394_OFFSETS_H_
#define __DC1394_OFFSETS_H_

/* See the 1394-Based Digital Camera Spec. for definitions of these */

/* Base ROM offsets */

#define ROM_BUS_INFO_BLOCK             0x400U
#define ROM_ROOT_DIRECTORY             0x414U
#ifndef CSR_CONFIG_ROM_END
#define CSR_CONFIG_ROM_END             0x800U
#endif

/* Absolute feature */

#define REG_CAMERA_FEATURE_ABS_HI_BASE      0x700U
#define REG_CAMERA_FEATURE_ABS_LO_BASE      0x780U

#define REG_CAMERA_ABS_MIN                  0x000U
#define REG_CAMERA_ABS_MAX                  0x004U
#define REG_CAMERA_ABS_VALUE                0x008U

/* Command registers offsets */

#define REG_CAMERA_INITIALIZE               0x000U
#define REG_CAMERA_V_FORMAT_INQ             0x100U
#define REG_CAMERA_V_MODE_INQ_BASE          0x180U
#define REG_CAMERA_V_RATE_INQ_BASE          0x200U
#define REG_CAMERA_V_REV_INQ_BASE           0x2C0U
#define REG_CAMERA_V_CSR_INQ_BASE           0x2E0U
#define REG_CAMERA_BASIC_FUNC_INQ           0x400U
#define REG_CAMERA_FEATURE_HI_INQ           0x404U
#define REG_CAMERA_FEATURE_LO_INQ           0x408U
#define REG_CAMERA_OPT_FUNC_INQ             0x40CU
#define REG_CAMERA_ADV_FEATURE_INQ          0x480U
#define REG_CAMERA_PIO_CONTROL_CSR_INQ      0x484U
#define REG_CAMERA_SIO_CONTROL_CSR_INQ      0x488U
#define REG_CAMERA_STROBE_CONTROL_CSR_INQ   0x48CU
#define REG_CAMERA_FEATURE_HI_BASE_INQ      0x500U
#define REG_CAMERA_FEATURE_LO_BASE_INQ      0x580U
#define REG_CAMERA_FRAME_RATE               0x600U
#define REG_CAMERA_VIDEO_MODE               0x604U
#define REG_CAMERA_VIDEO_FORMAT             0x608U
#define REG_CAMERA_ISO_DATA                 0x60CU
#define REG_CAMERA_POWER                    0x610U
#define REG_CAMERA_ISO_EN                   0x614U
#define REG_CAMERA_MEMORY_SAVE              0x618U
#define REG_CAMERA_ONE_SHOT                 0x61CU
#define REG_CAMERA_MEM_SAVE_CH              0x620U
#define REG_CAMERA_CUR_MEM_CH               0x624U
#define REG_CAMERA_SOFT_TRIGGER             0x62CU
#define REG_CAMERA_DATA_DEPTH               0x630U
#define REG_CAMERA_FEATURE_ERR_HI_INQ       0x640h
#define REG_CAMERA_FEATURE_ERR_LO_INQ       0x644h

#define REG_CAMERA_FEATURE_HI_BASE          0x800U
#define REG_CAMERA_FEATURE_LO_BASE          0x880U

#define REG_CAMERA_BRIGHTNESS               0x800U
#define REG_CAMERA_EXPOSURE                 0x804U
#define REG_CAMERA_SHARPNESS                0x808U
#define REG_CAMERA_WHITE_BALANCE            0x80CU
#define REG_CAMERA_HUE                      0x810U
#define REG_CAMERA_SATURATION               0x814U
#define REG_CAMERA_GAMMA                    0x818U
#define REG_CAMERA_SHUTTER                  0x81CU
#define REG_CAMERA_GAIN                     0x820U
#define REG_CAMERA_IRIS                     0x824U
#define REG_CAMERA_FOCUS                    0x828U
#define REG_CAMERA_TEMPERATURE              0x82CU
#define REG_CAMERA_TRIGGER_MODE             0x830U
#define REG_CAMERA_TRIGGER_DELAY            0x834U
#define REG_CAMERA_WHITE_SHADING            0x838U
#define REG_CAMERA_FRAME_RATE_FEATURE       0x83CU
#define REG_CAMERA_ZOOM                     0x880U
#define REG_CAMERA_PAN                      0x884U
#define REG_CAMERA_TILT                     0x888U
#define REG_CAMERA_OPTICAL_FILTER           0x88CU
#define REG_CAMERA_CAPTURE_SIZE             0x8C0U
#define REG_CAMERA_CAPTURE_QUALITY          0x8C4U

/* Format_7 offsets */

#define REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ            0x000U
#define REG_CAMERA_FORMAT7_UNIT_SIZE_INQ                 0x004U
#define REG_CAMERA_FORMAT7_IMAGE_POSITION                0x008U
#define REG_CAMERA_FORMAT7_IMAGE_SIZE                    0x00CU
#define REG_CAMERA_FORMAT7_COLOR_CODING_ID               0x010U
#define REG_CAMERA_FORMAT7_COLOR_CODING_INQ              0x014U
#define REG_CAMERA_FORMAT7_PIXEL_NUMBER_INQ              0x034U
#define REG_CAMERA_FORMAT7_TOTAL_BYTES_HI_INQ            0x038U
#define REG_CAMERA_FORMAT7_TOTAL_BYTES_LO_INQ            0x03CU
#define REG_CAMERA_FORMAT7_PACKET_PARA_INQ               0x040U
#define REG_CAMERA_FORMAT7_BYTE_PER_PACKET               0x044U
#define REG_CAMERA_FORMAT7_PACKET_PER_FRAME_INQ          0x048U
#define REG_CAMERA_FORMAT7_UNIT_POSITION_INQ             0x04CU
#define REG_CAMERA_FORMAT7_FRAME_INTERVAL_INQ            0x050U
#define REG_CAMERA_FORMAT7_DATA_DEPTH_INQ                0x054U
#define REG_CAMERA_FORMAT7_COLOR_FILTER_ID               0x058U
#define REG_CAMERA_FORMAT7_VALUE_SETTING                 0x07CU

/* PIO offsets */

#define REG_CAMERA_PIO_IN                                0x000U
#define REG_CAMERA_PIO_OUT                               0x004U

#endif /* __DC1394_OFFSETS_H__ */
