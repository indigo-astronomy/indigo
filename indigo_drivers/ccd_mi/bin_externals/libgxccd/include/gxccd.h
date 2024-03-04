/*
 * The Moravian Instruments (MI) camera library.
 *
 * Copyright (c) 2016-2024, Moravian Instruments <http://www.gxccd.com, linux@gxccd.com>
 * All rights reserved.
 *
 * Redistribution.  Redistribution and use in binary form, without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions must reproduce the above copyright notice and the
 *   following disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 * - Neither the name of Moravian Instruments nor the names of its
 *   suppliers may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 * - No reverse engineering, decompilation, or disassembly of this software
 *   is permitted.
 *
 * DISCLAIMER.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Default ETH Adapter IP address and port.
 * You can change these in adapter's web configuration and set path to your
 * config file with gxccd_configure() or pass new values to gxccd_configure_eth().
 */
#define GXETH_DEFAULT_IP   "192.168.0.5"
#define GXETH_DEFAULT_PORT 48899

/*
 * Data types used in this driver library.
 * Following types are defined in "stdint.h".
 * typedef short int            int16_t;
 * typedef unsigned char        uint8_t;
 * typedef unsigned short int   uint16_t;
 * typedef unsigned int         uint32_t;
 */

/* Typedef for prototype of enumeration function. */
typedef void (*enum_callback_t)(int /*device_id*/);

/* Typedef for prototype of anonymous camera structure. */
typedef struct camera camera_t;

/* Typedef for prototype of anonymous filter wheel structure. */
typedef struct fwheel fwheel_t;

/* =============================================================================
 * Structure of .ini configuration file.

  [driver]
  IP = 192.168.0.5
  Port = 48899
  ConnectTimeout = 1000
  SendTimeout = 3000
  ReceiveTimeout = 30000
  MicrometerFilterOffsets = false
  ClearTime = 15
  HWBinning = false
  BinningSum = false
  BinningSaturate = false

  [filters]
  Luminance, LGray, 0
  Red, LRed, 330
  Green, LGreen, 330
  Blue, LBlue, 330
  Clear, Black, 330

 *------------------------------------------------------------------------------
 * "IP" and "Port" are used for ETH adapter configuration and are ignored in
 * USB variant.
 * Values in [driver] section are used by default and therefore are optional.
 * "ConnectTimeout" is used when driver is connecting to adapter (in
 * gxccd_enumerate_eth() or gxccd_initialize_eth()).
 * "SendTimeout" and "ReceiveTimeout" are used during communication with
 * adapter.
 * All these values apply in erroneous states (network failure, adapter failure,
 * etc.) and it is not necessary to change them.
 * "MicrometerFilterOffsets" is explained in the following section.
 * "ClearTime" is interval in seconds in which the driver periodically clears
 * the chip. There is no need to change the default value (15 seconds).
 * If you want to use advanced USB functions below, you can turn this feature by
 * setting "ClearTime" to 0 or -1 and clear the chip manualy with gxusb_clear().
 * "HWBinning" controls whether the library bins the image itself or the camera
 * does the binning directly in hardware. This setting is valid only for C1x,
 * C2-9000, C3, C4 and C5 cameras.
 * "BinningSum" controls whether the library/camera sums binned pixels instead
 * of averaging them.
 * "BinningSaturate" controls whether the library/camera sets resulting binned
 * pixel to saturation value if any of the source pixels is saturated.
 *------------------------------------------------------------------------------
 * Section [filters] is for configuring cameras with filter wheel.
 * There is no way how to determine the actual filters in the filter wheel
 * automatically. You must create .ini file with this section.
 *
 * Every line in this section describes one filter position. Filter description
 * is a comma-separated list of three values:
 *
 *   1. Filter name:    This name is returned to the client application, which
 *                      can use it to list available filters in the filter wheel.
 *
 *   2. Filter color:   This color can be used by client application to display
 *                      the filter name with a color, hinting the filter type.
 *                      The color can be expressed by a name (see below) or
 *                      directly by decimal number representing the particular
 *                      color (0 is black, 16777215 (0xffffff) is white).
 *
 *   3. Filter offset:  Distance to move the focuser to refocus upon filter
 *                      change. Plan-parallel glass shifts the actual focus
 *                      position back for 1/3 of the glass thickness (exact
 *                      value depends on the glass refraction index, but for
 *                      almost all glasses 1/3 is very close to exact value).
 *                      Refocusing is useful when changing filters of different
 *                      thickness among exposures or when some exposures are
 *                      performed through filters and other without filters at
 *                      all.
 *
 * Posible filter color names are Red, LRed, Blue, LBlue, Green, LGreen, Cyan,
 * LCyan, Magenta, LMagenta, Yellow, LYellow, LGray, DGray, White, Black.
 *
 * Filter offsets can be defined in focuser dependent units (steps) or in
 * micrometers (μm). If the micrometers are used, it is necessary to inform
 * driver by the "MicrometerFilterOffsets" parameter in the [driver] section
 * of the ini file. Value of the "MicrometerFilterOffsets" parameter can be
 * expressed as keywords "true" or "false" (default) as well as numbers 1 (for
 * true) or 0 (for false).
 * ===========================================================================*/

