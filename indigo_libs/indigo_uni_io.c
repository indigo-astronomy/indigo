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
#include <zlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <libgen.h>
#include <dirent.h>
#include <termios.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#elif defined(INDIGO_WINDOWS)
#include <io.h>
#include <direct.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#pragma message ("TODO: Unified I/O")
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_driver.h>
#include <indigo/indigo_uni_io.h>

static int handle_index = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


indigo_uni_handle _indigo_stdin_handle = { INDIGO_FILE_HANDLE, 0, 0 };
indigo_uni_handle _indigo_stdout_handle = { INDIGO_FILE_HANDLE, 1, 0 };
indigo_uni_handle _indigo_stderr_handle = { INDIGO_FILE_HANDLE, 2, 0 };

indigo_uni_handle *indigo_stdin_handle = &_indigo_stdin_handle;
indigo_uni_handle *indigo_stdout_handle = &_indigo_stdout_handle;
indigo_uni_handle *indigo_stderr_handle = &_indigo_stderr_handle;

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
indigo_uni_handle *indigo_uni_create_file_handle(int fd, int log_level) {
	indigo_uni_handle *handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
	pthread_mutex_lock(&mutex);
	handle->index = handle_index++;
	pthread_mutex_unlock(&mutex);
	handle->type = INDIGO_FILE_HANDLE;
	handle->fd = fd;
	handle->log_level = log_level;
	indigo_log_on_level(log_level, "%d <- // Wrapped %d", handle->index, fd);
	return handle;
}
#endif

static int discard_data(indigo_uni_handle *handle) {
	if (handle->type != INDIGO_COM_HANDLE) {
		return -1;
	}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	return tcflush(handle->fd, TCIOFLUSH);
#elif defined(INDIGO_WINDOWS)
	return PurgeComm(handle->com, PURGE_RXCLEAR | PURGE_TXCLEAR) ? 0 : -1;
#else
#pragma message ("TODO: discard_data()")
#endif
}

static int wait_for_data(indigo_uni_handle *handle, long timeout) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		handle->last_error = 0;
		return 1;
	} else {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(handle->fd, &readout);
		struct timeval tv = { .tv_sec = timeout / 1000000L, .tv_usec = timeout % 1000000L };
		switch (select(handle->fd + 1, &readout, NULL, NULL, &tv)) {
			case -1:
				handle->last_error = errno;
				indigo_error("%d -> // Failed to wait for (%s)", handle->index, indigo_uni_strerror(handle));
				return -1;
			case 0:
				handle->last_error = 0;
				indigo_log_on_level(handle->log_level, "%d -> // timeout", handle->index);
				return 0;
			default:
				handle->last_error = 0;
				return 1;
		}
	}
#elif defined(INDIGO_WINDOWS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		return 1;
	} else if (handle->type == INDIGO_COM_HANDLE) {
		unsigned long mask;
		ResetEvent(handle->ov_read.hEvent);
		if (!WaitCommEvent(handle->com, &mask, &handle->ov_read)){
			if (GetLastError() != ERROR_IO_PENDING) {
				handle->last_error = GetLastError();
				indigo_error("%d -> // Failed to wait for (%s)", handle->index, indigo_uni_strerror(handle));
				return -1;
			}
		}
		switch (WaitForSingleObject(handle->ov_read.hEvent, timeout / 1000)) {
			case WAIT_OBJECT_0:
				return 1;
			case WAIT_TIMEOUT:
				CancelIo(handle->com);
				indigo_log_on_level(handle->log_level, "%d -> // timeout", handle->index);
				return 0;
			default:
				handle->last_error = GetLastError();
				indigo_error("%d -> // Failed to wait for (%s)", handle->index, indigo_uni_strerror(handle));
				return -1;
		}
	} else if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		fd_set readout;
		FD_ZERO(&readout);
		FD_SET(handle->sock, &readout);
		struct timeval tv = { .tv_sec = timeout / 1000000L, .tv_usec = timeout % 1000000L };
		int result = select(0, &readout, NULL, NULL, &tv);
		if (result == 0) {
			handle->last_error = 0;
			return 0;
		} else if (result == SOCKET_ERROR) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d -> // Failed to wait for (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		handle->last_error = 0;
		return 1;
	}
	handle->last_error = 0;
	return 0;
#else
#pragma message ("TODO: wait_for_com_data()")
#endif
}

static long read_data(indigo_uni_handle *handle, void *buffer, long length) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	long bytes_read = read(handle->fd, buffer, length);
	if (bytes_read < 0) {
		handle->last_error = errno;
		indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
		return -1;
	}
	handle->last_error = 0;
	return bytes_read;
#elif defined(INDIGO_WINDOWS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		long bytes_read = _read((int)handle->fd, buffer, length);
		if (bytes_read < 0) {
			handle->last_error = errno;
			indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			return bytes_read;
		}
	} else if (handle->type == INDIGO_COM_HANDLE) {
		ResetEvent(handle->ov_read.hEvent);
		long bytes_read = 0;
		bool read_result = ReadFile(handle->com, buffer, length, &bytes_read, &handle->ov_read);
		handle->last_error = GetLastError();
		if (!read_result && handle->last_error != ERROR_IO_PENDING) {
			indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		DWORD waitResult = WaitForSingleObject(handle->ov_read.hEvent, INFINITE);
		if (waitResult == WAIT_OBJECT_0) {
			if (GetOverlappedResult(handle->com, &handle->ov_read, &bytes_read, FALSE)) {
				handle->last_error = 0;
				return bytes_read;
			}
		}
		handle->last_error = GetLastError();
		indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
		CancelIo(handle->com);
		return -1;
	} else if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		long bytes_read = recv(handle->sock, buffer, length, 0);
		if (bytes_read < 0) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		handle->last_error = 0;
		return bytes_read;
	}
	return -1;
#else
#pragma message ("TODO: read_com_data()")
#endif
}

static long write_data(indigo_uni_handle *handle, const char *buffer, long length) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	long bytes_written = write(handle->fd, buffer, length);
	if (bytes_written < 0) {
		handle->last_error = errno;
		indigo_error("%d <- // Failed to write (%s)", handle->index, indigo_uni_strerror(handle));
		return -1;
	}
	handle->last_error = 0;
	return bytes_written;
