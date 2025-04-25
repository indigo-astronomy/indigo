// Copyright (c) 2024 CloudMakers, s. r. o.
// All rights reserved.
//
// Code is based on OpenPENTAX library
// Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO PENTAX Camera driver
 \file indigo_ccd_pentax.c
 */

#define DRIVER_VERSION 0x02000000
#define DRIVER_NAME "indigo_ccd_pentax"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include <indigo/indigo_usb_utils.h>

#include "indigo_ccd_pentax.h"

#define PRIVATE_DATA        ((pentax_private_data *)device->private_data)

// -------------------------------------------------------------------------------- SCSI-over-USB implementation

#define RETRY_MAX										5
#define TIMEOUT											1000
#define BOMS_GET_MAX_LUN  					0xFE
#define INQUIRY_LENGTH							0x24
#define MAX_HEX_DUMP								128
#define TIMEOUT											1000

#define PRIVATE_DATA        				((pentax_private_data *)device->private_data)

#define DSLR_PROGRAM_PROPERTY				PRIVATE_DATA->dslr_program_property
#define DSLR_CAPTURE_MODE_PROPERTY	PRIVATE_DATA->dslr_capture_mode_property
#define DSLR_APERTURE_PROPERTY			PRIVATE_DATA->dslr_aperture_property
#define DSLR_SHUTTER_PROPERTY				PRIVATE_DATA->dslr_shutter_property
#define DSLR_COMPRESSION_PROPERTY		PRIVATE_DATA->dslr_compression_property
#define DSLR_ISO_PROPERTY						PRIVATE_DATA->dslr_iso_property

typedef struct {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} command_block_wrapper;

typedef struct {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} command_status_wrapper;

enum flags {
	PENTAX_LITTLE_ENDIAN =    0x0001,
	PENTAX_LEGACY_COMMANDS =  0x0002,
	PENTAX_LEGACY_BULB =      0x0004,
};

enum status {
	PENTAX_EXPOSURE_MODE = 0,
	PENTAX_APERTURE,
	PENTAX_SHUTTER,
	PENTAX_ISO,
	PENTAX_LENS_APERTURE
};

