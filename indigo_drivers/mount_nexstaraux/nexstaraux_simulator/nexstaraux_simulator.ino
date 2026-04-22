/*
 * NexStar AUX Protocol Simulator for ESP32 by Claude AI
 *
 * Implements a WiFi TCP server on port 2000 that simulates the Celestron
 * NexStar AUX binary protocol for testing indigo_mount_nexstaraux without
 * real hardware.
 *
 * Simulated devices:
 *   - AZM motor controller (address 0x10)
 *   - ALT motor controller (address 0x11)
 *
 * Supported commands:
 *   MC_GET_POSITION, MC_GOTO_FAST, MC_GOTO_SLOW, MC_SET_POSITION,
 *   MC_SLEW_DONE, MC_MOVE_POS, MC_MOVE_NEG,
 *   MC_SET_POS/NEG_GUIDERATE, MC_GET/SET_AUTOGUIDE_RATE, MC_GET_VER
 *
 * Hardware: Any ESP32 board (e.g. ESP32 DevKit, WEMOS D1 Mini32)
 *
 * Usage:
 *   1. Optionally change AP_SSID / AP_PASSWORD below
 *   2. Flash to ESP32 with Arduino IDE or PlatformIO
 *   3. Connect your computer to the "NexStarSim" WiFi network
 *   4. In INDIGO driver, set mount port to: nexstar://192.168.4.1
 *
 * Protocol packet format:
 *   [0x3b][len][src][dst][cmd][data...][checksum]
 *   len      = 3 + data_bytes  (counts src + dst + cmd + data)
 *   checksum = (~sum(bytes 1..len+1) + 1) & 0xFF
 *
 */

#include <WiFi.h>

// ---- Access point configuration ---- (edit these) --------------------------

//#define AP

#ifdef AP
static const char *AP_SSID     = "NexStarSim";   // Network name to create
static const char *AP_PASSWORD = "nexstar1234";  // Min 8 chars, or NULL for open
#else
static const char *WIFI_SSID     = "iPhone 15";
static const char *WIFI_PASSWORD = "Zongora2";
#endif
static const int   TCP_PORT    = 2000;
// ESP32 AP default IP is always 192.168.4.1

// ---- NexStar AUX protocol constants ----------------------------------------
#define STX                   0x3b

// Device addresses
#define ADDR_APP              0x20
#define ADDR_AZM              0x10
#define ADDR_ALT              0x11

// Motor controller commands
#define MC_GET_POSITION       0x01
#define MC_GOTO_FAST          0x02
#define MC_SET_POSITION       0x04
#define MC_SET_POS_GUIDERATE  0x06
#define MC_SET_NEG_GUIDERATE  0x07
#define MC_SLEW_DONE          0x13
#define MC_GOTO_SLOW          0x17
#define MC_MOVE_POS           0x24
#define MC_MOVE_NEG           0x25
#define MC_SET_AUTOGUIDE_RATE 0x46
#define MC_GET_AUTOGUIDE_RATE 0x47
#define MC_GET_VER            0xFE

// 24-bit position encoding: 0x000000 = 0°, 0x1000000 = 360°
#define POS_FULL   0x1000000UL
#define POS_MASK   0x00FFFFFFUL

// Simulated slew speeds in position-units per second
#define SLEW_RATE_FAST  (POS_FULL / 90)    // ~4 deg/s
#define SLEW_RATE_SLOW  (POS_FULL / 720)   // ~0.5 deg/s

// Simulated firmware version reported to the driver
#define FW_MAJOR  3
#define FW_MINOR  6

// ---- Axis state ------------------------------------------------------------
typedef struct {
	uint8_t     addr;          // ADDR_AZM or ADDR_ALT
	const char *name;          // "AZM" or "ALT" for log output
	uint32_t    position;      // Current 24-bit position
	uint32_t    target;        // Active slew target
	bool        slewing_fast;  // MC_GOTO_FAST in progress
	bool        slewing_slow;  // MC_GOTO_SLOW in progress
	uint8_t     guide_rate;    // Autoguide rate 0–255 (maps to 0–100%)
} Axis;

