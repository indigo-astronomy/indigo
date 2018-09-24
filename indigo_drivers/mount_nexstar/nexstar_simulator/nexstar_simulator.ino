// NexStar mount simulator for Arduino
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

#define CELESTRON
//#define SKYWATCHER

#define RA_AXIS 16
#define DEC_AXIS 17
#define GSP 176

#define WRITE_BIN(data) Serial.write(data, sizeof(data)); Serial.write('#')
#define READ_BIN(data) Serial.readBytes(data, sizeof(data)); Serial.write('#')
#define WRITE_HEX(data) write_hex(data, sizeof(data)); Serial.write('#')

byte location[] = { 33, 50, 41, 0, 118, 20, 17, 1 };
byte time[] = { 15, 26, 0, 4, 6, 5, 251, 1 };
unsigned long time_lapse = 0;
bool is_aligned = true;
bool is_slewing = false;
byte tracking_mode = 2;
unsigned long ra = 0x00000000;
unsigned long dec = 0x40000000;

unsigned long ra_target = 0x00000000;
unsigned long dec_target = 0x40000000;

int rates[] = { 0, 1, 2, 4, 8, 16, 64, 256, 1024, 4096 };
int ra_rate = 0;
int dec_rate = 0;

byte ra_guiding_rate = 0;
byte dec_guiding_rate = 0;

byte ra_backlash = 0;
byte dec_backlash = 0;

double STEP_PER_SEC = 0x1000000L / 86400.0;
long SLEW_PER_SEC = 0x10000L;

#ifdef CELESTRON
byte _version[] = { 1, 2 };
byte _model = 5;
#endif
#ifdef SKYWATCHER
byte _version[] = { 4, 37, 7 };
byte _model = 0;
#endif

void write_hex(byte *data, int length) {
  for (int i = 0; i < length; i++) {
    Serial.print(data[i] / 0x10, HEX);
    Serial.print(data[i] % 0x10, HEX);
  }
}

void write_hex_lo(unsigned long data) {
  byte buffer[] = { (byte)(data / 0x1000000), (byte)(data / 0x10000 % 0x100) };
  write_hex(buffer, sizeof(buffer));
}

void write_hex_hi(unsigned long data) {
  byte buffer[] = {  (byte)(data / 0x1000000), (byte)(data / 0x10000 % 0x100), (byte)(data / 0x100 % 0x100), (byte)(data % 0x100) };
  write_hex(buffer, sizeof(buffer));
}

byte read_hex() {
  char c = Serial.read();
  if (isDigit(c))
    return c - '0';
  return c - 'A' + 10;
}

unsigned long read_hex_lo() {
  byte a = read_hex();
  byte b = read_hex();
  byte c = read_hex();
  byte d = read_hex();
  return a * 0x10000000L + b * 0x1000000L + c * 0x100000L + d * 0x10000L;
}

unsigned long read_hex_hi() {
  byte a = read_hex();
  byte b = read_hex();
  byte c = read_hex();
  byte d = read_hex();
  byte e = read_hex();
  byte f = read_hex();
  byte g = read_hex();
  byte h = read_hex();
  return a * 0x10000000L + b * 0x1000000L + c * 0x100000L + d * 0x10000L + e * 0x1000L + f * 0x100L + g * 0x10L + h;
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  while (!Serial)
    ;
}

