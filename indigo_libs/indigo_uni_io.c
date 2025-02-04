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
#include <sys/stat.h>

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#include <termios.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>
#elif defined(INDIGO_WINDOWS)
#include <io.h>
#include <direct.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#pragma message ("TODO: Unified I/O")
#endif
#ifdef INDIGO_LINUX
#include <netinet/tcp.h>
#endif

#include <indigo/indigo_bus.h>
#include <indigo/indigo_uni_io.h>

static int handle_index = 0;

indigo_uni_handle indigo_stdin_handle = { INDIGO_FILE_HANDLE, 0, 0 };
indigo_uni_handle indigo_stdout_handle = { INDIGO_FILE_HANDLE, 1, 0 };
indigo_uni_handle indigo_stderr_handle = { INDIGO_FILE_HANDLE, 2, 0 };

indigo_uni_handle *indigo_uni_create_file_handle(int fd) {
	indigo_uni_handle *handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
	handle->index = handle_index++;
	handle->type = INDIGO_FILE_HANDLE;
	handle->fd = fd;
	return handle;
}

char *indigo_uni_strerror(indigo_uni_handle *handle) {
	static char buffer[128] = "";
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	snprintf(buffer, sizeof(buffer), "%s", strerror(handle->last_error));
#elif defined(INDIGO_WINDOWS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		snprintf(buffer, sizeof(buffer), "%s", strerror(handle->last_error));
	} else if (handle->type == INDIGO_COM_HANDLE || handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		char *msg = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, handle->last_error, 0, (LPWSTR)&msg, 0, NULL);
		if (msg) {
			strncpy(buffer, msg, sizeof(buffer));
			LocalFree(msg);
		}
	}
#else
#pragma message ("TODO: indigo_uni_strerror()")
#endif
	return buffer;
}

indigo_uni_handle *indigo_uni_open_file(const char *path) {
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
		return handle;
	}
	return NULL;
}

indigo_uni_handle *indigo_uni_create_file(const char *path) {
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
		return handle;
	}
	return NULL;
}

static int map_str_baudrate(const char *baudrate) {
	static int valid_baud_rate [] = { 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 0 };
	int br = atoi(baudrate);
	for (int *vbr = valid_baud_rate; *vbr; vbr++) {
		if (*vbr == br)
			return br;
	}
	return -1;
}

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)

/* format is 9600-8N1 */

