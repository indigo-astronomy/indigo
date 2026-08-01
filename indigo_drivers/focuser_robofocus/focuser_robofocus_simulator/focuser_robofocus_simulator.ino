// RoboFocus focuser simulator for Arduino
//
// Copyright (c) 2020-2025 CloudMakers, s. r. o.
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

#define LCD

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

unsigned current_position = 2;
unsigned target_position = 2;
unsigned max_position = 0xFFFF;
char backlash[7] = "200020";
char configuration[7] = "000@@@";
char power[7] = "001111";
unsigned temperature = 600;

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("RoboFocus sim");
  lcd.setCursor(0, 1);
  lcd.print("Not connected");
#endif
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
#ifdef LCD
  lcd.clear();
#endif
}

void print_response(const char *format, ...) {
  char response[10] = { 0 };
  unsigned sum = 0;
  va_list args;
  va_start(args, format);
  vsnprintf(response, sizeof(response), format, args);
  va_end(args);
  for (int i = 0; i < 9; i++)
    sum += response[i];
  response[9] = sum & 0xFF;
  Serial.write(response, 9);
}

void loop() {
	if (target_position > current_position) {
    Serial.write('O');
		current_position++;
    if (target_position == current_position)
      print_response("FD%06d", current_position);
    delay(10);
	} else if (target_position < current_position) {
    Serial.write('I');
		current_position--;
    if (target_position == current_position)
      print_response("FD%06d", current_position);
    delay(10);
	}
#ifdef LCD
  char buffer[17];
  sprintf(buffer, "T:%05d C:%05d", target_position, current_position);
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  sprintf(buffer, "%s %s %s", backlash, configuration + 3, power + 2);
  lcd.setCursor(0, 1);
  lcd.print(buffer);
#endif
	if (Serial.available()) {
    char request[10] = { 0 };
    if (target_position != current_position) {
      target_position = current_position;
      print_response("FD%06d", current_position);
    }
    Serial.readBytes(request, 1);
    if (request[0] == 'F') {
      Serial.readBytes(request + 1, 8);
      request[9] = 0; // TODO check checksum
      switch (request[1]) {
        case 'V': {
          print_response("FV%06d", 0);
          break;
        }
        case 'G': {
          unsigned value = atoi(request + 3);
          if (value) {
            target_position = value;
            if (target_position > max_position)
              target_position = max_position;
          } else {
            print_response("FD%06d", current_position);
          }
          break;
        }
        case 'I': {
          unsigned value = atoi(request + 3);
          target_position = current_position - value;
          if (target_position < 1)
            target_position = 1;
          break;
        }
        case 'O': {
          unsigned value = atoi(request + 3);
          target_position = current_position + value;
          if (target_position > max_position)
            target_position = max_position;
          break;
        }
        case 'L': {
          unsigned value = atoi(request + 3);
          if (value)
            max_position = value;
          print_response("FL%06d", max_position);
          break;
        }
        case 'S': {
          unsigned value = atoi(request + 3);
          if (value)
            target_position = current_position = value;
          print_response("FD%06d", current_position);
          break;
        }
        case 'B': {
          unsigned value = atoi(request + 3);
          if (value)
            strncpy(backlash, request + 2, 6);
          print_response("FB%s", backlash);
          break;
        }
        case 'C': {
          unsigned value = atoi(request + 3);
          if (value)
            strncpy(configuration, request + 2, 6);
          print_response("FC%s", configuration);
          break;
        }
        case 'P': {
          unsigned value = atoi(request + 3);
          if (value)
            strncpy(power, request + 2, 6);
          print_response("FP%s", power);
          break;
        }
        case 'T': {
          print_response("FT%06d", temperature);
          break;
        }
      }
    }
	}
}
