// Optec rotator simulator for Arduino
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

bool is_sleeping = false;
int current = 0;
int direction = 1;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  String command = Serial.readStringUntil('\n');
  command = command.substring(0, command.length() - 1);
  if (is_sleeping) {
    if (command.equals("CWAKUP")) {
      is_sleeping = false;
      Serial.println("!");
    }
  } else {
    if (command.equals("CSLEEP")) {
      is_sleeping = true;
    } else if (command.equals("CCLINK")) {
      Serial.println("!");
    } else if (command.startsWith("CMREAD")) {
      Serial.println(direction == 1 ? "0" : "1");
    } else if (command.startsWith("CGETPA")) {
      if (current < 10) {
        Serial.print("00");
        Serial.println(current);
      } else if (current < 100) {
        Serial.print("0");
        Serial.println(current);
      } else {
        Serial.println(current);
      }
    } else if (command.startsWith("CD0")) {
      direction = 1;
    } else if (command.startsWith("CD1")) {
      direction = -1;
    } else if (command.startsWith("CPA")) {
      int target = atoi(command.substring(3).c_str());
      if (current == target) {
        Serial.println("ER=2");        
      } else {
        while (current != target) { 
          current = (current + direction + 360) % 360;
          Serial.print("!");
          delay(10);
        }
        Serial.print("F");
      }
    }
  }
}
