// SteelDriveII focuser simulator for Arduino
//
// Copyright (c) 2019-2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Based on the SteelDrive II technical documentation 0.770 by Baader Planetarium.
//
// 19200 8N1, lines end with CR LF. Every character received is echoed, so each command line comes back before its
// answer. Commands start with "$BS"; with CRC_ENABLE every line carries "*XX", the CRC-8 of the text before it, and
// a command without a valid one is ignored. The motor travels at 500 steps/s; SUMMARY reports GOING_UP, GOING_DOWN,
// STOPPED or ZEROED. RESET and REBOOT restart the controller, which greets with "$BS Hello World!".

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#define STEP_US 2000

const uint8_t crc_array[256] = {
  0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
  0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
  0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
  0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
  0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
  0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
  0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
  0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
  0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
  0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
  0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
  0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
  0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
  0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
  0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
  0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};

bool use_crc = false;
char name[20] = "BP_SD_01";
long position = 1000, target = 1000, limit = 2000;
bool zeroing = false, zeroed = false;
unsigned long last_step = 0;
char line[128];
int used = 0;

struct parameter {
  const char *name;
  bool real;
  double minimum, maximum, value, initial;
};

parameter parameters[] = {
  { "FOCUS", false, 0, 2147483647, 1234, 1234 },
  { "JOGSTEPS", false, 1, 2147483647, 50, 50 },
  { "SINGLESTEPS", false, 1, 2147483647, 1, 1 },
  { "BKLGT", false, 0, 100, 50, 50 },
  { "TEMP0_OFS", true, -50, 50, 0, 0 },
  { "TEMP1_OFS", true, -50, 50, 0, 0 },
  { "TCOMP", false, 0, 1, 0, 0 },
  { "TCOMP_FACTOR", true, -100000, 100000, 2.5, 2.5 },
  { "TCOMP_PERIOD", false, 0, 2147483647, 1000, 1000 },
  { "TCOMP_DELTA", true, 0, 100, 0.5, 0.5 },
  { "TCOMP_SENSOR", false, 0, 2, 2, 2 },
  { "USE_ENDSTOP", false, 0, 1, 0, 0 },
  { "PWM", false, 0, 100, 50, 50 },
  { "PID_CTRL", false, 0, 1, 0, 0 },
  { "PID_SENSOR", false, 0, 2, 0, 0 },
  { "AMBIENT_SENSOR", false, 0, 1, 1, 1 },
  { "AUTO_DEW", false, 0, 1, 0, 0 },
  { "PID_TARGET", true, -50, 50, 0, 0 },
  { "PID_DEW_OFS", true, -50, 50, 0, 0 },
};
#define PARAMETERS (sizeof(parameters) / sizeof(parameters[0]))

parameter *find(const char *key) {
  for (unsigned i = 0; i < PARAMETERS; i++)
    if (!strcmp(parameters[i].name, key))
      return parameters + i;
  return NULL;
}

double get(const char *key) {
  return find(key)->value;
}

uint8_t crc8(const char *text, size_t length) {
  uint8_t crc = 0;
  for (size_t i = 0; i < length; i++)
    crc = crc_array[(uint8_t)text[i] ^ crc];
  return crc;
}

void send(const char *text) {
  Serial.print(text);
  if (use_crc && strcmp(text, "$BS Hello World!")) {
    char suffix[4];
    snprintf(suffix, sizeof(suffix), "*%02X", crc8(text, strlen(text)));
    Serial.print(suffix);
  }
  Serial.print("\r\n");
}

void reset_controller() {
  for (unsigned i = 0; i < PARAMETERS; i++)
    parameters[i].value = parameters[i].initial;
  strcpy(name, "BP_SD_01");
  limit = 2000;
  position = target = 1000;
  zeroing = zeroed = false;
}

void setup() {
  Serial.begin(19200);
  while (!Serial)
    ;
  send("$BS Hello World!");
}

void step() {
  unsigned long now = micros();
  while (position != target && now - last_step >= STEP_US) {
    position += position < target ? 1 : -1;
    last_step += STEP_US;
  }
  if (position == target && zeroing) {
    zeroing = false;
    zeroed = true;
  }
}

const char *state() {
  if (position != target)
    return target > position ? "GOING_UP" : "GOING_DOWN";
  return zeroed ? "ZEROED" : "STOPPED";
}

void format(char *text, size_t size, const parameter *p) {
  if (p->real)
    snprintf(text, size, "$BS STATUS %s:%.2f", p->name, p->value);
  else
    snprintf(text, size, "$BS STATUS %s:%ld", p->name, (long)p->value);
}

bool number(const char *text, double *value) {
  char *end;
  if (!*text)
    return false;
  *value = strtod(text, &end);
  return *end == 0;
}

void start(long requested) {
  target = requested < 0 ? 0 : requested > limit ? limit : requested;
  last_step = micros();
  zeroed = false;
}

