// Talon6 simulator for Arduino
//
// Copyright (c) 2021-2026 CloudMakers, s. r. o.
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
// The open end is the full travel in encoder ticks configured in bytes 36 to 38 of the configuration.
#define OPENED full_travel()

#define STOPPED 0
#define OPEN    1
#define CLOSE  -1

// A full travel takes TRAVEL_TIME ms whatever the speed of the board, the position follows the time.
#define TRAVEL_TIME 15000L

long position = CLOSED;
int direction = STOPPED;
unsigned long last_step = 0;
int last_action = 0;

uint8_t configuration[55] = { 
  0x80, 0x81, 0xB4, 0x80, 0x81, 0x8C, 0x80, 0x80, 0x82, 0x80, 0x80, 0xE4, 0x80, 0x80, 0x8E, 0x80, 0x80, 0x94, 0x80, 0x80, 0xF8, 0x80, 0x80, 0xBC, 0x80, 0x80, 0xB6, 0x80, 0x80, 0xBC, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x83, 0x86, 0xD0, 0x80, 0x80, 0x85, 0x80, 0x80, 0x80, 0x84, 0xA0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x9E
};

long full_travel() {
  return ((long)(configuration[36] & 0x7F) << 14) | ((configuration[37] & 0x7F) << 7) | (configuration[38] & 0x7F);
}

uint8_t status[21] = {
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(500);
  while (!Serial)
    ;
  Serial.print("\n\notro reset\n");
}

void loop() {
  unsigned long now = millis();
  long delta = (long)((now - last_step) * OPENED / TRAVEL_TIME);
  if (delta > 0) {
    last_step = now;
    if (direction == CLOSE) {
      position = position - delta > CLOSED ? position - delta : CLOSED;
    } else if (direction == OPEN) {
      position = position + delta < OPENED ? position + delta : OPENED;
    }
    if ((direction == CLOSE && position == CLOSED) || (direction == OPEN && position == OPENED))
      direction = STOPPED;
  }
  if (Serial.available()) {
    String command = Serial.readStringUntil('#');
    if (command.equals("&V%")) {
      Serial.print("&V3.20N--#");
    } else if (command.equals("&G%")) {
      if (direction == STOPPED) {
        if (position == CLOSED)
          status[0] = (0x80 | (1 << 4) | last_action);
        else if (position == OPENED)
          status[0] = (0x80 | (0 << 4) | last_action);
        else
          status[0] = (0x80 | (4 << 4) | last_action);
      } else if (direction == OPEN) {
          status[0] = (0x80 | (2 << 4) | last_action);
      } else if (direction == CLOSE) {
          status[0] = (0x80 | (3 << 4) | last_action);
      }
      status[1] = (0x80 | ((position >> 14) & 0x7F));
      status[2] = (0x80 | ((position >> 7) & 0x7F));
      status[3] = (0x80 | (position & 0x7F));
      status[4] = 0x86;
      status[5] = 0xC4;
      status[6] = 0x80;
      status[7] = 0x80;
      status[8] = 0x80;
      status[9] = 0x80;
      status[10] = 0x80;
      status[11] = 0x80;
      status[12] = 0x80;
      status[13] = 0x80;
      status[14] = (0x80 | (position == CLOSED ? 1 << 4 : 0) | (position == OPENED ? 1 << 3 : 0));
      status[15] = 0x80;
      status[16] = 0x80;
      status[17] = 0x84;
      status[18] = 0x80;
      status[19] = 0x80;
      long sum = 0;
      for (int i = 0; i < 20; i++)
        sum += status[i];
      status[20] = (uint8_t)(0x80 | -(sum & 0x7F));
      Serial.print("&G");
      Serial.write(status, 21);
      Serial.print("#");
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
      // 13 is a motor stall in the manual, a stop on request is the emergency stop 14
      direction = STOPPED;
      last_action = 14;
      Serial.print("&#");
    } else if (command.equals("&p%")) {
      Serial.print("&p");
      Serial.write(configuration, sizeof(configuration));
      Serial.print("#");      
    } else if (command.startsWith("&a")) {
      for (int i = 0; i < 55; i++) {
        configuration[i] = command.charAt(i + 2);
      }
      Serial.print("&#");
    } else {
      Serial.print("&ERROR#");
    }
  }
}
