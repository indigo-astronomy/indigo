//
//  indigo_names.h
//  indigo
//
//  Created by Peter Polakovic on 22/11/2016.
//  Copyright © 2016 CloudMakers, s. r. o. All rights reserved.
//

#ifndef indigo_names_h
#define indigo_names_h

//----------------------------------------------------------------------
/** CONNECTION property name.
 */
#define CONNECTION_PROPERTY_NAME							"CONNECTION"

/** CONNECTION.CONNECTED property item name.
 */
#define CONNECTION_CONNECTED_ITEM_NAME				"CONNECTED"

/** CONNECTION.DISCONNECTED property item name.
 */
#define CONNECTION_DISCONNECTED_ITEM_NAME			"DISCONNECTED"

//----------------------------------------------------------------------
/** INFO property name.
 */
#define INFO_PROPERTY_NAME										"INFO"

/** INFO.DEVICE_NAME property item name.
 */
#define INFO_DEVICE_NAME_ITEM_NAME						"DEVICE_NAME"

/** INFO.DEVICE_DRIVER property item name.
 */
#define INFO_DEVICE_DRIVER_ITEM_NAME					"DEVICE_DRIVER"

/** INFO.DEVICE_VERSION property item name.
 */
#define INFO_DEVICE_VERSION_ITEM_NAME					"DEVICE_VERSION"

/** INFO.DEVICE_INTERFACE property item name.
 */
#define INFO_DEVICE_INTERFACE_ITEM_NAME				"DEVICE_INTERFACE"

/** INFO.FRAMEWORK_NAME property item name.
 */
#define INFO_FRAMEWORK_NAME_ITEM_NAME					"FRAMEWORK_NAME"

/** INFO.FRAMEWORK_VERSION property item name.
 */
#define INFO_FRAMEWORK_VERSION_ITEM_NAME			"FRAMEWORK_VERSION"

/** INFO.DEVICE_MODEL_ITEM_NAME property item name.
 */
#define INFO_DEVICE_MODEL_ITEM_NAME					"DEVICE_MODEL"

/** INFO.DEVICE_FW_REVISION_ITEM_NAME property item name.
 */
#define INFO_DEVICE_FW_REVISION_ITEM_NAME			"DEVICE_FIRMWARE_REVISION"

/** INFO.DEVICE_HW_REVISION_ITEM_NAME property item name.
 */
#define INFO_DEVICE_HW_REVISION_ITEM_NAME			"DEVICE_HARDWARE_REVISION"

/** INFO.DEVICE_SERIAL_NUM_ITEM_NAME property item name.
 */
#define INFO_DEVICE_SERIAL_NUM_ITEM_NAME			"DEVICE_SERIAL_NUMBER"

//----------------------------------------------------------------------
/** SIMULATION property name.
 */
#define SIMULATION_PROPERTY_NAME							"SIMULATION"

/** SIMULATION.DISABLED property name.
 */
#define SIMULATION_ENABLED_ITEM_NAME					"ENABLED"

/** SIMULATION.DISABLED property item name.
 */
#define SIMULATION_DISABLED_ITEM_NAME					"DISABLED"

//----------------------------------------------------------------------
/** CONFIG property name.
 */
#define CONFIG_PROPERTY_NAME									"CONFIG"

/** CONFIG.LOAD property item name.
 */
#define CONFIG_LOAD_ITEM_NAME									"LOAD"

/** CONFIG.SAVE property item name.
 */
#define CONFIG_SAVE_ITEM_NAME									"SAVE"

/** CONFIG.DEFAULT property item name.
 */
#define CONFIG_REMOVE_ITEM_NAME							"REMOVE"

//----------------------------------------------------------------------
/** PROFILE property name.
 */
#define PROFILE_PROPERTY_NAME									"PROFILE"

/** PROFILE.PROFILE_X property item name.
 */
#define PROFILE_ITEM_NAME											"PROFILE_%d"

//----------------------------------------------------------------------
/** DEVICE_PORT property name.
 */
#define DEVICE_PORT_PROPERTY_NAME							"DEVICE_PORT"

/** DEVICE_PORT.DEVICE_PORT property name.
 */
#define DEVICE_PORT_ITEM_NAME									"PORT"

/** DEVICE_BAUDRATE property name.
 */
#define DEVICE_BAUDRATE_PROPERTY_NAME                                                       "DEVICE_BAUDRATE"

/** DEVICE_BAUDRATE.DEVICE_BAUDRATE property name.
 */
#define DEVICE_BAUDRATE_ITEM_NAME                                                                   "BAUDRATE"

//----------------------------------------------------------------------
/** AUTHENTICATION property name.
 */
#define AUTHENTICATION_PROPERTY_NAME							"AUTHENTICATION"

/** AUTHENTICATION.PASSWORD property name.
 */
#define AUTHENTICATION_PASSWORD_ITEM_NAME					"PASSWORD"

/** AUTHENTICATION.USER property name.
 */
#define AUTHENTICATION_USER_ITEM_NAME							"USER"

//----------------------------------------------------------------------
/** ADDITIONAL_INSTANCES  property name.
 */
#define ADDITIONAL_INSTANCES_PROPERTY_NAME				"ADDITIONAL_INSTANCES"

/** ADDITIONAL_INSTANCES.COUNT property item mame.
 */
#define ADDITIONAL_INSTANCES_COUNT_ITEM_NAME			"COUNT"

//----------------------------------------------------------------------
/** DEVICE_PORTS property name.
 */
#define DEVICE_PORTS_PROPERTY_NAME						"DEVICE_PORTS"

/** DEVICE_PORTS.REFRESH item name.
 */
#define DEVICE_PORTS_REFRESH_ITEM_NAME				"REFRESH"

//----------------------------------------------------------------------
/** GEOGRAPHIC_COORDINATES property name.
 */
#define GEOGRAPHIC_COORDINATES_PROPERTY_NAME	"GEOGRAPHIC_COORDINATES"

/** GEOGRAPHIC_COORDINATES.LATITUDE property item name.
 */
#define GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME	"LATITUDE"

/** GEOGRAPHIC_COORDINATES.LONGITUDE property item name.
 */
#define GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME	"LONGITUDE"

/** GEOGRAPHIC_COORDINATES.ELEVATION property item name.
 */
#define GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME	"ELEVATION"

/** GEOGRAPHIC_COORDINATES.ACCURACY property item name.
 */
#define GEOGRAPHIC_COORDINATES_ACCURACY_ITEM_NAME	"ACCURACY"

//----------------------------------------------------------------------
/** DOME_SNOOP_DEVICES property name.
 */
#define SNOOP_DEVICES_PROPERTY_NAME	"SNOOP_DEVICES"

/** MOUNT_SNOOP_DEVICES property name.
 */
#define SNOOP_DEVICES_PROPERTY_NAME	"SNOOP_DEVICES"

/** SNOOP_DEVICES.MOUNT property item name.
 */
#define SNOOP_MOUNT_ITEM_NAME				"MOUNT"

/** SNOOP_DEVICES.GPS property item name.
 */
#define SNOOP_GPS_ITEM_NAME					"GPS"

/** SNOOP_DEVICES.CCD property item name.
 */
#define SNOOP_CCD_ITEM_NAME					"CCD"

/** SNOOP_DEVICES.DOME property item name.
 */
#define SNOOP_DOME_ITEM_NAME					"DOME"

/** SNOOP_DEVICES.WHEEL property item name.
 */
#define SNOOP_WHEEL_ITEM_NAME					"WHEEL"

/** SNOOP_DEVICES.FOCUSER property item name.
 */
#define SNOOP_FOCUSER_ITEM_NAME					"FOCUSER"

/** SNOOP_DEVICES.JOYSTICK property item name.
 */
#define SNOOP_JOYSTICK_ITEM_NAME					"JOYSTICK"


//----------------------------------------------------------------------
/** UTC_TIME property name.
 */
#define UTC_TIME_PROPERTY_NAME						"UTC_TIME"

/** UTC_TIME.UTC property item name.
 */
#define UTC_TIME_ITEM_NAME								"TIME"

/** UTC_TIME.OFFSET property item name.
 */
#define UTC_OFFSET_ITEM_NAME							"OFFSET"

//----------------------------------------------------------------------
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
#define CCD_INFO_MAX_HORIZONTAL_BIN_ITEM_NAME  "MAX_HORIZONTAL_BIN"

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

//----------------------------------------------------------------------

/** CCD_LENS property name.
*/
#define CCD_LENS_PROPERTY_NAME								"CCD_LENS"

/** CCD_LENS.APERTURE property item name.
 */
#define CCD_LENS_APERTURE_ITEM_NAME		        "APERTURE"

/** CCD_LENS.FOCAL_LENGTH property item name.
*/
#define CCD_LENS_FOCAL_LENGTH_ITEM_NAME				"FOCAL_LENGTH"

//----------------------------------------------------------------------

/** CCD_PREVIEW property name.
 */
#define CCD_PREVIEW_PROPERTY_NAME							"CCD_PREVIEW"

/** CCD_PREVIEW.ENABLED property item name.
 */
#define CCD_PREVIEW_ENABLED_ITEM_NAME       	"ENABLED"

/** CCD_PREVIEW.ENABLED_WITH_HISTOGRAM property item name.
 */
#define CCD_PREVIEW_ENABLED_WITH_HISTOGRAM_ITEM_NAME		"ENABLED_WITH_HISTOGRAM"

/** CCD_PREVIEW.DISABLED property item name.
 */
#define CCD_PREVIEW_DISABLED_ITEM_NAME       	"DISABLED"

/** CCD_PREVIEW_IMAGE property name.
 */
#define CCD_PREVIEW_IMAGE_PROPERTY_NAME				"CCD_PREVIEW_IMAGE"

/** CCD_PREVIEW_IMAGE.IMAGE property item name.
 */
#define CCD_PREVIEW_IMAGE_ITEM_NAME           "IMAGE"

/** CCD_PREVIEW_HISTOGRAM property name.
 */
#define CCD_PREVIEW_HISTOGRAM_PROPERTY_NAME				"CCD_PREVIEW_HISTOGRAM"

/** CCD_PREVIEW_HISTOGRAM.IMAGE property item name.
 */
#define CCD_PREVIEW_HISTOGRAM_ITEM_NAME           "IMAGE"

//----------------------------------------------------------------------
/** CCD_LOCAL_MODE property name.
 */
#define CCD_LOCAL_MODE_PROPERTY_NAME          "CCD_LOCAL_MODE"

/** CCD_LOCAL_MODE.DIR property item name.
 */
#define CCD_LOCAL_MODE_DIR_ITEM_NAME          "DIR"

/** CCD_LOCAL_MODE.PREFIX property item name.
 */
#define CCD_LOCAL_MODE_PREFIX_ITEM_NAME       "PREFIX"

//----------------------------------------------------------------------
/** CCD_EXPOSURE property name.
 */
#define CCD_EXPOSURE_PROPERTY_NAME            "CCD_EXPOSURE"

/** CCD_EXPOSURE.EXPOSURE property item name.
 */
#define CCD_EXPOSURE_ITEM_NAME                "EXPOSURE"


//----------------------------------------------------------------------
/** CCD_STREAMING property name.
 */
#define CCD_STREAMING_PROPERTY_NAME            "CCD_STREAMING"

/** CCD_STREAMING.COUNT property item name.
 */
#define CCD_STREAMING_EXPOSURE_ITEM_NAME			"EXPOSURE"

/** CCD_STREAMING.COUNT property item name.
 */
#define CCD_STREAMING_COUNT_ITEM_NAME         "COUNT"

//----------------------------------------------------------------------
/** CCD_ABORT_EXPOSURE property name.
 */
#define CCD_ABORT_EXPOSURE_PROPERTY_NAME      "CCD_ABORT_EXPOSURE"

/** CCD_ABORT_EXPOSURE.ABORT_EXPOSURE property item name.
 */
#define CCD_ABORT_EXPOSURE_ITEM_NAME          "ABORT_EXPOSURE"

//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
/** CCD_BIN property name.
 */
#define CCD_BIN_PROPERTY_NAME                 "CCD_BIN"

/** CCD_BIN.HORIZONTAL property item name.
 */
#define CCD_BIN_HORIZONTAL_ITEM_NAME          "HORIZONTAL"

/** CCD_BIN.VERTICAL property item name.
 */
#define CCD_BIN_VERTICAL_ITEM_NAME            "VERTICAL"

//----------------------------------------------------------------------
/** CCD_MODE property name.
 */
#define CCD_MODE_PROPERTY_NAME								"CCD_MODE"

//----------------------------------------------------------------------
/** CCD_READ_MODE property name.
 */
#define CCD_READ_MODE_PROPERTY_NAME						"CCD_READ_MODE"

/** CCD_READ_MODE.HIGH_SPEED property item name.
 */
#define CCD_READ_MODE_HIGH_SPEED_ITEM_NAME    "HIGH_SPEED"

/** CCD_READ_MODE.LOW_NOISE property item name.
 */
#define CCD_READ_MODE_LOW_NOISE_ITEM_NAME    	"LOW_NOISE"

//----------------------------------------------------------------------
/** CCD_GAIN property name.
 */
#define CCD_GAIN_PROPERTY_NAME                "CCD_GAIN"

/** CCD_GAIN.GAIN property item name.
 */
#define CCD_GAIN_ITEM_NAME                    "GAIN"

//----------------------------------------------------------------------
/** CCD_OFFSET property name.
 */
#define CCD_OFFSET_PROPERTY_NAME              "CCD_OFFSET"

/** CCD_OFFSET.OFFSET property item name.
 */
#define CCD_OFFSET_ITEM_NAME									"OFFSET"

//----------------------------------------------------------------------
/** CCD_GAMMA property name.
 */
#define CCD_GAMMA_PROPERTY_NAME               "CCD_GAMMA"

/** CCD_GAMMA.GAMMA property item name.
 */
#define CCD_GAMMA_ITEM_NAME                   "GAMMA"

//----------------------------------------------------------------------
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

/** CCD_FRAME_TYPE.DARKFLAT property item name.
 */
#define CCD_FRAME_TYPE_DARKFLAT_ITEM_NAME     "DARKFLAT"

//----------------------------------------------------------------------
/** CCD_IMAGE_FORMAT property name.
 */
#define CCD_IMAGE_FORMAT_PROPERTY_NAME        "CCD_IMAGE_FORMAT"

/** CCD_IMAGE_FORMAT.RAW property item name.
 */
#define CCD_IMAGE_FORMAT_RAW_ITEM_NAME        "RAW"

/** CCD_IMAGE_FORMAT.FITS property item name.
 */
#define CCD_IMAGE_FORMAT_FITS_ITEM_NAME       "FITS"

/** CCD_IMAGE_FORMAT.FITS property item name.
 */
#define CCD_IMAGE_FORMAT_XISF_ITEM_NAME       "XISF"

/** CCD_IMAGE_FORMAT.JPEG property item name.
 */
#define CCD_IMAGE_FORMAT_JPEG_ITEM_NAME       "JPEG"

/** CCD_IMAGE_FORMAT.TIFF property item name.
 */
#define CCD_IMAGE_FORMAT_TIFF_ITEM_NAME       "TIFF"

/** CCD_IMAGE_FORMAT.JPEG_AVI property item name.
 */
#define CCD_IMAGE_FORMAT_JPEG_AVI_ITEM_NAME   "JPEG_AVI"

/** CCD_IMAGE_FORMAT.RAW_SER property item name.
 */
#define CCD_IMAGE_FORMAT_RAW_SER_ITEM_NAME   "RAW_SER"

/** CCD_IMAGE_FORMAT.NATIVE property item name.
 */
#define CCD_IMAGE_FORMAT_NATIVE_ITEM_NAME 		"NATIVE"

/** CCD_IMAGE_FORMAT.NATIVE_AVI property item name.
 */
#define CCD_IMAGE_FORMAT_NATIVE_AVI_ITEM_NAME  "NATIVE_AVI"


//----------------------------------------------------------------------
/** CCD_IMAGE_FILE property name.
 */
#define CCD_IMAGE_FILE_PROPERTY_NAME          "CCD_IMAGE_FILE"

/** CCD_IMAGE_FILE.FILE property item name.
 */
#define CCD_IMAGE_FILE_ITEM_NAME              "FILE"

/** CCD_IMAGE property name.
 */
#define CCD_IMAGE_PROPERTY_NAME               "CCD_IMAGE"

/** CCD_IMAGE.IMAGE property item name.
 */
#define CCD_IMAGE_ITEM_NAME                   "IMAGE"

//----------------------------------------------------------------------
/** CCD_TEMPERATURE property name.
 */
#define CCD_TEMPERATURE_PROPERTY_NAME         "CCD_TEMPERATURE"

/** CCD_TEMPERATURE.TEMPERATURE property item name.
 */
#define CCD_TEMPERATURE_ITEM_NAME             "TEMPERATURE"

//----------------------------------------------------------------------
/** CCD_COOLER property name.
 */
#define CCD_COOLER_PROPERTY_NAME              "CCD_COOLER"

/** CCD_COOLER.ON property item name.
 */
#define CCD_COOLER_ON_ITEM_NAME               "ON"

/** CCD_COOLER.OFF property item name.
 */
