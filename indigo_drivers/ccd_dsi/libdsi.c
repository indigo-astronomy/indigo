/*
 * Copyright (c) 2017, Rumen G.Bogdanovski <rumen@skyarchive.org>
 * Based on the code by Roland Roberts <roland@astrofoto.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "libdsi.h"
#include "libdsi_firmware.h"

static int dsicmd_command_1(dsi_camera_t *dsi, dsi_command_t cmd);
static int dsicmd_command_2(dsi_camera_t *dsi, dsi_command_t cmd, int );
static int dsicmd_command_3(dsi_camera_t *dsi, dsi_command_t cmd, int, int);
static int dsicmd_command_4(dsi_camera_t *dsi, dsi_command_t cmd, int, int, int);
static int dsicmd_usb_command(dsi_camera_t *dsi, unsigned char *ibuf, int ibuf_len, int obuf_len);

static int verbose_init = 0;

struct DSI_CAMERA {
	struct libusb_device *device;
	struct libusb_device_handle *handle;
	unsigned char command_sequence_number;

	int is_simulation;
	int eeprom_length;
	int test_pattern;

	int exposure_time;
	int read_height_even;
	int read_height_odd;
	int read_height;
	int read_width;
	int read_bpp;
	int image_width;
	int image_height;
	int image_offset_x;
	int image_offset_y;
	int has_temperature_sensor;
	int is_binnable;
	int is_interlaced;
	int little_endian_data;
	char bayer_pattern[DSI_BAYER_LEN];

	double pixel_size_x;
	double pixel_size_y;

	int amp_gain_pct;
	int amp_offset_pct;

	char chip_name[DSI_NAME_LEN];
	char camera_name[DSI_NAME_LEN];
	char model_name[DSI_NAME_LEN];
	char serial_number[DSI_NAME_LEN];

	enum DSI_FW_DEBUG fw_debug;
	enum DSI_USB_SPEED usb_speed;
	enum DSI_BIN_MODE bin_mode;

	union {
		int value;
		unsigned char s[4];
		struct {
			unsigned char family;
			unsigned char model;
			unsigned char version;
			unsigned char revision;
		} c;
	} version;

	int read_command_timeout;
	int write_command_timeout;
	int read_image_timeout;

	enum DSI_IMAGE_STATE imaging_state;

	unsigned int last_time;
	int log_commands;

	size_t read_size_odd, read_size_even;
	unsigned char *read_buffer_odd;
	unsigned char *read_buffer_even;
};


/**
 * Look up the human-readable mnemonic for a numeric command code.
 *
 * @param cmd Command code to look up.
 * @param buffer Space to write the string mnemonic.
 * @param bufsize Size of write buffer.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *dsicmd_lookup_command_name_r(dsi_command_t cmd, char *buffer, int bufsize) {
	const char *bufptr = 0;

	switch(cmd) {
		case PING:
			bufptr = "PING";
			break;
		case RESET:
			bufptr = "RESET";
			break;
		case ABORT:
			bufptr = "ABORT";
			break;
		case TRIGGER:
			bufptr = "TRIGGER";
			break;
		case CLEAR_TS:
			bufptr = "CLEAR_TS";
			break;
		case GET_VERSION:
			bufptr = "GET_VERSION";
			break;
		case GET_STATUS:
			bufptr = "GET_STATUS";
			break;
		case GET_TIMESTAMP:
			bufptr = "GET_TIMESTAMP";
			break;
		case GET_EEPROM_LENGTH:
			bufptr = "GET_EEPROM_LENGTH";
			break;
		case GET_EEPROM_BYTE:
			bufptr = "GET_EEPROM_BYTE";
			break;
		case SET_EEPROM_BYTE:
			bufptr = "SET_EEPROM_BYTE";
			break;
		case GET_GAIN:
			bufptr = "GET_GAIN";
			break;
		case SET_GAIN:
			bufptr = "SET_GAIN";
			break;
		case GET_OFFSET:
			bufptr = "GET_OFFSET";
			break;
		case SET_OFFSET:
			bufptr = "SET_OFFSET";
			break;
		case GET_EXP_TIME:
			bufptr = "GET_EXP_TIME";
			break;
		case SET_EXP_TIME:
			bufptr = "SET_EXP_TIME";
			break;
		case GET_EXP_MODE:
			bufptr = "GET_EXP_MODE";
			break;
		case SET_EXP_MODE:
			bufptr = "SET_EXP_MODE";
			break;
		case GET_VDD_MODE:
			bufptr = "GET_VDD_MODE";
			break;
		case SET_VDD_MODE:
			bufptr = "SET_VDD_MODE";
			break;
		case GET_FLUSH_MODE:
			bufptr = "GET_FLUSH_MODE";
			break;
		case SET_FLUSH_MODE:
			bufptr = "SET_FLUSH_MODE";
			break;
		case GET_CLEAN_MODE:
			bufptr = "GET_CLEAN_MODE";
			break;
		case SET_CLEAN_MODE:
			bufptr = "SET_CLEAN_MODE";
			break;
		case GET_READOUT_SPEED:
			bufptr = "GET_READOUT_SPEED";
			break;
		case SET_READOUT_SPEED:
			bufptr = "SET_READOUT_SPEED";
			break;
		case GET_READOUT_MODE:
			bufptr = "GET_READOUT_MODE";
			break;
		case SET_READOUT_MODE:
			bufptr = "SET_READOUT_MODE";
			break;
		case GET_READOUT_DELAY:
			bufptr = "GET_NORM_READOUT_DELAY";
			break;
		case SET_READOUT_DELAY:
			bufptr = "SET_NORM_READOUT_DELAY";
			break;
		case GET_ROW_COUNT_ODD:
			bufptr = "GET_ROW_COUNT_ODD";
			break;
		case SET_ROW_COUNT_ODD:
			bufptr = "SET_ROW_COUNT_ODD";
			break;
		case GET_ROW_COUNT_EVEN:
			bufptr = "GET_ROW_COUNT_EVEN";
			break;
		case SET_ROW_COUNT_EVEN:
			bufptr = "SET_ROW_COUNT_EVEN";
			break;
		case GET_TEMP:
			bufptr = "GET_TEMP";
			break;
		case GET_EXP_TIMER_COUNT:
			bufptr = "GET_EXP_TIMER_COUNT";
			break;
		case PS_ON:
			bufptr = "PS_ON";
			break;
		case PS_OFF:
			bufptr = "PS_OFF";
			break;
		case CCD_VDD_ON:
			bufptr = "CCD_VDD_ON";
			break;
		case CCD_VDD_OFF:
			bufptr = "CCD_VDD_OFF";
			break;
		case AD_READ:
			bufptr = "AD_READ";
			break;
		case AD_WRITE:
			bufptr = "AD_WRITE";
			break;
		case TEST_PATTERN:
			bufptr = "TEST_PATTERN";
			break;
		case GET_DEBUG_VALUE:
			bufptr = "GET_DEBUG_VALUE";
			break;
		case GET_EEPROM_VIDPID:
			bufptr = "GET_EEPROM_VIDPID";
			break;
		case SET_EEPROM_VIDPID:
			bufptr = "SET_EEPROM_VIDPID";
			break;
		case ERASE_EEPROM:
			bufptr = "ERASE_EEPROM";
			break;
	}
	if (bufptr != 0) {
		snprintf(buffer, bufsize, "%s", bufptr);
	} else {
		snprintf(buffer, bufsize, "CMD_UNKNOWN, 0x%02x", cmd);
	}
	return buffer;
}

/**
 * Look up the human-readable mnemonic for a numeric command code, non-reentrant.
 *
 * @param cmd Command code to look up.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *dsicmd_lookup_command_name(dsi_command_t cmd) {
	static char scratch[100];
	return dsicmd_lookup_command_name_r(cmd, scratch, 100);
}

/**
 * Look up the human-readable mnemonic for a numeric imaging state code,
 * non-reentrant.
 *
 * @param state Imaging state code to look up.
 * @param buffer Space to write the string mnemonic.
 * @param bufsize Size of write buffer.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *dsicmd_lookup_image_state_r(enum DSI_IMAGE_STATE state, char *buffer, int bufsize) {
	char *bufptr = 0;

	switch(state) {
		case DSI_IMAGE_IDLE:
			bufptr = "DSI_IMAGE_IDLE";
			break;
		case DSI_IMAGE_EXPOSING:
			bufptr = "DSI_IMAGE_EXPOSING";
			break;
		case DSI_IMAGE_ABORTING:
			bufptr = "DSI_IMAGE_ABORTING";
			break;
	}
	if (bufptr != 0) {
		snprintf(buffer, bufsize, "%s", bufptr);
	} else {
		snprintf(buffer, bufsize, "DSI_IMAGE_UNKNOWN, 0x%02x", state);
	}
	return buffer;
}

/**
 * Look up the human-readable mnemonic for a numeric imaging state code,
 * non-reentrant.
 *
 * @param state Imaging state code to look up.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *dsicmd_lookup_image_state(enum DSI_IMAGE_STATE state) {
	/* not thread safe. */
	static char unknown[100];
	return dsicmd_lookup_image_state_r(state, unknown, 100);
}

/**
 * Look up the human-readable mnemonic for a USB speed code.
 *
 * @param speed USB speed code to look up.
 * @param buffer Space to write the string mnemonic.
 * @param bufsize Size of write buffer.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *dsicmd_lookup_usb_speed_r(enum DSI_USB_SPEED speed, char *buffer, int bufsize) {
	char *bufptr = 0;
	switch(speed) {
		case DSI_USB_SPEED_FULL:
			bufptr = "DSI_USB_SPEED_FULL";
			break;
		case DSI_USB_SPEED_HIGH:
			bufptr = "DSI_USB_SPEED_HIGH";
			break;
		case DSI_USB_SPEED_INVALID:
			bufptr = "DSI_USB_SPEED_INVALID";
			break;
	}
	if (bufptr != 0) {
		snprintf(buffer, bufsize, "%s", bufptr);
	} else {
		snprintf(buffer, bufsize, "DSI_USB_SPEED_UNKNOWN, 0x%02x", speed);
	}
	return buffer;
}

/**
 * Look up the human-readable mnemonic for a USB speed code, non-reentrant.
 *
 * @param speed USB speed code to look up.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *dsicmd_lookup_usb_speed(enum DSI_USB_SPEED speed) {
	/* not thread safe. */
	static char unknown[100];
	return dsicmd_lookup_usb_speed_r(speed, unknown, 100);
}


