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

#define DEC_PER_SEC 720
#define RA_PER_SEC 48

int date_day = 1;
int date_month = 1;
int date_year = 2018;

int time_hour = 22;
int time_minute = 15;
int time_second = 30;
int time_offset = 2;

char latitude[] = "+48*08";
char longitude[] = "+17*06";

bool high_precision = false;

long ra = 0;
long dec = 90L * 360000L;

long target_ra = 0;
long target_dec = 90L * 360000L;

bool is_slewing = false;
bool is_tracking = true;
bool is_parked = false;

void write_s(char *value) {
  Serial.write(value);
}

void write_u2(int value) {
  Serial.write('0' + (value / 10) % 10);
  Serial.write('0' + value % 10);
}

void write_s2(int value) {
  if (value < 0)
    Serial.write('-');
  else
    Serial.write('+');
  write_u2(abs(value));
}

void write_f2_1(double value) {
  int i = (int)value;
  Serial.write('0' + ((int)value / 10) % 10);
  Serial.write('0' + (int)value % 10);
  Serial.write('.');
  Serial.write('0' + ((int)value * 10) % 10);
}

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
}

void setup() {
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
      write_s("P");
    } else if (ch == ':') {
      char command[64];
      memset(command, 0, sizeof(command));
      Serial.readBytesUntil('#', command, sizeof(command));
      if (!strcmp(command, "GVP")) {
         write_s("Autostar#");
      } else if (!strcmp(command, "GVF")) {
         write_s("ETX Autostar|A|43Eg|Apr 03 2007@11:25:53#");
      } else if (!strcmp(command, "GVN")) {
         write_s("43Eg#");
      } else if (!strcmp(command, "GVD")) {
         write_s("Apr 03 2007#");
      } else if (!strcmp(command, "GVT")) {
         write_s("11:25:53#");
      } else if (!strncmp(command, "SC", 2)) {
         date_day = atoi(command + 5);
         date_month = atoi(command + 2);
         date_year = 2000 + atoi(command + 8);
         write_s("1Updating planetary data#                        #");
      } else if (!strcmp(command, "GC")) {
        write_u2(date_month); write_s("/"); write_u2(date_day); write_s("/"); write_u2(date_year % 100); write_s("#");
      } else if (!strncmp(command, "SG", 2)) {
         time_offset = atoi(command + 2);
         write_s("1");
      } else if (!strcmp(command, "GG")) {
        write_u2(time_offset); write_s("#");
      } else if (!strncmp(command, "SL", 2)) {
         time_hour = atoi(command + 2);
         time_minute = atoi(command + 5);
         time_second = atoi(command + 8);
         write_s("1");
      } else if (!strcmp(command, "GL")) {
        write_u2(time_hour); write_s(":"); write_u2(time_minute); write_s(":"); write_u2(time_second); write_s("#");
      } else if (!strncmp(command, "Sg", 2)) {
        strcpy(longitude, command + 2);
         write_s("1");
      } else if (!strcmp(command, "Gg")) {
        write_s(longitude); write_s("#");
      } else if (!strncmp(command, "St", 2)) {
        strcpy(latitude, command + 2);
         write_s("1");
      } else if (!strcmp(command, "Gt")) {
        write_s(latitude); write_s("#");
      } else if (!strcmp(command, "P")) {
        if (high_precision) {
          write_s("LOW  PRECISION");
          high_precision = false;
        } else {
          write_s("HIGH PRECISION");
          high_precision = true;
        }
      } else if (!strncmp(command, "Sr", 2)) {
        if  (command[7] == '.') {
          target_ra = atol(command + 2) * 360000L + atof(command + 5) * 60000L;
        } else {
          target_ra = atol(command + 2) * 360000L + atol(command + 5) * 6000L + atol(command + 8) * 100L;
        }
        write_s("1");
      } else if (!strcmp(command, "Gr")) {
        if (high_precision) {
          write_u2(target_ra / 360000L); write_s(":"); write_u2((target_ra / 6000L) % 60); write_s(":"); write_u2((target_ra / 100L) % 60); write_s("#");
        } else {
          write_u2(target_ra / 360000L); write_s(":"); write_f2_1(((target_ra / 600L) % 600) / 10.0); write_s("#");
        }
      } else if (!strcmp(command, "GR")) {
        if (high_precision) {
          write_u2(ra / 360000L); write_s(":"); write_u2((ra / 6000L) % 60); write_s(":"); write_u2((ra / 100L) % 60); write_s("#");
        } else {
          write_u2(ra / 360000L); write_s(":"); write_f2_1(((ra / 600L) % 600) / 10.0); write_s("#");
        }
      } else if (!strncmp(command, "Sd", 2)) {
        if  (command[8] == 0) {
          target_dec = atol(command + 2) * 360000L;
          if (target_dec < 0)
            target_dec -= atol(command + 6) * 6000L;
          else
            target_dec -= atol(command + 6) * 6000L;
        } else {
          target_dec = atol(command + 2) * 360000L;
          if (target_dec < 0)
            target_dec -= atol(command + 6) * 6000L + atol(command + 9) * 100L;
          else
            target_dec -= atol(command + 6) * 6000L + atol(command + 9) * 100L;
        }
        write_s("1");
      } else if (!strcmp(command, "Gd")) {
        if (high_precision) {
          write_s2(target_dec / 360000L); write_s("*"); write_u2((abs(target_dec) / 6000L) % 60); write_s("'"); write_u2((abs(target_dec) / 100L) % 60); write_s("#");
        } else {
          write_s2(target_dec / 360000L); write_s("*"); write_f2_1(((abs(target_dec) / 600L) % 600) / 10.0); write_s("#");
        }
      } else if (!strcmp(command, "GD")) {
        if (high_precision) {
          write_s2(dec / 360000L); write_s("*"); write_u2((abs(dec) / 6000L) % 60); write_s("'"); write_u2((abs(dec) / 100L) % 60); write_s("#");
        } else {
          write_s2(dec / 360000L); write_s("*"); write_f2_1(((abs(dec) / 600L) % 600) / 10.0); write_s("#");
        }
      } else if (!strcmp(command, "CM")) {
        ra = target_ra;
        dec = target_dec;
        write_s("M31 EX GAL MAG 3.5 SZ178.0'#");
      } else if (!strcmp(command, "MS")) {
        is_slewing = true;
        write_s("0");
      } else if (!strcmp(command, "Q")) {
        is_slewing = false;
      } else if (!strcmp(command, "AP")) {
        is_tracking = true;
      } else if (!strcmp(command, "AL")) {
        is_tracking = false;
      } else if (!strcmp(command, "GW")) {
        if (is_tracking)
          write_s("PT1#");
        else if (is_parked)
          write_s("PNP#");         
        else
          write_s("PN1#");         
      } else if (!strcmp(command, "TQ")) {
      } else if (!strcmp(command, "TS")) {
      } else if (!strcmp(command, "TL")) {
      } else if (!strcmp(command, "TM")) {
      }
    }
  }
}