#elif defined(INDIGO_WINDOWS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		long bytes_written = _write(handle->fd, buffer, length);
		if (bytes_written < 0) {
			handle->last_error = errno;
			indigo_error("%d <- // Failed to write (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		handle->last_error = 0;
		return bytes_written;
	} else if (handle->type == INDIGO_COM_HANDLE) {
		long bytes_written = 0;
		ResetEvent(handle->ov_write.hEvent);
		bool write_result = WriteFile(handle->com, buffer, (DWORD)length, &bytes_written, &handle->ov_write);
		handle->last_error = GetLastError();
		if (!write_result && handle->last_error != ERROR_IO_PENDING) {
			indigo_error("%d <- // Failed to write (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		if (WaitForSingleObject(handle->ov_write.hEvent, 15000) == WAIT_OBJECT_0) {
			if (GetOverlappedResult(handle->com, &handle->ov_write, &bytes_written, FALSE)) {
				handle->last_error = 0;
				return bytes_written;
			}
		}
		handle->last_error = GetLastError();
		indigo_error("%d <- // Failed to write (%s)", handle->index, indigo_uni_strerror(handle));
		return -1;
	} else if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		long bytes_written = send(handle->sock, buffer, length, 0);
		if (bytes_written < 0) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d <- // Failed to write (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		handle->last_error = 0;
		return bytes_written;
	}
	return -1;
#else
#pragma message ("TODO: indigo_uni_write()")
#endif
}

#if defined(INDIGO_WINDOWS)

char* indigo_last_wsa_error() {
	static char buffer[128] = "";
	char* msg = NULL;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, WSAGetLastError(), 0, (LPSTR)&msg, 0, NULL);
	if (msg) {
		strncpy(buffer, msg, sizeof(buffer));
		LocalFree(msg);
		char* lf = strrchr(buffer, '\n');
		if (lf != NULL) {
			*lf = 0;
		}
	}
	return buffer;
}

char* indigo_last_windows_error() {
	static char buffer[128] = "";
	char* msg = NULL;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, (LPSTR)&msg, 0, NULL);
	if (msg) {
		strncpy(buffer, msg, sizeof(buffer));
		LocalFree(msg);
		char* lf = strrchr(buffer, '\r');
		if (lf != NULL) {
			*lf = 0;
		}
	}
	return buffer;
}

const char *indigo_wchar_to_char(const wchar_t *string) {
	static char tmp[1024];
	WideCharToMultiByte(CP_UTF8, 0, string, -1, tmp, sizeof(tmp), NULL, NULL);
	return tmp;
}

const wchar_t *indigo_char_to_wchar(const char *string) {
	static wchar_t tmp[2 * 1024];
	MultiByteToWideChar(CP_UTF8, 0, string, -1, tmp, sizeof(tmp) / 2);
	return tmp;
}

#endif

char *indigo_uni_strerror(indigo_uni_handle *handle) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return "NULL handle";
	}
	static char buffer[128] = "";
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	snprintf(buffer, sizeof(buffer), "%s", strerror(handle->last_error));
#elif defined(INDIGO_WINDOWS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		snprintf(buffer, sizeof(buffer), "%s", strerror(handle->last_error));
	} else if (handle->type == INDIGO_COM_HANDLE || handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		char *msg = NULL;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, handle->last_error, 0, (LPSTR)&msg, 0, NULL);
		if (msg) {
			strncpy(buffer, msg, sizeof(buffer));
			LocalFree(msg);
			char* lf = strrchr(buffer, '\r');
			if (lf != NULL) {
				*lf = 0;
			}
		}
	}
#else
#pragma message ("TODO: indigo_uni_strerror()")
#endif
	return buffer;
}

bool indigo_uni_is_url(const char *name, const char *prefix) {
	if (name == NULL || *name == '\0') {
		return false;
	}
	if (prefix) {
		char prefix_full[INDIGO_NAME_SIZE];
		sprintf(prefix_full, "%s://", prefix);
		return ((!strncmp(name, "tcp://", 6)) || (!strncmp(name, "udp://", 6)) || (!strncmp(name, prefix_full, strlen(prefix_full))));
	}
	return ((!strncmp(name, "tcp://", 6)) || (!strncmp(name, "udp://", 6)));
}

indigo_uni_handle *indigo_uni_open_file(const char *path, int log_level) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int fd = open(path, O_RDONLY);
#elif defined(INDIGO_WINDOWS)
	int fd = _open(path, _O_RDONLY);
#else
#pragma message ("TODO: indigo_uni_open_file()")
#endif
	if (fd >= 0) {
		indigo_uni_handle *handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
		handle->index = handle_index++;
		handle->type = INDIGO_FILE_HANDLE;
		handle->fd = fd;
		handle->log_level = log_level;
		indigo_log_on_level(log_level, "%d <- // %s opened", handle->index, path);
		return handle;
	}
	indigo_log_on_level(log_level, "Failed to open %s", path);
	return NULL;
}

indigo_uni_handle *indigo_uni_create_file(const char *path, int log_level) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#elif defined(INDIGO_WINDOWS)
	int fd = _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);
#else
#pragma message ("TODO: indigo_uni_create_file()")
#endif
	if (fd >= 0) {
		indigo_uni_handle *handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
		handle->index = handle_index++;
		handle->type = INDIGO_FILE_HANDLE;
		handle->fd = fd;
		handle->log_level = log_level;
		indigo_log_on_level(log_level, "%d <- // %s created", handle->index, path);
		return handle;
	}
	indigo_error("Failed to create %s", path);
	return NULL;
}

static int map_baudrate(const char *baudrate) {
#if defined(INDIGO_WINDOWS)
	static int valid_baud_rate[] = { 2400, 4800, 9600, 19200, 38400, 57600, 115200, 0 };
	static int mapped_baud_rate[] = { CBR_2400, CBR_4800, CBR_9600, CBR_19200, CBR_38400, CBR_57600, CBR_115200, 0 };
#else
	static int valid_baud_rate[] = { 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 0 };
	static int mapped_baud_rate [] = { B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, 0 };
#endif
	int br = atoi(baudrate);
	int *vbr = valid_baud_rate;
	int *mbr = mapped_baud_rate;
	while (*vbr) {
		if (*vbr == br) {
			return *mbr;
		}
		vbr++;
		mbr++;
	}
	return -1;
}

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

/* format is 9600-8N1 */

