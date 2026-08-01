// RainbowAstro simulator for Arduino
//
// Copyright (c) 2020-2025 CloudMakers, s. r. o.
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

#define DISPLAY_INIT()
#define DISPLAY_BEGIN()
#define DISPLAY_TEXT(line, text)
#define DISPLAY_TEXTF(line, ...)
#define DISPLAY_END()
#ifdef ARDUINO_SAM_DUE
#ifdef LCD
#include <LiquidCrystal.h>
static LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
static char __buffer__[32];
#define DISPLAY_INIT() lcd.begin(16, 2);
#define DISPLAY_BEGIN() 
#define DISPLAY_TEXT(line, text) lcd.setCursor(0, line); lcd.print(text);
#define DISPLAY_TEXTF(line, format, ...) sprintf(__buffer__, format, __VA_ARGS__); lcd.setCursor(0, line); lcd.print(__buffer__);
#define DISPLAY_END()
#endif
#endif
#ifdef ARDUINO_TTGO_T1
#pragma message ARDUINO_TTGO_T1
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
static char __buffer__[32];
#define DISPLAY_INIT() tft.init(); tft.setRotation(1); tft.setTextSize(2); tft.setTextColor(TFT_GREEN, TFT_BLACK)
#define DISPLAY_BEGIN() tft.fillScreen(TFT_BLACK)
#define DISPLAY_TEXT(line, text) tft.drawString(text, 0, 32 * line)
#define DISPLAY_TEXTF(line, format, ...) sprintf(__buffer__, format, __VA_ARGS__); tft.drawString(__buffer__, 0, 32 * line)
#define DISPLAY_END()
#endif
#ifdef ARDUINO_WIFI_KIT_32
#pragma message ARDUINO_WIFI_KIT_32
#include "heltec.h"
static char __buffer__[32];
#define DISPLAY_INIT() Heltec.begin(true, false, true); Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT); Heltec.display->setFont(ArialMT_Plain_10)
#define DISPLAY_BEGIN() Heltec.display->clear()
#define DISPLAY_TEXT(line, text) Heltec.display->drawString(0, 16 * line, text)
#define DISPLAY_TEXTF(line, format, ...) sprintf(__buffer__, format, __VA_ARGS__); Heltec.display->drawString(0, 16 * line, __buffer__)
#define DISPLAY_END() Heltec.display->display()
#endif

#define DEC_PER_SEC (1440 * 10)
#define RA_PER_SEC (96 * 10)

int date_day = 1;
int date_month = 1;
int date_year = 2018;

int time_hour = 22;
int time_minute = 15;
int time_second = 30;
int time_offset = 2;

char latitude[16] = "+48*08'10";
char longitude[16] = "-17*06'20";

long ra = 0;
long dec = 90L * 3600000L;

long target_ra = 0;
long target_dec = 90L * 3600000L;

int ra_slew = 0;
int dec_slew = 0;

char tracking_rate = '0';
char slew_rate = 'M';

char guide_rate[4] = "0.3";

bool is_slewing = false;
bool is_tracking = false;
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
    if (ra == target_ra && dec == target_dec) {
      is_slewing = false;
      if (is_parked)
        Serial.print(":CHO#");
      else
        Serial.print(":MM0#");
    }
  } else if (!is_tracking) {
    ra = (ra + lapse) % (24L * 60L * 60000L);
  }
  if (ra_slew || dec_slew) {
    long rate;
    if (slew_rate == 'G')
      rate = 10 * lapse;
    else if (slew_rate == 'C')
      rate = 50 * lapse;
    else if (slew_rate == 'M')
      rate = 500 * lapse;
    else
      rate = 1500 * lapse;
    ra += ra_slew * rate / 15;
    ra = (ra + 24L * 3600000L) % (24L * 3600000L);
    dec += dec_slew * rate;
    if (dec < -90L * 3600000L)
      dec = -90L * 3600000L; 
    if (dec > 90L * 3600000L)
      dec = 90L * 3600000L; 
  }
  int s = time_second + time_lapse / 1000;
  int m = time_minute + s / 60;
  int h = time_hour + m / 60;
  time_second = s % 60;
  time_minute = m % 60;
  time_hour = h % 24;
  time_lapse %= 1000;
  DISPLAY_BEGIN();
  DISPLAY_TEXTF(0, "%02ld%02ld%02ld %02ld%02ld%02ld %c%c", ra / 3600000L, (ra / 60000L) % 60, (ra / 1000L) % 60, dec / 3600000L, (abs(dec) / 60000L) % 60, (abs(dec) / 1000L) % 60, is_tracking ? 'T' : 't', is_slewing ? 'S' : 's');
  DISPLAY_TEXTF(1, "%02d%02d%02d %c %c%c %c %c", time_hour, time_minute, time_second, slew_rate, (ra_slew < 0 ? 'E' : ra_slew > 0 ? 'W' : '0'), (dec_slew < 0 ? 'S' : dec_slew > 0 ? 'N' : '0'), tracking_rate, is_parked ? 'P' : 'p');
  DISPLAY_END();
}

