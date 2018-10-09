// MJKZZ simulator for Arduino
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

#include <stdarg.h>

#ifdef ARDUINO_SAM_DUE
#define MJKZZ Serial3
#define DEBUG  Serial
#endif

#ifdef LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

#define CMD_GVER 'v' // get version
#define CMD_SCAM 'A' // activate camera for lwValue
#define CMD_SFCS 'a' // activate half pressed
#define CMD_MOVE 'M' // move by positive or negative increment 
#define CMD_SREG 'R' // set register value
#define CMD_GREG 'r' // get register value
#define CMD_SPOS 'P' // set position
#define CMD_GPOS 'p' // get position
#define CMD_SSPD 'S' // set speed
#define CMD_GSPD 's' // get speed
#define CMD_SSET 'I' // set settle time
#define CMD_GSET 'i' // get settle time
#define CMD_SHLD 'D' // set hold period
#define CMD_GHLD 'd' // get hold period
#define CMD_SBCK 'K' // set backlash
#define CMD_GBCK 'k' // get backlash
#define CMD_SLAG 'G' // set shutter lag
#define CMD_GLAG 'g' // get shutter lag
#define CMD_SSPS 'B' // set start position
#define CMD_GSPS 'b' // get start position
#define CMD_SCFG 'F' // set configuration flag
#define CMD_GCFG 'f' // get configuration flag
#define CMD_SEPS 'E' // set ending position
#define CMD_GEPS 'e' // get ending position
#define CMD_SCNT 'N' // set number of captures
#define CMD_GCNT 'n' // get number of captures
#define CMD_SSSZ 'Z' // set step size
#define CMD_GSSZ 'z' // get step size
#define CMD_EXEC 'X' // execute commands
#define CMD_STOP 'x' // stop execution of commands

typedef struct {
    uint8_t ucADD;
    uint8_t ucCMD;
    uint8_t ucIDX;
    uint8_t ucMSG[4];
    uint8_t ucSUM;
  } mjkzz_message;

uint8_t address = 0x01;
int32_t target = 0;
int32_t position = 0;
int32_t speed = 0;

char buffer[32];


static int32_t mjkzz_get_int(mjkzz_message *message) {
  return ((((((int32_t)message->ucMSG[0] << 8) + (int32_t)message->ucMSG[1]) << 8) + (int32_t)message->ucMSG[2]) << 8) + (int32_t)message->ucMSG[3];
}

static void mjkzz_set_int(mjkzz_message *message, int32_t value) {
  message->ucMSG[0] = (uint8_t)(value >> 24);
  message->ucMSG[1] = (uint8_t)(value >> 16);
  message->ucMSG[2] = (uint8_t)(value >> 8);
  message->ucMSG[3] = (uint8_t)value;
}

void setup() {
#ifdef LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("MJKZZ Simulator");
  lcd.setCursor(0, 1);
  lcd.print("Not connected");
#endif
#ifdef DEBUG
  DEBUG.begin(9600);
  DEBUG.println("MJKZZ Simulator");
#endif
  MJKZZ.begin(9600);
  MJKZZ.setTimeout(1000);
}

void loop() {
  mjkzz_message message;
  static long unsigned last_time = 0;
  long unsigned current_time = millis() / 10;
  if (last_time < current_time) {
    if (target < position)
      position--;
    else if (target > position)
      position++;
    last_time = current_time;
  }
#ifdef LCD
    sprintf(buffer, "%1d %6d %6d", speed, target, position);
    lcd.setCursor(0, 1);
    lcd.print(buffer);
#endif
  if (MJKZZ.available()) {
    MJKZZ.readBytes((byte *)&message, 8);
#ifdef LCD
    sprintf(buffer, "%c %02x %02x %02x %02x %02x", message.ucCMD, message.ucIDX, message.ucMSG[0], message.ucMSG[1], message.ucMSG[2], message.ucMSG[3]);
    lcd.setCursor(0, 0);
    lcd.print(buffer);
#endif
#ifdef DEBUG
    sprintf(buffer, "< %c %02x %02x %02x %02x %02x (%d)", message.ucCMD, message.ucIDX, message.ucMSG[0], message.ucMSG[1], message.ucMSG[2], message.ucMSG[3], mjkzz_get_int(&message));
    DEBUG.println(buffer);
#endif
    if (message.ucADD == address)
      message.ucADD |= 0x80;
    switch (message.ucCMD) {
      case CMD_GVER:
        message.ucMSG[0] = 1;
        break;
      case CMD_SCAM:
        break;
      case CMD_SFCS:
        break;
      case CMD_MOVE:
        break;
      case CMD_SREG:
        break;
      case CMD_GREG:
        break;
      case CMD_SPOS:
        target = mjkzz_get_int(&message);
        break;
      case CMD_GPOS:
        mjkzz_set_int(&message, position);
        break;
      case CMD_SSPD:
        speed = mjkzz_get_int(&message);
        break;
      case CMD_GSPD:
        mjkzz_set_int(&message, speed);
        break;
      case CMD_SSET:
        break;
      case CMD_GSET:
        break;
      case CMD_SHLD:
        break;
      case CMD_GHLD:
        break;
      case CMD_SBCK:
        break;
      case CMD_GBCK:
        break;
      case CMD_SLAG:
        break;
      case CMD_GLAG:
        break;
      case CMD_SSPS:
        break;
      case CMD_GSPS:
        break;
      case CMD_SCFG:
        break;
      case CMD_GCFG:
        break;
      case CMD_SEPS:
        break;
      case CMD_GEPS:
        break;
      case CMD_SCNT:
        break;
      case CMD_GCNT:
        break;
      case CMD_SSSZ:
        break;
      case CMD_GSSZ:
        break;
      case CMD_EXEC:
        break;
      case CMD_STOP:
        mjkzz_set_int(&message, position);
        break;
    }
    message.ucCMD |= 0x80;
    message.ucSUM = message.ucADD + message.ucCMD + message.ucIDX + message.ucMSG[0] + message.ucMSG[1] + message.ucMSG[2] + message.ucMSG[3];
    MJKZZ.write((byte *)&message, 8);
#ifdef DEBUG
    sprintf(buffer, "> %c %02x %02x %02x %02x %02x", message.ucCMD & 0x7F, message.ucIDX, message.ucMSG[0], message.ucMSG[1], message.ucMSG[2], message.ucMSG[3]);
    DEBUG.println(buffer);
#endif
  }
}
