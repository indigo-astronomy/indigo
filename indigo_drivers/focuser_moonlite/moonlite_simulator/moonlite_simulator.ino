// MoonLite focuser simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
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
//#define Serial SerialUSB
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

unsigned current_position = 0x8000;
unsigned target_position = 0x8000;
float temperature = 0x0031;
unsigned moving_status = 0x00;
unsigned speed = 0x02;
unsigned step_mode = 0x00;
int temperature_compensation = 0x00;
bool use_compensation = false;

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("MoonLite sim");
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

int parse_hex(String string) {
	int value = 0;
	int length = string.length();
	for (int i = 0; i < length; i++) {
		char ch = string.charAt(i);
		if (isDigit(ch))
			value = value * 16 + ch - 48;
		else if (isHexadecimalDigit(ch))
			value = value * 16 + ch - 55;
		else
			break;
	}
	return value;
}

void print_hex(unsigned value, int length) {
	char buffer[5];
	buffer[length] = 0;
	while (length--) {
		int ch = value % 16;
		buffer[length] = ch < 10 ? ch + 48 : ch + 55;
		value /= 16;
	}
	Serial.write(buffer);
  Serial.write('#');
}

void loop() {
	if (moving_status == 0x01) {
		if (target_position > current_position) {
			current_position++;
      delay(10);
		} else if (target_position < current_position) {
			current_position--;
      delay(10);
		} else {
			moving_status = 0x00;
		}
	}
#ifdef LCD
  char buffer[17];
  sprintf(buffer, "T:%05d C:%05d", target_position, current_position);
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  sprintf(buffer, "M:%02x S:%02x D:%02x", moving_status, step_mode, speed);
  lcd.setCursor(0, 1);
  lcd.print(buffer);
#endif
	if (Serial.available()) {
    if (Serial.read() == ':') {
  		String command = Serial.readStringUntil('#');
  		if (command.equals("C")) {
  		} else if (command.equals("FG")) {
  			moving_status = 0x01;
  		} else if (command.equals("FQ")) {
  			moving_status = 0x00;
  		} else if (command.equals("GC")) {
				print_hex(temperature_compensation, 2);
  		} else if (command.equals("GD")) {
  			print_hex(speed, 2);
  		} else if (command.equals("GH")) {
  			print_hex(step_mode, 2);
  		} else if (command.equals("GI")) {
  			print_hex(moving_status, 2);
  		} else if (command.equals("GN")) {
  			print_hex(target_position, 4);
  		} else if (command.equals("GP")) {
  			print_hex(current_position, 4);
  		} else if (command.equals("GT")) {
  			print_hex(temperature, 4);
  		} else if (command.equals("GV")) {
  			Serial.print("12#");
  		} else if (command.startsWith("SD")) {
  			speed = parse_hex(command.substring(2));
  		} else if (command.equals("SF")) {
  			step_mode = 0x00;
  		} else if (command.equals("SH")) {
  			step_mode = 0xFF;
  		} else if (command.startsWith("SN")) {
  			target_position = parse_hex(command.substring(2));
  		} else if (command.startsWith("SP")) {
  			current_position = parse_hex(command.substring(2));
			} else if (command.startsWith("SC")) {
				temperature_compensation = parse_hex(command.substring(2));
  		} else if (command.equals("+")) {
        use_compensation = true;
  		} else if (command.equals("-")) {
        use_compensation = false;
  		} else if (command.startsWith("PO")) {
  		}
    }
	}
}
