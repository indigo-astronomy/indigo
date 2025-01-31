// Copyright (c) 2025 CloudMakers, s. r. o.
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

/** INDIGO Unified I/O
 \file indigo_uni_io.c
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
#include <unistd.h>
#include <termios.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>
#elif defined(INDIGO_WINDOWS)
#include <direct.h>
#else
#warning "TODO: Unified I/O"
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_uni_io.h>

char *indigo_uni_strerror(indigo_uni_handle handle) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	static char buffer[128];
	snprintf(buffer, sizeof(buffer), "%s (%d)", strerror(handle.last_error), handle.last_error);
	return buffer;
#else
#warning "TODO: indigo_uni_strerror()"
#endif
}

indigo_uni_handle indigo_uni_open_file(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	indigo_uni_handle handle;
	handle.type = INDIGO_FILE_HANDLE;
	handle.fd = open(path, O_RDONLY);
	if (handle.fd >= 0) {
		handle.opened = true;
	} else {
		handle.last_error = errno;
	}
	return handle;
#else
#warning "TODO: indigo_uni_open_file()"
#endif
}

indigo_uni_handle indigo_uni_create_file(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	indigo_uni_handle handle;
	handle.type = INDIGO_FILE_HANDLE;
	handle.fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (handle.fd >= 0) {
		handle.opened = true;
	} else {
		handle.last_error = errno;
	}
	return handle;
#else
#warning "TODO: indigo_uni_create_file()"
#endif
}

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
	int cbits = CS8, cpar = 0, ipar = IGNPAR, bstop = 0, baudr = 0;
	char *mode;
	char copy[32];
	strncpy(copy, baudrate, sizeof(copy));
	/* format is 9600-8N1, so split baudrate from the rest */
	mode = strchr(copy, '-');
	if (mode == NULL) {
		errno = EINVAL;
		return -1;
	}
	*mode++ = '\0';
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

static indigo_uni_handle open_tty(const char *tty_name, const struct termios *options, struct termios *old_options) {
	indigo_uni_handle handle = { INDIGO_FILE_HANDLE, -1, false, 0 };
	const char auto_prefix[] = "auto://";
	const int auto_prefix_len = sizeof(auto_prefix)-1;
	const char *tty_name_buf = tty_name;
	if (!strncmp(tty_name_buf, auto_prefix, auto_prefix_len)) {
		tty_name_buf += auto_prefix_len;
	}
	handle.fd = open(tty_name_buf, O_RDWR | O_NOCTTY | O_SYNC);
	if (!handle.opened) {
		handle.last_error = errno;
		return handle;
	}
	if (old_options) {
		if (tcgetattr(handle.fd, old_options) == -1) {
			handle.last_error = errno;
			close(handle.fd);
			handle.fd = -1;
			return handle;
		}
	}
	if (tcsetattr(handle.fd, TCSANOW, options) == -1) {
		handle.last_error = errno;
		close(handle.fd);
		handle.fd = -1;
		return handle;
	}
	handle.opened = true;
	return handle;
}

#endif

indigo_uni_handle indigo_open_uni_serial_with_config(const char *dev_file, const char *baudconfig) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	struct termios to;
	int res = configure_tty_options(&to, baudconfig);
	if (res == -1) {
		return (indigo_uni_handle) { INDIGO_FILE_HANDLE, -1, false, errno };
	}
	return open_tty(dev_file, &to, NULL);
#else
#warning "TODO: indigo_uni_create_file()"
#endif
}

indigo_uni_handle indigo_uni_open_serial_with_speed(const char *dev_file, int speed) {
	char baud_str[32];
	snprintf(baud_str, sizeof(baud_str), "%d-8N1", speed);
	return indigo_open_uni_serial_with_config(dev_file, baud_str);
}

indigo_uni_handle indigo_uni_open_serial(const char *dev_file) {
	return indigo_open_uni_serial_with_config(dev_file, "9600-8N1");
}

