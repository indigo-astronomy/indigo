// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
// Primary: ../doc/MGPBox Manual English 1.1.pdf, pp. 15–20.
// The manual specifies commands but not the device-type reply spelling;
// that reply alone follows the existing driver sample. Fault profiles are
// intentionally invalid input, not assertions about real device firmware.
#define SIM_UDP 0
#define SIM_NAME "aux_mgbox"
#define SIM_TERMINATOR '*'
#include "../../../indigo_test/simulator_common/aux_simulator_common.h"
static int calp, calt, calh, mm = 1, mg;
static bool identified, gps_on = true;
static double pulse_until;

static void sentence(const char *body) {
	unsigned char checksum = 0;
	for (const char *p = body; *p; p++) { checksum ^= *p; }
	char line[512];
	int size = snprintf(line, sizeof(line), "$%s*%02X\r\n", body, checksum);
	sim_send(line, size);
}

static void calibration(void) {
	char line[128];
	snprintf(line, sizeof(line), "PCAL,P,%d,T,%d,H,%d,MM,%d,MG,%d", calp, calt, calh, mm, mg);
	sentence(line);
}

static void sim_dispatch(const char *command) {
	if (!command) {
		if (!identified) { return; }
		if (!strcmp(sim_profile, "short-weather")) { sentence("PXDR"); return; }
		if (!strcmp(sim_profile, "short-gps")) { sentence("GPRMC"); return; }
		sentence("PXDR,P,96276.0,P,0,C,31.8,C,1,H,40.8,P,2,C,16.8,C,3,0.8");
		if (gps_on) {
			sentence("GPRMC,120000,A,4800.000,N,01700.000,E,0.0,0.0,090926,,,A");
			sentence("GPGGA,120000,4800.000,N,01700.000,E,1,08,1.0,250.0,M,0.0,M,,");
		}
		if (pulse_until && sim_time() >= pulse_until) { pulse_until = 0; }
		return;
	}
	if (!strcmp(command, ":devicetype*")) { identified = true; sentence("LOG: Device Type: MGPBox"); }
	else if (sscanf(command, ":calp,%d*", &calp) == 1 || sscanf(command, ":calt,%d*", &calt) == 1 || sscanf(command, ":calh,%d*", &calh) == 1) { calibration(); }
	else if (!strcmp(command, ":calreset*")) { calp = calt = calh = 0; calibration(); }
	else if (!strcmp(command, ":calget*")) { calibration(); }
	else if (sscanf(command, ":mm,%d*", &mm) == 1 || sscanf(command, ":mg,%d*", &mg) == 1) { calibration(); }
	else if (!strcmp(command, ":gpson*")) { gps_on = true; }
	else if (!strcmp(command, ":gpsoff*")) { gps_on = false; }
	else if (!strncmp(command, ":pulse,", 7)) { pulse_until = sim_time() + atoi(command + 7) / 1000.0; }
	else if (!strcmp(command, ":rebootgps*") || !strcmp(command, ":reboot*")) { gps_on = true; }
}

int main(int argc, char **argv) {
	return sim_main(argc, argv);
}
