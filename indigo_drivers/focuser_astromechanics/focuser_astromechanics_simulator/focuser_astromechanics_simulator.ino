// ASTROMECHANICS focuser simulator for Arduino
//
// Copyright (c) 2021-2025 CloudMakers, s. r. o.
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

#define LCD

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

unsigned position = 0;
unsigned aperture = 0;

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("ASTROMECHANICS");
#endif
  Serial.begin(38400);
  Serial.setTimeout(1000);
  randomSeed(analogRead(0));
  while (!Serial)
    ;
}

void loop() {
  char buffer[17];
#ifdef LCD
  sprintf(buffer, "P:%04d A:%02d", position, aperture);
  lcd.setCursor(0, 1);
  lcd.print(buffer);
#endif
  if (Serial.available()) {
    String command = Serial.readStringUntil('#');
    if (command.equals("P")) {
      sprintf(buffer, "%04d#\n", position);
      Serial.print(buffer);
    } else if (command.startsWith("M")) {
      position = atol(command.c_str() + 1);
    } else if (command.startsWith("A")) {
      aperture = atol(command.c_str() + 1);
    } else if (command.equals("V")) {
      double value = 20.0 + random(100) / 100.0;
      sprintf(buffer, "%4.2f#\n", value);
      Serial.print(buffer);
    }
  }
}