// Starting positions: AZM = 180° (HA = 12h, pointing south), ALT = 0° (horizon)
static Axis azm = { ADDR_AZM, "AZM", 0x800000, 0x800000, false, false, 0x80 };
static Axis alt = { ADDR_ALT, "ALT", 0x000000, 0x000000, false, false, 0x80 };

static unsigned long last_physics_ms = 0;

WiFiServer server(TCP_PORT);
WiFiClient client;

// ---- Checksum --------------------------------------------------------------
// Two's complement of the lower byte of the sum.
// Covers the slice buf[0..len-1].
static uint8_t nexstar_checksum(const uint8_t *buf, int len) {
	int sum = 0;
	for (int i = 0; i < len; i++) sum += buf[i];
	return (uint8_t)((~sum + 1) & 0xFF);
}

// ---- Send a response packet ------------------------------------------------
// Response format: [STX][len][src][dst][cmd][data...][checksum]
// In a response, src = the axis address, dst = ADDR_APP.
// len field = 3 + data_len  (counts src + dst + cmd + data).
static void send_reply(uint8_t src, uint8_t dst, uint8_t cmd, const uint8_t *data, int data_len) {
	if (!client || !client.connected()) return;
	uint8_t pkt[32];
	pkt[0] = STX;
	pkt[1] = (uint8_t)(data_len + 3);
	pkt[2] = src;
	pkt[3] = dst;
	pkt[4] = cmd;
	if (data && data_len > 0)
		memcpy(pkt + 5, data, data_len);
	// Checksum covers pkt[1 .. 4 + data_len]
	pkt[5 + data_len] = nexstar_checksum(pkt + 1, 4 + data_len);
	client.write(pkt, 6 + data_len);
}

// ---- Physics: move axis toward target --------------------------------------
static void update_axis(Axis &ax, unsigned long dt_ms) {
	if (!ax.slewing_fast && !ax.slewing_slow) return;
	uint32_t rate = ax.slewing_fast ? SLEW_RATE_FAST : SLEW_RATE_SLOW;
	uint32_t step = (uint32_t)((uint64_t)rate * dt_ms / 1000);
	if (step == 0) return;
	// Signed shortest-path difference around the 24-bit circle
	int32_t diff = (int32_t)((ax.target - ax.position) & POS_MASK);
	if (diff > (int32_t)(POS_FULL / 2)) diff -= (int32_t)POS_FULL;
	if ((uint32_t)abs(diff) <= step) {
		// Arrived
		ax.position    = ax.target;
		ax.slewing_fast = false;
		ax.slewing_slow = false;
		Serial.printf("[%s] Slew complete → pos=0x%06lX (%.2f°)\n", ax.name, (unsigned long)ax.position, ax.position * 360.0 / POS_FULL);
	} else {
		if (diff > 0)
			ax.position = (ax.position + step) & POS_MASK;
		else
			ax.position = (ax.position - step) & POS_MASK;
	}
}

