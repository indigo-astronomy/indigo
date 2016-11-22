//
//  indigo_names.h
//  indigo
//
//  Created by Peter Polakovic on 22/11/2016.
//  Copyright © 2016 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_names_h
#define indigo_names_h

/** CONNECTION property name.
 */
#define CONNECTION_PROPERTY_NAME					"CONNECTION"

/** CONNECTION.CONNECTED property item name.
 */
#define CONNECTION_CONNECTED_ITEM_NAME		"CONNECTED"

/** CONNECTION.DISCONNECTED property item name.
 */
#define CONNECTION_DISCONNECTED_ITEM_NAME	"DISCONNECTED"

/** INFO property name.
 */
#define INFO_PROPERTY_NAME								"INFO"

/** INFO.DEVICE_NAME property item name.
 */
#define INFO_DEVICE_NAME_ITEM_NAME				"DEVICE_NAME"

/** INFO.DEVICE_VERSION property item name.
 */
#define INFO_DEVICE_VERSION_ITEM_NAME     "DEVICE_VERSION"

/** INFO.DEVICE_INTERFACE property item name.
 */
#define INFO_DEVICE_INTERFACE_ITEM_NAME   "DEVICE_INTERFACE"

/** INFO.FRAMEWORK_NAME property item name.
 */
#define INFO_FRAMEWORK_NAME_ITEM_NAME     "FRAMEWORK_NAME"

/** INFO.FRAMEWORK_VERSION property item name.
 */
#define INFO_FRAMEWORK_VERSION_ITEM_NAME  "FRAMEWORK_VERSION"

/** DEBUG property name.
 */
#define DEBUG_PROPERTY_NAME               "DEBUG"

/** DEBUG.ENABLED property item name.
 */
#define DEBUG_ENABLED_ITEM_NAME           "ENABLED"

/** DEBUG.DISABLED property item name.
 */
#define DEBUG_DISABLED_ITEM_NAME          "DISABLED"

/** SIMULATION property name.
 */
#define SIMULATION_PROPERTY_NAME          "SIMULATION"

/** SIMULATION.DISABLED property name.
 */
#define SIMULATION_ENABLED_ITEM_NAME      "DISABLED"

/** SIMULATION.DISABLED property item name.
 */
#define SIMULATION_DISABLED_ITEM_NAME     "DISABLED"

/** CONFIG property name.
 */
#define CONFIG_PROPERTY_NAME              "CONFIG"

/** CONFIG.LOAD property item name.
 */
#define CONFIG_LOAD_ITEM_NAME             "LOAD"

/** CONFIG.SAVE property item name.
 */
#define CONFIG_SAVE_ITEM_NAME             "SAVE"

/** CONFIG.DEFAULT property item name.
 */
#define CONFIG_DEFAULT_ITEM_NAME          "DEFAULT"

/** DEVICE_PORT property name.
 */
#define DEVICE_PORT_PROPERTY_NAME					"DEVICE_PORT"

/** DEVICE_PORT.DEVICE_PORT property name.
 */
#define DEVICE_PORT_ITEM_NAME							"DEVICE_PORT"

/** CCD_INFO property name.
 */
#define CCD_INFO_PROPERTY_NAME                "CCD_INFO"

/** CCD_INFO.WIDTH property item name.
 */
#define CCD_INFO_WIDTH_ITEM_NAME              "WIDTH"

/** CCD_INFO.HEIGHT property item name.
 */
#define CCD_INFO_HEIGHT_ITEM_NAME             "HEIGHT"

/** CCD_INFO.MAX_HORIZONAL_BIN property item name.
 */
#define CCD_INFO_MAX_HORIZONAL_BIN_ITEM_NAME  "MAX_HORIZONAL_BIN"

/** CCD_INFO.MAX_VERTICAL_BIN property item name.
 */
#define CCD_INFO_MAX_VERTICAL_BIN_ITEM_NAME   "MAX_VERTICAL_BIN"

/** CCD_INFO.PIXEL_SIZE property item name.
 */
