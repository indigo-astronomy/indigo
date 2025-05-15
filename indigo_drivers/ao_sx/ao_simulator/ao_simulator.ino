// SX AO simulator for Arduino
//
// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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

int limit = 50;
int ra = 0;
int dec = 0;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char command = Serial.read();
  char params[7] = { 0 };
  if (command == 'K' || command == 'R') {
    ra = dec = 0;
    Serial.write("K");
  } else if (command == 'G') {
    Serial.readBytes(params, 6);
    int steps = atoi(params + 1);
    switch (params[0]) {
      case 'N':
        dec += steps;
        if (dec > limit) {
          dec == limit;
          Serial.write("L");
        } else {
          Serial.write("G");
        }
        break;
      case 'S':
        dec -= steps;
        if (dec < -limit) {
          dec == -limit;
          Serial.write("L");
        } else {
          Serial.write("G");
        }
        break;
      case 'T':
        ra += steps;
        if (ra > limit) {
          ra == limit;
          Serial.write("L");
        } else {
          Serial.write("G");
        }
        break;
      case 'W':
        ra -= steps;
        if (ra < -limit) {
          ra == limit;
          Serial.write("L");
        } else {
          Serial.write("G");
        }
        break;
    }
  } else if (command == 'M') {
    Serial.readBytes(params, 6);
    Serial.write("M");
  } else if (command == 'L') {
    char state = 0x30;
    if (ra == limit) {
      state |= 0x08;
    } else if (ra == -limit) {
      state |= 0x02;
    }
    if (dec == limit) {
      state |= 0x04;
    } else if (dec == -limit) {
      state |= 0x01;
    }
    Serial.write(state);
  } else if (command == 'V') {
    Serial.write("V123");
  } else if (command == 'X') {
    Serial.write("Y");
  } else if (command == 'U') {
    Serial.write("Z");
  }
}
