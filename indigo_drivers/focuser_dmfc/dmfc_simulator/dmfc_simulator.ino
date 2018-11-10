// DMFC Pegasus focuser simulator for Arduino
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

int motor_mode = 0;
float temperature = 22.4;
int position = 50;
int moving_status = 0;
int led_status = 0;
int reverse = 0;
int disabled_encoder = 0;
int backlash_value = 100;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  if (command.equals("#")) {
    Serial.println("OK_DMFCN");
  } else if (command.equals("A")) {
    Serial.print("OK_DMFCN:2.6:");
    Serial.print(motor_mode);
    Serial.print(":");
    Serial.print(temperature);
    Serial.print(":");
    Serial.print(position);
    Serial.print(":");
    Serial.print(moving_status);
    Serial.print(":");
    Serial.print(led_status);
    Serial.print(":");
    Serial.print(reverse);
    Serial.print(":");
    Serial.print(disabled_encoder);
    Serial.print(":");
    Serial.println(backlash_value);
  } else if (command.equals("L")) {
    Serial.print("L:");
    Serial.println(led_status);
  } else if (command.equals("L:1")) {
    Serial.println("L:0");
    led_status = 0;
  } else if (command.equals("L:2")) {
    Serial.println("L:1");
    led_status = 1;
  } else if (command.startsWith("S:")) {
    Serial.println(command);
  } else if (command.startsWith("G:")) {
    position += command.substring(2).toInt();
  } else if (command.startsWith("M:")) {
    position = command.substring(2).toInt();
	} else if (command.startsWith("W:")) {
		position = command.substring(2).toInt();
  } else if (command.startsWith("H")) {
    Serial.println(command);
  } else if (command.startsWith("N:")) {
    reverse = command.substring(2).toInt();
    Serial.print("N:");
    Serial.println(reverse);
  } else if (command.startsWith("R:")) {
    motor_mode = command.substring(2).toInt();
    Serial.println(motor_mode);
  } else if (command.startsWith("E:")) {
    disabled_encoder = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("C:")) {
    backlash_value = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.equals("T")) {
    Serial.println(temperature);
  } else if (command.equals("P")) {
    Serial.println(position);
  } else if (command.equals("I")) {
    Serial.println(moving_status);
  }
}