#define CCD_INFO_PIXEL_SIZE_ITEM_NAME         "PIXEL_SIZE"

/** CCD_INFO.PIXEL_WIDTH property item name.
 */
#define CCD_INFO_PIXEL_WIDTH_ITEM_NAME        "PIXEL_WIDTH"

/** CCD_INFO.PIXEL_HEIGHT property item name.
 */
#define CCD_INFO_PIXEL_HEIGHT_ITEM_NAME       "PIXEL_HEIGHT"

/** CCD_INFO.BITS_PER_PIXEL property item name.
 */
#define CCD_INFO_BITS_PER_PIXEL_ITEM_NAME     "BITS_PER_PIXEL"

/** CCD_UPLOAD_MODE property name.
 */
#define CCD_UPLOAD_MODE_PROPERTY_NAME         "CCD_UPLOAD_MODE"

/** CCD_UPLOAD_MODE.CLIENT property item name.
 */
#define CCD_UPLOAD_MODE_CLIENT_ITEM_NAME      "CLIENT"

/** CCD_UPLOAD_MODE.LOCAL property item name.
 */
#define CCD_UPLOAD_MODE_LOCAL_ITEM_NAME       "LOCAL"

/** CCD_UPLOAD_MODE.BOTH property item name.
 */
#define CCD_UPLOAD_MODE_BOTH_ITEM_NAME        "BOTH"

/** CCD_LOCAL_MODE property name.
 */
#define CCD_LOCAL_MODE_PROPERTY_NAME          "CCD_LOCAL_MODE"

/** CCD_LOCAL_MODE.DIR property item name.
 */
#define CCD_LOCAL_MODE_DIR_ITEM_NAME          "DIR"

/** CCD_LOCAL_MODE.PREFIX property item name.
 */
#define CCD_LOCAL_MODE_PREFIX_ITEM_NAME       "PREFIX"

/** CCD_EXPOSURE property name.
 */
#define CCD_EXPOSURE_PROPERTY_NAME            "CCD_EXPOSURE"

/** CCD_EXPOSURE.EXPOSURE property item name.
 */
#define CCD_EXPOSURE_ITEM_NAME                "EXPOSURE"

/** CCD_ABORT_EXPOSURE property name.
 */
#define CCD_ABORT_EXPOSURE_PROPERTY_NAME      "CCD_ABORT_EXPOSURE"

/** CCD_ABORT_EXPOSURE.ABORT_EXPOSURE property item name.
 */
#define CCD_ABORT_EXPOSURE_ITEM_NAME          "ABORT_EXPOSURE"

/** CCD_FRAME property name.
 */
#define CCD_FRAME_PROPERTY_NAME               "CCD_FRAME"

/** CCD_FRAME.LEFT property item name.
 */
#define CCD_FRAME_LEFT_ITEM_NAME              "LEFT"

/** CCD_FRAME.TOP property item name.
 */
#define CCD_FRAME_TOP_ITEM_NAME               "TOP"

/** CCD_FRAME.WIDTH property item name.
 */
#define CCD_FRAME_WIDTH_ITEM_NAME             "WIDTH"

/** CCD_FRAME.HEIGHT property item name.
 */
#define CCD_FRAME_HEIGHT_ITEM_NAME            "HEIGHT"

/** CCD_FRAME.BITS_PER_PIXEL property item name.
 */
#define CCD_FRAME_BITS_PER_PIXEL_ITEM_NAME    "BITS_PER_PIXEL"

/** CCD_BIN property name.
 */
#define CCD_BIN_PROPERTY_NAME                 "CCD_BIN"

/** CCD_BIN.HORIZONTAL property item name.
 */
#define CCD_BIN_HORIZONTAL_ITEM_NAME          "HORIZONTAL"

/** CCD_BIN.VERTICAL property item name.
 */
#define CCD_BIN_VERTICAL_ITEM_NAME            "VERTICAL"

/** CCD_MODE property name.
 */
