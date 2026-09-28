// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
// Refactored by OpenAI Codex, 2026.
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
static bool fault_sent;
static double identified_at;
static FILE *events;

// Standalone model capabilities follow the driver/README contract; the bundled
// manufacturer's manual describes MGPBox, not standalone PBox/MBox firmware.
static const char *model(void) {
	if (!strcmp(sim_profile, "pbox")) {
		return "PBox";
	}
	if (!strcmp(sim_profile, "mbox")) {
		return "MBox";
	}
	return "MGPBox";
}

static void event(const char *kind, const char *value) {
	if (events) {
		fprintf(events, "%.6f %s %s\n", sim_time(), kind, value);
		fflush(events);
	}
}

static void sentence(const char *body) {
	unsigned char checksum = 0;
	for (const char *p = body; *p; p++) {
		checksum ^= *p;
	}
	char line[512];
	int size = snprintf(line, sizeof(line), "$%s*%02X\r\n", body, checksum);
	if (!strcmp(sim_profile, "split")) {
		sim_send(line, size / 2);
		usleep(150000);
		sim_send(line + size / 2, size - size / 2);
		event("SPLIT", body);
	} else {
		sim_send(line, size);
	}
	event("TX", body);
}

static void calibration(void) {
	if (!strcmp(sim_profile, "no-cal-reply")) {
		return;
	}
	char line[128];
	snprintf(line, sizeof(line), "PCAL,P,%d,T,%d,H,%d,MM,%d,MG,%d", calp, calt, calh, mm, mg);
	sentence(line);
}

