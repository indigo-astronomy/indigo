// SteelDriveII focuser simulator for Arduino
//
// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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
// Based on SteelDrive II Focuser Technical documentation v0.733 by Baader Planetarium

#ifdef ARDUINO_SAM_DUE
#include <avr/dtostrf.h>
#define Serial SerialUSB
#endif

const char to_hex[] = "0123456789ABCDEF";

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

const char PROGMEM ok_reply[] = "$BS OK";

bool use_crc = false;
bool boot = true;
char name[16] = "BP_SD_01";
int bklgt = 50;
int use_endstop = 0;
int pos = 1000;
int limit = 2000;
int focus = 1234;
int jogstep = 50;
int singlestep = 1;
double temp0 = 22.45;
double temp1 = 21.78;
double temp0_ofs = 0;
double temp1_ofs = 0;
int tcomp = 0;
double tcomp_factor = 2.5;
int tcomp_period = 1000;
double tcomp_delta = 0.5;
int tcomp_sensor = 1000;
int pwm = 50;
int target = 0;
bool moving = false;
int pid_ctrl = 0;
double pid_target;
int pid_sensor = 0;
int ambient_sensor = 0;
double pid_dew_ofs = 0;
int auto_dew = 0;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command[64], response[64], *pnt;
  unsigned char crc = 0;
  if (boot) {
    Serial.println(F("$BS Hello World!"));
    boot = false;
  }
  if (moving) {
    if (target > pos) {
      pos++;
      delay(10);
    } else if (target < pos) {
      pos--;
      delay(10);
    } else {
      moving = false;
    }
  }
	if (Serial.available()) {
    pnt = command;
    while (true) {
      char ch = Serial.read();
      Serial.print(ch);
      if (ch == '\r')
        continue;
      if (ch == '\n')
        break;
      if (ch == '*')
         ch = 0;
      *pnt++ = ch;
    }
    *pnt = 0;
    response[0] = 0;
    if (random(10) == 0)
      Serial.println(F("$BS DEBUG: Initialization of temperature measurement failed for sensor #1"));
    if (!strcmp_PF(command, F("$BS GET VERSION"))) {
      strcpy_PF(response, F("$BS STATUS VERSION:0.700(Apr 5 2019)"));
    } else if (!strcmp_PF(command, F("$BS CRC_ENABLE"))) {
      use_crc = true;
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS CRC_DISABLE"))) {
      use_crc = false;
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS REBOOT"))) {
      delay(1000);
      boot = true;
      return;
    } else if (!strcmp_PF(command, F("$BS RESET"))) {
      use_crc = false;
      boot = true;
      strcpy(name, "BP_SD_01");
      bklgt = 50;
      use_endstop = 0;
      focus = 1234;
      jogstep = 50;
      singlestep = 1;
      Serial.println(ok_reply);
      Serial.println(F("$BS DEBUG:FACTORY RESET..."));
      Serial.println(F("$BS DEBUG: LOADING DEFAULTS..."));
      return;
    } else if (!strncmp_PF(command, F("$BS SET NAME:"), 13)) {
      strncpy(name, command + 13, sizeof(name));
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET NAME"))) {
      sprintf(response, "$BS STATUS NAME:%s", name);
    } else if (!strncmp_PF(command, F("$BS SET BKLGT:"), 14)) {
      bklgt = atoi(command + 14);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET BKLGT"))) {
      sprintf(response, "$BS STATUS BKLGT:%d", bklgt);
    } else if (!strncmp_PF(command, F("$BS SET USE_ENDSTOP:"), 20)) {
      use_endstop = atoi(command + 20);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET USE_ENDSTOP"))) {
      sprintf(response, "$BS STATUS USE_ENDSTOP:%d", use_endstop);
    } else if (!strcmp_PF(command, F("$BS INFO"))) {
			sprintf(response, "$BS STATUS NAME:%s;POS:%d;STATE:%s;LIMIT:%d", name, pos, moving ? (target > pos ? "GOING_UP" : "GOING_DOWN") : "STOPPED", limit);
    } else if (!strcmp_PF(command, F("$BS SUMMARY"))) {
      char temp0_str[6];
      char temp1_str[6];
      char temp_avg_str[6];
      dtostrf(temp0, 4, 2, temp0_str);
      dtostrf(temp1, 4, 2, temp1_str);
      dtostrf((temp0 + temp1) / 2, 4, 2, temp_avg_str);
      sprintf(response, "$BS STATUS NAME:%s;POS:%d;STATE:%s;LIMIT:%d;FOCUS:%d;TEMP0:%s;TEMP1:%s;TEMP_AVG:%s;TCOMP:%d;PWM:%d", name, pos, moving ? (target > pos ? "GOING_UP" : "GOING_DOWN") : "STOPPED", limit, focus, temp0_str, temp1_str, temp_avg_str, tcomp, pwm);
    } else if (!strcmp_PF(command, F("$BS ZEROING"))) {
      pos = 0;
      strcpy_PF(response, ok_reply);
    } else if (!strncmp_PF(command, F("$BS SET POS:"), 12)) {
      pos = atoi(command + 12);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET POS"))) {
      sprintf(response, "$BS STATUS POS:%d", pos);
    } else if (!strncmp_PF(command, F("$BS SET LIMIT:"), 14)) {
      limit = atoi(command + 14);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET LIMIT"))) {
      sprintf(response, "$BS STATUS LIMIT:%d", limit);
    } else if (!strncmp_PF(command, F("$BS SET FOCUS:"), 14)) {
      focus = atoi(command + 14);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET FOCUS"))) {
      sprintf(response, "$BS STATUS FOCUS:%d", focus);
    } else if (!strncmp_PF(command, F("$BS SET JOGSTEPS:"), 17)) {
      jogstep = atoi(command + 17);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET JOGSTEPS"))) {
      sprintf(response, "$BS STATUS JOGSTEPS:%d", jogstep);
    } else if (!strncmp_PF(command, F("$BS SET SINGLESTEPS:"), 21)) {
      singlestep = atoi(command + 21);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET SINGLESTEPS"))) {
      sprintf(response, "$BS STATUS SINGLESTEPS:%d", singlestep);
    } else if (!strncmp_PF(command, F("$BS SET TCOMP:"), 14)) {
      tcomp = atoi(command + 14);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TCOMP"))) {
      sprintf(response, "$BS STATUS TCOMP:%d", tcomp);
    } else if (!strncmp_PF(command, F("$BS SET TCOMP_FACTOR:"), 21)) {
      tcomp_factor = atof(command + 21);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TCOMP_FACTOR"))) {
      char str[6];
      dtostrf(tcomp_factor, 5, 2, str);
      sprintf(response, "$BS STATUS TCOMP_FACTOR:%s", str);
    } else if (!strncmp_PF(command, F("$BS SET TCOMP_PERIOD:"), 21)) {
      tcomp_period = atof(command + 21);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TCOMP_PERIOD"))) {
      sprintf(response, "$BS STATUS TCOMP_PERIOD:%d", tcomp_period);      
    } else if (!strncmp_PF(command, F("$BS SET TCOMP_DELTA:"), 21)) {
      tcomp_delta = atoi(command + 21);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TCOMP_DELTA"))) {
      char str[6];
      dtostrf(tcomp_delta, 5, 2, str);
      sprintf(response, "$BS STATUS TCOMP_DELTA:%s", str);      
    } else if (!strncmp_PF(command, F("$BS SET TCOMP_SENSOR:"), 21)) {
      tcomp_sensor = atoi(command + 21);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TCOMP_SENSOR"))) {
      sprintf(response, "$BS STATUS TCOMP_SENSOR:%d", tcomp_sensor);      
    } else if (!strncmp_PF(command, F("$BS GO "), 7)) {
      target = atoi(command + 7);
      if (target < 0)
        target = 0;
      else if (target > limit)
        target = limit;
      moving = true;
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS STOP"))) {
      moving = false;
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TCOMP0"))) {
      char str[6];
      dtostrf(temp0, 4, 2, str);
      sprintf(response, "$BS STATUS TCOMP0:%s", str);      
    } else if (!strcmp_PF(command, F("$BS GET TCOMP1"))) {
      char str[6];
      dtostrf(temp1, 4, 2, str);
      sprintf(response, "$BS STATUS TCOMP1:%s", str);      
    } else if (!strncmp_PF(command, F("$BS SET TEMP0_OFS:"), 18)) {
      temp0_ofs = atof(command + 18);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TEMP0_OFS"))) {
      char str[6];
      dtostrf(temp0_ofs, 5, 2, str);
      sprintf(response, "$BS STATUS TEMP0_OFS:%s", str);
    } else if (!strncmp_PF(command, F("$BS SET TEMP1_OFS:"), 18)) {
      temp1_ofs = atof(command + 18);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET TEMP1_OFS"))) {
      char str[6];
      dtostrf(temp1_ofs, 5, 2, str);
      sprintf(response, "$BS STATUS TEMP1_OFS:%s", str);
    } else if (!strncmp_PF(command, F("$BS SET PID_CTRL:"), 17)) {
      pid_ctrl = atoi(command + 17);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET PID_CTRL"))) {
      sprintf(response, "$BS STATUS PID_CTRL:%d", pid_ctrl);
    } else if (!strncmp_PF(command, F("$BS SET PID_TARGET:"), 19)) {
      pid_target = atof(command + 19);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET PID_TARGET"))) {
      char str[6];
      dtostrf(pid_target, 5, 2, str);
      sprintf(response, "$BS STATUS PID_TARGET:%s", str);
    } else if (!strncmp_PF(command, F("$BS SET PID_SENSOR:"), 19)) {
      pid_sensor = atoi(command + 19);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET PID_SENSOR"))) {
      sprintf(response, "$BS STATUS PID_SENSOR:%d", pid_sensor);
    } else if (!strncmp_PF(command, F("$BS SET PWM:"), 12)) {
      pwm = atoi(command + 12);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET PWM"))) {
      sprintf(response, "$BS STATUS PWM:%d", pwm);
    } else if (!strncmp_PF(command, F("$BS SET AMBIENT_SENSOR:"), 23)) {
      ambient_sensor = atoi(command + 23);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET AMBIENT_SENSOR"))) {
      sprintf(response, "$BS STATUS AMBIENT_SENSOR:%d", ambient_sensor);
    } else if (!strncmp_PF(command, F("$BS SET PID_DEW_OFS:"), 20)) {
      pid_dew_ofs = atof(command + 19);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET PID_DEW_OFS"))) {
      char str[6];
      dtostrf(pid_dew_ofs, 5, 2, str);
      sprintf(response, "$BS STATUS PID_DEW_OFS:%s", str);
    } else if (!strncmp_PF(command, F("$BS SET AUTO_DEW:"), 18)) {
      auto_dew = atoi(command + 18);
      strcpy_PF(response, ok_reply);
    } else if (!strcmp_PF(command, F("$BS GET AUTO_DEW"))) {
      sprintf(response, "$BS STATUS AUTO_DEW:%d", auto_dew);
    } else {
			strcpy_PF(response, F("$BS ERROR: Unknown command!"));
    }

    if (use_crc) {
      pnt = response;
      while (*pnt) {
        crc = crc_array[*pnt++ ^ crc];
      }
      *pnt++ = '*';
      *pnt++ = to_hex[crc / 16];
      *pnt++ = to_hex[crc % 16];
      *pnt = 0;
    }
    Serial.println(response);    
	}
}
