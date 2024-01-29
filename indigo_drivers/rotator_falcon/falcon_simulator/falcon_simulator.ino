// Pegasus Falcon rotator simulator for Arduino
//
// Copyright (c) 2023 CloudMakers, s. r. o.
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

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#define STEPS_PER_SEC 0.002

double position = 0;
double target = 0;
int moving = 0;
int reverse = 0;

void setup() {
	Serial.begin(9600);
	Serial.setTimeout(1000);
	while (!Serial)
		;
}

void update_state() {
  static long unsigned last_millis = 0;
  long unsigned current_millis = millis();
  long unsigned lapse = current_millis - last_millis;
  last_millis = current_millis;
  if (moving == 1) {
    long diff = target - position;
    if (abs(diff) < STEPS_PER_SEC * lapse) {
      position = target;
    } else {
      position = position + (diff > 0 ? STEPS_PER_SEC : -STEPS_PER_SEC) * lapse;
    }
    if (position == target) {
      moving = 0;
    }
  }
}

void loop() {
  update_state();
	String command = Serial.readStringUntil('\n');
	if (command.equals("F#")) {
		Serial.println("FR_OK");
	} else if (command.equals("FA")) {
		Serial.print("FR_OK:");
		Serial.print(position);
    Serial.print(":");
    Serial.print(moving);
		Serial.print(":0:0:");
		Serial.println(reverse);
	} else if (command.equals("FV")) {
		Serial.println("FV:1.2");
	} else if (command.equals("FD")) {
		Serial.print("FD:");
		Serial.println(position);
	} else if (command.startsWith("SD:")) {
		position = command.substring(3).toDouble();
    moving = 0;
		Serial.println(command);
	} else if (command.startsWith("MD:")) {
		target = command.substring(3).toDouble();
    moving = 1;
		Serial.println(command);
	} else if (command.equals("FH")) {
    moving = 0;
		Serial.println("FH:1");
	} else if (command.equals("FR")) {
		Serial.print("FR:");
    Serial.println(moving);
	} else if (command.startsWith("FN:")) {
		reverse = command.substring(3).toInt();
		Serial.println(command);
  } else if (command.startsWith("DR:")) {
    Serial.println(command);
	}
}