/*
 * An attempt to load .ini configuration file is made during enumeration and
 * initialization. If the configuration file is not set, the code looks for
 * "gxccd.ini" or "gxccd.camera_id.ini" (for example "gxccd.1111.ini", where
 * "camera_id" is 1111) in .config directory located in current user's home
 * directory (e.g. /home/username/.config/gxccd.ini) or in directory with
 * application executable binary.
 * In the case of the filter wheel, the filename is "gxfw.ini" or
 * "gxfw.wheel_id.ini".
 * If none of these files is found, default values (mentioned above) are used.
 */

/*
 * Sets global path to your configuration .ini file.
 * You can pass NULL or empty string to use lookup process described above.
 */
void gxccd_configure(const char *ini_path);

/*
 * Configures ip address and/or port of ethernet adapter.
 * To configure port only, pass NULL or empty string in "ip".
 * To configure ip address only, pass 0 in "port".
 */
void gxccd_configure_eth(const char *ip, uint16_t port);

/*
 * Enumerates all cameras currently connected to your computer (_usb) or the
 * ethernet adapter (_eth).
 * You have to provide callback function, which is called for each connected
 * camera and the camera identifier (camera ID) is passed as an argument to this
 * callback function.
 * If your application is designed to work with one camera only or the camera
 * identifier is known, gxccd_enumerate_*() needs not to be called at all.
 */
/* USB variant */
void gxccd_enumerate_usb(enum_callback_t callback);
/* Ethernet variant */
void gxccd_enumerate_eth(enum_callback_t callback);

/*
 * Driver is designed to handle multiple cameras at once. It distinguishes
 * individual cameras using pointer to individual instances.
 * This function returns pointer to initialized structure or NULL in case of
 * error.
 * "camera_id" is camera indentifier (e.g. obtained by gxccd_enumerate_*()
 * function) and is required. If you have one camera connected, you can pass -1
 * as "camera_id".
 */
/* USB variant */
camera_t *gxccd_initialize_usb(int camera_id);
/* Ethernet variant */
camera_t *gxccd_initialize_eth(int camera_id);

/*
 * Disconnects from camera and releases other resources. The memory pointed by
 * "camera" becomes invalid and you must not pass it to any of the following
 * functions!
 */
void gxccd_release(camera_t *camera);

/*==============================================================================
 * RULES FOR THE FOLLOWING FUNCTIONS:
 * 1. Each function works with initialized camera_t structure.
 * 2. Every parameter is required.
 * 3. On success returns 0, on error the value -1 is returned and application
 *    can obtain error explanation by calling gxccd_get_last_error().
 *============================================================================*/

