// USB_Dewpoint v2 simulator for Arduino
//
// Copyright (c) 2019 Rumen G. Bogdanovski
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
float temperature = 12.353;


void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char cmd[7];
  Serial.readBytes(cmd, 6);
  cmd[6] = 0;
  char response[80];
  String command = String(cmd);

  if (command.equals("SWHOIS")) {
    #ifdef V1
    Serial.println("UDP");
    #else
    Serial.println("UDP2(1446)");
    #endif

  } else if (command.equals("SEERAZ")) {
    Serial.println("EEPROM RESET");

  } else if (command.equals("SGETAL")) {
    #ifdef V1
    Serial.print("temp = ");
    Serial.println(temperature, 2);
    #else
    Serial.print("temp = ");
    Serial.println(temperature, 2);
    #endif
/*
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
    Serial.print(power5 ? 300 : 0);
    Serial.print(':');
    Serial.print(power6 ? 300 : 0);
#ifdef V2
    Serial.print(':');
    Serial.print(power7 ? 600 : 0);
    Serial.print(":0000010:");
#else
    Serial.print(":000001:");
#endif
    Serial.println(autodev);
  } else if (command.startsWith("PC")) {
    Serial.println("2.1:12:46");
  } else if (command.startsWith("PD:")) {
    autodev = command.charAt(3);
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
*/
  }
}