static bool configure_tty_options(struct termios *options, const char *baudrate) {
	int cbits = CS8, cpar = 0, ipar = IGNPAR, bstop = 0;
	int baudr = map_baudrate(baudrate);
	char *mode = strchr(baudrate, '-');
	if (mode == NULL) {
		errno = EINVAL;
		return false;
	}
	mode++;
	if (baudr == -1 || strlen(mode) != 3) {
		errno = EINVAL;
		return false;
	}
	switch (mode[0]) {
		case '8':
			cbits = CS8;
			break;
		case '7':
			cbits = CS7;
			break;
		default :
			errno = EINVAL;
			return false;
			break;
	}
	switch (mode[1]) {
		case 'N': case 'n':
			cpar = 0;
			ipar = IGNPAR;
			break;
		case 'E': case 'e':
			cpar = PARENB;
			ipar = INPCK;
			break;
		case 'O': case 'o':
			cpar = (PARENB | PARODD);
			ipar = INPCK;
			break;
		default :
			errno = EINVAL;
			return false;
			break;
	}
	switch (mode[2]) {
		case '1':
			bstop = 0;
			break;
		case '2':
			bstop = CSTOPB;
			break;
		default :
			errno = EINVAL;
			return false;
			break;
	}
	memset(options, 0, sizeof(*options));
	options->c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
	options->c_iflag = ipar;
	options->c_oflag = 0;
	options->c_lflag = 0;
	options->c_cc[VMIN] = 0;
	options->c_cc[VTIME] = 50;
	cfsetispeed(options, baudr);
	cfsetospeed(options, baudr);
	return true;
}

static indigo_uni_handle *open_tty(const char *serial, const struct termios *options, int log_level) {
	if (!strncmp(serial, "auto://", 7)) {
		serial += 7;
	}
	int fd = open(serial, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		indigo_error("Failed to open %s (%s)", serial, strerror(errno));
		return NULL;
	}
	if (tcsetattr(fd, TCSANOW, options) == -1) {
		indigo_error("Failed to open %s (%s)", serial, strerror(errno));
		close(fd);
		return NULL;
	}
	indigo_uni_handle *handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
	handle->index = handle_index++;
	handle->type = INDIGO_COM_HANDLE;
	handle->fd = fd;
	handle->log_level = log_level;
	indigo_log_on_level(log_level, "%d <- // %s opened", handle->index, serial);
	return handle;
}

#elif defined(INDIGO_WINDOWS)

static int configure_tty_options(DCB *dcb, const char *baudrate) {
	int baudr = map_baudrate(baudrate);
	char *mode = strchr(baudrate, '-');
	if (mode == NULL) {
		errno = EINVAL;
		return false;
	}
	mode++;
	if (baudr == -1 || strlen(mode) != 3) {
		return false;
	}
	dcb->BaudRate = baudr;
	dcb->ByteSize = 8;
	dcb->Parity = NOPARITY;
	dcb->StopBits = ONESTOPBIT;
	dcb->fDtrControl = DTR_CONTROL_ENABLE;
	switch (mode[0]) {
		case '8':
			dcb->ByteSize = 8;
			break;
		case '7':
			dcb->ByteSize = 7;
			break;
		default:
			return false;
	}
	switch (mode[1]) {
		case 'N': case 'n':
			dcb->Parity = NOPARITY;
			break;
		case 'E': case 'e':
			dcb->Parity = EVENPARITY;
			break;
		case 'O': case 'o':
			dcb->Parity = ODDPARITY;
			break;
		default:
			return false;
	}
	switch (mode[2]) {
		case '1':
			dcb->StopBits = ONESTOPBIT;
			break;
		case '2':
			dcb->StopBits = TWOSTOPBITS;
			break;
		default:
			return false;
	}
	return true;
}

static indigo_uni_handle *open_tty(const char *serial, DCB *dcb, int log_level) {
	const char *auto_prefix = "auto://";
	const int auto_prefix_len = sizeof(auto_prefix) - 1;
	const char *serial_buf = serial;
	if (!strncmp(serial_buf, auto_prefix, auto_prefix_len)) {
		serial_buf += auto_prefix_len;
	}
	HANDLE com = CreateFileA(serial_buf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (com == INVALID_HANDLE_VALUE || !SetCommMask(com, EV_RXCHAR)) {
		indigo_error("Failed to open %s (%s)", serial, indigo_last_windows_error());
		return NULL;
	}
	DCB old_dcb;
	memset(&old_dcb, 0, sizeof(DCB));
	old_dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(com, &old_dcb)) {
		indigo_error("Failed to open %s (%s)", serial, indigo_last_windows_error());
		CloseHandle(com);
		return NULL;
	}
	if (!SetCommState(com, dcb)) {
		indigo_error("Failed to open %s (%s)", serial, indigo_last_windows_error());
		CloseHandle(com);
		return NULL;
	}
	if (!SetCommMask(com, EV_RXCHAR)) {
		indigo_error("Failed to open %s (%s)", serial, indigo_last_windows_error());
		CloseHandle(com);
		return NULL;
	}
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = INFINITE;
	timeouts.ReadTotalTimeoutConstant = INFINITE;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = INFINITE;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(com, &timeouts);
	indigo_uni_handle *handle = (indigo_uni_handle *)indigo_safe_malloc(sizeof(indigo_uni_handle));
	handle->index = handle_index++;
	handle->type = INDIGO_COM_HANDLE;
	handle->com = com;
	handle->log_level = log_level;
	handle->ov_read.hEvent = CreateEvent(0, true, false, 0);
	handle->ov_write.hEvent = CreateEvent(0, true, false, 0);
	indigo_log_on_level(log_level, "%d <- // %s opened", handle->index, serial);
	return handle;
}

#endif

indigo_uni_handle *indigo_uni_open_serial_with_config(const char *serial, const char *baudconfig, int log_level) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	struct termios to;
	if (!configure_tty_options(&to, baudconfig)) {
		indigo_error("Failed to open %s (%s)", serial, strerror(errno));
		return NULL;
	}
	return open_tty(serial, &to, log_level);
#elif defined(INDIGO_WINDOWS)
	DCB dcb;
	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	if (!configure_tty_options(&dcb, baudconfig)) {
		indigo_error("Failed to open %s (%s)", serial, indigo_last_windows_error());
		return NULL;
	}
	return open_tty(serial, &dcb, log_level);
#else
#pragma message ("TODO: indigo_open_uni_serial_with_config()")
#endif
}