/* Standard gxccd_get_boolean_parameter() indexes */
enum
{
    GBP_CONNECTED = 0,             /* true if camera currently connected */
    GBP_SUB_FRAME,                 /* true if camera supports sub-frame read */
    GBP_READ_MODES,                /* true if camera supports multiple read modes */
    GBP_SHUTTER,                   /* true if camera is equipped with mechanical shutter */
    GBP_COOLER,                    /* true if camera is equipped with active chip cooler */
    GBP_FAN,                       /* true if camera fan can be controlled */
    GBP_FILTERS,                   /* true if camera controls filter wheel */
    GBP_GUIDE,                     /* true if camera is capable to guide the telescope
                             mount */
    GBP_WINDOW_HEATING,            /* true if camera can control the chip window heating */
    GBP_PREFLASH,                  /* true if camera can use chip preflash */
    GBP_ASYMMETRIC_BINNING,        /* true if camera horizontal and vertical binning
                             can differ */
    GBP_MICROMETER_FILTER_OFFSETS, /* true if filter focusing offsets are
                                    expressed in micrometers */
    GBP_POWER_UTILIZATION,         /* true if camera can return power utilization in
                             gxccd_get_value() */
    GBP_GAIN,                      /* true if camera can return gain in gxccd_get_value() */
    GBP_ELECTRONIC_SHUTTER,        /* true if camera has electronic shutter */
    GBP_GPS,                       /* true if camera has gps module */
    GBP_CONTINUOUS_EXPOSURES,      /* true if camera supports continuous exposures */
    GBP_TRIGGER,                   /* true if camera supports trigger port */
    GBP_CONFIGURED = 127,          /* true if camera is configured */
    GBP_RGB,                       /* true if camera has Bayer RGBG filters on the chip */
    GBP_CMY,                       /* true if camera has CMY filters on the chip */
    GBP_CMYG,                      /* true if camera has CMYG filters on the chip */
    GBP_DEBAYER_X_ODD,             /* true if camera Bayer masks starts on horizontal
                             odd pixel */
    GBP_DEBAYER_Y_ODD,             /* true if camera Bayer masks starts on vertical
                             odd pixel */
    GBP_INTERLACED = 256,           /* true if chip is interlaced
                             (else progressive) */
    GBP_HEX_VERSION_NUMBER = 1024  /* true if GIP_FIRMWARE_MAJOR should be represented
                                   as hexadecimal number */
};

/* Returns true or false in "value" depending on the "index". */
int gxccd_get_boolean_parameter(camera_t *camera, int index, bool *value);

/* Standard gxccd_get_integer_parameter() indexes */
enum
{
    GIP_CAMERA_ID = 0,        /* Identifier of the current camera */
    GIP_CHIP_W,               /* Chip width in pixels */
    GIP_CHIP_D,               /* Chip depth in pixels */
    GIP_PIXEL_W,              /* Chip pixel width in nanometers */
    GIP_PIXEL_D,              /* Chip pixel depth in nanometers */
    GIP_MAX_BINNING_X,        /* Maximum binning in horizontal direction */
    GIP_MAX_BINNING_Y,        /* Maximum binning in vertical direction */
    GIP_READ_MODES,           /* Number of read modes offered by the camera */
    GIP_FILTERS,              /* Number of filters offered by the camera */
    GIP_MINIMAL_EXPOSURE,     /* Shortest exposure time in microseconds (µs) */
    GIP_MAXIMAL_EXPOSURE,     /* Longest exposure time in milliseconds (ms) */
    GIP_MAXIMAL_MOVE_TIME,    /* Longest time to move the telescope in
                                 milliseconds (ms) */
    GIP_DEFAULT_READ_MODE,    /* Read mode to be used as default */
    GIP_PREVIEW_READ_MODE,    /* Read mode to be used for preview (fast read) */
    GIP_MAX_WINDOW_HEATING,   /* Maximal value for gxccd_set_window_heating() */
    GIP_MAX_FAN,              /* Maximal value for gxccd_set_fan() */
    GIP_MAX_GAIN,             /* Maximal value for gxccd_set_gain() */
    GIP_MAX_PIXEL_VALUE,      /* Maximal possible pixel value. May vary with read mode and binning,
                                 read only after gxccd_set_read_mode() and gxccd_set_binning() calls */
    GIP_FIRMWARE_MAJOR = 128, /* Camera firmware version (optional) */
    GIP_FIRMWARE_MINOR,
    GIP_FIRMWARE_BUILD,
    GIP_DRIVER_MAJOR,         /* This library version */
    GIP_DRIVER_MINOR,
    GIP_DRIVER_BUILD,
    GIP_FLASH_MAJOR,          /* Flash version (optional) */
    GIP_FLASH_MINOR,
    GIP_FLASH_BUILD,
};

/* Returns integer in "value" depending on the "index". */
int gxccd_get_integer_parameter(camera_t *camera, int index, int *value);

/* Standard gxccd_get_string_parameter() indexes */
enum
{
    GSP_CAMERA_DESCRIPTION = 0, /* Camera description */
    GSP_MANUFACTURER,           /* Manufacturer name */
    GSP_CAMERA_SERIAL,          /* Camera serial number */
    GSP_CHIP_DESCRIPTION        /* Used chip description */
};

