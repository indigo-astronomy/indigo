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

int current_position = 0x0000;
int target_position = 0x0000;
float temperature = 0x0031;
int moving_status = 0x00;
int speed = 0x02;
int step_mode = 0x00;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
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

void print_hex(int value, int length) {
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
	if (Serial.available()) {
		String command = Serial.readStringUntil('#');
    if (command.startsWith(":")) {
      command = command.substring(1);      
  		if (command.equals("C")) {
  		} else if (command.equals("FG")) {
  			moving_status = 0x01;
  		} else if (command.equals("FQ")) {
  			moving_status = 0x00;
  		} else if (command.equals("GC")) {
  			// TBD
  			Serial.print("00#");
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
  		} else if (command.equals("+")) {
  			// TBD
  		} else if (command.equals("-")) {
  			// TBD
  		} else if (command.startsWith("PO")) {
  			// TBD
  		}
    }
	}
}
