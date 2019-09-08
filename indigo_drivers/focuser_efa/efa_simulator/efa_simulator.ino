// PlaneWave EFA focuser simulator for Arduino
//
// Copyright (c) 2019 CloudMakers, s. r. o.
// All rights reserved.
//
// Lakeside focuser command set is extracted from INDI driver written
// by Phil Shepherd with help of Peter Chance.
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

#define LCD

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

unsigned long current_position = 0;
unsigned long target_position = 0;
unsigned long min_position = 0;
unsigned long max_position = 3799422;
bool stop_detect = true;
bool fans_on = false;

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("EFA");
#endif
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
#ifdef LCD
  lcd.clear();
#endif
}

void loop() {
  if (target_position > current_position && current_position < max_position) {
    current_position++;
		delay(30);
  } else if (target_position < current_position && current_position > min_position) {
    current_position--;
		delay(30);
  }
#ifdef LCD
  char buffer[17];
  sprintf(buffer, "T:%05d C:%05d", target_position, current_position);
  lcd.setCursor(0, 0);
  lcd.print(buffer);
#endif
  if (Serial.available() && Serial.read() == 0x3B) {
    char count = Serial.read();
    char packet[16] = { 0x3B, count };
    for (int i = 0; i <= count; i++)
      packet[i + 2] = Serial.read();
    if (packet[3] == 0x12) {
      switch (packet[4] & 0xFF) {
        case 0x01: // MTR_GET_POS
          packet[1] = 6;
          packet[5] = (current_position >> 16) & 0xFF;
          packet[6] = (current_position >> 8) & 0xFF;
          packet[7] = (current_position) & 0xFF;
          break;
        case 0x17: // MTR_GOTO_POS2
          target_position = (packet[5] & 0xFF) << 16 | (packet[6] & 0xFF) << 8 | (packet[7] & 0xFF);
          packet[1] = 4;
          packet[5] = 1;
          break;
        case 0x04: // MTR_OFFSET_CNT
          target_position = current_position = (packet[5] & 0xFF) << 16 | (packet[6] & 0xFF) << 8 | (packet[7] & 0xFF);
          packet[1] = 4;
          packet[5] = 1;
          break;
        case 0x13: // MTR_GOTO_OVER
          packet[1] = 4;
          packet[5] = target_position == current_position ? 0xFF : 0x00;
          break;
        case 0x1B: // MTR_SLEWLIMITMAX
          max_position = (packet[5] & 0xFF) << 16 | (packet[6] & 0xFF) << 8 | (packet[7] & 0xFF);
          packet[1] = 4;
          packet[5] = 1;
          break;
        case 0x1D: // MTR_SLEWLIMITGETMAX
          packet[1] = 6;
          packet[5] = (max_position >> 16) & 0xFF;
          packet[6] = (max_position >> 8) & 0xFF;
          packet[7] = (max_position) & 0xFF;
          break;
        case 0x24: // MTR_PMSLEW_RATE
          if (packet[5])
            target_position = max_position;
          else
            target_position = current_position;
          packet[1] = 4;
          packet[5] = 1;
          break;
        case 0x25: // MTR_NMSLEW_RATE
          if (packet[5])
            target_position = min_position;
          else
            target_position = current_position;
          packet[1] = 4;
          packet[5] = 1;
          break;
        case 0x26: // TEMP_GET - 2 or 3 bytes??
          packet[1] = 6;
          packet[6] = 0x5C;
          packet[7] = 0x01;
          break;
        case 0xEE: // MTR_GET_STOP_DETECT
          packet[1] = 4;
          packet[5] = stop_detect ? 1 : 0;
          break;
        case 0xEF: // MTR_STOP_DETECT
          stop_detect = packet[5] == 1;
          packet[1] = 3;
          break;
        case 0xFE: // GET_VERSION
          packet[1] = 5;
          packet[5] = 0x01;
          packet[6] = 0x05;
          break;
        default:
          packet[1] = 4;
          packet[5] = 0xFF;
          break;
      }
    } else if (packet[3] == 0x13) {
      switch (packet[4] & 0xFF) {
        case 0x27: // FANS_SET
          fans_on = packet[5] == 1;
          packet[1] = 4;
          packet[5] = 1;
          break;
        case 0x28: // FANS_GET
          packet[1] = 4;
          packet[5] = fans_on ? 0 : 3;
          break;
        default:
          packet[1] = 4;
          packet[5] = 0xFF;
          break;
      }
    }
    count = packet[1];
    char tmp = packet[2];
    packet[2] = packet[3];
    packet[3] = tmp;
    int sum = 0;
    for (int i = 0; i <= count; i++)
      sum += packet[i + 1];
    packet[count + 2] = (-sum) & 0xFF;
    Serial.write(packet, count + 3);
  }
}