/*
 * Returns string in "buf" depending on the "index".
 * The caller must specify the size of the buffer in "size".
 */
int gxccd_get_string_parameter(camera_t *camera, int index, char *buf, size_t size);

/* Standard gxccd_get_value() indexes */
enum
{
    GV_CHIP_TEMPERATURE = 0,    /* Current temperature of the chip
                                 in deg. Celsius */
    GV_HOT_TEMPERATURE,         /* Current temperature of the cooler hot side
                                 in deg. Celsius */
    GV_CAMERA_TEMPERATURE,      /* Current temperature inside the camera
                                 in deg. Celsius */
    GV_ENVIRONMENT_TEMPERATURE, /* Current temperature of the environment air
                                 in deg. Celsius */
    GV_SUPPLY_VOLTAGE = 10,     /* Current voltage of the camera power supply */
    GV_POWER_UTILIZATION,       /* Current utilization of the chip cooler
                                 (rational number from 0.0 to 1.0) */
    GV_ADC_GAIN = 20            /* Current gain of A/D converter in electrons/ADU */
};

/* Returns float in "value" depending on the "index". */
int gxccd_get_value(camera_t *camera, int index, float *value);

/*
 * Sets the required chip temperature.
 * If the camera has no cooler, this function has no effect.
 * "temp" is expressed in degrees Celsius.
 */
int gxccd_set_temperature(camera_t *camera, float temp);

/*
 * Sets the maximum speed with which the driver changes chip temperature.
 * If the camera has no cooler, this function has no effect.
 * "temp_ramp" is expressed in degrees Celsius per minute.
 */
int gxccd_set_temperature_ramp(camera_t *camera, float temp_ramp);

/*
 * Sets the required read binning.
 * If the camera does not support binning, this function has no effect.
 */
int gxccd_set_binning(camera_t *camera, int x, int y);

/*
 * If the camera is equipped with preflash electronics, this function sets it.
 * "preflash_time" defines time for which the preflash LED inside the camera is
 * switched on. "clear_num" defines how many times the chip has to be cleared
 * after the preflash. Actual values of these parameters depends on
 * the particular camera model (e.g. number and luminance of the LEDs used etc.).
 * Gx series of cameras typically need less than 1 second to completely
 * saturate the chip ("preflash_time"). Number of subsequent clears should be
 * at last 2, but more than 4 or 5 clears is not useful, too.
 */
int gxccd_set_preflash(camera_t *camera, double preflash_time, int clear_num);

/*
 * Starts new exposure.
 * "exp_time" is the required exposure time in seconds. "use_shutter" parameter
 * tells the driver the dark frame (without light) has to be acquired (false),
 * or the shutter has to be opened and closed to acquire normal light image (true).
 * Sub-frame coordinates are passed in "x", "y", "w" and "h".
 * If the camera does not support sub-frame read, "x" and "y" must be 0 and "w"
 * and "h" must be the chip pixel dimensions.
 * The y-axis grows down, 0 is at the top.
 */
int gxccd_start_exposure(camera_t *camera, double exp_time, bool use_shutter, int x, int y, int w, int h);

/*
 * The camera enters waiting state and when a signal is detected on trigger port,
 * it starts a new exposure.
 * The parameters are the same as for gxccd_start_exposure.
 */
int gxccd_start_exposure_trigger(camera_t *camera, double exp_time, bool use_shutter, int x, int y, int w, int h);

/*
 * When the exposure already started by gxccd_start_exposure() call has to be
 * terminated before the exposure time expires, this function has to be called.
 * Parameter "download" indicates whether the image should be digitized, because
 * the user will call gxccd_read_image() later or the image should be discarded.
 */
int gxccd_abort_exposure(camera_t *camera, bool download);

/*
 * When the exposure already started by gxccd_start_exposure() call, parameter
 * "ready" is false if the exposure is still running. When the exposure finishes
 * and it is possible to call gxccd_read_image(), parameter "ready" is true.
 * It is recommended to count the exposure time in the application despite
 * the fact the exact exposure time is determined by the camera/driver and to
 * start calling of gxccd_image_ready() only upon the exposure time expiration.
 * Starting to call gxccd_image_ready() in the infinite loop immediately after
 * gxccd_start_exposure() and call it for the whole exposure time (and thus
 * keeping at last one CPU core at 100% utilization) is a bad programming
 * manner (politely expressed).
 */
