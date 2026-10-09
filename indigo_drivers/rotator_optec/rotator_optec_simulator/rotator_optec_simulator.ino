// Optec rotator simulator for Arduino
//
// Copyright (c) 2022-2026 CloudMakers, s. r. o.
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

// Optec Pyxis (2-inch) serial protocol, manual 17645 section 4 (19200 8N1). Commands are six characters without a
// terminator. After power-up the Pyxis homes and waits awake in the serial loop; CWAKUP is answered only in sleep
// mode, and while asleep every other command is ignored. A move answers '!' for every motor step and 'F' when it
// ends: 5280 steps per revolution (about 15 per degree), the rate is the pulse delay in ms (8 = 120 ms per degree).
// While the motor runs the Pyxis accepts no command.

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#define STEPS_PER_DEGREE 15

bool sleeping = false;
int position = 0;
int target = 0;
int direction = 0;
int rate = 8;
int steps = 0;
bool moving = false;
unsigned long last_step = 0;
char command[7];
int used = 0;
unsigned long command_started = 0;

void setup() {
  Serial.begin(19200);
  while (!Serial)
    ;
}

void start(int requested) {
  target = requested;
  steps = 0;
  moving = true;
  last_step = millis();
}

void step() {
  unsigned long interval = rate > 0 ? rate : 1;
  while (moving && millis() - last_step >= interval) {
    last_step += interval;
    if (position == target) {
      moving = false;
      Serial.print("F");
      return;
    }
    Serial.print("!");
    if (++steps >= STEPS_PER_DEGREE) {
      steps = 0;
      position += position > target ? -1 : 1;
    }
  }
}

void dispatch() {
  if (sleeping) {
    if (!strcmp(command, "CWAKUP")) {
      sleeping = false;
      Serial.print("!\r\n");
    }
    return;
  }
  if (!strcmp(command, "CSLEEP")) {
    sleeping = true;
  } else if (!strcmp(command, "CCLINK")) {
    Serial.print("!\r\n");
  } else if (!strcmp(command, "CHOMES")) {
    // finds the home magnet and returns to position angle 0
    start(0);
  } else if (!strcmp(command, "CMREAD")) {
    Serial.print(direction);
    Serial.print("\r\n");
  } else if (!strcmp(command, "CGETPA")) {
    char reply[8];
    snprintf(reply, sizeof(reply), "%03d\r\n", position);
    Serial.print(reply);
  } else if (!strncmp(command, "CD", 2) && (command[2] == '0' || command[2] == '1')) {
    direction = command[2] - '0';
  } else if (!strncmp(command, "CPA", 3) && isdigit(command[3]) && isdigit(command[4]) && isdigit(command[5])) {
    int requested = atoi(command + 3);
    if (requested > 359)
      Serial.print("ER=3\r\n");
    else if (requested == position)
      Serial.print("ER=2\r\n");
    else
      start(requested);
  } else if (!strncmp(command, "CTxx", 4) && isdigit(command[4]) && isdigit(command[5])) {
    rate = atoi(command + 4);
    Serial.print("!");
  } else if (!strncmp(command, "CXxx", 4) && (command[4] == '0' || command[4] == '1') && command[5] >= '1' && command[5] <= '9') {
    // small steps; the position angle is not updated
    Serial.print("!");
  }
  // CWAKUP while awake is not recognized and gets no answer
}

void loop() {
  step();
  if (moving) {
    // no command is accepted until the move is completed
    while (Serial.available())
      Serial.read();
    used = 0;
    return;
  }
  if (used && millis() - command_started > 200)
    used = 0;
  while (Serial.available()) {
    if (used == 0)
      command_started = millis();
    command[used++] = Serial.read();
    if (used == 6) {
      command[6] = 0;
      used = 0;
      dispatch();
    }
  }
}
