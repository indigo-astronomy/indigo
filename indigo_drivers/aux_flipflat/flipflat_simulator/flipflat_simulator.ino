// Optec/Alnitak Flip-Flat Box simulator for Arduino
//
// Copyright (c) 2019 CloudMakers, s. r. o.
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

#define TFT
#ifdef TFT
#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif

//#define OLED
#ifdef OLED
#include "heltec.h"
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
#ifdef TFT
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("FlipFlat simulator", 0, 0);
#endif
#ifdef OLED
  Heltec.begin(true, false, true);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 0, "FlipFlat simulator");
  Heltec.display->display();
#endif
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
  char buffer[32];
  if (Serial.available()) {
    memset(buffer, 0, sizeof(buffer));
    Serial.readBytesUntil('\n', buffer, sizeof(buffer));
    if (!strcmp(buffer, ">P000")) {
      Serial.println("*P" DEVICE "000");
    } else if (!strcmp(buffer, ">O000")) {
      motor = 2000;
      cover = 2;
      Serial.println("*O" DEVICE "000");
    } else if (!strcmp(buffer, ">C000")) {
      motor = 2000;
      cover = 1;
      Serial.println("*C" DEVICE "000");
    } else if (!strcmp(buffer, ">L000")) {
      light = 1;
      Serial.println("*L" DEVICE "000");
    } else if (!strcmp(buffer, ">D000")) {
      light = 0;
      Serial.println("*D" DEVICE "000");
    } else if (!strncmp(buffer, ">B", 2)) {
      strncpy(brightness, buffer + 2, 3);
      Serial.print("*B" DEVICE);
      Serial.println(brightness);
    } else if (!strcmp(buffer, ">J000")) {
      Serial.print("*J" DEVICE);
      Serial.println(brightness);
    } else if (!strcmp(buffer, ">S000")) {
      Serial.print("*S" DEVICE);
      Serial.print(motor > 0 ? 1 : 0);
      Serial.print(light);
      Serial.println(motor > 0 ? 0 : cover);
    } else if (!strcmp(buffer, ">V000")) {
      Serial.println("*V" DEVICE "123");
    }
#ifdef TFT
    tft.fillScreen(TFT_BLACK);
    sprintf(buffer, "motor = %ld", motor);
    tft.drawString(buffer, 0, 0);
    sprintf(buffer, "cover = %d", cover);
    tft.drawString(buffer, 0, 32);
    sprintf(buffer, "light = %d %s", light, brightness);
    tft.drawString(buffer, 0, 64);
#endif    
#ifdef OLED
    Heltec.display->clear();
    sprintf(buffer, "motor = %ld", motor);
    Heltec.display->drawString(0, 0, buffer);
    sprintf(buffer, "cover = %d", cover);
    Heltec.display->drawString(0, 20, buffer);
    sprintf(buffer, "light = %d %s", light, brightness);
    Heltec.display->drawString(0, 40, buffer);
    Heltec.display->display();
#endif
  }
}
