// Copyright (c) 2016-2025 CloudMakers, s. r. o.
// Copyright (c) 2016-2025 Rumen G. Bogdanovski
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

/** INDIGO Bus
 \file indigo_io.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <termios.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>
#endif

#if defined(INDIGO_WINDOWS)
#include <io.h>
#include <winsock2.h>
#ifdef __MINGW32__
#include <wspiapi.h>
#endif
#define close closesocket
#pragma warning(disable:4996)
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

typedef struct {
	int value;
	size_t len;
	char *str;
} sbaud_rate;
#define BR(str,val) { val, sizeof(str), str }

static sbaud_rate br[] = {
	BR(     "50", B50),
	BR(     "75", B75),
	BR(    "110", B110),
	BR(    "134", B134),
	BR(    "150", B150),
	BR(    "200", B200),
	BR(    "300", B300),
	BR(    "600", B600),
	BR(   "1200", B1200),
	BR(   "1800", B1800),
	BR(   "2400", B2400),
	BR(   "4800", B4800),
	BR(   "9600", B9600),
	BR(  "19200", B19200),
	BR(  "38400", B38400),
	BR(  "57600", B57600),
	BR( "115200", B115200),
	BR( "230400", B230400),
#if !defined(__APPLE__) && !defined(__MACH__)
	BR( "460800", B460800),
	BR( "500000", B500000),
	BR( "576000", B576000),
	BR( "921600", B921600),
	BR("1000000", B1000000),
	BR("1152000", B1152000),
	BR("1500000", B1500000),
	BR("2000000", B2000000),
	BR("2500000", B2500000),
	BR("3000000", B3000000),
	BR("3500000", B3500000),
	BR("4000000", B4000000),
#endif /* not OSX */
	BR(       "", 0),
};

/* map string to actual baudrate value */
static int map_str_baudrate(const char *baudrate) {
	sbaud_rate *brp = br;
	while (strncmp(brp->str, baudrate, brp->len)) {
		if (brp->str[0]=='\0') return -1;
		brp++;
	}
	return brp->value;
}

static int configure_tty_options(struct termios *options, const char *baudrate) {
	int cbits = CS8, cpar = 0, ipar = IGNPAR, bstop = 0;
	int baudr = 0;
	char *mode;
	char copy[32];
	strncpy(copy, baudrate, sizeof(copy));

	/* format is 9600-8N1, so split baudrate from the rest */
	mode = strchr(copy, '-');
	if (mode == NULL) {
		errno = EINVAL;
		return -1;
	}
	*mode = '\0';
	++mode;

	baudr = map_str_baudrate(copy);
	if (baudr == -1) {
		errno = EINVAL;
		return -1;
	}

	if (strlen(mode) != 3) {
		errno = EINVAL;
		return -1;
	}

	switch (mode[0]) {
		case '8': cbits = CS8; break;
		case '7': cbits = CS7; break;
		case '6': cbits = CS6; break;
		case '5': cbits = CS5; break;
		default :
			errno = EINVAL;
			return -1;
			break;
	}

	switch (mode[1]) {
		case 'N':
		case 'n':
			cpar = 0;
			ipar = IGNPAR;
			break;
		case 'E':
		case 'e':
			cpar = PARENB;
			ipar = INPCK;
			break;
		case 'O':
		case 'o':
			cpar = (PARENB | PARODD);
			ipar = INPCK;
			break;
		default :
			errno = EINVAL;
			return -1;
			break;
	}

	switch (mode[2]) {
		case '1': bstop = 0; break;
		case '2': bstop = CSTOPB; break;
		default :
			errno = EINVAL;
			return -1;
			break;
	}

	memset(options, 0, sizeof(*options));  /* clear options struct */

	options->c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
	options->c_iflag = ipar;
	options->c_oflag = 0;
	options->c_lflag = 0;
	options->c_cc[VMIN] = 0;       /* block untill n bytes are received */
	options->c_cc[VTIME] = 50;     /* block untill a timer expires (n * 100 mSec.) */

	cfsetispeed(options, baudr);
	cfsetospeed(options, baudr);

	return 0;
}

static int open_tty(const char *tty_name, const struct termios *options, struct termios *old_options) {
	int tty_fd;
	const char auto_prefix[] = "auto://";
	const int auto_prefix_len = sizeof(auto_prefix)-1;
	const char *tty_name_buf = tty_name;

	if (!strncmp(tty_name_buf, auto_prefix, auto_prefix_len)) {
		tty_name_buf += auto_prefix_len;
    }

	tty_fd = open(tty_name_buf, O_RDWR | O_NOCTTY | O_SYNC);
	if (tty_fd == -1) {
		return -1;
	}

	if (old_options) {
		if (tcgetattr(tty_fd, old_options) == -1) {
			close(tty_fd);
			return -1;
		}
	}

	if (tcsetattr(tty_fd, TCSANOW, options) == -1) {
		close(tty_fd);
		return -1;
	}

	return tty_fd;
}