static int configure_tty_options(struct termios *options, const char *baudrate) {
	int cbits = CS8, cpar = 0, ipar = IGNPAR, bstop = 0, baudr = 0;
	char copy[32];
	strncpy(copy, baudrate, sizeof(copy));
	char *mode = strchr(copy, '-');
	if (mode == NULL) {
		errno = EINVAL;
		return -1;
	}
	*mode++ = '\0';
	baudr = map_str_baudrate(copy);
	if (baudr == -1 || strlen(mode) != 3) {
		errno = EINVAL;
		return -1;
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
			return -1;
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
			return -1;
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
			return -1;
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
	return 0;
}

static indigo_uni_handle *open_tty(const char *tty_name, const struct termios *options, struct termios *old_options) {
	const char auto_prefix[] = "auto://";
	const int auto_prefix_len = sizeof(auto_prefix) - 1;
	const char *tty_name_buf = tty_name;
	if (!strncmp(tty_name_buf, auto_prefix, auto_prefix_len)) {
		tty_name_buf += auto_prefix_len;
	}
	int fd = open(tty_name_buf, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		return NULL;
	}
	if (old_options) {
		if (tcgetattr(fd, old_options) == -1) {
			close(fd);
			return NULL;
		}
	}
	if (tcsetattr(fd, TCSANOW, options) == -1) {
		close(fd);
		return NULL;
	}
	indigo_uni_handle *handle = indigo_safe_malloc(sizeof(indigo_uni_handle));
	handle->index = handle_index++;
	handle->type = INDIGO_COM_HANDLE;
	handle->fd = fd;
	return handle;
}

#elif defined(INDIGO_WINDOWS)

static int configure_tty_options(DCB *dcb, const char *baudrate) {
	char copy[32];
	strncpy(copy, baudrate, sizeof(copy));
	char *mode = strchr(copy, '-');
	if (mode == NULL) {
		return -1;
	}
	*mode++ = '\0';
	int baudr = map_str_baudrate(copy);
	if (baudr == -1 || strlen(mode) != 3) {
		return -1;
	}
	dcb->BaudRate = baudr;
	dcb->ByteSize = 8;
	dcb->Parity = NOPARITY;
	dcb->StopBits = ONESTOPBIT;
	switch (mode[0]) {
		case '8':
			dcb->ByteSize = 8;
			break;
		case '7':
			dcb->ByteSize = 7;
			break;
		default:
			return -1;
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
			return -1;
	}
	switch (mode[2]) {
		case '1':
			dcb->StopBits = ONESTOPBIT;
			break;
		case '2':
			dcb->StopBits = TWOSTOPBITS;
			break;
		default:
			return -1;
	}
	return 0;
}

static indigo_uni_handle *open_tty(const char *tty_name, DCB *dcb) {
	const char auto_prefix[] = "auto://";
	const int auto_prefix_len = sizeof(auto_prefix) - 1;
	const char *tty_name_buf = tty_name;
	if (!strncmp(tty_name_buf, auto_prefix, auto_prefix_len)) {
		tty_name_buf += auto_prefix_len;
	}
	HANDLE com = CreateFileA(tty_name_buf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (com == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	DCB old_dcb;
	memset(&old_dcb, 0, sizeof(DCB));
	old_dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(com, &old_dcb)) {
		CloseHandle(com);
		return NULL;
	}
	if (!SetCommState(com, dcb)) {
		CloseHandle(com);
		return NULL;
	}
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	SetCommTimeouts(hComm, &timeouts);
	indigo_uni_handle *handle = (indigo_uni_handle *)malloc(sizeof(indigo_uni_handle));
	handle->index = handle_index++;
	handle->type = INDIGO_COM_HANDLE;
	handle->com = com;
	return handle;
}
#endif

indigo_uni_handle *indigo_open_uni_serial_with_config(const char *serial, const char *baudconfig) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	struct termios to;
	if (configure_tty_options(&to, baudconfig) != 0) {
		return NULL;
	}
	return open_tty(serial, &to, NULL);
#elif defined(INDIGO_WINDOWS)
	DCB dcb;
	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	if (configure_tty_options(&dcb, baudconfig) != 0) {
		return -1;
	}
	return open_tty(serial, &dcb);
#else
#pragma message ("TODO: indigo_open_uni_serial_with_config()")
#endif
}

indigo_uni_handle *indigo_uni_open_serial_with_speed(const char *serial, int speed) {
	char baud_str[32];
	snprintf(baud_str, sizeof(baud_str), "%d-8N1", speed);
	return indigo_open_uni_serial_with_config(serial, baud_str);
}

indigo_uni_handle *indigo_uni_open_serial(const char *serial) {
	return indigo_open_uni_serial_with_config(serial, "9600-8N1");
}

indigo_uni_handle *indigo_uni_open_client_socket(const char *host, int port, int type) {
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
				freeaddrinfo(address_list);
				INDIGO_DEBUG(indigo_debug("%d <- // %s socket connected to '%s:%d'", handle->index, type == SOCK_STREAM ? "TCP" : "UDP", host, port));
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
		INDIGO_DEBUG(indigo_debug("%d <- // %s socket connected to '%s:%d'", handle->index, type == SOCK_STREAM ? "TCP" : "UDP", host, port));
		return handle;
	}
	indigo_error("Can't connect %s socket for '%s:%d'", type == SOCK_STREAM ? "TCP" : "UDP", host, port);
	closesocket(sock);
	return NULL;
#else
#pragma message ("TODO: indigo_uni_open_client_socket()")
#endif
}