struct {
	uint32_t identity;
	uint16_t flags;
	uint16_t width, height;
	float pixel_size;
	uint8_t resolutions[4];
	enum status status[10];
	uint16_t shutter_denom_max;
	struct {
		uint32_t min, max;
	} iso;
	const char *name;
} MODEL_INFO[] = {
	{ 0x00013010, 0x0000, 8256, 6192, 5.3, { 51, 32, 21,  3 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 4000, { 100, 204800 }, "645Z" },
	{ 0x00012c20, 0x0004, 3872, 2592, 5.3, { 10,  6,  2,  0 }, { 0xac, 0x34, 0x2c, 0x60, 0x12c }, 4000, { 100,   1600 }, "GX10" },
	{ 0x00012cd4, 0x0004, 4672, 3104, 4.7, { 14, 10,  6,  2 }, { 0xac, 0x34, 0x2c, 0x60, 0x140 }, 4000, { 100,   3200 }, "GX20" },
	{ 0x00012ef8, 0x0004, 4928, 3264, 4.8, { 16, 12,  8,  5 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 4000, { 100,  12800 }, "K-01" },
	{ 0x00013092, 0x0001, 7360, 4912, 4.9, { 36, 22, 12,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x148 }, 8000, { 100, 204800 }, "K-1" },
	{ 0x00013240, 0x0001, 7360, 4912, 4.9, { 36, 22, 12,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x148 }, 8000, { 100, 819200 }, "K-1 II" },
	{ 0x00012d72, 0x0004, 3872, 2592, 5.6, { 10,  6,  2,  0 }, { 0xb0, 0x38, 0x30, 0x64, 0x140 }, 4000, { 100,   3200 }, "K-2000" },
	{ 0x00012fc0, 0x0005, 6016, 4000, 3.9, { 24, 14,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,  51200 }, "K-3" },
	{ 0x00012f52, 0x0004, 4928, 3264, 4.8, { 16, 12,  8,  5 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 100,  12800 }, "K-30" },
	{ 0x0001309c, 0x0001, 6016, 4000, 3.9, { 24, 14,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,  51200 }, "K-3II" },
	{ 0x00013254, 0x0001, 6192, 4128, 3.9, { 24, 14,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,  51200 }, "K-3III" },
	{ 0x00012e76, 0x0004, 4928, 3264, 4.8, { 16, 10,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,  12800 }, "K-5" },
	{ 0x00012fb6, 0x0004, 4928, 3264, 4.8, { 16, 12,  8,  5 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 100,  51200 }, "K-50" },
	{ 0x00012fca, 0x0004, 4928, 3264, 4.8, { 16, 12,  8,  5 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 100,  51200 }, "K-500" },
	{ 0x00012f70, 0x0004, 4928, 3264, 4.8, { 16, 10,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,  12800 }, "K-5II" },
	{ 0x00012f71, 0x0004, 4928, 3264, 4.8, { 16, 10,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,  12800 }, "K-5IIs" },
	{ 0x00012db8, 0x0004, 4672, 3104, 4.7, { 14, 10,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 8000, { 100,   3200 }, "K-7" },
	{ 0x00013222, 0x0001, 6000, 4000, 3.9, { 24, 14,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x148 }, 6000, { 100, 102400 }, "K-70" },
	{ 0x0001301a, 0x0005, 5472, 3648, 3.9, { 20, 12,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 100,  51200 }, "K-S1" },
	{ 0x00013024, 0x0005, 5472, 3648, 3.9, { 20, 12,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 100,  51200 }, "K-S2" },
	{ 0x00012d73, 0x0004, 3872, 2592, 5.6, { 10,  6,  2,  0 }, { 0xb0, 0x38, 0x30, 0x64, 0x140 }, 4000, { 100,   3200 }, "K-m" },
	{ 0x00012e6c, 0x0004, 4288, 2848, 5.0, { 12, 10,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 200,  12800 }, "K-r" },
	{ 0x00012dfe, 0x0004, 4288, 2848, 5.0, { 12, 10,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 6000, { 200,   6400 }, "K-x" },
	{ 0x00012b9c, 0x0006, 3008, 2008, 6.1, {  6,  4,  2,  0 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 4000, { 200,   3200 }, "K100D" },
	{ 0x00012ba2, 0x0006, 3008, 2008, 6.1, {  6,  4,  2,  0 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 4000, { 200,   3200 }, "K100D Super" },
	{ 0x00012c1e, 0x0004, 3872, 2592, 5.3, { 10,  6,  2,  0 }, { 0xac, 0x34, 0x2c, 0x60, 0x12c }, 4000, { 100,   1600 }, "K10D" },
	{ 0x00012b9d, 0x0004, 3008, 2008, 6.1, {  6,  4,  2,  0 }, { 0xb4, 0x3c, 0x34, 0x68, 0x144 }, 4000, { 200,   3200 }, "K110D" },
	{ 0x00012cfa, 0x0004, 3872, 2592, 5.3, { 10,  6,  2,  0 }, { 0xac, 0x34, 0x2c, 0x60, 0x13c }, 4000, { 100,   1600 }, "K200D" },
	{ 0x00012cd2, 0x0004, 4672, 3104, 4.7, { 14, 10,  6,  2 }, { 0xac, 0x34, 0x2c, 0x60, 0x140 }, 4000, { 100,   3200 }, "K20D" },
	{ 0x0001322c, 0x0001, 6016, 4000, 3.9, { 24, 14,  6,  2 }, { 0xb4, 0x3c, 0x34, 0x68, 0x148 }, 6000, { 100, 819200 }, "KP" },

	{ 0 }
};

enum exposure_mode {
	PENTAX_EXPOSURE_MODE_M = 8,
	PENTAX_EXPOSURE_MODE_AV = 5,
	PENTAX_EXPOSURE_MODE_TV = 4,
	PENTAX_EXPOSURE_MODE_SV = 15,
	PENTAX_EXPOSURE_MODE_P = 0,
	PENTAX_EXPOSURE_MODE_B = 9,
	PENTAX_EXPOSURE_MODE_AUTO = 1
};

static struct {
	enum exposure_mode value;
	const char *label;
} exporure_mode_list[] = {
	{ PENTAX_EXPOSURE_MODE_M, "M" },
	{ PENTAX_EXPOSURE_MODE_AV, "Av" },
	{ PENTAX_EXPOSURE_MODE_TV, "Tv" },
	{ PENTAX_EXPOSURE_MODE_SV, "Sv" },
	{ PENTAX_EXPOSURE_MODE_P, "P" },
	{ PENTAX_EXPOSURE_MODE_AUTO, "Auto" },
	{ 0, NULL }
};

static struct {
	uint32_t value;
	const char *label;
} aperture_list[] = {
	{ (12 << 16) |   10, "f/1.2" },
	{ (14 << 16) |   10, "f/1.4" },
	{ (18 << 16) |   10, "f/1.8" },
	{ (20 << 16) |   10, "f/2.0" },
	{ (28 << 16) |   10, "f/2.8" },
	{ (35 << 16) |   10, "f/3.5" },
	{ (40 << 16) |   10, "f/4.0" },
	{ (45 << 16) |   10, "f/4.5" },
	{ (56 << 16) |   10, "f/5.6" },
	{ (67 << 16) |   10, "f/6.7" },
	{ (80 << 16) |   10, "f/8.0" },
	{ (95 << 16) |   10, "f/9.5" },
	{ (110 << 16) |   10, "f/11" },
	{ (130 << 16) |   10, "f/13" },
	{ (160 << 16) |   10, "f/16" },
	{ (190 << 16) |   10, "f/19" },
	{ (220 << 16) |   10, "f/22" },
	{ (270 << 16) |   10, "f/27" },
	{ (320 << 16) |   10, "f/32" },
	{ (380 << 16) |   10, "f/38" },
	{ 0, NULL }
};

uint32_t iso_list[] = { 100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600, 819200, 0 };

static struct {
	uint32_t value;
	const char *label;
} shutter_list[] = {
	{ 0, "Bulb" },
	{ (30 << 16) |    1, "30s" },
	{ (20 << 16) |    1, "20s" },
	{ (15 << 16) |    1, "15s" },
	{ (10 << 16) |    1, "10s" },
	{ ( 8 << 16) |    1, "8s" },
	{ ( 6 << 16) |    1, "6s" },
	{ ( 4 << 16) |    1, "4s" },
	{ ( 3 << 16) |    1, "3s" },
	{ ( 2 << 16) |    1, "2s" },
	{ (15 << 16) |   10, "1.5s" },
	{ ( 1 << 16) |    1, "1s" },
	{ ( 7 << 16) |   10, "0.7s" },
	{ ( 5 << 16) |   10, "0.5s" },
	{ ( 3 << 16) |   10, "0.3s" },
	{ ( 1 << 16) |    4, "1/4s" },
	{ ( 1 << 16) |    6, "1/6s" },
	{ ( 1 << 16) |    8, "1/8s" },
	{ ( 1 << 16) |   10, "1/10s" },
	{ ( 1 << 16) |   15, "1/15s" },
	{ ( 1 << 16) |   20, "1/20s" },
	{ ( 1 << 16) |   30, "1/30s" },
	{ ( 1 << 16) |   45, "1/45s" },
	{ ( 1 << 16) |   60, "1/60s" },
	{ ( 1 << 16) |   90, "1/90s" },
	{ ( 1 << 16) |  125, "1/125s" },
	{ ( 1 << 16) |  180, "1/180s" },
	{ ( 1 << 16) |  250, "1/250s" },
	{ ( 1 << 16) |  350, "1/350s" },
	{ ( 1 << 16) |  500, "1/500s" },
	{ ( 1 << 16) |  750, "1/750s" },
	{ ( 1 << 16) | 1000, "1/1000s" },
	{ ( 1 << 16) | 1500, "1/1500s" },
	{ ( 1 << 16) | 2000, "1/2000s" },
	{ ( 1 << 16) | 3000, "1/3000s" },
	{ ( 1 << 16) | 4000, "1/4000s" },
	{ ( 1 << 16) | 6000, "1/6000s" },
	{ ( 1 << 16) | 8000, "1/8000s" },
	{ 0, NULL }
};

typedef struct {
	libusb_device *dev;
	libusb_device_handle *handle;
	uint8_t lun;
	uint8_t endpoint_in;
	uint8_t endpoint_out;
	uint32_t tag;
	uint32_t identity;
	int model;
	uint32_t flags;
	const char *name;
	uint32_t min_aperture, max_aperture;
	indigo_property *dslr_program_property;
	indigo_property *dslr_capture_mode_property;
	indigo_property *dslr_aperture_property;
	indigo_property *dslr_shutter_property;
	indigo_property *dslr_compression_property;
	indigo_property *dslr_iso_property;
	indigo_timer *state_timer;
	pthread_mutex_t mutex;

} pentax_private_data;

//static uint8_t *set_uint16_le(uint16_t value, uint8_t *buffer) {
//	buffer[0] = value & 0xff;
//	value >>= 8;
//	buffer[1] = value & 0xff;
//	return buffer;
//}

//static uint8_t *set_uint16_be(uint16_t value, uint8_t *buffer) {
//	buffer[1] = value & 0xff;
//	value >>= 8;
//	buffer[0] = value & 0xff;
//	return buffer;
//}

//static uint8_t *set_uint16(indigo_device *device, uint16_t value, uint8_t *buffer) {
//	if (PRIVATE_DATA->flags & PENTAX_LITTLE_ENDIAN) {
//		return set_uint16_le(value, buffer);
//	} else {
//		return set_uint16_be(value, buffer);
//	}
//}

//static uint16_t get_uint16_le(uint8_t *buffer) {
//	return (buffer[1] << 8) | buffer[0];
//}

//static uint16_t get_uint16_be(uint8_t *buffer) {
//	return (buffer[0] << 8) | buffer[1];
//}

//static uint16_t get_uint16(indigo_device *device, uint8_t *buffer) {
//	if (PRIVATE_DATA->flags & PENTAX_LITTLE_ENDIAN) {
//		return get_uint16_le(buffer);
//	} else {
//		return get_uint16_be(buffer);
//	}
//}

//static char *uint16_to_hex(uint16_t value) {
//	static char s[5];
//	sprintf(s, "%04x", value);
//	return s;
//}

static uint8_t *set_uint32_le(uint32_t value, uint8_t *buffer) {
	buffer[0] = value & 0xff;
	value >>= 8;
	buffer[1] = value & 0xff;
	value >>= 8;
	buffer[2] = value & 0xff;
	value >>= 8;
	buffer[3] = value & 0xff;
	return buffer;
}

static uint8_t *set_uint32_be(uint32_t value, uint8_t *buffer) {
	buffer[3] = value & 0xff;
	value >>= 8;
	buffer[2] = value & 0xff;
	value >>= 8;
	buffer[1] = value & 0xff;
	value >>= 8;
	buffer[0] = value & 0xff;
	return buffer;
}

static uint8_t *set_uint32(indigo_device *device, uint32_t value, uint8_t *buffer) {
	if (PRIVATE_DATA->flags & PENTAX_LITTLE_ENDIAN) {
		return set_uint32_le(value, buffer);
	} else {
		return set_uint32_be(value, buffer);
	}
}

static uint32_t get_uint32_le(uint8_t *buffer) {
	return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

static uint32_t get_uint32_be(uint8_t *buffer) {
	return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

static uint32_t get_uint32(indigo_device *device, uint8_t *buffer) {
	if (PRIVATE_DATA->flags & PENTAX_LITTLE_ENDIAN) {
		return get_uint32_le(buffer);
	} else {
		return get_uint32_be(buffer);
	}
}

static char *uint32_to_hex(uint32_t value) {
	static char s[9];
	sprintf(s, "%08x", value);
	return s;
}

static uint32_t hex_to_uint32(char *s) {
	uint32_t value;
	if (sscanf(s, "%x", &value) == 1) {
		return value;
	}
	return 0;
}

//static char *uint32_to_fraction(uint32_t value) {
//	static char s[9];
//	double nom = (value >> 16) & 0xffff;
//	double denom = value & 0xffff;
//	if (denom == 1) {
//		sprintf(s, "%.0f", nom / denom);
//	} else {
//		sprintf(s, "%.1f", nom / denom);
//	}
//	return s;
//}

static const char *to_hex(uint8_t *bytes, uint32_t count) {
	static char buffer[3 * MAX_HEX_DUMP + 5];
	int n = count <= MAX_HEX_DUMP ? count : MAX_HEX_DUMP;
	for (int i = 0; i < n; i++) {
		sprintf(buffer + 3 * i, "%02x ", bytes[i]);
	}
	if (count > MAX_HEX_DUMP) {
		sprintf(buffer + 3 * MAX_HEX_DUMP, "...");
	}
	return buffer;
}

static uint8_t scsi_command(indigo_device *device, uint8_t *cdb, int cdb_len, uint8_t flags, uint8_t *data, uint32_t data_length) {
	command_block_wrapper cbw = { { 'U', 'S', 'B', 'C' }, PRIVATE_DATA->tag++, data_length, flags, PRIVATE_DATA->lun, cdb_len, 0 };
	command_status_wrapper csw;
	int size = 0;
	memcpy(cbw.CBWCB, cdb, cdb_len);
	for (int i = 0; i < RETRY_MAX; i++) {
		int res = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->endpoint_out, (unsigned char*)&cbw, 31, &size, TIMEOUT);
		if (res == LIBUSB_ERROR_PIPE) {
			libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->endpoint_out);
			continue;
		}
		if (res == LIBUSB_SUCCESS) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d CDB bytes written: %s", cdb_len, to_hex(cdb, cdb_len));
			break;
		}
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "0xFF, %s (%d)", libusb_strerror((enum libusb_error)res), res);
		return 0xFF;
	}
	if (data != NULL && data_length > 0) {
		int res = libusb_bulk_transfer(PRIVATE_DATA->handle, (flags & 0x80) == 0 ? PRIVATE_DATA->endpoint_out : PRIVATE_DATA->endpoint_in, data, data_length, &size, TIMEOUT);
		if (res == LIBUSB_SUCCESS) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d bytes %s: %s", size, (flags & 0x80) == 0 ? "written" : "read", to_hex(data, size));
		} else {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "0xFF, %s (%d)", libusb_strerror((enum libusb_error)res), res);
		}
	}
	for (int i = 0; i < RETRY_MAX; i++) {
		int res = libusb_bulk_transfer(PRIVATE_DATA->handle, PRIVATE_DATA->endpoint_in, (unsigned char*)&csw, 13, &size, TIMEOUT);
		if (res == LIBUSB_ERROR_PIPE) {
			libusb_clear_halt(PRIVATE_DATA->handle, PRIVATE_DATA->endpoint_in);
			continue;
		}
		if (res != LIBUSB_SUCCESS) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "0xFF, %s (%d)", libusb_strerror((enum libusb_error)res), res);
			return 0xFF;
		}
		break;
	}
	if (size != 13) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "0xFF, invalid CSW read size");
		return 0xFF;
	}
	if (csw.dCSWTag != cbw.dCBWTag) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "0xFF, mismatched tags");
		return 0xFF;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x)", csw.bCSWStatus ? "Failure" : "OK",  csw.bCSWStatus);
	return csw.bCSWStatus;
}