int gxccd_image_ready(camera_t *camera, bool *ready);

/*
 * When gxccd_image_ready() returns "ready" == true, it is possible to call
 * gxccd_read_image(). Driver returns 16 bits per pixel (2 bytes) matrix copied
 * to "buf" address. The buffer must be allocated by the caller, driver does not
 * allocate any memory. The "size" parameter specifies allocated memory block
 * length in bytes (not in pixels!). It has to be greater or equal to image size
 * in bytes else the function fails.
 * Application can use: size = wanted_w * 2 * wanted_h;
 *
 * Format of the returned buffer:
 *   - one-dimensional array formed from lines (rows) stacked one after another
 *   - orientation of the image is similar to Cartesian coordinate system,
 *     pixel [0, 0] (first line) is located at bottom left of the resulting image,
 *     x coordinate grows right and y coordinate grows up
 *   - data is in little-endian encoding -> lower byte first
 *
 * Example with width = 2000px and height = 1000px:
 *   - allocate one-dimensional buffer: malloc(2000*2*1000) (malloc(width*2*height))
 *   - bottom left pixel's lower byte is located at buffer[0] and higher byte is
 *     at buffer[1]
 *   - first line has width = 2000 * 2 (bytes) -> bottom right pixel is located
 *     at buffer[3998] and buffer[3999] (width * 2 - 2 and width * 2 - 1)
 *   - top left pixel is at buffer[3996000] and buffer[3996001]
 *     ((height - 1) * width * 2 and (height - 1) * width * 2 + 1)
 *   - top right pixel is at buffer[3999998] and buffer[3999999]
 *     ((height - 1) * width * 2 + width * 2 - 2 and (height - 1) * width * 2
 *     + width * 2 - 1)
 */
int gxccd_read_image(camera_t *camera, void *buf, size_t size);

/*
 * Functionality and parameters are the same as for gxccd_read_image.
 * The only difference is that the camera starts another exposure before returning
 * the image. The time between continous exposures is therefore reduced by the time
 * of downloading the image to the PC.
 */
int gxccd_read_image_exposure(camera_t *camera, void *buf, size_t size);

/*
 * Open camera shutter.
 */
int gxccd_open_shutter(camera_t *camera);

/*
 * Close camera shutter.
 */
int gxccd_close_shutter(camera_t *camera);

/*
 * Enumerates all read modes provided by the camera.
 * This enumeration does not use any callback, the caller passes index
 * beginning with 0 and repeats the call with incremented index until the call
 * returns -1.
 * The caller must specify the size of the buffer in parameter "size".
 */
int gxccd_enumerate_read_modes(camera_t *camera, int index, char *buf, size_t size);

/*
 * Sets required read mode.
 * "mode" is the "index" used in gxccd_enumerate_read_modes() call.
 */
int gxccd_set_read_mode(camera_t *camera, int mode);

/*
 * Sets required gain. Range of parameter "gain" depends on particular camera
 * hardware, as it typically represents directly a register value.
 * This method is chosen to allow to control gain as precisely as each particular
 * camera allows. Low limit is 0, high limit is returned by function
 * gxccd_get_integer_parameter with index GIP_MAX_GAIN.
 */
int gxccd_set_gain(camera_t *camera, uint16_t gain);

/*
 * As the gxccd_set_gain function accepts camera-dependent parameter gain,
 * which typically does not represent actual gain as signal multiply or value in dB,
 * a helper function gxccd_convert_gain is provided to convert register value into
 * gain in logarithmic dB units as well as in linear times-signal units.
 */
int gxccd_convert_gain(camera_t *camera, uint16_t gain, double *db, double *times);

/*
 * Enumerates all filters provided by the camera.
 * This enumeration does not use any callback, by the caller passes index
 * beginning with 0 and repeats the call with incremented index until the call
 * returns -1.
 * Returns filter name in "buf". The caller must specify the size of the buffer
 * in parameter size.
 * "color" parameter hints the RGB color (e.g. cyan color is 0x00ffff), which
 * can be used to draw the filter name in the application.
 * "offset" indicates the focuser shift when the particular filter is selected.
 * Units of the "offset" can be micrometers or arbitrary focuser specific units
 * (steps). If the units used are micrometers, driver returns true from
 * gxccd_get_boolean_parameter() with GBP_MICROMETER_FILTER_OFFSETS "index".
 */