void indigo_uni_open_tcp_server_socket(int *port, indigo_uni_handle **server_handle,void (*worker)(indigo_uni_worker_data *), void *data, void (*callback)(int)) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		indigo_error("Can't create server socket (%s)", strerror(errno));
		return;
	}
	int reuse = 1;
	if (setsockopt(server_socket, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
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
	INDIGO_LOG(indigo_log("Server started on TCP port %d", *port));
	if (callback) {
		callback(0);
	}
	while (1) {
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
		worker_data->data = data;
		if (!indigo_async((void *(*)(void *))worker, worker_data)) {
			indigo_error("Can't create worker thread for connection (%s)", strerror(errno));
		}
	}
#else
#pragma message ("TODO: indigo_uni_open_server_socket()")
#endif
}

void indigo_uni_set_socket_read_timeout(indigo_uni_handle *handle, long timeout) {
	if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_TCP_HANDLE) {
		struct timeval tv = { .tv_sec = timeout / ONE_SECOND_DELAY, .tv_usec = timeout % ONE_SECOND_DELAY };
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (setsockopt(handle->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		if (setsockopt(handle->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == SOCKET_ERROR) {
			handle->last_error = WSAGetLastError();
		} else {
			handle->last_error = 0;
		}
#else
#pragma message ("TODO: indigo_uni_set_socket_read_timeout()")
#endif
	}
}

void indigo_uni_set_socket_write_timeout(indigo_uni_handle *handle, long timeout) {
	if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_TCP_HANDLE) {
		struct timeval tv = { .tv_sec = timeout / ONE_SECOND_DELAY, .tv_usec = timeout % ONE_SECOND_DELAY };
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if (setsockopt(handle->fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv) != 0) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		if (setsockopt(handle->sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv) == SOCKET_ERROR) {
			handle->last_error = WSAGetLastError();
		} else {
			handle->last_error = 0;
		}
#else
#pragma message ("TODO: indigo_uni_set_socket_write_timeout()")
#endif
	}
}

indigo_uni_handle *indigo_uni_open_url(const char *url, int default_port, indigo_uni_handle_type protocol_hint) {
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
		indigo_copy_name(host_name, host);
	}
	switch (protocol_hint) {
		case INDIGO_TCP_HANDLE:
			return indigo_uni_client_tcp_socket(host_name, port);
		case INDIGO_UDP_HANDLE:
			return indigo_uni_client_udp_socket(host_name, port);
		default:
			return NULL;
	}
}

long indigo_uni_read_available(indigo_uni_handle *handle, void *buffer, long length) {
	long bytes_read = -1;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	bytes_read = read(handle->fd, buffer, length);
	if (bytes_read < 0) {
		handle->last_error = errno;
	} else {
		handle->last_error = 0;
	}
#elif defined(INDIGO_WINDOWS)
	if (handle->type == INDIGO_FILE_HANDLE) {
		bytes_read = _read((int)handle->fd, buffer, length);
		if (bytes_read < 0) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
	} else if (handle->type == INDIGO_COM_HANDLE) {
		if (ReadFile(handle->com, buffer, (DWORD)bufferSize, (DWORD *)&bytesRead, NULL)) {
			handle->last_error = 0;
		} else {
			handle->last_error = GetLastError();
			bytesRead = -1;
		}
	} else if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_UDP_HANDLE) {
		bytes_read = recv(handle->sock, buffer, length, 0);
		if (bytes_read < 0) {
			handle->last_error = WSAGetLastError();
		} else {
			handle->last_error = 0;
		}
	}
#else
#pragma message ("TODO: indigo_uni_read_available()")
#endif
	return bytes_read;
}

