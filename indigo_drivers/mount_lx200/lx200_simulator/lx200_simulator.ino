// LX200 simulator for Arduino
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

#define DEC_PER_SEC 1440
#define RA_PER_SEC 96

bool is_meade = true;
bool is_10micron = false;
bool is_gemini = false;
bool is_avalon = false;
bool is_onstep = false;
bool is_zwo = false;
bool is_nyx = false;

int date_day = 1;
int date_month = 1;
int date_year = 2018;

int time_hour = 22;
int time_minute = 15;
int time_second = 30;
int time_offset = 2;
int time_dst = 1;

char latitude[16] = "+48*08";
char longitude[16] = "-17*06";

bool high_precision = true;

long ra = 0;
long dec = 90L * 360000L;

long target_ra = 0;
long target_dec = 90L * 360000L;

int ra_slew = 0;
int dec_slew = 0;

char tracking_rate = 'Q';
char slew_rate = 'M';
char pec = 'N';

bool is_slewing = false;
bool is_tracking = false;
bool is_parked = false;

char ra_guiding_speed[3] = "50";
char dec_guiding_speed[3] = "50";

int focuser_position = 0;
int focuser_move = 0;
int focuser_speed = 1;

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
      is_tracking = !is_parked;
    }
  } else if (!is_tracking) {
    ra = (ra + lapse) % (24L * 60L * 60L);
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
    ra = (ra + 24L * 360000L) % (24L * 360000L);
    dec += dec_slew * rate;
    if (dec < -90L * 360000L)
      dec = -90L * 360000L; 
    if (dec > 90L * 360000L)
      dec = 90L * 360000L; 
  }
  focuser_position += focuser_move * lapse;
  int s = time_second + time_lapse / 1000;
  int m = time_minute + s / 60;
  int h = time_hour + m / 60;
  time_second = s % 60;
  time_minute = m % 60;
  time_hour = h % 24;
  time_lapse %= 1000;
  static unsigned long last_display = 0;
  if (time_lapse < last_display) {
    DISPLAY_BEGIN();
    DISPLAY_TEXTF(0, "%02d%02d%02d %02d%02d%02d %c%c", ra / 360000L, (ra / 6000L) % 60, (ra / 100L) % 60, dec / 360000L, (abs(dec) / 6000L) % 60, (abs(dec) / 100L) % 60, is_tracking ? 'T' : 't', is_slewing ? 'S' : 's');
    DISPLAY_TEXTF(1, "%02d%02d%02d %c %c%c %c %c%c", time_hour, time_minute, time_second, slew_rate, (ra_slew < 0 ? 'E' : ra_slew > 0 ? 'W' : '0'), (dec_slew < 0 ? 'S' : dec_slew > 0 ? 'N' : '0'), tracking_rate, high_precision ? 'H' : 'h', is_parked ? 'P' : 'p');
    DISPLAY_END();
  }
  last_display = time_lapse;
}