/**
 * Utility to return system clock time in milliseconds.
 *
 * @return integer value of system clock in milliseconds.
 */
static unsigned int dsi_get_sysclock_ms() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (unsigned int)(tv.tv_sec * 1000 + tv.tv_usec/1000);
}

/**
 * Pretty-print a DSI command buffer for logging purposes.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param iswrite Was this command a write to the DSI (true), or a read from
 *        the DSI (false).
 * @param prefix String prefix for logging message.
 * @param length Buffer length
 * @param buffer Buffer sent to/received from DSI.
 * @param result If the command had a return value, this is a pointer to that value.
 */
static void dsi_log_command_info(dsi_camera_t *dsi,
					  int iswrite, const char *prefix, unsigned int length,
					  char *buffer, unsigned int *result) {
	if (!dsi->log_commands)
		return;

	unsigned int now = dsi_get_sysclock_ms();
	int i, count;

	fprintf(stderr, "%-4s %02x %-4s [dt=%d]",
			prefix, length, "", now-dsi->last_time);

	if (strcmp(prefix, "r 86") != 0) {
		char scratch[50];
		for (i = count = 0; i < length; i++) {
			if (i == 8)
				break;
			if ((i % 8) == 0) {
				fprintf(stderr, "\n    %08x:", i);
			}
			fprintf(stderr, " %02x", (unsigned char) buffer[i]);
		}
		for (i = 8-(length%8); i != 8 && i > 0; i--) {
			fprintf(stderr, "   ");
		}

		fprintf(stderr, "    %s",
				iswrite ? dsicmd_lookup_command_name_r(buffer[2], scratch, 50) : "ACK");

		if (result) {
			fprintf(stderr, " %d", (unsigned int) *result);
			if (*result < 128 && isprint(*result)) {
				fprintf(stderr, " (%c)", *result);
			}
		}

	}
	fprintf(stderr, "\n");
	dsi->last_time = now;
	return;
}

/**
 * Decode byte 4 of the buffer as an 8-bit unsigned integer.
 *
 * @param buffer raw query result buffer from DSI.
 *
 * @return decoded value as an unsigned integer.
 */
static unsigned int dsi_get_byte_result(unsigned char *buffer) {
	return (unsigned int) buffer[3];
}

/**
 * Decode bytes 4-5 of the buffer as a 16-bit big-endian unsigned integer.
 *
 * @param buffer raw query result buffer from DSI.
 *
 * @return decoded value as an unsigned integer.
 */
static unsigned int dsi_get_short_result(unsigned char *buffer) {
	return (unsigned int) ((buffer[4] << 8) | buffer[3]);
}

/**
 * Decode bytes 4-7 of the buffer as a 32-bit big-endian unsigned integer.
 *
 * @param buffer raw query result buffer from DSI.
 *
 * @return decoded value as an unsigned integer.
 */
static unsigned int dsi_get_int_result(unsigned char *buffer) {
	unsigned int result;
	result =                 buffer[6];
	result = (result << 8) | buffer[5];
	result = (result << 8) | buffer[4];
	result = (result << 8) | buffer[3];
	return result;
}

/**
 * Internal helper for sending a command to the DSI device.  If the command is
 * one which requires no parameters, then the actual execution will be
 * delegated to command(DeviceCommand,int,int)
 *
 * @param cmd command to be executed.
 *
 * @return decoded command response.
 */
static int dsicmd_command_1(dsi_camera_t *dsi, dsi_command_t cmd) {
	if (dsi->is_simulation) {
		return 0;
	}

	// This is the one place where having class-based enums instead of
	// built-in enums is annoying: you can't use a switch statement here.
	switch (cmd) {
		case PING:
		case RESET:
		case ABORT:
		case TRIGGER:
		case PS_ON:
		case PS_OFF:
		case CCD_VDD_ON:
		case CCD_VDD_OFF:
		case TEST_PATTERN:
		case ERASE_EEPROM:
		case GET_VERSION:
		case GET_STATUS:
		case GET_TIMESTAMP:
		case GET_EXP_TIME:
		case GET_EXP_TIMER_COUNT:
		case GET_EEPROM_VIDPID:
		case GET_EEPROM_LENGTH:
		case GET_GAIN:
		case GET_EXP_MODE:
		case GET_VDD_MODE:
		case GET_FLUSH_MODE:
		case GET_CLEAN_MODE:
		case GET_READOUT_SPEED:
		case GET_READOUT_MODE:
		case GET_OFFSET:
		case GET_READOUT_DELAY:
		case GET_TEMP:
		case GET_ROW_COUNT_ODD:
		case GET_ROW_COUNT_EVEN:
			return dsicmd_command_3(dsi, cmd, 0, 3);

		default:
			return -1;
	}
}

/**
 * Internal helper for sending a command to the DSI device.  This determines
 * what the length of the actual command will be and then delgates to
 * command(DeviceCommand,int,int) or command(DeviceCommand).
 *
 * @param cmd command to be executed.
 * @param param command parameter, ignored for SOME commands.
 *
 * @return decoded command response.
 */
static int dsicmd_command_2(dsi_camera_t *dsi, dsi_command_t cmd, int param) {
	if (dsi->is_simulation) {
		return 0;
	}

	// This is the one place where having class-based enums instead of
	// built-in enums is annoying: you can't use a switch statement here.
	switch (cmd) {

		case GET_EEPROM_BYTE:
		case SET_GAIN:
		case SET_EXP_MODE:
		case SET_VDD_MODE:
		case SET_FLUSH_MODE:
		case SET_CLEAN_MODE:
		case SET_READOUT_SPEED:
		case SET_READOUT_MODE:
		case AD_READ:
		case GET_DEBUG_VALUE:
			return dsicmd_command_3(dsi, cmd, param, 4);

		case SET_EEPROM_BYTE:
		case SET_OFFSET:
		case SET_READOUT_DELAY:
		case SET_ROW_COUNT_ODD:
		case SET_ROW_COUNT_EVEN:
		case AD_WRITE:
			return dsicmd_command_3(dsi, cmd, param, 5);

		case SET_EXP_TIME:
		case SET_EEPROM_VIDPID:
			return dsicmd_command_3(dsi, cmd, param, 7);

		default:
			return dsicmd_command_1(dsi, cmd);
	}


}

/**
 * Internal helper for sending a command to the DSI device.  This determines
 * what the expected response length is and then delegates actually processing
 * to command(DeviceCommand,int,int,int).
 *
 * @param cmd command to be executed.
 * @param param
 * @param param_len
 *
 * @return decoded command response.
 */
static int dsicmd_command_3(dsi_camera_t *dsi, dsi_command_t cmd, int param, int param_len) {
	switch(cmd) {
		case PING:
		case RESET:
		case ABORT:
		case TRIGGER:
		case TEST_PATTERN:
		case SET_EEPROM_BYTE:
		case SET_GAIN:
		case SET_OFFSET:
		case SET_EXP_TIME:
		case SET_VDD_MODE:
		case SET_FLUSH_MODE:
		case SET_CLEAN_MODE:
		case SET_READOUT_SPEED:
		case SET_READOUT_MODE:
		case SET_READOUT_DELAY:
		case SET_ROW_COUNT_ODD:
		case SET_ROW_COUNT_EVEN:
		case PS_ON:
		case PS_OFF:
		case CCD_VDD_ON:
		case CCD_VDD_OFF:
		case AD_WRITE:
		case SET_EEPROM_VIDPID:
		case ERASE_EEPROM:
			return dsicmd_command_4(dsi, cmd, param, param_len, 3);

		case GET_EEPROM_LENGTH:
		case GET_EEPROM_BYTE:
		case GET_GAIN:
		case GET_EXP_MODE:
		case GET_VDD_MODE:
		case GET_FLUSH_MODE:
		case GET_CLEAN_MODE:
		case GET_READOUT_SPEED:
		case GET_READOUT_MODE:
			return dsicmd_command_4(dsi, cmd, param, param_len, 4);

		case GET_OFFSET:
		case GET_READOUT_DELAY:
		case SET_EXP_MODE:
		case GET_ROW_COUNT_ODD:
		case GET_ROW_COUNT_EVEN:
		case GET_TEMP:
		case AD_READ:
		case GET_DEBUG_VALUE:
			return dsicmd_command_4(dsi, cmd, param, param_len, 5);

		case GET_VERSION:
		case GET_STATUS:
		case GET_TIMESTAMP:
		case GET_EXP_TIME:
		case GET_EXP_TIMER_COUNT:
		case GET_EEPROM_VIDPID:
			return dsicmd_command_4(dsi, cmd, param, param_len, 7);

		default:
			return -1;
	}
}

/**
 * Internal helper for sending a command to the DSI device.  This formats the
 * command as a sequence of bytes and delegates to command(unsigned char *,int,int)
 *
 * @param cmd command to be executed.
 * @param val command parameter value.
 * @param val_bytes size of the parameter value field.
 * @param ret_bytes size of the return value field.
 *
 * @return decoded command response.
 */
