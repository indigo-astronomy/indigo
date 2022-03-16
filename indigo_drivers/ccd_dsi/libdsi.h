/*
 * Copyright (c) 2017, Rumen G.Bogdanovski <rumen@skyarchive.org>
 * Based on the code by Roland Roberts <roland@astrofoto.org>
 */

#ifndef __libdsi_h
#define __libdsi_h

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

struct DSI_CAMERA;

typedef struct DSI_CAMERA dsi_camera_t;

#define DSI_ID_LEN 32
#define DSI_NAME_LEN 32
#define DSI_BAYER_LEN 5
#define DSI_MAX_DEVICES 32
#define NO_TEMP_SENSOR  99999999

typedef char dsi_device_list[DSI_MAX_DEVICES][DSI_ID_LEN];

/**
 * DSI Command code mnemonics.
 *
 * These are mostly reverse engineered from running the camera under Windows
 * with SniffUSB.  Some codes are documented here, complete with mnenomics as
 * reported by others, but in some cases with unknown function (what is
 * CLEAR_TS?).
 */
enum DSI_COMMAND {
	PING                   = 0x00,
	RESET                  = 0x01,
	ABORT                  = 0x02,
	TRIGGER                = 0x03,
	CLEAR_TS               = 0x04,      /* not seen */
	GET_VERSION            = 0x14,
	GET_STATUS             = 0x15,
	GET_TIMESTAMP          = 0x16,
	GET_EEPROM_LENGTH      = 0x1e,
	GET_EEPROM_BYTE        = 0x1f,
	SET_EEPROM_BYTE        = 0x20,
	GET_GAIN               = 0x32,
	SET_GAIN               = 0x33,
	GET_OFFSET             = 0x34,
	SET_OFFSET             = 0x35,
	GET_EXP_TIME           = 0x36,
	SET_EXP_TIME           = 0x37,
	GET_EXP_MODE           = 0x38,
	SET_EXP_MODE           = 0x39,
	GET_VDD_MODE           = 0x3a,
	SET_VDD_MODE           = 0x3b,
	GET_FLUSH_MODE         = 0x3c,
	SET_FLUSH_MODE         = 0x3d,
	GET_CLEAN_MODE         = 0x3e,      /* not seen */
	SET_CLEAN_MODE         = 0x3f,      /* not seen */
	GET_READOUT_SPEED      = 0x40,
	SET_READOUT_SPEED      = 0x41,
	GET_READOUT_MODE       = 0x42,
	SET_READOUT_MODE       = 0x43,
	GET_READOUT_DELAY      = 0x44,
	SET_READOUT_DELAY      = 0x45,
	GET_ROW_COUNT_ODD      = 0x46,      /* not seen, seems to work */
	SET_ROW_COUNT_ODD      = 0x47,      /* not seen, probably a bad idea! */
	GET_ROW_COUNT_EVEN     = 0x48,      /* not seen, seems to work */
	SET_ROW_COUNT_EVEN     = 0x49,      /* not seen, probably a bad idea! */
	GET_TEMP               = 0x4a,
	GET_EXP_TIMER_COUNT    = 0x4b,
	PS_ON                  = 0x64,      /* not seen */
	PS_OFF                 = 0x65,      /* not seen */
	CCD_VDD_ON             = 0x66,      /* not seen */
	CCD_VDD_OFF            = 0x67,      /* not seen */
	AD_READ                = 0x68,      /* reportedly Envisage does this, when? */
	AD_WRITE               = 0x69,      /* reportedly Envisage does this, when? */
	TEST_PATTERN           = 0x6a,      /* not seen */
	GET_DEBUG_VALUE        = 0x6b,      /* not seen */
	GET_EEPROM_VIDPID      = 0x6c,      /* not seen */
	SET_EEPROM_VIDPID      = 0x6d,      /* not seen */
	ERASE_EEPROM           = 0x6e,      /* not seen */
};

typedef enum DSI_COMMAND dsi_command_t;

/**
 * DSI USB Speed mnemonics.
 *
 * The DSI camera can be operated at both USB 1.1 ("full" speed) and at USB
 * 2.0 ("high" speed) rates, depending on the actual bus connection.  However,
 * this driver only actually supports DSI_USB_SPEED_HIGH.
 */
enum DSI_USB_SPEED {
	DSI_USB_SPEED_INVALID = -1,
	DSI_USB_SPEED_FULL = 0,
	DSI_USB_SPEED_HIGH = 1,
};


enum DSI_FW_DEBUG {
	DSI_FW_DEBUG_INVALID = -1,
	DSI_FW_DEBUG_OFF = 0,
	DSI_FW_DEBUG_ON  = 1,
};

/**
 * DSI Imaging state mnemonics.  These apply only to the this driver; they are
 * bookkeeping settings.
 */
enum DSI_IMAGE_STATE {
	DSI_IMAGE_IDLE     = 0,
	DSI_IMAGE_EXPOSING = 1,
	DSI_IMAGE_ABORTING = 2,
};

/**
 * DSI readout speed mnemonics.
 *
 * I haven't been able to find out what this is doing under the hood, but it
 * presumably refers to an internal (firmware?) method of reading pixels.  I
 * know that some CCDs support reading a pixel more than once as a way of
 * reducing readout noise.  I do not know if that is what this refers to or
 * not.
 *
 * However, this setting is toggled between the low and high modes (along with
 * readout mode and VDD mode) when the exposure goes from below one second to
 * above one second.
 */
