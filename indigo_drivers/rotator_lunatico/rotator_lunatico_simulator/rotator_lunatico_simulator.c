// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
// Primary protocol: https://lunaticoastro.com/slp_docs/data/commands.json
// Supplementary settings/reply details: shared Lunatico driver (see test note).
#ifndef SIM_UDP
#define SIM_UDP 0
#endif
#define SIM_NAME "rotator_lunatico"
#define SIM_TERMINATOR '#'
#include "../../../indigo_test/simulator_common/aux_simulator_common.h"
#include <math.h>
static double position[3] = { 1800, 1000, 1000 }, target[3] = { 1800, 1000, 1000 };
static double speed[3] = { 100, 100, 100 }, previous;
static int low[3], high[3], outputs[3][10];
static bool limits[3], has_moved;

static void advance(void) {
	double t = sim_time(), elapsed = previous ? t - previous : 0;
	previous = t;
	for (int i = 0; i < 3; i++) {
		double distance = target[i] - position[i];
		if (fabs(distance) <= speed[i] * elapsed) { position[i] = target[i]; }
		else { position[i] += copysign(speed[i] * elapsed, distance); }
	}
}

static void sim_dispatch(const char *command) {
	advance();
	if (!command) { return; }
	char result[128] = "0", reply[256], group[24], verb[24];
	int port = 0, a = 0, b = 0;
	if (!strcmp(sim_profile, "silent")) { return; }
	if (!strcmp(sim_profile, "oversized")) { memset(reply, 'x', 100); sim_send(reply, 100); return; }
	if (!strcmp(command, "!seletek version#")) { strcpy(result, !strcmp(sim_profile, "wrong-model") ? "4529" : "3529"); }
	else if (sscanf(command, "!%23s %23s %d %d %d", group, verb, &port, &a, &b) >= 3 && port >= 0 && port < 3) {
		if (!strcmp(group, "step")) {
			if (!strcmp(verb, "getpos")) { snprintf(result, sizeof(result), "%d", (int)lround(position[port])); }
			else if (!strcmp(verb, "ismoving")) { snprintf(result, sizeof(result), "%d", position[port] != target[port]); }
			else if (!strcmp(verb, "goto") || !strcmp(verb, "gopr")) {
				int requested = !strcmp(verb, "gopr") ? (int)position[port] + a : a;
				if (!strcmp(sim_profile, "goto-error") || (limits[port] && (requested < low[port] || requested > high[port]))) { strcpy(result, "-1"); }
				else { target[port] = requested; has_moved = true; }
			}
			else if (!strcmp(verb, "setpos")) { position[port] = target[port] = a; }
			else if (!strcmp(verb, "stop")) {
				if (!strcmp(sim_profile, "stop-error")) { strcpy(result, "-1"); }
				else { target[port] = position[port]; }
			}
			else if (!strcmp(verb, "speedrangeus") && a > 0) { speed[port] = 1000000.0 / a; }
			else if (!strcmp(verb, "setswlimits")) { limits[port] = true; low[port] = a; high[port] = b; }
			else if (!strcmp(verb, "delswlimits")) { limits[port] = false; }
			else if (strcmp(verb, "halfstep") && strcmp(verb, "wiremode") && strcmp(verb, "model") && strcmp(verb, "movepow") && strcmp(verb, "stoppow")) { strcpy(result, "-1"); }
			if (has_moved && !strcmp(sim_profile, "read-error") && (!strcmp(verb, "getpos") || !strcmp(verb, "ismoving"))) { strcpy(result, "invalid"); }
		} else if (!strcmp(group, "read") && !strcmp(verb, "an")) { snprintf(result, sizeof(result), "%d", 500 + a); }
		else if (!strcmp(group, "read") && !strcmp(verb, "temps")) { strcpy(result, "500"); }
		else if (!strcmp(group, "write") && !strcmp(verb, "dig") && a >= 0 && a < 10) { outputs[port][a] = b != 0; }
		else { strcpy(result, "-1"); }
	} else { strcpy(result, "-1"); }
	snprintf(reply, sizeof(reply), "%.*s:%s#", (int)strlen(command) - 1, command, result);
	sim_send(reply, strlen(reply));
}

int main(int argc, char **argv) {
	return sim_main(argc, argv);
}