// ---- Command dispatch for one axis -----------------------------------------
static void handle_command(Axis &ax, uint8_t cmd, const uint8_t *data, int dlen) {
	uint8_t reply[4] = { 0 };
	switch (cmd) {
		// Return current 24-bit position (3 bytes, big-endian)
		case MC_GET_POSITION:
			reply[0] = (ax.position >> 16) & 0xFF;
			reply[1] = (ax.position >>  8) & 0xFF;
			reply[2] =  ax.position        & 0xFF;
			send_reply(ax.addr, ADDR_APP, cmd, reply, 3);
			break;
		// Begin a fast or slow slew to a 24-bit target position
		case MC_GOTO_FAST:
		case MC_GOTO_SLOW:
			if (dlen >= 3) {
				ax.target = (((uint32_t)data[0] << 16) | ((uint32_t)data[1] <<  8) | (uint32_t)data[2]) & POS_MASK;
				ax.slewing_fast = (cmd == MC_GOTO_FAST);
				ax.slewing_slow = (cmd == MC_GOTO_SLOW);
				Serial.printf("[%s] %s → target=0x%06lX (%.2f°)\n", ax.name, cmd == MC_GOTO_FAST ? "GOTO_FAST" : "GOTO_SLOW", (unsigned long)ax.target, ax.target * 360.0 / POS_FULL);
			}
			send_reply(ax.addr, ADDR_APP, cmd, NULL, 0);
			break;
		// Sync: set current position without slewing
		case MC_SET_POSITION:
			if (dlen >= 3) {
				ax.position = (((uint32_t)data[0] << 16) | ((uint32_t)data[1] <<  8) | (uint32_t)data[2]) & POS_MASK;
				ax.target       = ax.position;
				ax.slewing_fast = false;
				ax.slewing_slow = false;
				Serial.printf("[%s] SET_POSITION → pos=0x%06lX (%.2f°)\n", ax.name, (unsigned long)ax.position, ax.position * 360.0 / POS_FULL);
			}
			send_reply(ax.addr, ADDR_APP, cmd, NULL, 0);
			break;
		// Poll whether a goto has completed: 0x00 = still moving, 0xFF = done
		case MC_SLEW_DONE:
			reply[0] = (ax.slewing_fast || ax.slewing_slow) ? 0x00 : 0xFF;
			send_reply(ax.addr, ADDR_APP, cmd, reply, 1);
			break;
		// Set tracking rate (2-byte) or stop tracking (3 zero bytes)
		case MC_SET_POS_GUIDERATE:
		case MC_SET_NEG_GUIDERATE: {
			bool stop = (dlen == 3 && data[0] == 0 && data[1] == 0 && data[2] == 0);
			if (stop) {
				Serial.printf("[%s] Tracking OFF\n", ax.name);
			} else if (dlen >= 2) {
				uint16_t rate = ((uint16_t)data[0] << 8) | data[1];
				const char *rname = (rate == 0xFFFF) ? "Sidereal" : (rate == 0xFFFE) ? "Solar" : (rate == 0xFFFD) ? "Lunar" : "Custom";
				Serial.printf("[%s] Tracking ON: %s (0x%04X)\n", ax.name, rname, rate);
			}
			send_reply(ax.addr, ADDR_APP, cmd, NULL, 0);
			break;
		}
		// Start or stop a variable-rate manual move (rate=0 stops)
		case MC_MOVE_POS:
		case MC_MOVE_NEG:
			if (dlen >= 1) {
				if (data[0] == 0) {
					ax.slewing_fast = false;
					ax.slewing_slow = false;
					Serial.printf("[%s] Manual move STOP\n", ax.name);
				} else {
					Serial.printf("[%s] Manual move %s rate=%d\n", ax.name, cmd == MC_MOVE_POS ? "+" : "-", data[0]);
				}
			}
			send_reply(ax.addr, ADDR_APP, cmd, NULL, 0);
			break;
		// Store autoguide rate (0–255 = 0–100%)
		case MC_SET_AUTOGUIDE_RATE:
			if (dlen >= 1) {
				ax.guide_rate = data[0];
				Serial.printf("[%s] Autoguide rate = %d (%.1f%%)\n", ax.name, ax.guide_rate, ax.guide_rate * 100.0 / 256.0);
			}
			send_reply(ax.addr, ADDR_APP, cmd, NULL, 0);
			break;
		// Return stored autoguide rate
		case MC_GET_AUTOGUIDE_RATE:
			reply[0] = ax.guide_rate;
			send_reply(ax.addr, ADDR_APP, cmd, reply, 1);
			break;
		// Return simulated firmware version (major, minor)
		case MC_GET_VER:
			reply[0] = FW_MAJOR;
			reply[1] = FW_MINOR;
			send_reply(ax.addr, ADDR_APP, cmd, reply, 2);
			break;
		default:
			Serial.printf("[%s] Unknown cmd 0x%02X — empty reply\n", ax.name, cmd);
			send_reply(ax.addr, ADDR_APP, cmd, NULL, 0);
			break;
	}
}

