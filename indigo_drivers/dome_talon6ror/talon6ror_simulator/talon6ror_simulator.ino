// Talon6 simulator for Arduino
//
// Copyright (c) 2021 CloudMakers, s. r. o.
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
#define LCD
#endif

#define CLOSED  0
#define OPENED 819100L

#define STOPPED 0
#define OPEN    1
#define CLOSE  -1

long position = CLOSED;
int direction = STOPPED;
int last_action = 0;

uint8_t configuration[51] = { 
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80
};

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(500);
  while (!Serial)
    ;
}

void loop() {
  if (position > CLOSED && direction == CLOSE)
    position--;
  else if (position < OPENED && direction == OPEN)
    position++;
  else
    direction = STOPPED;
  if (Serial.available()) {
    String command = Serial.readStringUntil('#');
    if (command.equals("&V%")) {
      Serial.print("&V1.00000#");
    } else if (command.equals("&G%")) {
      Serial.print("&G"); // 0
      if (direction == STOPPED) { // 1
        if (position == CLOSED)
          Serial.write(0x80 | (1 << 4) | last_action);
        else
          Serial.write(0x80 | (0 << 4) | last_action);
      } else if (direction == OPEN) {
          Serial.write(0x80 | (2 << 4) | last_action);
      } else if (direction == CLOSE) {
          Serial.write(0x80 | (3 << 4) | last_action);
      }
      Serial.write((char)(0x80 | ((position >> 14) & 0x7F))); // 2,3,4
      Serial.write((char)(0x80 | ((position >> 7) & 0x7F)));
      Serial.write((char)(0x80 | (position & 0x7F)));
      Serial.write(0x86); // 836/15 = 12.25V // 5,6
      Serial.write(0xC4);
      Serial.write(0x80); // 7,8,9
      Serial.write(0x80); 
      Serial.write(0x80); 
      Serial.write(0x80); // 10,11
      Serial.write(0x80);
      Serial.write(0x80); // 12,13
      Serial.write(0x80);
      Serial.write(0x80); // 14,15
      Serial.write(0x80 | (position == CLOSED ? 1 << 4 : 0) | (position == OPENED ? 1 << 3 : 0));
      Serial.print("00#");
    } else if (command.equals("&O%")) {
      direction = OPEN;
      last_action = 1;
      Serial.print("&#");
    } else if (command.equals("&C%")) {
      direction = CLOSE;
      last_action = 2;
      Serial.print("&#");
    } else if (command.equals("&P%")) {
      direction = CLOSE;
      last_action = 2;
      Serial.print("&#");
    } else if (command.equals("&S%")) {
      direction = STOPPED;
      last_action = 13;
      Serial.print("&#");
    } else if (command.equals("&p%")) {
      Serial.print("&p");
      Serial.write(configuration, sizeof(configuration));
      Serial.print("#");      
    } else if (command.startsWith("&a")) {
      for (int i = 0; i < 51; i++) {
        configuration[i] = command.charAt(i + 2);
      }
      Serial.print("&#");
    } else {
      Serial.print("&ERROR#");
    }
  }
}
