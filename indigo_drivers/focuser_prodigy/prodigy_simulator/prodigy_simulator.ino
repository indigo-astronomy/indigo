// Prodigy focuser simulator for Arduino
//
// Copyright (c) 2022 CloudMakers, s. r. o.
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

float temperature = 22.4;
int position = 50;
int moving_status = 0;
int backlash_value = 100;
int power_1 = 0;
int power_2 = 0;
int usb_1 = 0;
int usb_2 = 0;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  if (command.equals("#")) {
    Serial.println("OK_PRDG");
  } else if (command.equals("A")) {
    Serial.print("OK_PRDG:1.4:");
    Serial.print(1);
    Serial.print(":");
    Serial.print(temperature);
    Serial.print(":");
    Serial.print(position);
    Serial.print(":");
    Serial.print(moving_status);
    Serial.print(":");
    Serial.print(0);
    Serial.print(":");
    Serial.print(0);
    Serial.print(":");
    Serial.print(0);
    Serial.print(":");
    Serial.println(backlash_value);
  } else if (command.equals("D")) {
    Serial.print("D:");
    Serial.print(power_1);
    Serial.print(":");
    Serial.print(power_2);
    Serial.print(":");
    Serial.print(usb_1);
    Serial.print(":");
    Serial.println(usb_2);
  } else if (command.startsWith("S:")) {
    Serial.println(command);
  } else if (command.startsWith("G:")) {
    position += command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("M:")) {
    position = command.substring(2).toInt();
    Serial.println(command);
	} else if (command.startsWith("W:")) {
		position = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("H")) {
    Serial.println(0);
  } else if (command.startsWith("C:")) {
    backlash_value = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.equals("T")) {
    Serial.println(temperature);
  } else if (command.equals("P")) {
    Serial.println(position);
  } else if (command.equals("I")) {
    Serial.println(moving_status);
  } else if (command.equals("Z")) {
    position = 0;
    Serial.println("Z:1");
  } else if (command.startsWith("U:")) {
    usb_1 = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("J:")) {
    usb_2 = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("X:")) {
    power_1 = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("Y:")) {
    power_2 = command.substring(2).toInt();
    Serial.println(command);
  } else if (command.startsWith("Q")) {
  }
}