// ---- Parse and dispatch a complete received packet -------------------------
static void process_packet(const uint8_t *pkt, int total) {
	// Checksum covers pkt[1 .. total-2], stored at pkt[total-1]
	uint8_t cs = nexstar_checksum(pkt + 1, total - 2);
	if (cs != pkt[total - 1]) {
		Serial.printf("Checksum error: expected 0x%02X got 0x%02X\n", cs, pkt[total - 1]);
		return;
	}
	uint8_t        dst  = pkt[3];
	uint8_t        cmd  = pkt[4];
	const uint8_t *data = pkt + 5;
	int            dlen = (int)pkt[1] - 3;   // len field minus src+dst+cmd
	if (dlen < 0) dlen = 0;
	if (dst == ADDR_AZM) {
		handle_command(azm, cmd, data, dlen);
	} else if (dst == ADDR_ALT) {
		handle_command(alt, cmd, data, dlen);
	} else {
		Serial.printf("Unknown destination 0x%02X, ignoring\n", dst);
	}
}

// ---- Arduino setup ---------------------------------------------------------
void setup() {
	Serial.begin(115200);
	delay(500);
	Serial.println("");
	Serial.println("==============================");
	Serial.println("  NexStar AUX Simulator");
	Serial.println("==============================");
#ifdef AP
	WiFi.mode(WIFI_AP);
	WiFi.softAP(AP_SSID, AP_PASSWORD);
	Serial.printf("Access point \"%s\" started\n", AP_SSID);
	Serial.printf("IP address: %s\n", WiFi.softAPIP().toString().c_str());
	Serial.printf(">>> Connect to \"%s\", then set driver port to: nexstar://%s <<<\n\n", AP_SSID, WiFi.softAPIP().toString().c_str());
#else
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.print("Connecting to WiFi");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.printf("\nIP address: %s\n", WiFi.localIP().toString().c_str());
	Serial.printf(">>> Set driver port to: nexstar://%s <<<\n\n", WiFi.localIP().toString().c_str());
#endif
	server.begin();
	server.setNoDelay(true);
	last_physics_ms = millis();
}

// ---- Arduino loop ----------------------------------------------------------
#define RX_BUF_SIZE 64

static uint8_t rx_buf[RX_BUF_SIZE];
static int     rx_len = 0;

void loop() {
	// --- Physics update (every 50 ms) ---
	unsigned long now = millis();
	unsigned long dt  = now - last_physics_ms;
	if (dt >= 50) {
		update_axis(azm, dt);
		update_axis(alt, dt);
		last_physics_ms = now;
	}
	// --- Client management ---
	if (!client || !client.connected()) {
		if (client) {
			Serial.println("Client disconnected");
			client.stop();
			rx_len = 0;
		}
		WiFiClient nc = server.available();
		if (nc) {
			client = nc;
			client.setNoDelay(true);
			rx_len = 0;
			Serial.printf("Client connected from %s\n", client.remoteIP().toString().c_str());
		}
		return;
	}
	// --- Read and assemble packets ---
	while (client.available()) {
		uint8_t b = client.read();
		// Scan for STX sync byte at the start of each packet
		if (rx_len == 0) {
			if (b != STX) continue;
		}
		if (rx_len >= RX_BUF_SIZE) {
			Serial.println("RX overflow — resyncing");
			rx_len = 0;
			continue;
		}
		rx_buf[rx_len++] = b;
		// Once we have the length byte, we know the full packet size:
		// total = len_byte + 3  (STX + len_byte + len_byte bytes including checksum)
		if (rx_len >= 2) {
			int len_byte = rx_buf[1];
			if (len_byte < 3) {          // Minimum: src + dst + cmd, no data
				rx_len = 0;
				continue;
			}
			int expected = len_byte + 3;
			if (expected > RX_BUF_SIZE) {
				Serial.printf("Packet length %d too large — resyncing\n", expected);
				rx_len = 0;
				continue;
			}
			if (rx_len >= expected) {
				process_packet(rx_buf, expected);
				rx_len = 0;
			}
		}
	}
}
