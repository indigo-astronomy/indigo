// Prodigy focuser simulator for Arduino
//
// Copyright (c) 2022-2025 CloudMakers, s. r. o.
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

// Pegasus Astro Prodigy Microfocuser, serial command table for firmware >= 1.4 (19200 8N1, commands and replies
// end with a newline; the firmware's println adds a CR before it). The motor travels at the stored speed
// (steps/s); H stops it, Z parks it at 0, Q restarts the controller (no replies for a second).

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

float temperature = 22.4;
long position = 50;
long target = 50;
long speed = 400;
int backlash_value = 100;
int power_1 = 1;
int power_2 = 1;
int usb_1 = 1;
int usb_2 = 1;
unsigned long last_step = 0, rebooting_until = 0;
String line;

void setup() {
  Serial.begin(19200);
  while (!Serial)
    ;
}

void move_to(long requested) {
  target = requested;
  last_step = micros();
}

void step() {
  if (position == target) {
    return;
  }
  unsigned long interval = 1000000UL / (unsigned long)speed;
  unsigned long now = micros();
  while (position != target && now - last_step >= interval) {
    position += position < target ? 1 : -1;
    last_step += interval;
  }
}

void dispatch(String command) {
  if (command.equals("#")) {
    Serial.println("OK_PRDG");
  } else if (command.equals("A")) {
    Serial.print("OK_PRDG:1.4:1:");
    Serial.print(temperature, 1);
    Serial.print(":");
    Serial.print(position);
    Serial.print(":");
    Serial.print(position != target ? 1 : 0);
    Serial.print(":0:0:0:");
    Serial.println(backlash_value);
  } else if (command.equals("B")) {
    Serial.print("B:");
    Serial.println(speed);
  } else if (command.equals("D")) {
    Serial.print("D:");
    Serial.print(power_1);
    Serial.print(":");
    Serial.print(power_2);
    Serial.print(":");
    Serial.print(usb_1);
    Serial.print(":");
    Serial.println(usb_2);
  } else if (command.equals("V")) {
    Serial.println("1.4");
  } else if (command.startsWith("S:")) {
    speed = command.substring(2).toInt();
    speed = speed < 1 ? 1 : speed;
    Serial.println(command);
  } else if (command.startsWith("G:")) {
    move_to(position + command.substring(2).toInt());
    Serial.println(command);
  } else if (command.startsWith("M:")) {
    move_to(command.substring(2).toInt());
    Serial.println(command);
  } else if (command.startsWith("W:")) {
    position = target = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.equals("H")) {
    target = position;
    Serial.println(0);
  } else if (command.startsWith("C:")) {
    backlash_value = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.equals("T")) {
    Serial.println(temperature, 2);
  } else if (command.equals("P")) {
    Serial.println(position);
  } else if (command.equals("I")) {
    Serial.println(position != target ? 1 : 0);
  } else if (command.equals("Z")) {
    move_to(0);
    Serial.println("Z:1");
  } else if (command.startsWith("N:")) {
    Serial.println("N:0");
  } else if (command.startsWith("U:")) {
    usb_1 = command.substring(2).toInt() != 0;
    Serial.println(command);
  } else if (command.startsWith("J:")) {
    usb_2 = command.substring(2).toInt() != 0;
    Serial.println(command);
  } else if (command.startsWith("X:")) {
    power_1 = command.substring(2).toInt() != 0;
    Serial.println(command);
  } else if (command.startsWith("Y:")) {
    power_2 = command.substring(2).toInt() != 0;
    Serial.println(command);
  } else if (command.equals("Q")) {
    // the controller restarts: the motor stops and nothing answers for a second
    target = position;
    rebooting_until = millis() + 1000;
  }
}

void loop() {
  step();
  if (rebooting_until) {
    if ((long)(millis() - rebooting_until) < 0) {
      while (Serial.available())
        Serial.read();
      return;
    }
    rebooting_until = 0;
  }
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      line.trim();
      dispatch(line);
      line = "";
    } else if (line.length() < 32) {
      line += c;
    }
  }
}
