// Xagyl wheel simulator for Arduino
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

// The wheel turns one slot every STEP_TIME ms in the direction of the shortest path, as the
// manual describes for "G"; the real rotation time is not documented.
#define SLOTS 5
#define STEP_TIME 500

char current_filter = '1';
char target_filter = '1';
unsigned long last_step = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command[2];
  if (current_filter != target_filter && millis() - last_step >= STEP_TIME) {
    int forward = (target_filter - current_filter + SLOTS) % SLOTS;
    if (forward <= SLOTS - forward) {
      current_filter = current_filter == '0' + SLOTS ? '1' : current_filter + 1;
    } else {
      current_filter = current_filter == '1' ? '0' + SLOTS : current_filter - 1;
    }
    last_step = millis();
  }
  if (Serial.available() > 1) {
    Serial.readBytes(command, 2);
    switch (command[0]) {
      case 'I':
        switch (command[1]) {
          case '0':
            Serial.println("Xagyl FW5125VX");
            break;
          case '1':
            Serial.println("FW3.1.5");
            break;
          case '2':
            Serial.print("P");
            Serial.println(current_filter);
            break;
          case '3':
            Serial.println("S/N: 123456");
            break;
          case '8':
            Serial.print("FilterSlots ");
            Serial.println(SLOTS);
            break;
        }
        break;
      case 'G':
        if (command[1] >= '1' && command[1] <= '0' + SLOTS) {
          if (current_filter == target_filter) {
            last_step = millis();
          }
          target_filter = command[1];
        }
        break;
    }
  }
}