enum DSI_READOUT_SPEED {
	DSI_READOUT_SPEED_LOW  = 0,
	DSI_READOUT_SPEED_HIGH = 1,
};

/**
 * DSI readout mode mnemonics.
 *
 * The readout mode tells the DSI how we plan to read the image and needs to
 * be set before the exposure.  I think that "dual readout" normally refers to
 * a mode where two separate readout channels are used to read through the CCD
 * faster, but with increased readout noise.
 *
 * The Meade driver (and this driver) uses dual readout mode for exposures up
 * to one second, but single readout mode for longer exposures.
 */
enum DSI_READOUT_MODE {
	DSI_READOUT_MODE_DUAL   = 0,
	DSI_READOUT_MODE_SINGLE = 1,
	DSI_READOUT_MODE_ODD    = 2,
	DSI_READOUT_MODE_EVEN   = 3,
};

/**
 * DSI VDD mode mnemonics.
 *
 * VDD mode refers to whether or not the amplifier is on or off.  The DSI
 * supports three modes for this, on, off, and auto.  Short exposures simply
 * leave the amplifier on as turning it on and off takes time.  It may also
 * increase noise, but there are no good tests that show that.  At longer
 * exposures (more than one second), the Meade driver switches to auto which
 * basically turns the amplifier off until we try to read the image.
 *
 * We have no use cases for turning it off then manually turning it back on.
 */
enum DSI_VDD_MODE {
	DSI_VDD_MODE_AUTO = 0,
	DSI_VDD_MODE_ON   = 1,
	DSI_VDD_MODE_OFF  = 2,
};

/**
 * DSI flush mode mnemonics.
 *
 * "Flush" mode with CCDs describes the process of reading out the CCD to
 * prepare it for imaging.  This is normally a high-speed read-out whose whole
 * purpose is to clear any charge sitting in the pixel sites.  The DSI appears
 * to support three different flush modes, but it has been difficult to
 * document use cases when each mode is used by the Meade driver or when it
 * would be appropriate to use different modes.
 */
enum DSI_FLUSH_MODE {
	DSI_FLUSH_MODE_CONT   = 0,
	DSI_FLUSH_MODE_BEFORE = 1,
	DSI_FLUSH_MODE_NEVER  = 2,
};

/**
 * DSI flush mode mnemonics.
 */
enum DSI_BIN_MODE {
	BIN1X1 = 1,
	BIN2X2 = 2,
};

#define libdsi_inint() libusb_init(NULL)
#define libdsi_exit() libusb_exit(NULL)

void libdsi_set_verbose_init(int on);
int libdsi_get_verbose_init(void);

bool dsi_load_firmware(void);
int dsi_scan_usb(dsi_device_list devices);

dsi_camera_t *dsi_open_camera(const char *identifier);
void dsi_close_camera(dsi_camera_t *dsi);

void dsi_set_verbose(dsi_camera_t *dsi, int on);
int dsi_get_verbose(dsi_camera_t *dsi);

/* No setter; there is no thermal control for the DSI. */
double dsi_get_temperature(dsi_camera_t *dsi);
int dsi_get_bytespp(dsi_camera_t *dsi);

const char *dsi_get_chip_name(dsi_camera_t *dsi);
const char *dsi_get_model_name(dsi_camera_t *dsi);
const char *dsi_get_serial_number(dsi_camera_t *dsi);
const char *dsi_get_camera_name(dsi_camera_t *dsi);
const char *dsi_set_camera_name(dsi_camera_t *dsi, const char *name);
const char *dsi_get_bayer_pattern(dsi_camera_t *dsi);

int dsi_start_exposure(dsi_camera_t *dsi, double exptime);
int dsi_abort_exposure(dsi_camera_t *dsi);
void dsi_set_image_little_endian(dsi_camera_t *dsi, int little_endian);
int dsi_read_image(dsi_camera_t *dsi, unsigned char *buffer, int flags);

/* get frame width and height unaffected by binning */
int dsi_get_frame_width(dsi_camera_t *dsi);
int dsi_get_frame_height(dsi_camera_t *dsi);
/* get image width and height after binning */
int dsi_get_image_width(dsi_camera_t *dsi);
int dsi_get_image_height(dsi_camera_t *dsi);

double dsi_get_pixel_width(dsi_camera_t *dsi);
double dsi_get_pixel_height(dsi_camera_t *dsi);
double dsi_get_exposure_time_left(dsi_camera_t *dsi);

int dsi_set_binning(dsi_camera_t *dsi, enum DSI_BIN_MODE bin);
enum DSI_BIN_MODE dsi_get_max_binning(dsi_camera_t *dsi);
enum DSI_BIN_MODE dsi_get_binning(dsi_camera_t *dsi);

int dsi_set_amp_gain(dsi_camera_t *dsi, int gain);
int dsi_get_amp_gain(dsi_camera_t *dsi);

int dsi_set_amp_offset(dsi_camera_t *dsi, int offset);
int dsi_get_amp_offset(dsi_camera_t *dsi);

int dsi_reset_camera(dsi_camera_t *dsi);

int dsicmd_get_version(dsi_camera_t *dsi);

dsi_camera_t *dsitst_open(const char *chip_name);

#endif /* __libdsi_h */
