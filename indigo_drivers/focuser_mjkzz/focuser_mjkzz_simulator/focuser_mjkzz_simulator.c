// MJKZZ rail focuser simulator
//
// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// This simulator was refactored by a Codex agent.

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "../mjkzz_def.h"
#include "../../../indigo_test/simulator_common/serial_motion.h"
#include "../../../indigo_test/simulator_common/serial_simulator_common.h"

static const char *profile = "normal", *ready_file, *fault_file;
static FILE *events;
static bool headless, trace, moving, stalled;
static volatile sig_atomic_t running = 1;
static int serial_fd = -1;
static uint8_t device_address = 0x01;
static int32_t speed, settle_time, hold_period, backlash, shutter_lag, start_position, end_position, capture_count, step_size;
static uint8_t configuration, execution_mode;
static int32_t registers[7];
static serial_motion motion;

static int32_t message_value(const mjkzz_message *message) {
	uint32_t value = ((uint32_t)message->ucMSG[0] << 24) | ((uint32_t)message->ucMSG[1] << 16) | ((uint32_t)message->ucMSG[2] << 8) | message->ucMSG[3];
	return (int32_t)value;
}

static void set_message_value(mjkzz_message *message, int32_t value) {
	uint32_t encoded = (uint32_t)value;
	message->ucMSG[0] = (uint8_t)(encoded >> 24);
	message->ucMSG[1] = (uint8_t)(encoded >> 16);
	message->ucMSG[2] = (uint8_t)(encoded >> 8);
	message->ucMSG[3] = (uint8_t)encoded;
}

static uint8_t checksum(const mjkzz_message *message) {
	return (uint8_t)(message->ucADD + message->ucCMD + message->ucIDX + message->ucMSG[0] + message->ucMSG[1] + message->ucMSG[2] + message->ucMSG[3]);
}

static void event(const char *kind, const mjkzz_message *message) {
	if (events) {
		fprintf(events, "%.6f %s %02x %02x %u %d %02x\n", serial_motion_time(), kind, message->ucADD, message->ucCMD, message->ucIDX, message_value(message), message->ucSUM);
		fflush(events);
	}
}

static void trace_message(const char *direction, const mjkzz_message *message) {
	if (trace) {
		fprintf(stderr, "%s %02x %02x %02x [%02x %02x %02x %02x] %02x (%d)\n", direction, message->ucADD, message->ucCMD, message->ucIDX, message->ucMSG[0], message->ucMSG[1], message->ucMSG[2], message->ucMSG[3], message->ucSUM, message_value(message));
	}
}

static void stop_signal(int signal) {
	(void)signal;
	running = 0;
}

static bool command_index_valid(uint8_t command, uint8_t index) {
	if (command == CMD_SREG || command == CMD_GREG) {
		return index >= reg_STAT && index <= reg_ADDR;
	}
	if (command == CMD_MOVE) {
		return index == move_normal || index == move_bounded;
	}
	return index == 0;
}

static bool known_command(uint8_t command) {
	return command == CMD_GVER || command == CMD_SCAM || command == CMD_SFCS || command == CMD_MOVE || command == CMD_SREG || command == CMD_GREG || command == CMD_SPOS || command == CMD_GPOS || command == CMD_SSPD || command == CMD_GSPD || command == CMD_SSET || command == CMD_GSET || command == CMD_SHLD || command == CMD_GHLD || command == CMD_SBCK || command == CMD_GBCK || command == CMD_SLAG || command == CMD_GLAG || command == CMD_SSPS || command == CMD_GSPS || command == CMD_SCFG || command == CMD_GCFG || command == CMD_SEPS || command == CMD_GEPS || command == CMD_SCNT || command == CMD_GCNT || command == CMD_SSSZ || command == CMD_GSSZ || command == CMD_EXEC || command == CMD_STOP;
}

static void update_motion(void) {
	if (stalled) {
		return;
	}
	serial_motion_update(&motion);
	if (moving && motion.duration == 0) {
		moving = false;
		execution_mode = 0;
	}
}

static bool read_fault(uint8_t command, uint8_t index, char *action, size_t size) {
	char key[32] = { 0 };
	FILE *file = fault_file ? fopen(fault_file, "r") : NULL;
	if (!file) {
		return false;
	}
	if (fscanf(file, "%31s %63s", key, action) != 2) {
		*action = 0;
	}
	fclose(file);
	if (!strcmp(key, "external")) {
		serial_motion_sync(&motion, strtol(action, NULL, 10));
		moving = stalled = false;
		unlink(fault_file);
		return false;
	}
	char expected[32];
	if (command == CMD_SREG || command == CMD_GREG) {
		snprintf(expected, sizeof(expected), "%c:%u", command, index);
	} else {
		snprintf(expected, sizeof(expected), "%c", command);
	}
	if (strcmp(key, expected)) {
		*action = 0;
		return false;
	}
	action[size - 1] = 0;
	unlink(fault_file);
	return true;
}

