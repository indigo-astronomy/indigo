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


bool power1 = true;
bool power2 = true;
bool power3 = true;
bool power4 = true;
byte power5 = 0;
byte power6 = 0;
bool power_dslr = true;
bool autodev = true;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  if (command.equals("P#")) {
    Serial.println("PPB_OK");
  } else if (command.startsWith("PE:")) {
    power1 = command.charAt(3) == '1';
    power2 = command.charAt(4) == '1';
    power3 = command.charAt(5) == '1';
    power4 = command.charAt(6) == '1';
    Serial.println("PE:1");
  } else if (command.startsWith("P1:")) {
    power1 = power2 = power3 = power4 = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P2:")) {
    power_dslr = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.startsWith("P5:")) {
    power5 = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("P6:")) {
    power6 = command.substring(3).toInt();
    Serial.println(command);
  } else if (command.startsWith("PF")) {
    Serial.println("RBT");
  } else if (command.startsWith("PA")) {
    Serial.print("PPB:12.2:");
    Serial.print((power1 || power2 || power3 || power4) * 2 + 3 * (power5 + power6) / 255 + power_dslr);
    Serial.print(".0:23.2:59:14.7:");
    Serial.print(power1 + power2 + power3 + power4 ? '1' : '0');
    Serial.print(':');
		Serial.print(power_dslr ? '1' : '0');
		Serial.print(':');
    Serial.print(power5);
    Serial.print(':');
    Serial.print(power6);
    Serial.print(':');
    Serial.println(autodev ? '1' : '0');
  } else if (command.startsWith("PD:")) {
    autodev = command.charAt(3) == '1';
    Serial.println(command);
  } else if (command.equals("PV")) {
    Serial.println("1.0");
  }
}