int indigo_open_serial(const char *dev_file) {
	return indigo_open_serial_with_speed(dev_file, 9600);
}

int indigo_open_serial_with_speed(const char *dev_file, int speed) {
	char baud_str[32];

	snprintf(baud_str, sizeof(baud_str), "%d-8N1", speed);
	return indigo_open_serial_with_config(dev_file, baud_str);
}

/* baudconfig is in form "9600-8N1" */
int indigo_open_serial_with_config(const char *dev_file, const char *baudconfig) {
	struct termios to;

	int res = configure_tty_options(&to, baudconfig);
	if (res == -1) {
		return res;
	}

	return open_tty(dev_file, &to, NULL);
}

#endif /* Linux and Mac */

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

static int open_socket(const char *host, int port, int type) {
	struct addrinfo *address_list, *address;
	if (getaddrinfo(host, NULL, NULL, &address_list) != 0) {
		return -1;
	}
	int handle = -1;
	for (address = address_list; address != NULL; address = address->ai_next) {
		handle = socket(AF_INET, type, 0);
		if (handle == -1) {
			return handle;
		}
		*(uint16_t *)(address->ai_addr->sa_data) = htons(port);
		if (connect(handle, address->ai_addr, address->ai_addrlen) == 0) {
			struct timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;
			setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
			setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
			break;
		}
		indigo_error("Can't connect socket (%s)", strerror(errno));
		close(handle);
		handle = -1;
	}
	freeaddrinfo(address_list);
	return handle;
}

#endif

#if defined(INDIGO_WINDOWS)

static int open_socket(const char *host, int port, int type) {
	struct sockaddr_in srv_info;
	struct hostent *he;
	int sock;
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if ((he = gethostbyname(host)) == NULL) {
		return -1;
	}
	if ((sock = socket(AF_INET, type, 0))== -1) {
		return -1;
	}
	memset(&srv_info, 0, sizeof(srv_info));
	srv_info.sin_family = AF_INET;
	srv_info.sin_port = htons(port);
	srv_info.sin_addr = *((struct in_addr *)he->h_addr);
	if (connect(sock, (struct sockaddr *)&srv_info, sizeof(struct sockaddr)) < 0) {
		close(sock);
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		close(sock);
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		close(sock);
		return -1;
	}
	return sock;
}

#endif

int indigo_open_tcp(const char *host, int port) {
	return open_socket(host, port, SOCK_STREAM);
}

int indigo_open_udp(const char *host, int port) {
	return open_socket(host, port, SOCK_DGRAM);
}


bool indigo_is_device_url(const char *name, const char *prefix) {
	if (!name) return false;

	if (prefix) {
		char prefix_full[INDIGO_NAME_SIZE];
		sprintf(prefix_full, "%s://", prefix);
		return ((!strncmp(name, "tcp://", 6)) || (!strncmp(name, "udp://", 6)) || (!strncmp(name, prefix_full, strlen(prefix_full))));
	}
	return ((!strncmp(name, "tcp://", 6)) || (!strncmp(name, "udp://", 6)));
}

int indigo_open_network_device(const char *url, int default_port, indigo_network_protocol *protocol_hint) {
	int port = default_port;
	char host_name[INDIGO_NAME_SIZE];

	INDIGO_DEBUG(indigo_debug("Trying to open TCP or UDP..."));

	if (!url || !protocol_hint) return -1;

	char *found = strstr(url, "://");
	if (found == NULL) return -1;

	if (!strncmp(url, "tcp://", 6)) {
		*protocol_hint = INDIGO_PROTOCOL_TCP;
	} else if (!strncmp(url, "udp://", 6)) {
		*protocol_hint = INDIGO_PROTOCOL_UDP;
	}

	char *host = found + 3;
	char *colon = strchr(host, ':');
	if (colon) {
		strncpy(host_name, host, colon - host);
		host_name[colon - host] = 0;
		port = atoi(colon + 1);
	} else {
		INDIGO_COPY_NAME(host_name, host);
	}

	INDIGO_DEBUG(indigo_debug("Trying to open: '%s', port = %d, protocol = %d", host_name, port, *protocol_hint));

	switch (*protocol_hint) {
		case INDIGO_PROTOCOL_TCP: return indigo_open_tcp(host_name, port);
		case INDIGO_PROTOCOL_UDP: return indigo_open_udp(host_name, port);
	}
	return -1;
}

int indigo_read(int handle, char *buffer, long length) {
	long remains = length;
	long total_bytes = 0;
	while (true) {
#if defined(INDIGO_WINDOWS)
		long bytes_read = recv(handle, buffer, remains, 0);
		if (bytes_read == -1 && WSAGetLastError() == WSAETIMEDOUT) {
			Sleep(500);
			continue;
		}
#else
		long bytes_read = read(handle, buffer, remains);
#endif
		if (bytes_read <= 0) {
			if (bytes_read < 0) {
				INDIGO_ERROR(indigo_error("%d -> // %s", handle, strerror(errno)));
			}
			return (int)bytes_read;
		}
		total_bytes += bytes_read;
		if (bytes_read == remains) {
			return (int)total_bytes;
		}
		buffer += bytes_read;
		remains -= bytes_read;
	}
}

