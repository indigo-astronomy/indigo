// Copyright (c) 2026 CloudMakers, s. r. o.
// Use under the INDIGO Astronomy open-source license (see LICENSE.md).
// Primary: https://lunaticoastro.com/rs232-communication-protocol-cloudwatcher/
// Binary constants: https://lunaticoastro.com/aagcw/TechInfo/Rs232_Comms_v110.pdf
#include <assert.h>
#define SIM_UDP 0
#define SIM_NAME "aux_cloudwatcher"
#define SIM_TERMINATOR '!'
#include "../../../indigo_test/simulator_common/aux_simulator_common.h"
static int relay, pwm;
static char reply[120];
static size_t reply_size;

static void block(const char *text) {
	assert(reply_size + 15 <= sizeof(reply));
	memset(reply + reply_size, ' ', 15);
	memcpy(reply + reply_size, text, strlen(text));
	reply_size += 15;
}

// Profiles model the sensor sets and encodings the protocol allows:
// normal        - firmware 5.89 with SQ, low resolution RH/T and pressure
// no-sensors    - no RH/T, no pressure, no SQ, no anemometer, dark sky
// precise       - high resolution RH/T encoding plus the ambient thermistor
// wet-overcast  - raining, overcast, very light sky, humid, calm
// constants-high - factory M! constants whose low bytes exceed 127
// slow-switch    - the relay state reply is held, so a change can land during a poll
static void sim_dispatch(const char *command) {
	if (!command) { return; }
	reply_size = 0;
	if (!strcmp(sim_profile, "timeout")) {
		return;
	}
	const bool no_sensors = !strcmp(sim_profile, "no-sensors");
	const bool precise = !strcmp(sim_profile, "precise");
	const bool wet = !strcmp(sim_profile, "wet-overcast");
	if (!strcmp(command, "A!")) { block(!strcmp(sim_profile, "wrong-identity") ? "!N Unknown" : "!N CloudWatcher"); }
	else if (!strcmp(command, "B!")) { block("!V 5.89"); }
	else if (!strcmp(command, "K!")) { block("!K1234"); }
	else if (!strcmp(command, "F!")) {
		// slow-switch holds the relay state reply, so a test can issue a change request while the
		// driver's poll is still waiting for it and exercise that race deterministically.
		if (!strcmp(sim_profile, "slow-switch")) { usleep(700000); }
		block(relay ? "!Y" : "!X");
	}
	else if (!strcmp(command, "G!") || !strcmp(command, "H!")) {
		if (strcmp(sim_profile, "relay-error")) { relay = command[0] == 'H'; }
		block(!strcmp(sim_profile, "relay-error") ? "!Z" : relay ? "!Y" : "!X");
	}
	else if (!strcmp(command, "S!")) { block(wet ? "!1 500" : "!1 -1200"); }
	else if (!strcmp(command, "T!")) { block("!2 2000"); }
	else if (!strcmp(command, "E!")) { block(wet ? "!R 300" : "!R 2500"); }
	else if (!strcmp(command, "C!")) {
		block("!6 600");
		if (precise) { block("!3 512"); }
		block(no_sensors ? "!4 1020" : wet ? "!4 5" : "!4 512");
		block("!5 512");
		if (!no_sensors) { block("!8 100"); }
	}
	else if (!strcmp(command, "v!")) { block(no_sensors ? "!v 0" : "!v 1"); }
	else if (!strcmp(command, "V!")) { block(wet ? "!w 0" : "!w 36"); }
	else if (!strcmp(command, "h!")) { block(precise ? "!hh26214" : wet ? "!h 80" : "!h 40"); }
	else if (!strcmp(command, "t!")) { block(no_sensors ? "!t 65535" : precise ? "!th17000" : "!t 38"); }
	else if (!strcmp(command, "p!")) { block(no_sensors ? "!p 65535" : "!p 1600000"); }
	else if (!strcmp(command, "q!")) { block("!q 2000"); }
	else if (!strcmp(command, "M!")) {
		// The factory constants of a real unit need the full 0...255 range of
		// the low byte; LDR max R = 1744 kOhm is 0x06 0xD0.
		const char high[15] = { '!', 'M', 3, 32, 6, (char)208, 2, 48, 13, 122, 0, 10, 0, 10, ' ' };
		const char values[15] = { '!', 'M', 3, 100, 3, 100, 0, 100, 15, 0, 0, 100, 0, 100, ' ' };
		memcpy(reply, !strcmp(sim_profile, "constants-high") ? high : values, 15); reply_size = 15;
	}
	else if (command[0] == 'P' || command[0] == 'Q') {
		if (command[0] == 'P') { pwm = atoi(command + 1); }
		char value[15]; snprintf(value, sizeof(value), "!Q %d", pwm); block(value);
	}
	else { block("!Z 0"); }
	block("!\x11");
	reply[reply_size - 1] = '0';
	sim_send(reply, reply_size);
	return;
}

int main(int argc, char **argv) { return sim_main(argc, argv); }
