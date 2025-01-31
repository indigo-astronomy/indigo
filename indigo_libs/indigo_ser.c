// Copyright (c) 2020 CloudMakers, s. r. o.
// All rights reserved.
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

// http://www.grischa-hahn.homepage.t-online.de/astro/ser/SER%20Doc%20V3b.pdf

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_ser.h>

static bool write_int(indigo_uni_handle handle, uint32_t n) {
	unsigned char buffer[4];
	buffer[0] = n;
	buffer[1] = n >> 8;
	buffer[2] = n >> 16;
	buffer[3] = n >> 24;
	return indigo_uni_write(handle, (const char *)buffer, 4);
}

static bool write_long(indigo_uni_handle handle, uint64_t n) {
	unsigned char buffer[8];
	buffer[0] = n;
	buffer[1] = n >> 8;
	buffer[2] = n >> 16;
	buffer[3] = n >> 24;
	buffer[4] = n >> 32;
	buffer[5] = n >> 40;
	buffer[6] = n >> 48;
	buffer[7] = n >> 56;
	return indigo_uni_write(handle, (const char *)buffer, 8);
}

indigo_ser *indigo_ser_open(const char *filename, void *buffer) {
	indigo_ser *ser = NULL;
	indigo_uni_handle handle = indigo_uni_open_file(filename);
	if (!handle.opened) {
		INDIGO_ERROR(indigo_error("indigo_ser: failed to open file for writing"));
		goto failure;
	}
	if ((ser = (indigo_ser *)malloc(sizeof(indigo_ser))) == NULL) {
		INDIGO_ERROR(indigo_error("indigo_ser: could not allocate memory for indigo_ser structure"));
		goto failure;
	}
	ser->handle = handle;
	ser->count = 0;
	long result = indigo_uni_write(handle, "LUCAM-RECORDER", 14); // 0
	result = result && write_int(handle, 0); // 14
	indigo_raw_header *header = (indigo_raw_header *)buffer;
	int bits_per_pixel = 8;
	switch (header->signature) {
		case INDIGO_RAW_MONO8:
			result = result && write_int(handle, 0); // 18
			break;
		case INDIGO_RAW_MONO16:
			result = result && write_int(handle, 0); // 18
			bits_per_pixel = 16;
			break;
		case INDIGO_RAW_RGB24:
			result = result && write_int(handle, 101); // 18
			break;
		case INDIGO_RAW_RGB48:
			result = result && write_int(handle, 101); // 18
			bits_per_pixel = 16;
			break;
	}
	result = result && write_int(handle, false); // 22
	result = result && write_int(handle, header->width); // 26
	result = result && write_int(handle, header->height); // 30
	result = result && write_int(handle, bits_per_pixel); // 34
	result = result && write_int(handle, 0); // 38
	static char zero[40] = { 0 };
	result = result && indigo_uni_write(handle, zero, 40); // 42
	result = result && indigo_uni_write(handle, zero, 40); // 82
	result = result && indigo_uni_write(handle, zero, 40); // 122
	tzset();
	long time_utc = (time(NULL) + 62135600400L) * 10000000L;
	long timezone_diff =  (timezone - daylight ? 3600 : 0) * 10000000L;
	result = result && write_long(handle, time_utc); // 162
	result = result && write_long(handle, time_utc + timezone_diff); // 170
	if (!result)
		goto failure;
	return ser;
failure:
	indigo_uni_close(handle);
	indigo_safe_free(ser);
	return NULL;
}

bool indigo_ser_add_frame(indigo_ser *ser, void *buffer, size_t len) {
	ser->count++;
	return indigo_uni_write(ser->handle, buffer + 12, len - 12);
}

bool indigo_ser_close(indigo_ser *ser) {
	indigo_uni_handle handle = ser->handle;
	if (indigo_uni_seek(handle, 38, SEEK_SET) && write_int(handle, ser->count)) {
		indigo_uni_close(handle);
		free(ser);
		return true;
	}
	return false;
}