#define CCD_COOLER_OFF_ITEM_NAME              "OFF"

//----------------------------------------------------------------------
/** CCD_COOLER_POWER property name.
 */
#define CCD_COOLER_POWER_PROPERTY_NAME        "CCD_COOLER_POWER"

/** CCD_COOLER_POWER.POWER property item name.
 */
#define CCD_COOLER_POWER_ITEM_NAME            "POWER"

//----------------------------------------------------------------------
/** CCD_FITS_HEADERS property name.
 */
#define CCD_FITS_HEADERS_PROPERTY_NAME									"CCD_FITS_HEADERS"

/** CCD_FITS_HEADERS.HEADER_x property item name.
 */
#define CCD_FITS_HEADER_ITEM_NAME							"HEADER_%d"


//----------------------------------------------------------------------
/** CCD_JPEG_SETTINGS property name.
 */
#define CCD_JPEG_SETTINGS_PROPERTY_NAME				"CCD_JPEG_SETTINGS"

/** CCD_JPEG_SETTINGS.QUALITY property item name.
 */
#define CCD_JPEG_SETTINGS_QUALITY_ITEM_NAME		"QUALITY"

/** CCD_JPEG_SETTINGS.BLACK property item name.
 */
#define CCD_JPEG_SETTINGS_BLACK_ITEM_NAME			"BLACK"

/** CCD_JPEG_SETTINGS.WHITE property item name.
 */
#define CCD_JPEG_SETTINGS_WHITE_ITEM_NAME			"WHITE"

/** CCD_JPEG_SETTINGS.BLACK_TRESHOLD property item name.
 */
#define CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM_NAME			"BLACK_TRESHOLD"

/** CCD_JPEG_SETTINGS.WHITE_TRESHOLD property item name.
 */
#define CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM_NAME			"WHITE_TRESHOLD"

/** CCD_RBI_FLUSH_ENABLE property name.
 */
#define CCD_RBI_FLUSH_PROPERTY_NAME          "CCD_RBI_FLUSH_ENABLE"

/** CCD_RBI_FLUSH.EXPOSURE property item pointer.
 */
#define CCD_RBI_FLUSH_EXPOSURE_ITEM_NAME     "EXPOSURE"
/** CCD_RBI_FLUSH.COUNT property item name.
 */
#define CCD_RBI_FLUSH_COUNT_ITEM_NAME        "COUNT"

/** CCD_RBI_FLUSH_ENABLE property name.
 */
#define CCD_RBI_FLUSH_ENABLE_PROPERTY_NAME   "CCD_RBI_FLUSH_ENABLE"

/** CCD_RBI_FLUSH_ENABLE.ENABLE property item name.
 */
#define CCD_RBI_FLUSH_ENABLED_ITEM_NAME      "ENABLED"

/** CCD_RBI_FLUSH_ENABLE.DISABLE property item name.
 */
#define CCD_RBI_FLUSH_DISABLED_ITEM_NAME     "DISABLED"

//----------------------------------------------------------------------
/** DSLR_PROGRAM property name.
 */
#define DSLR_PROGRAM_PROPERTY_NAME						"DSLR_PROGRAM"

//----------------------------------------------------------------------
/** DSLR_APERTURE property name.
 */
#define DSLR_APERTURE_PROPERTY_NAME						"DSLR_APERTURE"

//----------------------------------------------------------------------
/** DSLR_SHUTTER property name.
 */
#define DSLR_SHUTTER_PROPERTY_NAME						"DSLR_SHUTTER"

//----------------------------------------------------------------------
/** DSLR_IMAGE_SIZE property name.
 */
#define DSLR_IMAGE_SIZE_PROPERTY_NAME					"DSLR_IMAGE_SIZE"

//----------------------------------------------------------------------
/** DSLR_COMPRESSION property name.
 */
#define DSLR_COMPRESSION_PROPERTY_NAME				"DSLR_COMPRESSION"

//----------------------------------------------------------------------
/** DSLR_WHITE_BALANCE property name.
 */
#define DSLR_WHITE_BALANCE_PROPERTY_NAME			"DSLR_WHITE_BALANCE"

//----------------------------------------------------------------------
/** DSLR_ISO property name.
 */
#define DSLR_ISO_PROPERTY_NAME								"DSLR_ISO"

//----------------------------------------------------------------------
/** DSLR_EXPOSURE_COMPENSATION property name.
 */
#define DSLR_EXPOSURE_COMPENSATION_PROPERTY_NAME		"DSLR_EXPOSURE_COMPENSATION"

//----------------------------------------------------------------------
/** DSLR_EXPOSURE_METERING property name.
 */
#define DSLR_EXPOSURE_METERING_PROPERTY_NAME				"DSLR_EXPOSURE_METERING"

//----------------------------------------------------------------------
/** DSLR_FOCUS_METERING property name.
 */
#define DSLR_FOCUS_METERING_PROPERTY_NAME						"DSLR_FOCUS_METERING"

//----------------------------------------------------------------------
/** DSLR_FOCUS_MODE property name.
 */
#define DSLR_FOCUS_MODE_PROPERTY_NAME								"DSLR_FOCUS_MODE"

//----------------------------------------------------------------------
/** DSLR_BATTERY_LEVEL property name.
 */
#define DSLR_BATTERY_LEVEL_PROPERTY_NAME						"DSLR_BATTERY_LEVEL"

//----------------------------------------------------------------------
/** DSLR_FOCAL_LENGTH property name.
 */
#define DSLR_FOCAL_LENGTH_PROPERTY_NAME							"DSLR_FOCAL_LENGTH"

//----------------------------------------------------------------------
/** DSLR_CAPTURE_MODE property name.
 */
#define DSLR_CAPTURE_MODE_PROPERTY_NAME							"DSLR_CAPTURE_MODE"

//----------------------------------------------------------------------
/** DSLR_COMPENSATION_STEP property name.
 */
#define DSLR_COMPENSATION_STEP_PROPERTY_NAME				"DSLR_COMPENSATION_STEP"

//----------------------------------------------------------------------
/** DSLR_FLASH_MODE property name.
 */
#define DSLR_FLASH_MODE_PROPERTY_NAME								"DSLR_FLASH_MODE"

//----------------------------------------------------------------------
/** DSLR_FLASH_COMPENSATION property name.
 */
#define DSLR_FLASH_COMPENSATION_PROPERTY_NAME		"DSLR_FLASH_COMPENSATION"

//----------------------------------------------------------------------
/** DSLR_EXT_FLASH_MODE property name.
 */
#define DSLR_EXT_FLASH_MODE_PROPERTY_NAME								"DSLR_EXT_FLASH_MODE"

//----------------------------------------------------------------------
/** DSLR_EXT_FLASH_COMPENSATION property name.
 */
#define DSLR_EXT_FLASH_COMPENSATION_PROPERTY_NAME		"DSLR_EXT_FLASH_COMPENSATION"

//----------------------------------------------------------------------
/** DSLR_PICTURE_STYLE property name.
 */
#define DSLR_PICTURE_STYLE_PROPERTY_NAME		"DSLR_PICTURE_STYLE"

//----------------------------------------------------------------------
/** DSLR_ASPECT_RATIO property name.
 */
#define DSLR_ASPECT_RATIO_PROPERTY_NAME		"DSLR_ASPECT_RATIO"

//----------------------------------------------------------------------
/** DSLR_COLOR_SPACE property name.
 */
#define DSLR_COLOR_SPACE_PROPERTY_NAME		"DSLR_COLOR_SPACE"

//----------------------------------------------------------------------
/** DSLR_LOCK property name.
 */
#define DSLR_LOCK_PROPERTY_NAME		"DSLR_LOCK"

/** DSLR_LOCK.LOCK property item name.
 */
#define DSLR_LOCK_ITEM_NAME        "LOCK"

/** DSLR_LOCK.LOCK property item name.
 */
#define DSLR_UNLOCK_ITEM_NAME        "UNLOCK"

//----------------------------------------------------------------------
/** DSLR_MIRROR_LOCKUP property name.
 */
#define DSLR_MIRROR_LOCKUP_PROPERTY_NAME		"DSLR_MIRROR_LOCKUP"

/** DSLR_MIRROR_LOCKUP.LOCK property item name.
 */
#define DSLR_MIRROR_LOCKUP_LOCK_ITEM_NAME        "LOCK"

/** DSLR_MIRROR_LOCKUP.UNLOCK property item name.
 */
#define DSLR_MIRROR_LOCKUP_UNLOCK_ITEM_NAME        "UNLOCK"

//----------------------------------------------------------------------
/** DSLR_AVOID_AF property name.
 */
#define DSLR_AVOID_AF_PROPERTY_NAME		"DSLR_AVOID_AF"

/** DSLR_AVOID_AF.ON property item name.
 */
#define DSLR_AVOID_AF_ON_ITEM_NAME        "ON"

/** DSLR_AVOID_AF.OFF property item name.
 */
#define DSLR_AVOID_AF_OFF_ITEM_NAME        "OFF"

//----------------------------------------------------------------------
/** DSLR_STREAMING_MODE property name.
 */
#define DSLR_STREAMING_MODE_PROPERTY_NAME   "DSLR_STREAMING_MODE"

/** DSLR_STREAMING_MODE.LIVE_VIEW property item name.
 */
#define DSLR_STREAMING_LIVE_VIEW_ITEM_NAME   "LIVE_VIEW"

/** DSLR_STREAMING_MODE.BURST_MODE property item name.
 */
#define DSLR_STREAMING_BURST_MODE_ITEM_NAME  "BURST_MODE"

//----------------------------------------------------------------------
/** DSLR_ZOOM_PREVIEW property name.
 */
#define DSLR_ZOOM_PREVIEW_PROPERTY_NAME		"DSLR_ZOOM_PREVIEW"

/** DSLR_ZOOM_PREVIEW.ON property item name.
 */
#define DSLR_ZOOM_PREVIEW_ON_ITEM_NAME        "ON"

/** DSLR_ZOOM_PREVIEW.OFF property item name.
 */
#define DSLR_ZOOM_PREVIEW_OFF_ITEM_NAME        "OFF"

//----------------------------------------------------------------------
/** DSLR_DELETE_IMAGE property name.
 */
#define DSLR_DELETE_IMAGE_PROPERTY_NAME		"DSLR_DELETE_IMAGE"

/** DSLR_DELETE_IMAGE.ON property item name.
 */
#define DSLR_DELETE_IMAGE_ON_ITEM_NAME        "ON"

/** DSLR_DELETE_IMAGE.OFF property item name.
 */
#define DSLR_DELETE_IMAGE_OFF_ITEM_NAME        "OFF"

//----------------------------------------------------------------------
/** DSLR_AF property name.
 */
#define DSLR_AF_PROPERTY_NAME		"DSLR_AF"

/** DSLR_AF.AF property item name.
 */
#define DSLR_AF_ITEM_NAME        "AF"

//----------------------------------------------------------------------
/** DSLR_SET_HOST_TIME property name.
 */
#define DSLR_SET_HOST_TIME_PROPERTY_NAME				"DSLR_SET_HOST_TIME"

/**  DSLR_SET_HOST_TIME.SET property item name.
 */
#define DSLR_SET_HOST_TIME_ITEM_NAME						"SET"

//----------------------------------------------------------------------
/** GUIDER_GUIDE_DEC property name.
 */
#define GUIDER_GUIDE_DEC_PROPERTY_NAME        "GUIDER_GUIDE_DEC"

/** GUIDER_GUIDE_DEC.NORTH property item name.
 */
#define GUIDER_GUIDE_NORTH_ITEM_NAME          "NORTH"

/** GUIDER_GUIDE_DEC.SOUTH property item name.
 */
#define GUIDER_GUIDE_SOUTH_ITEM_NAME          "SOUTH"

//----------------------------------------------------------------------
/** GUIDER_GUIDE_RA property name.
 */
#define GUIDER_GUIDE_RA_PROPERTY_NAME         "GUIDER_GUIDE_RA"

/** GUIDER_GUIDE_RA.EAST property item name.
 */
#define GUIDER_GUIDE_EAST_ITEM_NAME           "EAST"

/** GUIDER_GUIDE_RA.WEST property item name.
 */
#define GUIDER_GUIDE_WEST_ITEM_NAME           "WEST"

//----------------------------------------------------------------------
/** GUIDER_RATE property name.
 */
#define GUIDER_RATE_PROPERTY_NAME       			"GUIDER_RATE"

/** GUIDER_RATE.RATE property item name.
 */
#define GUIDER_RATE_ITEM_NAME           			"RATE"
#define GUIDER_DEC_RATE_ITEM_NAME           	"DEC_RATE"

//----------------------------------------------------------------------
/** AO_GUIDE_DEC property name
 */
#define AO_GUIDE_DEC_PROPERTY_NAME           "AO_GUIDE_DEC"

/** AO_GUIDE_DEC.NORTH property item name.
 */
#define AO_GUIDE_NORTH_ITEM_NAME             "NORTH"

/** AO_GUIDE_DEC.SOUTH property item name.
 */
#define AO_GUIDE_SOUTH_ITEM_NAME             "SOUTH"

//----------------------------------------------------------------------
/** AO_GUIDE_RA property name.
 */
#define AO_GUIDE_RA_PROPERTY_NAME            "AO_GUIDE_RA"

/** AO_GUIDE_RA.EAST property item name.
 */
#define AO_GUIDE_EAST_ITEM_NAME              "EAST"

/** AO_GUIDE_RA.WEST property item name.
 */
#define AO_GUIDE_WEST_ITEM_NAME              "WEST"

//----------------------------------------------------------------------
/** AO_RESET property pointer name.
 */
#define AO_RESET_PROPERTY_NAME         				"AO_RESET"

/** AO_RESET.CENTER property item name.
 */
#define AO_CENTER_ITEM_NAME            "CENTER"

/** AO_RESET.UNJAM property item name.
 */
#define AO_UNJAM_ITEM_NAME             "UNJAM"

//----------------------------------------------------------------------
/** WHEEL_SLOT property name.
 */
#define WHEEL_SLOT_PROPERTY_NAME							"WHEEL_SLOT"

/** WHEEL_SLOT.SLOT property item name.
 */
#define WHEEL_SLOT_ITEM_NAME									"SLOT"

//----------------------------------------------------------------------
/** WHEEL_SLOT_NAME property name.
 */
#define WHEEL_SLOT_NAME_PROPERTY_NAME					"WHEEL_SLOT_NAME"

/** WHEEL_SLOT_NAME.NAME_%d property item name.
 */
#define WHEEL_SLOT_NAME_ITEM_NAME							"SLOT_NAME_%d"

/** WHEEL_SLOT_NAME.NAME_1 property item name.
 */
#define WHEEL_SLOT_NAME_1_ITEM_NAME						"SLOT_NAME_1"

/** WHEEL_SLOT_NAME.NAME_2 property item name.
 */
#define WHEEL_SLOT_NAME_2_ITEM_NAME						"SLOT_NAME_2"

/** WHEEL_SLOT_NAME.NAME_3 property item name.
 */
#define WHEEL_SLOT_NAME_3_ITEM_NAME						"SLOT_NAME_3"

/** WHEEL_SLOT_NAME.NAME_4 property item name.
 */
#define WHEEL_SLOT_NAME_4_ITEM_NAME						"SLOT_NAME_4"

/** WHEEL_SLOT_NAME.NAME_5 property item name.
 */
#define WHEEL_SLOT_NAME_5_ITEM_NAME						"SLOT_NAME_5"

/** WHEEL_SLOT_NAME.NAME_6 property item name.
 */
#define WHEEL_SLOT_NAME_6_ITEM_NAME						"SLOT_NAME_6"

/** WHEEL_SLOT_NAME.NAME_7 property item name.
 */
#define WHEEL_SLOT_NAME_7_ITEM_NAME						"SLOT_NAME_7"

/** WHEEL_SLOT_NAME.NAME_8 property item name.
 */
#define WHEEL_SLOT_NAME_8_ITEM_NAME						"SLOT_NAME_8"

/** WHEEL_SLOT_NAME.NAME_9 property item name.
 */
#define WHEEL_SLOT_NAME_9_ITEM_NAME						"SLOT_NAME_9"

/** WHEEL_SLOT_NAME.NAME_10 property item name.
 */
#define WHEEL_SLOT_NAME_10_ITEM_NAME						"SLOT_NAME_10"

/** WHEEL_SLOT_NAME.NAME_11 property item name.
 */
#define WHEEL_SLOT_NAME_11_ITEM_NAME						"SLOT_NAME_11"

/** WHEEL_SLOT_NAME.NAME_12 property item name.
 */
#define WHEEL_SLOT_NAME_12_ITEM_NAME						"SLOT_NAME_12"

/** WHEEL_SLOT_NAME.NAME_3 property item name.
 */
#define WHEEL_SLOT_NAME_13_ITEM_NAME						"SLOT_NAME_13"

/** WHEEL_SLOT_NAME.NAME_4 property item name.
 */
#define WHEEL_SLOT_NAME_14_ITEM_NAME						"SLOT_NAME_14"

/** WHEEL_SLOT_NAME.NAME_15 property item name.
 */
#define WHEEL_SLOT_NAME_15_ITEM_NAME						"SLOT_NAME_15"

