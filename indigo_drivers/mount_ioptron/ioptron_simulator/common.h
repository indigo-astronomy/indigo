// iOptron simulator for Arduino
//
// Copyright (c) 2018-2021 CloudMakers, s. r. o.
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

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


#endif

int rates[] = { 1, 2, 8, 16, 64, 128, 256, 512, 1024 };

unsigned long current_ra = 0, target_ra = 0, park_ra = 0; // in milliarcseconds
long current_dec = 90L * 60L * 60L * 1000L, target_dec = 90L * 60L * 60L * 1000L, park_dec = 90L * 60L * 60L * 1000L; // in milliarcseconds

int slew_rate = 20000L;
int dec_rate = 0;
int ra_rate = 0;

bool parked = false;
bool parking = false;
bool tracking = false;
bool slewing = false;

long base_time = 0;

char *format60(long value, const char *format) {
  static char buffer[16];
  int sign = value >= 0 ? 1 : -1;
  value = abs(value);
  int sec = value % 60;
  value /= 60;
  int min = value % 60;
  value /= 60;
  sprintf(buffer, format, sign * (int)value, min, sec);
  return buffer;
}

long parse60(char *buffer, const char *format) {
  int value, min, sec;
  sscanf(buffer, format, &value, &min, &sec);
  if (value < 0) {
    return value * 3600L - min * 60L - sec;
  }
  return value * 3600L + min * 60L + sec;
}

char *formatLong(long value, bool sign, int length) {
  static char _buffer[16];
  char *buffer = _buffer;
  buffer[length] = 0;
  if (sign) {
    if (value >= 0) {
      *buffer++ = '+';
    } else {
      *buffer++ = '-';
      value = -value;
    }
    length--;
  }
  while (length > 0) {
    buffer[--length] = value % 10 + '0';
    value /= 10;
  }
  return _buffer;
}

long parseLong(const char *buffer, int length) {
  long value = 0;
  int sign = 1;
  if (*buffer == '+') {
    buffer++;
    length--;
  } else if (*buffer == '-') {
    buffer++;
    length--;
    sign = -1;
  }
  while (length > 0) {
    value = value * 10 + *buffer++ - '0';
    length--;
  }
  return value * sign;
}

void parseCommand();

void setup() {
#ifdef LCD
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("iOptron " VERSION);
	lcd.setCursor(0, 1);
	lcd.print("Not connected");
#endif
	Serial.begin(9600);
	Serial.setTimeout(1000);
	while (!Serial)
		;
}

void loop() {
  static long unsigned last_millis = 0;
  long unsigned current_millis = millis();
  long lapse = current_millis - last_millis;
  long ra_diff = target_ra - current_ra;
  long dec_diff = target_dec - current_dec;
  if (parked) {
  } else if (slewing) {
    slewing = false;
    if (abs(ra_diff) <= slew_rate * lapse) {
      current_ra = target_ra;
    } else {
      current_ra = current_ra + (ra_diff > 0 ? slew_rate : -slew_rate) * lapse;
      slewing = true;
    }
    if (abs(dec_diff) <= slew_rate * lapse) {
      current_dec = target_dec;
    } else {
      current_dec = current_dec + (dec_diff > 0 ? slew_rate : -slew_rate) * lapse;
      slewing = true;
    }
    if (!slewing && parking) {
      parking = false;
      parked = true;
    }
  } else {
    if (!tracking)
      current_ra = (current_ra + 15 * lapse) % (360L * 60L * 60L * 1000L);
    if (ra_rate)
      current_ra = (current_ra + 15 * ra_rate * lapse) % (360L * 60L * 60L * 1000L);
    if (dec_rate) {
      current_dec = (current_dec + 15 * dec_rate * lapse);
      if (current_dec < -90L * 60L * 60L * 1000L) {
        current_dec = -90L * 60L * 60L * 1000L;
        dec_rate = 0;
      } else if (current_dec > 90L * 60L * 60L * 1000L) {
        current_dec = 90L * 60L * 60L * 1000L;
        dec_rate = 0;
      }
    }
  }
  last_millis = current_millis;
  
#ifdef LCD
	lcd.setCursor(0, 0);
	lcd.print(format60(current_ra / 15 / 1000, "%02d%02d%02d"));
  lcd.print(format60(current_dec / 1000, "%+03d%02d%02d"));
  lcd.print(tracking ? "T" : " ");
  lcd.print(slewing ? "S" : " ");
  lcd.print(parked ? "P" : (parking ? "p" : " "));
  lcd.setCursor(0, 1);
  lcd.print(format60(target_ra / 15 / 1000, "%02d%02d%02d"));
  lcd.print(format60(target_dec / 1000, "%+03d%02d%02d"));
  lcd.print(" ");
  lcd.print(ra_diff > 0 ? "+" : ra_diff < 0 ? "-" : " ");
  lcd.print(dec_diff > 0 ? "+" : dec_diff < 0 ? "-" : " ");
#endif
	if (Serial.available()) {
		parseCommand();
	}
}
