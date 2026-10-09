// Optec TCF-S focuser simulator for Arduino
//
// Copyright (c) 2018-2026 CloudMakers, s. r. o.
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

// Optec TCF-S3 protocol (19200 8N1): 6-character commands without a terminator, replies end with LF CR.
// FInnnn/FOnnnn answer '*' when the move ends (as Optec's own driver waits for it) and serial input is
// ignored while the motor runs; FCENTR answers CENTER on arrival. The motor runs at 200 steps/s.
// FAMODE/FBMODE stream P=nnnn and T=±nn.n once a second until FQUIT1 or FMMODE.

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#define MAXIMUM 9999
#define STEP_MS 5

long position = 5000;
long target = 5000;
unsigned long last_step = 0, last_report = 0;
unsigned slope_a = 86;
unsigned slope_b = 86;
char slope_a_sign = '0';
char slope_b_sign = '0';
bool mode_a = false;
bool mode_b = false;
bool quiet = false;
// the reply sent when the running move ends: "*" or "CENTER"
const char *pending = NULL;
char command[7];
int used = 0;

void reply(const char *text) {
  Serial.print(text);
  Serial.print("\n\r");
}

void move_to(long requested, const char *done) {
  target = requested < 0 ? 0 : requested > MAXIMUM ? MAXIMUM : requested;
  last_step = millis();
  pending = done;
}

void setup() {
  Serial.begin(19200);
  while (!Serial)
    ;
}

void dispatch() {
  char buffer[16];
  if (!strcmp(command, "FMMODE")) {
    mode_a = mode_b = false;
    reply("!");
  } else if (!strcmp(command, "FQUIT0")) {
    quiet = false;
    reply("DONE");
  } else if (!strcmp(command, "FQUIT1")) {
    quiet = true;
    reply("DONE");
  } else if (mode_a || mode_b) {
    // in the automatic modes only FMMODE and FQUITn are accepted
  } else if (!strcmp(command, "FFMODE")) {
    reply("END");
  } else if (!strcmp(command, "FAMODE")) {
    mode_a = true;
    last_report = millis();
  } else if (!strcmp(command, "FBMODE")) {
    mode_b = true;
    last_report = millis();
  } else if (!strcmp(command, "FSLEEP")) {
    reply("ZZZ");
  } else if (!strcmp(command, "FWAKUP")) {
    reply("WAKE");
  } else if (!strcmp(command, "FPOSRO")) {
    sprintf(buffer, "P=%04ld", position);
    reply(buffer);
  } else if (!strcmp(command, "FTMPRO")) {
    reply("T=+24.5");
  } else if (!strcmp(command, "FCENTR")) {
    move_to(5000, "CENTER");
  } else if ((command[1] == 'I' || command[1] == 'O') && isdigit(command[2]) && isdigit(command[3]) && isdigit(command[4]) && isdigit(command[5])) {
    long steps = atol(command + 2);
    move_to(command[1] == 'I' ? position - steps : position + steps, "*");
  } else if (!strcmp(command, "FREADA")) {
    sprintf(buffer, "A=%04u", slope_a);
    reply(buffer);
  } else if (!strcmp(command, "FREADB")) {
    sprintf(buffer, "B=%04u", slope_b);
    reply(buffer);
  } else if (!strncmp(command, "FLA", 3)) {
    slope_a = atoi(command + 3);
    reply("DONE");
  } else if (!strncmp(command, "FLB", 3)) {
    slope_b = atoi(command + 3);
    reply("DONE");
  } else if (!strncmp(command, "FZAxx", 5)) {
    slope_a_sign = command[5];
    reply("DONE");
  } else if (!strncmp(command, "FZBxx", 5)) {
    slope_b_sign = command[5];
    reply("DONE");
  } else if (!strcmp(command, "FTxxxA")) {
    sprintf(buffer, "A=%c", slope_a_sign);
    reply(buffer);
  } else if (!strcmp(command, "FTxxxB")) {
    sprintf(buffer, "B=%c", slope_b_sign);
    reply(buffer);
  }
}

void loop() {
  unsigned long now = millis();
  while (position != target && now - last_step >= STEP_MS) {
    position += position < target ? 1 : -1;
    last_step += STEP_MS;
  }
  if (pending && position == target) {
    reply(pending);
    pending = NULL;
  }
  while (Serial.available()) {
    char c = Serial.read();
    if (pending) {
      // the controller does not listen while the motor runs
      continue;
    }
    command[used++] = c;
    command[used] = 0;
    if (used == 6) {
      dispatch();
      used = 0;
    }
  }
  if ((mode_a || mode_b) && !quiet && now - last_report >= 1000) {
    char buffer[16];
    sprintf(buffer, "P=%04ld", position);
    reply(buffer);
    reply("T=+24.5");
    last_report = now;
  }
}