indigo_uni_handle indigo_uni_open_client_socket(const char *host, int port, int type) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	indigo_uni_handle handle = { type == SOCK_STREAM ? INDIGO_TCP_HANDLE : INDIGO_UDP_HANDLE, -1, false, 0 };
	struct addrinfo *address_list, *address;
	if (getaddrinfo(host, NULL, NULL, &address_list) == 0) {
		for (address = address_list; address != NULL; address = address->ai_next) {
			handle.fd = socket(AF_INET, type, 0);
			if (handle.fd == -1) {
				handle.last_error = errno;
				indigo_error("Can't create %s socket for '%s:%d' [%s]", type == SOCK_STREAM ? "TCP" : "UDP", host, port, indigo_uni_strerror(handle));
				break;
			}
			*(uint16_t *)(address->ai_addr->sa_data) = htons(port);
			if (connect(handle.fd, address->ai_addr, address->ai_addrlen) == 0) {
				struct timeval timeout = { .tv_sec = 5 };
				if (setsockopt(handle.fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == 0 && setsockopt(handle.fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == 0) {
					INDIGO_TRACE(indigo_trace("%d <- // connected to '%s:%d'", handle.fd, host, port));
					break;
				}
			}
			handle.last_error = errno;
			indigo_error("Can't connect %s socket for '%s:%d' [%s]", type == SOCK_STREAM ? "TCP" : "UDP", host, port, indigo_uni_strerror(handle));
			close(handle.fd);
			handle.fd = -1;
		}
	}
	freeaddrinfo(address_list);
	return handle;
#else
	static bool initialized = false;
	if (!initialized) {
		WORD version_requested = MAKEWORD(1, 1);
		WSADATA data;
		WSAStartup(version_requested, &data);
		initializd = true;
	}
	indigo_uni_handle handle = { type == SOCK_STREAM ? INDIGO_TCP_HANDLE : INDIGO_UDP_HANDLE, -1, false, 0 };
	struct hostent *he;
	if ((he = gethostbyname(host)) == NULL) {
		handle.last_eror = WSAGetLastError();
		indigo_error("Can't resolve host for '%s:%d' [%s]", host, port, indigo_uni_strerror(handle));
		return handle;
	}
	if ((handle.fd = socket(AF_INET, type, 0))== -1) {
		handle.last_eror = WSAGetLastError();
		indigo_error("Can't create %s socket for '%s:%d' [%s]", type == SOCK_STREAM ? "TCP" : "UDP", host, port, indigo_uni_strerror(handle));
		return handle;
	}
	struct sockaddr_in srv_info;
	memset(&srv_info, 0, sizeof(srv_info));
	srv_info.sin_family = AF_INET;
	srv_info.sin_port = htons(port);
	srv_info.sin_addr = *((struct in_addr *)he->h_addr);
	struct timeval timeout = { .tv_sec = 5 };
	if (connect(handle.fd, (struct sockaddr *)&srv_info, sizeof(struct sockaddr)) == 0 && setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == 0 && setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		INDIGO_TRACE(indigo_trace("%d <- // connected to '%s:%d'", handle.fd, host, port));
		return handle;
	}
	handle.last_eror = WSAGetLastError();
	indigo_error("Can't connect %s socket for '%s:%d' [%s]", type == SOCK_STREAM ? "TCP" : "UDP", host, port, indigo_uni_strerror(handle));
	return handle;
#endif
}

indigo_uni_handle indigo_uni_open_url(const char *url, int default_port, indigo_uni_handle_type protocol_hint) {
	int port = default_port;
	char host_name[INDIGO_NAME_SIZE];
	if (url == NULL || *url == 0) {
		indigo_debug("Empty URL");
		return INDIGO_ERROR_HANDLE;
	}
	char *found = strstr(url, "://");
	if (found == NULL) {
		indigo_debug("Malformed URL '%s'", url);
		return INDIGO_ERROR_HANDLE;
	}
	if (!strncmp(url, "tcp://", 6)) {
		protocol_hint = INDIGO_TCP_HANDLE;
	} else if (!strncmp(url, "udp://", 6)) {
		protocol_hint = INDIGO_UDP_HANDLE;
	}
	char *host = found + 3;
	char *colon = strchr(host, ':');
	if (colon) {
		strncpy(host_name, host, colon - host);
		host_name[colon - host] = 0;
		port = atoi(colon + 1);
	} else {
		indigo_copy_name(host_name, host);
	}
	switch (protocol_hint) {
		case INDIGO_TCP_HANDLE:
			return indigo_uni_client_tcp_socket(host_name, port);
		case INDIGO_UDP_HANDLE:
			return indigo_uni_client_udp_socket(host_name, port);
		default:
			return INDIGO_ERROR_HANDLE;
	}
}

long indigo_uni_read_available(indigo_uni_handle handle, void *buffer, long length) {
	long bytes_read;
#if defined(INDIGO_WINDOWS)
	if (handle.type == INDIGO_FILE_HANDLE) {
		bytes_read = read(handle.fd, buffer, length);
	} else {
		bytes_read = recv(handle, buffer, length, 0);
		handle.last_error = WSAGetLastError();
	}
#else
	bytes_read = read(handle.fd, buffer, length);
	handle.last_error = errno;
#endif
	return bytes_read;
}


long indigo_uni_read(indigo_uni_handle handle, void *buffer, long length) {
	long remains = length;
	long total_bytes = 0;
	while (true) {
		long bytes_read;
#if defined(INDIGO_WINDOWS)
		if (handle.type == INDIGO_FILE_HANDLE) {
			bytes_read = read(handle.fd, buffer, remains);
		} else {
			bytes_read = recv(handle, buffer, remains, 0);
			handle.last_error = WSAGetLastError();
			if (bytes_read == -1 && handle.last_error == WSAETIMEDOUT) {
				Sleep(500);
			}
		}
#else
		bytes_read = read(handle.fd, buffer, remains);
		handle.last_error = errno;
#endif
		if (bytes_read <= 0) {
			if (bytes_read < 0) {
				INDIGO_ERROR(indigo_error("%d -> // %s", handle.fd, indigo_uni_strerror(handle)));
			}
			return (int)bytes_read;
		}
		total_bytes += bytes_read;
		if (bytes_read == remains) {
			INDIGO_TRACE(indigo_trace("%d -> // %ld bytes read", handle.fd, total_bytes));
			return (int)total_bytes;
		}
		buffer += bytes_read;
		remains -= bytes_read;
	}
}

long indigo_uni_read_section(indigo_uni_handle handle, char *buffer, long length, const char *terminators, const char *ignore, long timeout) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	long bytes_read = 0;
	struct timeval tv = { .tv_sec = timeout / ONE_SECOND_DELAY, .tv_usec = timeout % ONE_SECOND_DELAY };
	*buffer = 0;
	if (handle.type == INDIGO_UDP_HANDLE) {
		if (timeout >= 0) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(handle.fd, &readout);
			long result = select(handle.fd + 1, &readout, NULL, NULL, &tv);
			if (result == 0) {
				handle.last_error = 0;
				return 0;
			}
			if (result < 0) {
				handle.last_error = errno;
				return -1;
			}
		}
		bytes_read = recv(handle.fd, buffer, length - 1, 0);
		handle.last_error = errno;
		buffer[bytes_read] = 0;
	} else {
		bool terminated = false;
		int bytes_read = 0;
		while (bytes_read < length) {
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(handle.fd, &readout);
			long result = select(handle.fd + 1, &readout, NULL, NULL, &tv);
			if (result == 0) {
				handle.last_error = 0;
				return 0;
			}
			if (result < 0) {
				handle.last_error = errno;
				return -1;
			}
			char c;
			result = read(handle.fd, &c, 1);
			if (result < 1) {
				handle.last_error = errno;
				break;
			}
			bool ignored = false;
			for (const char *s = ignore; *s; s++) {
				if (c == *s) {
					ignored = true;
					break;
				}
			}
			if (!ignored) {
				buffer[bytes_read++] = c;
			}
			for (const char *s = terminators; *s; s++) {
				if (c == *s) {
					terminated = true;
					break;
				}
			}
			if (terminated) {
				break;
			}
			tv = (struct timeval) { .tv_sec = 0, .tv_usec = 100000 };
		}
		buffer[bytes_read++] = 0;
	}
	return bytes_read;
