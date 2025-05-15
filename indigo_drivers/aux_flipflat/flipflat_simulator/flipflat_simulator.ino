// Optec/Alnitak Flip-Flat Box simulator for Arduino
//
// Copyright (c) 2019-2025 CloudMakers, s. r. o.
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
#endif
#define DISPLAY_INIT()
#define DISPLAY_BEGIN()
#define DISPLAY_TEXTF(line, ...)
#define DISPLAY_TEXT(line, ...)
#define DISPLAY_END()
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
#define DISPLAY_INIT() Heltec.begin(true, false, true); Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT); Heltec.display->setFont(ArialMT_Plain_16)
#define DISPLAY_BEGIN() Heltec.display->clear()
#define DISPLAY_TEXT(line, text) Heltec.display->drawString(0, 16 * line, text)
#define DISPLAY_TEXTF(line, format, ...) sprintf(__buffer__, format, __VA_ARGS__); Heltec.display->drawString(0, 16 * line, __buffer__)
#define DISPLAY_END() Heltec.display->display()
#endif

// 10 = Flat-Man_XL
// 15 = Flat-Man_L
// 19 = Flat-Man
// 98 = Flip-Mask/Remote Dust Cover
// 99 = Flip-Flat.

#define DEVICE "99"

char brightness[4] = "128";
short cover = 1;
short light = 0;
long motor = 0;

void setup() {
  DISPLAY_INIT();
  DISPLAY_BEGIN();
  DISPLAY_TEXT(0, "FlipFlat simulator");
  DISPLAY_END();
  Serial.begin(9600);
  while (!Serial)
    ;
}

void loop() {
  static long unsigned last_millis = 0;
  long unsigned current_millis = millis();
  if (motor > 0) {
    long unsigned timelapse = current_millis - last_millis;
    if (motor < timelapse)
      motor = 0;
    else
      motor -= timelapse;
  }
  last_millis = current_millis;
  if (Serial.available()) {
    char buffer[32];
    memset(buffer, 0, sizeof(buffer));
    Serial.readBytesUntil('\r', buffer, sizeof(buffer));
    if (!strcmp(buffer, ">POOO")) {
      Serial.print("*P" DEVICE "OOO\n");
    } else if (!strcmp(buffer, ">OOOO")) {
      motor = 2000;
      cover = 2;
      Serial.print("*O" DEVICE "OOO\n");
    } else if (!strcmp(buffer, ">COOO")) {
      motor = 2000;
      cover = 1;
      Serial.print("*C" DEVICE "OOO\n");
    } else if (!strcmp(buffer, ">LOOO")) {
      light = 1;
      Serial.print("*L" DEVICE "OOO\n");
    } else if (!strcmp(buffer, ">DOOO")) {
      light = 0;
      Serial.print("*D" DEVICE "OOO\n");
    } else if (!strncmp(buffer, ">B", 2)) {
      strncpy(brightness, buffer + 2, 3);
      Serial.print("*B" DEVICE);
      Serial.print(brightness);
      Serial.print("\n");
    } else if (!strcmp(buffer, ">JOOO")) {
      Serial.print("*J" DEVICE);
      Serial.print(brightness);
      Serial.print("\n");
    } else if (!strcmp(buffer, ">SOOO")) {
      Serial.print("*S" DEVICE);
      Serial.print(motor > 0 ? 1 : 0);
      Serial.print(light);
      Serial.print(motor > 0 ? 0 : cover);
      Serial.print("\n");
    } else if (!strcmp(buffer, ">VOOO")) {
      Serial.print("*V" DEVICE "123\n");
    }
    DISPLAY_BEGIN();
    DISPLAY_TEXTF(0, "motor = %ld", motor);
    DISPLAY_TEXTF(1, "cover = %d", cover);
    DISPLAY_TEXTF(2, "light = %d %s", light, brightness);
    DISPLAY_END();
  }
}
