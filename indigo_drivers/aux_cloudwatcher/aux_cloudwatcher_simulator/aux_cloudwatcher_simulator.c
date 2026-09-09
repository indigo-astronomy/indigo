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

static void sim_dispatch(const char *command) {
	if (!command) { return; }
	reply_size = 0;
	if (!strcmp(sim_profile, "timeout")) {
		return;
	}
	if (!strcmp(command, "A!")) { block(!strcmp(sim_profile, "wrong-identity") ? "!N Unknown" : "!N CloudWatcher"); }
	else if (!strcmp(command, "B!")) { block("!V 5.89"); }
	else if (!strcmp(command, "K!")) { block("!K1234"); }
	else if (!strcmp(command, "F!")) { block(relay ? "!Y" : "!X"); }
	else if (!strcmp(command, "G!") || !strcmp(command, "H!")) {
		if (strcmp(sim_profile, "relay-error")) { relay = command[0] == 'H'; }
		block(!strcmp(sim_profile, "relay-error") ? "!Z" : relay ? "!Y" : "!X");
	}
	else if (!strcmp(command, "S!")) { block("!1 -1200"); }
	else if (!strcmp(command, "T!")) { block("!2 2000"); }
	else if (!strcmp(command, "E!")) { block("!R 2500"); }
	else if (!strcmp(command, "C!")) { block("!6 600"); block("!4 512"); block("!5 512"); block("!8 100"); }
	else if (!strcmp(command, "v!")) { block("!v 1"); }
	else if (!strcmp(command, "V!")) { block("!w 36"); }
	else if (!strcmp(command, "h!")) { block("!h 40"); }
	else if (!strcmp(command, "t!")) { block("!t 38"); }
	else if (!strcmp(command, "p!")) { block("!p 1600000"); }
	else if (!strcmp(command, "q!")) { block("!q 2000"); }
	else if (!strcmp(command, "M!")) {
		const char values[15] = { '!', 'M', 3, 100, 3, 100, 0, 100, 15, 0, 0, 100, 0, 100, ' ' };
		memcpy(reply, values, 15); reply_size = 15;
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
