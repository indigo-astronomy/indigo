// RoboFocus focuser simulator for Arduino
//
// Copyright (c) 2020-2026 CloudMakers, s. r. o.
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

// RoboFocus serial protocol (manual for firmware 3.x, appendix 1), 9600 8N1. Every data set is nine bytes:
// 'F', a command letter, six characters and a checksum (the low byte of the sum of the first eight). Values are
// ASCII digits, except the duty cycle, step delay and step size of FC, which are binary. A frame with a bad
// checksum or illegal characters is ignored. During a move the controller sends 'I' or 'O' per step and the
// position FDnnnnnn when the move ends; any byte received during a move stops it, answered with the position.

// #define LCD

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

// 50 steps per second, the top of the manual's 10-50 ticks per second
#define TICK_MS 20

long position = 32000;
long target = 32000;
long maximum = 64000;
char backlash_direction = '2';
int backlash = 20;
uint8_t duty = 128, step_delay = 10, step_size = 4;
char power[4] = { '2', '2', '1', '1' };
int temperature = 600;
unsigned long last_tick = 0;
uint8_t frame[9];
int used = 0;
unsigned long frame_started = 0;

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.print("RoboFocus sim");
#endif
  Serial.begin(9600);
  while (!Serial)
    ;
}

uint8_t checksum(const uint8_t *data) {
  unsigned sum = 0;
  for (int i = 0; i < 8; i++)
    sum += data[i];
  return sum & 0xFF;
}

void send(const uint8_t payload[8]) {
  uint8_t out[9];
  memcpy(out, payload, 8);
  out[8] = checksum(out);
  Serial.write(out, 9);
}

void send_value(char command, long value) {
  char text[10];
  snprintf(text, sizeof(text), "F%c%06ld", command, value);
  send((const uint8_t *)text);
}

bool digits(const uint8_t *data, int count, long *value) {
  long result = 0;
  for (int i = 0; i < count; i++) {
    if (data[i] < '0' || data[i] > '9')
      return false;
    result = result * 10 + data[i] - '0';
  }
  *value = result;
  return true;
}

bool all_zero(const uint8_t *data) {
  for (int i = 0; i < 6; i++)
    if (data[i] != '0')
      return false;
  return true;
}

void start(long requested) {
  target = requested < 1 ? 1 : requested > maximum ? maximum : requested;
  last_tick = millis();
  if (target == position)
    send_value('D', position);
}

void dispatch() {
  long value;
  const uint8_t *data = frame + 2;
  switch (frame[1]) {
    case 'V':
      send_value('V', 3020);
      break;
    case 'G':
      if (digits(data, 6, &value)) {
        if (value)
          start(value);
        else
          send_value('D', position);
      }
      break;
    case 'I':
      if (digits(data, 6, &value))
        start(position - value);
      break;
    case 'O':
      if (digits(data, 6, &value))
        start(position + value);
      break;
    case 'S':
      if (digits(data, 6, &value)) {
        if (value)
          position = target = value;
        send_value('D', position);
      }
      break;
    case 'L':
      if (digits(data, 6, &value)) {
        if (value)
          maximum = value;
        send_value('L', maximum);
      }
      break;
    case 'P': {
      if (!all_zero(data)) {
        for (int i = 0; i < 4; i++)
          if (data[i + 2] == '1' || data[i + 2] == '2')
            power[i] = data[i + 2];
      }
      uint8_t payload[8] = { 'F', 'P', '0', '0', (uint8_t)power[0], (uint8_t)power[1], (uint8_t)power[2], (uint8_t)power[3] };
      send(payload);
      break;
    }
    case 'C': {
      if (!all_zero(data)) {
        if (data[3] > 250 || data[4] < 1 || data[4] > 64 || data[5] < 1 || data[5] > 64)
          return;
        duty = data[3];
        step_delay = data[4];
        step_size = data[5];
      }
      uint8_t payload[8] = { 'F', 'C', '0', '0', '0', duty, step_delay, step_size };
      send(payload);
      break;
    }
    case 'B': {
      if (!all_zero(data)) {
        if ((data[0] != '2' && data[0] != '3') || !digits(data + 1, 5, &value) || value > 255)
          return;
        backlash_direction = data[0];
        backlash = value;
      }
      char text[10];
      snprintf(text, sizeof(text), "FB%c%05d", backlash_direction, backlash);
      send((const uint8_t *)text);
      break;
    }
    case 'T':
      send_value('T', temperature);
      break;
  }
}

void loop() {
  unsigned long now = millis();
  if (position != target) {
    if (Serial.available()) {
      // any byte stops the move; the rest of a frame sent with it is dropped
      target = position;
      send_value('D', position);
      delay(20);
      while (Serial.available())
        Serial.read();
      used = 0;
      return;
    }
    while (position != target && now - last_tick >= TICK_MS) {
      bool outward = target > position;
      position += outward ? 1 : -1;
      Serial.write(outward ? 'O' : 'I');
      last_tick += TICK_MS;
      if (position == target)
        send_value('D', position);
    }
    return;
  }
  // a frame not completed within 100 ms is dropped
  if (used && now - frame_started > 100)
    used = 0;
  while (Serial.available()) {
    uint8_t c = Serial.read();
    if (used == 0) {
      if (c != 'F')
        continue;
      frame_started = now;
    }
    frame[used++] = c;
    if (used == 9) {
      used = 0;
      if (frame[8] == checksum(frame))
        dispatch();
    }
  }
}