long indigo_uni_peek_available(indigo_uni_handle *handle, void *buffer, long length) {
	long bytes_read = -1;
	if (handle->type == INDIGO_TCP_HANDLE) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		bytes_read = recv(handle->fd, buffer, length, MSG_PEEK);
		if (bytes_read < 0) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		bytes_read = recv(handle->sock, buffer, length, MSG_PEEK);
		if (bytes_read < 0) {
			handle->last_error = WSAGetLastError();
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
	if (handle->type == INDIGO_UDP_HANDLE) {
		return indigo_uni_read_available(handle, buffer, length);
	}
	long remains = length;
	long total_bytes = 0;
	char *pnt = (char *)buffer;
	while (true) {
		long bytes_read;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		bytes_read = read(handle->fd, pnt, remains);
		if (bytes_read < 0) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		if (handle->type == INDIGO_FILE_HANDLE) {
			bytes_read = _read((int)handle->fd, pnt, remains);
			if (bytes_read < 0) {
				handle->last_error = errno;
			} else {
				handle->last_error = 0;
			}
		} else if (handle->type == INDIGO_COM_HANDLE) {
			if (ReadFile(handle->com, buffer, (DWORD)bufferSize, (DWORD *)&bytesRead, NULL)) {
				handle->last_error = 0;
			} else {
				handle->last_error = GetLastError();
				bytesRead = -1;
			}
		} else if (handle->type == INDIGO_TCP_HANDLE) {
			bytes_read = recv(handle->sock, pnt, remains, 0);
			if (bytes_read < 0) {
				handle->last_error = WSAGetLastError();
			} else {
				handle->last_error = 0;
			}
			if (bytes_read == -1 && handle->last_error == WSAETIMEDOUT) {
				Sleep(500);
			}
		} else {
			return -1;
		}
#else
#pragma message ("TODO: indigo_uni_read()")
#endif
		if (bytes_read <= 0) {
			if (bytes_read < 0) {
				indigo_error("%d -> // %s", handle->index, indigo_uni_strerror(handle));
			}
			return bytes_read;
		}
		total_bytes += bytes_read;
		if (bytes_read == remains) {
			if (handle->short_trace) {
				INDIGO_TRACE(indigo_trace("%d -> // %ld bytes read", handle->index, total_bytes));
			} else {
				INDIGO_TRACE(indigo_trace("%d -> %s", handle->index, buffer));
			}
			return (int)total_bytes;
		}
		pnt += bytes_read;
		remains -= bytes_read;
	}
}

long indigo_uni_read_section(indigo_uni_handle *handle, char *buffer, long length, const char *terminators, const char *ignore, long timeout) {
	long bytes_read = 0;
	long result;
	struct timeval tv = { .tv_sec = timeout / ONE_SECOND_DELAY, .tv_usec = timeout % ONE_SECOND_DELAY };
	*buffer = 0;
	if (handle->type == INDIGO_UDP_HANDLE) {
		if (timeout >= 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(handle->fd, &readout);
			result = select(handle->fd + 1, &readout, NULL, NULL, &tv);
			if (result == 0) {
				handle->last_error = 0;
				return 0;
			} else if (result < 0) {
				handle->last_error = errno;
				return -1;
			}
#elif defined(INDIGO_WINDOWS)
			fd_set readout;
			FD_ZERO(&readout);
			FD_SET(handle->sock, &readout);
			result = select(0, &readout, NULL, NULL, &tv);
			if (result == 0) {
				handle->last_error = 0;
				return 0;
			} else if (result == SOCKET_ERROR) {
				handle->last_error = WSAGetLastError();
				return -1;
			}
#else
#pragma message ("TODO: indigo_uni_read_section()")
#endif
		}
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		bytes_read = recv(handle->fd, buffer, length - 1, 0);
		if (bytes_read < 0) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		bytes_read = recv(handle->sock, buffer, length - 1, 0);
		if (bytes_read < 0) {
			handle->last_error = WSAGetLastError();
		} else {
			handle->last_error = 0;
		}
#else
#pragma message ("TODO: indigo_uni_read_section()")
#endif
		buffer[bytes_read++] = 0;
	} else {
		bool terminated = false;
		while (bytes_read < length) {
			if (timeout >= 0) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
				fd_set readout;
				FD_ZERO(&readout);
				FD_SET(handle->fd, &readout);
				result = select(handle->fd + 1, &readout, NULL, NULL, &tv);
				if (result == 0) {
					handle->last_error = 0;
					return 0;
				}
				if (result < 0) {
					handle->last_error = errno;
					return -1;
				}
#elif defined(INDIGO_WINDOWS)
				if (handle->type == INDIGO_FILE_HANDLE) {
					HANDLE hFile = (HANDLE)_get_osfhandle(handle->fd);
					if (hFile == INVALID_HANDLE_VALUE) {
						handle->last_error = GetLastError();
						return -1;
					}
					DWORD result = WaitForSingleObject(hFile, timeout * 1000);
					if (result == WAIT_TIMEOUT) {
						handle->last_error = 0;
						return 0;
					} if (result == WAIT_FAILED) {
						handle->last_error = GetLastError();
						return -1;
					}
				} else if (handle->type == INDIGO_COM_HANDLE) {
					result = WaitForSingleObject(handle->com, timeout * 1000);
					if (result == WAIT_TIMEOUT) {
						handle->last_error = 0;
						return 0;
					} if (result == WAIT_FAILED) {
						handle->last_error = GetLastError();
						return -1;
					}
				} else if handle->type == INDIGO_TCP_HANDLE) {
					fd_set readout;
					FD_ZERO(&readout);
					FD_SET(handle->sock, &readout);
					result = select(0, &readout, NULL, NULL, &tv);
					if (result == 0) {
						handle->last_error = 0;
						return 0;
					} else if (result == SOCKET_ERROR) {
						handle->last_error = WSAGetLastError();
						return -1;
					}
				} else {
					return -1;
				}
#else
#pragma message ("TODO: indigo_uni_read_section()")
#endif
			}
			char c;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
			result = read(handle->fd, &c, 1);
			if (result < 1) {
				handle->last_error = errno;
				break;
			}
#elif defined(INDIGO_WINDOWS)
			if handle->type == INDIGO_FILE_HANDLE) {
				result = read(handle->fd, &c, 1);
				if (result < 1) {
					handle->last_error = errno;
					break;
				}
			} else if (handle->type == INDIGO_COM_HANDLE) {
				if (!ReadFile(handle->com, buffer, (DWORD)1, (DWORD *)&result, NULL)) {
					handle->last_error = GetLastError();
					break;
				}
			} else if handle->type == INDIGO_TCP_HANDLE) {
				result = recv(handle->sock, &c, 1);
				if (result < 1) {
					handle->last_error = WSAGetLastError();
					break;
				}
			} else {
				return -1;
			}
