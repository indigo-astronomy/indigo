// Optec TCF-S focuser simulator for Arduino
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

unsigned position = 5000;
unsigned target = 5000;
unsigned slope_a = 86;
unsigned slope_b = 86;
char slope_a_sign = '0';
char slope_b_sign = '0';
bool mode_a = false;
bool mode_b = false;
bool quiet = false;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  char buffer[8];
  char command[7] = { 0 };
  if (Serial.available()) {
    Serial.readBytes(command, 6);
    if (!strcmp(command, "FMMODE")) {
      mode_a = false;
      mode_b = false;
      Serial.println("!");
    } else if (!strcmp(command, "FFMODE")) {
      mode_a = false;
      mode_b = false;
      Serial.println("END");
    } else if (!strcmp(command, "FAMODE")) {
      mode_a = true;
      mode_b = false;
    } else if (!strcmp(command, "FBMODE")) {
      mode_a = false;
      mode_b = true;
    } else if (!strcmp(command, "FQUIT0")) {
      quiet = false;
      Serial.println("DONE");
    } else if (!strcmp(command, "FQUIT1")) {
      quiet = true;
      Serial.println("DONE");
    } else if (!strcmp(command, "FSLEEP")) {
      Serial.println("ZZZ");
    } else if (!strcmp(command, "FWAKUP")) {
      Serial.println("WAKE");
    } else if (!strcmp(command, "FPOSRO")) {
      sprintf(buffer, "P=%04u", position);
      Serial.println(buffer);
    } else if (!strcmp(command, "FCENTR")) {
      position = target = 5000;
      Serial.println("CENTER");
    } else if (!strncmp(command, "FI", 2)) {
      target = position - atoi(command + 2);
      Serial.println("*");
    } else if (!strncmp(command, "FO", 2)) {
      target = position + atoi(command + 2);
      Serial.println("*");
    } else if (!strcmp(command, "FTMPRO")) {
      Serial.println("T=+24.5");
    } else if (!strcmp(command, "FREADA")) {
      sprintf(buffer, "A=%04u", slope_a);
      Serial.println(buffer);
    } else if (!strcmp(command, "FREADB")) {
      sprintf(buffer, "B=%04u", slope_b);
      Serial.println(buffer);
    } else if (!strncmp(command, "FLA", 3)) {
      slope_a = atoi(command + 3);
      Serial.println("DONE");
    } else if (!strncmp(command, "FLB", 3)) {
      slope_b = atoi(command + 3);
      Serial.println("DONE");
    } else if (!strncmp(command, "FZAxx", 5)) {
      slope_a_sign = command[5];
      Serial.println("DONE");
    } else if (!strncmp(command, "FZBxx", 5)) {
      slope_b_sign = command[5];
      Serial.println("DONE");
    } else if (!strcmp(command, "FTxxxA")) {
      sprintf(buffer, "A=%c", slope_a_sign);
      Serial.println(buffer);
    } else if (!strcmp(command, "FTxxxB")) {
      sprintf(buffer, "B=%c", slope_b_sign);
      Serial.println(buffer);
    }
  }
  unsigned long m = millis();
  static unsigned long last_m = m;
  if (target != position) {
    if (m / 10 > last_m / 10) {
      if (target < position)
        position--;
      else if (target > position)
        position++;
    }
  }
  if (mode_a || mode_b) {
    bool report = false;
    if (!quiet)
      report = m / 10 > last_m / 10;
    if (report) {
      sprintf(buffer, "P=%04u", position);
      Serial.println(buffer);
      Serial.println("T=+24.5");
    }
  }
  last_m = m;
}