int gxccd_enumerate_filters(camera_t *camera, int index, char *buf, size_t size, uint32_t *color, int *offset);

/*
 * Sets the required filter.
 * If the camera is not equipped with filter wheel, this function has no effect.
 */
int gxccd_set_filter(camera_t *camera, int index);

/*
 * Reinitializes camera filter wheel.
 * If parameter "num_filters" is not NULL, it will contain the number of detected
 * filters or 0 in case of error (or camera without filter wheel).
 */
int gxccd_reinit_filter_wheel(camera_t *camera, int *num_filters);

/*
 * If the camera is equipped with cooling fan and allows its control,
 * this function sets the fan rotation speed.
 * The maximum value of the "speed" parameter should be determined by
 * gxccd_get_integer_parameter() call with GIP_MAX_FAN "index".
 * If the particular camera supports only on/off switching, the maximum value
 * should be 1 (fan on), while value 0 means fan off.
 */
int gxccd_set_fan(camera_t *camera, uint8_t speed);

/*
 * If the camera is equipped with chip cold chamber front window heater
 * and allows its control, this function sets heating intensity.
 * The maximum value of the "heating" parameter should be determined by
 * gxccd_get_integer_parameter() call with GIP_MAX_WINDOW_HEATING "index".
 * If the particular camera supports only on/off switching, the maximum value
 * should be 1 (heating on), while value 0 means heating off.
 */
int gxccd_set_window_heating(camera_t *camera, uint8_t heating);

/*
 * Instructs the camera to initiate telescope movement in the R.A. and/or Dec.
 * axis for the defined period of time (in milliseconds).
 * The sign of the parameters defines the movement direction in the respective
 * coordinate. The maximum length is approx 32.7 seconds.
 * If the camera is not equipped with autoguider port, this function has no
 * effect.
 */
int gxccd_move_telescope(camera_t *camera, int16_t ra_duration_ms, int16_t dec_duration_ms);

/*
 * Sets "moving" to true if the movement started with gxccd_move_telescope()
 * call is still in progress.
 */
int gxccd_move_in_progress(camera_t *camera, bool *moving);

/*
 * Returns actual date and exact time of the last image exposure.
 * Date and time is obtained from GPS and is in UTC time standard.
 * Subsecond precision is additionally achieved with internal camera counter.
 * For this function to work, the camera must contain a GPS receiver module and
 * it must be synchronized with at least 5 satellites. You can call
 * "gxccd_get_gps_data()" to obtain GPS status.
 */
int gxccd_get_image_time_stamp(camera_t *camera, int *year, int *month,
                               int *day, int *hour, int *minute, double *second);

/*
 * Returns actual date, exact time, latitude, longitude, mean sea level and
 * status of GPS module. Date and time is in UTC time standard. Subsecond precision
 * is additionally achieved with internal camera counter. For this function to work,
 * the camera must contain a GPS receiver module. Position information needs at
 * least 3 satellites, date and time is returned after synchronization with 5
 * satellites.
 */
int gxccd_get_gps_data(camera_t *camera, double *lat, double *lon, double *msl,
                       int *year, int *month, int *day, int *hour, int *minute,
                       double *second, unsigned int *satellites, bool *fix);

/*
 * If any call fails (returns -1), this function returns failure description
 * in parameter buf.
 * The caller must specify the size of the buffer in parameter "size".
 */
void gxccd_get_last_error(camera_t *camera, char *buf, size_t size);

/*==============================================================================
 * External Filter Wheels
 *============================================================================*/

/*
 * Enumerates all filter wheels currently connected to your computer (_usb) or
 * the ethernet adapter (_eth).
 * You have to provide callback function, which is called for each connected
 * wheel and the filter wheel identifier (filter wheel ID) is passed as
 * an argument to this callback function.
 * If your application is designed to work with one wheel only or the filter
 * wheel identifier is known, gxfw_enumerate_*() needs not to be called at all.
 */
/* USB variant */
void gxfw_enumerate_usb(enum_callback_t callback);
/* Ethernet variant */
void gxfw_enumerate_eth(enum_callback_t callback);