indigo_uni_handle *indigo_uni_open_serial_with_speed(const char *serial, int speed, int log_level) {
	char baud_str[32];
	snprintf(baud_str, sizeof(baud_str), "%d-8N1", speed);
	return indigo_uni_open_serial_with_config(serial, baud_str, log_level);
}

indigo_uni_handle *indigo_uni_open_serial(const char *serial, int log_level) {
	return indigo_uni_open_serial_with_config(serial, "9600-8N1", log_level);
}

int indigo_uni_set_dtr(indigo_uni_handle *handle, bool state) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type != INDIGO_COM_HANDLE) {
		return -1;
	}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int fc_flag = TIOCM_DTR;
	if (ioctl(handle->fd, state ? TIOCMBIS : TIOCMBIC, &fc_flag) < 0) {
		handle->last_error = errno;
		indigo_error("Failed to %s RTS (%s)", state ? "set" : "clear", indigo_uni_strerror(handle));
		return -1;
	}
#elif defined(INDIGO_WINDOWS)
	if (!EscapeCommFunction(handle->com, state ? SETDTR : CLRDTR)) {
		handle->last_error = GetLastError();
		indigo_error("Failed to %s RTS (%s)", state ? "set" : "clear", indigo_uni_strerror(handle));
		return -1;
	}
#else
#pragma message ("TODO: indigo_uni_set_rts()")
#endif
	return 0;
}

int indigo_uni_set_rts(indigo_uni_handle *handle, bool state) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type != INDIGO_COM_HANDLE) {
		return -1;
	}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int fc_flag = TIOCM_RTS;
	if (ioctl(handle->fd, state ? TIOCMBIS : TIOCMBIC, &fc_flag) < 0) {
		handle->last_error = errno;
		indigo_error("Failed to %s RTS (%s)", state ? "set" : "clear", indigo_uni_strerror(handle));
		return -1;
	}
#elif defined(INDIGO_WINDOWS)
	if (!EscapeCommFunction(handle->com, state ? SETRTS : CLRRTS)) {
		handle->last_error = GetLastError();
		indigo_error("Failed to %s RTS (%s)", state ? "set" : "clear", indigo_uni_strerror(handle));
		return -1;
	}
#else
#pragma message ("TODO: indigo_uni_set_rts()")
#endif
	return 0;
}

int indigo_uni_set_cts(indigo_uni_handle *handle, bool state) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type != INDIGO_COM_HANDLE) {
		return -1;
	}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int fc_flag = TIOCM_CTS;
	if (ioctl(handle->fd, state ? TIOCMBIS : TIOCMBIC, &fc_flag) < 0) {
		handle->last_error = errno;
		indigo_error("Failed to %s CTS (%s)", state ? "set" : "clear", indigo_uni_strerror(handle));
		return -1;
	}
#elif defined(INDIGO_WINDOWS)
	return -1;
#else
#pragma message ("TODO: indigo_uni_set_cts()")
#endif
	return 0;
}

indigo_uni_handle *indigo_uni_open_client_socket(const char *host, int port, int type, int log_level) {
	indigo_uni_handle *handle = NULL;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	struct addrinfo *address_list, *address;
	if (getaddrinfo(host, NULL, NULL, &address_list) == 0) {
		for (address = address_list; address != NULL; address = address->ai_next) {
			int fd = socket(AF_INET, type, 0);
			if (fd == -1) {
				indigo_error("Can't create %s socket for '%s:%d' (%s)", type == SOCK_STREAM ? "TCP" : "UDP", host, port, strerror(errno));
				break;
			}
			*(uint16_t *)(address->ai_addr->sa_data) = htons(port);
			if (connect(fd, address->ai_addr, address->ai_addrlen) == 0) {
				handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
				handle->index = handle_index++;
				handle->type = type == SOCK_STREAM ? INDIGO_TCP_HANDLE : INDIGO_UDP_HANDLE;
				handle->fd = fd;
				handle->log_level = log_level;
				freeaddrinfo(address_list);
				indigo_log_on_level(log_level, "%d <- // %s socket connected to '%s:%d'", handle->index, type == SOCK_STREAM ? "TCP" : "UDP", host, port);
				return handle;
			}
			close(fd);
		}
	}
	indigo_error("Can't connect %s socket for '%s:%d' (%s)", type == SOCK_STREAM ? "TCP" : "UDP", host, port, strerror(errno));
	freeaddrinfo(address_list);
	return handle;
#elif defined(INDIGO_WINDOWS)
	struct hostent *he = gethostbyname(host);
	if (he == NULL) {
		return NULL;
	}
	SOCKET sock = socket(AF_INET, type, 0);
	if (sock == INVALID_SOCKET) {
		return NULL;
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr = *((struct in_addr *)he->h_addr);
	if (connect(sock, (struct sockaddr *)&address, sizeof(struct sockaddr)) == 0) {
		handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
		handle->index = handle_index++;
		handle->type = type == SOCK_STREAM ? INDIGO_TCP_HANDLE : INDIGO_UDP_HANDLE;
		handle->sock = sock;
		handle->log_level = log_level;
		indigo_log_on_level(log_level, "%d <- // %s socket connected to '%s:%d'", handle->index, type == SOCK_STREAM ? "TCP" : "UDP", host, port);
		return handle;
	}
	indigo_error("Can't connect %s socket for '%s:%d'", type == SOCK_STREAM ? "TCP" : "UDP", host, port);
	closesocket(sock);
	return NULL;
#else
#pragma message ("TODO: indigo_uni_open_client_socket()")
#endif
}

indigo_uni_handle *indigo_uni_open_url(const char *url, int default_port, indigo_uni_handle_type protocol_hint, int log_level) {
	int port = default_port;
	char host_name[INDIGO_NAME_SIZE];
	if (url == NULL || *url == 0) {
		indigo_debug("Empty URL");
		return NULL;
	}
	char *found = strstr(url, "://");
	if (found == NULL) {
		indigo_debug("Malformed URL '%s'", url);
		return NULL;
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
		INDIGO_COPY_NAME(host_name, host);
	}
	switch (protocol_hint) {
		case INDIGO_TCP_HANDLE:
			return indigo_uni_client_tcp_socket(host_name, port, log_level);
		case INDIGO_UDP_HANDLE:
			return indigo_uni_client_udp_socket(host_name, port, log_level);
		default:
			return NULL;
	}
}

void indigo_uni_open_tcp_server_socket(int *port, indigo_uni_handle **server_handle, void (*worker)(indigo_uni_worker_data *), void *data, void (*callback)(int), int log_level) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		indigo_error("Can't create server socket (%s)", strerror(errno));
		return;
	}
	int reuse = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		indigo_error("Can't setsockopt for server socket (%s)", strerror(errno));
		close(server_socket);
		return;
	}
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(*port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	unsigned int length = sizeof(server_address);
	if (bind(server_socket, (struct sockaddr *)&server_address, length) < 0) {
		indigo_error("Can't bind server socket (%s)", strerror(errno));
		close(server_socket);
		return;
	}
#if defined(INDIGO_LINUX)
	int val = 1;
	if (setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0) {
		close(server_socket);
		indigo_error("Can't setsockopt TCP_NODELAY, for server socket (%s)", strerror(errno));
		return;
	}
#endif
	if (getsockname(server_socket, (struct sockaddr *)&server_address, &length) == -1) {
		indigo_error("Can't get socket name (%s)", strerror(errno));
		close(server_socket);
		return;
	}
	*port = ntohs(server_address.sin_port);
	if (listen(server_socket, 64) < 0) {
		indigo_error("Can't listen on server socket (%s)", strerror(errno));
		close(server_socket);
		return;
	}
	*server_handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
	(*server_handle)->index = handle_index++;
	(*server_handle)->type = INDIGO_TCP_HANDLE;
	(*server_handle)->fd = server_socket;
	(*server_handle)->log_level = log_level;
	indigo_log_on_level(log_level, "Server started on TCP port %d", *port);
	if (callback) {
		callback(0);
	}
	while (true) {
		struct sockaddr_in client_name;
		unsigned int name_len = sizeof(client_name);
		int client_socket = accept(server_socket, (struct sockaddr *)&client_name, &name_len);
		if (client_socket == -1) {
			if (*server_handle == NULL) {
				break;
			}
			indigo_error("Can't accept connection (%s)", strerror(errno));
			continue;;
		}
		struct timeval timeout = { 0 };
		if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			indigo_error("Can't set recv() timeout (%s)", strerror(errno));
			close(client_socket);
			break;
		}
		timeout.tv_sec = 5;
		if (setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			indigo_error("Can't set send() timeout (%s)", strerror(errno));
			close(client_socket);
			break;
		}
		indigo_uni_worker_data *worker_data = indigo_safe_malloc(sizeof(indigo_uni_worker_data));
		worker_data->handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
		worker_data->handle->index = handle_index++;
		worker_data->handle->type = INDIGO_TCP_HANDLE;
		worker_data->handle->fd = client_socket;
		worker_data->handle->log_level = log_level;
		worker_data->data = data;
		if (!indigo_async((void *(*)(void *))worker, worker_data)) {
			indigo_error("Can't create worker thread for connection (%s)", strerror(errno));
		}
	}
	indigo_log_on_level(log_level, "Server stopped on TCP port %d", *port);
