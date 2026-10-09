// DMFC Pegasus focuser simulator for Arduino
//
// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

int motor_mode = 1;
float temperature = 22.4;
long position = 50;
long target = 50;
int led_status = 1;
int reverse = 0;
int disabled_encoder = 0;
int backlash_value = 100;
int speed = 400;
unsigned long last_step = 0;

// The motor travels at `speed` steps per second towards `target`; H stops it where it is.
void move_motor() {
  unsigned long now = millis();
  long delta = (long)((now - last_step) * speed / 1000);
  if (delta > 0) {
    last_step = now;
    if (position < target)
      position = position + delta < target ? position + delta : target;
    else if (position > target)
      position = position - delta > target ? position - delta : target;
  } else if (position == target) {
    last_step = now;
  }
}

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(100);
  while (!Serial)
    ;
}

// Replies end with a new line; C, G, H, M, S and W have no reply (DMFC-Serial-Command-Table.pdf).
void loop() {
  move_motor();
  if (!Serial.available())
    return;
  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command.equals("#")) {
    Serial.print("OK_DMFCN\n");
  } else if (command.equals("A")) {
    Serial.print("OK_DMFCN:2.6:");
    Serial.print(motor_mode);
    Serial.print(":");
    Serial.print(temperature);
    Serial.print(":");
    Serial.print(position);
    Serial.print(":");
    Serial.print(position != target ? 1 : 0);
    Serial.print(":");
    Serial.print(led_status);
    Serial.print(":");
    Serial.print(reverse);
    Serial.print(":");
    Serial.print(disabled_encoder);
    Serial.print(":");
    Serial.print(backlash_value);
    Serial.print("\n");
  } else if (command.equals("V")) {
    Serial.print("2.6\n");
  } else if (command.equals("B")) {
    Serial.print("B:");
    Serial.print((float)speed);
    Serial.print("\n");
  } else if (command.equals("L")) {
    Serial.print("L:");
    Serial.print(led_status);
    Serial.print("\n");
  } else if (command.equals("L:1")) {
    led_status = 0;
    Serial.print("L:0\n");
  } else if (command.equals("L:2")) {
    led_status = 1;
    Serial.print("L:1\n");
  } else if (command.startsWith("S:")) {
    int value = command.substring(2).toInt();
    if (value > 0)
      speed = value;
  } else if (command.startsWith("G:")) {
    target = position + command.substring(2).toInt();
  } else if (command.startsWith("M:")) {
    target = command.substring(2).toInt();
  } else if (command.startsWith("W:")) {
    target = position = command.substring(2).toInt();
  } else if (command.equals("H")) {
    target = position;
  } else if (command.startsWith("N:")) {
    reverse = command.substring(2).toInt();
    Serial.print("N:");
    Serial.print(reverse);
    Serial.print("\n");
  } else if (command.startsWith("R:")) {
    motor_mode = command.substring(2).toInt();
    Serial.print(motor_mode);
    Serial.print("\n");
  } else if (command.startsWith("E:")) {
    disabled_encoder = command.substring(2).toInt();
    Serial.print("E:");
    Serial.print(disabled_encoder);
    Serial.print("\n");
  } else if (command.startsWith("C:")) {
    backlash_value = command.substring(2).toInt();
  } else if (command.equals("T")) {
    Serial.print(temperature);
    Serial.print("\n");
  } else if (command.equals("P")) {
    Serial.print(position);
    Serial.print("\n");
  } else if (command.equals("I")) {
    Serial.print(position != target ? 1 : 0);
    Serial.print("\n");
  } else if (command.equals("X")) {
    Serial.print(position);
    Serial.print("\n");
  }
}