uint8_t pentax_command(indigo_device *device, uint8_t group, uint8_t command, uint8_t arg_count) {
	uint8_t cdb[16] = { 0xf0, 0x24, group, command, arg_count & 0xFF, (arg_count >> 8) & 0xFF };
	uint8_t result = scsi_command(device, cdb, 6, 0x80, NULL, 0);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%02x %02x %d %s (%02x)", group, command, arg_count, result ? "Failure" : "OK", result);
	return result;
}

uint8_t pentax_status(indigo_device *device, uint32_t *length) {
	uint8_t cdb[16] = { 0xf0, 0x26, 0, 0, 0, 0 };
	uint8_t buffer[8] = { 0 };
	int repeat = 0;
	if (length) {
		while (true) {
			uint8_t result = scsi_command(device, cdb, 6, 0x80, buffer, sizeof(buffer));
			if (result == 0) {
				if (buffer[7] != 0) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failure (%02x), s0 = %02x, s1 = %02x", buffer[7], buffer[6], buffer[7]);
					return buffer[7];
				} else if (buffer[6] == 0x01) {
					*length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OK (00) repeat %d, %d bytes to read, s0 = %02x, s1 = %02x", repeat, *length, buffer[6], buffer[7]);
					return 0;
				}
				usleep(50000);
				repeat++;
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x)", result ? "Failure" : "OK", result);
				return result;
			}
		}
	} else {
		while (true) {
			uint8_t result = scsi_command(device, cdb, 6, 0x80, buffer, sizeof(buffer));
			if (result == 0) {
				if (buffer[7] == 0) {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OK (00) repeat %d, s0 = %02x, s1 = %02x", repeat, buffer[6], buffer[7]);
					return 0;
				} else if (buffer[7] == 0x01) {
					usleep(50000);
					repeat++;
					continue;
				} else {
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, " Failure (%02x), s0 = %02x, s1 = %02x", buffer[7], buffer[6], buffer[7]);
					return buffer[7];
				}
			} else {
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x)", result ? "Failure" : "OK", result);
				return result;
			}
		}
	}
}