#else
#pragma message ("TODO: indigo_uni_read_section()")
#endif
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
	if (handle->short_trace) {
		INDIGO_TRACE(indigo_trace("%d -> // %ld bytes read", handle->index, bytes_read - 1));
	} else {
		INDIGO_TRACE(indigo_trace("%d -> %s", handle->index, buffer));
	}
	return bytes_read - 1;
}

int indigo_uni_scanf_line(indigo_uni_handle *handle, const char *format, ...) {
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
	const char *position = buffer;
	long remains = length;
	while (true) {
		long bytes_written;
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		bytes_written = write(handle->fd, position, remains);
		if (bytes_written < 0) {
			handle->last_error = errno;
		} else {
			handle->last_error = 0;
		}
#elif defined(INDIGO_WINDOWS)
		if (handle->type == INDIGO_FILE_HANDLE) {
			bytes_written = _write((int)handle->fd, position, remains);
			if (bytes_written < 0) {
				handle->last_error = errno;
			} else {
				handle->last_error = 0;
			}
		} else if (handle->type == INDIGO_COM_HANDLE) {
			if (WriteFile(handle->com, &position, remains, &bytesWritten, NULL)) {
				handle->last_error = 0;
			} else {
				handle->last_error = GetLastError();
			}
		} else if (handle->type == INDIGO_TCP_HANDLE || handle->type == INDIGO_TCP_HANDLE) {
			bytes_written = send(handle->sock, position, remains, 0);
			if (bytes_written < 0) {
				handle->last_error = WSAGetLastError();
			} else {
				handle->last_error = 0;
			}
		} else {
			return -1;
		}
#else
#pragma message ("TODO: indigo_uni_write()")
#endif
		if (bytes_written < 0) {
			indigo_error("%d <- // %s", handle->index, indigo_uni_strerror(handle));
			return -1;
		}
		if (bytes_written == remains) {
			if (handle->short_trace) {
				INDIGO_TRACE(indigo_trace("%d <- // %ld bytes written", handle->index, bytes_written));
			} else {
				INDIGO_TRACE(indigo_trace("%d <- %s", handle->index, buffer));
			}
			return bytes_written;
		}
		position += bytes_written;
		remains -= bytes_written;
	}
}