void dispatch(char *command) {
  char reply[200];
  double value;
  double t0 = 22.45 + get("TEMP0_OFS"), t1 = 21.78 + get("TEMP1_OFS");
  if (!strcmp(command, "$BS CRC_ENABLE")) {
    use_crc = true;
    send("$BS OK");
  } else if (!strcmp(command, "$BS CRC_DISABLE")) {
    use_crc = false;
    send("$BS OK");
  } else if (!strcmp(command, "$BS GET VERSION")) {
    send("$BS STATUS VERSION:0.770");
  } else if (!strcmp(command, "$BS GET NAME")) {
    snprintf(reply, sizeof(reply), "$BS STATUS NAME:%s", name);
    send(reply);
  } else if (!strncmp(command, "$BS SET NAME:", 13) && command[13] && strlen(command + 13) <= 19) {
    strcpy(name, command + 13);
    send("$BS OK");
  } else if (!strcmp(command, "$BS GET POS")) {
    snprintf(reply, sizeof(reply), "$BS STATUS POS:%ld", position);
    send(reply);
  } else if (!strncmp(command, "$BS SET POS:", 12) && number(command + 12, &value) && value >= 0) {
    position = target = (long)value;
    zeroed = position == 0;
    send("$BS OK");
  } else if (!strcmp(command, "$BS GET LIMIT")) {
    snprintf(reply, sizeof(reply), "$BS STATUS LIMIT:%ld", limit);
    send(reply);
  } else if (!strncmp(command, "$BS SET LIMIT:", 14) && number(command + 14, &value) && value >= 0) {
    limit = (long)value;
    if (position > limit)
      position = target = limit;
    send("$BS OK");
  } else if (!strncmp(command, "$BS GO ", 7) && number(command + 7, &value)) {
    start((long)value);
    send("$BS OK");
  } else if (!strcmp(command, "$BS STOP")) {
    target = position;
    zeroing = zeroed = false;
    send("$BS OK");
  } else if (!strcmp(command, "$BS ZEROING")) {
    if (get("USE_ENDSTOP")) {
      start(0);
      zeroing = true;
    } else {
      position = target = 0;
      zeroed = true;
    }
    send("$BS OK");
  } else if (!strcmp(command, "$BS INFO")) {
    snprintf(reply, sizeof(reply), "$BS STATUS NAME:%s;POS:%ld;STATE:%s;LIMIT:%ld", name, position, state(), limit);
    send(reply);
  } else if (!strcmp(command, "$BS SUMMARY")) {
    snprintf(reply, sizeof(reply), "$BS STATUS NAME:%s;POS:%ld;STATE:%s;LIMIT:%ld;FOCUS:%ld;TEMP0:%.2f;TEMP1:%.2f;TEMP_AVG:%.2f;TCOMP:%ld;PWM:%ld", name, position, state(), limit, (long)get("FOCUS"), t0, t1, (t0 + t1) / 2, (long)get("TCOMP"), (long)get("PWM"));
    send(reply);
  } else if (!strcmp(command, "$BS GET TEMP0")) {
    snprintf(reply, sizeof(reply), "$BS STATUS TEMP0:%.2f", t0);
    send(reply);
  } else if (!strcmp(command, "$BS GET TEMP1")) {
    snprintf(reply, sizeof(reply), "$BS STATUS TEMP1:%.2f", t1);
    send(reply);
  } else if (!strcmp(command, "$BS RESET")) {
    send("$BS OK");
    send("$BS DEBUG:FACTORY RESET...");
    send("$BS DEBUG: LOADING DEFAULTS...");
    use_crc = false;
    reset_controller();
    send("$BS Hello World!");
  } else if (!strcmp(command, "$BS REBOOT")) {
    use_crc = false;
    position = target;
    delay(500);
    send("$BS Hello World!");
  } else if (!strncmp(command, "$BS GET ", 8) && find(command + 8)) {
    format(reply, sizeof(reply), find(command + 8));
    send(reply);
  } else if (!strncmp(command, "$BS SET ", 8) && strchr(command, ':')) {
    char *colon = strchr(command, ':');
    *colon = 0;
    parameter *p = find(command + 8);
    double maximum = p && !strcmp(p->name, "SINGLESTEPS") ? get("JOGSTEPS") : p ? p->maximum : 0;
    if (p && number(colon + 1, &value) && value >= p->minimum && value <= maximum && (p->real || value == (long)value)) {
      p->value = value;
      if (!strcmp(p->name, "JOGSTEPS") && get("SINGLESTEPS") > value)
        find("SINGLESTEPS")->value = value;
      if (!strcmp(p->name, "PWM"))
        find("PID_CTRL")->value = 0;
      send("$BS OK");
    } else {
      send("$BS ERROR: Unknown command!");
    }
  } else {
    send("$BS ERROR: Unknown command!");
  }
}

void process(char *text) {
  // every character is echoed, the CRC suffix included
  Serial.print(text);
  Serial.print("\r\n");
  if (strncmp(text, "$BS", 3))
    return;
  bool exception = !strncmp(text, "$BS RESET", 9) || !strncmp(text, "$BS REBOOT", 10) || !strncmp(text, "$BS CRC_DISABLE", 15);
  char *star = strrchr(text, '*');
  if (star) {
    if (strlen(star) != 3 || strtol(star + 1, NULL, 16) != crc8(text, star - text))
      return;
    *star = 0;
  } else if (use_crc && !exception) {
    return;
  }
  dispatch(text);
}

void loop() {
  step();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r')
      continue;
    if (c == '\n') {
      line[used] = 0;
      if (used)
        process(line);
      used = 0;
    } else if (used < (int)sizeof(line) - 1) {
      line[used++] = c;
    }
  }
}