void loop() {
  static long unsigned last_millis = 0;
  long unsigned current_millis = millis();
  long unsigned lapse = current_millis - last_millis;
  last_millis = current_millis;
  // update time
  time_lapse += lapse;
  int s = time[2] + time_lapse / 1000;
  int m = time[1] + s / 60;
  int h = time[0] + m / 60;
  time[2] = s % 60;
  time[1] = m % 60;
  time[0] = h % 24;
  time_lapse %= 1000;
  // update position - slew
  ra += ra_rate * lapse * 50;
  dec += dec_rate * lapse * 50;
  // update position - tracking
  if (tracking_mode == 0) {
    ra = (ra + (long)(lapse * STEP_PER_SEC)) % 0x1000000L;
  }
  if (is_slewing) {
    long diff = ra_target - ra;
    if (abs(diff) < SLEW_PER_SEC * lapse) {
      ra = ra_target;
    } else {
      ra = ra + (diff > 0 ? SLEW_PER_SEC : -SLEW_PER_SEC) * lapse;
    }
    diff = dec_target - dec;
    if (abs(diff) < SLEW_PER_SEC * lapse) {
      dec = dec_target;
    } else {
      dec = dec + (diff > 0 ? SLEW_PER_SEC : -SLEW_PER_SEC) * lapse;
    }
    if (ra == ra_target && dec == dec_target)
      is_slewing = false;
  }
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'K':
        Serial.write(Serial.read());
        break;
      case 'V':
        #ifdef CELESTRON
        WRITE_BIN(_version);
        #endif
        #ifdef SKYWATCHER
          WRITE_HEX(_version);
        #endif
        break;
      case 'w':
        WRITE_BIN(location);
        break;
      case 'W':
        READ_BIN(location);
        break;
      case 'h':
        WRITE_BIN(time);
        break;
      case 'H':
        READ_BIN(time);
        break;
      case 'm':
        Serial.write(_model);
        Serial.write('#');
        break;
      case 'J':
        Serial.write(is_aligned ? 1 : 0);
        Serial.write('#');
        break;
      case 'L':
        Serial.write(is_slewing ? '1' : '0');
        Serial.write('#');
        break;
      case 'M':
        is_slewing = false;
        Serial.write('#');
        break;
      case 't':
        Serial.write(tracking_mode);
        Serial.write('#');
        break;
      case 'T':
        tracking_mode = Serial.read();
        Serial.write('#');
        break;
      case 'E':
        write_hex_lo(ra);
        Serial.write(',');
        write_hex_lo(dec);
        Serial.write('#');
        break;
      case 'e':
        write_hex_hi(ra);
        Serial.write(',');
        write_hex_hi(dec);
        Serial.write('#');
        break;
      case 'S':
        ra = read_hex_lo();
        Serial.print(ra, HEX);
        Serial.read();
        dec = read_hex_lo();
        Serial.write('#');
        is_slewing = false;
        break;
      case 's':
        ra = read_hex_hi();
        Serial.read();
        dec = read_hex_hi();
        Serial.write('#');
        is_slewing = false;
        break;
      case 'R':
        ra_target = read_hex_lo();
        Serial.print(ra, HEX);
        Serial.read();
        dec_target = read_hex_lo();
        Serial.write('#');
        is_slewing = true;
        break;
      case 'r':
        ra_target = read_hex_hi();
        Serial.read();
        dec_target = read_hex_hi();
        Serial.write('#');
        is_slewing = true;
        break;
			case 'P': {
				struct  {
					byte size;
					byte destination;
					byte command;
					byte data1;
					byte data2;
					byte data3;
					byte response_size;
				} pass_through;
        Serial.readBytes((byte *)&pass_through, 7);
        switch (pass_through.command) {
          case 6:
            if (pass_through.destination == RA_AXIS)
              ra_rate = pass_through.data1 * 0x100 + pass_through.data2;
            else if (pass_through.destination == DEC_AXIS)
              dec_rate = pass_through.data1 * 0x100 + pass_through.data2;
            break;
          case 7:
            if (pass_through.destination == RA_AXIS)
              ra_rate = -(pass_through.data1 * 0x100 + pass_through.data2);
            else if (pass_through.destination == DEC_AXIS)
              dec_rate = -(pass_through.data1 * 0x100 + pass_through.data2);
            break;
          case 16:
            if (pass_through.destination == RA_AXIS)
              ra_backlash = pass_through.data1;
            else if (pass_through.destination == DEC_AXIS)
              dec_backlash = pass_through.data1;
            break;
          case 36:
            if (pass_through.destination == RA_AXIS)
              ra_rate = rates[pass_through.data1];
            else if (pass_through.destination == DEC_AXIS)
              dec_rate = rates[pass_through.data1];
            break;
          case 37:
            if (pass_through.destination == RA_AXIS)
              ra_rate = -rates[pass_through.data1];
            else if (pass_through.destination == DEC_AXIS)
              dec_rate = -rates[pass_through.data1];
            break;
          case 64:
            if (pass_through.destination == RA_AXIS)
              Serial.write(ra_backlash);
            else if (pass_through.destination == DEC_AXIS)
              Serial.write(dec_backlash);
            break;
          case 70:
            if (pass_through.destination == RA_AXIS)
              ra_guiding_rate = pass_through.data1;
            else if (pass_through.destination == DEC_AXIS)
              dec_guiding_rate = pass_through.data1;
            break;
          case 71:
            if (pass_through.destination == RA_AXIS)
              Serial.write(ra_guiding_rate);
            else if (pass_through.destination == DEC_AXIS)
              Serial.write(dec_guiding_rate);
            break;
          case 254:
            Serial.write(1);
            Serial.write(2);
            break;
        }
        Serial.write('#');
        break;
			}
    } 
  }
}
