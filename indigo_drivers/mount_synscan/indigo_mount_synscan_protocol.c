//
//  indigo_mount_synscan_protocol.c
//  indigo_server
//
//  Created by David Hulse on 07/08/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include "indigo_driver.h"
#include "indigo_io.h"
#include "indigo_mount_synscan_private.h"
#include "indigo_mount_synscan_protocol.h"


#define SYNSCAN_START_CHAR								 ':'
#define SYNSCAN_REPLY_CHAR								 '='
#define SYNSCAN_ERROR_CHAR								 '!'
#define SYNSCAN_END_CHAR									 '\r'

#define SYNSCAN_FIRMWARE_VERSION					 ":e1"



enum MountType {
	kMountTypeEQ6 = 0x00,
	kMountTypeHEQ5 = 0x01,
	kMountTypeEQ5 = 0x02,
	kMountTypeEQ3 = 0x03,
	kMountTypeGT = 0x80,
	kMountTypeMF = 0x81,
	kMountType114GT = 0x82,
	kMountTypeDOB = 0x90
};


//  HELPERS

static const char hexDigit[] = "0123456789ABCDEF";

static inline int hexValue(char h) {
	if (h >= '0' && h <= '9')
		return h - '0';
	else if (h >= 'A' && h <= 'F')
		return h - 'A' + 10;
	else
		return 0;
}

static const char* longToHex(long n) {
	static char num[7];
	num[1] = hexDigit[n & 0xF];	n >>= 4;
	num[0] = hexDigit[n & 0xF];	n >>= 4;
	num[3] = hexDigit[n & 0xF];	n >>= 4;
	num[2] = hexDigit[n & 0xF];	n >>= 4;
	num[5] = hexDigit[n & 0xF];	n >>= 4;
	num[4] = hexDigit[n & 0xF];
	num[6] = 0;
	return num;
}

static long hexToLong(const char* b) {
	long num = 0;
	while (*b != 0) {
		num <<= 4;
		num |= hexValue(*b);
		b++;
	}
	return num;
}

static long hexResponseToLong(const char* b) {
	long num = 0;
	num |= hexValue(b[4]); num <<= 4;
	num |= hexValue(b[5]); num <<= 4;
	num |= hexValue(b[2]); num <<= 4;
	num |= hexValue(b[3]); num <<= 4;
	num |= hexValue(b[0]); num <<= 4;
	num |= hexValue(b[1]);
	return num;
}

//  GENERIC SYNSCAN COMMAND

static bool synscan_command(indigo_device* device, const char* cmd, char* r) {
	pthread_mutex_lock(&PRIVATE_DATA->port_mutex);
	int nretries = 0;
	while (nretries < 2) {
		//  Sleep for a bit if we're having to retry
		//if (nretries > 0)
		//    sleep(1);
		nretries++;

		//  Flush input
		unsigned char c;
		struct timeval tv;
		while (true) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(PRIVATE_DATA->handle, &readout);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			long result = select(1, &readout, NULL, NULL, &tv);
			if (result == 0)
				break;
			if (result < 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SELECT FAIL 1");
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
			result = read(PRIVATE_DATA->handle, &c, 1);
			if (result < 1) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "READ FAIL 1");
				pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
				return false;
			}
		}

		//  Send the command to the port
		INDIGO_DRIVER_TRACE(DRIVER_NAME, "CMD: [%s]", cmd);
		if (!indigo_write(PRIVATE_DATA->handle, cmd, strlen(cmd))) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Sending command failed");
			break;
		}
		if (!indigo_write(PRIVATE_DATA->handle, "\r", 1)) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Sending command terminator failed");
			break;
		}

		//  Read a response
		char resp[20];
		long total_bytes = 0;
		while (total_bytes < sizeof(resp)) {
			long bytes_read = read(PRIVATE_DATA->handle, &c, 1);
			if (bytes_read == 0) {
				INDIGO_DRIVER_ERROR(DRIVER_NAME, "SYNSCAN_TIMEOUT");
				break;
			}
			if (bytes_read > 0) {
				resp[total_bytes++] = c;
				if (c == '\r')
					//			else
					break;
				//		} else {
				//			errno = ECONNRESET;
				//			return -1;
			}
		}
		resp[total_bytes] = 0;
		if (total_bytes <= 0) {
		//NSData* resp = [self.transport readUpToChar:'\r' withTimeout:1000];
		//if (!resp) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "Reading response failed");
			continue;
		}

		//  Check response syntax =...<cr>, if invalid retry
		//const char* cresp = (const char*) [resp bytes];
		size_t len = strlen(resp);
		if (len < 2 || resp[0] != '=' || resp[len - 1] != '\r') {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "RESPONSE: [%.*s] - error", len - 1, resp);
			continue;
		} else {
			INDIGO_DRIVER_TRACE(DRIVER_NAME, "RESPONSE: [%.*s]", len - 1, resp);
		}

		//  Extract response payload, return
		if (r) {
			strncpy(r, resp+1, len-2);
			r[len-2] = 0;
		}
		pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
		return true;
	}

	//  Mount command failed
	pthread_mutex_unlock(&PRIVATE_DATA->port_mutex);
	return false;
}

static bool synscan_command_with_long_result(indigo_device* device, char* cmd, long* val) {
	char buffer[20];
	if (!synscan_command(device, cmd, buffer)) {
		return false;
	}
	if (val)
		*val = hexResponseToLong(buffer);
	return true;
}

static bool synscan_command_with_code_result(indigo_device* device, char* cmd, long* val) {
	char buffer[20];
	if (!synscan_command(device, cmd, buffer)) {
		return false;
	}
	if (val)
		*val = hexToLong(buffer);
	return true;
}