/** WHEEL_SLOT_NAME.NAME_16 property item name.
 */
#define WHEEL_SLOT_NAME_16_ITEM_NAME						"SLOT_NAME_16"

/** WHEEL_SLOT_NAME.NAME_7 property item name.
 */
#define WHEEL_SLOT_NAME_17_ITEM_NAME						"SLOT_NAME_17"

/** WHEEL_SLOT_NAME.NAME_18 property item name.
 */
#define WHEEL_SLOT_NAME_18_ITEM_NAME						"SLOT_NAME_18"

/** WHEEL_SLOT_NAME.NAME_19 property item name.
 */
#define WHEEL_SLOT_NAME_19_ITEM_NAME						"SLOT_NAME_19"

/** WHEEL_SLOT_NAME.NAME_20 property item name.
 */
#define WHEEL_SLOT_NAME_20_ITEM_NAME						"SLOT_NAME_20"

/** WHEEL_SLOT_NAME.NAME_21 property item name.
 */
#define WHEEL_SLOT_NAME_21_ITEM_NAME						"SLOT_NAME_21"

/** WHEEL_SLOT_NAME.NAME_22 property item name.
 */
#define WHEEL_SLOT_NAME_22_ITEM_NAME						"SLOT_NAME_22"

/** WHEEL_SLOT_NAME.NAME_23 property item name.
 */
#define WHEEL_SLOT_NAME_23_ITEM_NAME						"SLOT_NAME_23"

/** WHEEL_SLOT_NAME.NAME_24 property item name.
 */
#define WHEEL_SLOT_NAME_24_ITEM_NAME						"SLOT_NAME_24"

//----------------------------------------------------------------------
/** WHEEL_SLOT_OFFSET property name.
 */
#define WHEEL_SLOT_OFFSET_PROPERTY_NAME					"WHEEL_SLOT_OFFSET"

/** WHEEL_SLOT_OFFSET.OFFSET_%d property item name.
 */
#define WHEEL_SLOT_OFFSET_ITEM_NAME						"SLOT_OFFSET_%d"

/** WHEEL_SLOT_OFFSET.OFFSET_1 property item name.
 */
#define WHEEL_SLOT_OFFSET_1_ITEM_NAME						"SLOT_OFFSET_1"

/** WHEEL_SLOT_OFFSET.OFFSET_2 property item name.
 */
#define WHEEL_SLOT_OFFSET_2_ITEM_NAME						"SLOT_OFFSET_2"

/** WHEEL_SLOT_OFFSET.OFFSET_3 property item name.
 */
#define WHEEL_SLOT_OFFSET_3_ITEM_NAME						"SLOT_OFFSET_3"

/** WHEEL_SLOT_OFFSET.OFFSET_4 property item name.
 */
#define WHEEL_SLOT_OFFSET_4_ITEM_NAME						"SLOT_OFFSET_4"

/** WHEEL_SLOT_OFFSET.OFFSET_5 property item name.
 */
#define WHEEL_SLOT_OFFSET_5_ITEM_NAME						"SLOT_OFFSET_5"

/** WHEEL_SLOT_OFFSET.OFFSET_6 property item name.
 */
#define WHEEL_SLOT_OFFSET_6_ITEM_NAME						"SLOT_OFFSET_6"

/** WHEEL_SLOT_OFFSET.OFFSET_7 property item name.
 */
#define WHEEL_SLOT_OFFSET_7_ITEM_NAME						"SLOT_OFFSET_7"

/** WHEEL_SLOT_OFFSET.OFFSET_8 property item name.
 */
#define WHEEL_SLOT_OFFSET_8_ITEM_NAME						"SLOT_OFFSET_8"

/** WHEEL_SLOT_OFFSET.OFFSET_9 property item name.
 */
#define WHEEL_SLOT_OFFSET_9_ITEM_NAME						"SLOT_OFFSET_9"

/** WHEEL_SLOT_OFFSET.OFFSET_10 property item name.
 */
#define WHEEL_SLOT_OFFSET_10_ITEM_NAME						"SLOT_OFFSET_10"

/** WHEEL_SLOT_OFFSET.OFFSET_11 property item name.
 */
#define WHEEL_SLOT_OFFSET_11_ITEM_NAME						"SLOT_OFFSET_11"

/** WHEEL_SLOT_OFFSET.OFFSET_12 property item name.
 */
#define WHEEL_SLOT_OFFSET_12_ITEM_NAME						"SLOT_OFFSET_12"

/** WHEEL_SLOT_OFFSET.OFFSET_3 property item name.
 */
#define WHEEL_SLOT_OFFSET_13_ITEM_NAME						"SLOT_OFFSET_13"

/** WHEEL_SLOT_OFFSET.OFFSET_4 property item name.
 */
#define WHEEL_SLOT_OFFSET_14_ITEM_NAME						"SLOT_OFFSET_14"

/** WHEEL_SLOT_OFFSET.OFFSET_15 property item name.
 */
#define WHEEL_SLOT_OFFSET_15_ITEM_NAME						"SLOT_OFFSET_15"

/** WHEEL_SLOT_OFFSET.OFFSET_16 property item name.
 */
#define WHEEL_SLOT_OFFSET_16_ITEM_NAME						"SLOT_OFFSET_16"

/** WHEEL_SLOT_OFFSET.OFFSET_7 property item name.
 */
#define WHEEL_SLOT_OFFSET_17_ITEM_NAME						"SLOT_OFFSET_17"

/** WHEEL_SLOT_OFFSET.OFFSET_18 property item name.
 */
#define WHEEL_SLOT_OFFSET_18_ITEM_NAME						"SLOT_OFFSET_18"

/** WHEEL_SLOT_OFFSET.OFFSET_19 property item name.
 */
#define WHEEL_SLOT_OFFSET_19_ITEM_NAME						"SLOT_OFFSET_19"

/** WHEEL_SLOT_OFFSET.OFFSET_20 property item name.
 */
#define WHEEL_SLOT_OFFSET_20_ITEM_NAME						"SLOT_OFFSET_20"

/** WHEEL_SLOT_OFFSET.OFFSET_21 property item name.
 */
#define WHEEL_SLOT_OFFSET_21_ITEM_NAME						"SLOT_OFFSET_21"

/** WHEEL_SLOT_OFFSET.OFFSET_22 property item name.
 */
#define WHEEL_SLOT_OFFSET_22_ITEM_NAME						"SLOT_OFFSET_22"

/** WHEEL_SLOT_OFFSET.OFFSET_23 property item name.
 */
#define WHEEL_SLOT_OFFSET_23_ITEM_NAME						"SLOT_OFFSET_23"

/** WHEEL_SLOT_OFFSET.OFFSET_24 property item name.
 */
#define WHEEL_SLOT_OFFSET_24_ITEM_NAME						"SLOT_OFFSET_24"


//----------------------------------------------------------------------
/** FOCUSER_SPEED property name.
 */
#define FOCUSER_SPEED_PROPERTY_NAME						"FOCUSER_SPEED"

/** FOCUSER_SPEED.SPEED property item name.
 */
#define FOCUSER_SPEED_ITEM_NAME								"SPEED"

//----------------------------------------------------------------------

/** FOCUSER_REVERSE_MOTION property name.
 */
#define FOCUSER_REVERSE_MOTION_PROPERTY_NAME					"FOCUSER_REVERSE_MOTION"

/** FOCUSER_REVERSE_MOTION.DISABLED property item name.
 */
#define FOCUSER_REVERSE_MOTION_DISABLED_ITEM_NAME				"DISABLED"

/** FOCUSER_REVERSE_MOTION.ENABLED property item name.
 */
#define FOCUSER_REVERSE_MOTION_ENABLED_ITEM_NAME				"ENABLED"

//----------------------------------------------------------------------
/** FOCUSER_DIRECTION property name.
 */
#define FOCUSER_DIRECTION_PROPERTY_NAME				"FOCUSER_DIRECTION"

/** FOCUSER_DIRECTION.MOVE_INWARD property item name.
 */
#define FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME		"MOVE_INWARD"

/** FOCUSER_DIRECTION.MOVE_OUTWARD property item name.
 */
#define FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME	"MOVE_OUTWARD"

//----------------------------------------------------------------------
/** FOCUSER_STEPS property name.
 */
#define FOCUSER_STEPS_PROPERTY_NAME						"FOCUSER_STEPS"

/** FOCUSER_STEPS.STEPS property item name.
 */
#define FOCUSER_STEPS_ITEM_NAME								"STEPS"

//----------------------------------------------------------------------
/** FOCUSER_ON_POSITION_SET property name.
 */
#define FOCUSER_ON_POSITION_SET_PROPERTY_NAME	"FOCUSER_ON_POSITION_SET"

/** FOCUSER_ON_POSITION_SET.GOTO property item name.
 */
#define FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME	"GOTO"

/** FOCUSER_ON_POSITION_SET.SYNC property item name.
 */
#define FOCUSER_ON_POSITION_SET_SYNC_ITEM_NAME		"SYNC"

//----------------------------------------------------------------------
/** FOCUSER_POSITION property name.
 */
#define FOCUSER_POSITION_PROPERTY_NAME				"FOCUSER_POSITION"

/** FOCUSER_POSITION.POSITION property item name.
 */
#define FOCUSER_POSITION_ITEM_NAME						"POSITION"

//----------------------------------------------------------------------
/** FOCUSER_ABORT_MOTION property name.
 */
#define FOCUSER_ABORT_MOTION_PROPERTY_NAME		"FOCUSER_ABORT_MOTION"

/** FOCUSER_ABORT_MOTION.ABORT_MOTION property item name.
 */
#define FOCUSER_ABORT_MOTION_ITEM_NAME				"ABORT_MOTION"

//----------------------------------------------------------------------
/** FOCUSER_BACKLASH property name.
 */
#define FOCUSER_BACKLASH_PROPERTY_NAME			"FOCUSER_BACKLASH"

/** FOCUSER_BACKLASH.BACKLASH property item name.
 */
#define FOCUSER_BACKLASH_ITEM_NAME					"BACKLASH"

//----------------------------------------------------------------------
/** FOCUSER_TEMPERATURE property name.
 */
#define FOCUSER_TEMPERATURE_PROPERTY_NAME		"FOCUSER_TEMPERATURE"

/** FOCUSER_TEMPERATURE.TEMPERATURE property item name.
 */
#define FOCUSER_TEMPERATURE_ITEM_NAME				"TEMPERATURE"

//----------------------------------------------------------------------
/** FOCUSER_COMPENSATION property name.
 */
#define FOCUSER_COMPENSATION_PROPERTY_NAME		"FOCUSER_COMPENSATION"

/** FOCUSER_COMPENSATION.COMPENSATION property item name.
 */
#define FOCUSER_COMPENSATION_ITEM_NAME				"COMPENSATION"

/** FOCUSER_COMPENSATION.PERIOD property item name.
 */
#define FOCUSER_COMPENSATION_PERIOD_ITEM_NAME		"PERIOD"

/** FOCUSER_COMPENSATION.THRESHOLD property item name.
 */
#define FOCUSER_COMPENSATION_THRESHOLD_ITEM_NAME		"THRESHOLD"

//----------------------------------------------------------------------
/** FOCUSER_MODE property name.
 */
#define FOCUSER_MODE_PROPERTY_NAME						"FOCUSER_MODE"

/** FOCUSER_MODE.MANUAL property item name.
 */
#define FOCUSER_MODE_MANUAL_ITEM_NAME					"MANUAL"

/** FOCUSER_MODE.AUTOMATIC property item name.
 */
#define FOCUSER_MODE_AUTOMATIC_ITEM_NAME			"AUTOMATIC"

//----------------------------------------------------------------------
/** FOCUSER_LIMITS property name.
 */
#define FOCUSER_LIMITS_PROPERTY_NAME						"FOCUSER_LIMITS"

/** FOCUSER_LIMITS.MIN_POSITION property item name.
 */
#define FOCUSER_LIMITS_MIN_POSITION_ITEM_NAME					"MIN_POSITION"

/** FOCUSER_LIMITS.MIN_POSITION property item name.
 */
#define FOCUSER_LIMITS_MAX_POSITION_ITEM_NAME		"MAX_POSITION"

//----------------------------------------------------------------------
/** ROTATOR_ON_POSITION_SET property name.
 */
#define ROTATOR_ON_POSITION_SET_PROPERTY_NAME	"ROTATOR_ON_POSITION_SET"

/** ROTATOR_ON_POSITION_SET.GOTO property item name.
 */
#define ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME	"GOTO"

/** ROTATOR_ON_POSITION_SET.SYNC property item name.
 */
#define ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME		"SYNC"

//----------------------------------------------------------------------
/** ROTATOR_POSITION property name.
 */
#define ROTATOR_POSITION_PROPERTY_NAME				"ROTATOR_POSITION"

/** ROTATOR_POSITION.POSITION property item name.
 */
#define ROTATOR_POSITION_ITEM_NAME						"POSITION"


/** ROTATOR_DIRECTION property name.
 */
#define ROTATOR_DIRECTION_PROPERTY_NAME					"ROTATOR_DIRECTION"

/** ROTATOR_DIRECTION.NORMAL property item name.
 */
#define ROTATOR_DIRECTION_NORMAL_ITEM_NAME				"NORMAL"

/** ROTATOR_DIRECTION.REVERSED property item name.
 */
#define ROTATOR_DIRECTION_REVERSED_ITEM_NAME       "REVERSED"



//----------------------------------------------------------------------
/** ROTATOR_ABORT_MOTION property name.
 */
#define ROTATOR_ABORT_MOTION_PROPERTY_NAME		"ROTATOR_ABORT_MOTION"

/** ROTATOR_ABORT_MOTION.ABORT_MOTION property item name.
 */
#define ROTATOR_ABORT_MOTION_ITEM_NAME				"ABORT_MOTION"

//----------------------------------------------------------------------
/** ROTATOR_BACKLASH property name.
 */
#define ROTATOR_BACKLASH_PROPERTY_NAME			"ROTATOR_BACKLASH"

/** ROTATOR_BACKLASH.BACKLASH property item name.
 */
#define ROTATOR_BACKLASH_ITEM_NAME					"BACKLASH"

//----------------------------------------------------------------------
/** ROTATOR_LIMITS property name.
 */
#define ROTATOR_LIMITS_PROPERTY_NAME						"ROTATOR_LIMITS"

/** ROTATOR_LIMITS.MIN_POSITION property item name.
 */
#define ROTATOR_LIMITS_MIN_POSITION_ITEM_NAME					"MIN_POSITION"

/** ROTATOR_LIMITS.MIN_POSITION property item name.
 */
#define ROTATOR_LIMITS_MAX_POSITION_ITEM_NAME		"MAX_POSITION"

//----------------------------------------------------------------------
/** ROTATOR_STEPS_PER_REVOLUTION property name.
 */
#define ROTATOR_STEPS_PER_REVOLUTION_PROPERTY_NAME						"ROTATOR_STEPS_PER_REVOLUTION"

/** ROTATOR_STEPS_PER_REVOLUTION.STEPS_PER_REVOLUTION property item name.
 */
#define ROTATOR_STEPS_PER_REVOLUTION_ITEM_NAME		"STEPS_PER_REVOLUTION"


//----------------------------------------------------------------------

/** MOUNT_INFO property name.
 */
#define MOUNT_INFO_PROPERTY_NAME							"MOUNT_INFO"

/** MOUNT_INFO.MODEL property item name.
 */
#define MOUNT_INFO_MODEL_ITEM_NAME						"MODEL"

/** MOUNT_INFO.VENDOR property item name.
 */
#define MOUNT_INFO_VENDOR_ITEM_NAME						"VENDOR"

/** MOUNT_INFO.FIRMWARE property item name.
 */
#define MOUNT_INFO_FIRMWARE_ITEM_NAME					"FIRMWARE_VERSION"

//----------------------------------------------------------------------
/** MOUNT_LST_TIME property name.
 */
#define MOUNT_LST_TIME_PROPERTY_NAME						"MOUNT_LST_TIME"

/** MOUNT_TIME.TIME property item name.
 */
#define MOUNT_LST_TIME_ITEM_NAME								"TIME"

//----------------------------------------------------------------------
/** MOUNT_SET_HOST_TIME property name.
 */
#define MOUNT_SET_HOST_TIME_PROPERTY_NAME				"MOUNT_SET_HOST_TIME"

/**  MOUNT_SET_HOST_TIME.SET property item name.
 */
#define MOUNT_SET_HOST_TIME_ITEM_NAME						"SET"

//----------------------------------------------------------------------
/** MOUNT_PARK property name.
 */
#define MOUNT_PARK_PROPERTY_NAME								"MOUNT_PARK"

/** MOUNT_PARK.PARKED property item name.
 */
#define MOUNT_PARK_PARKED_ITEM_NAME							"PARKED"

/** MOUNT_PARK.UNPARKED property item name.
 */
#define MOUNT_PARK_UNPARKED_ITEM_NAME						"UNPARKED"

//----------------------------------------------------------------------
/** MOUNT_PARK_SET property name.
 */
#define MOUNT_PARK_SET_PROPERTY_NAME						"MOUNT_PARK_SET"

/** MOUNT_PARK_SET.DEFAULT property item name.
 */