#else
#warning "TODO: indigo_uni_create_file()"
#endif
}

int indigo_uni_scanf_line(indigo_uni_handle handle, const char *format, ...) {
	char *buffer = indigo_alloc_large_buffer();
	int count = 0;
	if (indigo_uni_read_line(handle, buffer, INDIGO_BUFFER_SIZE) > 0) {
		va_list args;
		va_start(args, format);
		count = vsscanf(buffer, format, args);
		va_end(args);
	}
	indigo_free_large_buffer(buffer);
	return count;
}

long indigo_uni_write(indigo_uni_handle handle, const char *buffer, long length) {
	long remains = length;
	while (true) {
		long bytes_written;
#if defined(INDIGO_WINDOWS)
		if (handle.type == INDIGO_FILE_HANDLE) {
			bytes_written = write(handle.fd, buffer, remains);
			handle.last_error = errno;
		} else {
			bytes_written = send(handle.fd, buffer, remains, 0);
			handle.last_error = WSAGetLastError();
		}
#else
		bytes_written = write(handle.fd, buffer, remains);
		handle.last_error = errno;
#endif
		if (bytes_written < 0) {
			INDIGO_ERROR(indigo_error("%d <- // %s", handle.fd, indigo_uni_strerror(handle)));
			return -1;
		}
		if (bytes_written == remains) {
			INDIGO_TRACE(indigo_trace("%d <- // %ld bytes written", handle.fd, bytes_written));
			return bytes_written;
		}
		buffer += bytes_written;
		remains -= bytes_written;
	}
}