#elif defined(INDIGO_WINDOWS)
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		indigo_error("Can't create server socket (%s)", indigo_last_wsa_error());
		return;
	}
	int reuse = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
		indigo_error("Can't setsockopt for server socket (%d)",  indigo_last_wsa_error());
		closesocket(server_socket);
		return;
	}
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons((unsigned short)*port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	int addrlen = sizeof(server_address);
	if (bind(server_socket, (struct sockaddr*)&server_address, addrlen) == SOCKET_ERROR) {
		indigo_error("Can't bind server socket (%d)",  indigo_last_wsa_error());
		closesocket(server_socket);
		return;
	}
	if (getsockname(server_socket, (struct sockaddr*)&server_address, &addrlen) == SOCKET_ERROR) {
		indigo_error("Can't get socket name (%d)",  indigo_last_wsa_error());
		closesocket(server_socket);
		return;
	}
	*port = ntohs(server_address.sin_port);
	if (listen(server_socket, 64) == SOCKET_ERROR) {
		indigo_error("Can't listen on server socket (%d)",  indigo_last_wsa_error());
		closesocket(server_socket);
		return;
	}
	*server_handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
	(*server_handle)->index = handle_index++;
	(*server_handle)->type = INDIGO_TCP_HANDLE;
	(*server_handle)->sock = server_socket;
	(*server_handle)->log_level = log_level;
	INDIGO_LOG(indigo_log("Server started on TCP port %d", *port));
	if (callback) {
		callback(0);
	}
	while (1) {
		struct sockaddr_in client_addr;
		int client_addr_len = sizeof(client_addr);
		SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_socket == INVALID_SOCKET) {
			if (*server_handle == NULL) {
				break;
			}
			indigo_error("Can't accept connection (%d)",  indigo_last_wsa_error());
			continue;
		}
		DWORD recv_timeout = 0;
		if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
			(const char*)&recv_timeout, sizeof(recv_timeout)) == SOCKET_ERROR) {
			indigo_error("Can't set recv() timeout (%d)",  indigo_last_wsa_error());
			closesocket(client_socket);
			break;
		}
		DWORD send_timeout = 5000;
		if (setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO,
			(const char*)&send_timeout, sizeof(send_timeout)) == SOCKET_ERROR) {
			indigo_error("Can't set send() timeout (%d)",  indigo_last_wsa_error());
			closesocket(client_socket);
			break;
		}
		indigo_uni_worker_data* worker_data = indigo_safe_malloc(sizeof(indigo_uni_worker_data));
		worker_data->handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
		worker_data->handle->index = handle_index++;
		worker_data->handle->type = INDIGO_TCP_HANDLE;
		worker_data->handle->sock = client_socket;
		worker_data->handle->log_level = log_level;
		worker_data->data = data;
		if (!indigo_async((void* (*)(void*))worker, worker_data)) {
			indigo_error("Can't create worker thread for connection (%d)",  indigo_last_wsa_error());
		}
	}
#else
#pragma message ("TODO: indigo_uni_open_server_socket()")
#endif
}

