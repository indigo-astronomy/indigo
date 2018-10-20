// Optec focuser simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
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
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("FMMODE")) {
      mode_a = false;
      mode_b = false;
      Serial.println("!");
    } else if (command.startsWith("FFMODE")) {
      mode_a = false;
      mode_b = false;
      Serial.println("END");
    } else if (command.startsWith("FAMODE")) {
      mode_a = true;
      mode_b = false;
    } else if (command.startsWith("FBMODE")) {
      mode_a = false;
      mode_b = true;
    } else if (command.startsWith("FQUIT0")) {
      quiet = true;
      Serial.println("DONE");
    } else if (command.startsWith("FQUIT1")) {
      mode_a = false;
      mode_b = false;
      Serial.println("DONE");
    } else if (command.startsWith("FSLEEP")) {
      Serial.println("ZZZ");
    } else if (command.startsWith("FWAKUP")) {
      Serial.println("WAKE");
    } else if (command.startsWith("FPOSRO")) {
      sprintf(buffer, "P=%04u", position);
      Serial.println(buffer);
    } else if (command.startsWith("FCENTR")) {
      position = target = 5000;
      Serial.println("CENTER");
    } else if (command.startsWith("FI")) {
      target = position - atoi(command.substring(2).c_str());
      Serial.println("*");
    } else if (command.startsWith("FO")) {
      target = position + atoi(command.substring(2).c_str());
      Serial.println("*");
    } else if (command.startsWith("FTMPRO")) {
      Serial.println("T=+24.5");
    } else if (command.startsWith("FREADA")) {
      sprintf(buffer, "A=%04u", slope_a);
      Serial.println(buffer);
    } else if (command.startsWith("FREADB")) {
      sprintf(buffer, "B=%04u", slope_b);
      Serial.println(buffer);
    } else if (command.startsWith("FLA")) {
      slope_a = atoi(command.substring(3).c_str());
      Serial.println("DONE");
    } else if (command.startsWith("FLB")) {
      slope_b = atoi(command.substring(3).c_str());
      Serial.println("DONE");
    } else if (command.startsWith("FZAxx")) {
      slope_a_sign = command.charAt(5);
      Serial.println("DONE");
    } else if (command.startsWith("FZBxx")) {
      slope_b_sign = command.charAt(5);
      Serial.println("DONE");
    } else if (command.startsWith("FTxxxA")) {
      sprintf(buffer, "A=%c", slope_a_sign);
      Serial.println(buffer);
    } else if (command.startsWith("FTxxxB")) {
      sprintf(buffer, "B=%c", slope_b_sign);
      Serial.println(buffer);
    }
  }
  unsigned long m = millis();
  static unsigned long last_m = m;
  if (mode_a || mode_b) {
    bool report = false;
    if (quiet)
      report = m / 1000 > last_m / 1000;
    else
      report = m / 10 > last_m / 10;
    if (report) {
      sprintf(buffer, "P=%04u", position);
      Serial.println(buffer);
      Serial.println("T=+24.5");
    }
  } else if (target != position) {
    if (m / 10 > last_m / 10) {
      if (target < position)
        position--;
      else if (target > position)
        position++;
    }
  }
  last_m = m;
}