static bool write_reply(mjkzz_message *reply, const char *action) {
	if (!strcmp(action, "silent")) {
		return true;
	}
	if (!strcmp(action, "close")) {
		running = 0;
		close(serial_fd);
		serial_fd = -1;
		return true;
	}
	if (!strcmp(action, "badaddr")) {
		reply->ucADD ^= 1;
	} else if (!strcmp(action, "badcmd") || !strcmp(action, "reject")) {
		reply->ucCMD &= 0x7f;
	} else if (!strcmp(action, "badidx")) {
		reply->ucIDX++;
	} else if (!strcmp(action, "badvalue")) {
		set_message_value(reply, 40000);
	}
	reply->ucSUM = checksum(reply);
	if (!strcmp(action, "badsum")) {
		reply->ucSUM++;
	}
	event("TX", reply);
	trace_message("<-", reply);
	if (!strcmp(action, "partial")) {
		return serial_simulator_write_all(serial_fd, (const char *)reply, 4);
	}
	if (!strcmp(profile, "split")) {
		if (!serial_simulator_write_all(serial_fd, (const char *)reply, 3)) {
			return false;
		}
		usleep(10000);
		return serial_simulator_write_all(serial_fd, (const char *)reply + 3, sizeof(*reply) - 3);
	}
	if (!serial_simulator_write_all(serial_fd, (const char *)reply, sizeof(*reply))) {
		return false;
	}
	if (!strcmp(action, "overlong")) {
		uint8_t extra = 0x55;
		return serial_simulator_write_all(serial_fd, (const char *)&extra, 1);
	}
	return true;
}

static void dispatch(const mjkzz_message *request) {
	event("RX", request);
	trace_message("->", request);
	if (request->ucADD == 0xff) {
		return;
	}
	if (request->ucADD != device_address) {
		return;
	}
	bool valid = request->ucSUM == checksum(request) && known_command(request->ucCMD) && command_index_valid(request->ucCMD, request->ucIDX);
	mjkzz_message reply = *request;
	reply.ucADD |= 0x80;
	if (valid) {
		reply.ucCMD |= 0x80;
	}
	char action[64] = { 0 };
	read_fault(request->ucCMD, request->ucIDX, action, sizeof(action));
	if (valid) {
		update_motion();
		switch (request->ucCMD) {
			case CMD_GVER:
				reply.ucMSG[0] = 1;
				reply.ucMSG[1] = 2;
				reply.ucMSG[2] = 3;
				reply.ucMSG[3] = 4;
				break;
			case CMD_SREG:
				if ((request->ucIDX == reg_LPWR || request->ucIDX == reg_HPWR) && (message_value(request) < 1 || message_value(request) > 12)) {
					reply.ucCMD &= 0x7f;
				} else if (request->ucIDX == reg_MSTEP && (message_value(request) < MOTOR_4STEP || message_value(request) > MOTOR_HSTEP)) {
					reply.ucCMD &= 0x7f;
				} else if (request->ucIDX == reg_MAXP && message_value(request) < 0) {
					reply.ucCMD &= 0x7f;
				} else if (request->ucIDX == reg_ADDR && (message_value(request) < 1 || message_value(request) > 0x7f)) {
					reply.ucCMD &= 0x7f;
				} else if (request->ucIDX == reg_STAT || request->ucIDX == reg_EXEC) {
					reply.ucCMD &= 0x7f;
				} else {
					registers[request->ucIDX - reg_STAT] = message_value(request);
					if (request->ucIDX == reg_ADDR) {
						device_address = (uint8_t)message_value(request);
					}
				}
				break;
			case CMD_GREG:
				if (request->ucIDX == reg_STAT) {
					reply.ucMSG[0] = 1;
					reply.ucMSG[1] = (uint8_t)registers[reg_LPWR - reg_STAT];
					reply.ucMSG[2] = (uint8_t)registers[reg_HPWR - reg_STAT];
					reply.ucMSG[3] = (uint8_t)registers[reg_MSTEP - reg_STAT];
				} else if (request->ucIDX == reg_EXEC) {
					reply.ucMSG[0] = execution_mode;
					reply.ucMSG[1] = reply.ucMSG[2] = reply.ucMSG[3] = 0;
				} else {
					set_message_value(&reply, registers[request->ucIDX - reg_STAT]);
				}
				break;
			case CMD_SCAM:
			case CMD_SFCS:
				if (message_value(request) < 0) {
					reply.ucCMD &= 0x7f;
				}
				break;
			case CMD_MOVE: {
				double target = motion.position + message_value(request);
				if (request->ucIDX == move_bounded) {
					int32_t maximum = registers[reg_MAXP - reg_STAT];
					target = target < 0 ? 0 : target > maximum ? maximum : target;
				}
				serial_motion_start(&motion, target, 2000.0 / (speed + 1));
				moving = true;
				break;
			}
			case CMD_SPOS:
				if (!strcmp(action, "stall")) {
					stalled = true;
					action[0] = 0;
				}
				serial_motion_start(&motion, message_value(request), 2000.0 / (speed + 1));
				moving = true;
				break;
			case CMD_GPOS:
				set_message_value(&reply, (int32_t)motion.position);
				break;
			case CMD_SSPD:
				if (message_value(request) < 0 || message_value(request) > 255) {
					reply.ucCMD &= 0x7f;
				} else {
					speed = message_value(request);
				}
				break;
			case CMD_GSPD:
				set_message_value(&reply, speed);
				break;
			case CMD_SSET:
				settle_time = message_value(request);
				break;
			case CMD_GSET:
				set_message_value(&reply, settle_time);
				break;
			case CMD_SHLD:
				hold_period = message_value(request);
				break;
			case CMD_GHLD:
				set_message_value(&reply, hold_period);
				break;
			case CMD_SBCK:
				backlash = message_value(request);
				break;
			case CMD_GBCK:
				set_message_value(&reply, backlash);
				break;
			case CMD_SLAG:
				shutter_lag = message_value(request);
				break;
			case CMD_GLAG:
				set_message_value(&reply, shutter_lag);
				break;
			case CMD_SSPS:
				start_position = message_value(request);
				break;
			case CMD_GSPS:
				set_message_value(&reply, start_position);
				break;
			case CMD_SEPS:
				end_position = message_value(request);
				break;
			case CMD_GEPS:
				set_message_value(&reply, end_position);
				break;
			case CMD_SCFG:
				configuration = request->ucMSG[0];
				break;
			case CMD_GCFG:
				reply.ucMSG[0] = configuration;
				reply.ucMSG[1] = reply.ucMSG[2] = reply.ucMSG[3] = 0;
				break;
			case CMD_SCNT:
				capture_count = message_value(request);
				break;
			case CMD_GCNT:
				set_message_value(&reply, capture_count);
				break;
			case CMD_SSSZ:
				step_size = message_value(request);
				break;
			case CMD_GSSZ:
				set_message_value(&reply, step_size);
				break;
			case CMD_EXEC:
				execution_mode = 1;
				serial_motion_start(&motion, start_position, 2000.0 / (speed + 1));
				moving = true;
				break;
			case CMD_STOP:
				if (stalled) {
					serial_motion_sync(&motion, motion.position);
				} else {
					serial_motion_stop(&motion);
				}
				moving = stalled = false;
				set_message_value(&reply, (int32_t)motion.position);
				break;
		}
	}
	write_reply(&reply, action);
}

