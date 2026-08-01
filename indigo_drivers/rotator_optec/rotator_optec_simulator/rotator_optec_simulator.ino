// Optec rotator simulator for Arduino
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

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

bool is_sleeping = false;
int current = 0;
int direction = 0;
int rate = 8;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void go(int target) {
  while (current != target) { 
    current = current > target ? current - 1 : current + 1;
    Serial.print("!");
    delay(rate);
  }
  Serial.print("F");
}

void loop() {
  char command[7];
  int count = Serial.readBytes(command, 6);
  command[count] = 0;
  if (is_sleeping) {
    if (!strcmp(command, "CWAKUP")) {
      is_sleeping = false;
      Serial.println("!");
    }
  } else {
    if (!strcmp(command, "CSLEEP")) {
      is_sleeping = true;
    } else if (!strcmp(command, "CCLINK")) {
      Serial.println("!");
    } else if (!strcmp(command, "CHOMES")) {
//      Serial.println("ER=1");        
      go(0);
    } else if (!strcmp(command, "CMREAD")) {
      Serial.println(direction);
    } else if (!strcmp(command, "CGETPA")) {
      if (current < 10) {
        Serial.print("00");
        Serial.println(current);
      } else if (current < 100) {
        Serial.print("0");
        Serial.println(current);
      } else {
        Serial.println(current);
      }
    } else if (!strncmp(command, "CD0", 3)) {
      direction = 0;
    } else if (!strncmp(command, "CD1", 3)) {
      direction = 1;
    } else if (!strncmp(command, "CPA", 3)) {
      int target = atoi(command + 3);
      if (target >= 360)
        Serial.println("ER=3");        
      else if (current == target)
        Serial.println("ER=2");        
      else
        go(target);
    } else if (!strncmp(command, "CT", 2)) {
      rate = atoi(command + 4);
      Serial.print("!");
    } else if (!strncmp(command, "CX", 2)) {
      Serial.print("!");
    }
  }
}
