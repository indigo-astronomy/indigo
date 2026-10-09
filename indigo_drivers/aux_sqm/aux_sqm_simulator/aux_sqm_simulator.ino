// SQM simulator for Arduino
//
// Copyright (c) 2019-2026 CloudMakers, s. r. o.
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

char led[6] = "A5,00";

// Interval reporting, section 8.7 of the SQM-LU-DL manual: the meter sends a reading on its own every
// period seconds with its serial number appended. "P" sets the EEPROM and RAM periods, "p" only the
// RAM one; the EEPROM period is active from power-up.
unsigned long interval_eeprom = 0;
unsigned long interval_ram = 0;
unsigned long next_report = 0;

// The reading falls by 0.01 and the counts rise by one per record, like the C simulator.
int samples = 0;

void print_reading(char prefix, bool serial) {
  char line[80];
  double brightness = 20.70 - 0.01 * (samples % 100);
  snprintf(line, sizeof(line), "%c, %05.2fm,0000022921Hz,%010dc,0000000.000s, 039.4C%s", prefix, brightness, 20 + samples, serial ? ",00000413" : "");
  Serial.println(line);
}

void print_interval_settings() {
  char line[64];
  snprintf(line, sizeof(line), "I,%010lus,%010lus,00000000.00m,00000000.00m", interval_eeprom, interval_ram);
  Serial.println(line);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  while (!Serial)
    ;
  interval_ram = interval_eeprom;
  next_report = millis() + interval_ram * 1000;
}

void loop() {
  if (interval_ram > 0 && (long)(millis() - next_report) >= 0) {
    print_reading('r', true);
    next_report = millis() + interval_ram * 1000;
  }
  if (!Serial.available()) {
    return;
  }
  String command = Serial.readStringUntil('x');
  command.trim();
  if (command.equals("i")) {
    Serial.println("i,00000002,00000003,00000001,00000413");
  } else if (command.equals("r")) {
    print_reading('r', false);
    samples++;
  } else if (command.equals("u")) {
    print_reading('u', false);
    samples++;
  } else if (command.equals("I")) {
    print_interval_settings();
  } else if ((command.charAt(0) == 'p' || command.charAt(0) == 'P') && command.length() == 11) {
    interval_ram = command.substring(1).toInt();
    if (command.charAt(0) == 'P') {
      interval_eeprom = interval_ram;
    }
    next_report = millis() + interval_ram * 1000;
    print_interval_settings();
  } else if (command.equals("L5")) {
    Serial.println("L5,238");
  } else if (command.equals("A50")) {
    led[3] = '0';
  } else if (command.equals("A51")) {
    led[3] = '1';
  } else if (command.equals("A5d")) {
    led[4] = '0';
  } else if (command.equals("A5e")) {
    led[4] = '1';
  } else if (command.equals("A5")) {
    Serial.println(led);
  }
}