#define MOUNT_PARK_SET_DEFAULT_ITEM_NAME				"DEFAULT"

/** MOUNT_PARK_SET.CURRENT property item name.
 */
#define MOUNT_PARK_SET_CURRENT_ITEM_NAME				"CURRENT"

//----------------------------------------------------------------------
/** MOUNT_PARK_POSITION property name.
 */
#define MOUNT_PARK_POSITION_PROPERTY_NAME				"MOUNT_PARK_POSITION"

/** MOUNT_PARK_POSITION.HA property item name.
 */
#define MOUNT_PARK_POSITION_HA_ITEM_NAME				"HA"

/** MOUNT_PARK_POSITION.DEC property item name.
 */
#define MOUNT_PARK_POSITION_DEC_ITEM_NAME				"DEC"

//----------------------------------------------------------------------
/** MOUNT_HOME property name.
 */
#define MOUNT_HOME_PROPERTY_NAME								"MOUNT_HOME"

/** MOUNT_HOME.HOMEED property item name.
 */
#define MOUNT_HOME_ITEM_NAME							      "HOME"

//----------------------------------------------------------------------
/** MOUNT_HOME_SET property name.
 */
#define MOUNT_HOME_SET_PROPERTY_NAME						"MOUNT_HOME_SET"

/** MOUNT_HOME_SET.DEFAULT property item name.
 */
#define MOUNT_HOME_SET_DEFAULT_ITEM_NAME				"DEFAULT"

/** MOUNT_HOME_SET.CURRENT property item name.
 */
#define MOUNT_HOME_SET_CURRENT_ITEM_NAME				"CURRENT"

//----------------------------------------------------------------------
/** MOUNT_HOME_POSITION property name.
 */
#define MOUNT_HOME_POSITION_PROPERTY_NAME				"MOUNT_HOME_POSITION"

/** MOUNT_HOME_POSITION.HA property item name.
 */
#define MOUNT_HOME_POSITION_HA_ITEM_NAME				"HA"

/** MOUNT_HOME_POSITION.DEC property item name.
 */
#define MOUNT_HOME_POSITION_DEC_ITEM_NAME				"DEC"
//----------------------------------------------------------------------
/** MOUNT_ON_COORDINATES_SET property name.
 */
#define MOUNT_ON_COORDINATES_SET_PROPERTY_NAME	"MOUNT_ON_COORDINATES_SET"

/** MOUNT_ON_COORDINATES_SET.TRACK property item name.
 */
#define MOUNT_ON_COORDINATES_SET_TRACK_ITEM_NAME	"TRACK"

/** MOUNT_ON_COORDINATES_SET.SYNC property item name.
 */
#define MOUNT_ON_COORDINATES_SET_SYNC_ITEM_NAME		"SYNC"

/** MOUNT_ON_COORDINATES_SET.SLEW property item name.
 */
#define MOUNT_ON_COORDINATES_SET_SLEW_ITEM_NAME		"SLEW"

//----------------------------------------------------------------------
/** MOUNT_SLEW_RATE property name.
 */
#define MOUNT_SLEW_RATE_PROPERTY_NAME						"MOUNT_SLEW_RATE"

/** MOUNT_SLEW_RATE.GUIDE property item name.
 */
#define MOUNT_SLEW_RATE_GUIDE_ITEM_NAME					"GUIDE"

/** MOUNT_SLEW_RATE.CENTERING property item name.
 */
#define MOUNT_SLEW_RATE_CENTERING_ITEM_NAME			"CENTERING"

/** MOUNT_SLEW_RATE.FIND property item name.
 */
#define MOUNT_SLEW_RATE_FIND_ITEM_NAME					"FIND"

/** MOUNT_SLEW_RATE.MAX property item name.
 */
#define MOUNT_SLEW_RATE_MAX_ITEM_NAME						"MAX"

//----------------------------------------------------------------------
/** MOUNT_MOTION_DEC_PROPERTY_NAME property name.
 */
#define MOUNT_MOTION_DEC_PROPERTY_NAME                "MOUNT_MOTION_DEC"

/** MOUNT_MOTION_DEC.NORTH property item name.
 */
#define MOUNT_MOTION_NORTH_ITEM_NAME                 "NORTH"

/** MOUNT_MOTION_DEC.SOUTH property item name.
 */
#define MOUNT_MOTION_SOUTH_ITEM_NAME                 "SOUTH"

//----------------------------------------------------------------------
/** MOUNT_MOTION_RA_PROPERTY_NAME property name.
 */
#define MOUNT_MOTION_RA_PROPERTY_NAME                "MOUNT_MOTION_RA"

/** MOUNT_MOTION_RA.WEST property item name.
 */
#define MOUNT_MOTION_WEST_ITEM_NAME                 "WEST"

/** MOUNT_MOTION_RA.EAST property item name.
 */
#define MOUNT_MOTION_EAST_ITEM_NAME                 "EAST"

//----------------------------------------------------------------------
/** MOUNT_TRACK_RATE property name.
 */
#define MOUNT_TRACK_RATE_PROPERTY_NAME					"MOUNT_TRACK_RATE"

/** MOUNT_TRACK_RATE.SIDEREAL property item name.
 */
#define MOUNT_TRACK_RATE_SIDEREAL_ITEM_NAME			"SIDEREAL"

/** MOUNT_TRACK_RATE.SOLAR property item name.
 */
#define MOUNT_TRACK_RATE_SOLAR_ITEM_NAME				"SOLAR"

/** MOUNT_TRACK_RATE.LUNAR property item name.
 */
#define MOUNT_TRACK_RATE_LUNAR_ITEM_NAME				"LUNAR"

/** MOUNT_TRACK_RATE.KING property item name.
 */
#define MOUNT_TRACK_RATE_KING_ITEM_NAME			"KING"

/** MOUNT_TRACK_RATE.CUSTOM property item name.
 */
#define MOUNT_TRACK_RATE_CUSTOM_ITEM_NAME			"CUSTOM"


//----------------------------------------------------------------------
/** MOUNT_TRACKING property name.
 */
#define MOUNT_TRACKING_PROPERTY_NAME						"MOUNT_TRACKING"

/** MOUNT_TRACKING.ON property item name.
 */
#define MOUNT_TRACKING_ON_ITEM_NAME							"ON"

/** MOUNT_TRACKING.OFF property item name.
 */
#define MOUNT_TRACKING_OFF_ITEM_NAME						"OFF"

//----------------------------------------------------------------------
/** MOUNT_GUIDE_RATE property name.
 */
#define MOUNT_GUIDE_RATE_PROPERTY_NAME					"MOUNT_GUIDE_RATE"

/** MOUNT_GUIDE_RATE.RA property item name.
 */
#define MOUNT_GUIDE_RATE_RA_ITEM_NAME						"RA"

/** MOUNT_GUIDE_RATE.DEC property item name.
 */
#define MOUNT_GUIDE_RATE_DEC_ITEM_NAME					"DEC"

//----------------------------------------------------------------------

#define MOUNT_CUSTOM_TRACKING_RATE_PROPERTY_NAME	"MOUNT_CUSTOM_TRACKING_RATE"

/** MOUNT_CUSTOM_TRACKING_RATE.RATE property item name.
 */
#define MOUNT_CUSTOM_TRACKING_RATE_ITEM_NAME			"RATE"

//----------------------------------------------------------------------
/** MOUNT_GEOGRAPHIC_COORDINATES property name.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY_NAME	"MOUNT_GEOGRAPHIC_COORDINATES"

/** MOUNT_GEOGRAPHIC_COORDINATES.LATITUDE property item name.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME	"LATITUDE"

/** MOUNT_GEOGRAPHIC_COORDINATES.LONGITUDE property item name.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME	"LONGITUDE"

/** MOUNT_GEOGRAPHIC_COORDINATES.ELEVATION property item name.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME	"ELEVATION"

/** MOUNT_GEOGRAPHIC_COORDINATES.ACCURACY property item name.
 */
#define MOUNT_GEOGRAPHIC_COORDINATES_ACCURACY_ITEM_NAME	"ACCURACY"

//----------------------------------------------------------------------
/** MOUNT_EQUATORIAL_COORDINATES property name.
 */
#define MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME	"MOUNT_EQUATORIAL_COORDINATES"

/** MOUNT_EQUATORIAL_COORDINATES.RA property item name.
 */
#define MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME		"RA"

/** MOUNT_EQUATORIAL_COORDINATES.DEC property item name.
 */
#define MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME	"DEC"

//----------------------------------------------------------------------
/** MOUNT_HORIZONTAL_COORDINATES property name.
 */
#define MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME	"MOUNT_HORIZONTAL_COORDINATES"

/** MOUNT_HORIZONTAL_COORDINATES.ALT property item name.
 */
#define MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME	"ALT"

/** MOUNT_HORIZONTAL_COORDINATES.AZ property item name.
 */
#define MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME		"AZ"

//----------------------------------------------------------------------
/** MOUNT_RAW_COORDINATES property name.
 */
#define MOUNT_RAW_COORDINATES_PROPERTY_NAME			"MOUNT_RAW_COORDINATES"

/** MOUNT_RAW_COORDINATES.RA property item name.
 */
#define MOUNT_RAW_COORDINATES_RA_ITEM_NAME				"RA"

/** MOUNT_RAW_COORDINATES.DEC property item name.
 */
#define MOUNT_RAW_COORDINATES_DEC_ITEM_NAME			"DEC"

//----------------------------------------------------------------------
/** MOUNT_ABORT_MOTION property name.
 */
#define MOUNT_ABORT_MOTION_PROPERTY_NAME				"MOUNT_ABORT_MOTION"

/** MOUN_ABORT_MOTION.ABORT_MOTION property item name.
 */
#define MOUNT_ABORT_MOTION_ITEM_NAME						"ABORT_MOTION"

//----------------------------------------------------------------------
/** MOUNT_ALIGNMENT_MODE property name.
 */
#define MOUNT_ALIGNMENT_MODE_PROPERTY_NAME			"MOUNT_ALIGNMENT_MODE"

/** MOUNT_ALIGNMENT_MODE.CONTROLLER property item name.
 */
#define MOUNT_ALIGNMENT_MODE_CONTROLLER_ITEM_NAME		"CONTROLLER"

/** MOUNT_ALIGNMENT_MODE.SINGLE_POINT property item name.
 */
#define MOUNT_ALIGNMENT_MODE_SINGLE_POINT_ITEM_NAME	"SINGLE_POINT"

/** MOUNT_ALIGNMENT_MODE.NEAREST_POINT property item name.
 */
#define MOUNT_ALIGNMENT_MODE_NEAREST_POINT_ITEM_NAME	"NEAREST_POINT"

/** MOUNT_ALIGNMENT_MODE.MULTI_POINT property item name.
 */
#define MOUNT_ALIGNMENT_MODE_MULTI_POINT_ITEM_NAME	"MULTI_POINT"

//----------------------------------------------------------------------
/** MOUNT_ALIGNMENT_SELCT_POINTS property name.
 */
#define MOUNT_ALIGNMENT_SELECT_POINTS_PROPERTY_NAME	"MOUNT_ALIGNMENT_SELECT_POINTS"

//----------------------------------------------------------------------
/** MOUNT_ALIGNMENT_DELETE_POINTS property name.
 */
#define MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY_NAME	"MOUNT_ALIGNMENT_DELETE_POINTS"

//----------------------------------------------------------------------
/** MOUNT_EPOCH property name.
 */
#define MOUNT_EPOCH_PROPERTY_NAME							"MOUNT_EPOCH"

/** MOUNT_EPOCH.EPOCH property item name.
 */
#define MOUNT_EPOCH_ITEM_NAME             		"EPOCH"

//----------------------------------------------------------------------
/** MOUNT_SIDE_OF_PIER property name.
 */
#define MOUNT_SIDE_OF_PIER_PROPERTY_NAME							"MOUNT_SIDE_OF_PIER"

/** MOUNT_SIDE_OF_PIER.EAST property item name.
 */
#define MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME             		"EAST"

/** MOUNT_SIDE_OF_PIER.WEST property item name.
 */
#define MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME             		"WEST"

//----------------------------------------------------------------------
/** MOUNT_ALIGNMENT_DELETE_POINTS property name.
 */
#define MOUNT_ALIGNMENT_DELETE_POINTS_PROPERTY_NAME	"MOUNT_ALIGNMENT_DELETE_POINTS"

//----------------------------------------------------------------------
/** MOUNT_PEC property name.
 */
#define MOUNT_PEC_PROPERTY_NAME		    					"MOUNT_PEC"
#define MOUNT_PEC_ENABLED_ITEM_NAME           		"ENABLED"
#define MOUNT_PEC_DISABLED_ITEM_NAME          		"DISABLED"

//----------------------------------------------------------------------
/** MOUNT_PEC_TRAINING property name.
 */
#define MOUNT_PEC_TRAINING_PROPERTY_NAME		    	"MOUNT_PEC_TRAINING"
#define MOUNT_PEC_TRAINIG_STARTED_ITEM_NAME      "STARTED"
#define MOUNT_PEC_TRAINIG_STOPPED_ITEM_NAME      "STOPPED"


//----------------------------------------------------------------------
/** GPS_STATUS property name.
 */
#define GPS_STATUS_PROPERTY_NAME                "GPS_STATUS"

/** GPS_STATUS.NO_FIX property item name.
 */
#define GPS_STATUS_NO_FIX_ITEM_NAME             "NO_FIX"

/** GPS_STATUS.2D_FIX property item name.
 */
#define GPS_STATUS_2D_FIX_ITEM_NAME             "2D_FIX"

/** GPS_STATUS.3D_FIX property item name.
 */
#define GPS_STATUS_3D_FIX_ITEM_NAME             "3D_FIX"

//--------------------------------------------------------------------
/** GPS_ADVANCED property name.
 */
#define GPS_ADVANCED_PROPERTY_NAME              "GPS_ADVANCED"

/** GPS_ADVANCED.ENABLED property item name.
 */
#define GPS_ADVANCED_ENABLED_ITEM_NAME          "ENABLED"

/** GPS_ADVANCED.DIABLED property item name.
 */
#define GPS_ADVANCED_DISABLED_ITEM_NAME         "DISABLED"

//------------------------------------------------------------------
/** GPS_ADVANCED_STATUS property name.
 */
#define GPS_ADVANCED_STATUS_PROPERTY_MANE              "GPS_ADVANCED_STATUS"

/** GPS_ADVANCED_STATUS.SVS_IN_USE property item name.
 */
#define GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM_NAME       "SVS_IN_USE"

/** GPS_ADVANCED_STATUS.SVS_IN_VIEW property item name.
 */
#define GPS_ADVANCED_STATUS_SVS_IN_VIEW_ITEM_NAME      "SVS_IN_VIEW"

/** GPS_ADVANCED_STATUS.PDOP property item name.
 */
#define GPS_ADVANCED_STATUS_PDOP_ITEM_NAME             "PDOP"

/** GPS_ADVANCED_STATUS.HDOP property item name.
 */
#define GPS_ADVANCED_STATUS_HDOP_ITEM_NAME             "HDOP"

/** GPS_ADVANCED_STATUS.VDOP property item name.
 */
#define GPS_ADVANCED_STATUS_VDOP_ITEM_NAME             "VDOP"

//----------------------------------------------------------------------
/** DOME_SPEED property name.
 */
#define DOME_SPEED_PROPERTY_NAME						"DOME_SPEED"

/** DOME_SPEED.SPEED property item name.
 */
#define DOME_SPEED_ITEM_NAME							"SPEED"

//----------------------------------------------------------------------

/** DOME_ROTATION property name.
 */
#define DOME_ROTATION_PROPERTY_NAME						"DOME_ROTATION"

/** DOME_ROTATION.CLOCKWISE property item name.
 */
#define DOME_ROTATION_CLOCKWISE_ITEM_NAME				"CLOCKWISE"

/** DOME_ROTATION.COUNTERCLOCKWISE property item name.
 */
#define DOME_ROTATION_COUNTERCLOCKWISE_ITEM_NAME		"COUNTERCLOCKWISE"

//----------------------------------------------------------------------
/** DOME_DIRECTION property name.
 */
#define DOME_DIRECTION_PROPERTY_NAME					"DOME_DIRECTION"

/** DOME_DIRECTION.MOVE_CLOCKWISE property item name.
 */
#define DOME_DIRECTION_MOVE_CLOCKWISE_ITEM_NAME			"MOVE_CLOCKWISE"

/** DOME_DIRECTION.MOVE_COUNTERCLOCKWISE property item name.
 */
#define DOME_DIRECTION_MOVE_COUNTERCLOCKWISE_ITEM_NAME	"MOVE_COUNTERCLOCKWISE"

//----------------------------------------------------------------------
/** DOME_ON_HORIZONTAL_COORDINATES_SET property name.
 */
#define DOME_ON_HORIZONTAL_COORDINATES_SET_PROPERTY_NAME	"DOME_ON_HORIZONTAL_COORDINATES_SET"

/** DOME_ON_HORIZONTAL_COORDINATES_SET.GOTO property item name.
 */
#define DOME_ON_HORIZONTAL_COORDINATES_SET_GOTO_ITEM_NAME	"GOTO"

