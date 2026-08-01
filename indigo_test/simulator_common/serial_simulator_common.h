// Shared helpers for headless serial device simulators.
//
// This source file was generated and refactored by a Codex agent.

#ifndef serial_simulator_common_h
#define serial_simulator_common_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static int serial_simulator_open_pty(char *port, size_t port_size) {
	int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("open /dev/ptmx");
		return -1;
	}
	if (grantpt(fd) != 0) {
		perror("grantpt");
		close(fd);
		return -1;
	}
	if (unlockpt(fd) != 0) {
		perror("unlockpt");
		close(fd);
		return -1;
	}
#if defined(INDIGO_MACOS) || defined(__APPLE__)
	char *name = ptsname(fd);
	if (name == NULL) {
		perror("ptsname");
		close(fd);
		return -1;
	}
	snprintf(port, port_size, "%s", name);
#else
	if (ptsname_r(fd, port, port_size) != 0) {
		perror("ptsname_r");
		close(fd);
		return -1;
	}
#endif
	return fd;
}

static bool serial_simulator_write_ready_file(const char *ready_file, const char *simulator_name, const char *port) {
	char tmp_file[PATH_MAX];
	snprintf(tmp_file, sizeof(tmp_file), "%s.tmp.%ld", ready_file, (long)getpid());

	FILE *file = fopen(tmp_file, "w");
	if (file == NULL) {
		perror("fopen ready file");
		return false;
	}

	fprintf(file, "INDIGO_SIMULATOR=%s\n", simulator_name);
	fprintf(file, "INDIGO_SIMULATOR_PORT=%s\n", port);
	fprintf(file, "INDIGO_SIMULATOR_PID=%ld\n", (long)getpid());
	if (fclose(file) != 0) {
		perror("fclose ready file");
		unlink(tmp_file);
		return false;
	}

	if (rename(tmp_file, ready_file) != 0) {
		perror("rename ready file");
		unlink(tmp_file);
		return false;
	}
	return true;
}

static void serial_simulator_trace_line(bool trace, const char *prefix, const char *line) {
	if (trace) {
		fprintf(stderr, "%s %s\n", prefix, line);
	}
}

static bool serial_simulator_write_all(int fd, const char *buffer, size_t length) {
	while (length > 0) {
		ssize_t written = write(fd, buffer, length);
		if (written < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		buffer += written;
		length -= (size_t)written;
	}
	return true;
}

#endif /* serial_simulator_common_h */