uint8_t pentax_read(indigo_device *device, uint8_t *buffer, uint16_t length, uint16_t offset) {
	uint8_t cdb[16] = { 0xf0, 0x49, offset & 0xFF, (offset >> 8) & 0xFF , length & 0xFF, (length >> 8) & 0xFF};
	uint8_t result = scsi_command(device, cdb, 6, 0x80, buffer, length);
	if (result == 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x), %s", result ? "Failure" : "OK", result, to_hex(buffer, length));
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x)", result ? "Failure" : "OK", result);
	}
	return result;
}

uint8_t pentax_parameter(indigo_device *device, uint8_t *buffer, uint16_t length, uint16_t offset) {
	uint8_t cdb[16] = { 0xf0, 0x4F, offset & 0xFF, (offset >> 8) & 0xFF , length & 0xFF, (length >> 8) & 0xFF};
	uint8_t result = scsi_command(device, cdb, 6, 0x00, buffer, length);
	if (result == 0) {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s %s (%02x), %s", to_hex(buffer, length), result ? "Failure" : "OK", result, to_hex(buffer, length));
	} else {
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s %s (%02x)", to_hex(buffer, length), result ? "Failure" : "OK", result);
	}
	return result;
}

uint8_t pentax_legacy_connect(indigo_device *device) {
	uint8_t result = pentax_command(device, 0x00, 0x05, 0);
	if (result == 0) {
		result = pentax_status(device, NULL);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x)", result ? "Failure" : "OK", result);
		return result;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failure (%02x)", result);
	return result;
}