/**  DOME_ON_HORIZONTAL_COORDINATES_SET.SYNC property item name.
 */
#define DOME_ON_HORIZONTAL_COORDINATES_SET_SYNC_ITEM_NAME	"SYNC"

//----------------------------------------------------------------------
/** DOME_STEPS property name.
 */
#define DOME_STEPS_PROPERTY_NAME						"DOME_STEPS"

/** DOME_STEPS.STEPS property item name.
 */
#define DOME_STEPS_ITEM_NAME							"STEPS"

//----------------------------------------------------------------------
/** DOME_EQUATORIAL_COORDINATES property name.
 */
#define DOME_EQUATORIAL_COORDINATES_PROPERTY_NAME	"DOME_EQUATORIAL_COORDINATES"

/** DOME_EQUATORIAL_COORDINATES.RA property item name.
 */
#define DOME_EQUATORIAL_COORDINATES_RA_ITEM_NAME		"RA"

/** DOME_EQUATORIAL_COORDINATES.DEC property item name.
 */
#define DOME_EQUATORIAL_COORDINATES_DEC_ITEM_NAME	"DEC"

//----------------------------------------------------------------------
/** DOME_HORIZONTAL_COORDINATES property name.
 */
#define DOME_HORIZONTAL_COORDINATES_PROPERTY_NAME						"DOME_HORIZONTAL_COORDINATES"

/** DOME_HORIZONTAL_COORDINATES.AZ property item name.
 */
#define DOME_HORIZONTAL_COORDINATES_AZ_ITEM_NAME							"AZ"

/** DOME_HORIZONTAL_COORDINATES.ALT property item name.
 */
#define DOME_HORIZONTAL_COORDINATES_ALT_ITEM_NAME							"ALT"

//----------------------------------------------------------------------
/** DOME_SLAVING property name.
 */
#define DOME_SLAVING_PROPERTY_NAME						                    "DOME_SLAVING"

/** DOME_SLAVING.ENABLE property item name.
 */
#define DOME_SLAVING_ENABLE_ITEM_NAME							"ENABLED"

/** DOME_SLAVING.DISABLE property item name.
 */
#define DOME_SLAVING_DISABLE_ITEM_NAME							"DISABLED"

//----------------------------------------------------------------------
/** DOME_SLAVING_PARAMETERS property name.
 */
#define DOME_SLAVING_PARAMETERS_PROPERTY_NAME					"DOME_SLAVING_PARAMETERS"

/** DOME_SYNC_PROPERTY.SYNC_THRESHOLD property item name.
 */
#define DOME_SLAVING_THRESHOLD_ITEM_NAME						"MOVE_THRESHOLD"

//----------------------------------------------------------------------
/** DOME_ABORT_MOTION property name.
 */
#define DOME_ABORT_MOTION_PROPERTY_NAME					"DOME_ABORT_MOTION"

/** DOME_ABORT_MOTION.ABORT_MOTION property item name.
 */
#define DOME_ABORT_MOTION_ITEM_NAME						"ABORT_MOTION"

//----------------------------------------------------------------------
/** DOME_SHUTTER property name.
 */
#define DOME_SHUTTER_PROPERTY_NAME					"DOME_SHUTTER"

/** DOME_SHUTTER.OPEN property item name.
 */
#define DOME_SHUTTER_OPENED_ITEM_NAME						"OPENED"

/** DOME_SHUTTER.CLOSE property item name.
 */
#define DOME_SHUTTER_CLOSED_ITEM_NAME						"CLOSED"

//----------------------------------------------------------------------
/** DOME_FLAP property name.
 */
#define DOME_FLAP_PROPERTY_NAME							"DOME_FLAP"

/** DOME_FLAP.OPEN property item name.
 */
#define DOME_FLAP_OPENED_ITEM_NAME						"OPENED"

/** DOME_FLAP.CLOSE property item name.
 */
#define DOME_FLAP_CLOSED_ITEM_NAME						"CLOSED"

//----------------------------------------------------------------------
/** DOME_PARK property name.
 */
#define DOME_PARK_PROPERTY_NAME								"DOME_PARK"

/** DOME_PARK.PARKED property item name.
 */
#define DOME_PARK_PARKED_ITEM_NAME							"PARKED"

/** DOME_PARK.UNPARKED property item name.
 */
#define DOME_PARK_UNPARKED_ITEM_NAME						"UNPARKED"

//----------------------------------------------------------------------
/** DOME_PARK_POSITION property name.
 */
#define DOME_PARK_POSITION_PROPERTY_NAME				"DOME_PARK_POSITION"

/** DOME_PARK_POSITION.AZ property item name.
 */
#define DOME_PARK_POSITION_AZ_ITEM_NAME				"AZ"

/** DOME_PARK_POSITION.ALT property item name.
 */
#define DOME_PARK_POSITION_ALT_ITEM_NAME				"ALT"

//----------------------------------------------------------------------
/** DOME_HOME property name.
 */
#define DOME_HOME_PROPERTY_NAME								"DOME_HOME"

/** DOME_HOME.HOME property item name.
 */
#define DOME_HOME_ITEM_NAME							      "HOME"

//----------------------------------------------------------------------
/** DOME_DIMENSION property name
 */
#define DOME_DIMENSION_PROPERTY_NAME					"DOME_DIMENSION"

/** DOME_DIMENSION.RADIUS property item name.
 */
#define DOME_RADIUS_ITEM_NAME										"RADIUS"

/** DOME_DIMENSION.SHUTTER_WIDTH property item name.
 */
#define DOME_SHUTTER_WIDTH_ITEM_NAME						"SHUTTER_WIDTH"

/** DOME_DIMENSION.MOUNT_PIVOT_OFFSET_NS property item name.
 */
#define DOME_MOUNT_PIVOT_OFFSET_NS_ITEM_NAME				"MOUNT_PIVOT_OFFSET_NS"

/** DOME_DIMENSION.MOUNT_PIVOT_OFFSET_EW property item name.
 */
#define DOME_MOUNT_PIVOT_OFFSET_EW_ITEM_NAME				"MOUNT_PIVOT_OFFSET_EW"

/** DOME_DIMENSION.MOUNT_PIVOT_VERTICAL_OFFSET property item name.
 */
#define DOME_MOUNT_PIVOT_VERTICAL_OFFSET_ITEM_NAME					"MOUNT_PIVOT_VERTICAL_OFFSET"

/** DOME_DIMENSION.MOUNT_PIVOT_OTA_OFFSET property item name.
 */
#define DOME_MOUNT_PIVOT_OTA_OFFSET_ITEM_NAME						"MOUNT_PIVOT_OTA_OFFSET"

/** DOME_SET_HOST_TIME property name.
 */
#define DOME_SET_HOST_TIME_PROPERTY_NAME				"DOME_SET_HOST_TIME"

/**  DOME_SET_HOST_TIME.SET property item name.
 */
#define DOME_SET_HOST_TIME_ITEM_NAME						"SET"


#define SNOOP_AGENT_NAME															"Snoop Agent"

#define SNOOP_ADD_RULE_PROPERTY_NAME									"SNOOP_ADD_RULE"
#define SNOOP_ADD_RULE_SOURCE_DEVICE_ITEM_NAME				"SOURCE_DEVICE"
#define SNOOP_ADD_RULE_SOURCE_PROPERTY_ITEM_NAME			"SOURCE_PROPERTY"
#define SNOOP_ADD_RULE_TARGET_DEVICE_ITEM_NAME				"TARGET_DEVICE"
#define SNOOP_ADD_RULE_TARGET_PROPERTY_ITEM_NAME			"TARGET_PROPERTY"

#define SNOOP_REMOVE_RULE_PROPERTY_NAME								"SNOOP_REMOVE_RULE"
#define SNOOP_REMOVE_RULE_SOURCE_DEVICE_ITEM_NAME			"SOURCE_DEVICE"
#define SNOOP_REMOVE_RULE_SOURCE_PROPERTY_ITEM_NAME		"SOURCE_PROPERTY"
#define SNOOP_REMOVE_RULE_TARGET_DEVICE_ITEM_NAME			"TARGET_DEVICE"
#define SNOOP_REMOVE_RULE_TARGET_PROPERTY_ITEM_NAME		"TARGET_PROPERTY"

#define SNOOP_RULES_PROPERTY_NAME											"SNOOP_RULES"

#define LX200_SERVER_AGENT_NAME												"LX200 Server Agent"

#define LX200_DEVICES_PROPERTY_NAME										"LX200_DEVICES"
#define LX200_DEVICES_MOUNT_ITEM_NAME									"MOUNT"
#define LX200_DEVICES_GUIDER_ITEM_NAME								"GUIDER"

#define LX200_CONFIGURATION_PROPERTY_NAME							"LX200_CONFIGURATION"
#define LX200_CONFIGURATION_PORT_ITEM_NAME						"PORT"

#define LX200_SERVER_PROPERTY_NAME										"LX200_SERVER"
#define LX200_SERVER_STARTED_ITEM_NAME								"STARTED"
#define LX200_SERVER_STOPPED_ITEM_NAME								"STOPPED"

#define JOYSTICK_BUTTONS_PROPERTY_NAME								"JOYSTICK_BUTTONS"
#define JOYSTICK_BUTTON_ITEM_NAME											"BUTTON_%d"

#define JOYSTICK_AXES_PROPERTY_NAME										"JOYSTICK_AXES"
#define JOYSTICK_AXIS_ITEM_NAME												"AXIS_%d"

#define JOYSTICK_MAPPING_PROPERTY_NAME								"JOYSTICK_MAPPING"
#define JOYSTICK_MAPPING_PARKED_ITEM_NAME							"PARKED"
#define JOYSTICK_MAPPING_UNPARKED_ITEM_NAME						"UNPARKED"
#define JOYSTICK_MAPPING_ABORT_ITEM_NAME							"ABORT"
#define JOYSTICK_MAPPING_TRACKING_ON_ITEM_NAME				"TRACKING_ON"
#define JOYSTICK_MAPPING_TRACKING_OFF_ITEM_NAME				"TRACKING_OFF"
#define JOYSTICK_MAPPING_MOTION_RA_ITEM_NAME					"MOTION_RA"
#define JOYSTICK_MAPPING_MOTION_DEC_ITEM_NAME					"MOTION_DEC"
#define JOYSTICK_MAPPING_RATE_GUIDE_ITEM_NAME					"RATE_GUIDE"
#define JOYSTICK_MAPPING_RATE_CENTERING_ITEM_NAME			"RATE_CENTERING"
#define JOYSTICK_MAPPING_RATE_FIND_ITEM_NAME					"RATE_FIND"
#define JOYSTICK_MAPPING_RATE_MAX_ITEM_NAME						"RATE_MAX"

#define JOYSTICK_OPTIONS_PROPERTY_NAME								"JOYSTICK_OPTIONS"
#define JOYSTICK_OPTIONS_ANALOG_STICK_ITEM_NAME				"ANALOG"
#define JOYSTICK_OPTIONS_SWAP_RA_ITEM_NAME						"SWAP_RA"
#define JOYSTICK_OPTIONS_SWAP_DEC_ITEM_NAME						"SWAP_DEC"

#define AUX_OUTLET_NAMES_PROPERTY_NAME								"AUX_OUTLET_NAMES"
#define AUX_POWER_OUTLET_NAME_1_ITEM_NAME							"POWER_OUTLET_NAME_1"
#define AUX_POWER_OUTLET_NAME_2_ITEM_NAME							"POWER_OUTLET_NAME_2"
#define AUX_POWER_OUTLET_NAME_3_ITEM_NAME							"POWER_OUTLET_NAME_3"
#define AUX_POWER_OUTLET_NAME_4_ITEM_NAME							"POWER_OUTLET_NAME_4"
#define AUX_POWER_OUTLET_NAME_5_ITEM_NAME							"POWER_OUTLET_NAME_5"
#define AUX_POWER_OUTLET_NAME_6_ITEM_NAME							"POWER_OUTLET_NAME_6"
#define AUX_POWER_OUTLET_NAME_7_ITEM_NAME							"POWER_OUTLET_NAME_7"
#define AUX_POWER_OUTLET_NAME_8_ITEM_NAME							"POWER_OUTLET_NAME_8"
#define AUX_HEATER_OUTLET_NAME_1_ITEM_NAME						"HEATER_OUTLET_NAME_1"
#define AUX_HEATER_OUTLET_NAME_2_ITEM_NAME						"HEATER_OUTLET_NAME_2"
#define AUX_HEATER_OUTLET_NAME_3_ITEM_NAME						"HEATER_OUTLET_NAME_3"
#define AUX_HEATER_OUTLET_NAME_4_ITEM_NAME						"HEATER_OUTLET_NAME_4"
#define AUX_HEATER_OUTLET_NAME_5_ITEM_NAME						"HEATER_OUTLET_NAME_5"
#define AUX_HEATER_OUTLET_NAME_6_ITEM_NAME						"HEATER_OUTLET_NAME_6"
#define AUX_HEATER_OUTLET_NAME_7_ITEM_NAME						"HEATER_OUTLET_NAME_7"
#define AUX_HEATER_OUTLET_NAME_8_ITEM_NAME						"HEATER_OUTLET_NAME_8"
#define AUX_USB_PORT_NAME_1_ITEM_NAME									"USB_PORT_NAME_1"
#define AUX_USB_PORT_NAME_2_ITEM_NAME									"USB_PORT_NAME_2"
#define AUX_USB_PORT_NAME_3_ITEM_NAME									"USB_PORT_NAME_3"
#define AUX_USB_PORT_NAME_4_ITEM_NAME									"USB_PORT_NAME_4"
#define AUX_USB_PORT_NAME_5_ITEM_NAME									"USB_PORT_NAME_5"
#define AUX_USB_PORT_NAME_6_ITEM_NAME									"USB_PORT_NAME_6"
#define AUX_USB_PORT_NAME_7_ITEM_NAME									"USB_PORT_NAME_7"
#define AUX_USB_PORT_NAME_8_ITEM_NAME									"USB_PORT_NAME_8"
#define AUX_GPIO_OUTLET_NAME_1_ITEM_NAME							"GPIO_OUTLET_NAME_1"
#define AUX_GPIO_OUTLET_NAME_2_ITEM_NAME							"GPIO_OUTLET_NAME_2"
#define AUX_GPIO_OUTLET_NAME_3_ITEM_NAME							"GPIO_OUTLET_NAME_3"
#define AUX_GPIO_OUTLET_NAME_4_ITEM_NAME							"GPIO_OUTLET_NAME_4"
#define AUX_GPIO_OUTLET_NAME_5_ITEM_NAME							"GPIO_OUTLET_NAME_5"
#define AUX_GPIO_OUTLET_NAME_6_ITEM_NAME							"GPIO_OUTLET_NAME_6"
#define AUX_GPIO_OUTLET_NAME_7_ITEM_NAME							"GPIO_OUTLET_NAME_7"
#define AUX_GPIO_OUTLET_NAME_8_ITEM_NAME							"GPIO_OUTLET_NAME_8"

#define AUX_SENSOR_NAMES_PROPERTY_NAME								"AUX_SENSOR_NAMES"
#define AUX_GPIO_SENSOR_NAME_1_ITEM_NAME							"GPIO_SENSOR_NAME_1"
#define AUX_GPIO_SENSOR_NAME_2_ITEM_NAME							"GPIO_SENSOR_NAME_2"
#define AUX_GPIO_SENSOR_NAME_3_ITEM_NAME							"GPIO_SENSOR_NAME_3"
#define AUX_GPIO_SENSOR_NAME_4_ITEM_NAME							"GPIO_SENSOR_NAME_4"
#define AUX_GPIO_SENSOR_NAME_5_ITEM_NAME							"GPIO_SENSOR_NAME_5"
#define AUX_GPIO_SENSOR_NAME_6_ITEM_NAME							"GPIO_SENSOR_NAME_6"
#define AUX_GPIO_SENSOR_NAME_7_ITEM_NAME							"GPIO_SENSOR_NAME_7"
#define AUX_GPIO_SENSOR_NAME_8_ITEM_NAME							"GPIO_SENSOR_NAME_8"

#define AUX_POWER_OUTLET_PROPERTY_NAME								"AUX_POWER_OUTLET"
#define AUX_POWER_OUTLET_1_ITEM_NAME									"OUTLET_1"
#define AUX_POWER_OUTLET_2_ITEM_NAME									"OUTLET_2"
#define AUX_POWER_OUTLET_3_ITEM_NAME									"OUTLET_3"
#define AUX_POWER_OUTLET_4_ITEM_NAME									"OUTLET_4"
#define AUX_POWER_OUTLET_5_ITEM_NAME									"OUTLET_5"
#define AUX_POWER_OUTLET_6_ITEM_NAME									"OUTLET_6"
#define AUX_POWER_OUTLET_7_ITEM_NAME									"OUTLET_7"
#define AUX_POWER_OUTLET_8_ITEM_NAME									"OUTLET_8"