void indigo_uni_set_socket_read_timeout(indigo_uni_handle *handle, long timeout) {
	if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		struct timeval tv = { .tv_sec = timeout / 1000000L, .tv_usec = timeout % 1000000L };
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (setsockopt(handle->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
			handle->last_error = errno;
			indigo_error("%d <- // Failed to set socket read timeout (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			indigo_log_on_level(handle->log_level, "%d <- // Set socket read timeout %ld", handle->index, timeout);
		}
#elif defined(INDIGO_WINDOWS)
		if (setsockopt(handle->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == SOCKET_ERROR) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d <- // Failed to set socket read timeout (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			indigo_log_on_level(handle->log_level, "%d <- // Set socket read timeout %ld", handle->index, timeout);
		}
#else
#pragma message ("TODO: indigo_uni_set_socket_read_timeout()")
#endif
	}
}

void indigo_uni_set_socket_write_timeout(indigo_uni_handle *handle, long timeout) {
	if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		struct timeval tv = { .tv_sec = timeout / 1000000L, .tv_usec = timeout % 1000000L };
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (setsockopt(handle->fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv) != 0) {
			handle->last_error = errno;
			indigo_error("%d <- // Failed to set socket write timeout (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			indigo_log_on_level(handle->log_level, "%d <- // Set socket write timeout %ld", handle->index, timeout);
		}
#elif defined(INDIGO_WINDOWS)
		if (setsockopt(handle->sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv) == SOCKET_ERROR) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d <- // Failed to set socket write timeout (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			indigo_log_on_level(handle->log_level, "%d <- // Set socket write timeout %ld", handle->index, timeout);
		}
#else
#pragma message ("TODO: indigo_uni_set_socket_write_timeout()")
#endif
	}
}

void indigo_uni_set_socket_nodelay_option(indigo_uni_handle *handle) {
	if (handle->type == INDIGO_TCP_HANDLE) {
		int value = 1;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (setsockopt(handle->fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(int)) != 0) {
			handle->last_error = errno;
			indigo_error("%d <- // Failed to set socket no delay option (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			indigo_log_on_level(handle->log_level, "%d <- // Set socket no delay %d", handle->index, value);
		}
#elif defined(INDIGO_WINDOWS)
		if (setsockopt(handle->sock, IPPROTO_TCP, TCP_NODELAY, (const char *) & value, sizeof(int)) == SOCKET_ERROR) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d <- // Failed to set socket no delay option (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
			indigo_log_on_level(handle->log_level, "%d <- // Set socket no delay %d", handle->index, value);
		}
#else
#pragma message ("TODO: indigo_uni_set_socket_nodelay_option()")
#endif
	}
}

long indigo_uni_read_available(indigo_uni_handle *handle, void *buffer, long length) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	long bytes_read = read_data(handle, buffer, length);
	if (bytes_read > 0) {
		if (handle->log_level < 0) {
			indigo_log_on_level(-handle->log_level, "%d -> // %ld bytes read", handle->index, bytes_read);
		} else {
			indigo_log_on_level(handle->log_level, "%d -> %.*s", handle->index, bytes_read, buffer);
		}
	}
	return bytes_read;
}

long indigo_uni_peek_available(indigo_uni_handle *handle, void *buffer, long length) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	long bytes_read = -1;
	if (handle->type == INDIGO_TCP_HANDLE) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		bytes_read = recv(handle->fd, buffer, length, MSG_PEEK);
		if (bytes_read < 0) {
			handle->last_error = errno;
			indigo_error("%d -> // Failed to peek (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		bytes_read = recv(handle->sock, buffer, length, MSG_PEEK);
		if (bytes_read < 0) {
			handle->last_error = WSAGetLastError();
			indigo_error("%d -> // Failed to peek (%s)", handle->index, indigo_uni_strerror(handle));
		} else {
			handle->last_error = 0;
		}
#else
#pragma message ("TODO: indigo_uni_peek_available()")
#endif
	}
	return bytes_read;
}

long indigo_uni_read(indigo_uni_handle *handle, void *buffer, long length) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type == INDIGO_UDP_HANDLE) {
		return indigo_uni_read_available(handle, buffer, length);
	}
	long remaining = length;
	char *pnt = (char *)buffer;
	while (true) {
		long bytes_read = read_data(handle, pnt, remaining);
		if (bytes_read < 0) {
			indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		remaining -= bytes_read;
		pnt += bytes_read;
		if (remaining == 0) {
			if (handle->log_level < 0) {
				indigo_log_on_level(-handle->log_level, "%d -> // %ld bytes read", handle->index, length);
			} else {
				indigo_log_on_level(handle->log_level, "%d -> %.*s", handle->index, length, buffer);
			}
			return length;
		}
	}
}

long indigo_uni_discard(indigo_uni_handle *handle) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type == INDIGO_FILE_HANDLE) {
		return 0;
	} else if (handle->type == INDIGO_COM_HANDLE) {
		return discard_data(handle);
	}
	char c;
	long bytes_read = 0;
	while (true) {
		if (wait_for_data(handle, 10000) <= 0) {
			break;
		}
		if (read_data(handle, &c, 1) <= 0) {
			indigo_error("%d -> // Failed to discard (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		bytes_read++;
	}
	indigo_log_on_level(handle->log_level, "%d <- // %ld bytes discarded", handle->index, bytes_read);
	return bytes_read;
}

long indigo_uni_read_section(indigo_uni_handle *handle, char *buffer, long length, const char *terminators, const char *ignore, long timeout) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	long bytes_read = 0;
	bool terminated = false;
	while (bytes_read < length) {
		char c = 0;
		if (timeout >= 0) {
			switch (wait_for_data(handle, timeout)) {
				case -1:
					return -1;
				case 0:
					if (bytes_read > 0) {
						if (handle->log_level < 0) {
							indigo_log_on_level(-handle->log_level, "%d -> // %ld bytes read", handle->index, bytes_read - 1);
						} else {
							indigo_log_on_level(handle->log_level, "%d -> %.*s", handle->index, bytes_read, buffer);
						}
					}
					buffer[bytes_read] = 0;
					return bytes_read;
				default:
					break;
			}
		}
		switch (read_data(handle, &c, 1)) {
			case -1:
				indigo_error("%d -> // Failed to read (%s)", handle->index, indigo_uni_strerror(handle));
				return -1;
			case 0:
				if (handle->log_level < 0) {
					indigo_log_on_level(-handle->log_level, "%d -> // %ld bytes read", handle->index, bytes_read - 1);
				} else {
					indigo_log_on_level(handle->log_level, "%d -> %.*s", handle->index, bytes_read, buffer);
				}
				return 0;
			default:
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
			buffer[bytes_read] = 0;
			break;
		}
		timeout = -1;
	}
	if (handle->log_level < 0) {
		indigo_log_on_level(-handle->log_level, "%d -> // %ld bytes read", handle->index, bytes_read);
	} else {
		indigo_log_on_level(handle->log_level, "%d -> %.*s", handle->index, bytes_read, buffer);
	}
	return bytes_read;
}

