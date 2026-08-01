// Pegasus ultimate powerbox simulator for Arduino
//
// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

#define V2

bool power1 = true;
bool power2 = true;
bool power3 = true;
bool power4 = true;
byte power5 = 0;
byte power6 = 0;
#ifdef V2
byte power7 = 0;
byte power8 = 12;
#endif
bool usb1 = true;
bool usb2 = true;
bool usb3 = true;
bool usb4 = true;
bool usb5 = true;
bool usb6 = true;
bool hub = true;
byte autodev = 0;
bool led = false;

float temperature = 22.4;
int target = 50;
double position = 50;
int reverse = 0;
int backlash = 100;
int speed = 400;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  double step = speed / 100.0;
  if (target > position) {
    if (target - position < step)
      position = target;
    else
      position += step;
  } else if (target < position) {
    if (position - target < step)
      position = target;
    else
      position -= step;
  }
  String command = Serial.readStringUntil('\n');
  if (command.equals("P#")) {
#ifdef V2
    Serial.println("UPB2_OK");
#else
    Serial.println("UPB_OK");
#endif
  } else if (command.startsWith("PE:")) {
    power1 = command.charAt(3) == '1';
    power2 = command.charAt(4) == '1';
    power3 = command.charAt(5) == '1';
    power4 = command.charAt(6) == '1';
    Serial.println("PE:1");
#ifdef V2
	} else if (command.startsWith("PS")) {
		Serial.print("PS:");
		Serial.print(power1 ? "1" : "0");
		Serial.print(power2 ? "1" : "0");
		Serial.print(power3 ? "1" : "0");
		Serial.print(power4 ? "1" : "0");
		Serial.print(":");
		Serial.println((int)power8);
#endif
  } else if (command.startsWith("P1:")) {
    power1 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P2:")) {
    power2 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P3:")) {
    power3 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P4:")) {
    power4 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P5:")) {
    power5 = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("P6:")) {
    power6 = command.substring(3).toInt();
    Serial.println(command);
#ifdef V2
	} else if (command.startsWith("P7:")) {
		power7 = command.substring(3).toInt();
		Serial.println(command);
	} else if (command.startsWith("P8:")) {
		power8 = command.substring(3).toInt();
		Serial.println(command);
  } else if (command.startsWith("U1:")) {
    usb1 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("U2:")) {
    usb2 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("U3:")) {
    usb3 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("U4:")) {
    usb4 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("U5:")) {
    usb5 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("U6:")) {
    usb6 = command.charAt(3) == '1';
    Serial.println(command);
#endif
  } else if (command.startsWith("PU:")) {
    hub = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("PF")) {
    Serial.println("RBT");
  } else if (command.startsWith("PA")) {
#ifdef V2
    Serial.print("UPB2:12.2:0.0:0:23.2:59:14.7:");
#else
    Serial.print("UPB:12.2:0.0:0:23.2:59:14.7:");
#endif
    Serial.print(power1 ? '1' : '0');
    Serial.print(power2 ? '1' : '0');
    Serial.print(power3 ? '1' : '0');
    Serial.print(power4 ? '1' : '0');
    Serial.print(':');
#ifdef V2
    Serial.print(usb1 ? '1' : '0');
    Serial.print(usb2 ? '1' : '0');
    Serial.print(usb3 ? '1' : '0');
    Serial.print(usb4 ? '1' : '0');
    Serial.print(usb5 ? '1' : '0');
    Serial.print(usb6 ? '1' : '0');
#else
    Serial.print(hub ? '0' : '1');
#endif    
		Serial.print(':');
    Serial.print(power5);
    Serial.print(':');
    Serial.print(power6);
    Serial.print(':');
#ifdef V2
    Serial.print(power7);
    Serial.print(':');
#endif    
    Serial.print(power1 ? 200 : 0);
    Serial.print(':');
    Serial.print(power2 ? 200 : 0);
    Serial.print(':');
    Serial.print(power3 ? 200 : 0);
    Serial.print(':');
    Serial.print(power4 ? 200 : 0);
    Serial.print(':');
    Serial.print(power5 ? 350 : 0);
    Serial.print(':');
    Serial.print(power6 ? 380 : 0);
#ifdef V2
    Serial.print(':');
    Serial.print(power7 ? 700 : 0);
    Serial.print(":0000010:");
#else
    Serial.print(":000001:");
#endif
    Serial.println(autodev);
  } else if (command.startsWith("PC")) {
    Serial.println("2.1:12:46");
  } else if (command.startsWith("PD:")) {
    autodev = command.charAt(3) == '0' ? 0 : 1;
    Serial.println(command);
  } else if (command.equals("PV")) {
    Serial.println("1.0");
  } else if (command.startsWith("PZ:")) {
    power1 = power2 = power3 = power4 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("PL:")) {
    led = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.equals("SA")) {
    Serial.print((long)position);
    Serial.print(":");
    Serial.print(target == position ? '0' : '1');
    Serial.print(":");
    Serial.print(reverse);
    Serial.print(":");
    Serial.println(backlash);
  } else if (command.startsWith("SB:")) {
    backlash = command.substring(3).toInt();
		Serial.println(command);
  } else if (command.equals("SP")) {
    Serial.println((int)position);
	} else if (command.equals("SS")) {
		Serial.println((int)speed);
  } else if (command.startsWith("SH")) {
    target = (long)position;
    Serial.println("H:1");
  } else if (command.equals("ST")) {
    Serial.println(temperature);
  } else if (command.equals("SI")) {
    Serial.println(target == position ? '0' : '1');
  } else if (command.startsWith("SM:")) {
    target = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("SR:")) {
    reverse = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("SC:")) {
    target = position = command.substring(3).toInt();
  } else if (command.startsWith("SS:")) {
    speed = command.substring(3).toInt();
		Serial.println((int)speed);
  } else if (command.startsWith("SG:")) {
    target = position + command.substring(3).toInt();
    Serial.println(command);
  } else if (command.equals("SS")) {
    Serial.println(speed);
  }
}
