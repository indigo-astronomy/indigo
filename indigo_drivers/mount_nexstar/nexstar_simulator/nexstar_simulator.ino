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

#define WRITE_BIN(data) Serial.write(data, sizeof(data)); Serial.write('#')
#define READ_BIN(data) Serial.readBytes(data, sizeof(data)); Serial.write('#')
#define WRITE_HEX(data) write_hex(data, sizeof(data)); Serial.write('#')

byte _location[] = { 33, 50, 41, 0, 118, 20, 17, 1 };
byte _time[] = { 15, 26, 0, 4, 6, 5, 251, 1 };
unsigned long _time_lapse = 0;
byte _aligned = 1;
byte _slewing = 0;
byte _tracking = 0;
unsigned long _ra = 0x00000000;
unsigned long _dec = 0x40000000;

struct  {
  byte size;
  byte destination;
  byte command;
  byte data1;
  byte data2;
  byte data3;
  byte response_size;
} _pass_through;

int _rates[] = { 0, 1, 2, 4, 8, 16, 64, 256, 1024, 4096 };
int _ra_rate = 0;
int _dec_rate = 0;

int _ra_guiding_rate = 0;
int _dec_guiding_rate = 0;

int _ra_backlash = 0;
int _dec_backlash = 0;


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
  _time_lapse += lapse;
  int s = _time[2] + _time_lapse / 1000;
  int m = _time[1] + s / 60;
  int h = _time[0] + m / 60;
  _time[2] = s % 60;
  _time[1] = m % 60;
  _time[0] = h % 24;
  _time_lapse %= 1000;
  _ra += _ra_rate * lapse;
  _dec += _dec_rate * lapse;
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
        WRITE_BIN(_location);
        break;
      case 'W':
        READ_BIN(_location);
        break;
      case 'h':
        WRITE_BIN(_time);
        break;
      case 'H':
        READ_BIN(_time);
        break;
      case 'm':
        Serial.write(_model);
        Serial.write('#');
        break;
      case 'J':
        Serial.write(_aligned);
        Serial.write('#');
      case 'L':
        Serial.write(_slewing);
        Serial.write('#');
        break;
      case 'M':
        _slewing = 0;
        Serial.write('#');
        break;
      case 't':
        Serial.write(_tracking);
        Serial.write('#');
        break;
      case 'T':
        _tracking = Serial.read();
        Serial.write('#');
        break;
      case 'E':
        write_hex_lo(_ra);
        Serial.write(',');
        write_hex_lo(_dec);
        Serial.write('#');
        break;
      case 'e':
        write_hex_hi(_ra);
        Serial.write(',');
        write_hex_hi(_dec);
        Serial.write('#');
        break;
      case 'S':
        _ra = read_hex_lo();
        Serial.print(_ra, HEX);
        Serial.read();
        _dec = read_hex_lo();
        Serial.write('#');
        break;
      case 's':
        _ra = read_hex_hi();
        Serial.read();
        _dec = read_hex_hi();
        Serial.write('#');
        break;
      case 'P':
        Serial.readBytes((byte *)&_pass_through, 7);
        switch (_pass_through.command) {
          case 6:
            if (_pass_through.destination == RA_AXIS)
              _ra_rate = _pass_through.data1 * 0x100 + _pass_through.data2;
            else if (_pass_through.destination == DEC_AXIS)
              _dec_rate = _pass_through.data1 * 0x100 + _pass_through.data2;
            break;
          case 7:
            if (_pass_through.destination == RA_AXIS)
              _ra_rate = -(_pass_through.data1 * 0x100 + _pass_through.data2);
            else if (_pass_through.destination == DEC_AXIS)
              _dec_rate = -(_pass_through.data1 * 0x100 + _pass_through.data2);
            break;
          case 16:
            if (_pass_through.destination == RA_AXIS)
              _ra_backlash = _pass_through.data1;
            else if (_pass_through.destination == DEC_AXIS)
              _dec_backlash = _pass_through.data1;
            break;
          case 36:
            if (_pass_through.destination == RA_AXIS)
              _ra_rate = _rates[_pass_through.data1];
            else if (_pass_through.destination == DEC_AXIS)
              _dec_rate = _rates[_pass_through.data1];
            break;
          case 37:
            if (_pass_through.destination == RA_AXIS)
              _ra_rate = -_rates[_pass_through.data1];
            else if (_pass_through.destination == DEC_AXIS)
              _dec_rate = -_rates[_pass_through.data1];
            break;
          case 64:
            if (_pass_through.destination == RA_AXIS)
              Serial.write(_ra_backlash);
            else if (_pass_through.destination == DEC_AXIS)
              Serial.write(_dec_backlash);
            break;
          case 70:
            if (_pass_through.destination == RA_AXIS)
              _ra_guiding_rate = _pass_through.data1;
            else if (_pass_through.destination == DEC_AXIS)
              _dec_guiding_rate = _pass_through.data1;
            break;
          case 71:
            if (_pass_through.destination == RA_AXIS)
              Serial.write(_ra_guiding_rate);
            else if (_pass_through.destination == DEC_AXIS)
              Serial.write(_dec_guiding_rate);
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