#define AUX_POWER_OUTLET_STATE_PROPERTY_NAME					"AUX_POWER_OUTLET_STATE"
#define AUX_POWER_OUTLET_STATE_1_ITEM_NAME						"OUTLET_1"
#define AUX_POWER_OUTLET_STATE_2_ITEM_NAME						"OUTLET_2"
#define AUX_POWER_OUTLET_STATE_3_ITEM_NAME						"OUTLET_3"
#define AUX_POWER_OUTLET_STATE_4_ITEM_NAME						"OUTLET_4"
#define AUX_POWER_OUTLET_STATE_5_ITEM_NAME						"OUTLET_5"
#define AUX_POWER_OUTLET_STATE_6_ITEM_NAME						"OUTLET_6"
#define AUX_POWER_OUTLET_STATE_7_ITEM_NAME						"OUTLET_7"
#define AUX_POWER_OUTLET_STATE_8_ITEM_NAME						"OUTLET_8"

#define AUX_USB_PORT_PROPERTY_NAME										"AUX_USB_PORT"
#define AUX_USB_PORT_1_ITEM_NAME											"PORT_1"
#define AUX_USB_PORT_2_ITEM_NAME											"PORT_2"
#define AUX_USB_PORT_3_ITEM_NAME											"PORT_3"
#define AUX_USB_PORT_4_ITEM_NAME											"PORT_4"
#define AUX_USB_PORT_5_ITEM_NAME											"PORT_5"
#define AUX_USB_PORT_6_ITEM_NAME											"PORT_6"
#define AUX_USB_PORT_7_ITEM_NAME											"PORT_7"
#define AUX_USB_PORT_8_ITEM_NAME											"PORT_8"

#define AUX_USB_PORT_STATE_PROPERTY_NAME							"AUX_USB_PORT_STATE"
#define AUX_USB_PORT_STATE_1_ITEM_NAME								"PORT_1"
#define AUX_USB_PORT_STATE_2_ITEM_NAME								"PORT_2"
#define AUX_USB_PORT_STATE_3_ITEM_NAME								"PORT_3"
#define AUX_USB_PORT_STATE_4_ITEM_NAME								"PORT_4"
#define AUX_USB_PORT_STATE_5_ITEM_NAME								"PORT_5"
#define AUX_USB_PORT_STATE_6_ITEM_NAME								"PORT_6"
#define AUX_USB_PORT_STATE_7_ITEM_NAME								"PORT_7"
#define AUX_USB_PORT_STATE_8_ITEM_NAME								"PORT_8"

#define AUX_POWER_OUTLET_CURRENT_PROPERTY_NAME				"AUX_POWER_OUTLET_CURRENT"
#define AUX_POWER_OUTLET_CURRENT_1_ITEM_NAME					"OUTLET_1"
#define AUX_POWER_OUTLET_CURRENT_2_ITEM_NAME					"OUTLET_2"
#define AUX_POWER_OUTLET_CURRENT_3_ITEM_NAME					"OUTLET_3"
#define AUX_POWER_OUTLET_CURRENT_4_ITEM_NAME					"OUTLET_4"
#define AUX_POWER_OUTLET_CURRENT_5_ITEM_NAME					"OUTLET_5"
#define AUX_POWER_OUTLET_CURRENT_6_ITEM_NAME					"OUTLET_6"
#define AUX_POWER_OUTLET_CURRENT_7_ITEM_NAME					"OUTLET_7"
#define AUX_POWER_OUTLET_CURRENT_8_ITEM_NAME					"OUTLET_8"

#define AUX_HEATER_OUTLET_PROPERTY_NAME								"AUX_HEATER_OUTLET"
#define AUX_HEATER_OUTLET_1_ITEM_NAME									"OUTLET_1"
#define AUX_HEATER_OUTLET_2_ITEM_NAME									"OUTLET_2"
#define AUX_HEATER_OUTLET_3_ITEM_NAME									"OUTLET_3"
#define AUX_HEATER_OUTLET_4_ITEM_NAME									"OUTLET_4"
#define AUX_HEATER_OUTLET_5_ITEM_NAME									"OUTLET_5"
#define AUX_HEATER_OUTLET_6_ITEM_NAME									"OUTLET_6"
#define AUX_HEATER_OUTLET_7_ITEM_NAME									"OUTLET_7"
#define AUX_HEATER_OUTLET_8_ITEM_NAME									"OUTLET_8"

#define AUX_HEATER_OUTLET_STATE_PROPERTY_NAME					"AUX_HEATER_OUTLET_STATE"
#define AUX_HEATER_OUTLET_STATE_1_ITEM_NAME						"OUTLET_1"
#define AUX_HEATER_OUTLET_STATE_2_ITEM_NAME						"OUTLET_2"
#define AUX_HEATER_OUTLET_STATE_3_ITEM_NAME						"OUTLET_3"
#define AUX_HEATER_OUTLET_STATE_4_ITEM_NAME						"OUTLET_4"
#define AUX_HEATER_OUTLET_STATE_5_ITEM_NAME						"OUTLET_5"
#define AUX_HEATER_OUTLET_STATE_6_ITEM_NAME						"OUTLET_6"
#define AUX_HEATER_OUTLET_STATE_7_ITEM_NAME						"OUTLET_7"
#define AUX_HEATER_OUTLET_STATE_8_ITEM_NAME						"OUTLET_8"

#define AUX_HEATER_OUTLET_CURRENT_PROPERTY_NAME				"AUX_HEATER_OUTLET_CURRENT"
#define AUX_HEATER_OUTLET_CURRENT_1_ITEM_NAME					"OUTLET_1"
#define AUX_HEATER_OUTLET_CURRENT_2_ITEM_NAME					"OUTLET_2"
#define AUX_HEATER_OUTLET_CURRENT_3_ITEM_NAME					"OUTLET_3"
#define AUX_HEATER_OUTLET_CURRENT_4_ITEM_NAME					"OUTLET_4"
#define AUX_HEATER_OUTLET_CURRENT_5_ITEM_NAME					"OUTLET_5"
#define AUX_HEATER_OUTLET_CURRENT_6_ITEM_NAME					"OUTLET_6"
#define AUX_HEATER_OUTLET_CURRENT_7_ITEM_NAME					"OUTLET_7"
#define AUX_HEATER_OUTLET_CURRENT_8_ITEM_NAME					"OUTLET_8"

#define AUX_DEW_CONTROL_PROPERTY_NAME									"AUX_DEW_CONTROL"
#define AUX_DEW_CONTROL_MANUAL_ITEM_NAME							"MANUAL"
#define AUX_DEW_CONTROL_AUTOMATIC_ITEM_NAME						"AUTOMATIC"

// generic numeric property to be accumulated from a multiple properties in a single sheet on GUI
#define AUX_INFO_PROPERTY_NAME												"AUX_INFO"
#define AUX_INFO_SKY_BRIGHTNESS_ITEM_NAME							"SKY_BRIGHTNESS"
#define AUX_INFO_SKY_TEMPERATURE_ITEM_NAME						"SKY_TEMPERATURE"
#define AUX_INFO_VOLTAGE_ITEM_NAME										"VOLTAGE"
#define AUX_INFO_CURRENT_ITEM_NAME										"CURRENT"
#define AUX_INFO_POWER_ITEM_NAME											"POWER"

#define AUX_WEATHER_PROPERTY_NAME											"AUX_WEATHER"
#define AUX_WEATHER_TEMPERATURE_ITEM_NAME							"TEMPERATURE"
#define AUX_WEATHER_HUMIDITY_ITEM_NAME								"HUMIDITY"
#define AUX_WEATHER_DEWPOINT_ITEM_NAME								"DEWPOINT"
#define AUX_WEATHER_WIND_SPEED_ITEM_NAME							"WIND_SPEED"
#define AUX_WEATHER_WIND_DIRECTION_ITEM_NAME					"WIND_DIRECTION"
#define AUX_WEATHER_PRESSURE_ITEM_NAME						    "ATMOSPHERIC_PRESSURE"
#define AUX_WEATHER_RAIN_ITEM_NAME						    		"RAIN"

#define AUX_HUMIDITY_THRESHOLDS_PROPERTY_NAME 				"AUX_HUMIDITY_THRESHOLDS"
#define AUX_HUMIDITY_PROPERTY_NAME            				"AUX_HUMIDITY"
#define AUX_HUMIDITY_HUMID_ITEM_NAME          				"HUMID"
#define AUX_HUMIDITY_NORMAL_ITEM_NAME         				"NORMAL"
#define AUX_HUMIDITY_DRY_ITEM_NAME            				"DRY"

#define AUX_WIND_THRESHOLDS_PROPERTY_NAME 						"AUX_WIND_THRESHOLDS"
#define AUX_WIND_PROPERTY_NAME            						"AUX_WIND"
#define AUX_WIND_STRONG_ITEM_NAME         						"STRONG"
#define AUX_WIND_MODERATE_ITEM_NAME       						"MODERATE"
#define AUX_WIND_CALM_ITEM_NAME           						"CALM"

#define AUX_RAIN_THRESHOLDS_PROPERTY_NAME 						"AUX_RAIN_THRESHOLDS"
#define AUX_RAIN_PROPERTY_NAME            						"AUX_RAIN"
#define AUX_RAIN_RAINING_ITEM_NAME        						"RAINING"
#define AUX_RAIN_WET_ITEM_NAME            						"WET"
#define AUX_RAIN_DRY_ITEM_NAME            						"DRY"

#define AUX_CLOUD_THRESHOLDS_PROPERTY_NAME  					"AUX_CLOUD_THRESHOLDS"
#define AUX_CLOUD_PROPERTY_NAME             					"AUX_CLOUD"

#define AUX_CLOUD_CLEAR_ITEM_NAME           					"CLEAR"
#define AUX_CLOUD_CLOUDY_ITEM_NAME          					"CLOUDY"
#define AUX_CLOUD_OVERCAST_ITEM_NAME        					"OVERCAST"

#define AUX_SKY_THRESHOLDS_PROPERTY_NAME  						"AUX_SKY_THRESHOLDS"
#define AUX_SKY_PROPERTY_NAME             						"AUX_SKY"

#define AUX_SKY_DARK_ITEM_NAME             						"DARK"
#define AUX_SKY_LIGHT_ITEM_NAME            						"LIGHT"
#define AUX_SKY_VERY_LIGHT_ITEM_NAME       						"VERY_LIGHT"

#define AUX_TEMPERATURE_SENSORS_PROPERTY_NAME					"AUX_TEMPERATURE_SENSORS"
#define AUX_TEMPERATURE_SENSORS_SENSOR_1_ITEM_NAME		"SENSOR_1"
#define AUX_TEMPERATURE_SENSORS_SENSOR_2_ITEM_NAME		"SENSOR_2"
#define AUX_TEMPERATURE_SENSORS_SENSOR_3_ITEM_NAME		"SENSOR_3"
#define AUX_TEMPERATURE_SENSORS_SENSOR_4_ITEM_NAME		"SENSOR_4"
#define AUX_TEMPERATURE_SENSORS_SENSOR_5_ITEM_NAME		"SENSOR_5"
#define AUX_TEMPERATURE_SENSORS_SENSOR_6_ITEM_NAME		"SENSOR_6"
#define AUX_TEMPERATURE_SENSORS_SENSOR_7_ITEM_NAME		"SENSOR_7"
#define AUX_TEMPERATURE_SENSORS_SENSOR_8_ITEM_NAME		"SENSOR_8"

#define AUX_DEW_WARNING_PROPERTY_NAME			    				"AUX_DEW_WARNING"
#define AUX_DEW_WARNING_SENSOR_1_ITEM_NAME			    	"AT_SENSOR_1"
#define AUX_DEW_WARNING_SENSOR_2_ITEM_NAME						"AT_SENSOR_2"
#define AUX_DEW_WARNING_SENSOR_3_ITEM_NAME						"AT_SENSOR_3"
#define AUX_DEW_WARNING_SENSOR_4_ITEM_NAME						"AT_SENSOR_4"
#define AUX_DEW_WARNING_SENSOR_5_ITEM_NAME						"AT_SENSOR_5"
#define AUX_DEW_WARNING_SENSOR_6_ITEM_NAME						"AT_SENSOR_6"
#define AUX_DEW_WARNING_SENSOR_7_ITEM_NAME						"AT_SENSOR_7"
#define AUX_DEW_WARNING_SENSOR_8_ITEM_NAME						"AT_SENSOR_8"

#define AUX_DEW_THRESHOLD_PROPERTY_NAME								"AUX_DEW_THRESHOLD"
#define AUX_DEW_THRESHOLD_SENSOR_1_ITEM_NAME    			"AT_SENSOR_1"
#define AUX_DEW_THRESHOLD_SENSOR_2_ITEM_NAME					"AT_SENSOR_2"
#define AUX_DEW_THRESHOLD_SENSOR_3_ITEM_NAME    			"AT_SENSOR_3"
#define AUX_DEW_THRESHOLD_SENSOR_4_ITEM_NAME					"AT_SENSOR_4"
#define AUX_DEW_THRESHOLD_SENSOR_5_ITEM_NAME    			"AT_SENSOR_5"
#define AUX_DEW_THRESHOLD_SENSOR_6_ITEM_NAME					"AT_SENSOR_6"
#define AUX_DEW_THRESHOLD_SENSOR_7_ITEM_NAME    			"AT_SENSOR_7"
#define AUX_DEW_THRESHOLD_SENSOR_8_ITEM_NAME					"AT_SENSOR_8"

#define AUX_RAIN_WARNING_PROPERTY_NAME			    			"AUX_RAIN_WARNING"
#define AUX_RAIN_WARNING_SENSOR_1_ITEM_NAME			    	"AT_SENSOR_1"
#define AUX_RAIN_WARNING_SENSOR_2_ITEM_NAME						"AT_SENSOR_2"
#define AUX_RAIN_WARNING_SENSOR_3_ITEM_NAME						"AT_SENSOR_3"
#define AUX_RAIN_WARNING_SENSOR_4_ITEM_NAME						"AT_SENSOR_4"
#define AUX_RAIN_WARNING_SENSOR_5_ITEM_NAME						"AT_SENSOR_5"
#define AUX_RAIN_WARNING_SENSOR_6_ITEM_NAME						"AT_SENSOR_6"
#define AUX_RAIN_WARNING_SENSOR_7_ITEM_NAME						"AT_SENSOR_7"
#define AUX_RAIN_WARNING_SENSOR_8_ITEM_NAME						"AT_SENSOR_8"

#define AUX_RAIN_THRESHOLD_PROPERTY_NAME							"AUX_RAIN_THRESHOLD"
#define AUX_RAIN_THRESHOLD_SENSOR_1_ITEM_NAME    			"AT_SENSOR_1"
#define AUX_RAIN_THRESHOLD_SENSOR_2_ITEM_NAME					"AT_SENSOR_2"
#define AUX_RAIN_THRESHOLD_SENSOR_3_ITEM_NAME    			"AT_SENSOR_3"
#define AUX_RAIN_THRESHOLD_SENSOR_4_ITEM_NAME					"AT_SENSOR_4"
#define AUX_RAIN_THRESHOLD_SENSOR_5_ITEM_NAME    			"AT_SENSOR_5"
#define AUX_RAIN_THRESHOLD_SENSOR_6_ITEM_NAME					"AT_SENSOR_6"
#define AUX_RAIN_THRESHOLD_SENSOR_7_ITEM_NAME    			"AT_SENSOR_7"
#define AUX_RAIN_THRESHOLD_SENSOR_8_ITEM_NAME					"AT_SENSOR_8"

#define AUX_WIND_WARNING_PROPERTY_NAME			    			"AUX_WIND_WARNING"
#define AUX_WIND_WARNING_SENSOR_1_ITEM_NAME			    	"AT_SENSOR_1"
#define AUX_WIND_WARNING_SENSOR_2_ITEM_NAME						"AT_SENSOR_2"
#define AUX_WIND_WARNING_SENSOR_3_ITEM_NAME						"AT_SENSOR_3"
#define AUX_WIND_WARNING_SENSOR_4_ITEM_NAME						"AT_SENSOR_4"
#define AUX_WIND_WARNING_SENSOR_5_ITEM_NAME						"AT_SENSOR_5"
#define AUX_WIND_WARNING_SENSOR_6_ITEM_NAME						"AT_SENSOR_6"
#define AUX_WIND_WARNING_SENSOR_7_ITEM_NAME						"AT_SENSOR_7"
#define AUX_WIND_WARNING_SENSOR_8_ITEM_NAME						"AT_SENSOR_8"

#define AUX_WIND_THRESHOLD_PROPERTY_NAME							"AUX_WIND_THRESHOLD"
#define AUX_WIND_THRESHOLD_SENSOR_1_ITEM_NAME    			"AT_SENSOR_1"
#define AUX_WIND_THRESHOLD_SENSOR_2_ITEM_NAME					"AT_SENSOR_2"
#define AUX_WIND_THRESHOLD_SENSOR_3_ITEM_NAME    			"AT_SENSOR_3"
#define AUX_WIND_THRESHOLD_SENSOR_4_ITEM_NAME					"AT_SENSOR_4"
#define AUX_WIND_THRESHOLD_SENSOR_5_ITEM_NAME    			"AT_SENSOR_5"
#define AUX_WIND_THRESHOLD_SENSOR_6_ITEM_NAME					"AT_SENSOR_6"
#define AUX_WIND_THRESHOLD_SENSOR_7_ITEM_NAME    			"AT_SENSOR_7"
#define AUX_WIND_THRESHOLD_SENSOR_8_ITEM_NAME					"AT_SENSOR_8"