long indigo_uni_printf(indigo_uni_handle *handle, const char *format, ...) {
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
		return indigo_uni_write(handle, format, (long)strlen(format));
	}
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
		handle->last_error = errno;
		return result;
	}
	return -1;
}

bool indigo_uni_lock_file(indigo_uni_handle *handle) {
	if (handle->type == INDIGO_FILE_HANDLE) {
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
				indigo_error("%d <- // %s", handle->index, strerror(errno));
				handle->last_error = errno;
				return false;
			}
#elif defined(INDIGO_WINDOWS)
			if (hFile == INVALID_HANDLE_VALUE) {
				handle->last_error = GetLastError();
				return false;
			}
			if (!LockFile(hFile, 0, 0, MAXDWORD, MAXDWORD)) {
				handle->last_error = GetLastError();
				return false;
			}
#else
#pragma message ("TODO: indigo_uni_lock_file()")
#endif
			return true;
		}
	}
	return false;
}


void indigo_uni_close(indigo_uni_handle **handle) {
	if (*handle) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
		if ((*handle)->type != INDIGO_FILE_HANDLE) {
			shutdown((*handle)->fd, SHUT_RDWR);
			indigo_usleep(ONE_SECOND_DELAY);
		}
		close((*handle)->fd);
#elif defined(INDIGO_WINDOWS)
		if ((*handle)->type == INDIGO_FILE_HANDLE) {
			_close((*handle)->fd);
		} else if ((*handle)->type == INDIGO_COM_HANDLE)
			CloseHandle(*handle)->com);
		} else if ((*handle)->type == INDIGO_TCP_HANDLE || (*handle)->type == INDIGO_UDP_HANDLE) {
			shutdown((*handle)->sock, SD_BOTH);
			closesocket((*handle)->sock);
		}
#else
#pragma message ("TODO: indigo_uni_close()")
#endif
		*handle = NULL;
	}
}

const char *indigo_uni_config_folder() {
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
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
	}
	return config_folder;
}

bool indigo_uni_mkdir(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (mkdir(path, 0777) == 0 || errno == EEXIST) {
		return true;
	}
#elif defined(INDIGO_WINDOWS)
	if (_mkdir(path) == 0 || errno == EEXIST) {
		return true;
	}
#else
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
	return false;
}

bool indigo_uni_remove(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (unlink(path) == 0) {
		return true;
	}
#else
	if (_unlink(path) == 0) {
		return true;
	}
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
	return false;
}

bool indigo_uni_is_readable(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (access(path, R_OK) == 0) {
		return true;
	}
#elif defined(INDIGO_WINDOWS)
	if (_access(path, R_OK) == 0) {
		return true;
	}
#else
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
	return false;
}

bool indigo_uni_is_writable(const char *path) {
#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
	if (access(path, W_OK) == 0) {
		return true;
	}
#elif defined(INDIGO_WINDOWS)
	if (_access(path, W_OK) == 0) {
		return true;
	}
#else
#pragma message ("TODO: indigo_uni_mkdir()")
#endif
	return false;
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
#pragma message ("TODO: indigo_uni_compress()")
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
#pragma message ("TODO: indigo_uni_decompress()")
#endif
}