static int dsicmd_command_4(dsi_camera_t *dsi, dsi_command_t cmd,
			  int val, int val_bytes, int ret_bytes) {
	unsigned char buffer[0x40];
	dsi->command_sequence_number++;

	buffer[0] = val_bytes;
	buffer[1] = dsi->command_sequence_number;
	buffer[2] = cmd;

	switch (val_bytes) {
		case 3:
			break;
		case 4:
			buffer[3] = val;
			break;
		case 5:
			buffer[3] = 0x0ff & val;
			buffer[4] = 0x0ff & (val >> 8);
			break;
		case 7:
			buffer[3] = 0x0ff & val;
			buffer[4] = 0x0ff & (val >>  8);
			buffer[5] = 0x0ff & (val >> 16);
			buffer[6] = 0x0ff & (val >> 24);
			break;
		default:
			return -1;
	}
	return dsicmd_usb_command(dsi, buffer, val_bytes, ret_bytes);
}

/**
 * Write a command buffer to the DSI device and return the decoded result value.
 *
 * @param ibuf raw command buffer, sent as input to the DSI.
 * @param ibuf_len length of the command buffer, in bytes.
 * @param obuf_len expected length of the response buffer, in bytes.
 *
 * @return decoded response as appropriate for the command.
 *
 * DSI commands return either 0, 1, 2, or 4 byte results.  The results are
 * nominally unsigned integers although in some cases (e.g. GET_VERSION),
 * the 4 bytes are actually 4 separate bytes.  However, all 4-byte responses
 * are treated as 32-bit unsigned integers and are decoded and returned that
 * way.  Similarly, 2-byte responses are treated as 16-bit unsigned integers
 * and are decoded and returned that way.
 */
static int dsicmd_usb_command(dsi_camera_t *dsi, unsigned char *ibuf, int ibuf_len, int obuf_len) {
	/* Yes, there is a conflict here.  Our decoded result is logically
	 * unsigned, but we need to be able to return negative values to indicate
	 * errors.  Worse, the GET_VERSION command seems to always return a buffer
	 * with the high bit set making it logically negative.  The command
	 * dsi_get_version will ignore the sign meaning we have one case where a
	 * failure can escape notice.  */
	unsigned int value = 0, result = 0;
	int retcode;

	/* The DSI endpoint for commands has is defined to only be able to return
	 * 64 bytes. */
	size_t obuf_size = 0x40;
	char obuf[0x40];

	switch (ibuf_len) {
		case 3:
			value = 0;
			break;
		case 4:
			value = dsi_get_byte_result((unsigned char *) ibuf);
			value = (unsigned int) ibuf[3];
			break;
		case 5:
			value = dsi_get_short_result((unsigned char *) ibuf);
			break;
		case 7:
			value = dsi_get_int_result((unsigned char *) ibuf);
			break;
		default:
			assert((ibuf_len >= 3) || (ibuf_len <= 7) || (ibuf_len != 6));
			break;
	}
	if (dsi->log_commands)
		dsi_log_command_info(dsi, 1, "w 1", (unsigned int) ibuf[0],
						 (char *) ibuf, (ibuf_len > 3 ? &value : 0));


	if (dsi->last_time == 0) {
		dsi->last_time = dsi_get_sysclock_ms();
	}

	int actual_length;
	retcode = (int)libusb_bulk_transfer(dsi->handle, 0x01, (unsigned char *) ibuf, ibuf[0], &actual_length, dsi->write_command_timeout);
	if (retcode < 0)
		return retcode;

	retcode = (int)libusb_bulk_transfer(dsi->handle, 0x81, (unsigned char *)obuf, (int)obuf_size, &actual_length, dsi->read_command_timeout);
	if (retcode < 0)
		return retcode;

	assert((unsigned char) obuf[1] == dsi->command_sequence_number);
	assert(obuf[2] == 6);

	switch (obuf_len) {
		case 3:
			result = 0;
			break;
		case 4:
			result = dsi_get_byte_result((unsigned char *) obuf);
			break;
		case 5:
			result = dsi_get_short_result((unsigned char *) obuf);
			break;
		case 7:
			result = dsi_get_int_result((unsigned char *) obuf);
			break;
		default:
			assert((obuf_len >= 3) && (obuf_len <= 7) && (obuf_len != 6));
			break;
	}

	if (dsi->log_commands)
		dsi_log_command_info(dsi, 0, "r 81", obuf[0], obuf, (obuf_len > 3 ? &result : 0));

	return result;
}

//static int dsicmd_wake_camera(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, PING);
//}

static int dsicmd_reset_camera(dsi_camera_t *dsi) {
	return dsicmd_command_1(dsi, RESET);
}

static int dsicmd_set_exposure_time(dsi_camera_t *dsi, int ticks) {
	if (ticks <= 0) ticks = 1;
	dsi->exposure_time = ticks;
	return dsicmd_command_2(dsi, SET_EXP_TIME, ticks);
}

static int dsicmd_get_exposure_time(dsi_camera_t *dsi) {
	return dsicmd_command_1(dsi, GET_EXP_TIME);
}

static int dsicmd_get_exposure_time_left(dsi_camera_t *dsi) {
	return dsicmd_command_1(dsi, GET_EXP_TIMER_COUNT);
}

static int dsicmd_start_exposure(dsi_camera_t *dsi) {
	dsi->imaging_state = DSI_IMAGE_EXPOSING;
	return dsicmd_command_1(dsi, TRIGGER);
}

static int dsicmd_abort_exposure(dsi_camera_t *dsi) {
	dsi->imaging_state = DSI_IMAGE_ABORTING;
	return dsicmd_command_1(dsi, ABORT);
}

static int dsicmd_set_gain(dsi_camera_t *dsi, int gain) {
	if (gain < 0 || gain > 63)
		return -1;
	return dsicmd_command_2(dsi, SET_GAIN, gain);
}

//static int dsicmd_get_gain(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, GET_GAIN);
//}

static int dsicmd_set_offset(dsi_camera_t *dsi, int offset) {
	/* FIXME: check offset for validity */
	return dsicmd_command_2(dsi, SET_OFFSET, offset);
}

//static int dsicmd_get_offset(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, GET_OFFSET);
//}

static int dsicmd_set_vdd_mode(dsi_camera_t *dsi, int mode) {
	/* FIXME: check mode for validity */
	return dsicmd_command_2(dsi, SET_VDD_MODE, mode);
}

//static int dsicmd_get_vdd_mode(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, GET_VDD_MODE);
//}

static int dsicmd_set_flush_mode(dsi_camera_t *dsi, int mode) {
	/* FIXME: check mode for validity */
	return dsicmd_command_2(dsi, SET_FLUSH_MODE, mode);
}

//static int dsicmd_get_flush_mode(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, GET_FLUSH_MODE);
//}

static int dsicmd_set_readout_mode(dsi_camera_t *dsi, int mode) {
	/* FIXME: check mode for validity */
	return dsicmd_command_2(dsi, SET_READOUT_MODE, mode);
}

static int dsicmd_get_readout_mode(dsi_camera_t *dsi) {
	return dsicmd_command_1(dsi, GET_READOUT_MODE);
}

static int dsicmd_set_readout_delay(dsi_camera_t *dsi, int delay) {
	/* FIXME: check mode for validity */
	return dsicmd_command_2(dsi, SET_READOUT_DELAY, delay);
}

//static int dsicmd_get_readout_delay(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, GET_READOUT_DELAY);
//}

static int dsicmd_set_readout_speed(dsi_camera_t *dsi, int speed) {
	/* FIXME: check speed for validity */
	return dsicmd_command_2(dsi, SET_READOUT_SPEED, speed);
}

//static int dsicmd_get_readout_speed(dsi_camera_t *dsi) {
//	return dsicmd_command_1(dsi, GET_READOUT_SPEED);
//}

static int dsicmd_get_temperature(dsi_camera_t *dsi) {
	if (!dsi->has_temperature_sensor) return NO_TEMP_SENSOR;
	return dsicmd_command_1(dsi, GET_TEMP);
}

//static int dsicmd_get_row_count_odd(dsi_camera_t *dsi) {
//	/* While we read the value from the camera, it lies except for the
//	   original DSI.  So if it has been set, we just use it as-is. */
//	if (dsi->read_height_odd <= 0)
//		dsi->read_height_odd = dsicmd_command_1(dsi, GET_ROW_COUNT_ODD);
//	return dsi->read_height_odd;
//}

//static int dsicmd_get_row_count_even(dsi_camera_t *dsi) {
//	/* While we read the value from the camera, it lies except for the
//	   original DSI.  So if it has been set, we just use it as-is. */
//	if (dsi->read_height_even <= 0)
//		dsi->read_height_even = dsicmd_command_1(dsi, GET_ROW_COUNT_EVEN);
//	return dsi->read_height_even;
//}


static unsigned char dsicmd_get_eeprom_byte(dsi_camera_t *dsi, int offset) {
	if (dsi->eeprom_length < 0) {
		dsi->eeprom_length = dsicmd_command_1(dsi, GET_EEPROM_LENGTH);
	}
	if ((offset < 0) || (offset > dsi->eeprom_length))
		return 0xff;
	return dsicmd_command_2(dsi, GET_EEPROM_BYTE, offset);
}

static unsigned char dsicmd_set_eeprom_byte(dsi_camera_t *dsi, char byte, int offset) {
	if (dsi->eeprom_length < 0) {
		dsi->eeprom_length = dsicmd_command_1(dsi, GET_EEPROM_LENGTH);
	}
	if ((offset < 0) || (offset > dsi->eeprom_length))
		return 0xff;
	return dsicmd_command_2(dsi, SET_EEPROM_BYTE, offset | (byte << 8));
}

static int dsicmd_get_eeprom_data(dsi_camera_t *dsi, char *buffer, int start, int length) {
	int i;
	for (i = 0; i < length; i++) {
		buffer[i] = dsicmd_get_eeprom_byte(dsi, start+i);
	}
	return length;
}

static int dsicmd_set_eeprom_data(dsi_camera_t *dsi, char *buffer, int start, int length) {
	int i;
	for (i = 0; i < length; i++) {
		dsicmd_set_eeprom_byte(dsi, buffer[i], start+i);
	}
	return length;
}