#define CCD_MODE_PROPERTY_NAME								"CCD_MODE"

/** CCD_GAIN property name.
 */
#define CCD_GAIN_PROPERTY_NAME                "CCD_GAIN"

/** CCD_GAIN.GAIN property item name.
 */
#define CCD_GAIN_ITEM_NAME                    "GAIN"

/** CCD_OFFSET property name.
 */
#define CCD_OFFSET_PROPERTY_NAME              "CCD_OFFSET"

/** CCD_OFFSET.OFFSET property item name.
 */
#define CCD_OFFSET_ITEM_NAME									"OFFSET"

/** CCD_GAMMA property name.
 */
#define CCD_GAMMA_PROPERTY_NAME               "CCD_GAMMA"

/** CCD_GAMMA.GAMMA property item name.
 */
#define CCD_GAMMA_ITEM_NAME                   "GAMMA"

/** CCD_FRAME_TYPE property name.
 */
#define CCD_FRAME_TYPE_PROPERTY_NAME          "CCD_FRAME_TYPE"

/** CCD_FRAME_TYPE.LIGHT property item name
 */
#define CCD_FRAME_TYPE_LIGHT_ITEM_NAME        "LIGHT"

/** CCD_FRAME_TYPE.BIAS property item name.
 */
#define CCD_FRAME_TYPE_BIAS_ITEM_NAME         "BIAS"

/** CCD_FRAME_TYPE.DARK property item name.
 */
#define CCD_FRAME_TYPE_DARK_ITEM_NAME         "DARK"

/** CCD_FRAME_TYPE.FLAT property item name.
 */
#define CCD_FRAME_TYPE_FLAT_ITEM_NAME         "FLAT"

/** CCD_IMAGE_FORMAT property name.
 */
#define CCD_IMAGE_FORMAT_PROPERTY_NAME        "CCD_IMAGE_FORMAT"

/** CCD_IMAGE_FORMAT.RAW property item name.
 */
#define CCD_IMAGE_FORMAT_RAW_ITEM_NAME        "RAW"

/** CCD_IMAGE_FORMAT.FITS property item name.
 */
#define CCD_IMAGE_FORMAT_FITS_ITEM_NAME       "FITS"

/** CCD_IMAGE_FORMAT.JPEG property item name.
 */
#define CCD_IMAGE_FORMAT_JPEG_ITEM_NAME       "JPEG"

/** CCD_IMAGE_FILE property name.
 */
#define CCD_IMAGE_FILE_PROPERTY_NAME          "CCD_IMAGE_FILE"

/** CCD_IMAGE_FILE.FILE property item name.
 */
#define CCD_IMAGE_FILE_ITEM_NAME              "FILE "

/** CCD_IMAGE property name.
 */
#define CCD_IMAGE_PROPERTY_NAME               "CCD_IMAGE"

/** CCD_IMAGE.IMAGE property item name.
 */
#define CCD_IMAGE_ITEM_NAME                   "IMAGE"

/** CCD_TEMPERATURE property name.
 */
#define CCD_TEMPERATURE_PROPERTY_NAME         "CCD_TEMPERATURE"

/** CCD_TEMPERATURE.TEMPERATURE property item name.
 */
#define CCD_TEMPERATURE_ITEM_NAME             "TEMPERATURE"

/** CCD_COOLER property name.
 */
#define CCD_COOLER_PROPERTY_NAME              "CCD_COOLER"

/** CCD_COOLER.ON property item name.
 */
#define CCD_COOLER_ON_ITEM_NAME               "ON"

/** CCD_COOLER.OFF property item name.
 */
#define CCD_COOLER_OFF_ITEM_NAME              "OFF"

/** CCD_COOLER_POWER property name.
 */
#define CCD_COOLER_POWER_PROPERTY_NAME        "CCD_COOLER_POWER"

/** CCD_COOLER_POWER.POWER property item name.
 */
#define CCD_COOLER_POWER_ITEM_NAME            "POWER"




#endif /* indigo_names_h */