void setup() {
  DISPLAY_INIT();
  DISPLAY_BEGIN();
  DISPLAY_TEXT(0, "LX200 simulator");
  DISPLAY_END();
  if (is_nyx || is_onstep) {
    Serial.begin(115200);
  } else {
    Serial.begin(9600);
  }
  Serial.setTimeout(1000);
  if (is_onstep) {
    Serial.print(
      "MSG: OnStep 4.24j\n"
      "MSG: MCU =\n"
      "ESP32, Pinmap = ESP32 Essential v1\n"
      "MSG: Init HAL\n"
      "MSG: Init serial\n"
      "MSG: Init pins\n"
      "MSG: Init TLS\n"
      "MSG: Start NV 4096 Bytes\n"
      "MSG: Init NV Axis1 defaults\n"
      "MSG: Init NV Axis2 defaults\n"
      "MSG: Init NV Axis3 defaults\n"
      "MSG: Init NV Axis4 defaults\n"
      "MSG: Init NV Axis5 defaults\n"
      "MSG: Read NV settings\n"
      "MSG: Init startup settings\n"
      "MSG: Init library/catalogs\n"
      "MSG: Init guiding\n"
      "MSG: Init weather\n"
      "MSG: Init sidereal timer\n"
      "MSG: Init motor timers\n"
      "MSG: Axis1/2 stepper drivers enabled\n"
      "MSG: Axis1/2 stepper drivers disabled\n"
      "MSG: Serial buffer flush\n"
      "MSG: OnStep is ready\n");
  }
  if (is_10micron) {
    is_tracking = false;
    is_parked = true;
  }
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
      if (!strcmp(buffer, "GVP")) {
        if (is_meade)
          Serial.print("Autostar#");
        else if (is_10micron)
          Serial.print("10micron GM1000HPS#");
        else if (is_gemini)
          Serial.print("Losmandy Gemini#");
        else if (is_avalon)
					Serial.print("Avalon#");
				else if (is_onstep)
					Serial.print("On-Step#");
				else if (is_zwo)
					Serial.print("AM5#");
        else if (is_nyx)
          Serial.print("NYX-101#");
			} else if (!strcmp(buffer, "GV")) {
				Serial.print("1.0.0#");
      } else if (!strcmp(buffer, "GU")) {
        Serial.print("G");
        Serial.print(is_tracking ? "" : "n");
        Serial.print(is_slewing ? "" : "N");        
        Serial.print("#");
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
        if (is_onstep || is_zwo)
          Serial.print("1");
        else
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
      } else if (!strncmp(buffer, "SH", 2)) {
        time_dst = atoi(buffer + 2);
      } else if (!strcmp(buffer, "GH")) {
				sprintf(buffer, "%d#", time_dst);
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
				Serial.print(longitude);
				Serial.print("#");
      } else if (!strncmp(buffer, "St", 2)) {
        strcpy(latitude, buffer + 2);
				Serial.print("1");
      } else if (!strcmp(buffer, "Gt")) {
				Serial.print(latitude);
				Serial.print("#");
      } else if (!strcmp(buffer, "U0")) {
				high_precision = false;
      } else if (!strcmp(buffer, "U1")) {
				high_precision = true;
      } else if (!strcmp(buffer, "U")) {
        high_precision = !high_precision;
      } else if (!strcmp(buffer, "P")) {
        if (high_precision) {
          Serial.print("LOW  PRECISION");
          if (is_meade)
            high_precision = false;
        } else {
          Serial.print("HIGH PRECISION");
          if (is_meade)
            high_precision = true;
        }
      } else if (!strncmp(buffer, "Sr", 2)) {
        if  (buffer[7] == '.') {
          target_ra = atol(buffer + 2) * 360000L + atof(buffer + 5) * 6000L;
        } else {
          target_ra = atol(buffer + 2) * 360000L + atol(buffer + 5) * 6000L + atol(buffer + 8) * 100L;
        }
        Serial.print("1");
      } else if (!strcmp(buffer, "Gr")) {
        if (high_precision) {
          sprintf(buffer, "%02ld:%02ld:%02ld#", target_ra / 360000L, (target_ra / 6000L) % 60, (target_ra / 100L) % 60);
        } else {
          sprintf(buffer, "%02ld:%04.1f#", target_ra / 360000L, ((target_ra / 600L) % 600) / 10.0);
        }
        Serial.print(buffer);
      } else if (!strncmp(buffer, "GR", 2)) {
        if (high_precision) {
          sprintf(buffer, "%02ld:%02ld:%02ld#", ra / 360000L, (ra / 6000L) % 60, (ra / 100L) % 60);
        } else {
          sprintf(buffer, "%02ld:%04.1f#", ra / 360000L, ((ra / 600L) % 600) / 10.0);
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
          sprintf(buffer, "%+03ld*%02ld'%02ld#", target_dec / 360000L, (abs(target_dec) / 6000L) % 60, (abs(target_dec) / 100L) % 60);
        } else {
          sprintf(buffer, "%+03ld*%0l2d#", target_dec / 360000L, (abs(target_dec) / 600L) % 60);
        }
        Serial.print(buffer);
      } else if (!strncmp(buffer, "GD", 2)) {
				if (high_precision) {
          sprintf(buffer, "%+03ld*%02ld'%02ld#", dec / 360000L, (abs(dec) / 6000L) % 60, (abs(dec) / 100L) % 60);
        } else {
          sprintf(buffer, "%+03ld*%02ld#", dec / 360000L, (abs(dec) / 600L) % 60);
        }
        Serial.print(buffer);
      } else if (!strcmp(buffer, "CM")) {
        ra = target_ra;
        dec = target_dec;
        is_parked = false;
        Serial.print("M31 EX GAL MAG 3.5 SZ178.0'#");
      } else if (!strcmp(buffer, "MS")) {
        is_slewing = true;
        is_parked = false;
				Serial.print("0");
      } else if (!strcmp(buffer, "D")) {
        if (is_slewing)
          Serial.print("*#");
        else
          Serial.print("#");
      } else if (!strcmp(buffer, "Q")) {
        is_slewing = false;
        ra_slew = 0;
        dec_slew = 0;
      } else if (!strcmp(buffer, "AP") || !strncmp(buffer, "192", 3) || !strcmp(buffer, "X122")) {
        is_tracking = true;
      } else if (!strcmp(buffer, "AL") || !strncmp(buffer, "191", 3) || !strcmp(buffer, "X120")) {
        is_tracking = false;
      } else if (!strcmp(buffer, "GW")) {
        if (is_tracking)
          Serial.print("PT1#");
        else if (is_parked)
          Serial.print("PNP#");         
        else
          Serial.print("PN1#");         
      } else if (!strcmp(buffer, "Gstat")) {
        if (is_slewing && is_parked)
          Serial.print("2#");         
        else if (is_parked)
          Serial.print("5#");         
        else if (is_slewing)
          Serial.print("6#");         
        else if (is_tracking)
          Serial.print("0#");
        else
          Serial.print("7#");
      } else if (!strcmp(buffer, "Gv")) {
        if (is_slewing)
          Serial.print("S");
        else if (is_tracking)
          Serial.print("T");
        else
          Serial.print("N");
			} else if (!strcmp(buffer, "X34")) {
        if (is_slewing)
          Serial.print("m55#");
        else if (is_tracking)
          Serial.print("m11#");
        else
          Serial.print("m00#");
      } else if (!strcmp(buffer, "X38")) {
        if (is_parked) {
          if (is_slewing)
            Serial.print("pB#");
          else
            Serial.print("p2#");
        } else {
          Serial.print("p0#");
        }
      } else if (!strcmp(buffer, "hP") || !strcmp(buffer, "hC") || !strcmp(buffer, "X362") || !strcmp(buffer, "Ch") || !strcmp(buffer, "KA")) {
        target_ra = 0;
        target_dec = 90L * 360000L;
        is_tracking = false;
        is_slewing = true;
        is_parked = true;
      } else if (!strcmp(buffer, "PO") || !strcmp(buffer, "hW") || !strcmp(buffer, "X370")) {
        is_tracking = true;
        is_slewing = false;
        is_parked = false;
      } else if (!strcmp(buffer, "h?")) {
        if (is_parked) {
          if (is_slewing)
            Serial.print("2");
          else
            Serial.print("1");
        } else {
          Serial.print("0");
        }
      } else if (!strcmp(buffer, "TQ") || !strncmp(buffer, "130:131", 7)) {
        tracking_rate = 'Q';
      } else if (!strcmp(buffer, "TS") || !strcmp(buffer, "TSOLAR") || !strncmp(buffer, "130:134", 7)) {
        tracking_rate = 'S';
      } else if (!strcmp(buffer, "TL") || !strncmp(buffer, "130:135", 7)) {
        tracking_rate = 'L';
      } else if (!strcmp(buffer, "TM")) {
        tracking_rate = 'M';
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
      } else if (!strcmp(buffer, "Qn")) {
        dec_slew = 0;
      } else if (!strcmp(buffer, "Ms")) {
        dec_slew = -1;
      } else if (!strcmp(buffer, "Qs")) {
        dec_slew = 0;
      } else if (!strcmp(buffer, "Mw")) {
        ra_slew = 1;
      } else if (!strcmp(buffer, "Qw")) {
        ra_slew = 0;
      } else if (!strcmp(buffer, "Me")) {
        ra_slew = -1;
      } else if (!strcmp(buffer, "Qe")) {
        ra_slew = 0;
      } else if (!strcmp(buffer, "F+")) {
        focuser_move = focuser_speed;
      } else if (!strcmp(buffer, "F-")) {
        focuser_move = -focuser_speed;
      } else if (!strcmp(buffer, "FS")) {
        focuser_speed = 1;
      } else if (!strcmp(buffer, "FF")) {
        focuser_speed = 5;
      } else if (!strcmp(buffer, "FQ")) {
        focuser_move = 0;
      } else if (!strcmp(buffer, "FP")) {
        sprintf(buffer, "%d#", focuser_position);
        Serial.print(buffer);
      } else if (!strcmp(buffer, "AT")) {
        if (is_tracking)
          Serial.print(":AT1#");
        else
          Serial.print(":AT0#");
      } else if (!strncmp(buffer, "Ck", 2)) {
        ra = atof(buffer + 2) * 360000L;
        dec = atof(buffer + 9) * 360000L;
      } else if (!strncmp(buffer, "X20", 3)) {
        strncpy(ra_guiding_speed, buffer + 3, 2);
				Serial.print("1");
      } else if (!strncmp(buffer, "X21", 3)) {
        strncpy(dec_guiding_speed, buffer + 3, 2);
				Serial.print("1");
      } else if (!strcmp(buffer, "X22")) {
        Serial.print(ra_guiding_speed);
        Serial.print("b");
        Serial.print(dec_guiding_speed);
        Serial.print("#");
      } else if (!strcmp(buffer, "X361")) {
        target_ra = 0;
        target_dec = 90L * 360000L;
        is_tracking = false;
        is_slewing = true;
        is_parked = false;
        Serial.print("pA#");
      } else if (!strcmp(buffer, "Gm")) {
        Serial.print("N#");
      } else if (!strcmp(buffer, "$QZ?")) {
        Serial.print(pec);
        Serial.print("#");
      } else if (!strcmp(buffer, "$QZ+")) {
        pec = 'P';
      } else if (!strcmp(buffer, "$QZ-")) {
        pec = 'I';
      }
    }
  }
}