#define AUX_LIGHT_SWITCH_PROPERTY_NAME								"AUX_LIGHT_SWITCH"
#define AUX_LIGHT_SWITCH_ON_ITEM_NAME									"ON"
#define AUX_LIGHT_SWITCH_OFF_ITEM_NAME								"OFF"

#define AUX_LIGHT_IMPULSE_PROPERTY_NAME						    "AUX_LIGHT_IMPULSE"
#define AUX_LIGHT_IMPULSE_DURATION_ITEM_NAME   				"DURATION"

#define AUX_LIGHT_INTENSITY_PROPERTY_NAME							"AUX_LIGHT_INTENSITY"
#define AUX_LIGHT_INTENSITY_ITEM_NAME									"LIGHT_INTENSITY"

#define AUX_COVER_PROPERTY_NAME          							"AUX_COVER"
#define AUX_COVER_CLOSE_ITEM_NAME            					"CLOSE"
#define AUX_COVER_OPEN_ITEM_NAME           						"OPEN"

#define AUX_GPIO_SENSORS_PROPERTY_NAME							"AUX_GPIO_SENSORS"
#define AUX_GPIO_SENSORS_SENSOR_1_ITEM_NAME					"SENSOR_1"
#define AUX_GPIO_SENSORS_SENSOR_2_ITEM_NAME					"SENSOR_2"
#define AUX_GPIO_SENSORS_SENSOR_3_ITEM_NAME					"SENSOR_3"
#define AUX_GPIO_SENSORS_SENSOR_4_ITEM_NAME					"SENSOR_4"
#define AUX_GPIO_SENSORS_SENSOR_5_ITEM_NAME					"SENSOR_5"
#define AUX_GPIO_SENSORS_SENSOR_6_ITEM_NAME					"SENSOR_6"
#define AUX_GPIO_SENSORS_SENSOR_7_ITEM_NAME					"SENSOR_7"
#define AUX_GPIO_SENSORS_SENSOR_8_ITEM_NAME					"SENSOR_8"

#define AUX_GPIO_OUTLETS_PROPERTY_NAME						"AUX_GPIO_OUTLETS"
#define AUX_GPIO_OUTLETS_OUTLET_1_ITEM_NAME					"OUTLET_1"
#define AUX_GPIO_OUTLETS_OUTLET_2_ITEM_NAME					"OUTLET_2"
#define AUX_GPIO_OUTLETS_OUTLET_3_ITEM_NAME					"OUTLET_3"
#define AUX_GPIO_OUTLETS_OUTLET_4_ITEM_NAME					"OUTLET_4"
#define AUX_GPIO_OUTLETS_OUTLET_5_ITEM_NAME					"OUTLET_5"
#define AUX_GPIO_OUTLETS_OUTLET_6_ITEM_NAME					"OUTLET_6"
#define AUX_GPIO_OUTLETS_OUTLET_7_ITEM_NAME					"OUTLET_7"
#define AUX_GPIO_OUTLETS_OUTLET_8_ITEM_NAME					"OUTLET_8"

/* GPIO PWM support. Enabled by AUX_GPIO_OUTLETS item */
#define AUX_OUTLET_PULSE_LENGTHS_PROPERTY_NAME				"AUX_OUTLET_PULSE_LENGTHS"
/* Items are AUX_GPIO_OUTLETS_OUTLET_X_ITEM_NAME */

/* GPIO PWM support. Enabled by AUX_GPIO_OUTLETS item */
#define AUX_GPIO_OUTLET_FREQUENCIES_PROPERTY_NAME			"AUX_GPIO_OUTLET_FREQUENCIES"
#define AUX_GPIO_OUTLET_DUTY_PROPERTY_NAME					"AUX_GPIO_OUTLET_DUTY_CYCLES"
/* Items are AUX_GPIO_OUTLETS_OUTLET_X_ITEM_NAME */


/* Agents */
#define FILTER_CCD_LIST_PROPERTY_NAME 								"FILTER_CCD_LIST"
#define FILTER_WHEEL_LIST_PROPERTY_NAME 							"FILTER_WHEEL_LIST"
#define FILTER_FOCUSER_LIST_PROPERTY_NAME							"FILTER_FOCUSER_LIST"
#define FILTER_GUIDER_LIST_PROPERTY_NAME							"FILTER_GUIDER_LIST"
#define FILTER_MOUNT_LIST_PROPERTY_NAME								"FILTER_MOUNT_LIST"
#define FILTER_DOME_LIST_PROPERTY_NAME								"FILTER_DOME_LIST"
#define FILTER_GPS_LIST_PROPERTY_NAME									"FILTER_GPS_LIST"
#define FILTER_JOYSTICK_LIST_PROPERTY_NAME						"FILTER_JOYSTICK_LIST"
#define FILTER_AUX_1_LIST_PROPERTY_NAME								"FILTER_AUX_1_LIST"
#define FILTER_AUX_2_LIST_PROPERTY_NAME								"FILTER_AUX_2_LIST"
#define FILTER_AUX_3_LIST_PROPERTY_NAME								"FILTER_AUX_3_LIST"
#define FILTER_AUX_4_LIST_PROPERTY_NAME								"FILTER_AUX_4_LIST"

#define FILTER_DEVICE_LIST_NONE_ITEM_NAME							"NONE"

#define FILTER_RELATED_CCD_LIST_PROPERTY_NAME 				"FILTER_RELATED_CCD_LIST"
#define FILTER_RELATED_WHEEL_LIST_PROPERTY_NAME 			"FILTER_RELATED_WHEEL_LIST"
#define FILTER_RELATED_FOCUSER_LIST_PROPERTY_NAME			"FILTER_RELATED_FOCUSER_LIST"
#define FILTER_RELATED_GUIDER_LIST_PROPERTY_NAME			"FILTER_RELATED_GUIDER_LIST"
#define FILTER_RELATED_MOUNT_LIST_PROPERTY_NAME				"FILTER_RELATED_MOUNT_LIST"
#define FILTER_RELATED_DOME_LIST_PROPERTY_NAME				"FILTER_RELATED_DOME_LIST"
#define FILTER_RELATED_GPS_LIST_PROPERTY_NAME					"FILTER_RELATED_GPS_LIST"
#define FILTER_RELATED_JOYSTICK_LIST_PROPERTY_NAME		"FILTER_RELATED_JOYSTICK_LIST"
#define FILTER_RELATED_AUX_1_LIST_PROPERTY_NAME				"FILTER_RELATED_AUX_1_LIST"
#define FILTER_RELATED_AUX_2_LIST_PROPERTY_NAME				"FILTER_RELATED_AUX_2_LIST"
#define FILTER_RELATED_AUX_3_LIST_PROPERTY_NAME				"FILTER_RELATED_AUX_3_LIST"
#define FILTER_RELATED_AUX_4_LIST_PROPERTY_NAME				"FILTER_RELATED_AUX_4_LIST"
#define FILTER_RELATED_AGENT_LIST_PROPERTY_NAME				"FILTER_RELATED_AGENT_LIST"

#define AGENT_START_PROCESS_PROPERTY_NAME 						"AGENT_START_PROCESS"
#define AGENT_IMAGER_START_PREVIEW_ITEM_NAME					"PREVIEW"
#define AGENT_IMAGER_START_EXPOSURE_ITEM_NAME					"EXPOSURE"
#define AGENT_IMAGER_START_STREAMING_ITEM_NAME				"STREAMING"
#define AGENT_IMAGER_START_FOCUSING_ITEM_NAME					"FOCUSING"
#define AGENT_IMAGER_START_SEQUENCE_ITEM_NAME					"SEQUENCE"
#define AGENT_GUIDER_START_PREVIEW_ITEM_NAME  				"PREVIEW"
#define AGENT_GUIDER_START_CALIBRATION_ITEM_NAME 			"CALIBRATION"
#define AGENT_GUIDER_START_CALIBRATION_AND_GUIDING_ITEM_NAME 	"CALIBRATION_AND_GUIDING"
#define AGENT_GUIDER_START_GUIDING_ITEM_NAME 					"GUIDING"

#define AGENT_PAUSE_PROCESS_PROPERTY_NAME							"AGENT_PAUSE_PROCESS"
#define AGENT_PAUSE_PROCESS_ITEM_NAME      						"PAUSE"

#define AGENT_ABORT_PROCESS_PROPERTY_NAME 						"AGENT_ABORT_PROCESS"
#define AGENT_ABORT_PROCESS_ITEM_NAME									"ABORT"

#define AGENT_IMAGER_BATCH_PROPERTY_NAME 							"AGENT_IMAGER_BATCH"
#define AGENT_IMAGER_BATCH_COUNT_ITEM_NAME						"COUNT"
#define AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME					"EXPOSURE"
#define AGENT_IMAGER_BATCH_DELAY_ITEM_NAME						"DELAY"

#define AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY_NAME			"AGENT_IMAGER_DOWNLOAD_FILE"
#define AGENT_IMAGER_DOWNLOAD_FILE_ITEM_NAME					"FILE"

#define AGENT_IMAGER_DELETE_FILE_PROPERTY_NAME				"AGENT_IMAGER_DELETE_FILE"
#define AGENT_IMAGER_DELETE_FILE_ITEM_NAME						"FILE"

#define AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY_NAME			"AGENT_IMAGER_DOWNLOAD_FILES"
#define AGENT_IMAGER_DOWNLOAD_FILES_REFRESH_ITEM_NAME	"REFRESH"

#define AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY_NAME			"AGENT_IMAGER_DOWNLOAD_IMAGE"
#define AGENT_IMAGER_DOWNLOAD_IMAGE_ITEM_NAME					"IMAGE"

#define AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME			"AGENT_GUIDER_DETECTION_MODE"
#define AGENT_GUIDER_DETECTION_DONUTS_ITEM_NAME  			"DONUTS"
#define AGENT_GUIDER_DETECTION_CENTROID_ITEM_NAME    	"CENTROID"
#define AGENT_GUIDER_DETECTION_MULTISTAR_ITEM_NAME		"MULTISTAR"
#define AGENT_GUIDER_DETECTION_SELECTION_ITEM_NAME    "SELECTION"

#define AGENT_GUIDER_DEC_MODE_PROPERTY_NAME						"AGENT_GUIDER_DEC_MODE"
#define AGENT_GUIDER_DEC_MODE_BOTH_ITEM_NAME    			"BOTH"
#define AGENT_GUIDER_DEC_MODE_NORTH_ITEM_NAME    			"NORTH"
#define AGENT_GUIDER_DEC_MODE_SOUTH_ITEM_NAME    			"SOUTH"
#define AGENT_GUIDER_DEC_MODE_NONE_ITEM_NAME    			"NONE"

#define AGENT_GUIDER_SETTINGS_PROPERTY_NAME						"AGENT_GUIDER_SETTINGS"
#define AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME   		"EXPOSURE"
#define AGENT_GUIDER_SETTINGS_DELAY_ITEM_NAME   			"DELAY"
#define AGENT_GUIDER_SETTINGS_STEP_ITEM_NAME      		"STEP0"
#define AGENT_GUIDER_SETTINGS_ANGLE_ITEM_NAME      		"ANGLE"
#define AGENT_GUIDER_SETTINGS_BACKLASH_ITEM_NAME   		"BACKLASH"
#define AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM_NAME   		"SPEED_RA"
#define AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM_NAME  		"SPEED_DEC"
#define AGENT_GUIDER_SETTINGS_BL_STEPS_ITEM_NAME  		"MAX_BL_STEPS"
#define AGENT_GUIDER_SETTINGS_BL_DRIFT_ITEM_NAME  		"MIN_BL_DRIFT"
#define AGENT_GUIDER_SETTINGS_CAL_STEPS_ITEM_NAME  		"MAX_CALIBRATION_STEPS"
#define AGENT_GUIDER_SETTINGS_CAL_DRIFT_ITEM_NAME  		"MIN_CALIBRATION_DRIFT"
#define AGENT_GUIDER_SETTINGS_AGG_RA_ITEM_NAME  			"AGGRESSIVITY_RA"
#define AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM_NAME  			"AGGRESSIVITY_DEC"
#define AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM_NAME  			"I_GAIN_RA"
#define AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM_NAME  		"I_GAIN_DEC"
#define AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM_NAME  			"MIN_ERROR"
#define AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM_NAME  		"MIN_PULSE"
#define AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM_NAME  		"MAX_PULSE"
#define AGENT_GUIDER_SETTINGS_DITH_X_ITEM_NAME				"DITHERING_X"
#define AGENT_GUIDER_SETTINGS_DITH_Y_ITEM_NAME				"DITHERING_Y"
#define AGENT_GUIDER_SETTINGS_DITH_LIMIT_ITEM_NAME		"DITHERING_LIMIT"
#define AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME					"STACK"

#define AGENT_GUIDER_STARS_PROPERTY_NAME							"AGENT_GUIDER_STARS"
#define AGENT_GUIDER_STARS_REFRESH_ITEM_NAME					"REFRESH"

#define AGENT_GUIDER_SELECTION_PROPERTY_NAME					"AGENT_GUIDER_SELECTION"
/* Selection guide related items */
#define AGENT_GUIDER_SELECTION_X_ITEM_NAME						"X"
#define AGENT_GUIDER_SELECTION_Y_ITEM_NAME						"Y"
#define AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME				"RADIUS"
#define AGENT_GUIDER_SELECTION_SUBFRAME_ITEM_NAME			"SUBFRAME"
#define AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME		"COUNT"
/* Donuts guide related items */
#define AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME			"EDGE_CLIPPING"

#define AGENT_GUIDER_STATS_PROPERTY_NAME							"AGENT_GUIDER_STATS"
#define AGENT_GUIDER_STATS_PHASE_ITEM_NAME						"PHASE"
#define AGENT_GUIDER_STATS_FRAME_ITEM_NAME						"FRAME"
#define AGENT_GUIDER_STATS_REFERENCE_X_ITEM_NAME			"REFERENCE_X"
#define AGENT_GUIDER_STATS_REFERENCE_Y_ITEM_NAME			"REFERENCE_Y"
#define AGENT_GUIDER_STATS_DRIFT_X_ITEM_NAME      		"DRIFT_X"
#define AGENT_GUIDER_STATS_DRIFT_Y_ITEM_NAME      		"DRIFT_Y"
#define AGENT_GUIDER_STATS_DRIFT_RA_ITEM_NAME      		"DRIFT_RA"
#define AGENT_GUIDER_STATS_DRIFT_DEC_ITEM_NAME      	"DRIFT_DEC"
#define AGENT_GUIDER_STATS_CORR_RA_ITEM_NAME      		"CORR_RA"
#define AGENT_GUIDER_STATS_CORR_DEC_ITEM_NAME      		"CORR_DEC"
#define AGENT_GUIDER_STATS_RMSE_RA_ITEM_NAME      		"RMSE_RA"
#define AGENT_GUIDER_STATS_RMSE_DEC_ITEM_NAME      		"RMSE_DEC"
#define AGENT_GUIDER_STATS_SNR_ITEM_NAME							"SNR"
#define AGENT_GUIDER_STATS_DELAY_ITEM_NAME						"DELAY"
#define AGENT_GUIDER_STATS_DITHERING_ITEM_NAME				"DITHERING"

#define AGENT_IMAGER_SEQUENCE_PROPERTY_NAME 					"AGENT_IMAGER_SEQUENCE"
#define AGENT_IMAGER_SEQUENCE_ITEM_NAME 							"SEQUENCE"

#define AGENT_WHEEL_FILTER_PROPERTY_NAME							"AGENT_WHEEL_FILTER"

#define AGENT_IMAGER_FOCUS_PROPERTY_NAME							"AGENT_IMAGER_FOCUS"
#define AGENT_IMAGER_FOCUS_INITIAL_ITEM_NAME    			"INITIAL"
#define AGENT_IMAGER_FOCUS_FINAL_ITEM_NAME  					"FINAL"
#define AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME     		"BACKLASH"
#define AGENT_IMAGER_FOCUS_BACKLASH_IN_ITEM_NAME     	"BACKLASH_IN"
#define AGENT_IMAGER_FOCUS_BACKLASH_OUT_ITEM_NAME     "BACKLASH_OUT"
#define AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM_NAME     "BACKLASH_OVERSHOOT_FACTOR"
#define AGENT_IMAGER_FOCUS_STACK_ITEM_NAME  					"STACK"
#define AGENT_IMAGER_FOCUS_REPEAT_ITEM_NAME						"REPEAT"
#define AGENT_IMAGER_FOCUS_DELAY_ITEM_NAME						"DELAY"

#define AGENT_IMAGER_FOCUS_FAILURE_PROPERTY_NAME			"AGENT_IMAGER_FOCUS_FAILURE"
#define AGENT_IMAGER_FOCUS_FAILURE_STOP_ITEM_NAME    	"STOP"
#define AGENT_IMAGER_FOCUS_FAILURE_RESTORE_ITEM_NAME  "RESTORE"

