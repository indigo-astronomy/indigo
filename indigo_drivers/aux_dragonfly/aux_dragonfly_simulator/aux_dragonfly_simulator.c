// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
// Primary framing, UDP and channel ranges:
// https://lunaticoastro.com/seletek-developers-guide/
// Pulse semantics: https://lunaticoastro.com/df-javascript/
// Supplementary command replies: ../relio_simulator/relio_simulator.pl.
// The public guide defers the detailed command catalogue to a spreadsheet
// available on request; undocumented reply details are not independent oracles.
#define SIM_UDP 1
#define SIM_NAME "aux_dragonfly"
#define SIM_TERMINATOR '#'
#include "../../../indigo_test/simulator_common/aux_simulator_common.h"
static int relay[8];
static double until[8];

static void sim_dispatch(const char *command) {
	for (int i = 0; i < 8; i++) {
		if (until[i] && sim_time() >= until[i]) { relay[i] = 0; until[i] = 0; }
	}
	if (!command) { return; }
	char response[256], result[128] = "-1";
	int port, channel, value;
	if (!strcmp(sim_profile, "oversized")) { memset(response, 'x', 100); sim_send(response, 100); return; }
	if (!strcmp(command, "!seletek version#")) { strcpy(result, !strcmp(sim_profile, "wrong-model") ? "1529" : "4529"); }
	else if (!strncmp(command, "!aux earnaccess ", 16)) { strcpy(result, "3"); }
	else if (!strcmp(command, "!relio snanrd 0 0 7#")) { strcpy(result, !strcmp(sim_profile, "short-sensors") ? "11,12" : "11,12,13,14,15,16,17,18"); }
	else if (!strcmp(command, "!relio rldgrd 0 0 7#")) { snprintf(result, sizeof(result), "%d,%d,%d,%d,%d,%d,%d,%d", relay[0], relay[1], relay[2], relay[3], relay[4], relay[5], relay[6], relay[7]); }
	else if (sscanf(command, "!relio rlset %d %d %d#", &port, &channel, &value) == 3 && channel >= 0 && channel < 8) {
		if (strcmp(sim_profile, "relay-error")) { relay[channel] = value != 0; snprintf(result, sizeof(result), "%d", relay[channel]); }
	}
	else if (sscanf(command, "!relio rlpulse %d %d %d#", &port, &channel, &value) == 3 && channel >= 0 && channel < 8 && value >= 0) {
		relay[channel] = 1; until[channel] = sim_time() + value / 1000.0; strcpy(result, "0");
	}
	snprintf(response, sizeof(response), "%.*s:%s#", (int)strlen(command) - 1, command, result);
	sim_send(response, strlen(response));
}

int main(int argc, char **argv) {
	return sim_main(argc, argv);
}