long indigo_uni_printf(indigo_uni_handle handle, const char *format, ...) {
	if (strchr(format, '%')) {
		char *buffer = indigo_alloc_large_buffer();
		va_list args;
		va_start(args, format);
		int length = vsnprintf(buffer, INDIGO_BUFFER_SIZE, format, args);
		va_end(args);
		bool result = indigo_uni_write(handle, buffer, length);
		indigo_free_large_buffer(buffer);
		return result;
	} else {
		return indigo_uni_write(handle, format, strlen(format));
	}
}

long indigo_uni_seek(indigo_uni_handle handle, long position, int whence) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	long result = lseek(handle.fd, position, whence);
	handle.last_error = errno;
	return result;
#else
#warning "TODO: indigo_uni_seek()
#endif
}

void indigo_uni_close(indigo_uni_handle handle) {
	if (handle.opened) {
#if defined(INDIGO_WINDOWS)
		if (handle.type == INDIGO_FILE_HANDLE) {
			close(handle.fd);
			handle.last_error = errno;
		} else {
			shutdown(handle.fd, SD_BOTH);
			closesocket(handle.fd);
			handle.last_error = WSAGetLastError();
		}
#else
		if (handle.type != INDIGO_FILE_HANDLE) {
			shutdown(handle.fd, SHUT_RDWR);
			indigo_usleep(ONE_SECOND_DELAY);
		}
		close(handle.fd);
		handle.last_error = errno;
#endif
		handle.fd = -1;
		handle.opened = false;
	}
}

const char *indigo_uni_config_folder() {
	static char config_folder[512] = { 0 };
	if (config_folder[0] == 0) {
		snprintf(config_folder, sizeof(config_folder), "%s/.indigo", getenv("HOME"));
	}
	return config_folder;
}

bool indigo_uni_mkdir(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (mkdir(path, 0777) == 0 || errno == EEXIST) {
		return true;
	}
	return false;
#else
#warning "TODO: indigo_uni_mkdir()"
#endif
}

void indigo_uni_compress(char *name, char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
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
#else
#warning "TODO: indigo_compress()"
#endif
}

void indigo_uni_decompress(char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
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
#else
#warning "TODO: indigo_decompress()"
#endif
}