bool synscan_firmware_version(indigo_device* device, long* v) {
	return synscan_command_with_long_result(device, ":e1", v);
}

//  Returns number of steps required to rotate the axis 360 degrees
bool synscan_total_axis_steps(indigo_device* device, enum AxisID axis, long* v) {
	char buffer[5];
	sprintf(buffer, ":a%c", axis);
	return synscan_command_with_long_result(device, buffer, v);
}

//  Returns number of steps required to rotate the axis worm gear 360 degrees.
//  Multiple worm rotations are required to rotate the axis 360 degrees.
//  This data is useful for determining the period of any PE.

//  pecPeriodSteps
bool synscan_worm_rotation_steps(indigo_device* device, enum AxisID axis, long* v) {
	char buffer[5];
	sprintf(buffer, ":s%c", axis);
	return synscan_command_with_long_result(device, buffer, v);
}

bool synscan_step_timer_frequency(indigo_device* device, enum AxisID axis, long* v) {
	char buffer[5];
	sprintf(buffer, ":b%c", axis);
	return synscan_command_with_long_result(device, buffer, v);
}

//  speed_steps = rads/sec * (timer_freq / (axisSteps / 2PI))
//              = rads/sec * (steps/axisSteps * 2PI/sec)

bool synscan_high_speed_ratio(indigo_device* device, enum AxisID axis, long* v) {
	char buffer[5];
	sprintf(buffer, ":g%c", axis);
	return synscan_command_with_long_result(device, buffer, v);
}

bool synscan_motor_status(indigo_device* device, enum AxisID axis, long* v) {
	char buffer[5];
	sprintf(buffer, ":f%c", axis);
	return synscan_command_with_code_result(device, buffer, v);
}

//  Read back the motor position
bool synscan_axis_position(indigo_device* device, enum AxisID axis, long* v) {
	char buffer[5];
	sprintf(buffer, ":j%c", axis);
	return synscan_command_with_long_result(device, buffer, v);
}

//  Set the encoder reference position for a given axis
bool synscan_init_axis_position(indigo_device* device, enum AxisID axis, long pos) {
	char buffer[11];
	sprintf(buffer, ":E%c%s", axis, longToHex(pos));
	//printf("*************************************************** INIT AXIS %s  (pos is %ld / 0x%lX", buffer, pos, pos);
	return synscan_command(device, buffer, NULL);
}

//  Energise the motor for a given axis
bool synscan_init_axis(indigo_device* device, enum AxisID axis) {
	char buffer[5];
	sprintf(buffer, ":F%c", axis);
	return synscan_command(device, buffer, NULL);
}

//  Stop any motion for the given axis
bool synscan_stop_axis(indigo_device* device, enum AxisID axis) {
	char buffer[5];
	sprintf(buffer, ":K%c", axis);
	return synscan_command(device, buffer, NULL);
}

//  Stop any motion for the given axis
bool synscan_instant_stop_axis(indigo_device* device, enum AxisID axis) {
	char buffer[5];
	sprintf(buffer, ":L%c", axis);
	return synscan_command(device, buffer, NULL);
}

//  Set gearing for the given axis
//  setAxisMode
bool synscan_set_axis_gearing(indigo_device* device, enum AxisID axis, enum AxisDirectionID d, enum AxisSpeedID g) {
	static char codes[2] = {'0', '1'}/*, {'2', '3'}}*/;
	char buffer[7];
	sprintf(buffer, ":G%c%c%c", axis, g, codes[d]);
	//printf(buffer);
	return synscan_command(device, buffer, NULL);
}

//  Set slew step count for the given axis
//  setAxisTargetIncrement
bool synscan_set_axis_step_count(indigo_device* device, enum AxisID axis, long s) {
	char buffer[11];
	sprintf(buffer, ":H%c%s", axis, longToHex(s));
	//printf(buffer);
	return synscan_command(device, buffer, NULL);
}

//  Set slew rate for the given axis
//  setAxisStepPeriod
bool synscan_set_axis_slew_rate(indigo_device* device, enum AxisID axis, long r) {
	char buffer[11];
	sprintf(buffer, ":I%c%s", axis, longToHex(r));
	//printf(buffer);
	return synscan_command(device, buffer, NULL);
}

//  Start slewing the given axis
//  startAxis
bool synscan_slew_axis(indigo_device* device, enum AxisID axis) {
	char buffer[5];
	sprintf(buffer, ":J%c", axis);
	//printf(buffer);
	return synscan_command(device, buffer, NULL);
}

//  Set slew slowdown count for the given axis
//  setAxisBreakpointIncrement
bool synscan_set_axis_slowdown(indigo_device* device, enum AxisID axis, long s) {
	char buffer[11];
	sprintf(buffer, ":M%c%s", axis, longToHex(s));
	//printf(buffer);
	return synscan_command(device, buffer, NULL);
}

//  setAxisBreakSteps:(enum AxisID)axis steps:(unsigned long)s
//     :U...

//  setSwitch (on/off)
//     :O10\r    :O11\r

bool synscan_set_polarscope_brightness(indigo_device* device, unsigned char brightness) {
	char buffer[7];
	sprintf(buffer, ":V1%02X", brightness);
	return synscan_command(device, buffer, NULL);
}

bool synscan_set_st4_guide_rate(indigo_device* device, enum AxisID axis, enum GuideRate rate) {
	char buffer[7];
	sprintf(buffer, ":P%c%d", axis, rate);
	return synscan_command(device, buffer, NULL);
}