static void dsicmd_get_eeprom_string(dsi_camera_t *dsi, unsigned char *buffer, int start, int length) {
	int i;
	dsicmd_get_eeprom_data(dsi, (char *)buffer, start, length);
	if ((buffer[0] == 0xff) || (buffer[1] == 0xff) || (buffer[2] == 0xff)) {
		strncpy((char *)buffer, "None", length);
	} else {
		for (i = 0; i < length-1; i++) {
			if (!isprint(buffer[i+1])) {
				/* some camera use GS as delimiter stop at any nonprinable character */
				buffer[i] = '\0';
				break;
			}
			buffer[i] = buffer[i+1];
		}
	}
}


/**
 * Write the provided string to a region in the EEPROM.  WARNING: I think it
 * may be possible to brick your camera with this one which is why I've made
 * it static.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param buffer
 * @param start
 * @param length
 *
 * @return
 */
static void dsicmd_set_eeprom_string(dsi_camera_t *dsi, char *buffer, int start, int length) {
	int i, n;
	char *scratch;
	/* The buffer is assumed to be a normal null-terminated C-string and has
	   to be encoded for storage in the EEPROM.  EEPROM strings have their
	   length as the first byte and are terminated (padded?) with 0xff. */
	scratch = malloc(length * sizeof(char));
	memset(scratch, 0xff, length);
	n = (int)strlen(buffer);
	if (n > length-2) {
		n = length - 2;
	}
	scratch[0] = n;
	for (i = 0; i < n; i++) {
		scratch[i+1] = buffer[i];
	}
	dsicmd_set_eeprom_data(dsi, scratch, start, length);
	free(scratch);
}

int dsicmd_get_version(dsi_camera_t *dsi) {
	if (dsi->version.value == -1) {
		unsigned int value = dsicmd_command_1(dsi, GET_VERSION);
		dsi->version.c.family   = 0xff & (value);
		dsi->version.c.model    = 0xff & (value >> 0x08);
		dsi->version.c.version  = 0xff & (value >> 0x10);
		dsi->version.c.revision = 0xff & (value >> 0x18);
		assert(dsi->version.c.family  == 10);
		assert(dsi->version.c.model   == 1);
		assert(dsi->version.c.version == 1);
	}
	return dsi->version.value;
}

static void dsicmd_load_status(dsi_camera_t *dsi) {
	if ((dsi->usb_speed == DSI_USB_SPEED_INVALID) || (dsi->fw_debug == DSI_FW_DEBUG_INVALID)) {
		int result = dsicmd_command_1(dsi, GET_STATUS);
		int usb_speed = (result & 0x0ff);
		int fw_debug  = ((result << 8) & 0x0ff);

		assert((usb_speed == DSI_USB_SPEED_FULL) || (usb_speed == DSI_USB_SPEED_HIGH));
		dsi->usb_speed = usb_speed;

		/* XXX: I suppose there is logically a DSI_FW_DEBUG_ON, but I don't know how
		 * to turn it on, nor what I would do if I turned it on. */
		assert((fw_debug == DSI_FW_DEBUG_OFF));
		dsi->fw_debug = fw_debug;
	}
}

//static int dsicmd_get_usb_speed(dsi_camera_t *dsi) {
//	dsicmd_load_status(dsi);
//	return dsi->usb_speed;
//}

int dsicmd_get_firmware_debug(dsi_camera_t *dsi) {
	dsicmd_load_status(dsi);
	return dsi->fw_debug;
}

/**
 * Initialize some internal parameters, wake-up the camera, and then query it
 * for some descriptive/identifying information.
 *
 * The DSI cameras all present the same USB vendor and device identifiers both
 * before and after renumeration, so it is not possible to determine what
 * camera model is attached to the bus until after you have connected and
 * (at least partially) intialized the device.
 *
 * The sequence here is mostly a combination of the sequence from the USB
 * traces of what Envisage and MaximDL 4.x do when connecting to the camera.
 * The only commands asking the camera to do something are the wakeup/reset
 * commands; everything else is querying the camera to find out what it is.
 *
 * The original DSI Pro identifies itself (dsi_get_camera_name) as "DSI1".
 * So it was somewhat surprising to find that the DSI Pro II identifies itself
 * as "Guider".  The most reliable way of identifying the camera seems to be
 * via the chip name.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 *
 * @return
 */
static dsi_camera_t *dsicmd_init_dsi(dsi_camera_t *dsi) {
	dsi->command_sequence_number = 0;
	dsi->eeprom_length    = -1;
	dsi->log_commands     = verbose_init;
	dsi->test_pattern     = 0;
	dsi->exposure_time    = 10;

	dsi->version.value = -1;
	dsi->fw_debug  = DSI_FW_DEBUG_INVALID;
	dsi->usb_speed = DSI_USB_SPEED_INVALID;

	dsi->little_endian_data = 1;
	dsi->bayer_pattern[0] = '\0';
	dsi->bin_mode = BIN1X1;

	if (!dsi->is_simulation) {
		dsicmd_command_1(dsi, PING);
		dsicmd_command_1(dsi, RESET);

		dsicmd_get_version(dsi);
		dsicmd_load_status(dsi);

		dsicmd_command_1(dsi, GET_READOUT_MODE);
	}
	dsi_get_chip_name(dsi);
	dsi_get_camera_name(dsi);
	// dsi_get_serial_number(dsi);

	// dsi->read_height_even = dsicmd_get_row_count_even(dsi);
	// dsi->read_height_odd  = dsicmd_get_row_count_odd(dsi);

	/* You would think these could be found by asking the camera, but I can't
	   find an example of it happening. */

	/* IMPORTANT: Compare only 7 or 8 characters as the rest may vary */
	if (strncmp(dsi->chip_name, "ICX254AL", 8) == 0) {
		/* DSI Pro I.
		 * Sony reports the following information:
		 * Effective pixels: 510 x 492
		 * Total pixels: 537 x 505
		 * Optical black: Horizontal, front 2, rear 25
		 *                Vertical, front 12, rear 1
		 * Dummy bits: horizontal 16
		 *             vertical 1 (even rows only)
		 */

		/* Okay, there is some interesting inconsistencies here.  MaximDL
		 * takes my DSI Pro I and spits out an image that is 508x489.
		 * Envisage spits out an image that is 780x586.  If I ask the camera
		 * firmware how many odd/even rows there are, they match the Sony
		 * specs.  If I ask MaximDL to square the pixels, I don't grow any new
		 * height, and the width doesn't scale out to 780.  I'm going with
		 * MaximDL even though it doesn't quite match the Sony specs (why not?
		 * What are the dummy bits?) */

		dsi->read_width       = 537;
		dsi->read_height_even = 253;
		dsi->read_height_odd  = 252;

		dsi->image_width      = 508;
		dsi->image_height     = 488;
		dsi->image_offset_x   = 23;
		dsi->image_offset_y   = 13;

		dsi->is_binnable      = 0;
		dsi->is_interlaced    = 1;
		dsi->has_temperature_sensor = 0;

		dsi->pixel_size_x     = 9.6;
		dsi->pixel_size_y     = 7.5;
		dsi->bayer_pattern[0] = '\0';

	} else if (strncmp(dsi->chip_name, "ICX404AK", 8) == 0) {
		/* DSI Color I.
		 * Sony reports the following information:
		 * Effective pixels: 510 x 492
		 * Total pixels:     537 x 505
		 * Optical black: Horizontal, front  2, rear 25
		 *                Vertical,   front 12, rear  1
		 * Dummy bits: horizontal 16
		 *             vertical 1 (even rows only)
		 */
		dsi->read_width       = 537;
		dsi->read_height_even = 253;
		dsi->read_height_odd  = 252;

		dsi->image_width      = 508;
		dsi->image_height     = 488;
		dsi->image_offset_x   = 23;
		dsi->image_offset_y   = 17;

		dsi->is_binnable      = 0;
		dsi->is_interlaced    = 1;
		dsi->has_temperature_sensor = 0;

		dsi->pixel_size_x     = 9.6;
		dsi->pixel_size_y     = 7.5;
		strncpy(dsi->bayer_pattern,"MCGY", DSI_BAYER_LEN);

	} else if (strncmp(dsi->chip_name, "ICX429A", 7) == 0) {
		/* DSI Pro/Color II.
		 * Sony reports the following information:
		 * Effective pixels: 752 x 582
		 * Total pixels:     795 x 596
		 * Optical black: Horizontal, front  3, rear 40
		 *                Vertical,   front 12, rear  2
		 * Dummy bits: horizontal 22
		 *             vertical 1 (even rows only)
		 */

		dsi->read_width       = 795;
		dsi->read_height_even = 299;
		dsi->read_height_odd  = 298;

		dsi->image_width      = 748;
		dsi->image_height     = 577;
		dsi->image_offset_x   = 30;     /* In bytes, not pixels */
		dsi->image_offset_y   = 13;     /* In rows. */

		dsi->pixel_size_x     = 8.6;
		dsi->pixel_size_y     = 8.3;
		dsi->has_temperature_sensor = 1;
		dsi->is_binnable      = 0;
		dsi->is_interlaced    = 1;

		if (strncmp(dsi->chip_name, "ICX429AK", 8) == 0)
			strncpy(dsi->bayer_pattern,"MCGY", DSI_BAYER_LEN);
		else /* ICX429ALL */
			dsi->bayer_pattern[0] = '\0';

		/* FIXME: Don't know if these are B&W specific or not. */
		dsicmd_command_2(dsi, SET_ROW_COUNT_EVEN, dsi->read_height_even);
		dsicmd_command_2(dsi, SET_ROW_COUNT_ODD,  dsi->read_height_odd);
		dsicmd_command_2(dsi, AD_WRITE,  88);
		dsicmd_command_2(dsi, AD_WRITE, 704);

	} else if (strncmp(dsi->chip_name, "ICX285A", 7) == 0) {
		/* DSI Pro/Color III.
		 * Sony reports the following information:
		 * Effective pixels: 1360 x 1024
		 * Total pixels:     1434 x 1050
		 */

		dsi->is_binnable      = 1;
		dsi->is_interlaced    = 0;
		dsi->read_width       = 1434;
		dsi->read_height_even = 0;
		dsi->read_height_odd  = 1050;

		dsi->image_width      = 1360;
		dsi->image_height     = 1024;
		dsi->image_offset_x   = 30;     /* In bytes, not pixels */
		dsi->image_offset_y   = 13;     /* In rows. */

		dsi->pixel_size_x     = 6.45;
		dsi->pixel_size_y     = 6.45;
		dsi->has_temperature_sensor = 1;

		if (strncmp(dsi->chip_name, "ICX285AQ", 8) == 0)
			strncpy(dsi->bayer_pattern,"RGGB", DSI_BAYER_LEN);
		else /* ICX285AL */
			dsi->bayer_pattern[0] = '\0';

		dsicmd_command_2(dsi, SET_ROW_COUNT_EVEN, dsi->read_height_even);
		dsicmd_command_2(dsi, SET_ROW_COUNT_ODD,  dsi->read_height_odd);
		dsicmd_command_2(dsi, AD_WRITE, 216);
		dsicmd_command_2(dsi, AD_WRITE, 704);

	} else {
		/* Die, camera not supported. */
		fprintf(stderr, "Camera %s not supported", dsi->chip_name);
		abort();
	}

	dsi->read_bpp         = 2;
	dsi->read_height      = dsi->read_height_even + dsi->read_height_odd;
	dsi->read_width       = ((dsi->read_bpp * dsi->read_width / 512) + 1) * 256;

	dsi->read_size_odd    = dsi->read_bpp * dsi->read_width * dsi->read_height_odd;
	dsi->read_size_even   = dsi->read_bpp * dsi->read_width * dsi->read_height_even;

	dsi->read_buffer_odd  = malloc(dsi->read_size_odd);
	dsi->read_buffer_even = malloc(dsi->read_size_even);

	dsi->read_command_timeout  = 1000;    /* milliseconds */
	dsi->write_command_timeout = 1000;    /* milliseconds */
	dsi->read_image_timeout   =  5000;    /* milliseconds */

	dsi->amp_gain_pct   = 100;
	dsi->amp_offset_pct =  50;

	dsi->imaging_state = DSI_IMAGE_IDLE;
	dsicmd_command_1(dsi, RESET);
	return dsi;
}