void setup() {
  DISPLAY_INIT();
  DISPLAY_BEGIN();
  DISPLAY_TEXT(0, "RainbowAstro");
  DISPLAY_END();
  Serial.begin(115200);
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
    } else if (ch == ':' || ch == '>') {
      char buffer[64];
      memset(buffer, 0, sizeof(buffer));
      Serial.readBytesUntil('#', buffer, sizeof(buffer));
      if (!strcmp(buffer, "GL")) {
        sprintf(buffer, ":GL%02d:%02d:%02d#", time_hour, time_minute, time_second);
        Serial.print(buffer);
      } else if (!strncmp(buffer, "SL", 2)) {
        time_hour = atoi(buffer + 2);
        time_minute = atoi(buffer + 5);
        time_second = atoi(buffer + 8);
      } else if (!strcmp(buffer, "GG")) {
        sprintf(buffer, ":GG%+02d#", -time_offset);
        Serial.print(buffer);
      } else if (!strncmp(buffer, "SG", 2)) {
        time_offset = -atoi(buffer + 2);
      } else if (!strcmp(buffer, "GC")) {
        sprintf(buffer, ":GC%02d/%02d/%02d#", date_month, date_day, date_year % 100);
        Serial.print(buffer);
      } else if (!strncmp(buffer, "SC", 2)) {
        date_month = atoi(buffer + 2);
        date_day = atoi(buffer + 5);
        date_year = atoi(buffer + 8) + 2000;
      } else if (!strncmp(buffer, "Sg", 2)) {
        strcpy(longitude, buffer + 2);
      } else if (!strcmp(buffer, "Gg")) {
        Serial.print(":Gg");
        Serial.print(longitude);
        Serial.print(".0#");
      } else if (!strncmp(buffer, "St", 2)) {
        strcpy(latitude, buffer + 2);
      } else if (!strcmp(buffer, "Gt")) {
        Serial.print(":Gt");
        Serial.print(latitude);
        Serial.print(".0#");
      } else if (!strncmp(buffer, "Sr", 2)) {
        target_ra = atol(buffer + 2) * 3600000L + atol(buffer + 5) * 60000L + atof(buffer + 8) * 1000L;
        Serial.print("1");
      } else if (!strcmp(buffer, "GR")) {
        sprintf(buffer, ":GR%02ld:%02ld:%02ld.0#", ra / 3600000L, (ra / 60000L) % 60, (ra / 1000L) % 60);          
        Serial.print(buffer);
      } else if (!strncmp(buffer, "Sd", 2)) {
        target_dec = atol(buffer + 2) * 3600000L;
        if (target_dec < 0)
          target_dec -= atol(buffer + 6) * 60000L + atof(buffer + 9) * 1000L;
        else
          target_dec += atol(buffer + 6) * 60000L + atof(buffer + 9) * 1000L;
        Serial.print("1");
      } else if (!strcmp(buffer, "GD")) {
         sprintf(buffer, ":GD%+03ld*%02ld'%02ld.0#", dec / 3600000L, (abs(dec) / 60000L) % 60, (abs(dec) / 1000L) % 60);
         Serial.print(buffer);
      } else if (!strcmp(buffer, "MS")) {
        is_slewing = true;
      } else if (!strcmp(buffer, "Ch")) {
        target_ra = 0;
        target_dec = 90L * 3600000L;
        is_slewing = true;
        is_parked = true;
        is_tracking = false;
      } else if (!strcmp(buffer, "Q")) {
        if (is_slewing) {
          Serial.print(":MME#");
          is_slewing = false;
        }
        ra_slew = 0;
        dec_slew = 0;
      } else if (!strcmp(buffer, "Qe") || !strcmp(buffer, "Qw")) {
        ra_slew = 0;
      } else if (!strcmp(buffer, "Qn") || !strcmp(buffer, "Qs")) {
        dec_slew = 0;
      } else if (!strcmp(buffer, "CtR")) {
        tracking_rate = '0';
      } else if (!strcmp(buffer, "CtS")) {
        tracking_rate = '1';
      } else if (!strcmp(buffer, "CtM")) {
        tracking_rate = '2';
      } else if (!strcmp(buffer, "TtU")) {
        tracking_rate = '3';
      } else if (!strcmp(buffer, "RG")) {
        slew_rate = 'G';
      } else if (!strcmp(buffer, "RC")) {
        slew_rate = 'C';
      } else if (!strcmp(buffer, "RM")) {
        slew_rate = 'M';
      } else if (!strcmp(buffer, "RS")) {
        slew_rate = 'S';
      } else if (!strcmp(buffer, "Mn")) {
        dec_slew = 1;
      } else if (!strcmp(buffer, "Ms")) {
        dec_slew = -1;
      } else if (!strcmp(buffer, "Mw")) {
        ra_slew = 1;
      } else if (!strcmp(buffer, "Me")) {
        ra_slew = -1;
      } else if (!strncmp(buffer, "Ck", 2)) {
        ra = atof(buffer + 2) * 3600000L;
        dec = atof(buffer + 9) * 3600000L;
      } else if (!strncmp(buffer, "CU0=", 4)) {
        strncpy(guide_rate, buffer + 4, 3);
      } else if (!strcmp(buffer, "CU0")) {
        Serial.print(":CU0=");
        Serial.print(guide_rate);
        Serial.print("#");
      } else if (!strcmp(buffer, "AV")) {
        //Serial.print(":AV190905#");
        Serial.print(":AV200625#");
      } else if (!strcmp(buffer, "CL")) {
        if (is_slewing)
          Serial.print(":CL1#");
        else
          Serial.print(":CL0#");
      } else if (!strcmp(buffer, "AT")) {
        if (is_tracking)
          Serial.print(":AT1#");
        else
          Serial.print(":AT0#");
      } else if (!strcmp(buffer, "Ct?")) {
          Serial.print(":CT");
          Serial.print(tracking_rate);
          Serial.print("#");
      } else if (!strcmp(buffer, "CtA")) {
        is_tracking = true;
      } else if (!strcmp(buffer, "CtL")) {
        is_tracking = false;
      } else if (!strcmp(buffer, "AH")) {
        if (is_parked && is_slewing)
          Serial.print(":AH1#");
        else
          Serial.print(":AH0#");
      } else if (!strcmp(buffer, "GH")) {
        if (is_parked && !is_slewing)
          Serial.print(":GH0#");
        else
          Serial.print(":GH0#");
      }
    }
  }
}