#if defined(INDIGO_WINDOWS)
int indigo_recv(int handle, char *buffer, long length) {
	while (true) {
		long bytes_read = recv(handle, buffer, length, 0);
		if (bytes_read == -1 && WSAGetLastError() == WSAETIMEDOUT) {
			Sleep(500);
			continue;
		}
		return (int)bytes_read;
	}
}

int indigo_close(int handle) {
	return closesocket(handle);
}
#endif

int indigo_read_line(int handle, char *buffer, int length) {
	char c = '\0';
	long total_bytes = 0;
	while (total_bytes < length) {
#if defined(INDIGO_WINDOWS)
		long bytes_read = recv(handle, &c, 1, 0);
		if (bytes_read == -1 && WSAGetLastError() == WSAETIMEDOUT) {
			Sleep(500);
			continue;
		}
#else
		long bytes_read = read(handle, &c, 1);
#endif
		if (bytes_read > 0) {
			if (c == '\r') {
				;
			} else if (c != '\n') {
				buffer[total_bytes++] = c;
			} else {
				break;
			}
		} else {
			errno = ECONNRESET;
			INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> // Connection reset", handle));
			return -1;
		}
	}
	buffer[total_bytes] = '\0';
	INDIGO_TRACE_PROTOCOL(indigo_trace("%d -> %s", handle, buffer));
	return (int)total_bytes;
}

bool indigo_write(int handle, const char *buffer, long length) {
	long remains = length;
	while (true) {

#if defined(INDIGO_WINDOWS)
		long bytes_written = send(handle, buffer, remains, 0);
#else
		long bytes_written = write(handle, buffer, remains);
#endif
		if (bytes_written < 0) {
			INDIGO_ERROR(indigo_error("%d <- // %s", handle, strerror(errno)));
			return false;
		}
		if (bytes_written == remains) {
			return true;
		}
		buffer += bytes_written;
		remains -= bytes_written;
	}
}

bool indigo_printf(int handle, const char *format, ...) {
	if (strchr(format, '%')) {
		char *buffer = indigo_alloc_large_buffer();
		va_list args;
		va_start(args, format);
		int length = vsnprintf(buffer, INDIGO_BUFFER_SIZE, format, args);
		va_end(args);
		INDIGO_TRACE_PROTOCOL(indigo_trace("%d <- %s", handle, buffer));
		bool result = indigo_write(handle, buffer, length);
		indigo_free_large_buffer(buffer);
		return result;
	} else {
		INDIGO_TRACE_PROTOCOL(indigo_trace("%d <- %s", handle, format));
		return indigo_write(handle, format, strlen(format));
	}
}

int indigo_scanf(int handle, const char *format, ...) {
	char *buffer = indigo_alloc_large_buffer();
	if (indigo_read_line(handle, buffer, INDIGO_BUFFER_SIZE) <= 0) {
		indigo_free_large_buffer(buffer);
		return 0;
	}
	va_list args;
	va_start(args, format);
	int count = vsscanf(buffer, format, args);
	va_end(args);
	indigo_free_large_buffer(buffer);
	return count;
}

int indigo_select(int handle, long usec) {
	struct timeval tv;
	fd_set readout;
	FD_ZERO(&readout);
	FD_SET(handle, &readout);
	tv.tv_sec = (int)(usec / 1000000);
	tv.tv_usec = (int)(usec % 1000000);
	return select(handle + 1, &readout, NULL, NULL, &tv);
}

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

void indigo_compress(char *name, char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size) {
	z_stream defstream;
	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;
	defstream.avail_in = in_size;
	defstream.next_in = (Bytef *)in_buffer;
	defstream.avail_out = *out_size;
	defstream.next_out = (Bytef *)out_buffer;
	gz_header header = { 0 };
	header.name = (Bytef *)name;
	header.comment = Z_NULL;
	header.extra = Z_NULL;
	int r = deflateInit2(&defstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY);
	r = deflateSetHeader(&defstream, &header);
	r = deflate(&defstream, Z_FINISH);
	r = deflateEnd(&defstream);
	*out_size = (unsigned)((unsigned char *)defstream.next_out - (unsigned char *)out_buffer);
}

void indigo_decompress(char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size) {
	z_stream infstream;
	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;
	infstream.avail_in = in_size;
	infstream.next_in = (Bytef *)in_buffer;
	infstream.avail_out = *out_size;
	infstream.next_out = (Bytef *)out_buffer;
	int r = inflateInit2(&infstream, MAX_WBITS + 16);
	r = inflate(&infstream, Z_NO_FLUSH);
	r = inflateEnd(&infstream);
	*out_size = (unsigned)((unsigned char *)infstream.next_out - (unsigned char *)out_buffer);
}

#endif