/**
 * Do the libusb part of initializing the DSI device.
 *
 * This initialization is based on USB trace logs, plus some trial-and-error.
 * The USB trace logs clearly show the first three usb_get_decriptor commands
 * and the usb_set_configuration command.  The usb_claim_interface is part of
 * the libusb mechanism for locking access to the device, and is not seen in
 * the traces (where were in Windows), but is necessary.
 *
 * The next three usb_clear_halt commands were added after some
 * trial-and-error trying to eliminate a problem with the reference
 * implementation of the DSI control program hanging after a successful
 * connect, image, disconnect sequence.  I don't know why they are necessary,
 * and, in fact, not all three seem to be necessary, but if none of the
 * endpoints are cleared, the device WILL hang.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 *
 * @return
 */
static void dsicmd_init_usb_device(dsi_camera_t *dsi) {
	const size_t size = 0x800;
	unsigned char data[size];

	/* This is monkey code.  SniffUSB shows that the Meade driver is doing
	 * this, but I can think of no reason why.  It does the equivalent of the
	 * following sequence
	 *
	 *    - usb_get_descriptor 1
	 *    - usb_get_descriptor 1
	 *    - usb_get_descriptor 2
	 *    - usb_set_configuration 1
	 *    - get the serial number
	 *    - get the chip name
	 *    - ping the device
	 *    - reset the device
	 *    - load the firmware information
	 *    - load the bus speed status
	 *
	 * libusb says I'm supposed to claim the interface before doing anything to
	 * the device.  I'm not clear what "anything" includes, but I can't do it
	 * before the usb_set_configuration command or else I get EBUSY.  So I
	 * stick it in the middle of the above sequence at what appears to be the
	 * first workable point.
	 */

	/* According the the libusb 1.0 documentation (but remember this is libusb
	 * 0.1 code), the "safe" way to set the configuration on a device is to
	 *
	 *  1. Query the configuration.
	 *  2. If it is not the desired on, set the desired configuration.
	 *  3. Claim the interface.
	 *  4. Check the configuration to make sure it is what you selected.  If
	 *     not, it means someone else got it.
	 *
	 * However, that does not seem to be what the USB trace is showing from
	 * the Windows driver.  It shows the sequence below (sans the claim
	 * interface call, but I'm not sure that actually sends data over the
	 * wire).
	 *
	 */
	assert(libusb_get_descriptor(dsi->handle, 0x01, 0x00, data, size) >= 0);
	assert(libusb_get_descriptor(dsi->handle, 0x01, 0x00, data, size) >= 0);
	assert(libusb_get_descriptor(dsi->handle, 0x02, 0x00, data, size) >= 0);
	assert(libusb_set_configuration(dsi->handle, 1) >= 0);
	assert(libusb_claim_interface(dsi->handle, 0) >= 0);

	/* This seems to solve the connect issue after reconnect without close ot close after short exposure */
	assert(libusb_reset_device(dsi->handle) >= 0);

	/* This is included out of desperation, but it works :-|
	 *
	 * After running once, an attempt to run a second time appears, for some
	 * unknown reason, to leave us unable to read from EP 0x81.  At the very
	 * least, we need to clear this EP.  However, believing in the power of
	 * magic, we clear them all.
	 */
	assert(libusb_clear_halt(dsi->handle, 0x01) >= 0);
	assert(libusb_clear_halt(dsi->handle, 0x81) >= 0);
	assert(libusb_clear_halt(dsi->handle, 0x86) >= 0);

	assert(libusb_clear_halt(dsi->handle, 0x02) >= 0);
	assert(libusb_clear_halt(dsi->handle, 0x04) >= 0);
	assert(libusb_clear_halt(dsi->handle, 0x88) >= 0);

}


/**
 * Decode the internal image buffer from an already read image.
 */
static unsigned char *dsicmd_decode_image(dsi_camera_t *dsi, unsigned char *buffer) {

	int xpix, ypix, outpos;
	int is_odd_row, row_start;
	int read_width, image_width, image_height, image_offset_x, image_offset_y;

	/* FIXME: This method should really only be called if the camera is an
	   post-imaging state. */

	if (buffer == NULL) return NULL;

	if (dsi->bin_mode == BIN2X2) {
		read_width       = dsi->read_width / 2;
		image_width      = dsi->image_width / 2;
		image_height     = dsi->image_height / 2;
		image_offset_x   = dsi->image_offset_x / 2;
		image_offset_y   = dsi->image_offset_y / 2;
	} else {
		read_width       = dsi->read_width;
		image_width      = dsi->image_width;
		image_height     = dsi->image_height;
		image_offset_x   = dsi->image_offset_x;
		image_offset_y   = dsi->image_offset_y;
    }

	outpos = 0;
	if (dsi->is_interlaced) {
		for (ypix = 0; ypix < image_height; ypix++) {
			int ixypos;
			/* The odd-even interlacing means that we advance the row start offset
			   every other pass through the loop.  It is the same offset on each
			   of those two passes, but we read from a different buffer. */
			is_odd_row = (ypix + image_offset_y) % 2;
			row_start  = read_width * ((ypix + image_offset_y) / 2);
			ixypos = 2 * (row_start + image_offset_x);
			/*
			  fprintf(stderr, "starting image row %d, outpos=%d, is_odd_row=%d, row_start=%d, ixypos=%d\n",
			  ypix, outpos, is_odd_row, row_start, ixypos);
			*/
			if (dsi->little_endian_data) {
				for (xpix = 0; xpix < image_width; xpix++) {
					if (is_odd_row) { /* invert bytes as camera givers big endian */
						buffer[outpos++] = dsi->read_buffer_odd[ixypos+1];
						buffer[outpos++] = dsi->read_buffer_odd[ixypos];
					} else {
						buffer[outpos++] = dsi->read_buffer_even[ixypos+1];
						buffer[outpos++] = dsi->read_buffer_even[ixypos];
					}
					ixypos += 2;
				}
			} else { /* just copy data as camera givers big endian */
				if (is_odd_row) {
					memcpy(buffer + outpos, dsi->read_buffer_odd + ixypos, image_width * dsi->read_bpp);
				} else {
					memcpy(buffer + outpos, dsi->read_buffer_even + ixypos, image_width * dsi->read_bpp);
				}
				outpos += image_width * dsi->read_bpp;
			}
		}
	} else { /* Non interlaced -> DSI III*/
		if (dsi->little_endian_data) { /* invert bytes as camera givers big endian */
			for (ypix = 0; ypix < image_height; ypix++) {
				int ixypos;
				row_start  = read_width * (ypix + image_offset_y);
				ixypos = 2 * (row_start + image_offset_x);
				/*
				  fprintf(stderr, "starting image row %d, outpos=%d, is_odd_row=%d, row_start=%d, ixypos=%d\n",
				  ypix, outpos, is_odd_row, row_start, ixypos);
				*/
				for (xpix = 0; xpix < image_width; xpix++) {
					buffer[outpos++] = dsi->read_buffer_odd[ixypos+1];
					buffer[outpos++] = dsi->read_buffer_odd[ixypos];
					ixypos += 2;
				}
			}
		} else {  /* just copy data as camera givers big endian */
			for (ypix = 0; ypix < image_height; ypix++) {
				int ixypos;
				row_start  = read_width * (ypix + image_offset_y);
				ixypos = 2 * (row_start + image_offset_x);
				/*
				  fprintf(stderr, "starting image row %d, outpos=%d, is_odd_row=%d, row_start=%d, ixypos=%d\n",
				  ypix, outpos, is_odd_row, row_start, ixypos);
				 */

				for (xpix = 0; xpix < image_width; xpix++) {
					buffer[outpos++] = dsi->read_buffer_odd[ixypos++];
					buffer[outpos++] = dsi->read_buffer_odd[ixypos++];
				}
			}
		}
	}
	return buffer;
}