static void sim_dispatch(const char *command) {
	if (!command) {
		if (pulse_until && sim_time() >= pulse_until) {
			pulse_until = 0;
			event("PULSE_OFF", "0");
		}
		if (!identified) {
			return;
		}
		if (!fault_sent && !strcmp(sim_profile, "short-weather")) {
			fault_sent = true;
			sentence("PXDR");
			return;
		}
		if (!fault_sent && !strcmp(sim_profile, "short-gps")) {
			fault_sent = true;
			sentence("GPRMC");
			return;
		}
		if (!fault_sent && !strcmp(sim_profile, "parser-faults")) {
			fault_sent = true;
			const char *faults[] = { "GPRMC", "GPGGA", "GPGSA", "GPGSV", "PXDR", "PCAL", "PCAL,P,0,T,0,H,0,MM", "PCAL,P,0,T,0,H,0,MG,nan", "PXDR,P,nan,P,0,C,31.8,C,1,H,40.8,P,2,C,16.8,C,3,0.8", "GPRMC,250000,A,4800,N,01700,E,0,0,310226", "GPGGA,120000,4800,N,01700,E,1,08,1,nan,M", "GPGSA,A,3,,,,,,,,,,,,,nan,1,1", "GPGSV,1,1,inf", "LOG:" };
			for (unsigned i = 0; i < sizeof(faults) / sizeof(faults[0]); i++) {
				sentence(faults[i]);
			}
			char overflow[900];
			memset(overflow, ',', sizeof(overflow) - 1);
			memcpy(overflow, "$PXDR,", 6);
			overflow[sizeof(overflow) - 1] = '\n';
			sim_send(overflow, sizeof(overflow));
			const char *bad_checksum = "$PXDR,P,1,P,0,C,1,C,1,H,1,P,2,C,1,C,3,0.8*zz\r\n";
			sim_send(bad_checksum, strlen(bad_checksum));
			char many_tokens[128];
			memset(many_tokens, ',', sizeof(many_tokens) - 1);
			memcpy(many_tokens, "PXDR", 4);
			many_tokens[sizeof(many_tokens) - 1] = 0;
			sentence(many_tokens);
			const char *mismatch = "$PXDR,P,1,P,0,C,1,C,1,H,1,P,2,C,1,C,3,0.8*00\r\n";
			sim_send(mismatch, strlen(mismatch));
			event("FAULT", "parser-faults");
			return;
		}
		if (strchr(model(), 'M')) {
			if (!strcmp(sim_profile, "alternate")) {
				sentence("PXDR,P,101325.0,P,0,C,20.0,C,1,H,50.0,P,2,C,10.0,C,3,0.8");
			} else {
				sentence("PXDR,P,96276.0,P,0,C,31.8,C,1,H,40.8,P,2,C,16.8,C,3,0.8");
			}
		}
		if (gps_on && strchr(model(), 'G')) {
			double age = sim_time() - identified_at;
			if (!strcmp(sim_profile, "gps-fix") && age >= 4 && age < 6) {
				sentence("GPRMC,120001,V,,,,,,,090926");
				sentence("GPGGA,120001,,,,,0");
				sentence("GPGSA,A,1");
			} else {
				if (!strcmp(sim_profile, "alternate")) {
					sentence("GPRMC,120000,A,4900.000,N,01800.000,E,0.0,0.0,090926,,,A");
					sentence("GPGGA,120000,4900.000,N,01800.000,E,1,08,1.0,250.0,M,0.0,M,,");
				} else if (!strcmp(sim_profile, "southern")) {
					if (age < 4) {
						sentence("GPRMC,235959,A,3351.000,S,15112.000,W,0.0,0.0,311226,,,A");
					} else {
						sentence("GPRMC,000000,A,3351.000,S,15112.000,W,0.0,0.0,010127,,,A");
					}
					sentence("GPGGA,235959,3351.000,S,15112.000,W,1,08,1.0,-12.0,M,0.0,M,,");
				} else {
					sentence("GPRMC,120000,A,4800.000,N,01700.000,E,0.0,0.0,090926,,,A");
					sentence("GPGGA,120000,4800.000,N,01700.000,E,1,08,1.0,250.0,M,0.0,M,,");
				}
				if (!strcmp(sim_profile, "gps-fix") && age >= 6 && age < 8) {
					sentence("GPGSA,A,2,01,02,03,04,05,06,07,08,,,,,1.8,1.0,1.5");
				} else {
					sentence("GPGSA,A,3,01,02,03,04,05,06,07,08,,,,,1.8,1.0,1.5");
				}
				sentence("GPGSV,3,1,12,01,45,180,40");
			}
		}
		return;
	}
	event("RX", command);
	if (!strcmp(sim_profile, "silent")) {
		return;
	}
	if (!strcmp(command, ":devicetype*")) {
		char reply[64];
		identified = true;
		identified_at = sim_time();
		snprintf(reply, sizeof(reply), "LOG: Device Type: %s", model());
		sentence(reply);
	} else if (!strncmp(command, ":pulse,", 7)) {
		int duration = 0, consumed = 0;
		if (strchr(model(), 'P') && sscanf(command, ":pulse,%d*%n", &duration, &consumed) == 1 && consumed > 0 && command[consumed] == '\0' && duration > 0) {
			pulse_until = sim_time() + duration / 1000.0;
			event("PULSE_ON", command + 7);
			if (!strcmp(sim_profile, "drop-after-pulse")) {
				sim_running = 0;
			}
		} else {
			event("REJECT", command);
		}
	} else if (!strcmp(command, ":reboot*")) {
		gps_on = true;
	} else if (strchr(model(), 'M') && (sscanf(command, ":calp,%d*", &calp) == 1 || sscanf(command, ":calt,%d*", &calt) == 1 || sscanf(command, ":calh,%d*", &calh) == 1)) {
		calibration();
	} else if (strchr(model(), 'M') && !strcmp(command, ":calreset*")) {
		calp = calt = calh = 0;
		calibration();
	} else if (strchr(model(), 'M') && !strcmp(command, ":calget*")) {
		calibration();
	} else if (strchr(model(), 'M') && sscanf(command, ":mm,%d*", &mm) == 1) {
		calibration();
	} else if (strchr(model(), 'G') && sscanf(command, ":mg,%d*", &mg) == 1) {
		calibration();
	} else if (strchr(model(), 'G') && (!strcmp(command, ":gpson*") || !strcmp(command, ":rebootgps*"))) {
		gps_on = true;
	} else if (strchr(model(), 'G') && !strcmp(command, ":gpsoff*")) {
		gps_on = false;
	} else {
		event("REJECT", command);
	}
}

int main(int argc, char **argv) {
	// The journal is out-of-band test instrumentation, never a device reply.
	const char *path = getenv("INDIGO_MGBOX_SIMULATOR_EVENTS");
	if (path) {
		events = fopen(path, "w");
		if (!events) {
			perror("MGBox event journal");
			return 1;
		}
	}
	int result = sim_main(argc, argv);
	if (events) {
		fclose(events);
	}
	return result;
}