#define AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY_NAME			"AGENT_IMAGER_FOCUS_ESTIMATOR"
#define AGENT_IMAGER_FOCUS_ESTIMATOR_HFD_PEAK_ITEM_NAME    	"HFD_PEAK"
#define AGENT_IMAGER_FOCUS_ESTIMATOR_RMS_CONTRAST_ITEM_NAME		"RMS_CONTRAST"

#define AGENT_IMAGER_DITHERING_PROPERTY_NAME 					"AGENT_IMAGER_DITHERING"
#define AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM_NAME "AGGRESSIVITY"
#define AGENT_IMAGER_DITHERING_TIME_LIMIT_ITEM_NAME 	"TIME_LIMIT"
#define AGENT_IMAGER_DITHERING_SKIP_FRAMES_ITEM_NAME	"SKIP_FRAMES"

#define AGENT_IMAGER_STARS_PROPERTY_NAME							"AGENT_IMAGER_STARS"
#define AGENT_IMAGER_STARS_REFRESH_ITEM_NAME					"REFRESH"

#define AGENT_IMAGER_SELECTION_PROPERTY_NAME					"AGENT_IMAGER_SELECTION"
#define AGENT_IMAGER_SELECTION_X_ITEM_NAME						"X"
#define AGENT_IMAGER_SELECTION_Y_ITEM_NAME						"Y"
#define AGENT_IMAGER_SELECTION_RADIUS_ITEM_NAME				"RADIUS"
#define AGENT_IMAGER_SELECTION_SUBFRAME_ITEM_NAME			"SUBFRAME"

#define AGENT_IMAGER_STATS_PROPERTY_NAME							"AGENT_IMAGER_STATS"
#define AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME					"EXPOSURE"
#define AGENT_IMAGER_STATS_DELAY_ITEM_NAME						"DELAY"
#define AGENT_IMAGER_STATS_FRAME_ITEM_NAME						"FRAME"
#define AGENT_IMAGER_STATS_FRAMES_ITEM_NAME						"FRAMES"
#define AGENT_IMAGER_STATS_BATCH_ITEM_NAME						"BATCH"
#define AGENT_IMAGER_STATS_BATCHES_ITEM_NAME					"BATCHES"
#define AGENT_IMAGER_STATS_DRIFT_X_ITEM_NAME      		"DRIFT_X"
#define AGENT_IMAGER_STATS_DRIFT_Y_ITEM_NAME      		"DRIFT_Y"
#define AGENT_IMAGER_STATS_FWHM_ITEM_NAME							"FWHM"
#define AGENT_IMAGER_STATS_HFD_ITEM_NAME							"HFD"
#define AGENT_IMAGER_STATS_PEAK_ITEM_NAME							"PEAK"
#define AGENT_IMAGER_STATS_DITHERING_ITEM_NAME				"DITHERING"
#define AGENT_IMAGER_STATS_FOCUS_OFFSET_ITEM_NAME			"FOCUS_OFFSET"
#define AGENT_IMAGER_STATS_RMS_CONTRAST_ITEM_NAME			"RMS_CONTRAST"
#define AGENT_IMAGER_STATS_FOCUS_DEVIATION_ITEM_NAME	"BEST_FOCUS_DEVIATION"
#define AGENT_IMAGER_STATS_FRAMES_TO_DITHERING_ITEM_NAME	"FRAMES_TO_DITHERING"

#define AGENT_ALIGNMENT_POINT_PROPERY_NAME						"AGENT_ALIGNMENT_POINT_%d"
#define AGENT_ALIGNMENT_POINT_RA_ITEM_NAME   					"RA"
#define AGENT_ALIGNMENT_POINT_DEC_ITEM_NAME   				"DEC"
#define AGENT_ALIGNMENT_POINT_RAW_RA_ITEM_NAME  			"RAW_RA"
#define AGENT_ALIGNMENT_POINT_RAW_DEC_ITEM_NAME  			"RAW_DEC"
#define AGENT_ALIGNMENT_POINT_LST_ITEM_NAME  					"LST"
#define AGENT_ALIGNMENT_POINT_SOP_ITEM_NAME  					"SIDE_OF_PIER"

#define AGENT_SITE_DATA_SOURCE_PROPERTY_NAME					"AGENT_SITE_DATA_SOURCE"
#define AGENT_SITE_DATA_SOURCE_HOST_ITEM_NAME  				"HOST"
#define AGENT_SITE_DATA_SOURCE_MOUNT_ITEM_NAME  			"MOUNT"
#define AGENT_SITE_DATA_SOURCE_DOME_ITEM_NAME  				"DOME"
#define AGENT_SITE_DATA_SOURCE_GPS_ITEM_NAME  				"GPS"

#define AGENT_SET_HOST_TIME_PROPERTY_NAME							"AGENT_SET_HOST_TIME"
#define AGENT_SET_HOST_TIME_MOUNT_ITEM_NAME						"MOUNT"
#define AGENT_SET_HOST_TIME_DOME_ITEM_NAME						"DOME"

#define AGENT_LX200_SERVER_PROPERTY_NAME							"AGENT_LX200_SERVER"
#define AGENT_LX200_SERVER_STARTED_ITEM_NAME					"STARTED"
#define AGENT_LX200_SERVER_STOPPED_ITEM_NAME					"STOPPED"

#define AGENT_LX200_CONFIGURATION_PROPERTY_NAME				"AGENT_LX200_CONFIGURATION"
#define AGENT_LX200_CONFIGURATION_PORT_ITEM_NAME			"PORT"

#define AGENT_LIMITS_PROPERTY_NAME										"AGENT_LIMITS"
#define AGENT_HA_TRACKING_LIMIT_ITEM_NAME							"HA_TRACKING"
#define AGENT_LOCAL_TIME_LIMIT_ITEM_NAME							"LOCAL_TIME"
#define AGENT_COORDINATES_PROPAGATE_THESHOLD_ITEM_NAME	"COORDINATES_PROPAGATE_THESHOLD"

#define AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY_NAME	"AGENT_SCRIPTING_EXECUTE_SCRIPT"
#define AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY_NAME		"AGENT_SCRIPTING_DELETE_SCRIPT"
#define AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY_NAME	"AGENT_SCRIPTING_ON_LOAD_SCRIPT"
#define AGENT_SCRIPTING_ON_UNLOAD_SCRIPT_PROPERTY_NAME	"AGENT_SCRIPTING_ON_UNLOAD_SCRIPT"

#define AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME					"AGENT_SCRIPTING_SCRIPT_%d"
#define AGENT_SCRIPTING_SCRIPT_NAME_ITEM_NAME					"NAME"
#define AGENT_SCRIPTING_SCRIPT_ITEM_NAME							"SCRIPT"

#define AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME			"AGENT_SCRIPTING_ADD_SCRIPT"
#define AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM_NAME			"NAME"
#define AGENT_SCRIPTING_ADD_SCRIPT_ITEM_NAME					"SCRIPT"

#define AGENT_ASTROMETRY_INDEX_41XX_PROPERTY_NAME			"AGENT_ASTROMETRY_INDEX_41XX"

#define AGENT_ASTROMETRY_INDEX_42XX_PROPERTY_NAME			"AGENT_ASTROMETRY_INDEX_42XX"

#define AGENT_ASTAP_INDEX_PROPERTY_NAME								"AGENT_ASTAP_INDEX"

#define AGENT_PLATESOLVER_USE_INDEX_PROPERTY_NAME			"AGENT_PLATESOLVER_USE_INDEX"

#define AGENT_PLATESOLVER_HINTS_PROPERTY_NAME					"AGENT_PLATESOLVER_HINTS"
#define AGENT_PLATESOLVER_HINTS_RADIUS_ITEM_NAME			"RADIUS"
#define AGENT_PLATESOLVER_HINTS_RA_ITEM_NAME					"RA"
#define AGENT_PLATESOLVER_HINTS_DEC_ITEM_NAME					"DEC"
#define AGENT_PLATESOLVER_HINTS_EPOCH_ITEM_NAME				"J2000"
#define AGENT_PLATESOLVER_HINTS_SCALE_ITEM_NAME				"SCALE"
#define AGENT_PLATESOLVER_HINTS_PARITY_ITEM_NAME			"PARITY"
#define AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM_NAME	"DOWNSAMPLE"
#define AGENT_PLATESOLVER_HINTS_DEPTH_ITEM_NAME				"DEPTH"
#define AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM_NAME		"CPULIMIT"

#define AGENT_PLATESOLVER_WCS_PROPERTY_NAME						"AGENT_PLATESOLVER_WCS"
#define AGENT_PLATESOLVER_WCS_RA_ITEM_NAME    				"RA"
#define AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME    				"DEC"
#define AGENT_PLATESOLVER_WCS_EPOCH_ITEM_NAME				"J2000"
#define AGENT_PLATESOLVER_WCS_ANGLE_ITEM_NAME    			"ANGLE"
#define AGENT_PLATESOLVER_WCS_WIDTH_ITEM_NAME   			"WIDTH"
#define AGENT_PLATESOLVER_WCS_HEIGHT_ITEM_NAME   			"HEIGHT"
#define AGENT_PLATESOLVER_WCS_SCALE_ITEM_NAME					"SCALE"
#define AGENT_PLATESOLVER_WCS_PARITY_ITEM_NAME				"PARITY"
#define AGENT_PLATESOLVER_WCS_INDEX_ITEM_NAME					"INDEX"


/* OBSOLETED */
#define AGENT_PLATESOLVER_SYNC_PROPERTY_NAME					"AGENT_PLATESOLVER_SYNC"
#define AGENT_PLATESOLVER_SYNC_DISABLED_ITEM_NAME			"DISABLED"
#define AGENT_PLATESOLVER_SYNC_SYNC_ITEM_NAME					"SYNC"
#define AGENT_PLATESOLVER_SYNC_CENTER_ITEM_NAME				"CENTER"
#define AGENT_PLATESOLVER_SYNC_CALCULATE_PA_ERROR_ITEM_NAME		"CALCULATE_PA_ERROR"
#define AGENT_PLATESOLVER_SYNC_RECALCULATE_PA_ERROR_ITEM_NAME		"RECALCULATE_PA_ERROR"

#define AGENT_PLATESOLVER_START_SOLVE_ITEM_NAME				"SOLVE"
#define AGENT_PLATESOLVER_START_SYNC_ITEM_NAME				"SYNC"
#define AGENT_PLATESOLVER_START_CENTER_ITEM_NAME			"CENTER"
#define AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM_NAME		"CALCULATE_PA_ERROR"
#define AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM_NAME	"RECALCULATE_PA_ERROR"


#define AGENT_PLATESOLVER_PA_STATE_PROPERTY_NAME		"AGENT_PLATESOLVER_PA_STATE"
#define AGENT_PLATESOLVER_PA_STATE_ITEM_NAME			"STATE"
#define AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_2_ITEM_NAME		"DEC_DRIFT_2"
#define AGENT_PLATESOLVER_PA_STATE_DEC_DRIFT_3_ITEM_NAME		"DEC_DRIFT_3"
#define AGENT_PLATESOLVER_PA_STATE_TARGET_RA_ITEM_NAME			"TARGET_RA"
#define AGENT_PLATESOLVER_PA_STATE_TARGET_DEC_ITEM_NAME			"TARGET_DEC"
#define AGENT_PLATESOLVER_PA_STATE_CURRENT_RA_ITEM_NAME			"CURRENT_RA"
#define AGENT_PLATESOLVER_PA_STATE_CURRENT_DEC_ITEM_NAME		"CURRENT_DEC"
#define AGENT_PLATESOLVER_PA_STATE_ALT_ERROR_ITEM_NAME		    "ALT_POLAR_ERROR"
#define AGENT_PLATESOLVER_PA_STATE_AZ_ERROR_ITEM_NAME			"AZ_POLAR_ERROR"
#define AGENT_PLATESOLVER_PA_STATE_POLAR_ERROR_ITEM_NAME			"POLAR_ERROR"
#define AGENT_PLATESOLVER_PA_STATE_ALT_CORRECTION_UP_ITEM_NAME			"ALT_CORRECTION_UP"
#define AGENT_PLATESOLVER_PA_STATE_AZ_CORRECTION_CW_ITEM_NAME			"AZ_CORRECTION_CW"

#define AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY_NAME			"AGENT_PLATESOLVER_PA_SETTINGS"
#define AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM_NAME				"EXPOSURE"
#define AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM_NAME		"HA_MOVE"
#define AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM_NAME	"COMPENSATE_REFRACTION"

#define AGENT_PLATESOLVER_ABORT_PROPERTY_NAME					"AGENT_PLATESOLVER_ABORT"
#define AGENT_PLATESOLVER_ABORT_ITEM_NAME							"ABORT"

#define AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME					"AGENT_PLATESOLVER_IMAGE"
#define AGENT_PLATESOLVER_IMAGE_ITEM_NAME							"IMAGE"

#define AGENT_MOUNT_FOV_PROPERTY_NAME									"AGENT_MOUNT_FOV"
#define AGENT_MOUNT_FOV_ANGLE_ITEM_NAME    						"ANGLE"
#define AGENT_MOUNT_FOV_WIDTH_ITEM_NAME   						"WIDTH"
#define AGENT_MOUNT_FOV_HEIGHT_ITEM_NAME   						"HEIGHT"

#define SERVER_INFO_PROPERTY_NAME											"INFO"
#define SERVER_INFO_VERSION_ITEM_NAME									"VERSION"
#define SERVER_INFO_SERVICE_ITEM_NAME									"SERVICE"

#define SERVER_DRIVERS_PROPERTY_NAME									"DRIVERS"

#define SERVER_SERVERS_PROPERTY_NAME									"SERVERS"

#define SERVER_LOAD_PROPERTY_NAME											"LOAD"
#define SERVER_LOAD_ITEM_NAME													"DRIVER"

#define SERVER_UNLOAD_PROPERTY_NAME										"UNLOAD"
#define SERVER_UNLOAD_ITEM_NAME												"DRIVER"

#define SERVER_RESTART_PROPERTY_NAME									"RESTART"
#define SERVER_RESTART_ITEM_NAME											"RESTART"

#define SERVER_LOG_LEVEL_PROPERTY_NAME								"LOG_LEVEL"
#define SERVER_LOG_LEVEL_ERROR_ITEM_NAME							"ERROR"
#define SERVER_LOG_LEVEL_INFO_ITEM_NAME								"INFO"
#define SERVER_LOG_LEVEL_DEBUG_ITEM_NAME							"DEBUG"
#define SERVER_LOG_LEVEL_TRACE_ITEM_NAME							"TRACE"

#define SERVER_BLOB_BUFFERING_PROPERTY_NAME						"BLOB_BUFFERING"
#define SERVER_BLOB_BUFFERING_DISABLED_ITEM_NAME			"DISABLED"
#define SERVER_BLOB_BUFFERING_ENABLED_ITEM_NAME				"ENABLED"
#define SERVER_BLOB_COMPRESSION_ENABLED_ITEM_NAME			"COMPRESSION"

#define SERVER_BLOB_PROXY_PROPERTY_NAME								"BLOB_PROXY"
#define SERVER_BLOB_PROXY_DISABLED_ITEM_NAME					"DISABLED"
#define SERVER_BLOB_PROXY_ENABLED_ITEM_NAME						"ENABLED"

#define SERVER_FEATURES_PROPERTY_NAME									"FEATURES"
#define SERVER_BONJOUR_ITEM_NAME											"BONJOUR"
#define SERVER_CTRL_PANEL_ITEM_NAME										"CTRL_PANEL"
#define SERVER_WEB_APPS_ITEM_NAME											"WEB_APPS"

#define SERVER_WIFI_AP_PROPERTY_NAME									"WIFI_AP"
#define SERVER_WIFI_AP_SSID_ITEM_NAME									"SSID"
#define SERVER_WIFI_AP_PASSWORD_ITEM_NAME							"PASSWORD"

#define SERVER_WIFI_INFRASTRUCTURE_PROPERTY_NAME			"WIFI_INFRASTRUCTURE"
#define SERVER_WIFI_INFRASTRUCTURE_SSID_ITEM_NAME			"SSID"
#define SERVER_WIFI_INFRASTRUCTURE_PASSWORD_ITEM_NAME	"PASSWORD"

#define SERVER_WIFI_CHANNEL_PROPERTY_NAME							"WIFI_CHANNEL"
#define SERVER_WIFI_CHANNEL_ITEM_NAME									"CHANNEL"

#define SERVER_INTERNET_SHARING_PROPERTY_NAME					"INTERNET_SHARING"
#define SERVER_INTERNET_SHARING_DISABLED_ITEM_NAME		"DISABLED"
#define SERVER_INTERNET_SHARING_ENABLED_ITEM_NAME			"ENABLED"

#define SERVER_HOST_TIME_PROPERTY_NAME								"HOST_TIME"
#define SERVER_HOST_TIME_ITEM_NAME										"TIME"

#define SERVER_SHUTDOWN_PROPERTY_NAME									"SHUTDOWN"
#define SERVER_SHUTDOWN_ITEM_NAME											"SHUTDOWN"

#define SERVER_REBOOT_PROPERTY_NAME										"REBOOT"
#define SERVER_REBOOT_ITEM_NAME												"REBOOT"

#define SERVER_INSTALL_PROPERTY_NAME									"INSTALL"

#endif /* indigo_names_h */