/*
 * Driver is designed to handle multiple filter wheels at once. It distinguishes
 * individual wheels using pointer to individual instances.
 * This function returns pointer to initialized structure or NULL in case of
 * error.
 * "wheel_id" is camera indentifier (e.g. obtained by gxfw_enumerate_*()
 * function) and is required. If you have one filter wheel connected, you can
 * pass -1 as "wheel_id".
 */
/* USB variant */
fwheel_t *gxfw_initialize_usb(int wheel_id);
/* Ethernet variant */
fwheel_t *gxfw_initialize_eth(int wheel_id);

/*
 * Disconnects from filter wheel and releases other resources. The memory
 * pointed by "wheel" becomes invalid and you must not pass it to any of the
 * following functions!
 */
void gxfw_release(fwheel_t *wheel);

/*==============================================================================
 * RULES FOR THE FOLLOWING FUNCTIONS:
 * 1. Each function works with initialized fwheel_t structure.
 * 2. Every parameter is required.
 * 3. On success returns 0, on error the value -1 is returned and application
 *    can obtain error explanation by calling gxfw_get_last_error().
 *============================================================================*/

/* Standard gxfw_get_boolean_parameter() indexes */
enum
{
    FW_GBP_CONNECTED = 0,             /* true if wheel is connected */
    FW_GBP_INITIALIZED,               /* true if wheel is initialized */
    FW_GBP_MICROMETER_FILTER_OFFSETS, /* true if filter focusing offsets are
                                       expressed in micrometers */
    FW_GBP_CONFIGURED = 127,          /* true if wheel is configured */
};

/* Returns true or false in "value" depending on the "index". */
int gxfw_get_boolean_parameter(fwheel_t *wheel, int index, bool *value);

/* Standard gxfw_get_integer_parameter() indexes */
enum
{
    FW_GIP_VERSION_1 = 0, /* Wheel firmware version (optional) */
    FW_GIP_VERSION_2,     /* -//- */
    FW_GIP_VERSION_3,     /* -//- */
    FW_GIP_VERSION_4,     /* -//- */
    FW_GIP_WHEEL_ID,      /* Identifier of the current filter wheel */
    FW_GIP_FILTERS,       /* Number of filters offered by the filter wheel */
};

/* Returns integer in "value" depending on the "index". */
int gxfw_get_integer_parameter(fwheel_t *wheel, int index, int *value);

/* Standard gxfw_get_string_parameter() indexes */
enum
{
    FW_GSP_DESCRIPTION = 0, /* Filter wheel description */
    FW_GSP_MANUFACTURER,    /* Manufacturer name */
    FW_GSP_SERIAL_NUMBER,   /* Filter wheel serial number */
};

/*
 * Returns string in "buf" depending on the "index".
 * The caller must specify the size of the buffer in "size".
 */
int gxfw_get_string_parameter(fwheel_t *wheel, int index, char *buf, size_t size);

/*
 * Enumerates all filters provided by the filter wheel.
 * This enumeration does not use any callback, by the caller passes index
 * beginning with 0 and repeats the call with incremented index until the call
 * returns -1.
 * Returns filter name in "buf". The caller must specify the size of the buffer
 * in parameter size.
 * "color" parameter hints the RGB color (e.g. cyan color is 0x00ffff), which
 * can be used to draw the filter name in the application.
 * "offset" indicates the focuser shift when the particular filter is selected.
 * Units of the "offset" can be micrometers or arbitrary focuser specific units
 * (steps). If the units used are micrometers, driver returns true from
 * gxfw_get_boolean_parameter() with FW_GBP_MICROMETER_FILTER_OFFSETS "index".
 */
int gxfw_enumerate_filters(fwheel_t *wheel, int index, char *buf, size_t size, uint32_t *color, int *offset);

/*
 * Sets the required filter.
 */
int gxfw_set_filter(fwheel_t *wheel, int index);

/*
 * Reinitializes the filter wheel.
 * If parameter "num_filters" is not NULL, it will contain the number of detected
 * filters or 0 in case of error.
 */
int gxfw_reinit_filter_wheel(fwheel_t *wheel, int *num_filters);

/*
 * If any call fails (returns -1), this function returns failure description
 * in parameter buf.
 * The caller must specify the size of the buffer in parameter "size".
 */
void gxfw_get_last_error(fwheel_t *wheel, char *buf, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
