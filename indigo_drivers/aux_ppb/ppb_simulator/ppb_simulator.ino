// Pegasus pocket powerbox simulator for Arduino
//
// Copyright (c) 2019 CloudMakers, s. r. o.
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

//#define PPBA

bool power_1234 = true;
byte power_5 = 0;
byte power_6 = 0;
bool power_dslr = true;
bool autodev = true;
#ifdef PPBA
bool power_alert = 0;
int power_adj = 5;
int dew_aggr = 210;
#endif

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  if (command.equals("P#")) {
#ifdef PPBA
    Serial.println("PPBA_OK");
#else
    Serial.println("PPB_OK");
#endif
  } else if (command.startsWith("PE:")) {
    power_1234 = command.charAt(5) == '1';
    power_dslr = command.charAt(6) == '1';
    Serial.println("PE:1");
  } else if (command.startsWith("P1:")) {
    power_1234 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P2:")) {
    int value = command.substring(3).toInt();
    switch (value) {
      case 0:
        power_dslr = false;
        break;
      case 1:
        power_dslr = true;
        break;
#ifdef PPBA
      default:
        power_dslr = true;
        power_adj = value;
        break;
#endif
    }
    Serial.println(command);
  } else if (command.startsWith("P3:")) {
    power_5 = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("P4:")) {
    power_6 = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("PF")) {
  } else if (command.startsWith("PA")) {
#ifdef PPBA
    Serial.print("PPBA:12.2:");
#else
    Serial.print("PPB:12.2:");
#endif
    Serial.print((power_1234 || power_dslr || power_5 || power_6) * 65);
    Serial.print(".0:23.2:59:14.7:");
    Serial.print(power_1234 ? '1' : '0');
    Serial.print(':');
		Serial.print(power_dslr ? '1' : '0');
		Serial.print(':');
    Serial.print(power_5);
    Serial.print(':');
    Serial.print(power_6);
    Serial.print(':');
#ifdef PPBA
    Serial.print(autodev ? '1' : '0');
    Serial.print(':');
    Serial.print(power_alert);
    Serial.print(':');
    Serial.println(power_adj);
#else    
    Serial.println(autodev ? '1' : '0');
#endif
  } else if (command.startsWith("PL:")) {
    Serial.println(command);
  } else if (command.equals("PV")) {
    Serial.println("1.5");
#ifdef PPBA
  } else if (command.startsWith("DA:")) {
    dew_aggr = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("PD:")) {
    autodev = command.charAt(3) == '1';
    Serial.print("PD:");
    Serial.println(dew_aggr);
#else    
  } else if (command.startsWith("PD:")) {
    autodev = command.charAt(3) == '1';
    Serial.println(command);
#endif
  }
}