int indigo_uni_scanf_line(indigo_uni_handle *handle, const char *format, ...) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
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

long indigo_uni_write(indigo_uni_handle *handle, const char *buffer, long length) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type == INDIGO_UDP_HANDLE) {
		long bytes_written = write_data(handle, buffer, length);
		if (handle->log_level < 0) {
			indigo_log_on_level(-handle->log_level, "%d <- // %ld bytes written", handle->index, length);
		} else {
			indigo_log_on_level(handle->log_level, "%d <- %.*s", handle->index, length, buffer);
		}
		return bytes_written;
	}
	long remaining = length;
	char *pnt = (char *)buffer;
	while (true) {
		long bytes_written = write_data(handle, buffer, remaining);
		if (bytes_written < 0) {
			indigo_error("%d -> // Failed to write (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		remaining -= bytes_written;
		pnt += bytes_written;
		if (remaining == 0) {
			if (handle->log_level < 0) {
				indigo_log_on_level(-handle->log_level, "%d <- // %ld bytes written", handle->index, length);
			} else {
				indigo_log_on_level(handle->log_level, "%d <- %.*s", handle->index, length, buffer);
			}
			return length;
		}
	}
}

long indigo_uni_printf(indigo_uni_handle *handle, const char *format, ...) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (strchr(format, '%')) {
		char *buffer = indigo_alloc_large_buffer();
		va_list args;
		va_start(args, format);
		int length = vsnprintf(buffer, INDIGO_BUFFER_SIZE, format, args);
		va_end(args);
		long result = indigo_uni_write(handle, buffer, length);
		indigo_free_large_buffer(buffer);
		return result;
	} else {
		return indigo_uni_write(handle, format, (long)strlen(format));
	}
}

long indigo_uni_vprintf(indigo_uni_handle *handle, const char *format, va_list args) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (strchr(format, '%')) {
		char *buffer = indigo_alloc_large_buffer();
		int length = vsnprintf(buffer, INDIGO_BUFFER_SIZE, format, args);
		long result = indigo_uni_write(handle, buffer, length);
		indigo_free_large_buffer(buffer);
		return result;
	} else {
		return indigo_uni_write(handle, format, (long)strlen(format));
	}
}

long indigo_uni_vtprintf(indigo_uni_handle *handle, const char *format, va_list args, char *terminator) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	long result;
	if (strchr(format, '%')) {
		char *buffer = indigo_alloc_large_buffer();
		int length = vsnprintf(buffer, INDIGO_BUFFER_SIZE, format, args);
		result = indigo_uni_write(handle, buffer, length);
		indigo_free_large_buffer(buffer);
	} else {
		result = indigo_uni_write(handle, format, (long)strlen(format));
	}
	if (result >= 0) {
		indigo_log_levels saved = handle->log_level;
		handle->log_level = INDIGO_LOG_NONE;
		result += indigo_uni_write(handle, terminator, strlen(terminator));
		handle->log_level = saved;
	}
	return result;
}

long indigo_uni_seek(indigo_uni_handle *handle, long position, int whence) {
	if (handle->type == INDIGO_FILE_HANDLE) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		long result = lseek(handle->fd, position, whence);
#elif defined(INDIGO_WINDOWS)
		long result = _lseek((int)handle->fd, position, whence);
#else
#pragma message ("TODO: indigo_uni_seek()")
#endif
		if (result < 0) {
			handle->last_error = errno;
			indigo_error("%d <- // Failed to seek (%s)", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		return result;
	}
	return -1;
}

bool indigo_uni_lock_file(indigo_uni_handle *handle) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return -1;
	}
	if (handle->type == INDIGO_FILE_HANDLE) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		static struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();
		int ret = fcntl(handle->fd, F_SETLK, &lock);
		if (ret == -1) {
			handle->last_error = errno;
			indigo_error("%d <- //Failed to lock %s", handle->index, indigo_uni_strerror(handle));
			return false;
		}
#elif defined(INDIGO_WINDOWS)
		HANDLE hFile = (HANDLE)_get_osfhandle(handle->fd);
		if (hFile == INVALID_HANDLE_VALUE) {
			handle->last_error = GetLastError();
			indigo_error("%d <- //Failed to lock %s", handle->index, indigo_uni_strerror(handle));
			return false;
		}
		if (!LockFile(hFile, 0, 0, MAXDWORD, MAXDWORD)) {
			handle->last_error = GetLastError();
			indigo_error("%d <- //Failed to lock %s", handle->index, indigo_uni_strerror(handle));
			return false;
		}
#else
#pragma message ("TODO: indigo_uni_lock_file()")
#endif
		indigo_log_on_level(handle->log_level, "%d <- // locked", handle->index);
		return true;
	}
	return false;
}

void indigo_uni_close(indigo_uni_handle **handle) {
	if (handle == NULL) {
		// indigo_error("%s used with NULL handle", __FUNCTION__);
		return;
	}
	pthread_mutex_lock(&mutex);
	if (*handle) {
		indigo_uni_handle *copy = *handle;
		*handle = NULL;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if ((copy)->type != INDIGO_FILE_HANDLE) {
			shutdown((copy)->fd, SHUT_RDWR);
			indigo_sleep(1);
		}
		close((copy)->fd);
#elif defined(INDIGO_WINDOWS)
		if ((copy)->type == INDIGO_FILE_HANDLE) {
			_close((copy)->fd);
		} else if ((copy)->type == INDIGO_COM_HANDLE) {
			CloseHandle((copy)->ov_read.hEvent);
			CloseHandle((copy)->ov_write.hEvent);
			CloseHandle((copy)->com);
		} else if ((copy)->type == INDIGO_TCP_HANDLE || (copy)->type == INDIGO_UDP_HANDLE) {
			shutdown((copy)->sock, SD_BOTH);
			closesocket((copy)->sock);
		}
#else
#pragma message ("TODO: indigo_uni_close()")
#endif
		indigo_log_on_level((copy)->log_level, "%d <- // closed", (copy)->index);
		indigo_safe_free(copy);
	}
	pthread_mutex_unlock(&mutex);
}