/* User Callable functions */

void dsi_set_image_little_endian(dsi_camera_t *dsi, int little_endian) {
	if (little_endian) {
		dsi->little_endian_data = 1;
	} else {
		dsi->little_endian_data = 0;
	}
}

int dsi_set_amp_gain(dsi_camera_t *dsi, int gain) {
	if (gain > 100)
		dsi->amp_gain_pct = 100;
	else if (gain < 0)
		dsi->amp_gain_pct = 0;
	else
		dsi->amp_gain_pct = gain;
	return dsi->amp_gain_pct;
}

int dsi_get_amp_gain(dsi_camera_t *dsi) {
	return dsi->amp_gain_pct;
}

int dsi_set_amp_offset(dsi_camera_t *dsi, int offset) {
	if (offset > 100)
		dsi->amp_offset_pct = 100;
	else if (offset < 0)
		dsi->amp_offset_pct = 0;
	else
		dsi->amp_offset_pct = offset;
	return dsi->amp_offset_pct;
}

int dsi_get_amp_offset(dsi_camera_t *dsi) {
	return dsi->amp_offset_pct;
}

int dsi_get_frame_width(dsi_camera_t *dsi) {
	return dsi->image_width;
}

int dsi_get_frame_height(dsi_camera_t *dsi) {
	return dsi->image_height;
}

int dsi_get_image_width(dsi_camera_t *dsi) {
	return dsi->image_width / dsi->bin_mode;
}

int dsi_get_image_height(dsi_camera_t *dsi) {
	return dsi->image_height / dsi->bin_mode;
}

double dsi_get_pixel_width(dsi_camera_t *dsi) {
	return dsi->pixel_size_x;
}

double dsi_get_pixel_height(dsi_camera_t *dsi) {
	return dsi->pixel_size_y;
}

double dsi_get_temperature(dsi_camera_t *dsi) {
	int raw_temp = dsicmd_get_temperature(dsi);
	if (raw_temp == NO_TEMP_SENSOR) return (double)NO_TEMP_SENSOR;
	return floor((double) raw_temp / 25.6) / 10.0;
}

const char *dsi_get_chip_name(dsi_camera_t *dsi) {
	if (dsi->chip_name[0] == 0) {
		memset(dsi->chip_name, 0, 21);
		dsicmd_get_eeprom_string(dsi, (unsigned char *)dsi->chip_name, 8, 20);
	}
	return dsi->chip_name;
}

const char *dsi_get_model_name(dsi_camera_t *dsi) {
	if (dsi->model_name[0] == 0) {
		memset(dsi->chip_name, 0, DSI_NAME_LEN);
		dsi_get_chip_name(dsi);
		/* IMPORTANT: compare only 8 characters as the 9th may vary */
		if (!strncmp(dsi->chip_name, "ICX254AL", 8)) {
			strncpy(dsi->model_name, "DSI Pro", DSI_NAME_LEN);
		} else if (!strncmp(dsi->chip_name, "ICX429ALL", 8)) {
			strncpy(dsi->model_name, "DSI Pro II", DSI_NAME_LEN);
		} else if (!strncmp(dsi->chip_name, "ICX429AKL", 8)) {
			strncpy(dsi->model_name, "DSI Color II", DSI_NAME_LEN);
		} else if (!strncmp(dsi->chip_name, "ICX404AK", 8)) {
			strncpy(dsi->model_name, "DSI Color", DSI_NAME_LEN);
		} else if (!strncmp(dsi->chip_name, "ICX285AL", 8)) {
			strncpy(dsi->model_name, "DSI Pro III", DSI_NAME_LEN);
		} else if (!strncmp(dsi->chip_name, "ICX285AQ", 8)) {
			strncpy(dsi->model_name, "DSI Color III", DSI_NAME_LEN);
		} else {
			strncpy(dsi->model_name, "DSI Unknown", DSI_NAME_LEN);
		}
	}
	return dsi->model_name;
}

const char *dsi_get_bayer_pattern(dsi_camera_t *dsi) {
	return dsi->bayer_pattern;
}

const char *dsi_get_camera_name(dsi_camera_t *dsi) {
	if (dsi->camera_name[0] == 0) {
		memset(dsi->camera_name, 0, DSI_NAME_LEN);
		dsicmd_get_eeprom_string(dsi, (unsigned char*)dsi->camera_name, 0x1c, 0x20);
	}
	return dsi->camera_name;
}

int dsi_get_bytespp(dsi_camera_t *dsi) {
	return dsi->read_bpp;
}

/**
 * Store a name for the DSI camera in its EEPROM for future reference.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param name
 *
 * @return
 */
const char *dsi_set_camera_name(dsi_camera_t *dsi, const char *name) {
	if (dsi->camera_name[0] == 0) {
		memset(dsi->camera_name, 0, DSI_NAME_LEN);
	}
	strncpy(dsi->camera_name, name, DSI_NAME_LEN);
	dsicmd_set_eeprom_string(dsi, dsi->camera_name, 0x1c, 0x20);
	return dsi->camera_name;
}

const char *dsi_get_serial_number(dsi_camera_t *dsi) {
	if (dsi->serial_number[0] == 0) {
		int i;
		char temp[10];
		dsicmd_get_eeprom_data(dsi, temp, 0, 8);
		for (i = 0; i < 8; i++) {
			sprintf(dsi->serial_number+2*i, "%02x", temp[i]);
		}
	}
	return dsi->serial_number;
}

int dsicmd_set_binning(dsi_camera_t *dsi, enum DSI_BIN_MODE bin) {
	unsigned int read_height_odd = dsi->read_height_odd / bin;
	int res = 0;
	if (dsi->is_binnable) {
		res = dsicmd_command_1(dsi, GET_EXP_MODE);
		res = dsicmd_command_2(dsi, SET_EXP_MODE, bin);
		res = dsicmd_command_2(dsi, SET_ROW_COUNT_ODD, read_height_odd);
	}
	return res;
}

enum DSI_BIN_MODE dsi_get_max_binning(dsi_camera_t *dsi) {
	if (dsi->is_binnable) return BIN2X2;
	else return BIN1X1;
}

int dsi_set_binning(dsi_camera_t *dsi, enum DSI_BIN_MODE bin) {
	if (dsi->is_binnable) {
		dsi->bin_mode = bin;
		return 0;
	}

	dsi->bin_mode = BIN1X1;
	return -1;
}

enum DSI_BIN_MODE dsi_get_binning(dsi_camera_t *dsi) {
	return dsi->bin_mode;
}

int dsi_get_identifier(libusb_device *device, char *identifier) {
	uint8_t data[10];
	char buf[10];
	int i;

	data[0]=libusb_get_bus_number(device);
	int n = libusb_get_port_numbers(device, &data[1], 9);
	if (n != LIBUSB_ERROR_OVERFLOW) {
		sprintf(identifier,"%X", data[0]);
		for (i = 1; i <= n; i++) {
			sprintf(buf, "%X", data[i]);
			strcat(identifier, ".");
			strcat(identifier, buf);
		}
	} else {
		identifier[0] = '\0';
		return n;
	}
	return LIBUSB_SUCCESS;
}

static int dsicmd_write_firmware(libusb_device_handle *handle) {
	unsigned char *pnt = FIRMWARE;
	int address;
	int length;
	unsigned char cpuStop = 0x01;
	unsigned char cpuStart = 0x00;
	int rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0xA0, 0xE600, 0, &cpuStop, 1, 3000);
	while (rc >= 0 && *pnt) {
		length = (*pnt++) & 0xFF;
		address = (*pnt++) & 0xFF;
		address = (address  << 8)| ((*pnt++) & 0xFF);
		rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0xA0, address, 0, pnt, length, 3000);
		pnt += length;
	}
	if (rc >= 0) {
		rc = (libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0xA0, 0xE600, 0, &cpuStart, 1, 3000));
	}
	return rc;
}

bool dsi_load_firmware(void) {
	struct libusb_device **list = NULL;
	struct libusb_device_descriptor desc;
	int i;

	// check for uninitialized cameras
	int cnt = (int)libusb_get_device_list(NULL, &list);
	for (i = 0; i < cnt; ++i) {
		if (!libusb_get_device_descriptor(list[i], &desc)) {
			if ((desc.idVendor == 0x156c) && (desc.idProduct == 0x0100)) {
				libusb_device_handle *handle;
				int rc = libusb_open(list[i], &handle);
				if (rc >= 0) {
					if (libusb_kernel_driver_active(handle, 0) == 1) {
						rc = libusb_detach_kernel_driver(handle, 0);
					}
					if (rc >= 0) {
						rc = libusb_claim_interface(handle, 0);
					}
					rc = dsicmd_write_firmware(handle);
					rc = libusb_release_interface(handle, 0);
					libusb_close(handle);
					libusb_free_device_list(list, 0);
					return true;
				}
			}
		}
	}
	libusb_free_device_list(list, 0);
	return false;
}


int dsi_scan_usb(dsi_device_list devices) {
	struct libusb_device **list = NULL;
	struct libusb_device_descriptor desc;
	char dev_id[20];
	int index = 0;
	int i;

	// check for initialized cameras
	int cnt = (int)libusb_get_device_list(NULL, &list);

	for (i = 0; i < cnt; ++i) {
		if (!libusb_get_device_descriptor(list[i], &desc)) {
			if ((desc.idVendor == 0x156c) && (desc.idProduct == 0x0101)) {
				dsi_get_identifier(list[i], dev_id);
				strncpy(devices[index], dev_id, DSI_ID_LEN);
				index++;
				if (index >= DSI_MAX_DEVICES) break;
			}
		}
	}
	libusb_free_device_list(list, 0);
	return index;
}