static bool parse_args(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) {
			headless = true;
		} else if (!strcmp(argv[i], "--trace")) {
			trace = true;
		} else if (i + 1 < argc && !strcmp(argv[i], "--ready-file")) {
			ready_file = argv[++i];
		} else if (i + 1 < argc && !strcmp(argv[i], "--profile")) {
			profile = argv[++i];
		} else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			printf("Usage: %s [--headless] [--trace] [--ready-file PATH] [--profile normal|split|alternate]\n", argv[0]);
			exit(0);
		} else {
			fprintf(stderr, "Unknown/incomplete option: %s\n", argv[i]);
			return false;
		}
	}
	return true;
}

int main(int argc, char **argv) {
	(void)serial_simulator_trace_line;
	if (!parse_args(argc, argv)) {
		return 1;
	}
	serial_motion_sync(&motion, !strcmp(profile, "alternate") ? -1000 : 0);
	speed = !strcmp(profile, "alternate") ? 2 : 0;
	registers[reg_HPWR - reg_STAT] = 8;
	registers[reg_LPWR - reg_STAT] = 1;
	registers[reg_MSTEP - reg_STAT] = 2;
	registers[reg_MAXP - reg_STAT] = 32767;
	registers[reg_ADDR - reg_STAT] = 1;
	fault_file = getenv("INDIGO_MJKZZ_FAULT");
	const char *event_path = getenv("INDIGO_MJKZZ_EVENTS");
	events = event_path ? fopen(event_path, "w") : NULL;
	signal(SIGTERM, stop_signal);
	signal(SIGINT, stop_signal);
	char port[128];
	serial_fd = serial_simulator_open_pty(port, sizeof(port));
	if (serial_fd < 0 || (ready_file && !serial_simulator_write_ready_file(ready_file, "focuser_mjkzz_simulator", port))) {
		if (events) {
			fclose(events);
		}
		return 1;
	}
	if (!headless) {
		printf("MJKZZ rail focuser simulator is listening on %s\n", port);
		fflush(stdout);
	}
	uint8_t buffer[sizeof(mjkzz_message)];
	size_t used = 0;
	while (running) {
		update_motion();
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(serial_fd, &reads);
		struct timeval timeout = { 0, 10000 };
		if (select(serial_fd + 1, &reads, NULL, NULL, &timeout) <= 0) {
			continue;
		}
		ssize_t count = read(serial_fd, buffer + used, sizeof(buffer) - used);
		if (count > 0) {
			used += (size_t)count;
			if (used == sizeof(buffer)) {
				dispatch((const mjkzz_message *)buffer);
				used = 0;
			}
		} else if (count < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK && errno != EIO) {
			break;
		}
	}
	if (serial_fd >= 0) {
		close(serial_fd);
	}
	if (events) {
		fclose(events);
	}
	return 0;
}
