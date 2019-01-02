// Lakeside focuser simulator for Arduino
//
// Copyright (c) 2018 CloudMakers, s. r. o.
// All rights reserved.
//
// Lakeside focuser command set is extracted from INDI driver written
// by Phil Shepherd with help of Peter Chance.
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

// ??# -> OK#           initial handshake
// ?D# -> Dnnnnn#       direction (0 = normal, 1 = reversed)
// ?T# -> Tnnnnn#       temperature C (temperature x2)
// ?K# -> Knnnnn#       temperature K (temperature x2)
// ?P# -> Pnnnnn#       position
// ?B# -> Bnnnnn#       backlash (0-255)
// ?I# -> Innnnn#       max travel (0-65536)
// ?S# -> Snnnnn#       step size (0-???)

// ?1# -> 1nnnnn#       slope 1 (steps/C)
// ?a# -> annnnn#       slope 1 direction (0 = +, 1 = -)
// ?c# -> cnnnnn#       slope 1 deadband (0-255)
// ?e# -> ennnnn#       slope 1 period (1-???)

// ?2# -> 2nnnnn#       slope 2 (steps/C)
// ?b# -> bnnnnn#       slope 2 direction (0 = +, 1 = -)
// ?d# -> dnnnnn#       slope 2 deadband (0-255)
// ?f# -> fnnnnn#       slope 1 period (1-???)

// CRBnnn# -> OK#       set backlash (0-255)
// CRSnnnnn# -> OK#     set step size (0-???)
// CRDn# -> OK#         set direction (0 = normal, 1 = reversed)
// CRgn# -> OK#         set active slope (1-2)
// CTx#                 set temperature tracking (N = enable, F = disable)

// CR1nnn# -> OK#       set slope 1 (steps/C)
// CRannn# -> OK#       set slope 1 direction  (0 = +, 1 = -)
// CRdnnn# -> OK#       set slope 1 deadband  (0-255)
// CRennn# -> OK#       set slope 1 period  (1-???)

// CR2nnn# -> OK#       set slope 2 (steps/C)
// CRbnnn# -> OK#       set slope 2 direction  (0 = +, 1 = -)
// CRennn# -> OK#       set slope 2 deadband  (0-255)
// CRfnnn# -> OK#       set slope 1 period  (1-???)

// CInnnnn#             move in (0-65536), sends Pnnnnn# while moving, #DONE once finished
// COnnnnn#             move out (0-65536), sends Pnnnnn# while moving, #DONE once finished
// CH#                  stop

// ... -> !#            unknown command



#define LCD

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

unsigned direction = 0;
unsigned temperature = 23;
unsigned current_position = 0x8000;
unsigned target_position = 0x8000;
unsigned backlash = 0;
unsigned max_travel = 0xFFFF;
unsigned step_size = 1;

unsigned active_slope = 1;
unsigned slope[2] = { 10, 20 };
unsigned slope_direction[2] = { 0, 1 };
unsigned slope_deadband[2] = { 5, 10 };
unsigned slope_period[2] = { 1, 10 };

void response(char *prefix, unsigned value) {
  char buffer[10];
  sprintf(buffer, "%s%u#", prefix, value);
  Serial.print(buffer);
}

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("LakesideASTRO");
#endif
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
#ifdef LCD
  lcd.clear();
#endif
}

void loop() {
  if (target_position > current_position) {
    current_position++;
    response("P", current_position);
    if (target_position == current_position)
      Serial.print("DONE#");
    else
      delay(30);
  } else if (target_position < current_position) {
    current_position--;
    response("P", current_position);
    if (target_position == current_position)
      Serial.print("DONE#");
    else
      delay(30);
  }
#ifdef LCD
  char buffer[17];
  sprintf(buffer, "T:%05d C:%05d", target_position, current_position);
  lcd.setCursor(0, 0);
  lcd.print(buffer);
#endif
  if (Serial.available()) {
    String command = Serial.readStringUntil('#');
    if (command.equals("??")) {
      Serial.print("OK#");
    } else if (command.equals("?D")) {
      response("D", direction);
    } else if (command.equals("?T")) {
      response("T", temperature * 2);
    } else if (command.equals("?P")) {
      response("P", current_position);
    } else if (command.equals("?B")) {
      response("B", backlash);
    } else if (command.equals("?I")) {
      response("I", max_travel);
    } else if (command.equals("?S")) {
      response("S", step_size);
    } else if (command.startsWith("CRB")) {
      backlash = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRS")) {
      step_size = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRg")) {
      active_slope = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRD")) {
      direction = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CT")) {
#ifdef LCD
      if (command.charAt(2) == 'N') {
        char buffer[17];
        sprintf(buffer, "S%d: %d %d %d %d", active_slope, slope[active_slope - 1], slope_direction[active_slope - 1], slope_deadband[active_slope - 1], slope_period[active_slope - 1]);
        lcd.setCursor(0, 1);
        lcd.print(buffer);
      } else {
        lcd.clear();
      }
#endif
    } else if (command.startsWith("CI")) {
      target_position -= atol(command.c_str() + 2);
    } else if (command.startsWith("CO")) {
      target_position += atol(command.c_str() + 2);
    } else if (command.equals("CH")) {
      target_position = current_position;
    } else if (command.equals("?1")) {
      response("1", slope[0]);
    } else if (command.equals("?a")) {
      response("a", slope_direction[0]);
    } else if (command.equals("?c")) {
      response("c", slope_deadband[0]);
    } else if (command.equals("?e")) {
      response("e", slope_period[0]);
    } else if (command.equals("?2")) {
      response("2", slope[1]);
    } else if (command.equals("?b")) {
      response("b", slope_direction[1]);
    } else if (command.equals("?d")) {
      response("d", slope_deadband[1]);
    } else if (command.equals("?f")) {
      response("f", slope_period[1]);
    } else if (command.startsWith("CR1")) {
      slope[0] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRa")) {
      slope_direction[0] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRc")) {
      slope_deadband[0] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRe")) {
      slope_period[0] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CR2")) {
      slope[1] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRb")) {
      slope_direction[1] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRd")) {
      slope_deadband[1] = atol(command.c_str() + 3);
      Serial.print("OK#");
    } else if (command.startsWith("CRf")) {
      slope_period[1] = atol(command.c_str() + 3);
      Serial.print("OK#");
    }
  }
}