/**
 * Open a DSI camera using the named device, or the first DSI device found if
 * the name is null.
 *
 * @param identifier as returned by dsi_scan()
 *
 * @return a dsi_camera_t handle which should be used for subsequent calls to
 * control the camera.
 */
dsi_camera_t *dsi_open_camera(const char *identifier) {
	struct libusb_device *dev = NULL;
	struct libusb_device_handle *handle = NULL;
	struct libusb_device **list = NULL;
	struct libusb_device_descriptor desc;
	dsi_camera_t *dsi = NULL;
	char dev_id[20];
	int i;

	int cnt = (int)libusb_get_device_list(NULL, &list);

	for (i = 0; i < cnt; ++i) {
		if (!libusb_get_device_descriptor(list[i], &desc)) {
			if ((desc.idVendor == 0x156c) && (desc.idProduct = 0x0101)) {
				dev = list[i];
				dsi_get_identifier(dev, dev_id);
				if (!strncmp(dev_id, identifier, DSI_ID_LEN)) {
					if (libusb_open(dev, &handle)) {
						dev = NULL;
					}
					break;
				}
			}
		}
	}
	libusb_free_device_list(list, 0);

	if (handle == NULL) return NULL;

	dsi = calloc(1, sizeof(dsi_camera_t));
	assert(dsi != 0);

	dsi->device = dev;
	dsi->handle = handle;
	dsi->is_simulation = 0;

	dsicmd_init_usb_device(dsi);
	dsicmd_init_dsi(dsi);

	dsi_start_exposure(dsi, 0.0001);
	dsi_read_image(dsi, 0, 0);
//	dsi_start_exposure(dsi, 0.0001);
//	dsi_read_image(dsi, 0, 0);
//	dsi_start_exposure(dsi, 0.1);
//	dsi_read_image(dsi, 0, 0);

	return dsi;
}

void dsi_close_camera(dsi_camera_t *dsi) {
	if (dsi == NULL) return;
	/* Next is guesswork but seems to work! */
	if(dsi->is_interlaced) {
		dsicmd_command_1(dsi, RESET);
	}
	dsicmd_command_1(dsi, PING);
	dsicmd_command_1(dsi, RESET);

	libusb_release_interface(dsi->handle, 0);
	libusb_close(dsi->handle);
	if (dsi->read_buffer_odd) free(dsi->read_buffer_odd);
	if (dsi->read_buffer_even) free(dsi->read_buffer_even);
	free(dsi);
}


/**
 * Set the verbose logging state for the library during camera
 * open/intialization.  This works the same as dsi_set_verbose, but you can't
 * call dsi_set_verbose until after you have opened a connection to the
 * camera, so this fills that gap.
 *
 * @param on turn on logging if logically true.
 */
void libdsi_set_verbose_init(int on) {
	verbose_init = on;
}

/**
 * Return verbose logging state for camera intialization.
 *
 * @return 0 if logging is off, non-zero for logging on.
 */
int libdsi_get_verbose_init() {
	return verbose_init;
}

/**
 * Turn on or off verbose logging state for low-level camera commands.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param on turn on logging if logically true.
 */
void dsi_set_verbose(dsi_camera_t *dsi, int on) {
	dsi->log_commands = on;
}

/**
 * Get verbose logging state for low-level camera commands.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 *
 * @return 0 if logging is off, non-zero for logging on.
 */
int dsi_get_verbose(dsi_camera_t *dsi) {
	return dsi->log_commands;
}

int dsi_start_exposure(dsi_camera_t *dsi, double exptime) {
	int gain, offset;
	int exposure_ticks = 10000 * exptime;

	gain = (int)(63 * dsi->amp_gain_pct / 100.0);

	/* FIXME: What is the mapping?
	 *     20% -> 409 -> 0x199
	 *     50% ->   0
	 *     80% -> 153 -> 0x099
	 * So, this looks like a 8-bit value with a sign bit.  Then 80% is
	 * (80-50)/50*255 = 153, and 20% is the same thing, but with the high bit
	 * set.
	 */

	if (dsi->amp_offset_pct < 50) {
		offset = 50 - dsi->amp_offset_pct;
		offset = (int)(255 * offset / 50.0);
		offset |= 0x100;
	} else {
		offset = dsi->amp_offset_pct - 50;
		offset = (int)(255 * offset / 50.0);
	}

	if (dsi->is_binnable) dsicmd_set_binning(dsi, dsi->bin_mode);

	if (dsi->is_interlaced) {
		dsicmd_set_gain(dsi, 0);
		dsicmd_set_offset(dsi, 0);
		dsicmd_set_exposure_time(dsi, exposure_ticks);
		if (exposure_ticks < 10000) {
			dsicmd_set_readout_speed(dsi, DSI_READOUT_SPEED_HIGH);
			dsicmd_set_readout_delay(dsi, 3);
			dsicmd_set_readout_mode(dsi, DSI_READOUT_MODE_DUAL);
			dsicmd_get_readout_mode(dsi);
			dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_ON);
		} else {
			dsicmd_set_readout_speed(dsi, DSI_READOUT_SPEED_LOW);
			dsicmd_set_readout_delay(dsi, 5);
			dsicmd_set_readout_mode(dsi, DSI_READOUT_MODE_SINGLE);
			dsicmd_get_readout_mode(dsi);
			dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_AUTO);
		}
		dsicmd_set_gain(dsi, gain);
		dsicmd_set_offset(dsi, offset);
	} else { /* Non interlaced - DSI III */
		dsicmd_set_gain(dsi, gain);
		dsicmd_set_offset(dsi, offset);
		dsicmd_set_exposure_time(dsi, exposure_ticks);
		dsicmd_set_readout_speed(dsi, DSI_READOUT_SPEED_HIGH);
		dsicmd_set_readout_delay(dsi, 4);
		if (exposure_ticks < 10000) {
			dsicmd_set_readout_mode(dsi, DSI_READOUT_MODE_DUAL);
			dsicmd_get_readout_mode(dsi);
			dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_ON);
		} else {
			dsicmd_set_readout_mode(dsi, DSI_READOUT_MODE_SINGLE);
			dsicmd_get_readout_mode(dsi);
			dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_OFF);
		}
		//FIXME! should take vdd_mode in to account
		//if ((dsi->vdd_mode) || (dsi->exposure_time < VDD_TRH)) {
		//	dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_ON);
		//} else {
		//	dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_OFF);
		//}
	}

	dsicmd_set_flush_mode(dsi, DSI_FLUSH_MODE_CONT);
	dsicmd_get_readout_mode(dsi);
	dsicmd_get_exposure_time(dsi);

	dsicmd_start_exposure(dsi);

	dsi->imaging_state = DSI_IMAGE_EXPOSING;
	return 0;
}

double dsi_get_exposure_time_left(dsi_camera_t *dsi) {
	return dsicmd_get_exposure_time_left(dsi) / 10000.0;
}

int dsi_abort_exposure(dsi_camera_t *dsi) {
	int res = dsicmd_abort_exposure(dsi);
	dsicmd_reset_camera(dsi);
	return res;
}

int dsi_reset_camera(dsi_camera_t *dsi) {
	return dsicmd_reset_camera(dsi);
}

/**
 * Read an image from the DSI camera.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param buffer pointer to a buffer pointer.  If *buffer == 0, dsi_read_image
 * will allocate the buffer.
 * @param flags set to O_NONBLOCK for asynchronous read.
 *
 * @return 0 on success, non-zero if the image was not read.
 *
 * If the dsi or buffer pointers are invalid, returns EINVAL.  If the camera
 * is not currently exposing, returns ENOTSUP.  If an I/O error occurs,
 * returns EIO.  If the image is not ready and O_NONBLOCK was specified,
 * returns EWOULDBLOCK.
 */