const char* indigo_uni_home_folder(void) {
	static char home_folder[512] = { 0 };
	if (home_folder[0] == 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		snprintf(home_folder, sizeof(home_folder), "%s", getenv("HOME"));
#elif defined(INDIGO_WINDOWS)
		const char* userprofile = getenv("USERPROFILE");
		if (userprofile) {
			snprintf(home_folder, sizeof(home_folder), "%s", userprofile);
		} else {
			snprintf(home_folder, sizeof(home_folder), "C:\\Users\\Default");
		}
#else
#pragma message ("TODO: indigo_uni_home_folder()")
#endif
	}
	return home_folder;
}

const char *indigo_uni_config_folder(void) {
	static char config_folder[512] = { 0 };
	if (config_folder[0] == 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		snprintf(config_folder, sizeof(config_folder), "%s/.indigo", getenv("HOME"));
#elif defined(INDIGO_WINDOWS)
		const char* appdata = getenv("APPDATA");
		if (appdata) {
			snprintf(config_folder, sizeof(config_folder), "%s\\indigo", appdata);
		} else {
			const char* userprofile = getenv("USERPROFILE");
			if (userprofile) {
				snprintf(config_folder, sizeof(config_folder), "%s\\.indigo", userprofile);
			} else {
				snprintf(config_folder, sizeof(config_folder), "C:\\Users\\Default\\.indigo");
			}
		}
#else
#pragma message ("TODO: indigo_uni_config_folder()")
#endif
	}
	return config_folder;
}

bool indigo_uni_mkdir(const char *path) {
	char temp[PATH_MAX];
	snprintf(temp, sizeof(temp), "%s", path);
	char *p = temp;
#if defined(INDIGO_WINDOWS)
	if (strlen(p) > 2 && p[1] == ':') {
		p += 2;
	}
#endif
	if (*p == INDIGO_PATH_SEPATATOR || *p == '/') {
		*p++ = INDIGO_PATH_SEPATATOR;
	}
	for (; *p; p++) {
		if (*p == INDIGO_PATH_SEPATATOR || *p == '/') {
			*p = '\0';
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
			if (mkdir(temp, 0777) != 0 && errno != EEXIST) {
				return false;
			}
#elif defined(INDIGO_WINDOWS)
			if (_mkdir(temp) != 0 && errno != EEXIST) {
				return false;
			}
#else
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
			*p++ = INDIGO_PATH_SEPATATOR;
		}
	}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (mkdir(temp, 0777) != 0 && errno != EEXIST) {
		return false;
	}
#elif defined(INDIGO_WINDOWS)
	if (_mkdir(temp) != 0 && errno != EEXIST) {
		return false;
	}
#else
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
	return true;
}

bool indigo_uni_remove(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (unlink(path) == 0) {
		return true;
	}
#elif defined(INDIGO_WINDOWS)
	if (_unlink(path) == 0) {
		return true;
	}
#else
#pragma message ("TODO: indigo_uni_remove()")
#endif
	return false;
}

bool indigo_uni_is_readable(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (access(path, R_OK) == 0) {
		return true;
	}
#elif defined(INDIGO_WINDOWS)
	if (_access(path, 4) == 0) {
		return true;
	}
#else
#pragma message ("TODO: indigo_uni_is_readable()")
#endif
	return false;
}

bool indigo_uni_is_writable(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (access(path, W_OK) == 0) {
		return true;
	}
#elif defined(INDIGO_WINDOWS)
	if (_access(path, 2) == 0) {
		return true;
	}
#else
#pragma message ("TODO: indigo_uni_is_writable()")
#endif
	return false;
}

char* indigo_uni_realpath(const char* path, char *resolved_path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	return realpath(path, resolved_path);
#elif defined(INDIGO_WINDOWS)
	return _fullpath(resolved_path, path, MAX_PATH);
#else
#pragma message ("TODO: indigo_uni_realpath()")
#endif
}

char *indigo_uni_basename(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	return basename((char *)path);
#elif defined(INDIGO_WINDOWS)
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	static char result[_MAX_FNAME + _MAX_EXT] = { 0 };
	if (_splitpath_s(path, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), ext, sizeof(ext)) == 0) {
		snprintf(result, sizeof(result), "%s%s", fname, ext);
	}
	return result;
#else
#pragma message ("TODO: indigo_uni_basename()")
#endif
}

int indigo_uni_scandir(const char* folder, char ***list, bool (*filter)(const char *)) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int result = -1;
	struct dirent **entries;
	int count = scandir(folder, &entries, NULL, alphasort);
	if (count >= 0) {
		result = 0;
		*list = (char **)indigo_safe_malloc(sizeof(const char *) * count);
		for (int i = 0; i < count; i++) {
			if (filter == NULL || filter(entries[i]->d_name)) {
				(*list)[result++] = strdup(entries[i]->d_name);
			}
			indigo_safe_free(entries[i]);
		}
		indigo_safe_free(entries);
	}
	return result;
#elif defined(INDIGO_WINDOWS)
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char searchPath[MAX_PATH];
	int result = 0;
	*list = NULL;
	snprintf(searchPath, MAX_PATH, "%s\\*", folder);
	hFind = FindFirstFileA((LPCSTR)searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return -1;
	}
	int list_size = 10;
	*list = (char**)indigo_safe_malloc(sizeof(char*) * list_size);
	do {
		if (filter((const char *)findFileData.cFileName)) {
			if (result >= list_size) {
				list_size *= 2;
				*list = (char**)indigo_safe_realloc(*list, sizeof(char*) * list_size);
			}
			(*list)[result++] = _strdup((const char *)findFileData.cFileName);
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);
	return result;
#else
#pragma message ("TODO: indigo_uni_basename()")
#endif
}

void indigo_uni_compress(char *name, char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size) {
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

void indigo_uni_decompress(char *in_buffer, unsigned in_size, unsigned char *out_buffer, unsigned *out_size) {
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

void indigo_gmtime(time_t* seconds, struct tm* tm) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	gmtime_r(seconds, tm);
#elif defined(INDIGO_WINDOWS)
	gmtime_s(tm, seconds);
#else
#pragma message ("TODO: indigo_gmtime()")
#endif
}

time_t indigo_timegm(struct tm* tm) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	return timegm(tm);
#elif defined(INDIGO_WINDOWS)
	return _mkgmtime(tm);
#else
#pragma message ("TODO: indigo_timegm()")
#endif
}
