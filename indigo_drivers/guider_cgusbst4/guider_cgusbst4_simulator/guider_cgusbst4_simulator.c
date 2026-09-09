// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
// Protocol evidence and unresolved dialect difference: indigo_test/GUIDER_PROTOCOL_TESTS.md.
// Default PHD2 dialect uses numeric directions; --profile indigo accepts letters.
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"
#include <signal.h>
#include <sys/select.h>
#include <time.h>
static volatile sig_atomic_t running = 1;
static FILE *events;
static double until[4];
static const char *profile = "phd2";

static double now(void) {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec / 1e9;
}

static void finish(int signal_number) { running = 0; }

static void record(const char *kind, int direction, int duration) {
	if (events) { fprintf(events, "%s %d %d %.9f\n", kind, direction, duration, now()); fflush(events); }
}

static void command(const char *text) {
	char direction;
	int duration;
	if (sscanf(text, ":Mg%c%d#", &direction, &duration) != 2 || duration < 0 || duration > 9999) { record("REJECT", -1, 0); return; }
	const char *alphabet = !strcmp(profile, "indigo") ? "ns ew" : "01 23";
	const char *found = strchr(alphabet, direction);
	if (!found || direction == ' ') { record("REJECT", direction, duration); return; }
	int index = (int)(found - alphabet);
	if (index > 2) { index--; }
	int opposite = index ^ 1;
	if (until[opposite]) { until[opposite] = 0; record("OFF", opposite, 0); }
	until[index] = now() + duration / 1000.0;
	record("ON", index, duration);
}

int main(int argc, char **argv) {
	const char *ready = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) { continue; }
		if (!strcmp(argv[i], "--ready-file") && i + 1 < argc) { ready = argv[++i]; continue; }
		if (!strcmp(argv[i], "--profile") && i + 1 < argc) { profile = argv[++i]; continue; }
		fprintf(stderr, "Usage: %s --headless --ready-file PATH [--profile phd2|indigo|wrong-identity|silent]\n", argv[0]);
		return 1;
	}
	signal(SIGTERM, finish);
	signal(SIGINT, finish);
	signal(SIGPIPE, SIG_IGN);
	char port[128], event_path[PATH_MAX];
	int fd = serial_simulator_open_pty(port, sizeof(port));
	if (fd < 0) { return 1; }
	if (ready) {
		snprintf(event_path, sizeof(event_path), "%s.events", ready);
		events = fopen(event_path, "w");
		if (!events || !serial_simulator_write_ready_file(ready, "guider_cgusbst4", port)) { close(fd); return 1; }
	}
	char text[64];
	size_t used = 0;
	while (running) {
		for (int i = 0; i < 4; i++) {
			if (until[i] && now() >= until[i]) { until[i] = 0; record("OFF", i, 0); }
		}
		fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		struct timeval timeout = { .tv_usec = 1000 };
		if (select(fd + 1, &set, NULL, NULL, &timeout) <= 0) { continue; }
		char input[64];
		ssize_t count = read(fd, input, sizeof(input));
		if (count <= 0) { usleep(1000); continue; }
		for (ssize_t i = 0; i < count; i++) {
			if (input[i] == 6) {
				if (strcmp(profile, "silent")) { serial_simulator_write_all(fd, !strcmp(profile, "wrong-identity") ? "B" : "A", 1); }
				record("HELLO", 0, 0);
				used = 0;
			} else if (input[i] == '#') {
				if (used) { text[used++] = '#'; text[used] = 0; command(text); }
				used = 0;
			} else if (used < sizeof(text) - 2) { text[used++] = input[i]; }
			else { used = 0; }
		}
	}
	if (events) { fclose(events); }
	close(fd);
	return 0;
}