int dsi_read_image(dsi_camera_t *dsi, unsigned char *buffer, int flags) {
	int status;
	int ticks_left, read_size_odd, read_size_even;
	int read_width, read_height_even, read_height_odd;

	if (dsi == NULL || buffer == NULL) return EINVAL;

	/* FIXME: This method should really only be callable if the imager is in a
	   currently imaging state. */

	if (dsi->imaging_state != DSI_IMAGE_EXPOSING)
		return ENOTSUP;

	if (dsi->exposure_time > 10000) {
		if (dsi->log_commands)
			fprintf(stderr, "long exposure, checking remaining time\n");
		/* These are in different units, so this really says "if the time left is
		   greater than 1/10 of the image read timeout time, wait."  ticks_left is
		   in units of 1/10 millisecond whle read_image_timeout is in units of
		   milliseconds. */
		ticks_left = dsicmd_get_exposure_time_left(dsi);
		/*    if (ticks_left < 0) {
			  fprintf(stderr, "ticks left < 0: %d\n", ticks_left);
			  return ticks_left;
			  }
		*/

		while (ticks_left > dsi->read_image_timeout) {
			if (dsi->log_commands)
				fprintf(stderr, "long exposure, %d ticks remaining exceeds threshold of %d\n",
						ticks_left, dsi->read_image_timeout);
			/* FIXME: There are other possible error codes which are just
			   status codes from underlying calls and not true errors.  We
			   need to fix this so that there is no possibility of overlap. */
			if ((flags & O_NONBLOCK) != 0) {
				if (dsi->log_commands)
					fprintf(stderr, "non-blocking requested, returning now\n");
				return EWOULDBLOCK;
			}
			if (dsi->log_commands)
				fprintf(stderr, "sleeping for %.4fs\n", ticks_left / 10000.0);
			usleep(100 * ticks_left);
			ticks_left = dsicmd_get_exposure_time_left(dsi);
		}
		/*    if (ticks_left < 0) {
			  fprintf(stderr, "ticks left < 0: %d\n", ticks_left);
			  return ticks_left;
			  }
		*/
	}

	if (dsi->bin_mode == BIN2X2) {
		read_width       = dsi->read_width / 2;
		read_height_even = dsi->read_height_even / 2;
		read_height_odd  = dsi->read_height_odd / 2;
	} else {
		read_width       = dsi->read_width;
		read_height_even = dsi->read_height_even;
		read_height_odd  = dsi->read_height_odd;
    }

	dsicmd_set_gain(dsi, (int)(63 * dsi->amp_gain_pct / 100.0));

	int actual_length;
	if (dsi->is_interlaced) {
		read_size_even = dsi->read_bpp * read_width * read_height_even;
		status = libusb_bulk_transfer(dsi->handle, 0x86, dsi->read_buffer_even, read_size_even, &actual_length,
							   3 * dsi->read_image_timeout);
		if (dsi->log_commands)
			dsi_log_command_info(dsi, 1, "r 86", read_size_even, (char *)dsi->read_buffer_even, 0);
		if (status < 0) {
			//fprintf(stderr, "libusb_bulk_transfer(%p, 0x86, %p, %d, %d) (even) -> returned %d\n",
			//		dsi->handle, dsi->read_buffer_even, read_size_even, 2*dsi->read_image_timeout, status);
			dsi->imaging_state = DSI_IMAGE_IDLE;
			return EIO;
		}

		read_size_odd = dsi->read_bpp * read_width * read_height_odd;
		status = libusb_bulk_transfer(dsi->handle, 0x86, dsi->read_buffer_odd, read_size_odd, &actual_length,
							   3 * dsi->read_image_timeout);
		if (dsi->log_commands)
			dsi_log_command_info(dsi, 1, "r 86", read_size_odd, (char *)dsi->read_buffer_odd, 0);
		if (status < 0) {
			//fprintf(stderr, "libusb_bulk_transfer(%p, 0x86, %p, %d, %d) (odd) -> returned %d\n",
			//		dsi->handle, dsi->read_buffer_odd, read_size_odd, 2*dsi->read_image_timeout, status);
			dsi->imaging_state = DSI_IMAGE_IDLE;
			return EIO;
		}
	} else { /* Non interlaced -> DSI III */
		int exposure_ticks = dsi->exposure_time * 10000;
		if (exposure_ticks >= 10000) {
			dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_ON);
		}
		read_size_odd = dsi->read_bpp * read_width * read_height_odd;
		status = libusb_bulk_transfer(dsi->handle, 0x86, dsi->read_buffer_odd, read_size_odd, &actual_length,
							   3 * dsi->read_image_timeout);
		if (dsi->log_commands)
			dsi_log_command_info(dsi, 1, "r 86", read_size_odd, (char *)dsi->read_buffer_odd, 0);
		if (status < 0) {
			//fprintf(stderr, "libusb_bulk_transfer(%p, 0x86, %p, %d, %d) (odd) -> returned %d\n",
			//		dsi->handle, dsi->read_buffer_odd, read_size_odd, 2*dsi->read_image_timeout, status);
			dsi->imaging_state = DSI_IMAGE_IDLE;
			return EIO;
		}
	}

	/* Set binning to 1x1 after reading the data */
	if (dsi->is_binnable) dsicmd_set_binning(dsi, BIN1X1);

	dsicmd_set_gain(dsi, 0);
	dsi->imaging_state = DSI_IMAGE_IDLE;
	return (dsicmd_decode_image(dsi, buffer) == NULL) ? EINVAL : 0;
}


/**
 * Create a simulated DSI camera intialized to behave like the named camera chip.
 *
 * @param chip_name
 *
 * @return pointer to simulated DSI camera.
 */
dsi_camera_t * dsitst_open(const char *chip_name) {
	dsi_camera_t *dsi = calloc(1, sizeof(dsi_camera_t));

	dsi->is_simulation = 1;

	dsi->command_sequence_number = 0;
	dsi->eeprom_length    = -1;
	dsi->log_commands     = verbose_init;
	dsi->test_pattern     = 0;
	dsi->exposure_time    = 10;

	dsi->version.c.family   = 10;
	dsi->version.c.model    =  1;
	dsi->version.c.version  =  1;
	dsi->version.c.revision =  0;
	dsi->fw_debug  = DSI_FW_DEBUG_OFF;
	dsi->usb_speed = DSI_USB_SPEED_HIGH;

	strncpy(dsi->chip_name, chip_name, DSI_NAME_LEN);
	strncpy(dsi->serial_number, "0123456789abcdef", DSI_NAME_LEN);

	dsicmd_init_dsi(dsi);

	/* Okay, this was learned the hard way.  The SniffUSB logs clearly showed
	   that the actual read size is calculated by rounding the size of EACH
	   ROW up to some multiple of 512 bytes.  The basic read size of the USB
	   endpoint is a multiple of 512 bytes, so this kind of makes sense as the
	   easiest implementation in firmware---just pad to 512 bytes for each
	   row, then it won't matter how many rows you stick in.  And the C++
	   driver was reading correctly, but the first implementation here was
	   rounding later, at the time of the read.  That results in core dumps
	   since have to size the buffers correctly.  Remember, each row must be a
	   multiple of 512 bytes. */

	dsi->read_bpp         = 2;
	dsi->read_height      = dsi->read_height_even + dsi->read_height_odd;
	dsi->read_width       = ((dsi->read_bpp * dsi->read_width / 512) + 1) * 256;

	fprintf(stderr, "read_size_odd  => %ld (0x%lx)\n", dsi->read_size_odd, dsi->read_size_odd);
	fprintf(stderr, "read_size_even => %ld (0x%lx)\n", dsi->read_size_even, dsi->read_size_even);
	fprintf(stderr, "read_size_bpp  => %d (0x%x)\n", dsi->read_bpp, dsi->read_bpp);

	return dsi;
}

/**
 * Read DSI image data from FILENAME into internal buffers.
 *
 * @param dsi dsi_camera_t camera struct into which the data will be loaded.
 * @param filename read from this file.  Data is assumed to be in raw format
 * @param is_binary true if the file is raw binary data to load into the read
 * buffers.  If false, the data is assumed to be in SniffUSB/USBsnoop format.
 *
 * This is a test routine to allow injecting data into the internal image
 * buffers for post-acquisition testing.
 *
 * The data to be read should be either from the same type of camera in use
 * for the test, or have been carefully constructed to be consistent with that
 * form.  If not, then the results are undefined (read, expect crashes or
 * crap).
 *
 * The raw binary data should be the same size as the read buffers that would
 * come from reading the camera.
 *
 * SniffUSB/USBsnoop data is of the form
 *
 *    00000000: 13 45 13 49 13 4e 12 ac 49 b3 4d f2 52 40 56 67
 *    00000010: 5a 46 5e 31 62 3b 65 98 69 29 6c bb 6f b2 72 9d
 *    ...
 *
 * Note that this will read the FIRST image found in the file which means it
 * won't work :-)  The problem is that both Envisage and MaximDL seem to
 * take several short throw-away images as part of the intialization.  I don't
 * know why and they don't seem to be necessary, so we don't do it.  But if
 * you use a SniffUSB dump from Envisage or MaximDL, and it includes the
 * initialization, the first apparent image in the dump is not the image you
 * get on the screen.  The point is, you want your test data to include ONLY
 * the test image you are going to compare.
 *
 * @return
 */
int dsitst_read_image(dsi_camera_t *dsi, const char *filename, int is_binary) {
	FILE *fptr = 0;
	int status = 0;

	/* state = 0, looking for first match.
	 * state = 1, reading first buffer.
	 * state = 2, looking for second match.
	 * state = 3, reading second buffer.
	 * state = 4, done.
	 */
	int state = 0;

	if (is_binary) {
		/* Oops, don't really support this yet. */
		status = -1;
		goto OOPS;

	} else {
		char line[1000];
		regex_t preg;
		regmatch_t pmatch[32];
		const char *regex = " *([0-9a-f]{8}):( [0-9a-f]{2}){16}";
		int status;
		unsigned char *write_buffer = NULL;
		size_t buffer_size = 0;

		status = regcomp(&preg, regex, REG_EXTENDED|REG_ICASE);
		assert(status == 0);

		fptr = fopen(filename, "r");
		assert(fptr != 0);

		char *res = fgets(line, 1000, fptr);
		while (!feof(fptr)) {

			if (regexec(&preg, line, 32, pmatch, 0) == 0) {
				if (state == 0) {
					state = 1;
					write_buffer = dsi->read_buffer_even;
					buffer_size  = dsi->read_size_even;
				}

				if (state == 2) {
					state = 3;
					write_buffer = dsi->read_buffer_odd;
					buffer_size  = dsi->read_size_odd;
				}

				if (state == 1 || state == 3) {
					unsigned char *wptr;
					int offset;
					int v[16], i;

					sscanf(line, "%08x:", &offset);
					assert(offset + 16 <= buffer_size);

					assert(write_buffer != 0);
					wptr = write_buffer + offset;
					sscanf(line + pmatch[1].rm_eo + 1,
						   "%02x %02x %02x %02x %02x %02x %02x %02x "
						   "%02x %02x %02x %02x %02x %02x %02x %02x",
						   v+0x0, v+0x1, v+0x2, v+0x3, v+0x4, v+0x5, v+0x6, v+0x7,
						   v+0x8, v+0x9, v+0xa, v+0xb, v+0xc, v+0xd, v+0xe, v+0xf);
					for (i = 0; i < 16; i++) {
						wptr[i] = v[i];
					}
				}

			} else if (state == 1 || state == 3) {
				state++;
				write_buffer = 0;
			}

			if (state > 3)
				break;

			res = fgets(line, 1000, fptr);
		}
		regfree(&preg);
	}

 OOPS:
	if (fptr != 0) {
		fclose(fptr);
	}

	return 0;
}
