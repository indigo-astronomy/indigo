// LX200 simulator for Arduino
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

#define LCD
#define DEC_PER_SEC 1440
#define RA_PER_SEC 96

#ifdef ARDUINO_SAM_DUE
#define Serial SerialUSB
#endif

#include <stdarg.h>

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

int date_day = 1;
int date_month = 1;
int date_year = 2018;

int time_hour = 22;
int time_minute = 15;
int time_second = 30;
int time_offset = 2;

char latitude[] = "+48*08";
char longitude[] = "+17*06";

bool high_precision = true;

long ra = 0;
long dec = 90L * 360000L;

long target_ra = 0;
long target_dec = 90L * 360000L;

bool is_slewing = false;
bool is_tracking = true;
bool is_parked = false;

void update_state() {
  static long unsigned last_millis = 0;
  static unsigned long time_lapse = 0;
  long unsigned current_millis = millis();
  long unsigned lapse = current_millis - last_millis;
  time_lapse += current_millis - last_millis;
  last_millis = current_millis;
  if (is_slewing) {
    long diff = target_ra - ra;
    if (abs(diff) < RA_PER_SEC * lapse) {
      ra = target_ra;
    } else {
      ra = ra + (diff > 0 ? RA_PER_SEC : -RA_PER_SEC) * lapse;
    }
    diff = target_dec - dec;
    if (abs(diff) < DEC_PER_SEC * lapse) {
      dec = target_dec;
    } else {
      dec = dec + (diff > 0 ? DEC_PER_SEC : -DEC_PER_SEC) * lapse;
    }
    if (ra == target_ra && dec == target_dec)
      is_slewing = false;
  } else if (!is_tracking) {
    ra = (ra + lapse) % (24L * 60L * 60L);
  }
  int s = time_second + time_lapse / 1000;
  int m = time_minute + s / 60;
  int h = time_hour + m / 60;
  time_second = s % 60;
  time_minute = m % 60;
  time_hour = h % 24;
  time_lapse %= 1000;
#ifdef LCD
  char buffer[17];
  sprintf(buffer, "%02d%02d%02d %02d%02d%02d %c%c", ra / 360000L, (ra / 6000L) % 60, (ra / 100L) % 60, dec / 360000L, (dec / 6000L) % 60, (dec / 100L) % 60, is_tracking ? 'T' : 't', is_slewing ? 'S' : 's');
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  sprintf(buffer, "%02d%02d%02d        %c%c", time_hour, time_minute, time_second, high_precision ? 'H' : 'h', is_parked ? 'P' : 'p');
  //sprintf(buffer, "%02d%02d%02d %02d%02d%02d", target_ra / 360000L, (target_ra / 6000L) % 60, (target_ra / 100L) % 60, target_dec / 360000L, (target_dec / 6000L) % 60, (target_dec / 100L) % 60);
  lcd.setCursor(0, 1);
  lcd.print(buffer);
#endif
}

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("LX200 simulator");
  lcd.setCursor(0, 1);
  lcd.print("Not connected");
#endif
  Serial.begin(9600);
  Serial.setTimeout(1000);
  while (!Serial)
    ;
}