uint8_t pentax_connect(indigo_device *device, uint32_t on) {
	uint8_t param[4];
	set_uint32(device, on ? 1 : 0, param);
	uint8_t result = pentax_parameter(device, param, 4, 0);
	if (result == 0) {
		result = pentax_command(device, 0x10, 0x0a, 4);
		if (result == 0) {
			result = pentax_status(device, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s (%02x)", on, result ? "Failure" : "OK", result);
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d Failure (%02x)", on, result);
	return result;
}

uint8_t pentax_get_identity(indigo_device *device) {
	uint32_t length;
	uint8_t result = pentax_command(device, 0x00, 0x04, 0);
	if (result == 0) {
		result = pentax_status(device, &length);
		if (result == 0) {
			uint8_t buffer[length];
			result = pentax_read(device, buffer, length, 0);
			if (result == 0) {
				uint32_t le_identity = get_uint32_le(buffer);
				uint32_t be_identity = get_uint32_be(buffer);
				for (int i = 0; i < MODEL_INFO[i].identity; i++) {
					if (MODEL_INFO[i].identity == be_identity || MODEL_INFO[i].identity == le_identity) {
						PRIVATE_DATA->identity = MODEL_INFO[i].identity == be_identity ? be_identity : le_identity;
						PRIVATE_DATA->model = i;
						PRIVATE_DATA->flags = MODEL_INFO[i].flags;
						sprintf(INFO_DEVICE_MODEL_ITEM->text.value, "PENTAX %s (%08x)", MODEL_INFO[i].name, PRIVATE_DATA->identity);
						CCD_INFO_WIDTH_ITEM->number.value = CCD_INFO_WIDTH_ITEM->number.min = CCD_INFO_WIDTH_ITEM->number.max = CCD_FRAME_WIDTH_ITEM->number.value = CCD_FRAME_WIDTH_ITEM->number.min = CCD_FRAME_WIDTH_ITEM->number.max =MODEL_INFO[i].width;
						CCD_INFO_HEIGHT_ITEM->number.value = CCD_INFO_HEIGHT_ITEM->number.min = CCD_INFO_HEIGHT_ITEM->number.max = CCD_FRAME_HEIGHT_ITEM->number.value = CCD_FRAME_HEIGHT_ITEM->number.min = CCD_FRAME_HEIGHT_ITEM->number.max = MODEL_INFO[i].height;
						CCD_INFO_PIXEL_SIZE_ITEM->number.value = CCD_INFO_PIXEL_SIZE_ITEM->number.min = CCD_INFO_PIXEL_SIZE_ITEM->number.max = CCD_INFO_PIXEL_WIDTH_ITEM->number.value = CCD_INFO_PIXEL_WIDTH_ITEM->number.min = CCD_INFO_PIXEL_WIDTH_ITEM->number.max = CCD_INFO_PIXEL_HEIGHT_ITEM->number.value = CCD_INFO_PIXEL_HEIGHT_ITEM->number.min = CCD_INFO_PIXEL_HEIGHT_ITEM->number.max = MODEL_INFO[i].pixel_size;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OK (00), %06x, %08x, '%s'", PRIVATE_DATA->identity, PRIVATE_DATA->flags, PRIVATE_DATA->name);
						return 0;
					}
				}
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failure (%02x)", result);
	return result;
}

uint8_t pentax_get_firmware(indigo_device *device) {
	uint32_t length;
	uint8_t result = pentax_command(device, 0x01, 0x01, 0);
	if (result == 0) {
		result = pentax_status(device, &length);
		if (result == 0) {
			uint8_t buffer[length];
			result = pentax_read(device, buffer, length, 0);
			if (result == 0) {
				if (PRIVATE_DATA->flags & PENTAX_LITTLE_ENDIAN) {
					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%d.%d.%d.%d", buffer[3], buffer[2], buffer[1], buffer[0]);
				} else {
					sprintf(INFO_DEVICE_FW_REVISION_ITEM->text.value, "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2], buffer[3]);
				}
				INDIGO_DRIVER_DEBUG(DRIVER_NAME, "OK (00), %d.%d.%d.%d", buffer[0], buffer[1], buffer[2], buffer[3]);
				return result;
			}
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failure (%02x)", result);
	return result;
}

uint8_t pentax_get_state(indigo_device *device) {
	uint32_t length;
	uint8_t result = pentax_command(device, 0x00, 0x01, 0);
	if (result == 0) {
		result = pentax_status(device, &length);
		if (result == 0) {
			uint8_t buffer[length];
			result = pentax_read(device, buffer, length, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x), %s", result ? "Failure" : "OK", result, to_hex(buffer, length));
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failure (%02x)", result);
	return result;
}

uint8_t pentax_get_full_state(indigo_device *device) {
	uint32_t length;
	uint8_t result = pentax_command(device, 0x00, 0x08, 0);
	if (result == 0) {
		result = pentax_status(device, &length);
		if (result == 0) {
			uint8_t buffer[length];
			result = pentax_read(device, buffer, length, 0);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%s (%02x), %s", result ? "Failure" : "OK", result, to_hex(buffer, length));
			if (result == 0) {
				uint16_t state;
				uint32_t value1, value2, value3;
				char *name;
				state = MODEL_INFO[PRIVATE_DATA->model].status[PENTAX_EXPOSURE_MODE];
				uint32_t exposure_mode = get_uint32(device, buffer + state);
				name = uint32_to_hex(exposure_mode == PENTAX_EXPOSURE_MODE_B ? PENTAX_EXPOSURE_MODE_M : exposure_mode);
				for (int i = 0; i < DSLR_PROGRAM_PROPERTY->count; i++) {
					indigo_item *item = DSLR_PROGRAM_PROPERTY->items + i;
					if (!strcmp(item->name, name)) {
						if (!item->sw.value) {
							indigo_set_switch(DSLR_PROGRAM_PROPERTY, item, true);
							indigo_update_property(device, DSLR_PROGRAM_PROPERTY, NULL);
						}
					}
				}
				state = MODEL_INFO[PRIVATE_DATA->model].status[PENTAX_LENS_APERTURE];
				value1 = (get_uint32(device, buffer + state) << 16) | get_uint32(device, buffer + state + 4);
				value2 = (get_uint32(device, buffer + state + 8) << 16) | get_uint32(device, buffer + state + 12);
				state = MODEL_INFO[PRIVATE_DATA->model].status[PENTAX_APERTURE];
				value3 = (get_uint32(device, buffer + state) << 16) | get_uint32(device, buffer + state + 4);
				indigo_property_perm perm = exposure_mode == PENTAX_EXPOSURE_MODE_M || exposure_mode == PENTAX_EXPOSURE_MODE_B || exposure_mode == PENTAX_EXPOSURE_MODE_AV || exposure_mode == PENTAX_EXPOSURE_MODE_P ? INDIGO_RW_PERM : INDIGO_RO_PERM;
				if (PRIVATE_DATA->min_aperture != value1 || PRIVATE_DATA->max_aperture != value2 || DSLR_APERTURE_PROPERTY->perm != perm) {
					PRIVATE_DATA->min_aperture = value1;
					PRIVATE_DATA->max_aperture = value2;
					indigo_delete_property(device, DSLR_APERTURE_PROPERTY, NULL);
					bool show = false;
					int count = 0;
					for (int i = 0; aperture_list[i].label; i++) {
						if (aperture_list[i].value == PRIVATE_DATA->min_aperture) {
							show = true;
						}
						if (show) {
							indigo_init_switch_item(DSLR_APERTURE_PROPERTY->items + count, uint32_to_hex(aperture_list[i].value), aperture_list[i].label, aperture_list[i].value == value3);
							count++;
						}
						if (aperture_list[i].value == PRIVATE_DATA->max_aperture) {
							show = false;
						}
					}
					DSLR_APERTURE_PROPERTY->count = count;
					DSLR_APERTURE_PROPERTY->perm = perm;
					indigo_define_property(device, DSLR_APERTURE_PROPERTY, NULL);
				} else {
					name = uint32_to_hex(value3);
					for (int i = 0; i < DSLR_APERTURE_PROPERTY->count; i++) {
						indigo_item *item = DSLR_APERTURE_PROPERTY->items + i;
						if (!strcmp(item->name, name)) {
							if (!item->sw.value) {
								indigo_set_switch(DSLR_APERTURE_PROPERTY, item, true);
								indigo_update_property(device, DSLR_APERTURE_PROPERTY, NULL);
							}
						}
					}
				}
				if (exposure_mode == PENTAX_EXPOSURE_MODE_B) {
					if (DSLR_SHUTTER_PROPERTY->perm == INDIGO_RO_PERM) {
						indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
						indigo_set_switch(DSLR_SHUTTER_PROPERTY, DSLR_SHUTTER_PROPERTY->items, true);
						DSLR_SHUTTER_PROPERTY->perm = INDIGO_RW_PERM;
						indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
					} else {
						indigo_item *item = DSLR_SHUTTER_PROPERTY->items;
						if (!item->sw.value) {
							indigo_set_switch(DSLR_SHUTTER_PROPERTY, item, true);
							indigo_update_property(device, DSLR_SHUTTER_PROPERTY, NULL);
						}
					}
				} else {
					state = MODEL_INFO[PRIVATE_DATA->model].status[PENTAX_SHUTTER];
					value1 = (get_uint32(device, buffer + state) << 16) | get_uint32(device, buffer + state + 4);
					name = uint32_to_hex(value1);
					indigo_property_perm perm = exposure_mode == PENTAX_EXPOSURE_MODE_M || exposure_mode == PENTAX_EXPOSURE_MODE_B || exposure_mode == PENTAX_EXPOSURE_MODE_TV || exposure_mode == PENTAX_EXPOSURE_MODE_P ? INDIGO_RW_PERM : INDIGO_RO_PERM;
					for (int i = 0; i < DSLR_SHUTTER_PROPERTY->count; i++) {
						indigo_item *item = DSLR_SHUTTER_PROPERTY->items + i;
						if (!strcmp(item->name, name)) {
							if (!item->sw.value || DSLR_SHUTTER_PROPERTY->perm != perm) {
								indigo_set_switch(DSLR_SHUTTER_PROPERTY, item, true);
								if (DSLR_SHUTTER_PROPERTY->perm != perm) {
									indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
									DSLR_SHUTTER_PROPERTY->perm = perm;
									indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
								} else {
									indigo_update_property(device, DSLR_SHUTTER_PROPERTY, NULL);
								}
							}
						}
					}
				}
				state = MODEL_INFO[PRIVATE_DATA->model].status[PENTAX_ISO];
				value1 = get_uint32(device, buffer + state);
				name = uint32_to_hex(value1);
				for (int i = 0; i < DSLR_ISO_PROPERTY->count; i++) {
					indigo_item *item = DSLR_ISO_PROPERTY->items + i;
					if (!strcmp(item->name, name)) {
						if (!item->sw.value) {
							indigo_set_switch(DSLR_ISO_PROPERTY, item, true);
							indigo_update_property(device, DSLR_ISO_PROPERTY, NULL);
						}
					}
				}

			}
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Failure (%02x)", result);
	return result;
}

uint8_t pentax_set_mode(indigo_device *device, uint32_t mode) {
	uint8_t param[4];
	set_uint32(device, mode, param);
	uint8_t result = pentax_parameter(device, param, 4, 0);
	if (result == 0) {
		result = pentax_command(device, 0x00, 0x00, 4);
		if (result == 0) {
			result = pentax_status(device, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s (%02x)", mode, result ? "Failure" : "OK", result);
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d Failure (%02x)", mode, result);
	return result;
}

uint8_t pentax_set_wu(indigo_device *device, uint32_t mode) {
	uint8_t param[4];
	set_uint32(device, mode, param);
	uint8_t result = pentax_parameter(device, param, 4, 0);
	if (result == 0) {
		result = pentax_command(device, 0x00, 0x09, 4);
		if (result == 0) {
			result = pentax_status(device, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %s (%02x)", mode, result ? "Failure" : "OK", result);
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d Failure (%02x)", mode, result);
	return result;
}

uint8_t pentax_set_aperture(indigo_device *device, uint32_t aperture_nom, uint32_t aperture_denom) {
	uint8_t param[4];
	set_uint32(device, aperture_nom, param);
	uint8_t result = pentax_parameter(device, param, 4, 0);
	if (result == 0) {
		set_uint32(device, aperture_denom, param);
		result = pentax_parameter(device, param, 4, 4);
	}
	if (result == 0) {
		result = pentax_command(device, 0x18, 0x17, 8);
		if (result == 0) {
			result = pentax_status(device, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %d %s (%02x)", aperture_nom, aperture_denom, result ? "Failure" : "OK", result);
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %d Failure (%02x)", aperture_nom, aperture_denom, result);
	return result;
}

uint8_t pentax_set_shutter(indigo_device *device, uint32_t shutter_nom, uint32_t shutter_denom) {
	uint8_t param[4];
	set_uint32(device, shutter_nom, param);
	uint8_t result = pentax_parameter(device, param, 4, 0);
	if (result == 0) {
		set_uint32(device, shutter_denom, param);
		result = pentax_parameter(device, param, 4, 4);
	}
	if (result == 0) {
		result = pentax_command(device, 0x18, 0x16, 8);
		if (result == 0) {
			result = pentax_status(device, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %d %s (%02x)", shutter_nom, shutter_denom, result ? "Failure" : "OK", result);
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %d Failure (%02x)", shutter_nom, shutter_denom, result);
	return result;
}

uint8_t pentax_set_iso(indigo_device *device, uint32_t iso, uint32_t min_iso, uint32_t max_iso) {
	uint8_t param[4];
	set_uint32(device, iso, param);
	uint8_t result = pentax_parameter(device, param, 4, 0);
	if (result == 0) {
		set_uint32(device, min_iso, param);
		result = pentax_parameter(device, param, 4, 4);
	}
	if (result == 0) {
		set_uint32(device, max_iso, param);
		result = pentax_parameter(device, param, 4, 8);
	}
	if (result == 0) {
		result = pentax_command(device, 0x18, 0x15, 12);
		if (result == 0) {
			result = pentax_status(device, NULL);
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %d %d %s (%02x)", iso, min_iso, max_iso, result ? "Failure" : "OK", result);
			return result;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%d %d %d Failure (%02x)",  iso, min_iso, max_iso, result);
	return result;
}

static bool pentax_open(indigo_device *device) {
	int rc = 0;
	libusb_device *dev = PRIVATE_DATA->dev;
	rc = libusb_open(dev, &PRIVATE_DATA->handle);
	libusb_device_handle *handle = PRIVATE_DATA->handle;
	rc = libusb_set_auto_detach_kernel_driver(handle, 1);
	if (rc != LIBUSB_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "libusb_set_auto_detach_kernel_driver: %s (%d)", libusb_strerror((enum libusb_error)rc), rc);
		libusb_close(handle);
		PRIVATE_DATA->handle = 0;
		return false;
	}
	rc = libusb_kernel_driver_active(handle, 0);
	if (rc == 0) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Kernel driver not used");
	} else if (rc == 1) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Kernel driver used");
	} else if (rc == LIBUSB_ERROR_NOT_SUPPORTED) {
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Kernel driver not supported");
	}
	rc = libusb_claim_interface(handle, 0);
	if (rc != LIBUSB_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "libusb_claim_interface: %s (%d)", libusb_strerror((enum libusb_error)rc), rc);
		libusb_close(handle);
		PRIVATE_DATA->handle = 0;
		indigo_send_message(device, "Failed to claim device - this driver requires to run the application with root permission!");
		return false;
	}
	rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, BOMS_GET_MAX_LUN, 0, 0, &PRIVATE_DATA->lun, 1, TIMEOUT);
	if (rc == LIBUSB_ERROR_PIPE) {
		PRIVATE_DATA->lun = 0;
	} else if (rc < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "libusb_control_transfer: %s (%d)", libusb_strerror((enum libusb_error)rc), rc);
		libusb_close(handle);
		PRIVATE_DATA->handle = 0;
		return false;
	}
	if (pentax_set_mode(device, 1) != 0) {
		libusb_close(handle);
		PRIVATE_DATA->handle = 0;
		return false;
	}
	return true;
}

static void pentax_close(indigo_device *device) {
	pentax_set_mode(device, 0);
	libusb_close(PRIVATE_DATA->handle);
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_close");
}

// -------------------------------------------------------------------------------- INDIGO CCD device implementation

static void state_timer_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	pentax_get_full_state(device);
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	indigo_reschedule_timer(device, 3, &PRIVATE_DATA->state_timer);
}

static void ccd_connect_callback(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool result = pentax_open(device);
		if (result) {
			pentax_get_identity(device);
			pentax_connect(device, true);
			if (PRIVATE_DATA->flags & PENTAX_LEGACY_COMMANDS) {
				pentax_legacy_connect(device);
			} else {
				pentax_set_wu(device, 2);
				pentax_connect(device, true);
			}
			pentax_get_firmware(device);
			indigo_update_property(device, INFO_PROPERTY, NULL);
			indigo_define_property(device, DSLR_PROGRAM_PROPERTY, NULL);
			indigo_define_property(device, DSLR_APERTURE_PROPERTY, NULL);
			uint16_t shutter_denom_max = MODEL_INFO[PRIVATE_DATA->model].shutter_denom_max;
			char *name = uint32_to_hex(0x00010000 | shutter_denom_max);
			for (int i = 0; i < DSLR_SHUTTER_PROPERTY->count; i++) {
				indigo_item *item = DSLR_SHUTTER_PROPERTY->items + i;
				if (!strcmp(item->name, name)) {
					DSLR_SHUTTER_PROPERTY->count = i + 1;
					break;
				}
			}
			indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
			uint32_t iso_min = MODEL_INFO[PRIVATE_DATA->model].iso.min;
			uint32_t iso_max = MODEL_INFO[PRIVATE_DATA->model].iso.max;
			int count = 0;
			for (int i = 0; i < DSLR_ISO_PROPERTY->count; i++) {
				uint32_t iso = iso_list[i];
				if (iso_min <= iso && iso <= iso_max) {
					char label[8];
					sprintf(label, "%d", iso);
					indigo_init_switch_item(DSLR_ISO_PROPERTY->items + count, uint32_to_hex(iso), label, false);
					count++;
				}
			}
			DSLR_ISO_PROPERTY->count = count;
			indigo_define_property(device, DSLR_ISO_PROPERTY, NULL);
			indigo_set_timer(device, 1, state_timer_callback, &PRIVATE_DATA->state_timer);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
		} else {
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->state_timer);
		indigo_delete_property(device, DSLR_PROGRAM_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_PROGRAM_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_APERTURE_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
		indigo_delete_property(device, DSLR_ISO_PROPERTY, NULL);
		pentax_connect(device, false);
		pentax_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_ccd_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void iso_callback(indigo_device *device) {
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	for (int i = 0; i < DSLR_ISO_PROPERTY->count; i++) {
		indigo_item *item = DSLR_ISO_PROPERTY->items + i;
		if (item->sw.value) {
			if (pentax_set_iso(device, hex_to_uint32(item->name), MODEL_INFO[PRIVATE_DATA->model].iso.min, MODEL_INFO[PRIVATE_DATA->model].iso.max) == 0) {
				DSLR_ISO_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, DSLR_ISO_PROPERTY, NULL);
				break;
			} else {
				DSLR_ISO_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, DSLR_ISO_PROPERTY, NULL);
				break;
			}
		}
	}
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result ccd_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_ccd_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- CCD_INFO, CCD_BIN, CCD_FRAME
		INFO_PROPERTY->count = 6;
		CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.value = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.min = CCD_FRAME_BITS_PER_PIXEL_ITEM->number.max = 8;
		CCD_FRAME_PROPERTY->perm = INDIGO_RO_PERM;
		// -------------------------------------------------------------------------------- DSLR_PROGRAM
		int count;
		for (count = 0; exporure_mode_list[count].label; count++) ;
		DSLR_PROGRAM_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_PROGRAM_PROPERTY_NAME, "DSLR", "Program mode", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, count);
		for (int i = 0; i < count; i++) {
			indigo_init_switch_item(DSLR_PROGRAM_PROPERTY->items + i, uint32_to_hex(exporure_mode_list[i].value), exporure_mode_list[i].label, false);
		}
		// -------------------------------------------------------------------------------- DSLR_APERTURE
		for (count = 0; aperture_list[count].label; count++) ;
		DSLR_APERTURE_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_APERTURE_PROPERTY_NAME, "DSLR", "Aperture", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, count);
		// -------------------------------------------------------------------------------- DSLR_SHUTTER
		for (count = 0; shutter_list[count].label; count++) ;
		DSLR_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_SHUTTER_PROPERTY_NAME, "DSLR", "Shutter speed", INDIGO_OK_STATE,INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, count);
		for (int i = 0; i < count; i++) {
			indigo_init_switch_item(DSLR_SHUTTER_PROPERTY->items + i, uint32_to_hex(shutter_list[i].value), shutter_list[i].label, false);
		}
		// -------------------------------------------------------------------------------- DSLR_SHUTTER
		for (count = 0; iso_list[count]; count++) ;
		DSLR_ISO_PROPERTY = indigo_init_switch_property(NULL, device->name, DSLR_ISO_PROPERTY_NAME, "DSLR", "ISO", INDIGO_OK_STATE,INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, count);
		// --------------------------------------------------------------------------------
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return ccd_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result ccd_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(DSLR_PROGRAM_PROPERTY);
		indigo_define_matching_property(DSLR_APERTURE_PROPERTY);
		indigo_define_matching_property(DSLR_SHUTTER_PROPERTY);
		indigo_define_matching_property(DSLR_ISO_PROPERTY);
	}
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, ccd_connect_callback, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(DSLR_ISO_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_ISO_PROPERTY, property, false);
		DSLR_ISO_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, DSLR_ISO_PROPERTY, NULL);
		indigo_set_timer(device, 0, iso_callback, NULL);
		return INDIGO_OK;
	}
	return indigo_ccd_change_property(device, client, property);
}

static indigo_result ccd_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		ccd_connect_callback(device);
	}
	indigo_release_property(DSLR_PROGRAM_PROPERTY);
	indigo_release_property(DSLR_APERTURE_PROPERTY);
	indigo_release_property(DSLR_SHUTTER_PROPERTY);
	indigo_release_property(DSLR_ISO_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_ccd_detach(device);
}

// -------------------------------------------------------------------------------- hot-plug support

#define MAX_DEVICES 10

#define PENTAX_VENDOR_ID 	0x0A17
#define SAMSUNG_VENDOR_ID 0x04E8

static indigo_device *devices[MAX_DEVICES];
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_plug_event(libusb_device *dev) {
	static indigo_device ccd_template = INDIGO_DEVICE_INITIALIZER(
		"",
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);
	struct libusb_device_descriptor descriptor;
	struct libusb_config_descriptor *config;
	libusb_device_handle *handle;
	uint8_t endpoint_in = 0;
	uint8_t endpoint_out = 0;
	pthread_mutex_lock(&device_mutex);
	int rc = libusb_get_device_descriptor(dev, &descriptor);
	if (rc == LIBUSB_SUCCESS && (descriptor.idVendor == PENTAX_VENDOR_ID || descriptor.idVendor == SAMSUNG_VENDOR_ID)) {
		unsigned char vendor[32], product[32];
		rc = libusb_open(dev, &handle);
		if (rc == LIBUSB_SUCCESS) {
			if (descriptor.iManufacturer) {
				libusb_get_string_descriptor_ascii(handle, descriptor.iManufacturer, vendor, sizeof(vendor));
			}
			if (descriptor.iProduct) {
				libusb_get_string_descriptor_ascii(handle, descriptor.iProduct, product, sizeof(product));
			}
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "detected %s %s (0x%04x, 0x%04x)", vendor, product, descriptor.idVendor, descriptor.idProduct);
			rc = libusb_get_config_descriptor(dev, 0, &config);
			if (rc == LIBUSB_SUCCESS) {
				int n = config->bNumInterfaces;
				for (int i = 0; i < n; i++) {
					for (int j = 0; j < config->interface[i].num_altsetting; j++) {
						const struct libusb_interface_descriptor *altsetting = config->interface[i].altsetting + j;
						if ((altsetting->bInterfaceClass == LIBUSB_CLASS_MASS_STORAGE) && ((altsetting->bInterfaceSubClass == 0x01) || (altsetting->bInterfaceSubClass == 0x06)) && (altsetting->bInterfaceProtocol == 0x50)) {
							for (int k = 0; k < altsetting->bNumEndpoints; k++) {
								const struct libusb_endpoint_descriptor *endpoint = &altsetting->endpoint[k];
								if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK | LIBUSB_TRANSFER_TYPE_INTERRUPT)) {
									if ((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) && endpoint_in == 0) {
										endpoint_in = endpoint->bEndpointAddress;
									} else if (!(endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) && endpoint_out == 0) {
										endpoint_out = endpoint->bEndpointAddress;
									}
								}
							}
						}
					}
				}
				libusb_free_config_descriptor(config);
			}
			libusb_close(handle);
		}
		if (endpoint_in != 0 && endpoint_out != 0) {
			pentax_private_data *private_data = indigo_safe_malloc(sizeof(pentax_private_data));
			libusb_ref_device(dev);
			private_data->dev = dev;
			private_data->endpoint_in = endpoint_in;
			private_data->endpoint_out = endpoint_out;
			indigo_device *device = indigo_safe_malloc_copy(sizeof(indigo_device), &ccd_template);
			indigo_device *master_device = device;
			char usb_path[INDIGO_NAME_SIZE];
			indigo_get_usb_path(dev, usb_path);
			snprintf(device->name, INDIGO_NAME_SIZE, "%s %s", vendor, product);
			indigo_make_name_unique(device->name, "%s", usb_path);
			device->private_data = private_data;
			device->master_device = master_device;
			for (int j = 0; j < MAX_DEVICES; j++) {
				if (devices[j] == NULL) {
					indigo_attach_device(devices[j] = device);
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static void process_unplug_event(libusb_device *dev) {
	pthread_mutex_lock(&device_mutex);
	pentax_private_data *private_data = NULL;
	for (int j = 0; j < MAX_DEVICES; j++) {
		if (devices[j] != NULL) {
			indigo_device *device = devices[j];
			if (PRIVATE_DATA->dev == dev) {
				indigo_detach_device(device);
				libusb_unref_device(dev);
				free(private_data);
				free(device);
				devices[j] = NULL;
			}
		}
	}
	pthread_mutex_unlock(&device_mutex);
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	switch (event) {
	case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
		INDIGO_ASYNC(process_plug_event, dev);
		break;
	case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
		process_unplug_event(dev);
		break;
	}
	return 0;
};

static libusb_hotplug_callback_handle callback_handle;

indigo_result indigo_ccd_pentax(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "PENTAX Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		for (int i = 0; i < MAX_DEVICES; i++) {
			devices[i] = 0;
		}
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_register_callback ->  %s", rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		for (int i = 0; i < MAX_DEVICES; i++)
			VERIFY_NOT_CONNECTED(devices[i]);
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		for (int j = 0; j < MAX_DEVICES; j++) {
			if (devices[j] != NULL) {
				indigo_device *device = devices[j];
				hotplug_callback(NULL, PRIVATE_DATA->dev, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, NULL);
			}
		}
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