void loop() {
  update_state();
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 6) {
      Serial.print("P");
    } else if (ch == ':') {
      char buffer[64];
      memset(buffer, 0, sizeof(buffer));
      Serial.readBytesUntil('#', buffer, sizeof(buffer));
      if (!strcmp(buffer, "GVP")) {
         Serial.print("Autostar#");
      } else if (!strcmp(buffer, "GVF")) {
         Serial.print("ETX Autostar|A|43Eg|Apr 03 2007@11:25:53#");
      } else if (!strcmp(buffer, "GVN")) {
         Serial.print("43Eg#");
      } else if (!strcmp(buffer, "GVD")) {
         Serial.print("Apr 03 2007#");
      } else if (!strcmp(buffer, "GVT")) {
         Serial.print("11:25:53#");
      } else if (!strncmp(buffer, "SC", 2)) {
         date_day = atoi(buffer + 5);
         date_month = atoi(buffer + 2);
         date_year = 2000 + atoi(buffer + 8);
         Serial.print("1Updating planetary data#                        #");
      } else if (!strcmp(buffer, "GC")) {
        sprintf(buffer, "%02d/%02d/%02d#", date_month, date_day, date_year % 100);
        Serial.print(buffer);
      } else if (!strncmp(buffer, "SG", 2)) {
         time_offset = atoi(buffer + 2);
         Serial.print("1");
      } else if (!strcmp(buffer, "GG")) {
        sprintf(buffer, "%+03d#", time_offset);
        Serial.print(buffer);
      } else if (!strncmp(buffer, "SL", 2)) {
         time_hour = atoi(buffer + 2);
         time_minute = atoi(buffer + 5);
         time_second = atoi(buffer + 8);
         Serial.print("1");
      } else if (!strcmp(buffer, "GL")) {
        sprintf(buffer, "%02d:%02d:%02d#", time_hour, time_minute, time_second);
        Serial.print(buffer);
      } else if (!strncmp(buffer, "Sg", 2)) {
        strcpy(longitude, buffer + 2);
        Serial.print("1");
      } else if (!strcmp(buffer, "Gg")) {
        Serial.print(longitude); Serial.print("#");
      } else if (!strncmp(buffer, "St", 2)) {
        strcpy(latitude, buffer + 2);
        Serial.print("1");
      } else if (!strcmp(buffer, "Gt")) {
        Serial.print(latitude); Serial.print("#");
      } else if (!strcmp(buffer, "P")) {
        if (high_precision) {
          Serial.print("LOW  PRECISION");
          high_precision = false;
        } else {
          Serial.print("HIGH PRECISION");
          high_precision = true;
        }
      } else if (!strncmp(buffer, "Sr", 2)) {
        if  (buffer[7] == '.') {
          target_ra = atol(buffer + 2) * 360000L + atof(buffer + 5) * 60000L;
        } else {
          target_ra = atol(buffer + 2) * 360000L + atol(buffer + 5) * 6000L + atol(buffer + 8) * 100L;
        }
        Serial.print("1");
      } else if (!strcmp(buffer, "Gr")) {
        if (high_precision) {
          sprintf(buffer, "%02d:%02d:%02d#", target_ra / 360000L, (target_ra / 6000L) % 60, (target_ra / 100L) % 60);
        } else {
          sprintf(buffer, "%02d:%04.1f#", target_ra / 360000L, ((target_ra / 600L) % 600) / 10.0);
        }
        Serial.print(buffer);
      } else if (!strcmp(buffer, "GR")) {
        if (high_precision) {
          sprintf(buffer, "%02d:%02d:%02d#", ra / 360000L, (ra / 6000L) % 60, (ra / 100L) % 60);
        } else {
          sprintf(buffer, "%02d:%04.1f#", ra / 360000L, ((ra / 600L) % 600) / 10.0);
        }
        Serial.print(buffer);
      } else if (!strncmp(buffer, "Sd", 2)) {
        if  (buffer[8] == 0) {
          target_dec = atol(buffer + 2) * 360000L;
          if (target_dec < 0)
            target_dec -= atol(buffer + 6) * 6000L;
          else
            target_dec += atol(buffer + 6) * 6000L;
        } else {
          target_dec = atol(buffer + 2) * 360000L;
          if (target_dec < 0)
            target_dec -= atol(buffer + 6) * 6000L + atol(buffer + 9) * 100L;
          else
            target_dec += atol(buffer + 6) * 6000L + atol(buffer + 9) * 100L;
        }
        Serial.print("1");
      } else if (!strcmp(buffer, "Gd")) {
        if (high_precision) {
          sprintf(buffer, "%+03d*%02d'%02d#", target_dec / 360000L, (abs(target_dec) / 6000L) % 60, (abs(target_dec) / 100L) % 60);
        } else {
          sprintf(buffer, "%+03d*%02d#", target_dec / 360000L, (abs(target_dec) / 600L) % 60);
        }
        Serial.print(buffer);
      } else if (!strcmp(buffer, "GD")) {
        if (high_precision) {
          sprintf(buffer, "%+03d*%02d'%02d#", dec / 360000L, (abs(dec) / 6000L) % 60, (abs(dec) / 100L) % 60);
        } else {
          sprintf(buffer, "%+03d*%02d#", dec / 360000L, (abs(dec) / 600L) % 60);
        }
        Serial.print(buffer);
      } else if (!strcmp(buffer, "CM")) {
        ra = target_ra;
        dec = target_dec;
        Serial.print("M31 EX GAL MAG 3.5 SZ178.0'#");
      } else if (!strcmp(buffer, "MS")) {
        is_slewing = true;
        Serial.print("0");
      } else if (!strcmp(buffer, "Q")) {
        is_slewing = false;
      } else if (!strcmp(buffer, "AP")) {
        is_tracking = true;
      } else if (!strcmp(buffer, "AL")) {
        is_tracking = false;
      } else if (!strcmp(buffer, "GW")) {
        if (is_tracking)
          Serial.print("PT1#");
        else if (is_parked)
          Serial.print("PNP#");         
        else
          Serial.print("PN1#");         
      } else if (!strcmp(buffer, "hP")) {
        target_ra = 0;
        target_dec = 90L * 360000L;
        is_tracking = false;
        is_slewing = true;
        is_parked = true;
      } else if (!strcmp(buffer, "TQ")) {
      } else if (!strcmp(buffer, "TS")) {
      } else if (!strcmp(buffer, "TL")) {
      } else if (!strcmp(buffer, "TM")) {
      }
    }
  }
}
